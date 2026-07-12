#include "utils.h"

#include <tlhelp32.h>
#include <wchar.h>

BYTE* GetModuleAddress(DWORD pID) {
    HANDLE hSnapshot = INVALID_HANDLE_VALUE;
    MODULEENTRY32W me32 = {};
    LPVOID baseAddress = nullptr;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID);
    if (hSnapshot == INVALID_HANDLE_VALUE) { return nullptr; }
    me32.dwSize = sizeof(MODULEENTRY32W);

    if (Module32FirstW(hSnapshot, &me32)) {
        do {
            if (_wcsicmp(me32.szModule, L"comctl32.dll") == 0) {
                CloseHandle(hSnapshot);
                return me32.modBaseAddr;
            }
        } while (Module32NextW(hSnapshot, &me32));
    }
    else { return nullptr; }

    CloseHandle(hSnapshot);
    return nullptr;
}

BYTE* Scan(HANDLE hProcess, BYTE* base, DWORD size, const BYTE* pat, const char* mask) {
    BYTE* data = (BYTE*)malloc(size);
    if (data == nullptr) { return nullptr; }

    SIZE_T bytes = 0;
    DWORD patLen = (DWORD)strlen(mask);

    if (!ReadProcessMemory(hProcess, base, data, size, &bytes) || bytes != size) { return nullptr; }

    for (DWORD i = 0; i <= size - patLen; ++i) {
        bool match = true;
        for (DWORD j = 0; j < patLen; ++j) {
            if (mask[j] != '?' && data[i + j] != pat[j]) {
                match = false;
                break;
            }
        }
        if (match) { return base + i; }
    }

    return nullptr;
}

DWORD FindProcess() {
    HANDLE hProcessSnap;
    PROCESSENTRY32W pe32 = {};
    DWORD result = 0;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) { return 0; }

    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return 0;
    }

    do {
        if (_wcsicmp(pe32.szExeFile, L"explorer.exe") == 0) {
            result = pe32.th32ProcessID;
            break;
        }
    } while (Process32NextW(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return result;
}

bool Initialize(HANDLE* hProcess, BYTE** address1, BYTE** address2, Config* config, HANDLE* hFile) {
    DWORD pID = FindProcess();
    if (pID == 0) {
        MessageBoxW(nullptr, L"Counldn't find explorer.exe, is it running?", APP_NAME, MB_ICONERROR | MB_OK);
        return false;
    }

    BYTE* base = GetModuleAddress(pID);

    if (base == nullptr) {
        MessageBoxW(nullptr, L"Counldn't get base address.", APP_NAME, MB_ICONERROR | MB_OK);
        return false;
    }

    HANDLE hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pID);
    if (!hProc) {
        MessageBoxW(nullptr, L"Counldn't open process.", APP_NAME, MB_ICONERROR | MB_OK);
        return false;
    }

    IMAGE_DOS_HEADER dosHeader{};
    SIZE_T bytes = 0;
    if (!ReadProcessMemory(hProc, base, &dosHeader, sizeof(IMAGE_DOS_HEADER), &bytes) || bytes != sizeof(IMAGE_DOS_HEADER)) {
        MessageBoxW(nullptr, L"Failed to read DOS header.", APP_NAME, MB_ICONERROR | MB_OK);
        CloseHandle(hProc);
        return false;
    }

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        MessageBoxW(nullptr, L"Invalid DOS header signature.", APP_NAME, MB_ICONWARNING | MB_OK);
    }

    bytes = 0;
    IMAGE_NT_HEADERS nt{};
    if (!ReadProcessMemory(hProc, base + dosHeader.e_lfanew, &nt, sizeof(IMAGE_NT_HEADERS), &bytes) || bytes != sizeof(IMAGE_NT_HEADERS)) {
        MessageBoxW(nullptr, L"Failed to read NT headers.", APP_NAME, MB_ICONERROR | MB_OK);
        CloseHandle(hProc);
        return false;
    }

    IMAGE_SECTION_HEADER sh{};
    BYTE* textBase = nullptr;
    DWORD textSize = 0;
    BYTE* sectionTableAddr = base + dosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + nt.FileHeader.SizeOfOptionalHeader;

    for (int i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
        bytes = 0;
        if (!ReadProcessMemory(hProc, sectionTableAddr + i * sizeof(IMAGE_SECTION_HEADER), &sh, sizeof(IMAGE_SECTION_HEADER), &bytes) || bytes != sizeof(sh)) {
            MessageBoxW(nullptr, L"Failed to read image section header.", APP_NAME, MB_ICONERROR | MB_OK);
            CloseHandle(hProc);
            return false;
        }
        if (memcmp(sh.Name, ".text", 5) == 0) {
            textBase = base + sh.VirtualAddress;
            textSize = sh.Misc.VirtualSize;
            break;
        }
    }

    if (textBase == nullptr || textSize <= 0) {
        MessageBoxW(nullptr, L"Failed to find .text section.", APP_NAME, MB_ICONERROR | MB_OK);
        CloseHandle(hProc);
        return false;
    }

    BYTE* addr1 = Scan(hProc, textBase, textSize, pattern1, mask1);
    if (addr1 == nullptr) {
        MessageBoxW(nullptr, L"Couldn't find address!", APP_NAME, MB_ICONERROR | MB_OK);
        CloseHandle(hProc);
        return false;
    }
    addr1 += 19;

    BYTE* addr2 = Scan(hProc, textBase, textSize, pattern2, mask2);
    if (addr2 == nullptr) {
        MessageBoxW(nullptr, L"Couldn't find address!", APP_NAME, MB_ICONERROR | MB_OK);
        CloseHandle(hProc);
        return false;
    }
    addr2 += 7;

    Config cfg = { 0 };

    wchar_t path[MAX_PATH];
    GetTempPathW(MAX_PATH, path);
    wcscat_s(path, MAX_PATH, L"\\CDS.cfg");

    HANDLE file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        WriteFile(file, &cfg, sizeof(Config), nullptr, nullptr);
    } else { ReadFile(file, &cfg, sizeof(Config), nullptr, nullptr); }

    *config = cfg;
    *hFile = file;
    *hProcess = hProc;
    *address1 = addr1;
    *address2 = addr2;

    return true;
}


bool ApplyPatch(HANDLE hProcess, BYTE* addr1, BYTE* addr2, COLORREF color) {
    const BYTE r = GetRValue(color);
    const BYTE g = GetGValue(color);
    const BYTE b = GetBValue(color);

    const BYTE fill[5] = { 0xB8, r, g, b, 0x00 };
    const BYTE stroke[5] = { 0xB8, r, BYTE(g > 18 ? g - 18 : 0), BYTE(b > 11 ? b - 11 : 0), 0x00 };

    SIZE_T bytes = 0;

    if (!WriteProcessMemory(hProcess, addr1, stroke, 5, &bytes) || bytes != 5) {
        MessageBoxW(nullptr, L"Failed to write to process!", APP_NAME, MB_ICONERROR | MB_OK);
        return false;
    }

    addr2 -= 63;
    bytes = 0;
    if (!WriteProcessMemory(hProcess, addr2, fill, 5, &bytes) || bytes != 5) {
        MessageBoxW(nullptr, L"Failed to write to process!", APP_NAME, MB_ICONERROR | MB_OK);
        return false;
    }

    addr2 += 63;

    bytes = 0;
    if (!WriteProcessMemory(hProcess, addr2, fill, 5, &bytes) || bytes != 5) {
        MessageBoxW(nullptr, L"Failed to write to process!", APP_NAME, MB_ICONERROR | MB_OK);
        return false;
    }

    addr2 += 19;

    bytes = 0;
    if (!WriteProcessMemory(hProcess, addr2, fill, 5, &bytes) || bytes != 5) {
        MessageBoxW(nullptr, L"Failed to write to process!", APP_NAME, MB_ICONERROR | MB_OK);
        return false;
    }

    return true;
}