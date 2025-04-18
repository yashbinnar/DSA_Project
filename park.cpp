#include <windows.h>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include "parking_backend.h"

#define ID_SLOT_START 1000
#define ID_GATE_START 2000

const int SLOT_SIZE = 40;
const int GAP = 5;
HWND hwndSlots[ROWS][COLS];
HWND hwndGates[4];
HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void UpdateSlotDisplay(HWND, int, int);
void HighlightTemp(HWND hBtn, COLORREF color, int ms);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ParkingSystem";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowA("ParkingSystem", "Smart Parking System",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 700,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxA(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hInst = hInstance;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

void CreateSlots(HWND hwnd) {
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            int id = ID_SLOT_START + i * COLS + j;
            hwndSlots[i][j] = CreateWindowA("BUTTON", "",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10 + j * (SLOT_SIZE + GAP),
                10 + i * (SLOT_SIZE + GAP),
                SLOT_SIZE, SLOT_SIZE,
                hwnd, (HMENU)(INT_PTR)id, hInst, nullptr);
            UpdateSlotDisplay(hwndSlots[i][j], i, j);
        }
    }
}

void CreateGates(HWND hwnd) {
    int gatePositions[4][2] = {
        {0, 0},
        {0, COLS - 1},
        {ROWS - 1, 0},
        {ROWS - 1, COLS - 1}
    };

    for (int i = 0; i < 4; ++i) {
        int id = ID_GATE_START + i;
        int r = gatePositions[i][0];
        int c = gatePositions[i][1];
        hwndGates[i] = CreateWindowA("BUTTON", ("Gate " + std::to_string(i + 1)).c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10 + c * (SLOT_SIZE + GAP),
            10 + r * (SLOT_SIZE + GAP),
            SLOT_SIZE, SLOT_SIZE,
            hwnd, (HMENU)(INT_PTR)id, hInst, nullptr);
    }
}

std::string InputBox(const std::string& prompt) {
    std::string result;
    HWND hwndDialog = CreateWindowExA(WS_EX_DLGMODALFRAME, "STATIC", prompt.c_str(),
        WS_VISIBLE | WS_POPUP | WS_CAPTION | WS_SYSMENU,
        300, 300, 250, 120, NULL, NULL, hInst, NULL);

    HWND hwndEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        20, 30, 200, 20, hwndDialog, NULL, hInst, NULL);

    HWND hwndOK = CreateWindowA("BUTTON", "OK", WS_CHILD | WS_VISIBLE,
        50, 60, 60, 25, hwndDialog, (HMENU)1, hInst, NULL);

    HWND hwndCancel = CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE,
        130, 60, 60, 25, hwndDialog, (HMENU)2, hInst, NULL);

    ShowWindow(hwndDialog, SW_SHOW);
    UpdateWindow(hwndDialog);

    MSG msg;
    bool done = false;
    while (!done && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_COMMAND) {
            if (LOWORD(msg.wParam) == 1) { // OK clicked
                char buffer[256];
                GetWindowTextA(hwndEdit, buffer, 256);
                result = buffer;
                done = true;
            } else if (LOWORD(msg.wParam) == 2) { // Cancel clicked
                done = true;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(hwndDialog);
    return result;
}

void UpdateSlotDisplay(HWND hBtn, int r, int c) {
    const char* label = occupied[r][c] ? slotVehicleMap[{r, c}].c_str() : "";
    SetWindowTextA(hBtn, label);
    InvalidateRect(hBtn, NULL, TRUE);
}

void HighlightTemp(HWND hBtn, COLORREF color, int ms) {
    HDC hdc = GetDC(hBtn);
    HBRUSH brush = CreateSolidBrush(color);
    RECT rect;
    GetClientRect(hBtn, &rect);
    FillRect(hdc, &rect, brush);
    ReleaseDC(hBtn, hdc);
    DeleteObject(brush);

    Sleep(ms);

    InvalidateRect(hBtn, NULL, TRUE);
}

void HandleSlotClick(int id, HWND hwnd) {
    int index = id - ID_SLOT_START;
    int r = index / COLS, c = index % COLS;

    if (!occupied[r][c]) {
        return; // Do nothing on empty slot
    }

    std::string plate = slotVehicleMap[{r, c}];
    int gateIndex = findNearestGate(r, c);

    occupied[r][c] = false;
    slotVehicleMap.erase({r, c});
    UpdateSlotDisplay(hwndSlots[r][c], r, c);
    HighlightTemp(hwndGates[gateIndex], RGB(0, 255, 255), 5000);

    logToFile("Exited", plate, r * COLS + c + 1, gateIndex + 1);
}

void HandleGateClick(int gateIndex, HWND hwnd) {
    std::string carID = InputBox("Enter Car ID to Park:");
    if (carID.empty()) {
        MessageBoxA(hwnd, "Car ID cannot be empty!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    auto nearestSlot = findNearestEmptySlot(gates[gateIndex]);
    if (nearestSlot.first == -1) {
        MessageBoxA(hwnd, "No empty parking slots available!", "Parking Full", MB_OK | MB_ICONWARNING);
        return;
    }

    int r = nearestSlot.first;
    int c = nearestSlot.second;

    occupied[r][c] = true;
    slotVehicleMap[{r, c}] = carID;
    UpdateSlotDisplay(hwndSlots[r][c], r, c);
    HighlightTemp(hwndSlots[r][c], RGB(0, 255, 0), 500);

    int slotNumber = r * COLS + c + 1;
    logToFile("Entered", carID, slotNumber, gateIndex + 1);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            initRandomOccupancy();
            CreateSlots(hwnd);
            CreateGates(hwnd);
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) >= ID_SLOT_START && LOWORD(wParam) < ID_SLOT_START + ROWS * COLS) {
                HandleSlotClick(LOWORD(wParam), hwnd);
            }
            else if (LOWORD(wParam) >= ID_GATE_START && LOWORD(wParam) < ID_GATE_START + 4) {
                HandleGateClick(LOWORD(wParam) - ID_GATE_START, hwnd);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
