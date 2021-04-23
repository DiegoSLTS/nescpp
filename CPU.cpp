#include "CPU.h"

#include "NES.h"
#include "PPU.h"
#include "Memory.h"
#include "Logger.h"

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
}

CPU::CPU(Memory& memory) : memory(memory) {
	logStream = Logger::GetCPUStream();
	log = Logger::logCPU;
}

void CPU::Reset() {
	P |= 0x04;
	S -= 3;
	CountCycles(5);
	PC = Read(0xFFFC) | (Read(0xFFFD) << 8);
	instructionsCount++;
}

void CPU::Update() {
	lastOpCycles = 0;
	if (dmaInProgress) {
		dmaCycles++;

		if ((dmaCycles & 0x01) == 0) // make sure 514 does not override sprite at 0
			ppu->Write(memory.Read(dmaAddress++), 0x2004);

		if (dmaCycles == 513) { // TODO 514 in some cases
			dmaInProgress = false;
			if (log) (*logStream) << "OAMDMA Finished" << std::endl;
		}

		CountCycles(1);
		return;
	}

	if (NMIRequested) {
		if (log) (*logStream) << "NMI cycles " << cpuCycles << " ";
		BRK(0xFFFA, true);
		NMIRequested = false;
	}

	if (IsIRQRequested()) {
		if (delayInterruptHandling)
			delayInterruptHandling = false;
		else
			BRK(0xFFFE, true);
	}

	if (PC == 0xE4FD)
		int a = 0;

	u8 opCode = ReadAtPC();
	ExecuteOpCode(opCode);
}

bool CPU::IsIRQRequested() const {
	return false;
};


void CPU::StartOAMDMA(u8 page) {
	dmaInProgress = true;
	dmaCycles = 0;
	dmaAddress = page << 8;
	if (log) (*logStream) << std::endl << "OAMDMA Started from " << ToHex(dmaAddress) << std::endl;
}

void CPU::SetFlag(Flag flagBit, bool set) {
	if (set)
		P |= flagBit;
	else
		P &= ~flagBit;
}

bool CPU::HasFlag(Flag flagBit) const {
	return (P & flagBit) != 0;
}

void CPU::CountCycles(u8 cycles) {
	lastOpCycles += cycles;
	cpuCycles += cycles;
	if (nes->cycleAccurate) nes->DoCycles(cycles);
}

u8 CPU::ReadAtPC() {
	return Read(PC++);
}

u8 CPU::Read(u16 address) {
	u8 value = memory.Read(address);
	CountCycles(1);
	return value;
}

void CPU::Write(u8 value, u16 address) {
	memory.Write(value, address);
	CountCycles(1);
}

u8 CPU::Peak8(u16 address) {
	return memory.Read(address);
}

u16 CPU::Peak16(u16 address) {
	u8 lsb = Peak8(address);
	u8 msb = Peak8(address + 1);

	return (msb << 8) | lsb;
}

u8 CPU::PeakOpCode() {
	return Peak8(PC);
}

u8 CPU::PeakOperand8() {
	return Peak8(PC + 1);
}

u16 CPU::PeakOperand16() {
	return Peak16(PC + 1);
}

u16 CPU::MakeImmediate() {
	if (log) (*logStream) << "#$" << ToHex(Peak8(PC));
	return PC++;
}

u16 CPU::MakeRelative() {
	if (log) (*logStream) << "*" << (s16)((s8)Peak8(PC));
	return PC++;
}

u16 CPU::MakeZeroPage() {
	if (log) (*logStream) << "$" << ToHex(Peak8(PC));
	return ReadAtPC();
}

u16 CPU::MakeZeroPageX() {
	if (log) (*logStream) << "$" << ToHex(Peak8(PC)) << ",X";
	u16 address = (ReadAtPC() + X) & 0x00FF;
	if (log) (*logStream) << " ADD = " << ToHex(address);
	CountCycles(1);
	return address;
}

u16 CPU::MakeZeroPageY() {
	if (log) (*logStream) << "$" << ToHex(Peak8(PC)) << ",Y";
	u16 address = (ReadAtPC() + Y) & 0x00FF;
	if (log) (*logStream) << " ADD = " << ToHex(address);
	return address;
}

u16 CPU::MakeAbsolute() {
	if (log) (*logStream) << "$" << ToHex(Peak16(PC));
	return ReadAtPC() | (ReadAtPC() << 8);
}

u16 CPU::MakeAbsoluteX(bool alwaysExtraCycle) {
	u16 address = MakeAbsolute() + X;
	if (log) (*logStream) << ",X ADD = " << ToHex(address);
	// if lsb of address is less than X it means it carried to msb
	// so we have to wait one extra cycle
	if (alwaysExtraCycle || (address & 0xFF) < X)
		CountCycles(1);
	return address;
}

u16 CPU::MakeAbsoluteY(bool alwaysExtraCycle) {
	u16 address = MakeAbsolute() + Y;
	if (log) (*logStream) << ",Y ADD = " << ToHex(address);
	// if lsb of address is less than Y it means it carried to msb
	// so we have to wait one extra cycle
	if (alwaysExtraCycle || (address & 0xFF) < Y)
		CountCycles(1); // Crossed page bound
	return address;
}

u16 CPU::MakeIndirect() {
	if (log) (*logStream) << "($" << ToHex(Peak16(PC)) << ")";
	// If absAddressLsb is 0xFF there's no carry to absAddressMsb so absAddress breaks
	// I handle absAddressLsb++ in a byte so it wraps to 0 to simulate the bug
	u8 absAddressLsb = ReadAtPC();
	u8 absAddressMsb = ReadAtPC();
	u8 addressLsb = Read((absAddressMsb << 8) | (absAddressLsb++));
	u8 addressMsb = Read((absAddressMsb << 8) | absAddressLsb);

	u16 address = (addressMsb << 8) | addressLsb;
	if (log) (*logStream) << " ADD = " << ToHex(address);
	return address;
}

u16 CPU::MakeIndirectX() {
	if (log) (*logStream) << "($" << ToHex(Peak8(PC)) << ",X)";
	u16 zpAddress = ReadAtPC() + X;
	CountCycles(1);
	u8 lsb = Read(zpAddress & 0x00FF);
	u8 msb = Read((zpAddress + 1) & 0x00FF);
	u16 address = (msb << 8) | lsb;
	if (log) (*logStream) << " ADD = " << ToHex(address);
	return address;
}

u16 CPU::MakeIndirectY(bool alwaysExtraCycle) {
	if (log) (*logStream) << "($" << ToHex(Peak8(PC)) << "),Y";
	u16 zpAddress = ReadAtPC();
	u8 lsb = Read(zpAddress);
	u8 msb = Read((zpAddress + 1) & 0xFF);
	u16 address = (msb << 8) | lsb;
	address += Y;
	if (log) (*logStream) << " ADD = " << ToHex(address);
	// if lsb of address is less than Y it means it carried to msb
	// so we have to wait one extra cycle
	if (alwaysExtraCycle || (address & 0xFF) < Y)
		CountCycles(1);
	return address;
}

void CPU::Push8(u8 Value) {
	Write(Value, 0x0100 | S);
	S--;
}

void CPU::Push16(u16 Value) {
	Push8((u8)(Value >> 8));
	Push8((u8)Value);
}

void CPU::IncS() {
	S++;
	CountCycles(1);
}

u8 CPU::Pull8() {
	S++;
	CountCycles(1);
	// Incrementing S after reading
	// Instructions that pull must call IncS once before multiple Pulls
	return Read(0x0100 | S);
}

u16 CPU::Pull16() {
	u8 lsb = Pull8();
	u8 msb = Pull8();
	return (msb << 8) | lsb;
}

void CPU::ExecuteOpCode(u8 opCode) {
	u8 tempP = P, tempS = S, tempA = A, tempX = X, tempY = Y;
	unsigned long tempI = instructionsCount, tempC = cpuCycles - 1; // -1 to ignore the opcode read
	if (log) (*logStream) << ToHex((u16)(PC - 1)) << ": " << inst[opCode] << " ";
	switch (opCode) {
	case 0x69: ADC(Read(MakeImmediate())); break;
	case 0x65: ADC(Read(MakeZeroPage())); break;
	case 0x75: ADC(Read(MakeZeroPageX())); break;
	case 0x6D: ADC(Read(MakeAbsolute())); break;
	case 0x7D: ADC(Read(MakeAbsoluteX())); break;
	case 0x79: ADC(Read(MakeAbsoluteY())); break;
	case 0x61: ADC(Read(MakeIndirectX())); break;
	case 0x71: ADC(Read(MakeIndirectY())); break;
	case 0x29: AND(Read(MakeImmediate())); break;
	case 0x25: AND(Read(MakeZeroPage())); break;
	case 0x35: AND(Read(MakeZeroPageX())); break;
	case 0x2D: AND(Read(MakeAbsolute())); break;
	case 0x3D: AND(Read(MakeAbsoluteX())); break;
	case 0x39: AND(Read(MakeAbsoluteY())); break;
	case 0x21: AND(Read(MakeIndirectX())); break;
	case 0x31: AND(Read(MakeIndirectY())); break;
	case 0x0A: ASL(); break;
	case 0x06: ASL(MakeZeroPage()); break;
	case 0x16: ASL(MakeZeroPageX()); break;
	case 0x0E: ASL(MakeAbsolute()); break;
	case 0x1E: ASL(MakeAbsoluteX(true)); break;
	case 0x90: BCC(Read(MakeRelative())); break;
	case 0xB0: BCS(Read(MakeRelative())); break;
	case 0xF0: BEQ(Read(MakeRelative())); break;
	case 0xD0: BNE(Read(MakeRelative())); break;
	case 0x30: BMI(Read(MakeRelative())); break;
	case 0x10: BPL(Read(MakeRelative())); break;
	case 0x50: BVC(Read(MakeRelative())); break;
	case 0x70: BVS(Read(MakeRelative())); break;
	case 0x24: BIT(Read(MakeZeroPage())); break;
	case 0x2C: BIT(Read(MakeAbsolute())); break;
	case 0x00: BRK(); break;
	case 0x18: CLC(); break;
	case 0xD8: CLD(); break;
	case 0x58: CLI(); break;
	case 0xB8: CLV(); break;
	case 0xC9: CMP(Read(MakeImmediate())); break;
	case 0xC5: CMP(Read(MakeZeroPage())); break;
	case 0xD5: CMP(Read(MakeZeroPageX())); break;
	case 0xCD: CMP(Read(MakeAbsolute())); break;
	case 0xDD: CMP(Read(MakeAbsoluteX())); break;
	case 0xD9: CMP(Read(MakeAbsoluteY())); break;
	case 0xC1: CMP(Read(MakeIndirectX())); break;
	case 0xD1: CMP(Read(MakeIndirectY())); break;
	case 0xE0: CPX(Read(MakeImmediate())); break;
	case 0xE4: CPX(Read(MakeZeroPage())); break;
	case 0xEC: CPX(Read(MakeAbsolute())); break;
	case 0xC0: CPY(Read(MakeImmediate())); break;
	case 0xC4: CPY(Read(MakeZeroPage())); break;
	case 0xCC: CPY(Read(MakeAbsolute())); break;
	case 0xC6: DEC(MakeZeroPage()); break;
	case 0xD6: DEC(MakeZeroPageX()); break;
	case 0xCE: DEC(MakeAbsolute()); break;
	case 0xDE: DEC(MakeAbsoluteX(true)); break;
	case 0xCA: DEX(); break;
	case 0x88: DEY(); break;
	case 0x49: EOR(Read(MakeImmediate())); break;
	case 0x45: EOR(Read(MakeZeroPage())); break;
	case 0x55: EOR(Read(MakeZeroPageX())); break;
	case 0x4D: EOR(Read(MakeAbsolute())); break;
	case 0x5D: EOR(Read(MakeAbsoluteX())); break;
	case 0x59: EOR(Read(MakeAbsoluteY())); break;
	case 0x41: EOR(Read(MakeIndirectX())); break;
	case 0x51: EOR(Read(MakeIndirectY())); break;
	case 0xE6: INC(MakeZeroPage()); break;
	case 0xF6: INC(MakeZeroPageX()); break;
	case 0xEE: INC(MakeAbsolute()); break;
	case 0xFE: INC(MakeAbsoluteX()); break;
	case 0xE8: INX(); break;
	case 0xC8: INY(); break;
	case 0x4C: JMP(MakeAbsolute()); break;
	case 0x6C: JMP(MakeIndirect()); break;
	case 0x20: JSR(MakeAbsolute()); break;
	case 0xA9: LDA(Read(MakeImmediate())); break;
	case 0xA5: LDA(Read(MakeZeroPage())); break;
	case 0xB5: LDA(Read(MakeZeroPageX())); break;
	case 0xAD: LDA(Read(MakeAbsolute())); break;
	case 0xBD: LDA(Read(MakeAbsoluteX())); break;
	case 0xB9: LDA(Read(MakeAbsoluteY())); break;
	case 0xA1: LDA(Read(MakeIndirectX())); break;
	case 0xB1: LDA(Read(MakeIndirectY())); break;
	case 0xA2: LDX(Read(MakeImmediate())); break;
	case 0xA6: LDX(Read(MakeZeroPage())); break;
	case 0xB6: LDX(Read(MakeZeroPageY())); break;
	case 0xAE: LDX(Read(MakeAbsolute())); break;
	case 0xBE: LDX(Read(MakeAbsoluteY())); break;
	case 0xA0: LDY(Read(MakeImmediate())); break;
	case 0xA4: LDY(Read(MakeZeroPage())); break;
	case 0xB4: LDY(Read(MakeZeroPageX())); break;
	case 0xAC: LDY(Read(MakeAbsolute())); break;
	case 0xBC: LDY(Read(MakeAbsoluteX())); break;
	case 0x4A: LSR(); break;
	case 0x46: LSR(MakeZeroPage()); break;
	case 0x56: LSR(MakeZeroPageX()); break;
	case 0x4E: LSR(MakeAbsolute()); break;
	case 0x5E: LSR(MakeAbsoluteX(true)); break;
	case 0xEA: NOP(); break;
	case 0x09: ORA(Read(MakeImmediate())); break;
	case 0x05: ORA(Read(MakeZeroPage())); break;
	case 0x15: ORA(Read(MakeZeroPageX())); break;
	case 0x0D: ORA(Read(MakeAbsolute())); break;
	case 0x1D: ORA(Read(MakeAbsoluteX())); break;
	case 0x19: ORA(Read(MakeAbsoluteY())); break;
	case 0x01: ORA(Read(MakeIndirectX())); break;
	case 0x11: ORA(Read(MakeIndirectY())); break;
	case 0x48: PHA(); break;
	case 0x08: PHP(); break;
	case 0x68: PLA(); break;
	case 0x28: PLP(); break;
	case 0x2A: ROL(); break;
	case 0x26: ROL(MakeZeroPage()); break;
	case 0x36: ROL(MakeZeroPageX()); break;
	case 0x2E: ROL(MakeAbsolute()); break;
	case 0x3E: ROL(MakeAbsoluteX(true)); break;
	case 0x6A: ROR(); break;
	case 0x66: ROR(MakeZeroPage()); break;
	case 0x76: ROR(MakeZeroPageX()); break;
	case 0x6E: ROR(MakeAbsolute()); break;
	case 0x7E: ROR(MakeAbsoluteX(true)); break;
	case 0x40: RTI(); break;
	case 0x60: RTS(); break;
	case 0xE9: SBC(Read(MakeImmediate())); break;
	case 0xE5: SBC(Read(MakeZeroPage())); break;
	case 0xF5: SBC(Read(MakeZeroPageX())); break;
	case 0xED: SBC(Read(MakeAbsolute())); break;
	case 0xFD: SBC(Read(MakeAbsoluteX())); break;
	case 0xF9: SBC(Read(MakeAbsoluteY())); break;
	case 0xE1: SBC(Read(MakeIndirectX())); break;
	case 0xF1: SBC(Read(MakeIndirectY())); break;
	case 0x38: SEC(); break;
	case 0xF8: SED(); break;
	case 0x78: SEI(); break;
	case 0x85: STA(MakeZeroPage()); break;
	case 0x95: STA(MakeZeroPageX()); break;
	case 0x8D: STA(MakeAbsolute()); break;
	case 0x9D: STA(MakeAbsoluteX(true)); break;
	case 0x99: STA(MakeAbsoluteY(true)); break;
	case 0x81: STA(MakeIndirectX()); break;
	case 0x91: STA(MakeIndirectY(true)); break;
	case 0x86: STX(MakeZeroPage()); break;
	case 0x96: STX(MakeZeroPageY()); break;
	case 0x8E: STX(MakeAbsolute()); break;
	case 0x84: STY(MakeZeroPage()); break;
	case 0x94: STY(MakeZeroPageX()); break;
	case 0x8C: STY(MakeAbsolute()); break;
	case 0xAA: TAX(); break;
	case 0xA8: TAY(); break;
	case 0xBA: TSX(); break;
	case 0x8A: TXA(); break;
	case 0x9A: TXS(); break;
	case 0x98: TYA(); break;
	default: InvalidOpCode();
	}

	instructionsCount++;

	if (log) {
		(*logStream) << " i = " << tempI << ", c = " << tempC;
		(*logStream) << " A = " << ToHex(tempA) << ", X = " << ToHex(tempX) << ", Y = " << ToHex(tempY);
		(*logStream) << " P = " << ToFlags(tempP) << ", S = " << ToHex(tempS) << std::endl;
	}
}

void CPU::InvalidOpCode() {
	// TODO log error
}

void CPU::CLC() {
	SetFlag(Flag::C, false);
	CountCycles(1);
}

void CPU::CLD() {
	SetFlag(Flag::D, false);
	CountCycles(1);
}

void CPU::CLI() {
	if (IsIRQRequested() && HasFlag(Flag::I))
		delayInterruptHandling = true;
	SetFlag(Flag::I, false);
	CountCycles(1);
}

void CPU::CLV() {
	SetFlag(Flag::V, false);
	CountCycles(1);
}

void CPU::DEX() {
	X--;
	SetFlag(Flag::Z, X == 0);
	SetFlag(Flag::N, (X & 0x80) > 0);
	CountCycles(1);
}

void CPU::DEY() {
	Y--;
	SetFlag(Flag::Z, Y == 0);
	SetFlag(Flag::N, (Y & 0x80) > 0);
	CountCycles(1);
}

void CPU::INX() {
	X++;
	SetFlag(Flag::Z, X == 0);
	SetFlag(Flag::N, (X & 0x80) > 0);
	CountCycles(1);
}

void CPU::INY() {
	Y++;
	SetFlag(Flag::Z, Y == 0);
	SetFlag(Flag::N, (Y & 0x80) > 0);
	CountCycles(1);
}

u8 CPU::LSR_Internal(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	u8 bit0 = M & 0x01;
	M >>= 1;
	SetFlag(Flag::C, bit0 > 0);
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
	CountCycles(1);
	return M;
}

void CPU::LSR() {
	if (log) (*logStream) << "A";
	A = LSR_Internal(A);
}

void CPU::LSR(u16 address) {
	Write(LSR_Internal(Read(address)), address);
}

void CPU::NOP() {
	CountCycles(1);
}

u8 CPU::ROL_Internal(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	u8 bit7 = M & 0x80;
	M <<= 1;
	if (HasFlag(Flag::C))
		M |= 0x01;
	SetFlag(Flag::C, bit7 > 0);
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
	CountCycles(1);
	return M;
}

void CPU::ROL() {
	if (log) (*logStream) << "A";
	A = ROL_Internal(A);
}

void CPU::ROL(u16 address) {
	Write(ROL_Internal(Read(address)), address);
}

u8 CPU::ROR_Internal(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	u8 bit0 = M & 0x01;
	M >>= 1;
	if (HasFlag(Flag::C))
		M |= 0x80;
	SetFlag(Flag::C, bit0 > 0);
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
	CountCycles(1);
	return M;
}

void CPU::ROR() {
	if (log) (*logStream) << "A";
	A = ROR_Internal(A);
}

void CPU::ROR(u16 address) {
	Write(ROR_Internal(Read(address)), address);
}

void CPU::SEC() {
	SetFlag(Flag::C, true);
	CountCycles(1);
}

void CPU::SED() {
	SetFlag(Flag::D, true);
	CountCycles(1);
}

void CPU::SEI() {
	SetFlag(Flag::I, true);
	CountCycles(1);
}

void CPU::TAX() {
	X = A;
	SetFlag(Flag::Z, X == 0);
	SetFlag(Flag::N, (X & 0x80) > 0);
	CountCycles(1);
}

void CPU::TAY() {
	Y = A;
	SetFlag(Flag::Z, Y == 0);
	SetFlag(Flag::N, (Y & 0x80) > 0);
	CountCycles(1);
}

void CPU::TSX() {
	X = S;
	SetFlag(Flag::Z, X == 0);
	SetFlag(Flag::N, (X & 0x80) > 0);
	CountCycles(1);
}

void CPU::TXA() {
	A = X;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
	CountCycles(1);
}

void CPU::TXS() {
	S = X;
	CountCycles(1);
}

void CPU::TYA() {
	A = Y;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
	CountCycles(1);
}

void CPU::ADC(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	u8 C = HasFlag(Flag::C) ? 1 : 0;
	u16 temp16 = A + M + C;
	u8 temp8 = (u8)temp16;
	SetFlag(Flag::V, ((A ^ temp8) & (M ^ temp8) & 0x80) > 0); // TODO verificar

	A = temp8;
	SetFlag(Flag::C, temp16 > 0xFF);
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
}

void CPU::AND(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	A &= M;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
}

void CPU::ASL() {
	if (log) (*logStream) << "A";
	A = ASL_Internal(A);
}

void CPU::ASL(u16 address) {
	Write(ASL_Internal(Read(address)), address);
}

u8 CPU::ASL_Internal(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	u8 bit7 = M & 0x80;
	M <<= 1;
	SetFlag(Flag::C, bit7 > 0);
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
	CountCycles(1);
	return M;
}

void CPU::BranchWithFlag(Flag Flag, bool Set, s8 AddressOffset) {
	if (HasFlag(Flag) == Set) {
		u8 msb = PC >> 8;
		PC += AddressOffset;
		if (msb != (PC >> 8))
			CountCycles(2);
		else
			CountCycles(1);
	}
}

void CPU::BCC(s8 addressOffset) {
	BranchWithFlag(Flag::C, false, addressOffset);
}

void CPU::BCS(s8 addressOffset) {
	BranchWithFlag(Flag::C, true, addressOffset);
}

void CPU::BEQ(s8 addressOffset) {
	BranchWithFlag(Flag::Z, true, addressOffset);
}

void CPU::BNE(s8 addressOffset) {
	BranchWithFlag(Flag::Z, false, addressOffset);
}

void CPU::BIT(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	u8 bit = A & M;
	SetFlag(Flag::Z, bit == 0);
	SetFlag(Flag::V, (M & 0x40) > 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
}

void CPU::BMI(s8 addressOffset) {
	BranchWithFlag(Flag::N, true, addressOffset);
}

void CPU::BPL(s8 addressOffset) {
	BranchWithFlag(Flag::N, false, addressOffset);
}

void CPU::BRK(u16 vectorAddress, bool isInterrupt) {
	if (log)
		(*logStream) << "brk before: PC = " << ToHex(PC) << ", P = " << ToHex(P) << ", S = " << ToHex(S) << std::endl;
	
	// BRK instruction must increment PC (there's an extra read but the value is not used)
	// NIM and IRQ does the read, but PC is NOT incremented
	if (!isInterrupt)
		ReadAtPC();
	else
		Read(PC);

	Push16(PC);
	if (!isInterrupt)
		Push8(P | 0x30);
	else
		Push8(P | 0x20);
	SetFlag(Flag::I, true);
	PC = Read(vectorAddress) | (Read(vectorAddress + 1) << 8);

	if (log)
		(*logStream) << "brk after: PC = " << ToHex(PC) << ", P = " << ToHex(P) << ", S = " << ToHex(S) << std::endl;
}

void CPU::BVC(s8 addressOffset) {
	BranchWithFlag(Flag::V, false, addressOffset);
}

void CPU::BVS(s8 addressOffset) {
	BranchWithFlag(Flag::V, true, addressOffset);
}

void CPU::Compare(u8 R, u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	u8 cmp = R - M;
	SetFlag(Flag::C, R >= M);
	SetFlag(Flag::Z, cmp == 0);
	SetFlag(Flag::N, (cmp & 0x80) > 0);
}

void CPU::CMP(u8 M) {
	Compare(A, M);
}

void CPU::CPX(u8 M) {
	Compare(X, M);
}

void CPU::CPY(u8 M) {
	Compare(Y, M);
}

void CPU::DEC(u16 address) {
	u8 M = Read(address);
	if (log) (*logStream) << " [M] = " << ToHex(M);
	M--;
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
	CountCycles(1);
	Write(M, address);
}

void CPU:: EOR(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	A ^= M;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
}

void CPU::INC(u16 address) {
	u8 M = Read(address);
	if (log) (*logStream) << " [M] = " << ToHex(M);
	M++;
	if (address == 0x0000 && M == 0)
		int a = 0;
	if (address == 0x0000 && M == 0xFF)
		int a = 0;
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
	CountCycles(1);
	Write(M, address);
}

void CPU::JMP(u16 Address) {
	PC = Address;
}

void CPU::JSR(u16 Address) {
	CountCycles(1);
	// PC is incremented while reading Address (absolute addressing), but
	// the 6502 actually pushes the PC to the stack after reading the lsb
	// of Address, so the pushed value is not the address of the next
	// instruction (PC is still one byte behind).
	// RTS increments it when pulling PC to compensate
	Push16(PC - 1);

	PC = Address;
	CountCycles(1);
}

void CPU::LDA(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	A = M;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
}

void CPU::LDX(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	X = M;
	SetFlag(Flag::Z, X == 0);
	SetFlag(Flag::N, (X & 0x80) > 0);
}

void CPU::LDY(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	Y = M;
	SetFlag(Flag::Z, Y == 0);
	SetFlag(Flag::N, (Y & 0x80) > 0);
}

void CPU::ORA(u8 M) {
	if (log) (*logStream) << " [M] = " << ToHex(M);
	A |= M;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
}

void CPU::PHA() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	Read(PC);
	Push8(A);
}

void CPU::PHP() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	Read(PC);
	Push8(P | 0x30);
}

void CPU::PLA() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	Read(PC);
	A = Pull8();
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80 ) > 0);
}

void CPU::PLP() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	Read(PC);
	u8 PFromStack = Pull8() & 0xCF;

	if (IsIRQRequested() && HasFlag(Flag::I) && ((PFromStack & Flag::I) == 0))
		delayInterruptHandling = true;

	P = PFromStack;
}

void CPU::RTI() {
	if (log)
		(*logStream) << "rti before: PC = " << ToHex(PC) << ", P = " << ToHex(P) << ", S = " << ToHex(S) << std::endl;
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	Read(PC);
	P = Pull8() & 0xCF;
	PC = Pull16();
	if (log)
		(*logStream) << "rti after: PC = " << ToHex(PC) << ", P = " << ToHex(P) << ", S = " << ToHex(S) << std::endl;
}

void CPU::RTS() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	Read(PC);
	PC = Pull16() + 1;
	CountCycles(1);
}

void CPU::SBC(u8 M) {
	// https://stackoverflow.com/questions/29193303/6502-emulation-proper-way-to-implement-adc-and-sbc
	ADC(~M);
}

void CPU::STA(u16 Address) {
	Write(A, Address);
}

void CPU::STX(u16 Address) {
	Write(X, Address);
}

void CPU::STY(u16 Address) {
	Write(Y, Address);
}
