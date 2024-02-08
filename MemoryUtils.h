#pragma once

#include <windows.h>
#include <vector>
#include <string>

struct ItemDescription {
    std::string name;
    std::vector<LONG64> finalOffsets;
};

std::vector<ItemDescription> readItemsFromFile(const std::string& filePath, const std::vector<LONG64>& offsets, LONG64 baseOffset, LONG64 offsetIncrement);

DWORD_PTR GetModuleBaseAddress(DWORD procId, const wchar_t* modName);

DWORD_PTR FindDmaAddy(HANDLE hProc, DWORD_PTR baseAddr, std::vector<LONG64> offsets);

DWORD GetProcessId(const wchar_t* processName);