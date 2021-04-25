#include <iostream>

#include "Mapper.h"

void Header::Print() {
	std::cout << "PrgROMUnits: " << PrgROMUnits << std::endl;
	std::cout << "ChrROMUnits: " << ChrROMUnits << std::endl;
	std::cout << "Mirroring: " << (u16)Mirroring << std::endl;
	std::cout << "HasBattery: " << HasBattery << std::endl;
	std::cout << "HasTrainer: " << HasTrainer << std::endl;
	std::cout << "IgnoreMirroring: " << IgnoreMirroring << std::endl;
	std::cout << "VSUnisystem: " << VSUnisystem << std::endl;
	std::cout << "PlayChoice: " << PlayChoice << std::endl;
	std::cout << "Format: " << Format << std::endl;
	std::cout << "Console: " << Console << std::endl;
	std::cout << "PrgRAMUnits: " << (u16)PrgRAMUnits << std::endl;
	std::cout << "Submapper: " << (u16)Submapper << std::endl;
	std::cout << "TV: " << TV << std::endl;
	std::cout << "PrgRAMPresent: " << PrgRAMPresent << std::endl;
	std::cout << "HasBusConflicts: " << HasBusConflicts << std::endl;
	std::cout << "PrgRAMShiftCount: " << (u16)PrgRAMShiftCount << std::endl;
	std::cout << "PrgNVRAMShiftCount: " << (u16)PrgNVRAMShiftCount << std::endl;
	std::cout << "ChrRAMShiftCount: " << (u16)ChrRAMShiftCount << std::endl;
	std::cout << "ChrNVRAMShiftCount: " << (u16)ChrNVRAMShiftCount << std::endl;
	std::cout << "Timing: " << Timing << std::endl;
	std::cout << "VSPPUType: " << (u16)VSPPUType << std::endl;
	std::cout << "VSHardwareType: " << (u16)VSHardwareType << std::endl;
	std::cout << "ExtendedConsoleType: " << (u16)ExtendedConsoleType << std::endl;
	std::cout << "MiscellaneousRoms: " << (u16)MiscellaneousRoms << std::endl;
	std::cout << "ExpansionDevice: " << ExpansionDevice << std::endl;
	std::cout << "Mapper: " << Mapper << std::endl;
	std::cout << "Padding: " << Padding << std::endl;
}

Mapper::Mapper(const Header& header, const char* fileContent) : header(header) {
	InitArrays(fileContent);
}

Mapper::~Mapper() {
	if (prgRom != nullptr)
		delete[] prgRom;
	if (chrRom != nullptr)
		delete[] chrRom;
	if (prgRam != nullptr)
		delete[] prgRam;
	if (chrRam != nullptr)
		delete[] chrRam;
	if (prgNVRam != nullptr)
		delete[] prgNVRam;
	if (chrNVRam != nullptr)
		delete[] chrNVRam;
}

void Mapper::InitArrays(const char* fileContent) {
	u32 prgROMSize = header.PrgROMUnits * 0x4000;
	u32 chrROMSize = header.ChrROMUnits * 0x2000;

	u32 prgRAMSize = 0; // TODO iNES
	u32 prgNVRAMSize = 0;
	u32 chrRAMSize = 0;
	u32 chrNVRAMSize = 0;

	if (header.Format == HeaderFormat::NES2) {
		if ((header.PrgROMUnits & 0xF0) == 0xF00) {
			u8 m = (header.PrgROMUnits & 0x03) * 2 + 1;
			u8 e = (header.PrgROMUnits & 0xFC) >> 2;
			prgROMSize = (2 << e) * m;
		}

		if ((header.ChrROMUnits & 0xF0) == 0xF00) {
			u8 m = (header.ChrROMUnits & 0x03) * 2 + 1;
			u8 e = (header.ChrROMUnits & 0xFC) >> 2;
			chrROMSize = (2 << e) * m;
		}

		if (header.PrgRAMShiftCount > 0)
			prgRAMSize = 64 << header.PrgRAMShiftCount;
		if (header.PrgNVRAMShiftCount > 0)
			prgNVRAMSize = 64 << header.PrgNVRAMShiftCount;
		if (header.ChrRAMShiftCount > 0)
			chrRAMSize = 64 << header.ChrRAMShiftCount;
		if (header.ChrNVRAMShiftCount > 0)
			chrNVRAMSize = 64 << header.ChrNVRAMShiftCount;
	}
	printf("romsize = %d", prgROMSize);
	if (prgROMSize > 0)
		prgRom = new u8[prgROMSize];
	if (chrROMSize > 0)
		chrRom = new u8[chrROMSize];
	if (prgRAMSize > 0)
		prgRam = new u8[prgRAMSize];
	if (prgNVRAMSize > 0)
		prgNVRam = new u8[prgNVRAMSize];
	if (chrRAMSize > 0)
		chrRam = new u8[chrRAMSize];
	if (chrNVRAMSize > 0)
		chrNVRam = new u8[chrNVRAMSize];

	u32 romStart = 0x10;
	if (header.HasTrainer)
		romStart += 512;

	if (prgRom != nullptr)
		memcpy(prgRom, fileContent + romStart, prgROMSize);
	if (chrRom != nullptr)
		memcpy(chrRom, fileContent + romStart + prgROMSize, chrROMSize);
}

Mapper0::Mapper0(const Header& header, const char* fileContent) : Mapper(header, fileContent) {
	if (chrRom == nullptr)
		chrRam = new u8[8 * 1024];
}

u8 Mapper0::Read(u16 address) {
	if (address < 0x6000)
		return 0; // invalid?
	else if (address < 0x8000) {
		if (prgRam != nullptr)
			return prgRam[address - 0x6000];
	} else {
		address -= 0x8000;
		if (header.PrgROMUnits == 1)
			address &= 0x3FFF; // faster than % 0x4000
		return prgRom[address];
	}

	return 0;
}

void Mapper0::Write(u8 value, u16 address) {
	if (address >= 0x6000 && address < 0x8000 && prgRam != nullptr)
		prgRam[address - 0x6000] = value;
	else if (address < 8 * 1024 && chrRam != nullptr)
		chrRam[address] = value;
}

u8 Mapper0::ReadChr(u16 address) {
	if (address < 0x2000) {
		if (chrRom != nullptr)
			return chrRom[address];
		if (chrRam != nullptr)
			return chrRam[address];
	}

	return 0;
}

MirroringMode Mapper0::GetMirroring() const {
	return header.Mirroring;
}

Mapper1::Mapper1(const Header& header, const char* fileContent) : Mapper(header, fileContent) {
	
}

u8 Mapper1::Read(u16 address) {
	if (address < 0x6000)
		return 0; // invalid?
	else if (address < 0x8000) {
		if (prgRam != nullptr && (prgBank & 0x10) > 0)
			return prgRam[address - 0x6000];
	} else {
		u8 bank = 0;
		u16 offset = address - 0x8000;
		u8 mode = (control >> 2) & 0x03;
		switch (mode) {
		case 0:
		case 1:
			bank = (prgBank & 0x0E) % header.PrgROMUnits;
			break;
		case 2:
			if (address < 0xC000)
				bank = 0;
			else {
				bank = (prgBank & 0x0F) % header.PrgROMUnits;
				offset -= 0x4000;
			}
		case 3:
			if (address < 0xC000)
				bank = (prgBank & 0x0F) % header.PrgROMUnits;
			else {
				bank = header.PrgROMUnits - 1;
				offset -= 0x4000;
			}
		}

		return prgRom[bank * 0x4000 + offset];
	}

	return 0;
}

void Mapper1::Write(u8 value, u16 address) {
	if (address >= 0x6000 && address < 0x8000 && prgRam != nullptr)
		prgRam[address - 0x6000] = value;
	else if (address >= 0x8000 && address < 0xA000) {
		if ((value & 0x80) > 0)
			loadRegister = 0x10;
		else {
			u8 out = loadRegister & 0x01;
			u8 in = (value && 0x01) << 4;
			loadRegister >>= 1;
			loadRegister |= in;
			if (out == 1) {
				switch ((address >> 12) & 0x03) {
				case 0:
					control = loadRegister;
					break;
				case 1:
					chrBank0 = loadRegister;
					break;
				case 2:
					chrBank1 = loadRegister;
					break;
				case 3:
					prgBank = loadRegister;
					break;
				}
				loadRegister = 0x10;
			}
		}
	}
}

u8 Mapper1::ReadChr(u16 address) {
	if (address < 0x2000) {
		if ((control & 0x10) == 0) {
			u8 banksCount = header.ChrROMUnits * 2;
			u8 bank = (chrBank0 & 0xFE) % banksCount;
			return chrRom[bank * 0x1000 + address];
		}
		
		if (address < 0x1000) {
			u8 banksCount = header.ChrROMUnits * 2;
			u8 bank = chrBank0 % banksCount;
			return chrRom[bank * 0x1000 + address];
		}

		u8 banksCount = header.ChrROMUnits * 2;
		u8 bank = chrBank1 % banksCount;
		return chrRom[bank * 0x1000 + (address - 0x1000)];
	}

	/*if (chrRam != nullptr)
		return chrRam[address];*/

	return 0;
}

MirroringMode Mapper1::GetMirroring() const {
	switch (control & 0x03) {
	case 0:
		return MirroringMode::OneScreenLower;
	case 1:
		return MirroringMode::OneScreenUpper;
	case 2:
		return MirroringMode::Vertical;
	case 3:
		return MirroringMode::Horizontal;
	}
}