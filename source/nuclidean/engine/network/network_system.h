// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>
#include <engine/input/game_input.h> 
#include <engine/network/constants.h>

#include <cstdint>
#include <string>
#include <vector>

namespace nc
{

using CmdArgs = std::vector<std::string>;

struct InputExchangeResult
{
  PlayerInputArray inputs;
  bool             desynced       = false;
  bool             state_mismatch = false;
};

class NetworkSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  static NetworkSystem& get();

  bool init(const CmdArgs& args);
  void on_event(ModuleEvent& event) override;

  bool is_multiplayer() const;
  u8   get_player_index() const;
  bool is_host() const;
  bool is_client() const;

  InputExchangeResult exchange(const PlayerSpecificInputs& local_inputs, u64 frame_index, u64 state_checksum);

  std::string exchange_blob(const std::string& local_blob);

private:
  bool        m_is_multiplayer = false;
  u8          m_player_index   = 0;
  std::string m_peer_ip;
  u16         m_port           = 18082;
  uintptr_t   m_peer_socket    = 0;
  bool        m_wsa_started    = false;
  bool        m_desync_reported = false;

  bool establish_connection();
  bool connect_as_host();
  bool connect_as_client();

  void close_connection();
};

}