#pragma once

#include "Window.h"

class NES;
class PPU;

class PalettesWindow : public Window {
public:
	PalettesWindow(NES& nes);

protected:
	virtual void RenderContent() override;

private:
	PPU& ppu;
	std::vector<sf::RectangleShape> colorShapes;
};