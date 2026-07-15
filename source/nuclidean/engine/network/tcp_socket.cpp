// Project Nuclidean Source File
#include <winsock2.h> // must be included before anything else includes windows.h

#include <engine/network/tcp_socket.h>

#include <common.h>
#include <logging.h>
#include <types.h>

#include <array>
#include <bit>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include <ws2tcpip.h>

#if NC_TESTS
#include <unit_test.h>
#endif

namespace nc::net
{

// Determine if WSA was started.
static bool g_wsa_started = false;

// Disables Nagle's algorithm on the TCP socket handle.
// When Nagle is on, TCP waits ~40ms after each write to buffer more data.
static bool set_no_delay(SOCKET handle);
// Get error message based on system category error code.
static std::string get_error_message(int error_code);
// Get last WSA error message.
static std::string get_last_wsa_error();
// Get transfer result from error code.
static TransferResult get_transfer_result(int error_code);

static_assert(TCPSocket::invalid_handle == INVALID_SOCKET);

//==============================================================================
std::optional<IPv4Address> IPv4Address::parse(std::string_view view)
{
  char str[INET_ADDRSTRLEN]{};
  if (view.size() >= sizeof(str))
  {
    return std::nullopt;
  }
  std::memcpy(str, view.data(), view.size());

  in_addr address{};
  if (inet_pton(AF_INET, str, &address) != 1)
  {
    return std::nullopt;
  }

  return IPv4Address{.octets = std::bit_cast<std::array<u8, 4>>(address.s_addr)};
}

//==============================================================================
bool TCPSocket::is_valid() const
{
  return handle != INVALID_SOCKET;
}

//==============================================================================
bool init()
{
  WSADATA wsa{};
  const int error = WSAStartup(MAKEWORD(2, 2), &wsa);
  if (error != 0)
  {
    nc_crit("[net] WSAStartup() failed: \"{}\"", get_error_message(error));
    return false;
  }

  g_wsa_started = true;
  return true;
}

//==============================================================================
bool shutdown()
{
  if (!g_wsa_started)
  {
    return true;
  }

  const int error = WSACleanup();
  if (error != 0)
  {
    nc_crit("[net] WSACleanup() failed: \"{}\"", get_error_message(error));
    return false;
  }

  g_wsa_started = false;
  return true;
}

//==============================================================================
std::optional<TCPSocket> create_socket(IPv4Address server_address, u16 server_port)
{
  const SOCKET handle = socket(AF_INET, SOCK_STREAM, 0);
  if (handle == INVALID_SOCKET)
  {
    nc_crit("[net] socket() failed: \"{}\"", get_last_wsa_error());
    return std::nullopt;
  }

  return TCPSocket
  {
    .handle = cast<u64>(handle),
    .address = server_address,
    .port = server_port,
  };
}

//==============================================================================
bool close_socket(TCPSocket socket)
{
  if (!socket.is_valid())
  {
    return true;
  }

  if (closesocket(cast<SOCKET>(socket.handle)) == SOCKET_ERROR)
  {
    nc_crit("[net] closesocket() failed: \"{}\"", get_last_wsa_error());
    return false;
  }

  return true;
}

//==============================================================================
bool start_listening(TCPSocket server_socket)
{
  const SOCKET server_handle = cast<SOCKET>(server_socket.handle);

  BOOL enabled = TRUE;
  const int setsockopt_result = setsockopt(
    server_handle,
    SOL_SOCKET, 
    SO_REUSEADDR, 
    recast<const char*>(&enabled), 
    sizeof(enabled)
  );
  if (setsockopt_result == SOCKET_ERROR)
  {
    nc_crit("[net] setsockopt() failed: \"{}\"", get_last_wsa_error());
    return false;
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = std::bit_cast<ULONG>(server_socket.address);
  address.sin_port = htons(server_socket.port);

  if (bind(server_handle, recast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
  {
    nc_crit("[net] bind() failed: \"{}\"", get_last_wsa_error());
    return false;
  }

  if (listen(server_handle, SOMAXCONN) == SOCKET_ERROR)
  {
    nc_crit("[net] listen() failed: \"{}\"", get_last_wsa_error());
    return false;
  }

  return true;
}

//==============================================================================
std::optional<TCPSocket> accept_client(TCPSocket server_socket)
{
  const SOCKET server_handle = cast<SOCKET>(server_socket.handle);

  sockaddr_in client_address{};
  int client_address_length = sizeof(client_address);

  const SOCKET client_handle = accept(server_handle, recast<sockaddr*>(&client_address), &client_address_length);
  if (client_handle == INVALID_SOCKET)
  {
    nc_crit("[net] accept() failed: \"{}\"", get_last_wsa_error());
    return std::nullopt;
  }

  if (!set_no_delay(client_handle))
  {
    close_socket(TCPSocket{.handle = cast<u64>(client_handle)});
    return std::nullopt;
  }

  return TCPSocket
  {
    .handle = cast<u64>(client_handle),
    .address = IPv4Address{.octets = std::bit_cast<std::array<u8, 4>>(client_address.sin_addr.s_addr)},
    .port = ntohs(client_address.sin_port),
  };
}

//==============================================================================
bool connect(TCPSocket client_socket)
{
  const SOCKET client_handle = cast<SOCKET>(client_socket.handle);

  sockaddr_in server_address{};
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = std::bit_cast<ULONG>(client_socket.address);
  server_address.sin_port = htons(client_socket.port);

  if (connect(client_handle, recast<const sockaddr*>(&server_address), sizeof(server_address)) != 0)
  {
    nc_crit("[net] connect() failed: \"{}\"", get_last_wsa_error());
    return false;
  }

  if (!set_no_delay(client_handle))
  {
    return false;
  }

  return true;
}

//==============================================================================
TransferResult send_data(TCPSocket socket, std::span<const std::byte> data_to_send)
{
  u64 sent = 0;
  while (sent < data_to_send.size())
  {
    // send is allowed to accept fewer bytes than asked
    const int result = send(
      cast<SOCKET>(socket.handle), 
      recast<const char*>(data_to_send.data() + sent), 
      data_to_send.size() - sent, 
      0
    );

    if (result == SOCKET_ERROR)
    {
      const int error = WSAGetLastError();
      nc_crit("[net] send() failed: \"{}\"", get_error_message(error));
      return get_transfer_result(error);
    }

    sent += cast<u64>(result);
  }

  return TransferResult::success;
}

//==============================================================================
TransferResult receive_data(TCPSocket socket, std::span<std::byte> data_to_receive)
{
  u64 received = 0;
  while (received < data_to_receive.size())
  {
    const int result = recv(
      cast<SOCKET>(socket.handle),
      recast<char*>(data_to_receive.data() + received),
      data_to_receive.size() - received,
      0
    );

    if (result == SOCKET_ERROR)
    {
      const int error = WSAGetLastError();
      nc_crit("[net] recv() failed: \"{}\"", get_error_message(error));
      return get_transfer_result(error);
    }
    if (result == 0)
    {
      // proper disconnect
      return TransferResult::disconnected;
    }

    received += cast<u64>(result);
  }

  return TransferResult::success;
}

//==============================================================================
static bool set_no_delay(SOCKET handle)
{
  BOOL enabled = TRUE;
  if (setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, recast<const char*>(&enabled), sizeof(enabled)) == SOCKET_ERROR)
  {
    nc_crit("[net] setsockopt() failed \"{}\"", get_last_wsa_error());
    return false;
  }

  return true;
}

//==============================================================================
static std::string get_error_message(int error_code)
{
  const std::error_code error{error_code, std::system_category()};
  return std::format("{} ({})", error.message(), error.value());
}

//==============================================================================
static std::string get_last_wsa_error()
{
  return get_error_message(WSAGetLastError());
}

//==============================================================================
static TransferResult get_transfer_result(int error_code)
{
  switch (error_code)
  {
    case WSAECONNRESET:
    case WSAECONNABORTED:
    case WSAENETRESET:
    case WSAETIMEDOUT:
    case WSAESHUTDOWN:
      return TransferResult::disconnected;

    default:
      return TransferResult::error;
  }
}

}

#if NC_TESTS
//============================================================================//
//                                   TESTS                                    //
//============================================================================//
// CMD args:
// -unit_test -test_filter=Net.*
namespace nc::net
{

//==============================================================================
bool ipv4_address_parse_valid_test(unit_test::TestCtx& /*ctx*/)
{
  struct TestCase
  {
    std::string_view input;
    IPv4Address      expected;
  };

  const TestCase TEST_CASES[]
  {
    {"0.0.0.0",         IPv4Address::any()               },
    {"127.0.0.1",       IPv4Address::loopback()          },
    {"1.2.3.4",         IPv4Address{{1, 2, 3, 4}}        },
    {"192.168.0.255",   IPv4Address{{192, 168, 0, 255}}  },
    {"255.255.255.255", IPv4Address{{255, 255, 255, 255}}},
  };

  for (const TestCase& test_case : TEST_CASES)
  {
    const std::optional<IPv4Address> parsed = IPv4Address::parse(test_case.input);
    if (!parsed.has_value())
    {
      nc_warn("[net] parse(\"{}\") returned nullopt, expected an address.", test_case.input);
      NC_TEST_FAIL;
    }

    if (*parsed != test_case.expected)
    {
      nc_warn(
        "[net] parse(\"{}\") returned {}.{}.{}.{}, expected {}.{}.{}.{}.",
        test_case.input,
        parsed->octets[0], parsed->octets[1], parsed->octets[2], parsed->octets[3],
        test_case.expected.octets[0], test_case.expected.octets[1],
        test_case.expected.octets[2], test_case.expected.octets[3]
      );
      NC_TEST_FAIL;
    }
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(ipv4_address_parse_valid_test)->name("Net IPv4Address Parse Valid");

//==============================================================================
bool ipv4_address_parse_invalid_test(unit_test::TestCtx& /*ctx*/)
{
  const std::string_view TEST_CASES[]
  {
    "",
    "1",
    "1.2",
    "1.2.3",
    "1.2.3.4.5",
    "1.2.3.",
    ".1.2.3",
    "256.0.0.1",
    "1.2.3.256",
    "-1.0.0.1",
    "010.0.0.1",
    "1.2.3.04",
    " 1.2.3.4",
    "1.2.3.4 ",
    "1.2.3.a",
    "abc",
    "localhost",
    "::1",
    "255.255.255.2555",
    "1234567890123456789",
  };

  for (const std::string_view test_case : TEST_CASES)
  {
    const std::optional<IPv4Address> parsed = IPv4Address::parse(test_case);
    if (parsed.has_value())
    {
      nc_warn(
        "[net] parse(\"{}\") returned {}.{}.{}.{}, expected nullopt.",
        test_case,
        parsed->octets[0], parsed->octets[1], parsed->octets[2], parsed->octets[3]
      );
      NC_TEST_FAIL;
    }
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(ipv4_address_parse_invalid_test)->name("Net IPv4Address Parse Invalid");

//==============================================================================
bool ipv4_address_parse_unterminated_view_test(unit_test::TestCtx& /*ctx*/)
{
  const std::string      loopback_buffer = "127.0.0.1255.255.255.255";
  const std::string_view loopback_view{loopback_buffer.data(), 9};

  const std::optional<IPv4Address> loopback = IPv4Address::parse(loopback_view);
  if (!loopback.has_value() || *loopback != IPv4Address::loopback())
  {
    nc_warn("[net] parse() of a non null-terminated view did not yield 127.0.0.1.");
    NC_TEST_FAIL;
  }

  const std::string      truncated_buffer = "1.2.3.45";
  const std::string_view truncated_view{truncated_buffer.data(), 7};

  const std::optional<IPv4Address> truncated = IPv4Address::parse(truncated_view);
  if (!truncated.has_value() || *truncated != IPv4Address{{1, 2, 3, 4}})
  {
    nc_warn("[net] parse() read past the end of the view, expected 1.2.3.4.");
    NC_TEST_FAIL;
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(ipv4_address_parse_unterminated_view_test)->name("Net IPv4Address Parse Unterminated View");

//==============================================================================
bool tcp_socket_create_and_close_test(unit_test::TestCtx& /*ctx*/)
{
  NC_TEST_ASSERT(!TCPSocket{}.is_valid());
  NC_TEST_ASSERT(close_socket(TCPSocket{}));

  NC_TEST_ASSERT(init());

  static constexpr u16 TEST_PORT = 27015;

  const std::optional<TCPSocket> socket = create_socket(IPv4Address::loopback(), TEST_PORT);
  if (!socket.has_value())
  {
    nc_warn("[net] create_socket() returned nullopt.");
    shutdown();
    NC_TEST_FAIL;
  }

  const bool handle_is_valid   = socket->is_valid();
  const bool address_preserved = socket->address == IPv4Address::loopback();
  const bool port_preserved    = socket->port == TEST_PORT;
  const bool close_succeeded   = close_socket(*socket);

  NC_TEST_ASSERT(shutdown());

  NC_TEST_ASSERT(handle_is_valid);
  NC_TEST_ASSERT(address_preserved);
  NC_TEST_ASSERT(port_preserved);
  NC_TEST_ASSERT(close_succeeded);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(tcp_socket_create_and_close_test)->name("Net TCPSocket Create And Close");

//==============================================================================
bool transfer_result_from_error_code_test(unit_test::TestCtx& /*ctx*/)
{
  struct TestCase
  {
    int            error_code;
    TransferResult expected;
  };

  const TestCase TEST_CASES[]
  {
    {WSAECONNRESET,   TransferResult::disconnected},
    {WSAECONNABORTED, TransferResult::disconnected},
    {WSAENETRESET,    TransferResult::disconnected},
    {WSAETIMEDOUT,    TransferResult::disconnected},
    {WSAESHUTDOWN,    TransferResult::disconnected},
    {WSAENOTSOCK,     TransferResult::error       },
    {WSAEINVAL,       TransferResult::error       },
    {WSAEFAULT,       TransferResult::error       },
    {0,               TransferResult::error       },
  };

  for (const TestCase& test_case : TEST_CASES)
  {
    const TransferResult result = get_transfer_result(test_case.error_code);
    if (result != test_case.expected)
    {
      nc_warn(
        "[net] get_transfer_result({}) returned {}, expected {}.",
        test_case.error_code,
        cast<u32>(result),
        cast<u32>(test_case.expected)
      );
      NC_TEST_FAIL;
    }
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(transfer_result_from_error_code_test)->name("Net TransferResult From Error Code");

}

#endif