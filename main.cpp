#include <windows.h>
#include <Tlhelp32.h>
#include <vector>
#include <iostream>

#define IDC_CHECKBOX_IMMORTALITY 101
#define PROCESS_NAME L"XR_3DA.exe"
#define MODULE_NAME L"XR_3DA.exe"

const uintptr_t BASE_OFFSET = 0x0010BB88;
const std::vector<uintptr_t> OFFSETS = { 0x0010BB88, 0x18, 0x2C, 0x1C, 0xA10 };

DWORD GetProcessIdByName(const std::wstring& processName);
HANDLE OpenGameProcess();
uintptr_t GetModuleBaseAddress(DWORD processID, const std::wstring& moduleName);
uintptr_t GetPointerAddress(HANDLE hProcess, uintptr_t baseAddress, const std::vector<uintptr_t>& offsets);
bool WriteHealth(HANDLE hProcess, uintptr_t address, float value);


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);

        if (wmId == IDC_CHECKBOX_IMMORTALITY && wmEvent == BN_CLICKED) {
            BOOL checked = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;

            DWORD pid = GetProcessIdByName(PROCESS_NAME);
            HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
            if (!hProcess) {
                MessageBox(hwnd, L"Процесс игры не найден!", L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }

            uintptr_t baseAddress = GetModuleBaseAddress(pid, MODULE_NAME);
            std::wcout << L"Базовый адрес XR_3DA.exe: 0x" << std::hex << baseAddress << std::endl;
            if (baseAddress == 0) {
                MessageBox(hwnd, L"Модуль не найден!", L"Ошибка", MB_OK | MB_ICONERROR);
                CloseHandle(hProcess);
                break;
            }

            uintptr_t healthAddr = GetPointerAddress(hProcess, baseAddress, OFFSETS);
            if (healthAddr == 0) {
                MessageBox(hwnd, L"Не удалось получить адрес здоровья!", L"Ошибка", MB_OK | MB_ICONERROR);
                CloseHandle(hProcess);
                break;
            }

            if (checked) {
                WriteHealth(hProcess, healthAddr, 1.0f);
                MessageBox(hwnd, L"Бессмертие ВКЛ", L"Статус", MB_OK);
            }
            else {
                WriteHealth(hProcess, healthAddr, 1.0f);
                MessageBox(hwnd, L"Бессмертие ВЫКЛ", L"Статус", MB_OK);
            }
            CloseHandle(hProcess);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD GetProcessIdByName(const std::wstring& processName) {
    HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(handle, &pe32)) {
        CloseHandle(handle);
        return 0;
    }

    do {
        if (lstrcmpi(pe32.szExeFile, processName.c_str()) == 0) {
            CloseHandle(handle);
            return pe32.th32ProcessID;
        }
    } while (Process32Next(handle, &pe32));
    CloseHandle(handle);
    return 0;
}

uintptr_t GetModuleBaseAddress(DWORD processID, const std::wstring& moduleName) {
    uintptr_t moduleBaseAddress = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);

    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W moduleEntry;
        moduleEntry.dwSize = sizeof(moduleEntry);

        if (Module32FirstW(snapshot, &moduleEntry)) {
            do {
                if (moduleName == moduleEntry.szModule) {
                    moduleBaseAddress = (uintptr_t)moduleEntry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(snapshot, &moduleEntry));
        }
        CloseHandle(snapshot);
    }
    return moduleBaseAddress;
}

uintptr_t GetPointerAddress(HANDLE hProcess, uintptr_t baseAddress, const std::vector<uintptr_t>& offsets) {
    uintptr_t addr = baseAddress + offsets[0];
    uintptr_t buffer = 0;

    for (size_t i = 1; i < offsets.size(); ++i) {
        if (!ReadProcessMemory(hProcess, (LPCVOID)addr, &buffer, sizeof(buffer), nullptr)) {
            DWORD err = GetLastError();
            std::wcout << L"Ошибка чтения по адресу: 0x" << std::hex << addr << std::endl;
            return 0;
        }
        addr = buffer + offsets[i];
        buffer = 0;
    }

    return addr + 0x4;
}

bool WriteHealth(HANDLE hProcess, uintptr_t address, float value) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(value), &bytesWritten) && bytesWritten == sizeof(value);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    const wchar_t CLASS_NAME[] = L"Класс для окна";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Чит на сталкер тени чернобыля",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr,
        hInstance,
        nullptr
    );

    HWND hwndCheckbox = CreateWindowEx(
        0,
        L"BUTTON",
        L"Бессмертие",
        WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
        20, 20,
        120, 30,
        hwnd,
        (HMENU)IDC_CHECKBOX_IMMORTALITY,
        hInstance,
        nullptr
    );
    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    if (!hwnd) {
        return 0;
    }

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}