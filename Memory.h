#pragma once

#include "Types.h"

class Cartridge;
class CPU;
class PPU;

class Memory {
public:
	Memory(Cartridge* cart);

	u8 Read(u16 Address) const;
	void Write(u8 Value, u16 Address);

	CPU* cpu = nullptr;

private:
	u8 ram[0x0800] = { 0 };
	u8 ioRegisters[0x20] = { 0 };

	PPU* ppu = nullptr;
	Cartridge* cart = nullptr;
};