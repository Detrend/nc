// Project Nuclidean Source File
#include <cstring>
#include <engine/network/protocol.h>
#include <engine/network/tcp_socket.h>
#include <logging.h>
#include <optional>
#include <utility>
#include <variant>

#if NC_TESTS
#include <unit_test.h>
#include <algorithm>
#endif

namespace nc::net::protocol
{

//==============================================================================
namespace impl
{
  using MessageIndices = std::make_index_sequence<std::variant_size_v<MessageBase>>;
  using MessageFactory = Message(*)(std::span<const std::byte>);

  template <size_t... Is>
  constexpr auto make_sizes(std::index_sequence<Is...>)
  {
    return std::array<u64, sizeof...(Is)>{sizeof(std::variant_alternative_t<Is, MessageBase>)...};
  }

  template <typename T>
  Message message_from_bytes(std::span<const std::byte> bytes)
  {
    return Message{std::get<0>(from_bytes<T>(bytes))};
  }

  template <size_t... Is>
  constexpr auto make_factories(std::index_sequence<Is...>)
  {
    return std::array<MessageFactory, sizeof...(Is)>{&message_from_bytes<std::variant_alternative_t<Is, MessageBase>>...};
  }
}

// Maps message type index to message size.
inline constexpr auto MESSAGE_SIZES = impl::make_sizes(impl::MessageIndices{});
// Maps message type index to message factory.
inline constexpr auto MESSAGE_FACTORIES = impl::make_factories(impl::MessageIndices{});

//==============================================================================
TransferResult send(Connection& connection, const Message& message)
{
  TransferResult result;
  const u8 index = cast<u8>(message.index());
  auto visitor = [&connection, &result, index](const auto& message)
  {
    result = send_data(connection.socket, index, message);
  };

  std::visit(visitor, message);
  return result;
}

//==============================================================================
// Pop pending message already buffered on the connection.
// If no complete message is buffered return `std::nullopt`.
static std::optional<Message> pop_buffered_message(Connection& connection)
{
  if (connection.buffer_size < sizeof(u8))
  {
    // No data arrived yet.
    return std::nullopt;
  }

  const u8 message_type_index = cast<u8>(connection.buffer[0]);
  if (message_type_index >= std::variant_size_v<MessageBase>)
  {
    nc_warn("[net] received unknown message type - \"{}\"", message_type_index);
    connection.buffer_size--;
    std::memmove(connection.buffer.data(), connection.buffer.data() + sizeof(u8), connection.buffer_size);
    return std::nullopt;
  }

  const u32 message_size = cast<u32>(sizeof(u8)) + cast<u32>(MESSAGE_SIZES[message_type_index]);
  if (connection.buffer_size < message_size)
  {
    // Rest of the message has not yet arrived.
    return std::nullopt;
  }

  Message message = MESSAGE_FACTORIES[message_type_index](
    std::span{connection.buffer.data() + sizeof(u8), MESSAGE_SIZES[message_type_index]}
  );
  nc_assert(message_size <= connection.buffer_size);
  connection.buffer_size -= message_size;
  std::memmove(connection.buffer.data(), connection.buffer.data() + message_size, connection.buffer_size);

  return message;
}

//==============================================================================
std::pair<TransferResult, std::optional<Message>> pop_message(Connection& connection)
{
  auto [result, amount] = receive_raw_data(
    connection.socket,
    std::span{connection.buffer}.subspan(connection.buffer_size)
  );
  connection.buffer_size += cast<u32>(amount);
  nc_assert(connection.buffer_size <= connection.buffer.size());

  if (result != TransferResult::success)
    return {result, std::nullopt};

  return {result, pop_buffered_message(connection)};
}

}

#if NC_TESTS
//============================================================================//
//                                   TESTS                                    //
//============================================================================//
// CMD args:
// -unit_test -test_filter=Net.*
namespace nc::net::protocol
{

//==============================================================================
bool message_index_matches_tables_test(unit_test::TestCtx& /*ctx*/)
{
  NC_TEST_ASSERT(MESSAGE_SIZES.size() == std::variant_size_v<MessageBase>);
  NC_TEST_ASSERT(MESSAGE_FACTORIES.size() == std::variant_size_v<MessageBase>);

  auto check = [&]<typename T>(u8 expected_index) -> bool
  {
    const Message message = T{};
    if (message.index() != expected_index)
      return false;
    if (MESSAGE_SIZES[message.index()] != sizeof(T))
      return false;
    if (MESSAGE_FACTORIES[message.index()] == nullptr)
      return false;

    const auto bytes = to_bytes(T{});
    const Message rebuilt = MESSAGE_FACTORIES[message.index()](bytes);
    return rebuilt.index() == expected_index;
  };

  NC_TEST_ASSERT(check.template operator()<messages::NewPlayerData>     (0));
  NC_TEST_ASSERT(check.template operator()<messages::PlayerConnected>   (1));
  NC_TEST_ASSERT(check.template operator()<messages::PlayerDisconnected>(2));
  NC_TEST_ASSERT(check.template operator()<messages::PlayerInputs>      (3));
  NC_TEST_ASSERT(check.template operator()<messages::AllPlayersInputs>  (4));

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(message_index_matches_tables_test)->name("Net Protocol Message Index Matches Tables");

//==============================================================================
bool message_factory_round_trip_test(unit_test::TestCtx& /*ctx*/)
{
  {
    const messages::NewPlayerData sent{.player_id = cast<u8>(MAX_PLAYER_COUNT - 1)};
    const Message rebuilt = MESSAGE_FACTORIES[Message{sent}.index()](to_bytes(sent));
    const auto* got = std::get_if<messages::NewPlayerData>(&rebuilt);
    NC_TEST_ASSERT(got != nullptr);
    NC_TEST_ASSERT(got->player_id == cast<u8>(MAX_PLAYER_COUNT - 1));
  }

  {
    const messages::PlayerDisconnected sent{.player_id = INVALID_PLAYER_ID};
    const Message rebuilt = MESSAGE_FACTORIES[Message{sent}.index()](to_bytes(sent));
    const auto* got = std::get_if<messages::PlayerDisconnected>(&rebuilt);
    NC_TEST_ASSERT(got != nullptr);
    NC_TEST_ASSERT(got->player_id == INVALID_PLAYER_ID);
  }

  {
    const messages::PlayerInputs sent
    {
      .inputs = {.keys = 0xFFFF, .analog = {-180.0f, 89.5f}}
    };
    const Message rebuilt = MESSAGE_FACTORIES[Message{sent}.index()](to_bytes(sent));
    const auto* got = std::get_if<messages::PlayerInputs>(&rebuilt);
    NC_TEST_ASSERT(got != nullptr);
    NC_TEST_ASSERT(got->inputs.keys == 0xFFFF);
    NC_TEST_ASSERT(got->inputs.analog[0] == -180.0f);
    NC_TEST_ASSERT(got->inputs.analog[1] == 89.5f);
  }

  {
    messages::AllPlayersInputs sent{};
    for (u64 slot = 0; slot < MAX_PLAYER_COUNT; ++slot)
    {
      sent.inputs_array[slot].keys      = cast<PlayerKeyFlags>(1u << slot);
      sent.inputs_array[slot].analog[0] = cast<f32>(slot);
      sent.inputs_array[slot].analog[1] = -cast<f32>(slot);
    }

    const Message rebuilt = MESSAGE_FACTORIES[Message{sent}.index()](to_bytes(sent));
    const auto* got = std::get_if<messages::AllPlayersInputs>(&rebuilt);
    NC_TEST_ASSERT(got != nullptr);

    for (u64 slot = 0; slot < MAX_PLAYER_COUNT; ++slot)
    {
      NC_TEST_ASSERT(got->inputs_array[slot].keys      == cast<PlayerKeyFlags>(1u << slot));
      NC_TEST_ASSERT(got->inputs_array[slot].analog[0] ==  cast<f32>(slot));
      NC_TEST_ASSERT(got->inputs_array[slot].analog[1] == -cast<f32>(slot));
    }
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(message_factory_round_trip_test)->name("Net Protocol Message Factory Round Trip");

//==============================================================================
bool connection_buffer_fits_largest_message_test(unit_test::TestCtx& /*ctx*/)
{
  const u64 largest_message = *std::max_element(MESSAGE_SIZES.begin(), MESSAGE_SIZES.end());

  const Connection connection{};
  NC_TEST_ASSERT(connection.buffer.size() >= sizeof(u8) + largest_message);
  NC_TEST_ASSERT(connection.buffer_size == 0);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(connection_buffer_fits_largest_message_test)->name("Net Protocol Connection Buffer Fits Largest Message");

//==============================================================================
bool message_process_dispatch_test(unit_test::TestCtx& /*ctx*/)
{
  enum class Handled : u8
  {
    none,
    new_player_data,
    player_connected,
    player_disconnected,
    fallback,
  };

  Handled handled        = Handled::none;
  u8      seen_player_id = INVALID_PLAYER_ID;

  auto dispatch = [&](const Message& message)
  {
    handled        = Handled::none;
    seen_player_id = INVALID_PLAYER_ID;

    message.process(
      [&](const messages::NewPlayerData& payload)
      {
        handled        = Handled::new_player_data;
        seen_player_id = payload.player_id;
      },
      [&](const messages::PlayerConnected& payload)
      {
        handled        = Handled::player_connected;
        seen_player_id = payload.player_id;
      },
      [&](const messages::PlayerDisconnected& payload)
      {
        handled        = Handled::player_disconnected;
        seen_player_id = payload.player_id;
      },
      [&](const auto&){ handled = Handled::fallback; }
    );
  };

  dispatch(Message{messages::NewPlayerData{.player_id = 3}});
  NC_TEST_ASSERT(handled == Handled::new_player_data);
  NC_TEST_ASSERT(seen_player_id == 3);

  dispatch(Message{messages::PlayerConnected{.player_id = 5}});
  NC_TEST_ASSERT(handled == Handled::player_connected);
  NC_TEST_ASSERT(seen_player_id == 5);

  dispatch(Message{messages::PlayerDisconnected{.player_id = 0}});
  NC_TEST_ASSERT(handled == Handled::player_disconnected);
  NC_TEST_ASSERT(seen_player_id == 0);

  dispatch(Message{messages::PlayerInputs{}});
  NC_TEST_ASSERT(handled == Handled::fallback);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(message_process_dispatch_test)->name("Net Protocol Message Process Dispatch");

//==============================================================================
static void push_raw_bytes(Connection& connection, std::span<const std::byte> bytes)
{
  nc_assert(connection.buffer_size + bytes.size() <= connection.buffer.size());
  std::memcpy(connection.buffer.data() + connection.buffer_size, bytes.data(), bytes.size());
  connection.buffer_size += cast<u32>(bytes.size());
}

//==============================================================================
template <typename T>
static auto make_wire_bytes(const T& payload)
{
  return to_bytes(cast<u8>(Message{payload}.index()), payload);
}

//==============================================================================
bool pop_buffered_message_empty_buffer_test(unit_test::TestCtx& /*ctx*/)
{
  Connection connection{};

  NC_TEST_ASSERT(!pop_buffered_message(connection).has_value());
  NC_TEST_ASSERT(connection.buffer_size == 0);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(pop_buffered_message_empty_buffer_test)->name("Net Protocol Pop Buffered Message Empty Buffer");

//==============================================================================
bool pop_buffered_message_partial_message_test(unit_test::TestCtx& /*ctx*/)
{
  const messages::PlayerInputs sent
  {
    .inputs = {.keys = 0x1234, .analog = {12.5f, -7.25f}}
  };
  const auto wire = make_wire_bytes(sent);

  Connection connection{};

  push_raw_bytes(connection, std::span{wire}.subspan(0, 1));
  NC_TEST_ASSERT(!pop_buffered_message(connection).has_value());
  NC_TEST_ASSERT(connection.buffer_size == 1);

  push_raw_bytes(connection, std::span{wire}.subspan(1, wire.size() - 2));
  NC_TEST_ASSERT(!pop_buffered_message(connection).has_value());
  NC_TEST_ASSERT(connection.buffer_size == wire.size() - 1);

  push_raw_bytes(connection, std::span{wire}.subspan(wire.size() - 1));
  const std::optional<Message> popped = pop_buffered_message(connection);
  NC_TEST_ASSERT(popped.has_value());
  NC_TEST_ASSERT(connection.buffer_size == 0);

  const auto* got = std::get_if<messages::PlayerInputs>(&*popped);
  NC_TEST_ASSERT(got != nullptr);
  NC_TEST_ASSERT(got->inputs.keys == 0x1234);
  NC_TEST_ASSERT(got->inputs.analog[0] ==  12.5f);
  NC_TEST_ASSERT(got->inputs.analog[1] == -7.25f);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(pop_buffered_message_partial_message_test)->name("Net Protocol Pop Buffered Message Partial Message");

//==============================================================================
bool pop_buffered_message_two_messages_test(unit_test::TestCtx& /*ctx*/)
{
  const auto first  = make_wire_bytes(messages::PlayerConnected{.player_id = 2});
  const auto second = make_wire_bytes(messages::PlayerDisconnected{.player_id = 5});

  Connection connection{};
  push_raw_bytes(connection, first);
  push_raw_bytes(connection, second);

  const std::optional<Message> popped_first = pop_buffered_message(connection);
  NC_TEST_ASSERT(popped_first.has_value());
  const auto* connected = std::get_if<messages::PlayerConnected>(&*popped_first);
  NC_TEST_ASSERT(connected != nullptr);
  NC_TEST_ASSERT(connected->player_id == 2);
  NC_TEST_ASSERT(connection.buffer_size == second.size());

  const std::optional<Message> popped_second = pop_buffered_message(connection);
  NC_TEST_ASSERT(popped_second.has_value());
  const auto* disconnected = std::get_if<messages::PlayerDisconnected>(&*popped_second);
  NC_TEST_ASSERT(disconnected != nullptr);
  NC_TEST_ASSERT(disconnected->player_id == 5);
  NC_TEST_ASSERT(connection.buffer_size == 0);

  NC_TEST_ASSERT(!pop_buffered_message(connection).has_value());

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(pop_buffered_message_two_messages_test)->name("Net Protocol Pop Buffered Message Two Messages");

//==============================================================================
bool pop_buffered_message_unknown_type_resync_test(unit_test::TestCtx& /*ctx*/)
{
  const std::byte garbage[]{std::byte{cast<u8>(std::variant_size_v<MessageBase>)}};
  const auto      valid = make_wire_bytes(messages::PlayerConnected{.player_id = 7});

  Connection connection{};
  push_raw_bytes(connection, garbage);
  push_raw_bytes(connection, valid);

  NC_TEST_ASSERT(!pop_buffered_message(connection).has_value());
  NC_TEST_ASSERT(connection.buffer_size == valid.size());

  const std::optional<Message> popped = pop_buffered_message(connection);
  NC_TEST_ASSERT(popped.has_value());
  const auto* got = std::get_if<messages::PlayerConnected>(&*popped);
  NC_TEST_ASSERT(got != nullptr);
  NC_TEST_ASSERT(got->player_id == 7);
  NC_TEST_ASSERT(connection.buffer_size == 0);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(pop_buffered_message_unknown_type_resync_test)->name("Net Protocol Pop Buffered Message Unknown Type Resync");

//==============================================================================
bool pop_buffered_message_largest_message_test(unit_test::TestCtx& /*ctx*/)
{
  messages::AllPlayersInputs sent{};
  for (u64 slot = 0; slot < MAX_PLAYER_COUNT; ++slot)
  {
    sent.inputs_array[slot].keys      = cast<PlayerKeyFlags>(slot + 1);
    sent.inputs_array[slot].analog[0] =  cast<f32>(slot);
    sent.inputs_array[slot].analog[1] = -cast<f32>(slot);
  }

  const auto wire = make_wire_bytes(sent);

  Connection connection{};
  NC_TEST_ASSERT(wire.size() <= connection.buffer.size());
  push_raw_bytes(connection, wire);

  const std::optional<Message> popped = pop_buffered_message(connection);
  NC_TEST_ASSERT(popped.has_value());
  NC_TEST_ASSERT(connection.buffer_size == 0);

  const auto* got = std::get_if<messages::AllPlayersInputs>(&*popped);
  NC_TEST_ASSERT(got != nullptr);
  for (u64 slot = 0; slot < MAX_PLAYER_COUNT; ++slot)
  {
    NC_TEST_ASSERT(got->inputs_array[slot].keys      == cast<PlayerKeyFlags>(slot + 1));
    NC_TEST_ASSERT(got->inputs_array[slot].analog[0] ==  cast<f32>(slot));
    NC_TEST_ASSERT(got->inputs_array[slot].analog[1] == -cast<f32>(slot));
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(pop_buffered_message_largest_message_test)->name("Net Protocol Pop Buffered Message Largest Message");

}

#endif