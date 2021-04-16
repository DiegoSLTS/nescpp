#include "Memory.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Controller.h"

Memory::Memory(Cartridge* cart) : cart(cart) {}

u8 Memory::Read(u16 Address) const {
	if (Address < 0x2000)
		// internal ram (0,0x07FF) mirrored 4 times
		return ram[Address & 0x07FF];
	else if (Address < 0x4000)
		return ppu->Read(Address);
	else if (Address < 0x4020) {// APU and I/O registers
		if (Address == 0x4016 || Address == 0x4017)
			return controller->Read(Address);
		return ioRegisters[Address & 0x001F];
	} else if (cart != nullptr)
		return cart->Read(Address);
	return 0; // TODO verify if 0 or 0xFF
}

void Memory::Write(u8 Value, u16 Address) {
	if (Address < 0x2000)
		// internal ram (0,0x07FF) mirrored 4 times
		ram[Address & 0x07FF] = Value;
	else if (Address < 0x4000)
		ppu->Write(Value, Address);
	else if (Address < 0x4020) {// APU and I/O registers
		if (Address == 0x4014)
			cpu->StartOAMDMA(Value);
		else if (Address == 0x4016 || Address == 0x4017) {
			controller->Write(Value, Address);
			return;
		}
		ioRegisters[Address & 0x001F] = Value;
	} else if (cart != nullptr)
		cart->Write(Value, Address);
}