#pragma once

#include "Types.h"

#include <sstream>

typedef union {
	u8 v;
	struct {
		u8 Nametable : 2;
		bool VerticalIncrement : 1;
		u8 SpriteTile : 1;
		u8 BackgroundTile : 1;
		u8 SpriteHeight : 1;
		bool PPUMaster : 1;
		bool NMIEnable : 1;
	};
} PPUCTRLT;

typedef union {
	u8 v;
	struct {
		bool GrayScale : 1;
		bool BGLeftColumnEnabled : 1;
		bool SpriteLeftColumnEnabled : 1;
		bool BGEnabled : 1;
		bool SpritesEnabled : 1;
		u8 Emphasis : 3;
	};
} PPUMASKT;

typedef union {
	u8 v;
	struct {
		u8 Unused : 5;
		bool SpriteOverflow : 1;
		bool Sprite0Hit : 1;
		bool VBlank : 1;
	};
} PPUSTATUST;

typedef union {
	u8 v;
	struct {
		bool isBG : 1;
		bool spriteMatch : 1;
		u8 colorIndex : 2;
		u8 paletteIndex : 2;
	};
} PIXELINFOT;

typedef union {
	u16 v;
	struct {
		u8 coarseX : 5;
		u8 coarseY : 5;
		u8 nameTable : 2;
		u8 fineY : 3;
	};
	struct {
		u8 lsb;
		u8 msb;
	};
} VRAMAddressT;

class CPU;
class Cartridge;
class GameWindow;

class PPU {
	friend class NameTableWindow;
	friend class SpritesWindow;
	friend class PalettesWindow;
	friend class GameWindow;
public:
	PPU(Cartridge* cart);
	// return true if frame finished
	bool Update(u8 cycles);
	void Reset();

	u8 Read(u16 address);
	void Write(u8 value, u16 address);

	CPU* cpu = nullptr;
	Cartridge* cart = nullptr;
	GameWindow* gameWindow = nullptr;

private:
	std::stringstream* logStream;
	bool log = false;

	PPUCTRLT PPUCTRL = { 0x00 }; // $2000
	PPUMASKT PPUMASK = { 0x00 }; // $2001
	PPUSTATUST PPUSTATUS = { 0x00 }; // $2002
	u8 OAMADDR = 0; // $2003
	u8 OAMDATA = 0; // $2004
	u8 PPUSCROLL = 0; // $2005
	u8 PPUADDR = 0; // $2006
	u8 PPUDATA = 0; // $2007

	u32 cyclesCount = 0;
	u8 xScroll, yScroll = 0;
	u16 ppuAddress = 0;
	u16 ppuAddressTemp = 0;

	u8 vram[0x2000] = { 0 };
	u8 palette[0x20] = { 0 };
	u8 oam[64 * 4] = { 0 };

	void ReadPalette(const std::string& filePath);
	u32 paletteRGBA[64] = { 0 };
	
	void InternalVRAMWrite();
	u8 InternalVRAMRead();
	u8 MirroredVRAMRead(u16 ppuAddress);
	void InternalOAMWrite();
	u8 readBuffer = 0;
	u8 ioBus = 0; //TODO decay to 0 after 1 second

	void DrawCurrentLine();
	void DrawBackground(PIXELINFOT* pixels);
	void DrawBackground2(PIXELINFOT* pixels);
	void DrawSprites(PIXELINFOT* pixels);
	void UpdateLineSprites();

	bool oddFrame = false;
	u16 currentLine = 242;
	u16 lineCycles = 0;
	u32 frameCycles = 0;
	u32 cyclesSinceReset = 0;
	u16 lineTargetCycles = 341;

	u8 secondaryOAM[64*4] = { 0 };
	bool isSprite0InLine = false;
	u8 spritesInLineCount = 0;

	// cycle ppu
	u16 vramAddress = { 0x00 };
	u16 tempVramAddress = { 0x00 };
	u8 fineXScroll = 0;
	bool firstWrite = true;

	u16 patternShift[2] = { 0 };
	u8 paletteShift[2] = { 0 };
	u8 paletteLatchRegister = 0;
	u8 shiftCycles = 0;

	u16 spritePatternShift[8] = { 0 };
	u8 spriteAttrbutesLatches[8] = { 0 };
	u8 spriteXPosition[8] = { 0 };
};