// Project Nuclidean Source File
#pragma once

#include <engine/network/tcp_socket.h>

#include <array>
#include <bit>
#include <functional>
#include <span>
#include <type_traits>

namespace nc::net
{

//==============================================================================
// TODO: bit_cast copies struct padding
template <typename... Ts>
  requires (... && std::is_trivially_copyable_v<Ts>)
constexpr std::array<std::byte, (sizeof(Ts) + ...)> to_bytes(const Ts&... vals)
{
  std::array<std::byte, (sizeof(Ts) + ...)> out{};
  std::size_t pos = 0;
  ([&]
    {
      auto b = std::bit_cast<std::array<std::byte, sizeof(Ts)>>(vals);
      for (std::byte byte : b) out[pos++] = byte;
    }(),
    ...
  );
  return out;
}

//==============================================================================
template <typename... Ts>
    requires (... && std::is_trivially_copyable_v<Ts>)
constexpr std::tuple<Ts...> from_bytes(std::span<const std::byte> bytes)
{
  std::size_t position = 0;
  auto read = [&]<typename T>() -> T
  {
    std::array<std::byte, sizeof(T)> temp{};
    std::copy_n(bytes.begin() + position, sizeof(T), temp.begin());
    position += sizeof(T);
    return std::bit_cast<T>(temp);
  };
  return std::tuple<Ts...>{ read.template operator()<Ts>()... };
}

//==============================================================================
template <typename... Ts>
  requires (... && std::is_trivially_copyable_v<Ts>)
TransferResult send_data(TCPSocket socket, const Ts&... vals)
{
  return send_raw_data(socket, to_bytes(vals...));
}

//==============================================================================
template <typename... Ts>
    requires (... && std::is_trivially_copyable_v<Ts>)
std::tuple<TransferResult, Ts...> receive_data(TCPSocket socket)
{
  std::array<std::byte, (sizeof(Ts) + ...)> buf{};
  const TransferResult result =receive_raw_data(socket, buf);
  return std::tuple_cat(std::make_tuple(result), from_bytes<Ts...>(buf));
}

}

//==============================================================================
template <>
struct std::hash<nc::net::IPv4Address>
{
  std::size_t operator()(const nc::net::IPv4Address& address) const noexcept
  {
    return std::hash<nc::u32>{}(std::bit_cast<nc::u32>(address.octets));
  }
};