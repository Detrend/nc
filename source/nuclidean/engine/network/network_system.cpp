// Project Nuclidean Source File
// winsock2 must be included before anything else includes windows.h
#include <engine/input/game_input.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <engine/network/network_system.h>

#include <common.h>
#include <logging.h>

#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/core/engine.h>

#include <algorithm>
#include <iterator>

// TODO: this is only supported by MSVC, for other compiler support this would need to be properly linked
#pragma comment(lib, "ws2_32.lib")

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
bool NetworkSystem::init(const CmdArgs& args)
{
  const auto host_it = std::find(args.begin(), args.end(), "-mp_host");
  const auto client_it = std::find(args.begin(), args.end(), "-mp_client");

  if (host_it != args.end())
  {
    m_is_multiplayer = true;
    m_player_index   = 0;
  }
  else if (client_it != args.end() && std::next(client_it) != args.end())
  {
    m_is_multiplayer = true;
    m_player_index   = 1;
    m_peer_ip        = *std::next(client_it);
  }

  if (m_is_multiplayer)
  {
    WSADATA wsa{};
    const int result = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (result != 0)
    {
      nc_crit("[net] WSAStartup failed: {}", result);
      return false;
    }
    m_wsa_started = true;
  }

  return true;
}

//==============================================================================
void NetworkSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::post_init:
    {
      if (m_is_multiplayer && !establish_connection())
      {
        nc_crit("[net] failed to establish connection, quitting.");
        get_engine().request_quit();
      }
    }
    break;

    case ModuleEventType::terminate:
    {
      close_connection();
      if (m_wsa_started)
      {
        WSACleanup();
        m_wsa_started = false;
      }
    }
    break;
  } 
}

//==============================================================================
bool NetworkSystem::is_multiplayer() const
{
  return m_is_multiplayer;
}

//==============================================================================
u8 NetworkSystem::get_player_index() const
{
  return m_player_index;
}

//==============================================================================
bool NetworkSystem::is_host() const
{
  return m_player_index == 0; 
}

//==============================================================================
bool NetworkSystem::is_client() const
{
  return !is_host(); 
}

//==============================================================================
bool send_data(SOCKET peer_socket, const char* buffer, int length)
{
  int sent = 0;
  while (sent < length)
  {
    // send is allowed to accept fewer bytes than asked
    const int n = send(peer_socket, buffer + sent, length - sent, 0);

    if (n <= 0)
      return false;

    sent += n;
  }

  return true;
}

//==============================================================================
bool receive_data(SOCKET peer_socket, char* buffer, int length)
{
  int received = 0;
  while (received < length)
  {
    // recv is allowed to accept fewer bytes than asked
    const int n = recv(peer_socket, buffer + received, length - received, 0);

    if (n <= 0)
      return false;

    received += n;
  }

  return true;
}

//==============================================================================
PlayerInputArray NetworkSystem::exchange(const PlayerSpecificInputs& local_inputs)
{
  const SOCKET peer_socket = (SOCKET)m_peer_socket;

  PlayerSpecificInputs peer_inputs;

  const bool result = send_data(peer_socket, (const char *)&local_inputs, sizeof(local_inputs))
    && receive_data(peer_socket, (char*)&peer_inputs, sizeof(peer_inputs));

  if (!result)
  {
    nc_crit("[net] peer disconnected, quitting.");
    get_engine().request_quit();
    return PlayerInputArray();
  }

  const u8 local_index = get_player_index();
  const u8 peer_index  = local_index ^ 1;

  PlayerInputArray inputs;
  inputs[local_index] = local_inputs;
  inputs[peer_index]  = peer_inputs;

  return inputs;
}

//==============================================================================
bool NetworkSystem::establish_connection()
{
  return is_host() ? connect_as_host() : connect_as_client();
}

//==============================================================================
static void set_no_delay(SOCKET socket)
{
  /*
   * Disables Nagle's algorithm on the TCP socket.
   * When Nagle is on TCP buffers writes by waiting ~40ms after each write.
   */

  BOOL enabled = TRUE;
  setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&enabled, sizeof(enabled));
}

//==============================================================================
bool NetworkSystem::connect_as_host()
{
  const SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_socket == INVALID_SOCKET)
  {
    nc_crit("[net] host socket() failed: {}", WSAGetLastError());
    return false;
  }

  sockaddr_in address{};
  address.sin_family      = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port        = htons(m_port);

  if (bind(listen_socket, (const sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
  {
    nc_crit("[net] bind() failed: {}", WSAGetLastError());
    closesocket(listen_socket);
    return false;
  }

  if (listen(listen_socket, 1) == SOCKET_ERROR)
  {
    nc_crit("[net] listen() failed: {}", WSAGetLastError());
    closesocket(listen_socket);
    return false;
  }

  nc_log("[net] host listening on port {}, waiting for peer...", m_port);

  sockaddr_in peer_address{};
  int         peer_address_length = sizeof(peer_address);

  const SOCKET peer_socket = accept(listen_socket, (sockaddr*)&peer_address, &peer_address_length);
  closesocket(listen_socket);

  if (peer_socket == INVALID_SOCKET)
  {
    nc_crit("[net] accept() failed: {}", WSAGetLastError());
    return false;
  }

  set_no_delay(peer_socket);
  m_peer_socket = (uintptr_t)peer_socket;

  char client_ip[INET_ADDRSTRLEN]{};
  inet_ntop(AF_INET, &peer_address.sin_addr, client_ip, sizeof(client_ip));
  nc_log("[net] peer connected from {}:{}.", client_ip, ntohs(peer_address.sin_port));

  return true;
}

//==============================================================================
bool NetworkSystem::connect_as_client()
{
  sockaddr_in server_address{};
  server_address.sin_family = AF_INET;
  server_address.sin_port   = htons(m_port);
  if (inet_pton(AF_INET, m_peer_ip.c_str(), &server_address.sin_addr) != 1)
  {
    nc_crit("[net] invalid peer ip '{}'.", m_peer_ip);
    return false;
  }

  while (true)
  {
    const SOCKET host_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (host_socket == INVALID_SOCKET)
    {
      nc_crit("[net] client socket() failed: {}", WSAGetLastError());
      return false;
    }

    if (connect(host_socket, (const sockaddr*)&server_address, sizeof(server_address)) == 0)
    {
      m_peer_socket = (uintptr_t)host_socket;
      set_no_delay(m_peer_socket);
      nc_log("[net] connected to host {}:{}.", m_peer_ip, m_port);
      return true;
    }

    closesocket(host_socket);
    nc_warn("[net] connect attempt to {} failed, retrying...", m_peer_ip);
    Sleep(500);
  }

  return false;
}

//==============================================================================
void NetworkSystem::close_connection()
{
  if ((SOCKET)m_peer_socket != INVALID_SOCKET)
  {
    closesocket((SOCKET)m_peer_socket);
    m_peer_socket = (uintptr_t)INVALID_SOCKET;
  }
}

}
