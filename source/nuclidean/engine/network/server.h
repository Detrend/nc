// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <engine/network/constants.h>
#include <engine/network/protocol.h>
#include <engine/network/tcp_socket.h>

#include <array>
#include <stop_token>
#include <thread>

namespace nc::net
{

class Server
{
public:
  Server(IPv4Address address, u16 port);
  ~Server();

private:
  // Client slot connection status.
  enum class Status : u8
  {
    none,
    connecting,
    connected,
    disconnecting,
  }; 
  struct ClientData
  {
    Status status = Status::none;
    protocol::Connection connection{};
    bool input_received = false;
  };

  TCPSocket m_listen_socket;
  std::jthread m_server_thread;
  std::array<ClientData, MAX_PLAYER_COUNT> m_clients;
  InputArray m_inputs;

  // Drains and process client messages.
  void process_messages(PlayerID player_id);
  // Get unused player id.
  std::optional<PlayerID> get_free_id() const;
  // Send message to specified client.
  void send(ClientData& client, const protocol::Message& message);
  // Broadcast message to all clients.
  void broadcast(const protocol::Message& message);

  // Loops until all inputs are received.
  void loop_until_inputs_received();
  // Drain all accepts on listen socket.
  void drain_accepts();
  // Handle connects/disconnects.
  void handle_connection_events();
  // Broadcast all player inputs.
  void broadcast_all_player_inputs();

  void run_server_thread(std::stop_token token);
};

}