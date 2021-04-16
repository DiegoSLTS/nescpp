#pragma once

#include <stdint.h>
#include <iomanip>
#include <sstream>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

static std::string ToHex(u8 value) {
	std::stringstream stream;
	stream << std::setfill('0') << std::setw(2) << std::hex << (u16)value;
	return stream.str();
}

static std::string ToHex(u16 value) {
	std::stringstream stream;
	stream << std::setfill('0') << std::setw(4) << std::hex << value;
	return stream.str();
}

// from http://graphics.stanford.edu/~seander/bithacks.html#BitReverseTable
// Generates a lookup table with each byte value reverted
// e.g. returns 0x10 for 0x08 (0b0001_0000 for 0b0000_1000)
static const u8 BitReverseTable256[256] =
{
#   define R2(n)     n,     n + 2*64,     n + 1*64,     n + 3*64
#   define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#   define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
	R6(0), R6(2), R6(1), R6(3)
};
