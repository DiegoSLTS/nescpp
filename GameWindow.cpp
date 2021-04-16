#include "GameWindow.h"

GameWindow::GameWindow(unsigned int Width, unsigned int Height, const std::string& Title, const int XPosition, const int YPosition, bool Open, bool scale) :
	Window(Width, Height, Title, XPosition, YPosition, Open, scale) {}

GameWindow::~GameWindow() {}

void GameWindow::DrawLine(u16 lineNumber, u32* lineColors) {
	u16 xMin = lineNumber * width;

	for (u16 x = 0; x < 256; x++) {
		u32 colorRGBA = lineColors[x];
		u32 colorABGR = (colorRGBA & 0x000000FF) << 24;
		colorABGR |= (colorRGBA & 0x0000FF00) << 8;
		colorABGR |= (colorRGBA & 0x00FF0000) >> 8;
		colorABGR |= (colorRGBA & 0xFF000000) >> 24;
		screenArray[xMin + x] = colorABGR;
	}
}