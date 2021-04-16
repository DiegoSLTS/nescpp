#include "StateWindow.h"

#include "NES.h"
#include "Types.h"

namespace {
	std::string inst[256] = {
//        0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
/* 0 */	"BRK","ORA","___","___","___","ORA","ASL","___","PHP","ORA","ASL","___","___","ORA","ASL","___",
/* 1 */	"BPL","ORA","___","___","___","ORA","ASL","___","CLC","ORA","___","___","___","ORA","ASL","___",
/* 2 */	"JSR","AND","___","___","BIT","AND","ROL","___","PLP","AND","ROL","___","BIT","AND","ROL","___",
/* 3 */	"BMI","AND","___","___","___","AND","ROL","___","SEC","AND","___","___","___","AND","ROL","___",
/* 4 */	"RTI","EOR","___","___","___","EOR","LSR","___","PHA","EOR","LSR","___","JMP","EOR","LSR","___",
/* 5 */	"BVC","EOR","___","___","___","EOR","LSR","___","CLI","EOR","___","___","___","EOR","LSR","___",
/* 6 */	"RTS","ADC","___","___","___","ADC","ROR","___","PLA","ADC","ROR","___","JMP","ADC","ROR","___",
/* 7 */	"BVS","ADC","___","___","___","ADC","ROR","___","SEI","ADC","___","___","___","ADC","ROR","___",
/* 8 */	"___","STA","___","___","STY","STA","STX","___","DEY","___","TXA","___","STY","STA","STX","___",
/* 9 */	"BCC","STA","___","___","STY","STA","STX","___","TYA","STA","TXS","___","___","STA","___","___",
/* A */	"LDY","LDA","LDX","___","LDY","LDA","LDX","___","TAY","LDA","TAX","___","LDY","LDA","LDX","___",
/* B */	"BCS","LDA","___","___","LDY","LDA","LDX","___","CLV","LDA","TSX","___","LDY","LDA","LDX","___",
/* C */	"CPY","CMP","___","___","CPY","CMP","DEC","___","INY","CMP","DEX","___","CPY","CMP","DEC","___",
/* D */	"BNE","CMP","___","___","___","CMP","DEC","___","CLD","CMP","___","___","___","CMP","DEC","___",
/* E */	"CPX","SBC","___","___","CPX","SBC","INC","___","INX","SBC","NOP","___","CPX","SBC","INC","___",
/* F */	"BEQ","SBC","___","___","___","SBC","INC","___","SED","SBC","___","___","___","SBC","INC","___"
	};

	u8 length[256] = {
//      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
/* 0 */ 1, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2, 3, 3, 2,
/* 1 */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 2, 2, 2, 3, 3, 2,
/* 2 */ 3, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 2,
/* 3 */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 2, 2, 2, 3, 3, 2,
/* 4 */ 1, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 2,
/* 5 */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 2, 2, 2, 3, 3, 2,
/* 6 */ 1, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 2,
/* 7 */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 2, 2, 2, 3, 3, 2,
/* 8 */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 2,
/* 9 */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 1, 2, 2, 3, 2, 2,
/* A */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 2,
/* B */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 1, 2, 3, 3, 3, 2,
/* C */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 2,
/* D */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 2, 2, 2, 3, 3, 2,
/* E */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 2,
/* F */ 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 2, 2, 2, 3, 3, 2
	};
}

StateWindow::StateWindow(NES& nes) : Window(250, 144, "State", 550, 50, false, false), cpu(nes.cpu) {
	if (!font.loadFromFile("Pokemon_GB.ttf"))
	{
		// TODO log error
	}

	SetupLabel(labelPC, "PC", 2, 2);
	SetupLabel(labelS, "S", 2, 12);
	SetupLabel(labelP, "P", 2, 22);
	SetupLabel(labelA, "A", 2, 32);
	SetupLabel(labelX, "X", 2, 42);
	SetupLabel(labelY, "Y", 2, 52);
	SetupLabel(labelInst, "Inst", 2, 62);

	SetupValue(valuePC, 52, 2);
	SetupValue(valueS, 52, 12);
	SetupValue(valueP, 52, 22);
	SetupValue(valueA, 52, 32);
	SetupValue(valueX, 52, 42);
	SetupValue(valueY, 52, 52);
	SetupValue(valueInst, 52, 62);

	allTexts.push_back(&labelPC);
	allTexts.push_back(&labelS);
	allTexts.push_back(&labelP);
	allTexts.push_back(&labelA);
	allTexts.push_back(&labelX);
	allTexts.push_back(&labelY);
	allTexts.push_back(&labelInst);
	allTexts.push_back(&valuePC);
	allTexts.push_back(&valueS);
	allTexts.push_back(&valueP);
	allTexts.push_back(&valueA);
	allTexts.push_back(&valueX);
	allTexts.push_back(&valueY);
	allTexts.push_back(&valueInst);

	Update();
}

StateWindow::~StateWindow() {}

void StateWindow::RenderContent() {
    if (!IsOpen())
        return;

	valuePC.setString(ToHex(cpu.PC));
	valueS.setString(ToHex(cpu.S));
	valueP.setString(Flags());
	valueA.setString(ToHex(cpu.A));
	valueX.setString(ToHex(cpu.X));
	valueY.setString(ToHex(cpu.Y));
	valueInst.setString(PeakInst());

	for (auto it = allTexts.begin(); it != allTexts.end(); it++)
		renderWindow->draw(**it);
}

void StateWindow::SetupLabel(sf::Text& label, const std::string& text, float x, float y) {
	label.setFont(font);
	label.setString(text);
	label.setCharacterSize(10);
	label.setFillColor(sf::Color::White);
	label.setStyle(sf::Text::Style::Regular);
	label.setPosition(x, y);
}

void StateWindow::SetupValue(sf::Text& value, float x, float y) {
	value.setFont(font);
	value.setCharacterSize(10);
	value.setFillColor(sf::Color::White);
	value.setStyle(sf::Text::Style::Regular);
	value.setPosition(x, y);
}

std::string StateWindow::PeakInst() {
	u8 opCode = cpu.PeakOpCode();
	std::stringstream stream;
	stream << inst[opCode];
	if (length[opCode] == 2)
		stream << " " << ToHex(cpu.PeakOperand8());
	else if (length[opCode] == 3)
		stream << " " << ToHex(cpu.PeakOperand16());
	stream << " (" << ToHex(opCode) << ")";
	return stream.str();
}

std::string StateWindow::Flags() {
	std::stringstream stream;
	stream << (cpu.HasFlag(Flag::N) ? "n" : "-");
	stream << (cpu.HasFlag(Flag::V) ? "v" : "-");
	stream << "-";
	stream << (cpu.HasFlag(Flag::B) ? "b" : "-");
	stream << (cpu.HasFlag(Flag::D) ? "d" : "-");
	stream << (cpu.HasFlag(Flag::I) ? "i" : "-");
	stream << (cpu.HasFlag(Flag::Z) ? "z" : "-");
	stream << (cpu.HasFlag(Flag::C) ? "c" : "-");
	return stream.str();
}
