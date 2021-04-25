#include "Cartridge.h"

#include <iostream>
#include <fstream>

#include "unzip.h"

namespace {
    const u8 ExpectedNESHeader[] = { 'N', 'E', 'S', 0x1A };
    const u8 Zeros[4] = { 0 };
}

Cartridge::Cartridge(std::string RomPath) {
    LoadFile(RomPath);
}

Cartridge::~Cartridge() {
    if (mapper != nullptr)
        delete mapper;
}

bool Cartridge::LoadHeader(const char* fileContent) {
    if (memcmp(ExpectedNESHeader, fileContent, 4) != 0) {
        std::cout << "ERROR: Not an iNES or NES2.0 file" << std::endl;
        return false;
    }

    header.PrgROMUnits = fileContent[4];
    header.ChrROMUnits = fileContent[5];

    u8 flagsByte = fileContent[6];

    header.Mirroring = (MirroringMode)(flagsByte & 0x01);
    header.HasBattery = (flagsByte & 0x02) > 0;
    header.HasTrainer = (flagsByte & 0x04) > 0;
    header.IgnoreMirroring = (flagsByte & 0x08) > 0;

    header.Mapper = (flagsByte & 0xF0) >> 4;

    flagsByte = fileContent[7];

    header.Format = (flagsByte & 0x0C) == 0x08 ? HeaderFormat::NES2 : HeaderFormat::iNESv1;
    if (header.Format == HeaderFormat::NES2)
        header.Console = (ConsoleType)(flagsByte & 0x03);
    else {
        header.VSUnisystem = (flagsByte & 0x01) > 0;
        header.PlayChoice = (flagsByte & 0x02) > 0;
    }
    header.Mapper |= (flagsByte & 0xF0);

    flagsByte = fileContent[8];

    if (header.Format == HeaderFormat::NES2) {
        header.Mapper |= (flagsByte & 0x0F) << 8;
        header.Submapper = (flagsByte & 0xF0) >> 4;
    } else
        header.PrgRAMUnits = flagsByte;

    flagsByte = fileContent[9];

    if (header.Format == HeaderFormat::NES2) {
        header.PrgROMUnits |= (flagsByte & 0x0F) << 8;
        header.ChrROMUnits |= (flagsByte & 0xF0) << 4;
    } else
        header.TV = (TVMode)(flagsByte & 0x01);

    flagsByte = fileContent[10];

    if (header.Format == HeaderFormat::NES2) {
        header.PrgRAMShiftCount = (flagsByte & 0x0F);
        header.PrgNVRAMShiftCount = (flagsByte & 0xF0) >> 4;
    } else {
        switch (flagsByte & 0x03) {
        case 0: header.TV = TVMode::NTSC; break;
        case 2: header.TV = TVMode::PAL; break;
        default: header.TV = (TVMode)(TVMode::NTSC | TVMode::PAL);
        }
        header.PrgRAMPresent = (flagsByte & 0x10) == 0;
        header.HasBusConflicts = (flagsByte & 0x20) > 0;
    }

    flagsByte = fileContent[11];

    header.Padding[0] = flagsByte;
    if (header.Format == HeaderFormat::NES2) {
        header.ChrRAMShiftCount = (flagsByte & 0x0F);
        header.ChrNVRAMShiftCount = (flagsByte & 0xF0) >> 4;
    }

    flagsByte = fileContent[12];

    header.Padding[1] = flagsByte;
    if (header.Format == HeaderFormat::NES2)
        header.Timing = (CPUTiming)(flagsByte & 0x03);

    flagsByte = fileContent[13];

    header.Padding[2] = flagsByte;
    if (header.Format == HeaderFormat::NES2) {
        header.VSPPUType = (flagsByte & 0x0F);
        header.VSHardwareType = (flagsByte & 0xF0) >> 4;
    }

    flagsByte = fileContent[14];

    header.Padding[3] = flagsByte;
    if (header.Format == HeaderFormat::NES2)
        header.MiscellaneousRoms = flagsByte & 0x03;

    flagsByte = fileContent[15];

    header.Padding[4] = flagsByte;
    if (header.Format == HeaderFormat::NES2)
        header.ExpansionDevice = flagsByte & 0x3F;

    if (header.Format == HeaderFormat::iNESv1 && memcmp(header.Padding + 1, Zeros, 4) != 0)
        std::cout << "WARNING: Archaic iNES header" << std::endl;

    return true;
}

void Cartridge::LoadFile(const std::string& path) {
    char* fileContent = nullptr;

    HZIP hz = OpenZip(path.c_str(), 0);
    if (hz != NULL) {
        ZIPENTRY ze;
        GetZipItem(hz, -1, &ze);

        bool found = false;
        for (int zi = 0; zi < ze.index; zi++) {
            GetZipItem(hz, zi, &ze);
            if (strcmp(ze.name + strlen(ze.name) - 4, ".nes") == 0) {
                found = true;
                break;
            }
        }

        if (found) {
            fileContent = new char[ze.unc_size];
            UnzipItem(hz, ze.index, fileContent, ze.unc_size);
        } else
            std::cout << "ERROR: No .nes file inside zip file at " << path << std::endl;
        CloseZip(hz);
    } else {
        std::ifstream readStream;
        readStream.open(path, std::ios::in | std::ios::binary);

        if (readStream.fail()) {
            char errorMessage[256];
            strerror_s(errorMessage, 256);
            std::cout << "ERROR: Could not open file " << path << ". " << errorMessage << std::endl;
        } else {
            readStream.seekg(0, std::ios::end);
            std::streampos fileSize = readStream.tellg();
            if (fileSize > 0) {
                readStream.seekg(0, std::ios::beg);

                fileContent = new char[(u32)fileSize];
                readStream.read(fileContent, fileSize);
            } else
                std::cout << "ERROR: Rom file is empty " << path << std::endl;
        }
    }

    if (fileContent != nullptr) {
        if (LoadHeader(fileContent)) {
            InitMapper(fileContent);
            header.Print();
        } else
            std::cout << "ERROR: Could not load header info for " << path << std::endl;

        delete[] fileContent;
    }
}

void Cartridge::InitMapper(const char* fileContent) {
    switch (header.Mapper) {
    case 0: mapper = new Mapper0(header, fileContent); break;
    case 1: mapper = new Mapper1(header, fileContent); break;
    }

    if (mapper == nullptr) {
        std::cout << "ERROR: mapper " << (u16)header.Mapper << " not supported" << std::endl;
        return;
    }
}

u8 Cartridge::Read(u16 address) {
    if (mapper == nullptr)
        return 0;
	
    return mapper->Read(address);
}

void Cartridge::Write(u8 value, u16 address) {
    if (mapper != nullptr)
        mapper->Write(value, address);
}

u8 Cartridge::ReadChr(u16 address) {
    if (mapper == nullptr)
        return 0;

    return mapper->ReadChr(address);
}

MirroringMode Cartridge::GetMirroring() const {
    return mapper->GetMirroring();
}