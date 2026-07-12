#pragma once
#include "resource.h"

struct Config {
	bool useAccentColor;
	unsigned long color;
	unsigned long customColors[16];
};

BYTE* GetModuleAddress(DWORD pID);
BYTE* Scan(HANDLE hProcess, BYTE* base, DWORD size, const BYTE* pat, const char* mask);
DWORD FindProcess();
bool ApplyPatch(HANDLE hProcess, BYTE* addr1, BYTE* addr2, COLORREF color);

bool Initialize(HANDLE* hProcess, BYTE** address1, BYTE** address2, Config* config, HANDLE* hFile);
