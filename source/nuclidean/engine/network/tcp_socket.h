// Project Nuclidean Source File
#pragma once

#include <types.h>

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace nc::net
{

enum class TransferResult : u8
{
  success,
  disconnected,
  error,
};

struct IPv4Address
{
  std::array<u8, 4> octets{};

  auto operator<=>(const IPv4Address&) const = default;

  static constexpr IPv4Address any()      { return {{  0, 0, 0, 0}}; }
  static constexpr IPv4Address loopback() { return {{127, 0, 0, 1}}; }

  // Parses IPv4Address from string. Return std::nullopt on failure. 
  static std::optional<IPv4Address> parse(std::string_view view);
};

// Low level representation of TCP networking socket.
// This is non-owning handle.
// Interpretation of field depends on role:
// - server listen socket -> server's own address and port
// - client accepted on server -> client's address and port
// - client connecting to server -> server's address and port
struct TCPSocket
{
  static constexpr u64 invalid_handle = ~0ull;

  // Internal socket handle.
  u64 handle = invalid_handle;
  IPv4Address address{};
  u16 port{};

  bool is_valid() const;
};

// Init the networking backend. Return success/fail.
bool init();
// Shutdown the networking backend. This doesn't automatically close all sockets. Return success/fail.
bool shutdown();

// Create socket. Return std::nullopt on failure.
std::optional<TCPSocket> create_socket(IPv4Address server_address, u16 server_port);
// Closes socket. Return success/fail. Remains dangling after close.
bool close_socket(TCPSocket socket);

// Start listening for clients. Return success/fail. This does not close the socket on failure.
bool start_listening(TCPSocket server_socket);
// Accept a connected client. This is blocking call.
// Return std::nullopt on failure otherwise socket of newly connected client.
std::optional<TCPSocket> accept_client(TCPSocket server_socket);
// Connect to server. This is blocking call. Return success/fail. This does not close the socket on failure.
bool connect(TCPSocket client_socket);

// Send arbitrary data over network. This is blocking call.
// On anything other than success the socket should be closed and data may have been partially sent.
TransferResult send_data(TCPSocket socket, std::span<const std::byte> data_to_send);
// Receive arbitrary data over network. This is blocking call. Blocks until buffer is full.
// On anything other than success the socket should be closed and data may have been partially received.
TransferResult receive_data(TCPSocket socket, std::span<std::byte> data_to_receive);

}