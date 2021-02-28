#pragma once

#include "Types.h"

class Memory {
public:
	u8 Read(u16 Address) const;
	void Write(u8 Value, u16 Address);
};