// Project Nuclidean Source File
#include <engine/network/client.h>
#include <engine/network/server.h>

#include <common.h>
#include <logging.h>
#include <engine/network/constants.h>
#include <engine/network/protocol.h>
#include <engine/network/tcp_socket.h>

namespace nc::net
{

//==============================================================================
Server::Server(IPv4Address address, u16 port, u32 expected_player_count)
  : m_expected_player_count(expected_player_count)
{
  const auto maybe_socket = create_socket(address, port);
  if (!maybe_socket)
  {
    nc_crit("[net][server] failed to create listen socket");
    return;
  }
  m_listen_socket = *maybe_socket;

  if (!start_listening(m_listen_socket, false))
  {
    nc_crit("[net][server] failed to start listening");
    return;
  }

  m_server_thread = std::jthread([this](const std::stop_token& token){ run_server_thread(token); });
}

//==============================================================================
Server::~Server()
{
  m_server_thread.request_stop();
  m_server_thread.join();

  for (ClientData& client : m_clients)
  {
    if (client.status != Status::connected)
      continue;

    close_socket(client.connection.socket);
    client.status = Status::none;
  }

  close_socket(m_listen_socket);
}

//==============================================================================
void Server::process_messages(PlayerID player_id)
{
  nc_assert(player_id < m_clients.size(), "invalid player id - \"{}\"", player_id);

  ClientData& client = m_clients[player_id];
  nc_assert(client.status == Status::connected);

  while(true)
  {
    auto [result, maybe_message] = protocol::pop_message(client.connection);

    switch (result)
    {
    case TransferResult::error:
      nc_warn("[net][server] protocol pop message error");
      return;
    case TransferResult::disconnected:
      client.status = Status::disconnecting;
      return;
    case TransferResult::success:
      if (!maybe_message)
        return;

      const protocol::Message message = *maybe_message;
      message.process(
        [this, player_id, &client](const protocol::messages::PlayerInputs& message)
        {
          m_inputs[player_id] = message.inputs;
          client.input_received = true;
        },
        [this](const protocol::messages::PositionSync& message)
        {
          broadcast(message);
        },
        [&client](const protocol::messages::HashSync& message)
        {
          client.state_hash = message.hash;
          client.hash_received = true;
        },
        [](const auto&){ nc_warn("[net][server] received invalid message"); }
      );
      break;
    }
  }
}

//==============================================================================
std::optional<PlayerID> Server::get_free_id() const
{
  for (PlayerID player_id = 0; player_id < m_clients.size(); ++player_id)
  {
    const ClientData& client = m_clients[player_id];

    if (client.status == Status::none)
      return player_id;
  }

  return std::nullopt;
}

//==============================================================================
u32 Server::get_connected_count() const
{
  u32 count = 0;
  for (const ClientData& client : m_clients)
  {
    if (client.status == Status::connected)
      count += 1;
  }

  return count;
}

//==============================================================================
void Server::send(ClientData& client, const protocol::Message& message)
{
  nc_assert(client.status == Status::connecting || client.status == Status::connected);

  const TransferResult result = protocol::send(client.connection, message);

  switch (result)
  {
  case TransferResult::success:
    break;
  case TransferResult::error:
    nc_warn("[net][server] protocol send message error");
    break;
  case TransferResult::disconnected:
    client.status = Status::disconnecting;
    break;
  }
}

//==============================================================================
void Server::broadcast(const protocol::Message& message)
{
  for (ClientData& client : m_clients)
  {
    if (client.status != Status::connected)
      continue;

    send(client, message);
  }
}

//==============================================================================
void Server::loop_until_inputs_received()
{
  bool all_inputs_received = false;
  while (!all_inputs_received)
  {
    all_inputs_received = true;
    for (PlayerID player_id = 0; player_id < m_clients.size(); ++player_id)
    {
      ClientData& client = m_clients[player_id];
      if (client.status != Status::connected)
        continue;

      process_messages(player_id);

      if (!client.input_received)
        all_inputs_received = false;
    }

    check_state_hashes();
  }
}

//==============================================================================
void Server::check_state_hashes()
{
  std::optional<PlayerID> reference_id;
  for (PlayerID player_id = 0; player_id < m_clients.size(); ++player_id)
  {
    const ClientData& client = m_clients[player_id];
    if (client.status != Status::connected)
      continue;

    // Wait until all clients sent their hash.
    if (!client.hash_received)
      return;

    if (!reference_id)
      reference_id = player_id;
  }

  if (!reference_id)
    return;

  const u64 reference_hash = m_clients[*reference_id].state_hash;

  bool desync = false;
  for (const ClientData& client : m_clients)
  {
    if (client.status == Status::connected && client.state_hash != reference_hash)
      desync = true;
  }

  if (desync)
  {
    nc_warn("[net][server] desync detected");

    for (PlayerID player_id = 0; player_id < m_clients.size(); ++player_id)
    {
      const ClientData& client = m_clients[player_id];
      if (client.status != Status::connected)
        continue;

      nc_warn(
        "[net][server]   player {} ({}:{}) hash {:016x}{}",
        player_id,
        client.connection.socket.address.to_string(),
        client.connection.socket.port,
        client.state_hash,
        client.state_hash == reference_hash ? "" : " <- differs"
      );
    }

    broadcast(protocol::messages::DesyncDetected{});
  }
  else
  {
    broadcast(protocol::messages::NoDesync{});
  }

  for (ClientData& client : m_clients)
    client.hash_received = false;
}

//==============================================================================
void Server::drain_accepts()
{
  while (is_accept_pending(m_listen_socket))
  {
    const auto maybe_client_socket = accept_client(m_listen_socket, false);
    if (!maybe_client_socket)
    {
      nc_warn("[net][server] failed to accept client");
      continue;
    }
    TCPSocket client_socket = *maybe_client_socket;

    const auto maybe_player_id = get_free_id();
    if (!maybe_player_id)
    {
      nc_log("[net][server] rejected accept - server is full");
      close_socket(client_socket);
      continue;
    }
    PlayerID player_id = *maybe_player_id;

    ClientData& client = m_clients[player_id];
    client.status = Status::connecting;
    client.connection = protocol::Connection{.socket = client_socket};
    client.input_received = false;
    client.hash_received = false;
  }
}

//==============================================================================
void Server::handle_connection_events()
{
  // handle connects
  for (PlayerID player_id = 0; player_id < m_clients.size(); ++player_id)
  {
    ClientData& client = m_clients[player_id];

    if (client.status != Status::connecting)
      continue;

    send(client, protocol::messages::NewPlayerData{.player_id = player_id});

    // TODO: In the future whole game state would be send to a newly connected player, for now we just spawn for him all
    // existing players.
    for (PlayerID id = 0; id < m_clients.size(); ++id)
    {
      if (m_clients[id].status != Status::connected)
        continue;

      send(client, protocol::messages::PlayerConnected{.player_id = id});
    }
    
    if (client.status != Status::connecting)
      continue;
    
    client.status = Status::connected;

    broadcast(protocol::messages::PlayerConnected{.player_id = player_id});
  }

  // handle disconnects
  for (PlayerID player_id = 0; player_id < m_clients.size(); ++player_id)
  {
    ClientData& client = m_clients[player_id];

    if (client.status != Status::disconnecting)
      continue;

    close_socket(client.connection.socket);
    client.status = Status::none;
    client.input_received = false;
    client.hash_received = false;

    broadcast(protocol::messages::PlayerDisconnected{.player_id = player_id});
  }
}

//==============================================================================
void Server::broadcast_all_player_inputs()
{
  for (ClientData& client : m_clients)
    client.input_received = false;

  broadcast(protocol::messages::AllPlayersInputs{.inputs_array = m_inputs});
}

//==============================================================================
void Server::run_server_thread(const std::stop_token& token)
{
  // TODO: detect disconnects while waiting for players
  while (get_connected_count() < m_expected_player_count)
  {
    if (token.stop_requested())
      return;

    drain_accepts();
    handle_connection_events();
  }

  close_socket(m_listen_socket);
  broadcast(protocol::messages::GameStart{});

  while (!token.stop_requested())
  {
    loop_until_inputs_received();
    handle_connection_events();
    broadcast_all_player_inputs();
  }
}

}
