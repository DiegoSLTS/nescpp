#pragma once

#include <string>
#include <fstream>
#include <sstream>

class Logger {
public:
	static void Init(bool cpu, bool ppu);
	static void DumpLogs();

	static bool split;
	static bool logCPU;
	static bool logPPU;

	static std::stringstream* GetCPUStream();
	static std::stringstream* GetPPUStream();

private:
	static std::ofstream cpuFile;
	static std::ofstream ppuFile;
	static std::ofstream logFile;
	static std::stringstream cpuStrings;
	static std::stringstream ppuStrings;
	static std::stringstream logStrings;
};