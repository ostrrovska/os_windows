// ============================================================
//  NovaSys Control Panel — Lab 3: Windows Controls & Menus
//  Predmet: Osnovy systemnoho prohramuvannia v OS Windows
//  Demonstruye: buttons, scrollbars, edit controls, listbox,
//               toolbar, tooltip, progressbar, trackbar,
//               statusbar, menu, common controls
// ============================================================
#define UNICODE
#define _UNICODE
#define _WIN32_IE 0x0600
#define _WIN32_WINNT 0x0600
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <algorithm>

// ── IDs ───────────────────────────────────────────────────
#define IDM_FILE_NEW        1001
#define IDM_FILE_OPEN       1002
#define IDM_FILE_SAVE       1003
#define IDM_FILE_EXIT       1004
#define IDM_TOOLS_THEME     1010
#define IDM_TOOLS_CLEAR     1011
#define IDM_TOOLS_ABOUT     1012
#define IDM_VIEW_TOOLBAR    1020
#define IDM_VIEW_STATUS     1021

#define IDT_NEW             2001
#define IDT_OPEN            2002
#define IDT_SAVE            2003
#define IDT_CLEAR           2004
#define IDT_ABOUT           2005

#define IDC_EDIT_INPUT      3001
#define IDC_EDIT_LOG        3002
#define IDC_BTN_SEND        3003
#define IDC_BTN_CLEAR_LOG   3004
#define IDC_BTN_LAUNCH      3005
#define IDC_LISTBOX         3006
#define IDC_SCROLLBAR_H     3007
#define IDC_TRACKBAR        3008
#define IDC_PROGRESS        3009
#define IDC_PROGRESS_TIMER  3010
#define IDC_STATUSBAR       3011
#define IDC_TOOLBAR         3012
#define IDC_TOOLTIP         3013
#define IDC_BTN_BOOST       3014
#define IDC_BTN_RESET       3015
#define IDC_LABEL_POWER     3016

#define TIMER_CLOCK         4001
#define TIMER_PROGRESS      4002
#define TIMER_ANIMATE       4003

// ── Globals ───────────────────────────────────────────────
HINSTANCE g_hInst;
HWND g_hMain, g_hToolbar, g_hStatus, g_hTooltip;
HWND g_hEditInput, g_hEditLog, g_hListBox;
HWND g_hScrollH, g_hTrackbar, g_hProgress;
HWND g_hBtnSend, g_hBtnClear, g_hBtnLaunch, g_hBtnBoost, g_hBtnReset;
HWND g_hLabelPower;

HFONT g_hFontMain  = NULL;
HFONT g_hFontMono  = NULL;
HFONT g_hFontTitle = NULL;
HFONT g_hFontSmall = NULL;

HBRUSH g_hBrushBg    = NULL;
HBRUSH g_hBrushPanel = NULL;
HBRUSH g_hBrushAccent= NULL;

bool g_bDarkTheme  = true;
bool g_bShowToolbar= true;
bool g_bShowStatus = true;
int  g_iProgress   = 0;
int  g_iPower      = 50;
int  g_iScrollVal  = 0;
int  g_iLogLines   = 0;
int  g_iAnimFrame  = 0;
bool g_bBoostActive= false;

struct ThemeColors {
    COLORREF bg, panel, accent, text, textDim,
             btnNorm, btnHover, success, warning, border;
} g_theme;

void ApplyTheme(bool dark) {
    if (dark) {
        g_theme.bg      = RGB(10,  12,  20);
        g_theme.panel   = RGB(18,  22,  38);
        g_theme.accent  = RGB(0,  200, 255);
        g_theme.text    = RGB(220, 235, 255);
        g_theme.textDim = RGB(100, 120, 160);
        g_theme.btnNorm = RGB(25,  40,  80);
        g_theme.btnHover= RGB(0,  100, 180);
        g_theme.success = RGB(0,  220, 130);
        g_theme.warning = RGB(255, 180,  0);
        g_theme.border  = RGB(0,  100, 180);
    } else {
        g_theme.bg      = RGB(235, 238, 245);
        g_theme.panel   = RGB(255, 255, 255);
        g_theme.accent  = RGB(0,   100, 200);
        g_theme.text    = RGB(20,   30,  60);
        g_theme.textDim = RGB(100, 120, 160);
        g_theme.btnNorm = RGB(210, 220, 240);
        g_theme.btnHover= RGB(0,   120, 215);
        g_theme.success = RGB(0,   160, 100);
        g_theme.warning = RGB(200, 130,   0);
        g_theme.border  = RGB(160, 180, 220);
    }
    if (g_hBrushBg)    DeleteObject(g_hBrushBg);
    if (g_hBrushPanel) DeleteObject(g_hBrushPanel);
    if (g_hBrushAccent)DeleteObject(g_hBrushAccent);
    g_hBrushBg    = CreateSolidBrush(g_theme.bg);
    g_hBrushPanel = CreateSolidBrush(g_theme.panel);
    g_hBrushAccent= CreateSolidBrush(g_theme.accent);
}

void AppendLog(const wchar_t* msg) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    wchar_t buf[512];
    // Виправлено: %s змінено на %ls для широких рядків
    swprintf(buf, 512, L"[%02d:%02d:%02d] %ls\r\n",
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, msg);

    int len = GetWindowTextLength(g_hEditLog);
    SendMessage(g_hEditLog, EM_SETSEL, len, len);
    SendMessage(g_hEditLog, EM_REPLACESEL, FALSE, (LPARAM)buf);
    g_iLogLines++;

    wchar_t status[64];
    swprintf(status, 64, L"  Log entries: %d", g_iLogLines);
    SendMessage(g_hStatus, SB_SETTEXT, 2, (LPARAM)status);
}

void AddTooltip(HWND hCtrl, const wchar_t* tip) {
    TOOLINFO ti = {0};
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd     = g_hMain;
    ti.uId      = (UINT_PTR)hCtrl;
    ti.lpszText = (LPWSTR)tip;
    SendMessage(g_hTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
}

void DrawRoundRect(HDC hdc, RECT r, int rx, COLORREF fill, COLORREF border) {
    HBRUSH hbr = CreateSolidBrush(fill);
    HPEN   hpn = CreatePen(PS_SOLID, 1, border);
    SelectObject(hdc, hbr);
    SelectObject(hdc, hpn);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rx, rx);
    DeleteObject(hbr);
    DeleteObject(hpn);
}

void DrawPanel(HDC hdc, RECT r, const wchar_t* title) {
    DrawRoundRect(hdc, r, 10, g_theme.panel, g_theme.border);
    HPEN hpAccent = CreatePen(PS_SOLID, 2, g_theme.accent);
    SelectObject(hdc, hpAccent);
    MoveToEx(hdc, r.left+10, r.top+1, NULL);
    LineTo(hdc,   r.left+60, r.top+1);
    DeleteObject(hpAccent);
    SetTextColor(hdc, g_theme.accent);
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, g_hFontSmall);
    RECT tr = {r.left+12, r.top+6, r.right-8, r.top+24};
    DrawText(hdc, title, -1, &tr, DT_LEFT | DT_SINGLELINE);
}

void CreateControls(HWND hwnd) {
    g_hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwnd, NULL, g_hInst, NULL);
    SendMessage(g_hTooltip, TTM_SETMAXTIPWIDTH, 0, 300);

    TBBUTTON tbb[] = {
        {0, IDT_NEW,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"New"},
        {1, IDT_OPEN,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Open"},
        {2, IDT_SAVE,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Save"},
        {0, 0,         TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
        {3, IDT_CLEAR, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"Clear"},
        {4, IDT_ABOUT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"About"},
    };
    g_hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_TOP,
        0, 0, 0, 0, hwnd, (HMENU)IDC_TOOLBAR, g_hInst, NULL);
    SendMessage(g_hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(g_hToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(16,16));
    HIMAGELIST hImgList = ImageList_Create(16, 16, ILC_COLOR32|ILC_MASK, 5, 0);
    for (int i = 0; i < 5; i++) {
        HBITMAP hbm = CreateBitmap(16,16,1,32,NULL);
        HDC hdcTmp  = CreateCompatibleDC(NULL);
        HBITMAP hOld= (HBITMAP)SelectObject(hdcTmp, hbm);
        COLORREF cols[] = {RGB(0,200,255),RGB(0,255,160),RGB(255,220,0),
                           RGB(255,80,80),RGB(180,100,255)};
        HBRUSH hbr = CreateSolidBrush(cols[i]);
        RECT rr={0,0,16,16}; FillRect(hdcTmp,&rr,hbr);
        DeleteObject(hbr);
        SelectObject(hdcTmp, hOld);
        DeleteDC(hdcTmp);
        ImageList_Add(hImgList, hbm, NULL);
        DeleteObject(hbm);
    }
    SendMessage(g_hToolbar, TB_SETIMAGELIST, 0, (LPARAM)hImgList);
    SendMessage(g_hToolbar, TB_ADDBUTTONS, 6, (LPARAM)tbb);
    SendMessage(g_hToolbar, TB_AUTOSIZE, 0, 0);

    g_hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, hwnd, (HMENU)IDC_STATUSBAR, g_hInst, NULL);
    int sParts[] = {220, 420, -1};
    SendMessage(g_hStatus, SB_SETPARTS, 3, (LPARAM)sParts);
    SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)L"  NovaSys v1.0 - Ready");
    SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)L"  System: ONLINE");
    SendMessage(g_hStatus, SB_SETTEXT, 2, (LPARAM)L"  Log entries: 0");

    g_hEditInput = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        0, 0, 0, 0, hwnd, (HMENU)IDC_EDIT_INPUT, g_hInst, NULL);
    SendMessage(g_hEditInput, EM_SETLIMITTEXT, 256, 0);
    SendMessage(g_hEditInput, EM_SETCUEBANNER, TRUE, (LPARAM)L"Enter command or message...");

    g_hBtnSend = CreateWindowEx(0, L"BUTTON", L"> SEND",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        0,0,0,0, hwnd,(HMENU)IDC_BTN_SEND, g_hInst, NULL);
    g_hBtnClear = CreateWindowEx(0, L"BUTTON", L"X CLEAR LOG",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        0,0,0,0, hwnd,(HMENU)IDC_BTN_CLEAR_LOG, g_hInst, NULL);
    g_hBtnLaunch = CreateWindowEx(0, L"BUTTON", L"! LAUNCH SEQUENCE",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        0,0,0,0, hwnd,(HMENU)IDC_BTN_LAUNCH, g_hInst, NULL);
    g_hBtnBoost = CreateWindowEx(0, L"BUTTON", L"^ BOOST",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        0,0,0,0, hwnd,(HMENU)IDC_BTN_BOOST, g_hInst, NULL);
    g_hBtnReset = CreateWindowEx(0, L"BUTTON", L"R RESET",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        0,0,0,0, hwnd,(HMENU)IDC_BTN_RESET, g_hInst, NULL);

    g_hEditLog = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        0,0,0,0, hwnd,(HMENU)IDC_EDIT_LOG, g_hInst, NULL);

    g_hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        0,0,0,0, hwnd,(HMENU)IDC_LISTBOX, g_hInst, NULL);

    const wchar_t* items[] = {
        L"[SYS] Core System Monitor",
        L"[SEC] Security Subsystem",
        L"[NET] Network Interface",
        L"[HDD] Storage Controller",
        L"[PWR] Power Management",
        L"[TMP] Thermal Regulator",
        L"[DAT] Analytics Engine",
        L"[DIA] Diagnostic Module",
        L"[LNC] Launch Controller",
        L"[CFG] Config Manager",
    };
    for (int i = 0; i < 10; i++)
        SendMessage(g_hListBox, LB_ADDSTRING, 0, (LPARAM)items[i]);
    SendMessage(g_hListBox, LB_SETCURSEL, 0, 0);

    g_hScrollH = CreateWindowEx(0, L"SCROLLBAR", NULL,
        WS_CHILD | WS_VISIBLE | SBS_HORZ,
        0,0,0,0, hwnd,(HMENU)IDC_SCROLLBAR_H, g_hInst, NULL);
    SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL, 0, 100, 10, 50};
    SetScrollInfo(g_hScrollH, SB_CTL, &si, TRUE);

    g_hTrackbar = CreateWindowEx(0, TRACKBAR_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | TBS_TOOLTIPS,
        0,0,0,0, hwnd,(HMENU)IDC_TRACKBAR, g_hInst, NULL);
    SendMessage(g_hTrackbar, TBM_SETRANGE,  TRUE, MAKELPARAM(0, 100));
    SendMessage(g_hTrackbar, TBM_SETTICFREQ,10, 0);
    SendMessage(g_hTrackbar, TBM_SETPOS,    TRUE, 50);
    SendMessage(g_hTrackbar, TBM_SETPAGESIZE, 0, 10);

    g_hProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH | PBS_MARQUEE,
        0,0,0,0, hwnd,(HMENU)IDC_PROGRESS, g_hInst, NULL);
    SendMessage(g_hProgress, PBM_SETRANGE32, 0, 100);
    SendMessage(g_hProgress, PBM_SETPOS, 0, 0);

    g_hLabelPower = CreateWindowEx(0, L"STATIC", L"POWER: 50%",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0,0,0,0, hwnd,(HMENU)IDC_LABEL_POWER, g_hInst, NULL);

    AddTooltip(g_hBtnSend,   L"Send command to the system log");
    AddTooltip(g_hBtnClear,  L"Clear all log entries");
    AddTooltip(g_hBtnLaunch, L"Launch automated progress sequence");
    AddTooltip(g_hBtnBoost,  L"Boost power level to maximum");
    AddTooltip(g_hBtnReset,  L"Reset power level to default (50%)");
    AddTooltip(g_hTrackbar,  L"Adjust system power level (0-100%)");
    AddTooltip(g_hScrollH,   L"Horizontal scroll adjusts scan offset");
    AddTooltip(g_hListBox,   L"Select a subsystem module");

    SetTimer(hwnd, TIMER_CLOCK,   1000, NULL);
    SetTimer(hwnd, TIMER_ANIMATE,  80,  NULL);
}

void CreateAppMenu(HWND hwnd) {
    HMENU hMenu  = CreateMenu();
    HMENU hFile  = CreatePopupMenu();
    HMENU hView  = CreatePopupMenu();
    HMENU hTools = CreatePopupMenu();

    AppendMenu(hFile,  MF_STRING, IDM_FILE_NEW,    L"&New Session\tCtrl+N");
    AppendMenu(hFile,  MF_STRING, IDM_FILE_OPEN,   L"&Open Log...\tCtrl+O");
    AppendMenu(hFile,  MF_STRING, IDM_FILE_SAVE,   L"&Save Log...\tCtrl+S");
    AppendMenu(hFile,  MF_SEPARATOR, 0, NULL);
    AppendMenu(hFile,  MF_STRING, IDM_FILE_EXIT,   L"E&xit\tAlt+F4");

    AppendMenu(hView,  MF_STRING|MF_CHECKED, IDM_VIEW_TOOLBAR, L"&Toolbar");
    AppendMenu(hView,  MF_STRING|MF_CHECKED, IDM_VIEW_STATUS,  L"&Status Bar");

    AppendMenu(hTools, MF_STRING, IDM_TOOLS_THEME, L"Toggle &Theme\tCtrl+T");
    AppendMenu(hTools, MF_STRING, IDM_TOOLS_CLEAR, L"&Clear All\tCtrl+L");
    AppendMenu(hTools, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTools, MF_STRING, IDM_TOOLS_ABOUT, L"&About NovaSys...");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFile,  L"&File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hView,  L"&View");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hTools, L"&Tools");

    SetMenu(hwnd, hMenu);
}

void LayoutControls(HWND hwnd) {
    RECT rc; GetClientRect(hwnd, &rc);
    int W = rc.right, H = rc.bottom;
    int tbH = 0, sbH = 0;
    if (g_bShowToolbar && g_hToolbar) {
        RECT tr; GetWindowRect(g_hToolbar, &tr);
        tbH = tr.bottom - tr.top;
    }
    if (g_bShowStatus && g_hStatus) {
        RECT sr; GetWindowRect(g_hStatus, &sr);
        sbH = sr.bottom - sr.top;
    }

    int top  = tbH + 8;
    int bot  = H - sbH - 8;
    int avail = bot - top;
    int lpW = 220;
    int lpX = 8;
    int rpX = lpX + lpW + 10;
    int rpW = W - rpX - 8;

    MoveWindow(g_hListBox, lpX, top+28, lpW, avail-100, TRUE);
    MoveWindow(g_hScrollH, lpX, top+28+avail-100+4, lpW, 20, TRUE);
    MoveWindow(g_hTrackbar, lpX, top+28+avail-60, lpW, 30, TRUE);
    MoveWindow(g_hLabelPower, lpX, top+28+avail-28, lpW, 22, TRUE);

    int inputH = 32;
    int btnW   = 90;
    MoveWindow(g_hEditInput, rpX, top+28, rpW-btnW*2-8, inputH, TRUE);
    MoveWindow(g_hBtnSend,   rpX+rpW-btnW*2-4, top+28, btnW, inputH, TRUE);
    MoveWindow(g_hBtnClear,  rpX+rpW-btnW,     top+28, btnW, inputH, TRUE);

    int logH = avail - inputH - 80 - 10;
    MoveWindow(g_hEditLog,  rpX, top+28+inputH+8, rpW, logH, TRUE);

    int progY = top+28+inputH+8+logH+8;
    MoveWindow(g_hProgress, rpX, progY, rpW-200, 24, TRUE);
    MoveWindow(g_hBtnLaunch,rpX+rpW-200+4, progY, 196, 24, TRUE);

    int btn2Y = progY+30;
    MoveWindow(g_hBtnBoost, rpX,        btn2Y, 90, 28, TRUE);
    MoveWindow(g_hBtnReset, rpX+94,     btn2Y, 90, 28, TRUE);
}

void OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc; GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, g_hBrushBg);

    RECT tbRect = {0};
    if (g_bShowToolbar && g_hToolbar) {
        GetWindowRect(g_hToolbar, &tbRect);
        tbRect.bottom -= tbRect.top; tbRect.top = 0;
    }
    int tbH = (int)tbRect.bottom;

    int sbH = 0;
    if (g_bShowStatus && g_hStatus) {
        RECT sr; GetWindowRect(g_hStatus, &sr);
        sbH = sr.bottom - sr.top;
    }
    int top  = tbH + 8;
    int bot  = rc.bottom - sbH - 8;
    int avail = bot - top;
    int lpW  = 220, lpX = 8;
    int rpX  = lpX + lpW + 10;
    int rpW  = rc.right - rpX - 8;
    int inputH = 32;
    int logH = avail - inputH - 80 - 10;

    RECT panelL = {lpX, top, lpX+lpW, bot};
    DrawPanel(hdc, panelL, L"SUBSYSTEMS");

    RECT panelR = {rpX, top, rpX+rpW, bot};
    DrawPanel(hdc, panelR, L"COMMAND TERMINAL");

    int progY = top+28+inputH+8+logH+8;
    SetTextColor(hdc, g_theme.accent);
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, g_hFontSmall);
    RECT lbr = {rpX, progY-16, rpX+rpW, progY};
    DrawText(hdc, L"SYSTEM LOAD", -1, &lbr, DT_LEFT|DT_SINGLELINE|DT_VCENTER);

    int dotX = rc.right - 80;
    int dotY = top + 14;
    for (int i = 0; i < 5; i++) {
        int phase = (g_iAnimFrame + i*3) % 15;
        COLORREF dc = (phase < 8) ? g_theme.accent : g_theme.panel;
        HBRUSH hbr = CreateSolidBrush(dc);
        RECT dr = {dotX+i*14, dotY, dotX+i*14+8, dotY+8};
        FillRect(hdc, &dr, hbr);
        DeleteObject(hbr);
    }

    HPEN hScan = CreatePen(PS_SOLID, 1, RGB(
        GetRValue(g_theme.bg)+5,
        GetGValue(g_theme.bg)+5,
        GetBValue(g_theme.bg)+8));
    SelectObject(hdc, hScan);
    for (int y = top; y < bot; y += 4) {
        MoveToEx(hdc, lpX, y, NULL);
        LineTo(hdc, rc.right-8, y);
    }
    DeleteObject(hScan);

    EndPaint(hwnd, &ps);
}

void StyleControls() {
    HWND ctrls[] = {g_hEditInput, g_hEditLog, g_hListBox,
                    g_hBtnSend, g_hBtnClear, g_hBtnLaunch,
                    g_hBtnBoost, g_hBtnReset, g_hLabelPower, NULL};
    for (int i = 0; ctrls[i]; i++)
        SendMessage(ctrls[i], WM_SETFONT, (WPARAM)g_hFontMono, TRUE);

    SendMessage(g_hStatus,  WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);
    SendMessage(g_hToolbar, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);
}

void HandleSend() {
    wchar_t buf[260];
    GetWindowText(g_hEditInput, buf, 260);
    if (buf[0] == 0) {
        AppendLog(L"[WARN] Empty command - please enter something.");
        return;
    }
    wchar_t logMsg[320];
    // Виправлено: %s змінено на %ls
    swprintf(logMsg, 320, L"[CMD] %ls", buf);
    AppendLog(logMsg);
    SetWindowText(g_hEditInput, L"");

    int sel = (int)SendMessage(g_hListBox, LB_GETCURSEL, 0, 0);
    wchar_t item[128] = L"(none)";
    if (sel != LB_ERR) SendMessage(g_hListBox, LB_GETTEXT, sel, (LPARAM)item);

    // Виправлено: %s змінено на %ls
    swprintf(logMsg, 320, L"[OK] Dispatched to: %ls", item);
    AppendLog(logMsg);

    SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)L"  Last cmd: sent");
    MessageBeep(MB_OK);
}

void HandleLaunch() {
    AppendLog(L"[INFO] Launch sequence initiated...");
    g_iProgress = 0;
    SendMessage(g_hProgress, PBM_SETPOS, 0, 0);
    LONG style = GetWindowLong(g_hProgress, GWL_STYLE);
    style &= ~PBS_MARQUEE;
    SetWindowLong(g_hProgress, GWL_STYLE, style);
    SetTimer(g_hMain, TIMER_PROGRESS, 40, NULL);
    EnableWindow(g_hBtnLaunch, FALSE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        CreateAppMenu(hwnd);
        ApplyTheme(g_bDarkTheme);
        g_hFontMain  = CreateFont(16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                          DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
        g_hFontMono  = CreateFont(14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                          FIXED_PITCH|FF_MODERN, L"Consolas");
        g_hFontTitle = CreateFont(22,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                          DEFAULT_PITCH|FF_SWISS, L"Segoe UI Semibold");
        g_hFontSmall = CreateFont(12,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                          DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
        CreateControls(hwnd);
        StyleControls();
        AppendLog(L"[*] NovaSys Control Panel initialized.");
        AppendLog(L"[-] All subsystems nominal.");
        AppendLog(L"[-] Awaiting operator commands...");
        return 0;

    case WM_SIZE:
        if (g_hToolbar) SendMessage(g_hToolbar, TB_AUTOSIZE, 0, 0);
        if (g_hStatus)  SendMessage(g_hStatus,  WM_SIZE, wp, lp);
        LayoutControls(hwnd);
        return 0;

    case WM_PAINT:
        OnPaint(hwnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, g_theme.text);
        SetBkColor(hdc,   g_theme.panel);
        return (LRESULT)g_hBrushPanel;
    }

    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, g_theme.text);
        SetBkColor(hdc,   g_theme.btnNorm);
        return (LRESULT)CreateSolidBrush(g_theme.btnNorm);
    }

    case WM_HSCROLL: {
        if ((HWND)lp == g_hScrollH) {
            SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL};
            GetScrollInfo(g_hScrollH, SB_CTL, &si);
            int prev = si.nPos;
            switch (LOWORD(wp)) {
                case SB_LEFT:       si.nPos = si.nMin; break;
                case SB_RIGHT:      si.nPos = si.nMax; break;
                case SB_LINELEFT:   si.nPos -= 1; break;
                case SB_LINERIGHT:  si.nPos += 1; break;
                case SB_PAGELEFT:   si.nPos -= si.nPage; break;
                case SB_PAGERIGHT:  si.nPos += si.nPage; break;
                case SB_THUMBTRACK: si.nPos  = si.nTrackPos; break;
            }
            si.nPos = std::max(si.nMin, std::min(si.nMax - (int)si.nPage + 1, si.nPos));
            SetScrollInfo(g_hScrollH, SB_CTL, &si, TRUE);
            if (si.nPos != prev) {
                g_iScrollVal = si.nPos;
                wchar_t s[64];
                swprintf(s, 64, L"  Scan offset: %d", g_iScrollVal);
                SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)s);
                wchar_t log[80];
                swprintf(log, 80, L"[ADJ] Scan offset adjusted to %d", g_iScrollVal);
                AppendLog(log);
            }
        }
        return 0;
    }

    case WM_VSCROLL:
        return 0;

    case WM_NOTIFY: {
        NMHDR* nmh = (NMHDR*)lp;
        if (nmh->idFrom == IDC_TRACKBAR) {
            int pos = (int)SendMessage(g_hTrackbar, TBM_GETPOS, 0, 0);
            if (pos != g_iPower) {
                g_iPower = pos;
                wchar_t label[32], log[64];
                swprintf(label, 32, L"POWER: %d%%", g_iPower);
                SetWindowText(g_hLabelPower, label);
                swprintf(log, 64, L"[PWR] Power level: %d%%", g_iPower);
                SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)log);
            }
        }
        if (nmh->code == TTN_GETDISPINFO) {
            NMTTDISPINFO* tt = (NMTTDISPINFO*)lp;
            switch (nmh->idFrom) {
                case IDT_NEW:   tt->lpszText = (LPWSTR)L"New Session"; break;
                case IDT_OPEN:  tt->lpszText = (LPWSTR)L"Open Log File"; break;
                case IDT_SAVE:  tt->lpszText = (LPWSTR)L"Save Log"; break;
                case IDT_CLEAR: tt->lpszText = (LPWSTR)L"Clear Log"; break;
                case IDT_ABOUT: tt->lpszText = (LPWSTR)L"About NovaSys"; break;
            }
        }
        return 0;
    }

    case WM_COMMAND: {
        int id  = LOWORD(wp);
        int evt = HIWORD(wp);

        if (id == IDC_LISTBOX && evt == LBN_SELCHANGE) {
            wchar_t item[128] = {};
            int sel = (int)SendMessage(g_hListBox, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                SendMessage(g_hListBox, LB_GETTEXT, sel, (LPARAM)item);
                wchar_t log[160];
                // Виправлено: %s змінено на %ls
                swprintf(log, 160, L"[SEL] Module selected: %ls", item);
                AppendLog(log);
                SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)item);
            }
            return 0;
        }

        switch (id) {
        case IDC_BTN_SEND:      HandleSend(); break;
        case IDC_BTN_CLEAR_LOG:
            SetWindowText(g_hEditLog, L"");
            g_iLogLines = 0;
            SendMessage(g_hStatus, SB_SETTEXT, 2, (LPARAM)L"  Log entries: 0");
            AppendLog(L"[*] Log cleared.");
            break;
        case IDC_BTN_LAUNCH:    HandleLaunch(); break;
        case IDC_BTN_BOOST:
            SendMessage(g_hTrackbar, TBM_SETPOS, TRUE, 100);
            g_iPower = 100;
            SetWindowText(g_hLabelPower, L"POWER: 100%");
            AppendLog(L"[MAX] BOOST activated - power at 100%!");
            SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)L"  [MAX] BOOST ACTIVE");
            g_bBoostActive = true;
            break;
        case IDC_BTN_RESET:
            SendMessage(g_hTrackbar, TBM_SETPOS, TRUE, 50);
            g_iPower = 50;
            SetWindowText(g_hLabelPower, L"POWER: 50%");
            AppendLog(L"[RST] Power reset to 50%.");
            SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)L"  System: ONLINE");
            g_bBoostActive = false;
            break;

        case IDT_NEW:
            SetWindowText(g_hEditLog, L"");
            g_iLogLines = 0;
            SendMessage(g_hProgress, PBM_SETPOS, 0, 0);
            AppendLog(L"[*] New session started.");
            break;
        case IDT_OPEN:
        case IDM_FILE_OPEN: {
            OPENFILENAME ofn = {};
            wchar_t file[MAX_PATH] = {};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner   = hwnd;
            ofn.lpstrFilter = L"Log Files (*.log)\0*.log\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile   = file;
            ofn.nMaxFile    = MAX_PATH;
            ofn.Flags       = OFN_FILEMUSTEXIST;
            if (GetOpenFileName(&ofn)) {
                wchar_t log[320];
                // Виправлено: %s змінено на %ls
                swprintf(log, 320, L"[FILE] File opened: %ls", file);
                AppendLog(log);
            }
            break;
        }
        case IDT_SAVE:
        case IDM_FILE_SAVE: {
            OPENFILENAME ofn = {};
            wchar_t file[MAX_PATH] = L"novasys_log.log";
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner   = hwnd;
            ofn.lpstrFilter = L"Log Files (*.log)\0*.log\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile   = file;
            ofn.nMaxFile    = MAX_PATH;
            ofn.Flags       = OFN_OVERWRITEPROMPT;
            if (GetSaveFileName(&ofn)) {
                int len = GetWindowTextLength(g_hEditLog);
                wchar_t* buf = (wchar_t*)malloc((len+2)*sizeof(wchar_t));
                if (buf) {
                    GetWindowText(g_hEditLog, buf, len+1);
                    HANDLE hf = CreateFile(file, GENERIC_WRITE, 0, NULL,
                                           CREATE_ALWAYS, 0, NULL);
                    if (hf != INVALID_HANDLE_VALUE) {
                        DWORD written;
                        WriteFile(hf, buf, len*sizeof(wchar_t), &written, NULL);
                        CloseHandle(hf);
                        AppendLog(L"[SAVE] Log saved to file.");
                    }
                    free(buf);
                }
            }
            break;
        }
        case IDT_CLEAR:
        case IDM_TOOLS_CLEAR:
            SendMessage(g_hMain, WM_COMMAND, IDC_BTN_CLEAR_LOG, 0);
            break;
        case IDT_ABOUT:
        case IDM_TOOLS_ABOUT:
            MessageBox(hwnd,
                L"NovaSys Control Panel v1.0\n\n"
                L"Lab 3 - Windows Controls & Menus\n"
                L"Predmet: Osnovy systemnoho prohramuvannia v OS Windows\n\n"
                L"Demonstrates:\n"
                L"  - Buttons (BS_PUSHBUTTON, BS_FLAT)\n"
                L"  - Multiline Edit Control (read-only log)\n"
                L"  - Single-line Edit Control (input)\n"
                L"  - ListBox (LBS_NOTIFY)\n"
                L"  - Horizontal ScrollBar\n"
                L"  - Trackbar / Slider\n"
                L"  - ProgressBar (PBS_SMOOTH)\n"
                L"  - Toolbar (TBSTYLE_FLAT + ImageList)\n"
                L"  - Tooltip (TTS_BALLOON)\n"
                L"  - StatusBar (SB_SETPARTS)\n"
                L"  - Menu with accelerators\n"
                L"  - Open/Save dialogs\n"
                L"  - Custom WM_CTLCOLOR theming",
                L"About NovaSys",
                MB_OK | MB_ICONINFORMATION);
            break;

        case IDM_FILE_NEW:
            SendMessage(hwnd, WM_COMMAND, IDT_NEW, 0);
            break;
        case IDM_FILE_EXIT:
            PostQuitMessage(0);
            break;
        case IDM_TOOLS_THEME:
            g_bDarkTheme = !g_bDarkTheme;
            ApplyTheme(g_bDarkTheme);
            InvalidateRect(hwnd, NULL, TRUE);
            AppendLog(g_bDarkTheme ? L"[THEME] Theme: Dark" : L"[THEME] Theme: Light");
            break;
        case IDM_VIEW_TOOLBAR:
            g_bShowToolbar = !g_bShowToolbar;
            ShowWindow(g_hToolbar, g_bShowToolbar ? SW_SHOW : SW_HIDE);
            {   HMENU hm = GetMenu(hwnd);
                HMENU hv = GetSubMenu(hm, 1);
                CheckMenuItem(hv, IDM_VIEW_TOOLBAR,
                    g_bShowToolbar ? MF_CHECKED : MF_UNCHECKED); }
            SendMessage(g_hToolbar, TB_AUTOSIZE, 0, 0);
            LayoutControls(hwnd);
            break;
        case IDM_VIEW_STATUS:
            g_bShowStatus = !g_bShowStatus;
            ShowWindow(g_hStatus, g_bShowStatus ? SW_SHOW : SW_HIDE);
            {   HMENU hm = GetMenu(hwnd);
                HMENU hv = GetSubMenu(hm, 1);
                CheckMenuItem(hv, IDM_VIEW_STATUS,
                    g_bShowStatus ? MF_CHECKED : MF_UNCHECKED); }
            LayoutControls(hwnd);
            break;
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wp == VK_RETURN) HandleSend();
        return 0;

    case WM_TIMER:
        switch (wp) {
        case TIMER_CLOCK: {
            time_t t = time(NULL);
            struct tm* ti = localtime(&t);
            wchar_t s[64];
            swprintf(s, 64, L"  NovaSys v1.0  |  %02d:%02d:%02d",
                     ti->tm_hour, ti->tm_min, ti->tm_sec);
            SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)s);
            break;
        }
        case TIMER_PROGRESS:
            g_iProgress += 2;
            if (g_iProgress > 100) {
                g_iProgress = 100;
                KillTimer(hwnd, TIMER_PROGRESS);
                EnableWindow(g_hBtnLaunch, TRUE);
                AppendLog(L"[DONE] Launch sequence complete - 100%");
                SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)L"  [OK] Sequence complete");
            }
            SendMessage(g_hProgress, PBM_SETPOS, g_iProgress, 0);
            if (g_iProgress % 20 == 0 && g_iProgress < 100) {
                wchar_t log[64];
                swprintf(log, 64, L"[PROG] Progress: %d%%", g_iProgress);
                AppendLog(log);
            }
            break;
        case TIMER_ANIMATE:
            g_iAnimFrame = (g_iAnimFrame + 1) % 30;
            {   RECT rc2; GetClientRect(hwnd, &rc2);
                rc2.bottom = rc2.top + 120;
                InvalidateRect(hwnd, &rc2, FALSE); }
            break;
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_CLOCK);
        KillTimer(hwnd, TIMER_ANIMATE);
        if (g_hFontMain)  DeleteObject(g_hFontMain);
        if (g_hFontMono)  DeleteObject(g_hFontMono);
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        if (g_hFontSmall) DeleteObject(g_hFontSmall);
        if (g_hBrushBg)   DeleteObject(g_hBrushBg);
        if (g_hBrushPanel)DeleteObject(g_hBrushPanel);
        if (g_hBrushAccent)DeleteObject(g_hBrushAccent);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInst;

    INITCOMMONCONTROLSEX icc = {sizeof(icc),
        ICC_WIN95_CLASSES | ICC_BAR_CLASSES |
        ICC_PROGRESS_CLASS | ICC_COOL_CLASSES | ICC_USEREX_CLASSES};
    InitCommonControlsEx(&icc);

    WNDCLASSEX wc  = {sizeof(WNDCLASSEX)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance   = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = L"NovaSysWnd";
    wc.hCursor     = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon       = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm     = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wc);

    g_hMain = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"NovaSysWnd",
        L"NovaSys Control Panel  |  Lab 3 - Windows Controls",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1100, 720,
        NULL, NULL, hInst, NULL);

    ShowWindow(g_hMain, nCmdShow);
    UpdateWindow(g_hMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hMain, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}