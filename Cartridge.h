#pragma once

#include "Types.h"
#include "Mapper.h"

class Cartridge {
public:
	Cartridge(std::string RomPath);
	virtual ~Cartridge();

	u8 Read(u16 address);
	void Write(u8 value, u16 address);

	u8 ReadChr(u16 address);

	MirroringMode GetMirroring() const;

private:
	Header header;
	Mapper* mapper = nullptr;

	void LoadFile(const std::string& path);
	void InitMapper(const char* fileContent);

	bool LoadHeader(const char* fileContent);
};