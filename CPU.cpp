#include "CPU.h"
#include "Memory.h"

CPU::CPU(Memory& memory) : memory(memory) {}

void CPU::Update() {
	u8 opCode = ReadAtPC();
	ExecuteOpCode(opCode);
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

u8 CPU::PeakOpCode() {
	return memory.Read(PC);
}

u8 CPU::PeakOperand8() {
	return memory.Read(PC + 1);
}

u16 CPU::PeakOperand16() {
	return (memory.Read(PC + 2) << 8) | PeakOperand8();
}

u16 CPU::MakeZeroPage() {
	return ReadAtPC();
}

u16 CPU::MakeZeroPageX() {
	u16 address = (ReadAtPC() + X) & 0x00FF;
	CountCycles(1);
	return address;
}

u16 CPU::MakeZeroPageY() {
	return (ReadAtPC() + Y) & 0x00FF;
}

u16 CPU::MakeAbsolute() {
	return ReadAtPC() | (ReadAtPC() << 8);
}

u16 CPU::MakeAbsoluteX() {
	u16 address = MakeAbsolute() + X;
	// if lsb of address is less than X it means it carried to msb
	// so we have to wait one extra cycle
	if ((address & 0xFF) < X)
		CountCycles(1);
	return address;
}

u16 CPU::MakeAbsoluteY() {
	u16 address = MakeAbsolute() + Y;
	// if lsb of address is less than Y it means it carried to msb
	// so we have to wait one extra cycle
	if ((address & 0xFF) < Y)
		CountCycles(1); // Crossed page bound
	return address;
}

u16 CPU::MakeIndirect() {
	// If absAddressLsb is 0xFF there's no carry to absAddressMsb so absAddress breaks
	// I handle absAddressLsb++ in a byte so it wraps to 0 to simulate the bug
	u8 absAddressLsb = ReadAtPC();
	u8 absAddressMsb = ReadAtPC();
	u8 addressLsb = Read((absAddressMsb << 8) | (absAddressLsb++));
	u8 addressMsb = Read((absAddressMsb << 8) | absAddressLsb);

	return (addressMsb << 8) | addressLsb;
}

u16 CPU::MakeIndirectX() {
	u16 zpAddress = (ReadAtPC() + X) & 0x00FF;
	CountCycles(1);
	return Read(zpAddress) | (Read(zpAddress + 1) << 8);
}

u16 CPU::MakeIndirectY() {
	u16 zpAddress = ReadAtPC();
	u16 address = Read(zpAddress) | (Read(zpAddress + 1) << 8);
	address += Y;
	// if lsb of address is less than Y it means it carried to msb
	// so we have to wait one extra cycle
	if ((address & 0xFF) < Y)
		CountCycles(1);
	return address;
}

void CPU::Push8(u8 Value) {
	Write(Value, 0x0100 | (S--));
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
	// Incrementing S after reading
	// Instructions that pull must call IncS once before multiple Pulls
	return Read(0x0100 | (S++));
}

u16 CPU::Pull16() {
	return Pull8() | (Pull8() << 8);
}

void CPU::ExecuteOpCode(u8 opCode) {
	switch (opCode) {
	case 0x69: ADC(ReadAtPC()); break;
	case 0x65: ADC(Read(MakeZeroPage())); break;
	case 0x75: ADC(Read(MakeZeroPageX())); break;
	case 0x6D: ADC(Read(MakeAbsolute())); break;
	case 0x7D: ADC(Read(MakeAbsoluteX())); break;
	case 0x79: ADC(Read(MakeAbsoluteY())); break;
	case 0x61: ADC(Read(MakeIndirectX())); break;
	case 0x71: ADC(Read(MakeIndirectY())); break;
	case 0x29: AND(ReadAtPC()); break;
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
	case 0x1E: ASL(MakeAbsoluteX()); break; // TODO 7 cycles always, doesn't care about crossing pages
	case 0x90: BCC(ReadAtPC()); break;
	case 0xB0: BCS(ReadAtPC()); break;
	case 0xF0: BEQ(ReadAtPC()); break;
	case 0xD0: BNE(ReadAtPC()); break;
	case 0x30: BMI(ReadAtPC()); break;
	case 0x10: BPL(ReadAtPC()); break;
	case 0x50: BVC(ReadAtPC()); break;
	case 0x70: BVS(ReadAtPC()); break;
	case 0x24: BIT(Read(MakeZeroPage())); break;
	case 0x2C: BIT(Read(MakeAbsolute())); break;
	case 0x00: BRK(); break;
	case 0x18: CLC(); break;
	case 0xD8: CLD(); break;
	case 0x58: CLI(); break;
	case 0xB8: CLV(); break;
	case 0xC9: CMP(ReadAtPC()); break;
	case 0xC5: CMP(Read(MakeZeroPage())); break;
	case 0xD5: CMP(Read(MakeZeroPageX())); break;
	case 0xCD: CMP(Read(MakeAbsolute())); break;
	case 0xDD: CMP(Read(MakeAbsoluteX())); break;
	case 0xD9: CMP(Read(MakeAbsoluteY())); break;
	case 0xC1: CMP(Read(MakeIndirectX())); break;
	case 0xD1: CMP(Read(MakeIndirectY())); break;
	case 0xE0: CPX(ReadAtPC()); break;
	case 0xE4: CPX(Read(MakeZeroPage())); break;
	case 0xEC: CPX(Read(MakeAbsolute())); break;
	case 0xC0: CPY(ReadAtPC()); break;
	case 0xC4: CPY(Read(MakeZeroPage())); break;
	case 0xCC: CPY(Read(MakeAbsolute())); break;
	case 0xC6: DEC(MakeZeroPage()); break;
	case 0xD6: DEC(MakeZeroPageX()); break;
	case 0xCE: DEC(MakeAbsolute()); break;
	case 0xDE: DEC(MakeAbsoluteX()); break; // TODO 7 cycles always, doesn't care about crossing pages
	case 0xCA: DEX(); break;
	case 0x88: DEY(); break;
	case 0x49: EOR(ReadAtPC()); break;
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
	case 0xA9: LDA(ReadAtPC()); break;
	case 0xA5: LDA(Read(MakeZeroPage())); break;
	case 0xB5: LDA(Read(MakeZeroPageX())); break;
	case 0xAD: LDA(Read(MakeAbsolute())); break;
	case 0xBD: LDA(Read(MakeAbsoluteX())); break;
	case 0xB9: LDA(Read(MakeAbsoluteY())); break;
	case 0xA1: LDA(Read(MakeIndirectX())); break;
	case 0xB1: LDA(Read(MakeIndirectY())); break;
	case 0xA2: LDX(ReadAtPC()); break;
	case 0xA6: LDX(Read(MakeZeroPage())); break;
	case 0xB6: LDX(Read(MakeZeroPageY())); break;
	case 0xAE: LDX(Read(MakeAbsolute())); break;
	case 0xBE: LDX(Read(MakeAbsoluteY())); break;
	case 0xA0: LDY(ReadAtPC()); break;
	case 0xA4: LDY(Read(MakeZeroPage())); break;
	case 0xB4: LDY(Read(MakeZeroPageX())); break;
	case 0xAC: LDY(Read(MakeAbsolute())); break;
	case 0xBC: LDY(Read(MakeAbsoluteX())); break;
	case 0x4A: LSR(); break;
	case 0x46: LSR(MakeZeroPage()); break;
	case 0x56: LSR(MakeZeroPageX()); break;
	case 0x4E: LSR(MakeAbsolute()); break;
	case 0x5E: LSR(MakeAbsoluteX()); break; // TODO 7 cycles always, doesn't care about crossing pages
	case 0xEA: NOP(); break;
	case 0x09: ORA(ReadAtPC()); break;
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
	case 0x3E: ROL(MakeAbsoluteX()); break; // TODO 7 cycles always, doesn't care about crossing pages
	case 0x6A: ROR(); break;
	case 0x66: ROR(MakeZeroPage()); break;
	case 0x76: ROR(MakeZeroPageX()); break;
	case 0x6E: ROR(MakeAbsolute()); break;
	case 0x7E: ROR(MakeAbsoluteX()); break; // TODO 7 cycles always, doesn't care about crossing pages
	case 0x40: RTI(); break;
	case 0x60: RTS(); break;
	case 0xE9: SBC(ReadAtPC()); break;
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
	case 0x9D: STA(MakeAbsoluteX()); break; // TODO always 5 cycles
	case 0x99: STA(MakeAbsoluteY()); break; // TODO always 5 cycles
	case 0x81: STA(MakeIndirectX()); break;
	case 0x91: STA(MakeIndirectY()); break; // TODO always 6 cycles
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
	u8 bit0 = M & 0x01;
	M >>= 1;
	SetFlag(Flag::C, bit0 > 0);
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
	CountCycles(1);
	return M;
}

void CPU::LSR() {
	A = LSR_Internal(A);
}

void CPU::LSR(u16 address) {
	Write(LSR_Internal(Read(address)), address);
}

void CPU::NOP() {
	CountCycles(1);
}

u8 CPU::ROL_Internal(u8 M) {
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
	A = ROL_Internal(A);
}

void CPU::ROL(u16 address) {
	Write(ROL_Internal(Read(address)), address);
}

u8 CPU::ROR_Internal(u8 M) {
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
	A &= M;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
}

void CPU::ASL() {
	A = ASL_Internal(A);
}

void CPU::ASL(u16 address) {
	Write(ASL_Internal(Read(address)), address);
}

u8 CPU::ASL_Internal(u8 M) {
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
		u16 msb = PC & 0xFF00;
		PC += AddressOffset;
		if (msb != (PC & 0xFF00))
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
	M &= A;
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::V, (M & 0x40) > 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
}

void CPU::BMI(s8 addressOffset) {
	BranchWithFlag(Flag::N, true, addressOffset);
}

void CPU::BPL(s8 addressOffset) {
	BranchWithFlag(Flag::N, false, addressOffset);
}

void CPU::BRK() {
	ReadAtPC(); // next byte after opcode must be discarded, PC must be incremented
	Push16(PC);
	Push8(P | Flag::B);
	SetFlag(Flag::I, true);
	PC = Read(0xFFFE) | (Read(0xFFFF) << 8);
}

void CPU::BVC(s8 addressOffset) {
	BranchWithFlag(Flag::V, false, addressOffset);
}

void CPU::BVS(s8 addressOffset) {
	BranchWithFlag(Flag::V, true, addressOffset);
}

void CPU::Compare(u8 R, u8 M) {
	M = R - M;
	SetFlag(Flag::C, M > 0);
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
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
	u8 M = Read(address) - 1;
	SetFlag(Flag::Z, M == 0);
	SetFlag(Flag::N, (M & 0x80) > 0);
	CountCycles(1);
	Write(M, address);
}

void CPU:: EOR(u8 M) {
	A ^= M;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
}

void CPU::INC(u16 address) {
	u8 M = Read(address) + 1;
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
	// The 6502 actually pushes the PC to the stack after reading
	// the lsb of the absolute address, so the pushed value is not
	// the address of the next instruction (PC is still one byte behind).
	// RTI increments it when pulling PC to compensate
	Push16(PC - 1);

	PC = Address;
	CountCycles(1);
}

void CPU::LDA(u8 M) {
	A = M;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
}

void CPU::LDX(u8 M) {
	X = M;
	SetFlag(Flag::Z, X == 0);
	SetFlag(Flag::N, (X & 0x80) > 0);
}

void CPU::LDY(u8 M) {
	Y = M;
	SetFlag(Flag::Z, Y == 0);
	SetFlag(Flag::N, (Y & 0x80) > 0);
}

void CPU::ORA(u8 M) {
	A |= M;
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
}

void CPU::PHA() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	CountCycles(1);
	Push8(A);
}

void CPU::PHP() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	CountCycles(1);
	Push8(P | Flag::B);
}

void CPU::PLA() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	CountCycles(1);
	IncS();
	A = Pull8();
}

void CPU::PLP() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	CountCycles(1);
	IncS();
	P = Pull8();
}

void CPU::RTI() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	CountCycles(1);
	IncS();
	P = Pull8();
	PC = Pull16();
}

void CPU::RTS() {
	// 6502 reads the byte at PC but does nothing with it
	// and doesn't increment it
	CountCycles(1);
	IncS();
	PC = Pull16() + 1;
	CountCycles(1);
}

void CPU::SBC(u8 M) {
	u8 C = !HasFlag(Flag::C) ? 1 : 0;
	u16 temp16 = A + (0xFF - M - C);
	u8 temp8 = (u8)temp16;
	SetFlag(Flag::V, ((A ^ temp8) & (M ^ temp8) & 0x80) > 0); // TODO verificar

	A = temp8;
	SetFlag(Flag::C, temp16 > 0xFF);
	SetFlag(Flag::Z, A == 0);
	SetFlag(Flag::N, (A & 0x80) > 0);
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
