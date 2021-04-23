#include "NES.h"

NES::NES(const std::string& romPath) : cartridge(romPath), memory(&cartridge), cpu(memory), ppu(&cartridge) {
    ppu.cpu = &cpu;
    memory.cpu = &cpu;
    memory.ppu = &ppu;
    memory.controller = &controller;
    cpu.ppu = &ppu;

    cpu.nes = this;
    cycleAccurate = true;
}
NES::~NES() {}

void NES::DoInstruction() {
    cpu.Update();
    if (!cycleAccurate) DoCycles(cpu.lastOpCycles);
    controller.Update();
}

void NES::DoCycles(u8 cpuCycles) {
    frameFinished |= ppu.Update(cpuCycles * 3);
    // TODO update sound
}

void NES::Reset() {
    cpu.Reset();
    ppu.Reset();
}
