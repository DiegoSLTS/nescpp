#pragma once

#include "Types.h"

#include <sstream>

class NES;
class PPU;
class Memory;
class StateWindow;

enum Flag : u8 {
	C = 1 << 0,
	Z = 1 << 1,
	I = 1 << 2,
	D = 1 << 3,
	B = 1 << 4,
	// 1 << 5 not used
	V = 1 << 6,
	N = 1 << 7
};

class CPU {
	friend StateWindow;
public:
	CPU(Memory& memory);

	u8 A = 0, X = 0, Y = 0;
	u16 PC;
	u8 S = 0;
	u8 P = 0x04;

	u8 lastOpCycles = 0;
	unsigned long cpuCycles = 0;
	unsigned long instructionsCount = 0;

	Memory& memory;
	PPU* ppu = nullptr;

	NES* nes = nullptr;

	void Reset();
	void Update();

	bool NMIRequested = false;
	bool delayInterruptHandling = false;
	bool IsIRQRequested() const;
	
	void StartOAMDMA(u8 page);

private:
	std::stringstream* logStream;
	bool log = false;

	bool dmaInProgress = false;
	u16 dmaCycles = 0;
	u16 dmaAddress = 0;

	void SetFlag(Flag flagBit, bool set);
	bool HasFlag(Flag flagBit) const;

	void ExecuteOpCode(u8 opCode);
	void CountCycles(u8 cycles);

	// stack
	void Push8(u8 Value);
	void Push16(u16 Value);
	u8 Pull8();
	u16 Pull16();
	// used to move the stack before pulling one byte, which takes one cycle
	// pulls will increment S without consuming a cycle, so consecutive pulls
	// require just one IncS call
	void IncS();
	//

	u8 ReadAtPC();
	u8 Read(u16 address);
	void Write(u8 value, u16 address);

	u8 Peak8(u16 address);
	u16 Peak16(u16 address);
	u8 PeakOpCode();
	u8 PeakOperand8();
	u16 PeakOperand16();

	// addressing modes
	u16 MakeImmediate();
	u16 MakeZeroPage();
	u16 MakeZeroPageX();
	u16 MakeZeroPageY();
	u16 MakeRelative();
	u16 MakeAbsolute();
	u16 MakeAbsoluteX(bool alwaysExtraCycle = false);
	u16 MakeAbsoluteY(bool alwaysExtraCycle = false);
	u16 MakeIndirect();
	u16 MakeIndirectX();
	u16 MakeIndirectY(bool alwaysExtraCycle = false);
	//

	// instructions
	void CLC();
	void CLD();
	void CLI();
	void CLV();
	void DEX();
	void DEY();
	void INX();
	void INY();
	u8 LSR_Internal(u8 M);
	void LSR();
	void LSR(u16 address);
	void NOP();
	u8 ROL_Internal(u8 M);
	void ROL();
	void ROL(u16 address);
	u8 ROR_Internal(u8 M);
	void ROR();
	void ROR(u16 address);
	void SEC();
	void SED();
	void SEI();
	void TAX();
	void TAY();
	void TSX();
	void TXA();
	void TXS();
	void TYA();

	void ADC(u8 M);
	void AND(u8 M);
	u8 ASL_Internal(u8 M);
	void ASL();
	void ASL(u16 address);
	void BCC(s8 addressOffset);
	void BCS(s8 addressOffset);
	void BEQ(s8 addressOffset);
	void BIT(u8 M);
	void BMI(s8 addressOffset);
	void BNE(s8 addressOffset);
	void BPL(s8 addressOffset);
	void BRK(u16 vectorAddress = 0xFFFE, bool isInterrupt = false);
	void BVC(s8 addressOffset);
	void BVS(s8 addressOffset);
	void CMP(u8 M);
	void CPX(u8 M);
	void CPY(u8 M);
	void DEC(u16 address);
	void EOR(u8 M);
	void INC(u16 address);
	void JMP(u16 Address);
	void JSR(u16 Address);
	void LDA(u8 M);
	void LDX(u8 M);
	void LDY(u8 M);
	void ORA(u8 M);
	void PHA();
	void PHP();
	void PLA();
	void PLP();
	void RTI();
	void RTS();
	void SBC(u8 M);
	void STA(u16 Address);
	void STX(u16 Address);
	void STY(u16 Address);
	//

	// helper functions
	void BranchWithFlag(Flag Flag, bool Set, s8 AddressOffset);
	void Compare(u8 R, u8 M);
	void InvalidOpCode();
	//
};