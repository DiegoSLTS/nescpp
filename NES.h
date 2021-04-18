#pragma once

#include "CPU.h"
#include "PPU.h"
#include "Memory.h"
#include "Cartridge.h"
#include "Controller.h"

class NES {
public:
	CPU cpu;
	PPU ppu;
	Memory memory;
	Cartridge cartridge;
	Controller controller; // TODO split into 2 controllers

	bool cycleAccurate = false;

	bool frameFinished = false;

	NES(const std::string& romPath);
	virtual ~NES();

	void DoInstruction();
	void DoCycles(u8 cpuCycles);
	void Reset();
	void DumpLogs();
};