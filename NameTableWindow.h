#pragma once

#include "Window.h"

class NES;
class PPU;
class Cartridge;

class NameTableWindow : public Window {
public:
	NameTableWindow(NES& nes);

	virtual void RenderContent() override;

	PPU& ppu;
	Cartridge& cart;

protected:
	void DrawTile(u8 x, u8 y, u8 nameTable);
};