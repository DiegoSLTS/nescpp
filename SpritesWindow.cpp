#include "SpritesWindow.h"

#include "NES.h"

SpritesWindow::SpritesWindow(NES& nes) : Window(256, 240, "Sprites", 550, 0, false, true), ppu(nes.ppu), cart(nes.cartridge) {}

void SpritesWindow::RenderContent() {
	memset(screenArray, 0, width * height * 4);
	for (u8 sprite = 0; sprite < 64; sprite++)
		DrawSprite(sprite);

	Window::RenderContent();
}

void SpritesWindow::DrawSprite(u8 spriteIndex) {
	u8 y = ppu.oam[spriteIndex * 4] + 1;
	u8 tileIndex = ppu.oam[spriteIndex * 4 + 1];
	u8 attributes = ppu.oam[spriteIndex * 4 + 2];
	u8 x = ppu.oam[spriteIndex * 4 + 3];
	
	if (ppu.PPUCTRL.SpriteHeight == 1) {
		tileIndex &= 0xFE;
		u8 patternTable = tileIndex & 0x01;
		bool flipY = attributes & 0x80;
		if (flipY) {
			DrawTile(tileIndex + 1, patternTable, x, y, attributes);
			DrawTile(tileIndex, patternTable, x, y + 8, attributes);
		} else {
			DrawTile(tileIndex, patternTable, x, y, attributes);
			DrawTile(tileIndex + 1, patternTable, x, y + 8, attributes);
		}
	} else {
		u8 patternTable = ppu.PPUCTRL.SpriteTile;
		DrawTile(tileIndex, patternTable, x, y, attributes);
	}
}

void SpritesWindow::DrawTile(u8 tileIndex, u8 patternTable, u8 x, u8 y, u8 attributes) {
	bool flipY = attributes & 0x80;
	bool flipX = attributes & 0x40;
	u8 paletteIndex = (attributes & 0x03) + 4;
	u16 chrAddress = patternTable * 0x1000 + tileIndex * 16;

	for (u8 line = 0; line < 8; line++) {
		u16 pixelY = line + y;
		if (pixelY >= 240)
			return;

		u8 tileLine = flipY ? 7 - line : line;
		u8 lowByte = cart.ReadChr(chrAddress + tileLine);
		u8 highByte = cart.ReadChr(chrAddress + tileLine + 8);

		if (flipX) {
			lowByte = BitReverseTable256[lowByte];
			highByte = BitReverseTable256[highByte];
		}

		u16 lowByte16 = lowByte;
		u16 highByte16 = highByte << 1;

		for (u8 bit = 0; bit < 8; bit++) {
			u16 pixelX = x + (7 - bit);
			if (pixelX >= 256)
				break;

			u8 lBit = (lowByte16 >> bit) & 0x01;
			u8 hBit = (highByte16 >> bit) & 0x02;

			u8 color = lBit | hBit;
			if (color == 0)
				continue;

			u8 paletteIndex = color == 0 ? 0 : ((attributes & 0x03) + 4) * 4 + color;
			u8 nesColor = ppu.palette[paletteIndex];
			screenArray[pixelY * 256 + pixelX] = ppu.paletteRGBA[nesColor];
		}
	}
}
