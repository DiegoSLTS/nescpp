#include "PalettesWindow.h"

#include "NES.h"

PalettesWindow::PalettesWindow(NES& nes) : Window(128, 128, "Palettes", 550, 0, false, true), ppu(nes.ppu) {
	// 64 for the full palette, 4*8 for each nes palette
	colorShapes.reserve(64 + 32);

	sf::RectangleShape shape(sf::Vector2f(8.0f, 8.0f));

	for (u8 i = 0; i < 64; i++) {
		shape.setFillColor(sf::Color(ppu.paletteRGBA[i]));
		shape.setPosition((i % 16) * 8, (i / 16) * 8);
		colorShapes.push_back(shape);
	}

	for (u8 i = 0; i < 32; i++) {
		shape.setFillColor(sf::Color::White);
		shape.setPosition((i % 16) * 8, (i / 16) * 9 + 40);
		colorShapes.push_back(shape);
	}
}

void PalettesWindow::RenderContent() {
	for (u8 i = 0; i < 32; i++) {
		sf::RectangleShape& colorRect = colorShapes.at(64 + i);
		colorRect.setFillColor(sf::Color(ppu.paletteRGBA[ppu.palette[i]]));
	}

	for (u8 i = 0; i < colorShapes.size(); i++)
		renderWindow->draw(colorShapes[i]);

	Window::RenderContent();
}
