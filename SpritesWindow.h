#pragma once

#include "Window.h"

class NES;
class PPU;
class Cartridge;

class SpritesWindow : public Window {
public:
	SpritesWindow(NES& nes);

	PPU& ppu;
	Cartridge& cart;

protected:
	virtual void RenderContent() override;

private:
	void DrawSprite(u8 spriteIndex);
	void DrawTile(u8 tileIndex, u8 patternTable, u8 x, u8 y, u8 attributes);
};