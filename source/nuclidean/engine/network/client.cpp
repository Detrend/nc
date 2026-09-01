// Project Nuclidean Source File
#include <engine/network/client.h>

#include <logging.h>
#include <engine/network/protocol.h>
#include <engine/network/tcp_socket.h>

namespace nc::net
{

//==============================================================================
Client::Client(IPv4Address server_address, u16 server_port)
{
  auto maybe_socket = create_socket(server_address, server_port);
  if (!maybe_socket)
  {
    nc_crit("[net][client] failed to create socket");
    return;
  }
  m_connection.socket = *maybe_socket;

  if (!connect(m_connection.socket, false))
  {
    nc_crit("[net][client] failed to connect to server");
    close_socket(m_connection.socket);
    return;
  }
}

//==============================================================================
Client::~Client()
{
  close_socket(m_connection.socket);
}

//==============================================================================
bool Client::is_connected() const
{
  return m_connection.socket.is_valid();
}

//==============================================================================
void Client::send_inputs(const PlayerSpecificInputs& inputs)
{
  const TransferResult result = protocol::send(
    m_connection,
    protocol::messages::PlayerInputs{.inputs = inputs}
  );

  switch (result)
  {
  case TransferResult::success:
    break;
  case TransferResult::error:
    nc_warn("[net][client] protocol send message error");
    break;
  case TransferResult::disconnected:
    nc_warn("[net][client] server disconnected");
    break;
  }
}

//==============================================================================
void Client::send_positions(const PositionArray& positions)
{
  const TransferResult result = protocol::send(
    m_connection,
    protocol::messages::PositionSync{.position_array = positions}
  );

  switch (result)
  {
  case TransferResult::success:
    break;
  case TransferResult::error:
    nc_warn("[net][client] protocol send message error");
    break;
  case TransferResult::disconnected:
    nc_warn("[net][client] server disconnected");
    break;
  }
}

//==============================================================================
std::optional<protocol::Message> Client::pop_message()
{
  auto [result, maybe_message] = protocol::pop_message(m_connection);

  switch (result)
  {
  case TransferResult::success:
    return maybe_message;
  case TransferResult::error:
    nc_warn("[net][client] protocol receive message error");
    break;
  case TransferResult::disconnected:
    nc_warn("[net][client] server disconnected");
    break;
  }

  return std::nullopt;
}

}