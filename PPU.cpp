#include "PPU.h"
#include "Cartridge.h"
#include "GameWindow.h"
#include "CPU.h"
#include <iostream>
#include "Logger.h"

PPU::PPU(Cartridge* cart) : cart(cart) {
	ReadPalette("nespalette.pal");
	logStream = Logger::GetPPUStream();
	log = Logger::logPPU;
}

void PPU::Reset() {
	PPUCTRL.v = 0;
	PPUMASK.v = 0;
	firstWrite = true;
	PPUSCROLL = 0;
	readBuffer = 0;
	oddFrame = false;
	cyclesSinceReset = 0;
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

u8 PPU::Read(u16 address) {
	switch (address & 0x0007) {
	case 2: {
		ioBus &= 0x1F;
		ioBus |= PPUSTATUS.v & 0xE0;
		PPUSTATUS.VBlank = false;
		firstWrite = true;
		break;
	}
	case 4: {
		ioBus = oam[OAMADDR];
		break;
	}
	case 7: {
		ioBus = InternalVRAMRead();
		break;
	}
	}
	return ioBus;
}

void PPU::Write(u8 value, u16 address) {
	ioBus = value;
	switch (address & 0x0007) {
	case 0: {
		if (cyclesSinceReset >= 29658 * 3) {
			bool oldNMI = PPUCTRL.NMIEnable;
			PPUCTRL.v = value;
			if (!oldNMI && PPUCTRL.NMIEnable && PPUSTATUS.VBlank)
				cpu->NMIRequested = true;
			tempVramAddress &= 0x73FF;
			tempVramAddress |= (PPUCTRL.Nametable << 10);
			if (log) (*logStream) << "Set nametable to " << ToHex(tempVramAddress) << " (" << ToHex(PPUCTRL.Nametable) << ")" << std::endl;
		}
		break;
	}
	case 1: {
		if (cyclesSinceReset >= 29658 * 3) {
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
		if (cyclesSinceReset >= 29658 * 3) {
			PPUSCROLL = value;
			if (firstWrite) {
				xScroll = PPUSCROLL;
				tempVramAddress &= 0x7FE0;
				tempVramAddress |= value >> 3;
				fineXScroll = value & 0x07;
				if (log) (*logStream) << "Set x temp scroll to " << ToHex(tempVramAddress) << " (" << ToHex(value) << ")" << std::endl;
				if (log) (*logStream) << "Set finex to " << ToHex(fineXScroll) << " (" << ToHex(value) << ")" << std::endl;
			} else {
				yScroll = PPUSCROLL;
				tempVramAddress &= 0x8C1F;
				tempVramAddress |= (value & 0xF8) << 2;
				tempVramAddress |= (value & 0x07) << 12;
				if (log) (*logStream) << "Set y temp scroll to " << ToHex(tempVramAddress) << " (" << ToHex(value) << ")" << std::endl;
			}
			firstWrite = !firstWrite;
		}
		break;
	}
	case 6: {
		if (cyclesSinceReset >= 29658 * 3) {
			PPUADDR = value;
			if (firstWrite) {
				ppuAddressTemp = ((PPUADDR & 0x3F) << 8);
				tempVramAddress &= 0x00FF;
				tempVramAddress |= (value & 0x3F) << 8;
				if (log) (*logStream) << "Set temp address to " << ToHex(tempVramAddress) << " (" << ToHex(value) << ")" << std::endl;
				//if (log) (*logStream) << "Set ppu temp address to " << ToHex(ppuAddressTemp) << std::endl;
			} else {
				ppuAddress = ppuAddressTemp | PPUADDR;
				tempVramAddress &= 0x7F00;
				tempVramAddress |= value;
				vramAddress |= tempVramAddress;
				if (log) (*logStream) << "Set address to " << ToHex(vramAddress) << " (" << ToHex(value) << ")" << std::endl;
				//if (log) (*logStream) << "Set ppu address to " << ToHex(ppuAddress) << std::endl;
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
		if (log) (*logStream) << "cart[" << ToHex(ppuAddress) << "] = " << ToHex(PPUDATA) << std::endl;
	} else if (ppuAddress < 0x3F00) {
		// ppu mirroring
		u16 address = ppuAddress & 0xEFFF; // 0x3XXX -> 0x2XXX
		// cart mirroring
		if (cart->GetMirroring() == MirroringMode::Vertical)
			address &= 0xF7FF;
		else
			address &= 0xFBFF;
		vram[address - 0x2000] = PPUDATA;
		if (log) (*logStream) << "vram[" << ToHex(address) << "] = " << ToHex(PPUDATA) << std::endl;
	} else {
		u16 tempAddress = ppuAddress & 0x001F;
		if ((ppuAddress & 0x03) == 0)
			tempAddress &= ~0x0010; // mirror 0x3F10/3F14/3F18/3F1C to 0x3F00/3F04/3F08/3F0C
		palette[tempAddress] = PPUDATA;
		if (log) (*logStream) << "palette[" << ToHex((u16)(ppuAddress & 0x001F)) << "] = " << ToHex(PPUDATA) << std::endl;
	}
	ppuAddress += PPUCTRL.VerticalIncrement ? 32 : 1;
	ppuAddress &= 0x3FFF;
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
	ppuAddress &= 0x3FFF;

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
	if (log) (*logStream) << "oam[" << ToHex(OAMADDR) << "] = " << ToHex(OAMDATA) << std::endl;
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
	if (cyclesSinceReset < 29658 * 3) {
		cyclesSinceReset += cycles;
		//if (cyclesSinceReset >= 29658 * 3)
			//log = true;
		return false;
	}

	u16 temp = lineCycles + cycles;
	
	if (lineCycles < 260 && temp >= 260 && currentLine <= 239)
		DrawCurrentLine();

	if (lineCycles < 256 && temp >= 256 && (PPUMASK.BGEnabled || PPUMASK.SpritesEnabled)) {
		u8 fineY = vramAddress >> 12;
		u8 coarseY = (vramAddress >> 5) & 0x001F;
		u8 nametableY = (vramAddress >> 11) & 0x0001;
		if (fineY < 7) {
			fineY++;
		} else {
			fineY = 0;
			if (coarseY == 29) {
				coarseY = 0;
				nametableY ^= 0x1;
			} else if (coarseY == 31)
				coarseY = 0;
			else
				coarseY++;
		}

		vramAddress &= 0x841F;
		vramAddress |= (fineY << 12);
		vramAddress |= (coarseY << 5);
		vramAddress |= (nametableY << 11);
		if (log) (*logStream) << "y updated to " << ToHex(vramAddress) << std::endl;
	}

	if (lineCycles < 257 && temp >= 257 && (PPUMASK.BGEnabled || PPUMASK.SpritesEnabled)) {
		vramAddress &= 0x7BE0;
		vramAddress |= tempVramAddress & 0x041F; // coarseX and nametableX
		if (log) (*logStream) << "x updated to " << ToHex(vramAddress) << std::endl;
	}

	if (lineCycles < 280 && temp >= 280 && (PPUMASK.BGEnabled || PPUMASK.SpritesEnabled) && currentLine == 261) {
		vramAddress &= 0x841F;
		vramAddress |= tempVramAddress & 0x7BE0;  // coarseY, nametableY and fineY
		if (log) (*logStream) << "v updated to " << ToHex(vramAddress) << std::endl;
	}

	lineCycles += cycles;
	//TODO if (lineCycles >= 257 && lineCycles <= 320)
	//	OAMADDR = 0;
	if (lineCycles >= lineTargetCycles) {
		//if (log) (*logStream) << "finished line " << currentLine << "(" << ToHex(currentLine) << ")" << std::endl;

		if (currentLine == 240)
			frameFinished = true;
		else if (currentLine == 241) {
			if (log) (*logStream) << "finished frame " << frameCycles << std::endl;
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

		if (log) (*logStream) << "starting line " << ToHex(currentLine) << std::endl;

		// vramAddress.coarseX starts updating every 8 dots after 328,
		// so it updates twice by the end of the line
		for (int i = 0; i < 2; i++) {
			u8 coarseX = vramAddress & 0x001F;
			u16 nametableX = vramAddress & 0x0400;
			if (coarseX == 31) {
				coarseX = 0;
				nametableX ^= 0x0400;
			} else
				coarseX++;
			vramAddress &= 0x7BE0;
			vramAddress |= coarseX;
			vramAddress |= nametableX;
			if (log) (*logStream) << "eol x updated to " << ToHex(vramAddress) << std::endl;
		}
	}

	return frameFinished;
}

void PPU::DrawCurrentLine() {
	// https://austinmorlan.com/posts/nes_rendering_overview/

	PIXELINFOT pixels[256] = { 0 };
	
	// TODO PPUMASK.BGEnabled and PPUMASK.SpritesEnabled may change during the current line
	if (PPUMASK.BGEnabled)
		DrawBackground2(pixels);
	
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
	u8 tempFineXScroll = xScroll % 8;
	u16 tileXEnd = tileXStart + 32;

	//if (log) (*logStream) << "bg " << currentLine << " from tile = " << tileXStart << " to tile = " << tileXEnd - 1 << std::endl;

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

		//if (log) (*logStream) << "tile = " << ToHex(tileIndex) << " l = " << ToHex(lowByte) << " h = " << ToHex(highByte) << std::endl;

		for (u8 bit = 7 - tempFineXScroll; bit < 8; bit--) {
			PIXELINFOT pixelInfo = { 0x01 }; // isBG
			if (pixelIndex >= 8 || firstColumnEnabled) {
				u8 lBit = (lowByte >> bit) & 0x01;
				u8 hBit = (highByte >> bit) & 0x02;
				u8 color = lBit | hBit;

				//if (log) (*logStream) << "pixel = " << ToHex(pixelIndex) << " c = " << ToHex(c) << std::endl;
				pixelInfo.colorIndex = color;
				pixelInfo.paletteIndex = attributesShifted;
			}

			pixels[pixelIndex++] = pixelInfo;
			if (pixelIndex == 256)
				return;
		}
		tempFineXScroll = 0;

		if (coarseX == 31) {
			coarseX = 0;
			nameTable ^= 0x01;
		} else
			coarseX++;
	}
}
#pragma optimize("", off)
void PPU::DrawBackground2(PIXELINFOT* pixels) {
	u8 tempFineXScroll = fineXScroll;
	for (u16 pixelIndex = 0; pixelIndex < 256; ) {
		u8 tileIndex = MirroredVRAMRead(0x2000 | (vramAddress & 0xFFF));
		u16 v = vramAddress;
		u8 attributes = MirroredVRAMRead(0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07));

		u8 patternSection = ((vramAddress & 0x02) >> 1) + ((vramAddress & 0x40) >> 5);
		u8 attributesShifted = (attributes >> (patternSection << 1)) & 0x03;

		u16 chrAddress = PPUCTRL.BackgroundTile * 0x1000 + tileIndex * 16;

		u16 lowByte = cart->ReadChr(chrAddress + (vramAddress >> 12));
		u16 highByte = cart->ReadChr(chrAddress + (vramAddress >> 12) + 8) << 1;

		//if (log) (*logStream) << "tile = " << ToHex(tileIndex) << " l = " << ToHex(lowByte) << " h = " << ToHex(highByte) << std::endl;

		for (u8 bit = 7 - tempFineXScroll; bit < 8; bit--) {
			PIXELINFOT pixelInfo = { 0x01 }; // isBG
			if (pixelIndex >= 8 || PPUMASK.BGLeftColumnEnabled) {
				u8 lBit = (lowByte >> bit) & 0x01;
				u8 hBit = (highByte >> bit) & 0x02;
				u8 color = lBit | hBit;

				//if (log) (*logStream) << "pixel = " << ToHex(pixelIndex) << " c = " << ToHex(color) << std::endl;
				pixelInfo.colorIndex = color;
				pixelInfo.paletteIndex = attributesShifted;
			}

			pixels[pixelIndex++] = pixelInfo;

			if (pixelIndex == 256)
				return;
		}

		u8 coarseX = vramAddress & 0x001F;
		u16 nametableX = vramAddress & 0x0400;
		if (coarseX == 31) {
			coarseX = 0;
			nametableX ^= 0x0400;
		}
		else
			coarseX++;
		vramAddress &= 0x7BE0;
		vramAddress |= coarseX;
		vramAddress |= nametableX;

		tempFineXScroll = 0;
	}
}
#pragma optimize("", on)
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
				if (log) (*logStream) << "sprite address " << (int)i << " t " << ToHex(oam[i + 1]) << " x " << ToHex(oam[i + 3]) << std::endl;
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
	
	if (log && spritesInLineCount > 0) (*logStream) << "line " << (int)currentLine << " sprites count " << (int)spritesInLineCount << std::endl;
}