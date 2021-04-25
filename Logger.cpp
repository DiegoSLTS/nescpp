#include "Logger.h"

bool Logger::split = false;
bool Logger::logCPU = false;
bool Logger::logPPU = false;
std::ofstream Logger::cpuFile;
std::ofstream Logger::ppuFile;
std::ofstream Logger::logFile;
std::stringstream Logger::cpuStrings;
std::stringstream Logger::ppuStrings;
std::stringstream Logger::logStrings;

void Logger::Init(bool cpu, bool ppu) {
	logCPU = cpu;
	logPPU = ppu;
	if (split) {
		if (logCPU)
			cpuFile.open("cpu.log", std::ofstream::out | std::ofstream::trunc);
		if (logPPU)
			ppuFile.open("ppu.log", std::ofstream::out | std::ofstream::trunc);
	} else {
		if (logCPU || logPPU)
			logFile.open("nes.log", std::ofstream::out | std::ofstream::trunc);
	}
}

std::stringstream* Logger::GetCPUStream() {
	return split ? &cpuStrings : &logStrings;
}

std::stringstream* Logger::GetPPUStream() {
	return split ? &ppuStrings : &logStrings;
}

void Logger::DumpLogs() {
	if (split) {
		if (logCPU) {
			cpuFile << cpuStrings.str();
			cpuStrings.str(std::string());
		}
		if (logCPU) {
			ppuFile << ppuStrings.str();
			ppuStrings.str(std::string());
		}
	} else {
		if (logCPU || logPPU) {
			logFile << logStrings.str();
			cpuStrings.str(std::string());
		}
	}
}