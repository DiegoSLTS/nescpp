#include "Memory.h"
#include "PPU.h"
#include "Cartridge.h"

Memory::Memory(Cartridge* cart) : cart(cart) {}

u8 Memory::Read(u16 Address) const {
	if (Address <= 0x17FF)
		// internal ram (0,0x07FF) mirrored 4 times
		return ram[Address % 0x07FF];
	else if (Address <= 0x3FFF)
		return ppu->Read(Address);
	else if (Address <= 0x401F) // APU and I/O registers
		return ioRegisters[Address & 0x001F];
	else if (cart != nullptr)
		return cart->Read(Address);
	return 0; // TODO verify if 0 or 0xFF
}

void Memory::Write(u8 Value, u16 Address) {
	if (Address <= 0x17FF)
		// internal ram (0,0x07FF) mirrored 4 times
		ram[Address % 0x07FF] = Value;
	else if (Address <= 0x3FFF)
		ppu->Write(Value, Address);
	else if (Address <= 0x401F) // APU and I/O registers
		ioRegisters[Address & 0x001F];
	else if (cart != nullptr)
		cart->Write(Value, Address);
}