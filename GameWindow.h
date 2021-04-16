#pragma once

#include "Window.h"

class GameWindow : public Window {
public:
	GameWindow(unsigned int Width, unsigned int Height, const std::string& Title, const int XPosition, const int YPosition, bool Open = true, bool scale = true);
	virtual ~GameWindow();

	void DrawLine(u16 lineNumber, u32* lineColors);
};