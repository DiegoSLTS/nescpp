#pragma once

#include "Window.h"

class NES;
class Cartridge;

class PatternTableWindow : public Window {
public:
	PatternTableWindow(NES& nes);

	Cartridge& cart;

protected:
	virtual void RenderContent() override;
	void DrawTile(u8 x, u8 y, u8 patternTable);
};