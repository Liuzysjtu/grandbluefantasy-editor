#include "MemoryUtils.h"
#include <tlhelp32.h>
#include <fstream>

// 在这里实现 readItemsFromFile, GetModuleBaseAddress, FindDmaAddy, 和 GetProcessId 函数
std::vector<ItemDescription> readItemsFromFile(const std::string& filePath, const std::vector<LONG64>& offsets, LONG64 baseOffset, LONG64 offsetIncrement) {
    std::vector<ItemDescription> items;
    std::ifstream file(filePath);
    std::string line;
    int index = 0;

    while (std::getline(file, line)) {
        if (!line.empty()) {
            ItemDescription item{ line };
            // 计算每个item的偏移
            LONG64 itemOffset = baseOffset + static_cast<LONG64>(index++) * offsetIncrement;
            std::vector<LONG64> finalOffsets = offsets;
            finalOffsets.push_back(itemOffset); // 将这个特定项目的最终偏移添加到路径末尾
            item.finalOffsets = finalOffsets;
            items.push_back(item);
        }
    }
    return items;
}


DWORD_PTR GetModuleBaseAddress(DWORD procId, const wchar_t* modName) {
    DWORD_PTR modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (wcscmp(modEntry.szModule, modName) == 0) {
                    modBaseAddr = (DWORD_PTR)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
        CloseHandle(hSnap);
    }
    return modBaseAddr;
}

DWORD_PTR FindDmaAddy(HANDLE hProc, DWORD_PTR baseAddr, std::vector<LONG64> offsets) {
    DWORD_PTR addr = baseAddr;
    for (unsigned int i = 0; i < offsets.size(); ++i) {
        ReadProcessMemory(hProc, (LPCVOID)addr, &addr, sizeof(addr), NULL);
        addr += offsets[i];
    }
    return addr;
}

DWORD GetProcessId(const wchar_t* processName) {
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processesSnapshot == INVALID_HANDLE_VALUE) return 0;

    Process32First(processesSnapshot, &processInfo);
    if (!wcscmp(processName, processInfo.szExeFile)) {
        CloseHandle(processesSnapshot);
        return processInfo.th32ProcessID;
    }

    while (Process32Next(processesSnapshot, &processInfo)) {
        if (!wcscmp(processName, processInfo.szExeFile)) {
            CloseHandle(processesSnapshot);
            return processInfo.th32ProcessID;
        }
    }

    CloseHandle(processesSnapshot);
    return 0;
}