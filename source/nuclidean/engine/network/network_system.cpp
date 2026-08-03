// Project Nuclidean Source File
#include <engine/network/network_system.h>

#include <logging.h>

#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/core/engine.h>
#include <engine/input/input_system.h>
#include <engine/network/protocol.h>
#include <engine/network/server.h>
#include <engine/network/client.h>
#include <engine/network/tcp_socket.h>

namespace nc
{

//==============================================================================
EngineModuleId NetworkSystem::get_module_id()
{
  return EngineModule::network_system;
}

//==============================================================================
NetworkSystem& NetworkSystem::get()
{
  return get_engine().get_module<NetworkSystem>();
}

//==============================================================================
NetworkSystem::NetworkSystem() = default;

//==============================================================================
NetworkSystem::~NetworkSystem() = default;

//==============================================================================
bool NetworkSystem::init(const CmdArgs& args)
{
  const auto host_it = std::find(args.begin(), args.end(), "-mp_host");
  const auto client_it = std::find(args.begin(), args.end(), "-mp_client");

  // return if not multiplayer
  if (!((host_it != args.end()) || (client_it != args.end())))
    return true;

  if (!net::init())
  {
    nc_crit("[net][network system] failed to init");
    return false;
  }

  // is server?
  if (host_it != args.end())
  {
    m_server = std::make_unique<net::Server>(net::IPv4Address::any(), net::protocol::PORT);
    m_client = std::make_unique<net::Client>(net::IPv4Address::loopback(), net::protocol::PORT);
  }
  // is client?
  else
  {
    if (std::next(client_it) == args.end())
    {
      nc_crit("[net][network system] missing server IP after \"-mp_client\"");
      return false;
    }

    auto maybe_address = net::IPv4Address::parse(*std::next(client_it));
    if (!maybe_address)
    {
      nc_crit("[net][network system] failed to parse server IP");
      return false;
    }

    m_client = std::make_unique<net::Client>(*maybe_address, net::protocol::PORT);
  }

  if (!m_client->is_connected())
  {
    nc_crit("[net][network system] client is not connected to the server");
    return false;
  }

  m_is_multiplayer = true;
  return true;
}

//==============================================================================
void NetworkSystem::on_event(ModuleEvent& event)
{
  if (!m_is_multiplayer) return;

  switch (event.type)
  {
  case ModuleEventType::frame_start:
    m_input_received = false;
    m_client->send_inputs(InputSystem::get().get_inputs().player_inputs);
    break;
  case ModuleEventType::terminate:
    m_client = nullptr;
    m_server = nullptr;

    if (!net::shutdown())
      nc_crit("[net][network system] failed to shutdown");

    break;
  }
}

//==============================================================================
bool NetworkSystem::is_player_connected(PlayerID player_id) const
{
  nc_assert(player_id < MAX_PLAYER_COUNT, "invalid player id - \"{}\"", player_id);
  return m_connected_players[player_id];
}

//==============================================================================
void NetworkSystem::poll_network()
{
  if (!m_is_multiplayer) return;

  while (const std::optional<net::protocol::Message> message = m_client->pop_message())
  {
    using namespace net::protocol::messages;

    message->process(
      [this](const NewPlayerData& message)
      {
        m_local_player_id = message.player_id;
      },
      [this](const PlayerConnected& message)
      {
        nc_assert(message.player_id < MAX_PLAYER_COUNT, "invalid player id - \"{}\"", message.player_id);
        m_connected_players[message.player_id] = true;

        get_engine().send_event(
        {
          .type = ModuleEventType::net_player_spawned,
          .net_player = { .player_id = message.player_id },
        });
      },
      [this](const PlayerDisconnected& message)
      {
        nc_assert(message.player_id < MAX_PLAYER_COUNT, "invalid player id - \"{}\"", message.player_id);
        m_connected_players[message.player_id] = false;

        get_engine().send_event(
        {
          .type = ModuleEventType::net_player_despawned,
          .net_player = { .player_id = message.player_id },
        });
      },
      [this](const AllPlayersInputs& message)
      {
        get_engine().send_event(ModuleEvent
        {
          .type = ModuleEventType::net_input_arrived,
          .net_inputs = { .inputs = &message.inputs_array },
        });
        m_input_received = true;
      },
      [](const auto&){ nc_warn("[net][network system] client received invalid message"); }
    );
  }
}

}