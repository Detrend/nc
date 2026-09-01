// Project Nuclidean Source File
#include <engine/network/network_system.h>

#include <logging.h>

#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/core/engine.h>
#include <engine/entity/entity_system.h>
#include <engine/game/game_system.h>
#include <engine/input/input_system.h>
#include <engine/network/protocol.h>
#include <engine/network/server.h>
#include <engine/network/client.h>
#include <engine/network/tcp_socket.h>
#include <engine/player/player.h>

#include <charconv>
#include <chrono>
#include <thread>

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
  const auto server_it = std::find(args.begin(), args.end(), "-mp_server");
  const auto client_it = std::find(args.begin(), args.end(), "-mp_client");

  // return if not multiplayer
  if (!((server_it != args.end()) || (client_it != args.end())))
    return true;

  if (!net::init())
  {
    nc_crit("[net][network system] failed to init");
    return false;
  }

  u32 expected_player_count = 0;

  // is server?
  if (server_it != args.end())
  {
    if (std::next(server_it) == args.end())
    {
      nc_crit("[net][network system] missing expected player count after \"-mp_server\"");
      return false;
    }

    const std::string& count_arg = *std::next(server_it);
    const cstr number_begin = count_arg.data();
    const cstr number_end   = number_begin + count_arg.size();

    const auto [parse_end, error] = std::from_chars(number_begin, number_end, expected_player_count);

    const bool valid_number = error == std::errc{} && parse_end == number_end;
    const bool valid_count  = expected_player_count >= 1 && expected_player_count <= MAX_PLAYER_COUNT;

    if (!valid_number || !valid_count)
    {
      nc_crit("[net][network system] invalid expected player count after \"-mp_server\" (must be 1..{})", MAX_PLAYER_COUNT);
      return false;
    }

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

  if (m_server)
    wait_for_players(expected_player_count);

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

    // WARNING: Used as temporary workaround to solve desync issues.
    {
      m_frame_counter += 1;
      if (m_server && m_frame_counter % 30 == 0)
      {
        PositionArray positions{};

        EntityRegistry&    entities   = GameSystem::get().get_entities();
        const PlayerArray& player_ids = GameSystem::get().get_player_ids();

        for (PlayerID player_id = 0; player_id < MAX_PLAYER_COUNT; ++player_id)
        {
          if (const Player* player = entities.get_entity<Player>(player_ids[player_id]))
            positions[player_id] = player->get_position();
        }

        m_client->send_positions(positions);
      }
    }
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
      [this](const PositionSync& message)
      {
        EntityRegistry&    entities   = GameSystem::get().get_entities();
        const PlayerArray& player_ids = GameSystem::get().get_player_ids();

        for (PlayerID player_id = 0; player_id < MAX_PLAYER_COUNT; ++player_id)
        {
          if (Player* player = entities.get_entity<Player>(player_ids[player_id]))
            player->set_position(message.position_array[player_id]);
        }
      },
      [](const auto&){ nc_warn("[net][network system] client received invalid message"); }
    );
  }
}

//==============================================================================
void NetworkSystem::wait_for_players(u32 expected_player_count)
{
  using namespace net::protocol::messages;

  nc_log("[net][network system] waiting for {} players", expected_player_count);

  u32 connected_count = 0;
  while (connected_count < expected_player_count)
  {
    m_client->send_inputs(PlayerSpecificInputs{});

    while (const std::optional<net::protocol::Message> message = m_client->pop_message())
    {
      message->process(
        [this](const NewPlayerData& message)
        {
          m_local_player_id = message.player_id;
        },
        [this, &connected_count, expected_player_count](const PlayerConnected& message)
        {
          nc_assert(message.player_id < MAX_PLAYER_COUNT, "invalid player id - \"{}\"", message.player_id);
          if (!m_connected_players[message.player_id])
          {
            m_connected_players[message.player_id] = true;
            connected_count += 1;
            nc_log("[net][network system] player {} joined ({}/{})", message.player_id, connected_count, expected_player_count);
          }
        },
        [](const AllPlayersInputs&){ /* Inputs are ignored while waiting for players. */ },
        [](const auto&){ nc_warn("[net][network system] unexpected message while waiting for players"); }
      );
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
}

}
