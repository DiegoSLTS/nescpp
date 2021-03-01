#pragma once

#include "Types.h"

class PPU {
public:
	u8 Read(u16 address);
	void Write(u8 value, u16 address);
};