#include "PPU.h"
#include "Cartridge.h"
#include "GameWindow.h"
#include "CPU.h"
#include <iostream>

PPU::PPU(Cartridge* cart) : cart(cart) {
	ReadPalette("nespalette.pal");
	logFile.open("ppu.log", std::ofstream::out | std::ofstream::trunc);
	log = false;
	//log = true;
}

PPU::~PPU() {
	logFile.close();
}

void PPU::ReadPalette(const std::string& filePath) {
	std::ifstream readStream;
	readStream.open(filePath, std::ios::in | std::ios::binary);

	if (readStream.fail()) {
		char errorMessage[256];
		strerror_s(errorMessage, 256);
		std::cout << "ERROR: Could not open file " << filePath << ". " << errorMessage << std::endl;
	}
	else {
		readStream.seekg(0, std::ios::end);
		std::streampos fileSize = readStream.tellg();
		if (fileSize > 0) {
			readStream.seekg(0, std::ios::beg);

			char* fileContent = new char[(u32)fileSize];
			readStream.read(fileContent, fileSize);

			for (u8 colorIndex = 0; colorIndex < 64; colorIndex++) {
				u32 color = 0x000000FF;
				color |= ((u8)fileContent[colorIndex * 3]) << 24;
				color |= ((u8)fileContent[colorIndex * 3 + 1]) << 16;
				color |= ((u8)fileContent[colorIndex * 3 + 2]) << 8;
				paletteRGBA[colorIndex] = color;
			}
		}
		else
			std::cout << "ERROR: Rom file is empty " << filePath << std::endl;
	}
}

void PPU::DumpLogs() {
	if (!log)
		return;

	logFile << logStrings.str();
	logStrings.str(std::string());
}

u8 PPU::Read(u16 address) {
	switch (address & 0x0007) {
	case 0: return PPUCTRL.v;
	case 1: return PPUMASK.v;
	case 2: {
		u8 v = PPUSTATUS.v;
		PPUSTATUS.VBlank = false;
		firstWrite = true;
		return v | lastWrittenValue;
	}
	case 3: return OAMADDR;
	case 4: return oam[OAMADDR];
	case 5: return PPUSCROLL;
	case 6: return PPUADDR;
	case 7: return InternalVRAMRead();
	}
	return 0;
}

void PPU::Write(u8 value, u16 address) {
	lastWrittenValue = value & 0x1F;
	switch (address & 0x0007) {
	case 0: {
		if (cyclesSinceReset >= 29658) {
			bool oldNMI = PPUCTRL.NMIEnable;
			PPUCTRL.v = value;
			if (!oldNMI && PPUCTRL.NMIEnable && PPUSTATUS.VBlank)
				cpu->NMIRequested = true;
		}
		break;
	}
	case 1: {
		if (cyclesSinceReset >= 29658) {
			PPUMASK.v = value;
		}
		break;
	}
	case 2: {
		bool vblank = PPUSTATUS.VBlank;
		PPUSTATUS.v = value & 0xE0;
		PPUSTATUS.VBlank = vblank;
		break;
	}
	case 3: OAMADDR = value & 0x00FF; break;
	case 4: {
		OAMDATA = value;
		InternalOAMWrite();
		break;
	}
	case 5: {
		if (cyclesSinceReset >= 29658) {
			PPUSCROLL = value;
			if (firstWrite)
				xScroll = PPUSCROLL;
			else
				yScroll = PPUSCROLL;
			firstWrite = !firstWrite;
		}
		break;
	}
	case 6: {
		if (cyclesSinceReset >= 29658) {
			PPUADDR = value;
			if (firstWrite) {
				ppuAddressTemp = ((PPUADDR & 0x3F) << 8);
				if (log) logStrings << "Set ppu temp address to " << ToHex(ppuAddressTemp) << std::endl;
			} else {
				ppuAddress = ppuAddressTemp | PPUADDR;
				if (log) logStrings << "Set ppu address to " << ToHex(ppuAddress) << std::endl;
			}
			firstWrite = !firstWrite;
		}
		break;
	}
	case 7: {
		PPUDATA = value;
		InternalVRAMWrite();
		break;
	}
	}
}

void PPU::InternalVRAMWrite() {
	if (ppuAddress < 0x2000) {
		cart->Write(PPUDATA, ppuAddress);
		if (log) logStrings << "cart[" << ToHex(ppuAddress) << "] = " << ToHex(PPUDATA) << std::endl;
	} else if (ppuAddress < 0x3F00) {
		// ppu mirroring
		u16 address = ppuAddress & 0xEFFF; // 0x3XXX -> 0x2XXX
		// cart mirroring
		if (cart->GetMirroring() == MirroringMode::Vertical)
			address &= 0xF7FF;
		else
			address &= 0xFBFF;
		vram[address - 0x2000] = PPUDATA;
		if (log) logStrings << "vram[" << ToHex(address) << "] = " << ToHex(PPUDATA) << std::endl;
	} else {
		u16 tempAddress = ppuAddress & 0x001F;
		if ((ppuAddress & 0x03) == 0)
			tempAddress &= ~0x0010; // mirror 0x3F10/3F14/3F18/3F1C to 0x3F00/3F04/3F08/3F0C
		palette[tempAddress] = PPUDATA;
		if (log) logStrings << "palette[" << ToHex((u16)(ppuAddress & 0x001F)) << "] = " << ToHex(PPUDATA) << std::endl;
	}
	ppuAddress += PPUCTRL.VerticalIncrement ? 32 : 1;
}

u8 PPU::InternalVRAMRead() {
	u8 returnValue = readBuffer;

	// palettes are read directly
	if (ppuAddress >= 0x3F00 && ppuAddress < 0x3F20)
		returnValue = palette[(ppuAddress & 0x03) == 0 ? 0 : ppuAddress & 0x001F];

	// update buffer
	if (ppuAddress < 0x2000)
		readBuffer = cart->ReadChr(ppuAddress);
	else
		// for palette address the buffer uses the mirrored nametable value too
		readBuffer = MirroredVRAMRead(ppuAddress);
	
	ppuAddress += PPUCTRL.VerticalIncrement ? 32 : 1;

	return returnValue;
}

u8 PPU::MirroredVRAMRead(u16 ppuAddress) {
	// ppu mirroring
	u16 address = ppuAddress & 0xEFFF; // 0x3XXX -> 0x2XXX
	// cart mirroring
	if (cart->GetMirroring() == MirroringMode::Vertical)
		address &= 0xF7FF;
	else
		address &= 0xFBFF;
	return vram[address - 0x2000];
}

void PPU::InternalOAMWrite() {
	if (log) logStrings << "oam[" << ToHex(OAMADDR) << "] = " << ToHex(OAMDATA) << std::endl;
	if ((currentLine == 262 || currentLine < 240) && (PPUMASK.BGEnabled || PPUMASK.SpritesEnabled))
		OAMADDR += 0x04;
	else {
		oam[OAMADDR] = OAMDATA;
		OAMADDR++;
	}
}

bool PPU::Update(u8 cycles) {
	bool frameFinished = false;

	frameCycles += cycles;
	if (cyclesSinceReset < 29658)
		cyclesSinceReset += cycles;

	if (lineCycles < 260 && lineCycles + cycles >= 260 && currentLine <= 239)
		DrawCurrentLine();

	lineCycles += cycles;
	//TODO if (lineCycles >= 257 && lineCycles <= 320)
	//	OAMADDR = 0;
	if (lineCycles >= lineTargetCycles) {
		//if (log) logStrings << "finished line " << currentLine << "(" << ToHex(currentLine) << ")" << std::endl;

		if (currentLine == 240)
			frameFinished = true;
		else if (currentLine == 241) {
			if (log) logStrings << "finished frame" << std::endl;
			PPUSTATUS.VBlank = true;
			if (PPUCTRL.NMIEnable)
				cpu->NMIRequested = true;
		} else if (currentLine == 261) { // prerender line
			PPUSTATUS.VBlank = false;
			PPUSTATUS.Sprite0Hit = false;
			PPUSTATUS.SpriteOverflow = false;
		}

		lineCycles -= lineTargetCycles;
		if (++currentLine == 262) {
			currentLine = 0;
			frameCycles -= oddFrame ? 341 * 262 - 1 : 341 * 262;
			oddFrame = !oddFrame;
		}
		lineTargetCycles = currentLine == 261 && oddFrame ? 340 : 341;
	}

	return frameFinished;
}

void PPU::DrawCurrentLine() {
	// https://austinmorlan.com/posts/nes_rendering_overview/

	PIXELINFOT pixels[256] = { 0 };
	
	// TODO PPUMASK.BGEnabled and PPUMASK.SpritesEnabled may change during the current line
	if (PPUMASK.BGEnabled)
		DrawBackground(pixels);
	
	if (PPUMASK.SpritesEnabled) {
		DrawSprites(pixels);
		UpdateLineSprites();
	}

	u32 lineColors[256] = { 0 };
	for (u16 i = 0; i < 256; i++) {
		u8 index = 0;
		if (pixels[i].isBG)
			index = pixels[i].colorIndex == 0 ? 0 : pixels[i].paletteIndex * 4 + pixels[i].colorIndex;
		else
			index = pixels[i].colorIndex == 0 ? 0 : (pixels[i].paletteIndex + 4) * 4 + pixels[i].colorIndex;

		lineColors[i] = paletteRGBA[palette[index]];
	}
	
	gameWindow->DrawLine(currentLine, lineColors);
}

void PPU::DrawBackground(PIXELINFOT* pixels) {
	// TODO yScroll and xScroll may change during the scanline
	// TODO yScroll > 240 causes a glitch
	u8 nameTable = PPUCTRL.Nametable;
	u16 temp = currentLine + yScroll;
	if (temp >= 240) {
		temp -= 240;
		nameTable ^= 0x02;
	}
	u16 tileY = temp / 8;
	
	u8 lineInTile = temp % 8;
	u8 coarseX = xScroll >> 3;
	u16 tileXStart = tileY * 32 + coarseX;
	u8 fineXScroll = xScroll % 8;
	u16 tileXEnd = tileXStart + 32;

	//if (log) logStrings << "bg " << currentLine << " from tile = " << tileXStart << " to tile = " << tileXEnd - 1 << std::endl;

	s16 pixelIndex = 0;
	bool firstColumnEnabled = PPUMASK.BGLeftColumnEnabled;

	for (u16 tile = 0; tile <= 32; tile++) {
		u16 tileX = tileY * 32 + coarseX;
		u8 tileIndex = MirroredVRAMRead(nameTable * 0x400 + tileX + 0x2000);

		u8 patternX = (tileX & 0x001F) / 4;
		u8 patternY = (tileX >> 5) / 4;

		u8 patternIndex = patternY * 8 + patternX;
		u8 patternSection = ((tileX & 0x02) >> 1) + ((tileX & 0x40) >> 5);

		u8 attributes = MirroredVRAMRead(0x23C0 + nameTable * 0x400 + patternIndex);

		u8 attributesShifted = (attributes >> (patternSection << 1)) & 0x03;

		u16 chrAddress = PPUCTRL.BackgroundTile * 0x1000 + tileIndex * 16;

		u16 lowByte = cart->ReadChr(chrAddress + lineInTile);
		u16 highByte = cart->ReadChr(chrAddress + lineInTile + 8) << 1;

		//if (log) logStrings << "tile = " << ToHex(tileIndex) << " l = " << ToHex(lowByte) << " h = " << ToHex(highByte) << std::endl;

		for (u8 bit = 7 - fineXScroll; bit < 8; bit--) {
			PIXELINFOT pixelInfo = { 0x01 }; // isBG
			if (pixelIndex >= 8 || firstColumnEnabled) {
				u8 lBit = (lowByte >> bit) & 0x01;
				u8 hBit = (highByte >> bit) & 0x02;
				u8 color = lBit | hBit;

				//if (log) logStrings << "pixel = " << ToHex(pixelIndex) << " c = " << ToHex(c) << std::endl;
				pixelInfo.colorIndex = color;
				pixelInfo.paletteIndex = attributesShifted;
			}

			pixels[pixelIndex++] = pixelInfo;
			if (pixelIndex == 256)
				return;
		}
		fineXScroll = 0;

		if (coarseX == 31) {
			coarseX = 0;
			nameTable ^= 0x01;
		} else
			coarseX++;
	}
}

void PPU::DrawSprites(PIXELINFOT* pixels) {
	bool firstColumnEnabled = PPUMASK.SpriteLeftColumnEnabled;
	for (u8 sprite = 0; sprite < spritesInLineCount; sprite++) {
		u8 y = secondaryOAM[sprite * 4] + 1;
		u8 tileIndex = secondaryOAM[sprite * 4 + 1];
		u8 attributes = secondaryOAM[sprite * 4 + 2];
		u8 x = secondaryOAM[sprite * 4 + 3];
		if (x == 0 && !firstColumnEnabled)
			continue;

		bool flipY = attributes & 0x80;
		bool flipX = attributes & 0x40;
		bool bgPriority = attributes & 0x20; // bgPriority = false -> front
		u16 patternTable = PPUCTRL.SpriteTile;

		u8 lineInSprite = currentLine - y;
		u8 lineInTile = lineInSprite; // it's the same for 8x8 sprites, but might be different for 8x16

		if (PPUCTRL.SpriteHeight == 1) {
			patternTable = tileIndex & 0x01;

			// drawing even or odd tile?
			if (lineInSprite < 8 != flipY)
				tileIndex &= 0xFE;
			else
				tileIndex |= 0x01;

			// drawing first or second tile?
			if (lineInSprite >= 8)
				lineInTile -= 8;
		}

		u16 chrAddress = patternTable * 0x1000 + tileIndex * 16;
		lineInTile = flipY ? 7 - lineInTile : lineInTile;
		u8 lowByte = cart->ReadChr(chrAddress + lineInTile);
		u8 highByte = cart->ReadChr(chrAddress + lineInTile + 8);

		// reverse bytes here instead of iterating backwards in the for loop below
		if (flipX) {
			lowByte = BitReverseTable256[lowByte];
			highByte = BitReverseTable256[highByte];
		}

		u16 lowByte16 = lowByte;
		u16 highByte16 = highByte << 1;

		for (u8 bit = 0; bit < 8; bit++) {
			u16 pixelIndex = x + (7 - bit);
			if (pixelIndex == 0 && !firstColumnEnabled)
				continue;

			if (pixelIndex >= 256)
				break;

			if (pixels[pixelIndex].spriteMatch)
				continue;

			u8 lBit = (lowByte16 >> bit) & 0x01;
			u8 hBit = (highByte16 >> bit) & 0x02;
			u8 color = lBit | hBit;
			if (color != 0) {
				if (isSprite0InLine && sprite == 0 && pixels[pixelIndex].colorIndex != 0 && pixelIndex != 255 && !PPUSTATUS.Sprite0Hit)
					PPUSTATUS.Sprite0Hit = true;

				// TODO use color index instead of palette index
				if (pixels[pixelIndex].isBG && pixels[pixelIndex].colorIndex != 0 && bgPriority) {
					pixels[pixelIndex].spriteMatch = true;
					continue;
				}

				pixels[pixelIndex].isBG = false;
				pixels[pixelIndex].spriteMatch = true;
				pixels[pixelIndex].colorIndex = color;
				pixels[pixelIndex].paletteIndex = attributes & 0x03;
			}
		}
	}
}

void PPU::UpdateLineSprites() {
	u8 spriteHeight = PPUCTRL.SpriteHeight == 0 ? 8 : 16;
	spritesInLineCount = 0;
	isSprite0InLine = false;
	memset(secondaryOAM, 0xFF, 64*4);
	for (u16 i = 0; i < 256; i += 4) {
		u8 spriteY = oam[i];
		if (spritesInLineCount < 8) {
			secondaryOAM[spritesInLineCount * 4] = spriteY;
			if (currentLine >= spriteY && currentLine < spriteY + spriteHeight) {
				if (log) logStrings << "sprite address " << (int)i << " t " << ToHex(oam[i + 1]) << " x " << ToHex(oam[i + 3]) << std::endl;
				memcpy(secondaryOAM + spritesInLineCount * 4 + 1, oam + i + 1, 3);
				spritesInLineCount++;
				if (i == 0)
					isSprite0InLine = true;
			}
		} else {
			PPUSTATUS.SpriteOverflow = true;
			break;
		}
	}
	
	if (log && spritesInLineCount > 0) logStrings << "line " << (int)currentLine << " sprites count " << (int)spritesInLineCount << std::endl;
}