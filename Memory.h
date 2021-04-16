#pragma once

#include "Types.h"

class Cartridge;
class Controller;
class CPU;
class PPU;

class Memory {
public:
	Memory(Cartridge* cart);

	u8 Read(u16 Address) const;
	void Write(u8 Value, u16 Address);

	CPU* cpu = nullptr;
	PPU* ppu = nullptr;
	Controller* controller = nullptr;

private:
	u8 ram[0x0800] = { 0 };
	u8 ioRegisters[0x20] = { 0 };
	Cartridge* cart = nullptr;
};