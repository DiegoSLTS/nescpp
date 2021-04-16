#include "Controller.h"

u8 Controller::Read(u16 address) {
	u8* statePtr = address == 0x4016 ? &controller1State : &controller2State;
	u8 state = *statePtr;
	u8 bit = state & 0x01;
	if (reg == 0) {
		state >>= 1;
		state |= 0x80;
		*statePtr = state;
	}
	return bit;
}

void Controller::Write(u8 value, u16 address) {
	if (address == 0x4016)
		reg = value;
}

void Controller::Update() {
	if (reg == 1) {
		controller1State = 0;
		controller2State = 0;
	}
}