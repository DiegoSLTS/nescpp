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