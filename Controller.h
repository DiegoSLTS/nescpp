#pragma once

#include "Types.h"

class Controller {
public:
	u8 Read(u16 address);
	void Write(u8 value, u16 address);
	void Update();

private:
	u8 reg = 0; // $4016
	u8 controller1State = 0;
	u8 controller2State = 0;
};