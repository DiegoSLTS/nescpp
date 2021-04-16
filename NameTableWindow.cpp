#include "NameTableWindow.h"
#include "NES.h"

NameTableWindow::NameTableWindow(NES& nes) : Window(256 * 2 + (256 * 2) / 8 - 1, 240 * 2 + (240 * 2) / 8 - 1, "NameTable", 550, 0, false, false), ppu(nes.ppu), cart(nes.cartridge) {
	const u32 red = 0xFF000033;
	for (u16 y = 8; y < height; y += 9)
		for (u16 x = 0; x < width; x++)
			screenArray[y * width + x] = red;

	for (u16 x = 8; x < width; x += 9)
		for (u16 y = 0; y < height; y++)
			screenArray[y * width + x] = red;
}

void NameTableWindow::RenderContent() {
	for (u8 nameTable = 0; nameTable < 4; nameTable++)
		for (u8 y = 0; y < 30; y++)
			for (u8 x = 0; x < 32; x++)
				DrawTile(x, y, nameTable);

	Window::RenderContent();
}

void NameTableWindow::DrawTile(u8 x, u8 y, u8 nameTable) {
	u8 patterntable = 0;
	u8 tileIndex = ppu.MirroredVRAMRead(nameTable*0x400 + y*32 + x + 0x2000);

	u8 patternX = (x & 0x001F) / 4;
	u8 patternY = ((x & 0xFFE0) >> 5) / 4;

	u8 patternIndex = patternY * 8 + patternX;
	u8 patternSection = ((x & 0x02) >> 1) + ((x & 0x40) >> 5);

	u8 attributes = ppu.MirroredVRAMRead(0x23C0 + ppu.PPUCTRL.Nametable * 64 + patternIndex);
	u8 attributesShifted = (attributes >> (patternSection << 1)) & 0x03;

	u16 chrAddress = patterntable*0x1000 + tileIndex * 16;

	// Top Left or right nametable
	u32 nameTableOffset = (nameTable % 2) * ((width / 2) + 1);
	if (nameTable > 1) // Bottom nametables
		nameTableOffset += width * (height / 2 + 1);

	u32 tileStart = y * 9 * width + x * 9 + nameTableOffset;
	for (u8 line = 0; line < 8; line++) {
		u16 lowByte = cart.ReadChr(chrAddress + line);
		u16 highByte = cart.ReadChr(chrAddress + line + 8) << 1;

		u32 lineStart = tileStart + line * width;
		for (u8 bit = 7; bit < 8; bit--) {
			u8 lBit = (lowByte >> bit) & 0x01;
			u8 hBit = (highByte >> bit) & 0x02;

			u8 color = lBit | hBit;

			u32 pixelIndex = lineStart + (7 - bit);
			u8 paletteIndex = color == 0 ? 0 : attributesShifted * 4 + color;
			screenArray[pixelIndex] = ppu.paletteRGBA[ppu.palette[paletteIndex]];
		}
	}
}
