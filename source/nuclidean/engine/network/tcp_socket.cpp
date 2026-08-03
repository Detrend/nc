// Project Nuclidean Source File
#include <winsock2.h> // must be included before anything else includes windows.h

#include <engine/network/tcp_socket.h>

#include <common.h>
#include <logging.h>
#include <types.h>

#include <bit>
#include <cstring>
#include <format>
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
// Set socket to no blocking mode.
static bool set_blocking_mode(SOCKET handle, bool should_block);
// Get error message based on system category error code.
static std::string get_error_message(int error_code);
// Get last WSA error message.
static std::string get_last_wsa_error();
// Get transfer result from error code.
static TransferResult get_transfer_result(int error_code);

static_assert(TCPSocket::invalid_handle == INVALID_SOCKET);

//==============================================================================
std::string IPv4Address::to_string() const
{
  return std::format("{}.{}.{}.{}", octets[0], octets[1], octets[2], octets[3]);
}

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
std::optional<TCPSocket> create_socket(IPv4Address address, u16 port)
{
  const SOCKET handle = socket(AF_INET, SOCK_STREAM, 0);
  if (handle == INVALID_SOCKET)
  {
    nc_crit("[net] socket() failed: \"{}\"", get_last_wsa_error());
    return std::nullopt;
  }

  if (!set_no_delay(handle)) return std::nullopt;

  return TCPSocket
  {
    .handle = cast<u64>(handle),
    .address = address,
    .port = port,
  };
}

//==============================================================================
bool close_socket(TCPSocket& socket)
{
  if (!socket.is_valid())
  {
    return true;
  }

  if (closesocket(cast<SOCKET>(socket.handle)) == SOCKET_ERROR)
  {
    socket.handle = TCPSocket::invalid_handle;
    nc_warn("[net] closesocket() failed: \"{}\"", get_last_wsa_error());
    return false;
  }

  socket.handle = TCPSocket::invalid_handle;

  return true;
}

//==============================================================================
bool start_listening(TCPSocket server_socket, bool blocking)
{
  const SOCKET server_handle = cast<SOCKET>(server_socket.handle);

  BOOL enabled = TRUE;
  const int setsockopt_result = setsockopt(
    server_handle,
    SOL_SOCKET, 
    SO_EXCLUSIVEADDRUSE, 
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

  if (!set_blocking_mode(server_handle, blocking))
    nc_warn("[net] failed to set blocking mode");

  return true;
}

//==============================================================================
bool is_accept_pending(TCPSocket server_socket)
{
  WSAPOLLFD poll{};
  poll.fd = cast<SOCKET>(server_socket.handle);
  poll.events = POLLRDNORM;

  const int n = WSAPoll(&poll, 1, 0);
  
  return n > 0 && (poll.revents & POLLRDNORM);
}

//==============================================================================
std::optional<TCPSocket> accept_client(TCPSocket server_socket, bool blocking)
{
  const SOCKET server_handle = cast<SOCKET>(server_socket.handle);

  sockaddr_in client_address{};
  int client_address_length = sizeof(client_address);

  const SOCKET client_handle = accept(server_handle, recast<sockaddr*>(&client_address), &client_address_length);
  if (client_handle == INVALID_SOCKET)
  {
    nc_warn("[net] accept() failed: \"{}\"", get_last_wsa_error());
    return std::nullopt;
  }

  if (!set_no_delay(client_handle))
  {
    TCPSocket client_socket = TCPSocket{.handle = cast<u64>(client_handle)};
    close_socket(client_socket);
    return std::nullopt;
  }

  if (!set_blocking_mode(client_handle, blocking))
    nc_warn("[net] failed to set blocking mode");

  return TCPSocket
  {
    .handle = cast<u64>(client_handle),
    .address = IPv4Address{.octets = std::bit_cast<std::array<u8, 4>>(client_address.sin_addr.s_addr)},
    .port = ntohs(client_address.sin_port),
  };
}

//==============================================================================
bool connect(TCPSocket client_socket, bool blocking)
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

  if (!set_blocking_mode(client_handle, blocking))
    nc_warn("[net] failed to set blocking mode");

  return true;
}

//==============================================================================
TransferResult send_raw_data(TCPSocket socket, std::span<const std::byte> data_to_send)
{
  const SOCKET handle = cast<SOCKET>(socket.handle);

  u64 sent = 0;
  while (sent < data_to_send.size())
  {
    // send is allowed to send fewer bytes than asked
    const int result = send(
      handle, 
      recast<const char*>(data_to_send.data() + sent), 
      cast<int>(data_to_send.size() - sent), 
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
std::pair<TransferResult, size_t> receive_raw_data(TCPSocket socket, std::span<std::byte> data_to_receive)
{
  const SOCKET handle = cast<SOCKET>(socket.handle);

  u32 received = 0;
  while (received < data_to_receive.size())
  {
    // recv is allowed to receive fewer bytes than asked
    const int result = recv(
      handle,
      recast<char*>(data_to_receive.data() + received),
      cast<int>(data_to_receive.size() - received),
      0
    );

    if (result == 0)
      return {TransferResult::disconnected, received};
    if (result == SOCKET_ERROR)
    {
      const int error = WSAGetLastError();

      if (error == WSAEWOULDBLOCK)
        return {TransferResult::success, received};

      nc_crit("[net] recv() failed: \"{}\"", get_error_message(error));
      return {get_transfer_result(error), received};
    }

    received += result;
  }

  return {TransferResult::success, received};
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
static bool set_blocking_mode(SOCKET handle, bool should_block)
{
  u_long mode = should_block ? 0 : 1;
  if (ioctlsocket(handle, FIONBIO, &mode) == SOCKET_ERROR)
  {
    nc_crit("[net] ioctlsocket() failed: \"{}\"", get_last_wsa_error());
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
    case WSAEINTR:
    case WSAENOTSOCK:
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
  TCPSocket empty_socket{};
  NC_TEST_ASSERT(!empty_socket.is_valid());
  NC_TEST_ASSERT(close_socket(empty_socket));

  NC_TEST_ASSERT(init());

  static constexpr u16 TEST_PORT = 27015;

  const std::optional<TCPSocket> maybe_socket = create_socket(IPv4Address::loopback(), TEST_PORT);
  if (!maybe_socket.has_value())
  {
    nc_warn("[net] create_socket() returned nullopt.");
    shutdown();
    NC_TEST_FAIL;
  }

  const bool handle_is_valid   = maybe_socket->is_valid();
  const bool address_preserved = maybe_socket->address == IPv4Address::loopback();
  const bool port_preserved    = maybe_socket->port == TEST_PORT;
  TCPSocket socket = *maybe_socket;
  const bool close_succeeded   = close_socket(socket);

  NC_TEST_ASSERT(shutdown());

  NC_TEST_ASSERT(handle_is_valid);
  NC_TEST_ASSERT(address_preserved);
  NC_TEST_ASSERT(port_preserved);
  NC_TEST_ASSERT(close_succeeded);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(tcp_socket_create_and_close_test)->name("Net TCPSocket Create And Close");

//==============================================================================
bool ipv4_address_to_string_test(unit_test::TestCtx& /*ctx*/)
{
  struct TestCase
  {
    IPv4Address      input;
    std::string_view expected;
  };

  const TestCase TEST_CASES[]
  {
    {IPv4Address::any(),                "0.0.0.0"        },
    {IPv4Address::loopback(),           "127.0.0.1"      },
    {IPv4Address{{1, 2, 3, 4}},         "1.2.3.4"        },
    {IPv4Address{{192, 168, 0, 255}},   "192.168.0.255"  },
    {IPv4Address{{255, 255, 255, 255}}, "255.255.255.255"},
  };

  for (const TestCase& test_case : TEST_CASES)
  {
    const std::string result = test_case.input.to_string();
    if (result != test_case.expected)
    {
      nc_warn("[net] to_string() returned \"{}\", expected \"{}\".", result, test_case.expected);
      NC_TEST_FAIL;
    }

    const std::optional<IPv4Address> reparsed = IPv4Address::parse(result);
    if (!reparsed.has_value() || *reparsed != test_case.input)
    {
      nc_warn("[net] parse(to_string()) did not round trip for \"{}\".", result);
      NC_TEST_FAIL;
    }
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(ipv4_address_to_string_test)->name("Net IPv4Address To String");

//==============================================================================
static_assert(to_bytes(u8{0x01}, u8{0x02}).size() == 2);
static_assert(to_bytes(u8{0x01}, u8{0x02})[0] == std::byte{0x01});
static_assert(to_bytes(u8{0x01}, u8{0x02})[1] == std::byte{0x02});

//==============================================================================
bool to_bytes_size_and_order_test(unit_test::TestCtx& /*ctx*/)
{
  const auto single = to_bytes(u32{0});
  NC_TEST_ASSERT(single.size() == sizeof(u32));

  const auto mixed = to_bytes(u8{0}, u16{0}, u32{0}, u64{0});
  NC_TEST_ASSERT(mixed.size() == sizeof(u8) + sizeof(u16) + sizeof(u32) + sizeof(u64));

  const auto ordered = to_bytes(u8{0xAA}, u8{0xBB}, u8{0xCC});
  NC_TEST_ASSERT(ordered.size() == 3);
  NC_TEST_ASSERT(ordered[0] == std::byte{0xAA});
  NC_TEST_ASSERT(ordered[1] == std::byte{0xBB});
  NC_TEST_ASSERT(ordered[2] == std::byte{0xCC});

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(to_bytes_size_and_order_test)->name("Net To Bytes Size And Order");

//==============================================================================
bool byte_conversion_round_trip_test(unit_test::TestCtx& /*ctx*/)
{
  {
    const auto bytes = to_bytes(u8{0}, u16{0}, u32{0}, u64{0});
    const auto [a, b, c, d] = from_bytes<u8, u16, u32, u64>(bytes);
    NC_TEST_ASSERT(a == 0);
    NC_TEST_ASSERT(b == 0);
    NC_TEST_ASSERT(c == 0);
    NC_TEST_ASSERT(d == 0);
  }

  {
    const auto bytes = to_bytes(u8{0xFF}, u16{0xFFFF}, u32{0xFFFFFFFFu}, u64{~0ull});
    const auto [a, b, c, d] = from_bytes<u8, u16, u32, u64>(bytes);
    NC_TEST_ASSERT(a == 0xFF);
    NC_TEST_ASSERT(b == 0xFFFF);
    NC_TEST_ASSERT(c == 0xFFFFFFFFu);
    NC_TEST_ASSERT(d == ~0ull);
  }

  {
    const auto bytes = to_bytes(s8{-1}, s16{-32768}, s32{-2147483647 - 1}, s64{-1});
    const auto [a, b, c, d] = from_bytes<s8, s16, s32, s64>(bytes);
    NC_TEST_ASSERT(a == -1);
    NC_TEST_ASSERT(b == -32768);
    NC_TEST_ASSERT(c == -2147483647 - 1);
    NC_TEST_ASSERT(d == -1);
  }

  {
    const auto bytes = to_bytes(f32{-0.5f}, f64{0.0}, f32{3.4028235e38f});
    const auto [a, b, c] = from_bytes<f32, f64, f32>(bytes);
    NC_TEST_ASSERT(a == -0.5f);
    NC_TEST_ASSERT(b == 0.0);
    NC_TEST_ASSERT(c == 3.4028235e38f);
  }

  {
    const auto bytes = to_bytes(u8{0x11});
    const auto [only] = from_bytes<u8>(bytes);
    NC_TEST_ASSERT(only == 0x11);
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(byte_conversion_round_trip_test)->name("Net Byte Conversion Round Trip");

//==============================================================================
bool byte_conversion_wire_types_round_trip_test(unit_test::TestCtx& /*ctx*/)
{
  enum class TestMessageType : u8
  {
    first = 0,
    last  = 255,
  };

  {
    const auto bytes = to_bytes(TestMessageType::last, u8{7});
    const auto [type, player_id] = from_bytes<TestMessageType, u8>(bytes);
    NC_TEST_ASSERT(bytes.size() == sizeof(u8) + sizeof(u8));
    NC_TEST_ASSERT(type == TestMessageType::last);
    NC_TEST_ASSERT(player_id == 7);
  }

  struct PaddedPayload
  {
    u16 keys;
    f32 analog[2];
  };
  static_assert(sizeof(PaddedPayload) > sizeof(u16) + sizeof(f32) * 2);

  {
    const PaddedPayload sent{.keys = 0xBEEF, .analog = {-1.25f, 8.5f}};
    const auto bytes = to_bytes(sent);
    const auto [got] = from_bytes<PaddedPayload>(bytes);
    NC_TEST_ASSERT(bytes.size() == sizeof(PaddedPayload));
    NC_TEST_ASSERT(got.keys == sent.keys);
    NC_TEST_ASSERT(got.analog[0] == sent.analog[0]);
    NC_TEST_ASSERT(got.analog[1] == sent.analog[1]);
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(byte_conversion_wire_types_round_trip_test)->name("Net Byte Conversion Wire Types Round Trip");

//==============================================================================
bool from_bytes_reads_from_span_start_test(unit_test::TestCtx& /*ctx*/)
{
  const auto bytes = to_bytes(u16{0xBEEF}, u32{0xDEADBEEFu}, u8{0x42});
  const std::span<const std::byte> whole{bytes};

  const auto [leading] = from_bytes<u16>(whole);
  NC_TEST_ASSERT(leading == 0xBEEF);

  const auto [middle] = from_bytes<u32>(whole.subspan(sizeof(u16)));
  NC_TEST_ASSERT(middle == 0xDEADBEEFu);

  const auto [trailing] = from_bytes<u8>(whole.subspan(sizeof(u16) + sizeof(u32)));
  NC_TEST_ASSERT(trailing == 0x42);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(from_bytes_reads_from_span_start_test)->name("Net From Bytes Reads From Span Start");

}

#endif