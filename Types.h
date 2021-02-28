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