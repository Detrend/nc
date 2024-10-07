// Project Nucledian Source File

// Info: basic types used along the whole codebase,
// both in engine and in the game
#pragma once

#include <cstdint>

namespace nc
{

using cstr = const char*;
using wstr = const wchar_t*;

using u8 = std::uint8_t;  // unsigned 8 bit
using s8 = std::int8_t;   // signed 8 bit
using byte = u8;

using u16 = std::uint16_t;
using s16 = std::int16_t;

using u32 = std::uint32_t;
using s32 = std::int32_t;

using u64 = std::uint64_t;
using s64 = std::int64_t;

using f32 = float;
using f64 = double;

}
