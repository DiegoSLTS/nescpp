#include "Controller.h"

#include <SFML\Window\Keyboard.hpp>
#include <SFML\Window\Joystick.hpp>

enum XBoxButton : u8 {
	A = 0,
	B,
	X,
	Y,
	LB,
	RB,
	Back,
	Start,
	LSB,
	RSB
};

enum XBoxAxis : u8 {
	LStickX = 0, // +Right, -Left
	LStickY, // +Down, -Up
	Trigger, // +L, -R
	Unused,
	RStickX, // +Right, -Left
	RStickY, // +Down, -Up
	DPadX, // +Right, -Left
	DPadY, // +Up, -Down
};

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
	if (reg == 0)
		return;

	controller1State = 0;
	controller2State = 0;

	// controller 1
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) // A
		controller1State |= 0x01;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) // B
		controller1State |= 0x02;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)) // select
		controller1State |= 0x04;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) // start
		controller1State |= 0x08;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
		controller1State |= 0x10;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
		controller1State |= 0x20;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
		controller1State |= 0x40;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
		controller1State |= 0x80;
	
	// controller 2
	if (!sf::Joystick::isConnected(0))
		return;

	if (sf::Joystick::isButtonPressed(0, XBoxButton::B)) // A
		controller2State |= 0x01;

	if (sf::Joystick::isButtonPressed(0, XBoxButton::A)) // B
		controller2State |= 0x02;

	if (sf::Joystick::isButtonPressed(0, XBoxButton::Back))
		controller2State |= 0x04;

	if (sf::Joystick::isButtonPressed(0, XBoxButton::Start))
		controller2State |= 0x08;

	float dpad = sf::Joystick::getAxisPosition(0, (sf::Joystick::Axis)XBoxAxis::DPadY);
	if (dpad > 90.0f) // up
		controller2State |= 0x10;

	if (dpad < -90.0f) // down
		controller2State |= 0x20;

	dpad = sf::Joystick::getAxisPosition(0, (sf::Joystick::Axis)XBoxAxis::DPadX);
	if (dpad < -90.0f) // left
		controller2State |= 0x40;

	if (dpad > 90.0f) // right
		controller2State |= 0x80;
}