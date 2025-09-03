// IRoseWindows.cpp
// Compilar: Win32 (Windows SDK), Subsystem: Windows

#define OEMRESOURCE
#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <thread>
#include <atomic>
#include <chrono>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

// ---- IDs de controles ----
enum : int {
    ID_LIST_WINDOWS = 1001,
    ID_BTN_REFRESH = 1002,
    ID_BTN_SEND = 1003,
    ID_BTN_SELALL = 1004,
    ID_BTN_CLEAR = 1005,
    ID_BTN_START = 1006,
    ID_BTN_STOP = 1007,
    ID_CHK_FOCUS = 1008,
    ID_STATUS_TXT = 1009,
    ID_CHK_F1 = 1101, // até 1112 (F12)
};

// ---- Globais ----
HWND g_hMain = nullptr;
HWND g_hList = nullptr;
HWND g_hChkFocus = nullptr;
HWND g_hStatus = nullptr;
HWND g_hChkFn[12] = { 0 };
std::vector<HWND> g_hwnds;  // mapeia índice do ListBox -> HWND da janela
HFONT g_hFont = nullptr;

// Thread / controle Start/Stop
std::atomic<bool> g_running(false);
std::thread g_worker;

// Converte wstring (UTF-16 no Windows) para string (UTF-8)
std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();

    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.c_str(), -1,
        nullptr, 0, nullptr, nullptr);

    std::string result(sizeNeeded - 1, 0); // remove o '\0'
    WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.c_str(), -1,
        &result[0], sizeNeeded - 1, nullptr, nullptr);

    return result;
}

// Prepara lParam para WM_KEYDOWN/UP (scancode nos bits 16-23)
static LPARAM MakeKeyLParam(UINT vk, bool keyup) {
    UINT sc = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    LPARAM lParam = 1 | (sc << 16);
    if (keyup) lParam |= (1 << 30) | (1u << 31);
    return lParam;
}

static BOOL IsOurWindow(HWND h) { return h == g_hMain; }

// Enumera janelas topo (visíveis, com título)
static BOOL CALLBACK EnumWindowsProc(HWND h, LPARAM lParam) {
    if (!IsWindowVisible(h) || IsOurWindow(h)) return TRUE;

    wchar_t title[256];
    GetWindowTextW(h, title, 256);
    if (title[0] == L'\0') return TRUE;

    LONG_PTR style = GetWindowLongPtr(h, GWL_STYLE);
    if (!(style & WS_CAPTION)) return TRUE;

    auto* vec = reinterpret_cast<std::vector<std::pair<HWND, std::wstring>>*>(lParam);
    vec->push_back({ h, title });
    return TRUE;
}

static void SetStatus(const std::wstring& msg) {
    SetWindowTextW(g_hStatus, msg.c_str());
}

static void PopulateWindowsList() {
    SendMessage(g_hList, LB_RESETCONTENT, 0, 0);
    g_hwnds.clear();

    std::vector<std::pair<HWND, std::wstring>> items;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&items));

    std::sort(items.begin(), items.end(),
        [](auto& a, auto& b) { return a.second < b.second; });

    for (auto& it : items) {
        int idx = (int)SendMessageW(g_hList, LB_ADDSTRING, 0, (LPARAM)it.second.c_str());
        if (idx >= 0) {
            SendMessage(g_hList, LB_SETITEMDATA, idx, (LPARAM)it.first);
            g_hwnds.push_back(it.first);
        }
    }

    std::wstringstream ss;
    ss << L"Janelas encontradas: " << items.size() << L". Selecione até 10.";
    SetStatus(ss.str());
}

static std::vector<UINT> SelectedFunctionKeys() {
    std::vector<UINT> vks;
    for (int i = 0; i < 12; ++i) {
        if (Button_GetCheck(g_hChkFn[i]) == BST_CHECKED)
            vks.push_back(VK_F1 + i);
    }
    return vks;
}

static std::vector<HWND> SelectedWindowsFromList() {
    int selCount = (int)SendMessage(g_hList, LB_GETSELCOUNT, 0, 0);
    std::vector<int> selIdx(selCount);
    if (selCount > 0) SendMessage(g_hList, LB_GETSELITEMS, selCount, (LPARAM)selIdx.data());

    std::vector<HWND> result;
    for (int idx : selIdx) {
        if (idx >= 0) {
            HWND hItem = (HWND)SendMessage(g_hList, LB_GETITEMDATA, idx, 0);
            if (IsWindow(hItem)) result.push_back(hItem);
        }
    }
    return result;
}

// Envia teclas uma por vez para uma janela
static void SendKeysToWindow(HWND hTarget, const std::vector<UINT>& vks, bool forceFocus) {
    if (!IsWindow(hTarget)) return;

    if (forceFocus) { SetForegroundWindow(hTarget); Sleep(100); }

    for (UINT vk : vks) {
        LPARAM lpDown = MakeKeyLParam(vk, false);
        LPARAM lpUp = MakeKeyLParam(vk, true);
        PostMessage(hTarget, WM_KEYDOWN, vk, lpDown);
        PostMessage(hTarget, WM_KEYUP, vk, lpUp);
        Sleep(60);
    }
}

// Loop contínuo de envio de teclas
void AutoSendKeysLoop(std::vector<HWND> targets, std::vector<UINT> vks, bool forceFocus) {
    // Ordena do menor ao maior
    std::sort(vks.begin(), vks.end());

    while (g_running) {
        for (HWND h : targets) {
            if (!g_running) break;
            SendKeysToWindow(h, vks, forceFocus);
            Sleep(1000); // intervalo entre janelas
        }
        Sleep(5000); // intervalo entre ciclos completos
    }
}

static void OnClickSend() {
    auto vks = SelectedFunctionKeys();
    auto targets = SelectedWindowsFromList();
    if (vks.empty()) { SetStatus(L"Selecione pelo menos uma tecla (F1–F12)."); return; }
    if (targets.empty()) { SetStatus(L"Selecione pelo menos uma janela."); return; }
    if (targets.size() > 10) { SetStatus(L"Selecione no máximo 10 janelas."); return; }

    bool forceFocus = (Button_GetCheck(g_hChkFocus) == BST_CHECKED);
    int okCount = 0;
    for (HWND h : targets) {
        if (IsWindow(h)) { SendKeysToWindow(h, vks, forceFocus); ++okCount; }
        Sleep(80);
    }

    std::wstringstream ss;
    ss << L"Enviado para " << okCount << L" janela(s).";
    SetStatus(ss.str());
}

// Selecionar ou desmarcar todas F1–F12
static void OnSelectAllKeys(bool check) {
    for (int i = 0; i < 12; ++i) Button_SetCheck(g_hChkFn[i], check ? BST_CHECKED : BST_UNCHECKED);
}

// Fonte do sistema
static HFONT CreateUiFont() {
    NONCLIENTMETRICSW ncm{ sizeof(ncm) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    return CreateFontIndirectW(&ncm.lfMessageFont);

 
}

static void ApplyFont(HWND parent) {
    SendMessage(parent, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hList, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hChkFocus, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessage(g_hStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    for (auto& c : g_hChkFn) SendMessage(c, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    HWND hBtn;
    hBtn = GetDlgItem(parent, ID_BTN_REFRESH); if (hBtn) SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    hBtn = GetDlgItem(parent, ID_BTN_SEND);    if (hBtn) SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    hBtn = GetDlgItem(parent, ID_BTN_SELALL);  if (hBtn) SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    hBtn = GetDlgItem(parent, ID_BTN_CLEAR);   if (hBtn) SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    hBtn = GetDlgItem(parent, ID_BTN_START);   if (hBtn) SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    hBtn = GetDlgItem(parent, ID_BTN_STOP);    if (hBtn) SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);
}

// Cria todos os controles
static void CreateUi(HWND hWnd) {
    int margin = 10, w = 500, h = 500;
    g_hList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | LBS_EXTENDEDSEL | LBS_NOTIFY | WS_VSCROLL,
        margin, margin, 320, 280,
        hWnd, (HMENU)ID_LIST_WINDOWS, GetModuleHandle(nullptr), nullptr);

    int chkW = 140, chkH = 22, col = 0, row = 0;
    int baseX = margin + 320 + 10;
    int baseY = margin;
    for (int i = 0; i < 12; ++i) {
        std::wstringstream label; label << L"F" << (i + 1);
        int colX = baseX + (col * 70);
        int rowY = baseY + (row * (chkH + 4));
        g_hChkFn[i] = CreateWindowExW(0, L"BUTTON", label.str().c_str(),
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            colX, rowY, 60, chkH,
            hWnd, (HMENU)(ID_CHK_F1 + i), GetModuleHandle(nullptr), nullptr);
        ++row;
        if (row == 6) { row = 0; ++col; }
    }

    // Botões
    CreateWindowExW(0, L"BUTTON", L"Atualizar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin, margin + 280 + 10, 90, 28, hWnd, (HMENU)ID_BTN_REFRESH, GetModuleHandle(nullptr), nullptr);
    CreateWindowExW(0, L"BUTTON", L"Selecionar todas (teclas)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + 100, margin + 280 + 10, 170, 28, hWnd, (HMENU)ID_BTN_SELALL, GetModuleHandle(nullptr), nullptr);
    CreateWindowExW(0, L"BUTTON", L"Limpar (teclas)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + 280, margin + 280 + 10, 120, 28, hWnd, (HMENU)ID_BTN_CLEAR, GetModuleHandle(nullptr), nullptr);
    CreateWindowExW(0, L"BUTTON", L"Enviar uma vez", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        w - margin - 90, margin + 280 + 10, 90, 28, hWnd, (HMENU)ID_BTN_SEND, GetModuleHandle(nullptr), nullptr);
    CreateWindowExW(0, L"BUTTON", L"Start", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin, margin + 280 + 10 + 40, 90, 28, hWnd, (HMENU)ID_BTN_START, GetModuleHandle(nullptr), nullptr);
    CreateWindowExW(0, L"BUTTON", L"Stop", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + 100, margin + 280 + 10 + 40, 90, 28, hWnd, (HMENU)ID_BTN_STOP, GetModuleHandle(nullptr), nullptr);

    // Checkbox Forçar foco
    g_hChkFocus = CreateWindowExW(0, L"BUTTON", L"Forçar foco ao enviar",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        margin, margin + 280 + 10 + 80, 200, 24,
        hWnd, (HMENU)ID_CHK_FOCUS, GetModuleHandle(nullptr), nullptr);

    // Status
    g_hStatus = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"Pronto.",
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        margin, h - margin - 36, w - margin * 2, 26,
        hWnd, (HMENU)ID_STATUS_TXT, GetModuleHandle(nullptr), nullptr);

    // Rodapé
    CreateWindowExW(0, L"STATIC", L"FerrareziGamers",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 500 - 20, 850, 20,
        hWnd, nullptr, GetModuleHandle(nullptr), nullptr);


    g_hFont = CreateUiFont();
    ApplyFont(hWnd);
    PopulateWindowsList();
}

static void SetClientSize500x500(HWND hWnd) {
    RECT rc{ 0,0,500,500 };
    AdjustWindowRectEx(&rc, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));
    SetWindowPos(hWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateUi(hWnd);
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case ID_BTN_REFRESH: PopulateWindowsList(); return 0;
        case ID_BTN_SEND: OnClickSend(); return 0;
        case ID_BTN_SELALL: OnSelectAllKeys(true); SetStatus(L"Todas as teclas F1–F12 marcadas."); return 0;
        case ID_BTN_CLEAR: OnSelectAllKeys(false); SetStatus(L"Teclas desmarcadas."); return 0;
        case ID_BTN_START: {
            if (!g_running) {
                auto vks = SelectedFunctionKeys();
                auto targets = SelectedWindowsFromList();
                if (vks.empty() || targets.empty()) { SetStatus(L"Selecione janelas e teclas."); break; }
                if (targets.size() > 10) { SetStatus(L"Selecione até 10 janelas."); break; }
                g_running = true;
                g_worker = std::thread(AutoSendKeysLoop, targets, vks,
                    (Button_GetCheck(g_hChkFocus) == BST_CHECKED));
                g_worker.detach();
                SetStatus(L"Envio contínuo iniciado...");
            }
            return 0;
        }
        case ID_BTN_STOP: {
            g_running = false;
            SetStatus(L"Envio contínuo parado.");
            return 0;
        }
        }
        break;
    }

    case WM_DESTROY:
        g_running = false;
        if (g_hFont) { DeleteObject(g_hFont); g_hFont = nullptr; }
        PostQuitMessage(0);
        return 0;

    case WM_SIZE: return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    const wchar_t* kClass = L"IRoseWindowsClass";
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.hInstance = hInst;
    wc.lpszClassName = kClass;
    wc.lpfnWndProc = WndProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);

    g_hMain = CreateWindowExW(
        0, kClass, L"IRoseWindows",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
        nullptr, nullptr, hInst, nullptr);

    if (!g_hMain) return 0;

    SetClientSize500x500(g_hMain);
    ShowWindow(g_hMain, nCmdShow);
    UpdateWindow(g_hMain);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
