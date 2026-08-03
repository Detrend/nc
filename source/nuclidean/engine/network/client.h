// Project Nuclidean Source File
#pragma once

#include <engine/input/game_input.h>
#include <engine/network/protocol.h>
#include <engine/network/tcp_socket.h>

#include <optional>

namespace nc::net
{

class Client
{
public:
  Client(IPv4Address server_address, u16 server_port);
  ~Client();

  // Determine if the connection to the server was established.
  bool is_connected() const;

  // Send player inputs over the network.
  void send_inputs(const PlayerSpecificInputs& inputs);
  // Pop next received message. If no messages are pending return `std::nullopt`.
  std::optional<protocol::Message> pop_message();

private:
  protocol::Connection m_connection{};

};

}