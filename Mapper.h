#pragma once

#include "Types.h"

enum MirroringMode {
	Horizontal,
	Vertical
};

enum HeaderFormat {
	iNESv1,
	NES2
};

enum TVMode {
	NTSC,
	PAL
};

enum ConsoleType {
	NES,
	VSSystem,
	Playchoice,
	Other
};

enum CPUTiming {
	RP2C02, // NTSC
	RP2C07, // PAL
	Multi,
	Umc // Dendy
};

struct Header {
	u16 PrgROMUnits; // 16KB per unit (only 8 bits on iNES, 12 on NES2)
	u16 ChrROMUnits; // 8KB per unit (only 8 bits on iNES, 12 on NES2)

	// Flags 6
	MirroringMode Mirroring;
	bool HasBattery;
	bool HasTrainer; // TODO
	bool IgnoreMirroring; // For 4-screen mode

	// Flags 7
	bool VSUnisystem; // TODO (iNES)
	bool PlayChoice; // TODO (iNES)
	HeaderFormat Format; // From Flags 7
	ConsoleType Console;

	// Flags 8
	u8 PrgRAMUnits; // 16KB per unit (iNES)
	u8 Submapper; // NES2

	// Flags 9 (iNES) or Flags 10 (NES2)
	TVMode TV; // almost never from flags 9

	// Flags 10
	bool PrgRAMPresent; // almost never used (iNES)
	bool HasBusConflicts;
	u8 PrgRAMShiftCount;
	u8 PrgNVRAMShiftCount;

	// Flags 11
	u8 ChrRAMShiftCount;
	u8 ChrNVRAMShiftCount;

	// Flags 12
	CPUTiming Timing;

	// Flags 13
	u8 VSPPUType; // TODO
	u8 VSHardwareType; // TODO
	u8 ExtendedConsoleType; // TODO

	// Flags 14
	u8 MiscellaneousRoms; // TODO

	// Flags 15
	u8 ExpansionDevice; // TODO

	u16 Mapper; // From Flags 6, 7 (iNES) and 8 (NES2)

	// Flags 11-15
	u8 Padding[5]; // iNES

	void Print();
};

class Mapper {
public:
	Mapper(const Header& header, const char* fileContent);
	virtual ~Mapper();

	virtual u8 Read(u16 address) = 0;
	virtual void Write(u8 value, u16 address) = 0;

	virtual void InitArrays(const char* fileContent);

protected:
	const Header& header;
	u8* prgRom = nullptr;
	u8* chrRom = nullptr;
	u8* prgRam = nullptr;
	u8* prgNVRam = nullptr;
	u8* chrRam = nullptr;
	u8* chrNVRam = nullptr;
};

class Mapper0 : public Mapper {
public:
	Mapper0(const Header& header, const char* fileContent);

	virtual u8 Read(u16 address) override;
	virtual void Write(u8 value, u16 address) override;
};