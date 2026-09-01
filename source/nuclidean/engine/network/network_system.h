// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>
#include <engine/network/constants.h>

#include <array>
#include <memory>
#include <vector>
#include <string>

namespace nc
{

using CmdArgs = std::vector<std::string>;

namespace net
{
  class Server;
  class Client;
}

// TODO: integration tests

class NetworkSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  static NetworkSystem& get();

  NetworkSystem();
  ~NetworkSystem() override;

  bool init(const CmdArgs& args);
  void on_event(ModuleEvent& event) override;

  // Return true if game is running in multiplayer; false if game is running in singleplayer.
  bool is_multiplayer() const { return m_is_multiplayer; }
  PlayerID get_local_player_id() const { return m_local_player_id; }
  // Determine if a player occupies the given slot. Slots are filled by the server on connect.
  bool is_player_connected(PlayerID player_id) const;
  // Determine if this frame's input was received.
  bool are_inputs_received() const { return m_input_received; }

  // Poll available network events. This is non-blocking call.
  void poll_network();

private:
  //  Blocking wait until the expected number of players joins the session.
  void wait_for_players(u32 expected_player_count);

  // true if game is running in multiplayer; false if game is running in singleplayer.
  bool m_is_multiplayer = false;
  PlayerID m_local_player_id = 0;
  // Slot occupancy of all players in the session, including the local one.
  std::array<bool, MAX_PLAYER_COUNT> m_connected_players{};
  // Determine if this frame's input was received.
  bool m_input_received = false;
  u64 m_frame_counter = 0;

  std::unique_ptr<net::Server> m_server = nullptr;
  std::unique_ptr<net::Client> m_client = nullptr;
  
};

}