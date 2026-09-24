// Project Nuclidean Source File
#pragma once

#include <common.h>
#include <types.h>
#include <math/vector.h>

#include <engine/input/game_input.h>
#include <engine/network/constants.h>
#include <engine/network/tcp_socket.h>

#include <array>
#include <optional>
#include <variant>

// =============================================================================
//                             _   _   _____  ______ 
//                            | \ | | /  __ \ | ___ \
//                            |  \| | | /  \/ | |_/ /
//                            | . ` | | |     |  __/ 
//                            | |\  | | \__/\ | |    
//                            \_| \_/  \____/ \_|    
//
// =============================================================================
//                      Nuclidean Communication Protocol
// =============================================================================

namespace nc::net::protocol
{

inline constexpr u16 PORT = 18082;

// Each type in this namespace represent a message which can be send over the network.
namespace messages
{
  // Send from server to a newly connected client.
  struct NewPlayerData
  {
    // Assigned player id.
    u8 player_id;
    // TODO: serialized game state
  };

  // Send from server to client when new player connects.
  struct PlayerConnected
  {
    // Id of newly connected player.
    u8 player_id;
  };

  // Send from server to client when some player disconnects.
  struct PlayerDisconnected
  {
    // Id of disconnected player.
    u8 player_id;
  };

  // Send from client to server at start of every frame.
  struct PlayerInputs
  {
    // Player inputs.
    PlayerSpecificInputs inputs;
  };

  // Send from server to all clients when server receives `PlayerInputs` from each of them.
  struct AllPlayersInputs
  {
    // Input from all players.
    InputArray inputs_array;
  };

  // Broadcast from server to all clients. Contains position of each client.
  // WARNING: This is used only as temporary workaround to desync issues.
  struct PositionSync
  {
    PositionArray position_array;
  };

  // Broadcast from server to all clients once the expected number of players is connected.
  struct GameStart {};

  // Send from client to server before `PlayerInputs` when the desync check is enabled.
  struct HashSync
  {
    // Hash of the game state (frame number and position of all entities).
    u64 hash;
  };

  // Broadcast from server to all clients when hashes of all clients match.
  struct NoDesync {};

  // Broadcast from server to all clients when hashes of clients differ.
  struct DesyncDetected {};
}

using MessageBase = std::variant
<
  messages::NewPlayerData,
  messages::PlayerConnected,
  messages::PlayerDisconnected,
  messages::PlayerInputs,
  messages::AllPlayersInputs,
  messages::PositionSync,
  messages::GameStart,
  messages::HashSync,
  messages::NoDesync,
  messages::DesyncDetected
>;

// Represent message which can be send/received over the network.
struct Message : MessageBase
{
  using MessageBase::MessageBase;

  // Process the network message based on the message type.
  template<typename... Ts>
  void process(Ts&&... handlers) const
  {
    std::visit(MessageVisitor<std::decay_t<Ts>...>{ std::forward<Ts>(handlers)... }, *this);
  }

private:
  template<typename... Ts>
  struct MessageVisitor : Ts... { using Ts::operator()...; };
};

// Represent network connection - wrapper around `TCPSocket` + data buffer.
struct Connection
{
  TCPSocket socket;
  std::array<std::byte, sizeof(Message) + 1> buffer;
  u32 buffer_size = 0;
};

// Send message over network to specified connection.
TransferResult send(Connection& connection, const Message& message);
// Pop pending message received over the network. If not messages are pending return `std::nullopt`.
std::pair<TransferResult, std::optional<Message>> pop_message(Connection& connection);

}
