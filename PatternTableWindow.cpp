#include "PatternTableWindow.h"

#include "NES.h"

PatternTableWindow::PatternTableWindow(NES& nes) : Window(256 + 256 / 8 - 1, 128 + 128 / 8 - 1, "Pattern Tables", 550, 0, false, true), cart(nes.cartridge) {
	const u32 red = 0xFF000033;
	for (u16 y = 8; y < height; y += 9)
		for (u16 x = 0; x < width; x++)
			screenArray[y * width + x] = red;

	for (u16 x = 8; x < width; x += 9)
		for (u16 y = 0; y < height; y++)
			screenArray[y * width + x] = red;
}

void PatternTableWindow::RenderContent() {
	for (u8 patternTable = 0; patternTable < 2; patternTable++)
		for (u8 y = 0; y < 16; y++)
			for (u8 x = 0; x < 16; x++)
				DrawTile(x, y, patternTable);

	Window::RenderContent();
}

void PatternTableWindow::DrawTile(u8 x, u8 y, u8 patternTable) {
	u16 tileIndex = y * 16 + x;
	u16 chrAddress = patternTable * 0x1000 + tileIndex * 16;
	u32 tileStart = y * 9 * width + x * 9 + patternTable * ((width / 2) + 1);

	for (u8 line = 0; line < 8; line++) {
		u16 lowByte = cart.ReadChr(chrAddress + line);
		u16 highByte = cart.ReadChr(chrAddress + line + 8) << 1;

		u32 lineStart = tileStart + line * width;
		for (u8 bit = 0; bit < 8; bit++) {
			u8 lBit = (lowByte >> bit) & 0x01;
			u8 hBit = (highByte >> bit) & 0x02;

			u8 color = lBit | hBit;

			u32 abgr = 0xFF000000;
			u8 gray = color * 85;

			abgr |= gray;
			abgr |= (gray << 8);
			abgr |= (gray << 16);
			u32 pixelIndex = lineStart + (7 - bit);
			screenArray[pixelIndex] = abgr;
		}
	}
}