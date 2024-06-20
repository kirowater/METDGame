#include "stdafx.h"


HANDLE process = 0;
bool g_gameRunning = false;


bool MaskCompare(const char *s1, const char *s2, char *mask) {
	for (; *mask; ++mask, ++s1, ++s2) {
		if (*mask == 'x' && *s1 != *s2) {
			return false;
		}
	}

	return true;
}

DWORD FindPattern(HANDLE process, char *pattern, char *mask) {
	MODULEENTRY32 entry = { 0 };
	entry.dwSize = sizeof(MODULEENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(process));
	Module32First(snapshot, &entry);
	CloseHandle(snapshot);

	char *buffer = (char *)malloc(entry.modBaseSize);
	ReadProcessMemory(process, entry.modBaseAddr, buffer, entry.modBaseSize, 0);

	DWORD length = entry.modBaseSize - strlen(mask);
	for (DWORD i = 0; i < length; ++i) {
		if (MaskCompare(&buffer[i], pattern, mask)) {
			free(buffer);
			return (DWORD)entry.modBaseAddr + i;
		}
	}

	free(buffer);
	return 0;
}

PROCESSENTRY32 GetProcessInfoByName(wchar_t *exe_name) {
	PROCESSENTRY32 entry = { 0 };
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry)) {
		do {
			if (_wcsicmp(entry.szExeFile, exe_name) == 0) {
				CloseHandle(snapshot);
				return entry;
			}
		} while (Process32Next(snapshot, &entry));
	}

	CloseHandle(snapshot);
	return{ 0 };
}

bool IsMirrorsEdgeRunning() {
	DWORD pid = 0, tpid = 0;
	PROCESSENTRY32 entry;
	HANDLE snapshot;

	entry.dwSize = sizeof(entry);

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (snapshot == INVALID_HANDLE_VALUE) {
		return false;
	}

	if (Process32First(snapshot, &entry)) {
		do {
			if (_wcsicmp(entry.szExeFile, L"MirrorsEdge.exe") == 0) {
				tpid = entry.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &entry));
	}

	CloseHandle(snapshot);

	if (!tpid) {
		if (process) {
			CloseHandle(process);
			process = NULL;
		}
		return false;
	}

	if (process == NULL || GetProcessId(process) != tpid) {
		if (process) {
			CloseHandle(process);
		}
		process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, tpid);
	}

	return process != NULL;
}

DWORD WINAPI MonitorGameStatus(LPVOID lpParam) {
	HWND hWnd = (HWND)lpParam;
	bool lastStatus = false;
	bool currentStatus;

	while (true) {
		currentStatus = IsMirrorsEdgeRunning();
		if (currentStatus != lastStatus) {
			lastStatus = currentStatus;
			g_gameRunning = currentStatus;
			InvalidateRect(hWnd, NULL, TRUE);
		}
		Sleep(1000);
	}

	return 0;
}

void StartScan()
{
	static DWORD pid = 0;

	PROCESSENTRY32 entry = GetProcessInfoByName(L"MirrorsEdge.exe");
	if (entry.th32ProcessID) {
		if (pid != entry.th32ProcessID) {
			HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, 0, entry.th32ProcessID);
			if (process) {
				DWORD c1 = FindPattern(process, "\xE8\x00\x00\x00\x00\x83\xC4\x04\x39\x00\x00\x00\x00\x00\x74\x05", "x????xxxx?????xx"); // E8 ?? ?? ?? ?? 83 C4 04 39 ?? ?? ?? ?? ?? 74 05
				if (c1) {
					DWORD c2 = FindPattern(process, "\xE8\x00\x00\x00\x00\x83\xC4\x04\x39\x00\x00\x00\x00\x00\x74\x1E", "x????xxxx?????xx"); // E8 ?? ?? ?? ?? 83 C4 04 39 ?? ?? ?? ?? ?? 74 1E
					if (c2) {
						DWORD c3 = FindPattern(process, "\x68\xFE\x7F\x00\x00\x8D\x44\x24\x06", "xxxxxxxxx"); // 68 FE 7F 00 00 8D 44 24 06
						if (c3) {
							WriteProcessMemory(process, (void *)c1, "\x90\x90\x90\x90\x90", 5, 0);
							WriteProcessMemory(process, (void *)c2, "\x90\x90\x90\x90\x90", 5, 0);
							WriteProcessMemory(process, (void *)c3, "\x81\xC4\x04\x80\x00\x00\xC3", 7, 0);
						}
					}
				}
				system("cls");
				std::cout << "[+] Mirror's Edge is now running!\n[+] Closing in 5 Seconds";
				Sleep(5000);
				exit(0);
				CloseHandle(process);
			}
		}
	}
	else {
		pid = 0;
	}
}

int main()
{
	CreateThread(NULL, 0, MonitorGameStatus, 0, 0, NULL);
	bool gameRunning = IsMirrorsEdgeRunning();
	system("color b");
	if (g_gameRunning = true)
	{
		SetTimer(0, 0, 500, (TIMERPROC)StartScan);
	}
	std::cout << "[-] Mirror's Edge is not running!\n[+] Made by github.com/kirowater/METDGame";

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

    return 0;
}

