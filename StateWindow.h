#pragma once

#include "Types.h"
#include "Window.h"

class CPU;

class StateWindow : public Window {
public:
	StateWindow(CPU& gameBoy);
	virtual ~StateWindow();

	virtual void RenderContent() override;

private:
	CPU& cpu;

	sf::Font font;

	// cpu
	sf::Text labelPC;
	sf::Text labelS;
	sf::Text labelP;
	sf::Text labelA;
	sf::Text labelX;
	sf::Text labelY;
	sf::Text labelInst;

	sf::Text valuePC;
	sf::Text valueS;
	sf::Text valueP;
	sf::Text valueA;
	sf::Text valueX;
	sf::Text valueY;
	sf::Text valueInst;

	std::vector<sf::Text*> allTexts;

	void SetupLabel(sf::Text& label, const std::string& text, float x, float y);
	void SetupValue(sf::Text& value, float x, float y);
	std::string PeakInst();
	std::string Flags();
};
