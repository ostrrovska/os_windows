// ============================================================
//  CashFlow  ★  Modern Personal Budget Tracker
//  Lab 5 — Dialogue Panels  (Win32 API, no MFC/ATL/.NET)
//
//  All Lab 5 requirements demonstrated:
//  ① Modal dialog     — Add/Edit Transaction  (DialogBoxIndirect)
//  ② Modal dialog     — Budget Goal setup     (DialogBoxIndirect)
//  ③ Modeless dialog  — Live Statistics panel (CreateDialogIndirect)
//  ④ Standard dialog  — ChooseColor           (accent colour picker)
//  ⑤ Standard dialog  — ChooseFont           (transaction list font)
//  ⑥ Standard dialog  — GetOpenFileName      (import CSV)
//  ⑦ Standard dialog  — GetSaveFileName      (export / save CSV)
//  ⑧ Standard dialog  — MessageBox           (delete confirmation)
//  ⑨ In-memory DLGTEMPLATE / DLGITEMTEMPLATE (no .rc file needed)
//
//  Real-use features:
//  • Owner-drawn transaction list (colour-coded cards)
//  • Auto-save to cashflow_data.csv on exit, auto-load on start
//  • Category filter sidebar
//  • Real-time search bar
//  • Budget-goal progress bar
//  • Dark / Light theme toggle  (Ctrl+T)
// ============================================================
#define UNICODE
#define _UNICODE
#define _WIN32_IE    0x0600
#define _WIN32_WINNT 0x0601
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' "\
    "name='Microsoft.Windows.Common-Controls' version='6.0.0.0' "\
    "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <algorithm>

// ── Menu IDs ─────────────────────────────────────────────────
#define IDM_FILE_NEW        1001
#define IDM_FILE_IMPORT     1002
#define IDM_FILE_EXPORT     1003
#define IDM_FILE_EXIT       1004
#define IDM_TXN_ADD         1010
#define IDM_TXN_EDIT        1011
#define IDM_TXN_DELETE      1012
#define IDM_TOOLS_GOAL      1020
#define IDM_TOOLS_COLOR     1021
#define IDM_TOOLS_FONT      1022
#define IDM_TOOLS_ABOUT     1023
#define IDM_VIEW_THEME      1030
#define IDM_VIEW_STATS      1031
#define IDM_VIEW_TOOLBAR    1032
#define IDM_VIEW_STATUS     1033

// ── Toolbar button IDs ────────────────────────────────────────
#define IDT_ADD             2001
#define IDT_EDIT            2002
#define IDT_DELETE          2003
#define IDT_STATS           2004
#define IDT_GOAL            2005
#define IDT_THEME           2006

// ── Control IDs ──────────────────────────────────────────────
#define IDC_TOOLBAR         3001
#define IDC_STATUSBAR       3002
#define IDC_LISTBOX         3003   // main owner-draw transaction list
#define IDC_CAT_LIST        3004   // category filter sidebar
#define IDC_SEARCH          3005   // search edit

// ── Add/Edit Transaction dialog IDs ──────────────────────────
#define IDC_TXN_EXPENSE     4001
#define IDC_TXN_INCOME      4002
#define IDC_TXN_TRANSFER    4003
#define IDC_TXN_AMOUNT      4004
#define IDC_TXN_CAT         4005   // combobox
#define IDC_TXN_DESC        4006
#define IDC_TXN_DATE        4007
#define IDC_TXN_RECURRING   4008

// ── Budget Goal dialog IDs ────────────────────────────────────
#define IDC_GOAL_NAME       4020
#define IDC_GOAL_TARGET     4021

// ── Statistics modeless dialog IDs ───────────────────────────
#define IDC_STAT_INCOME     4030
#define IDC_STAT_EXPENSE    4031
#define IDC_STAT_BALANCE    4032
#define IDC_STAT_COUNT      4033
#define IDC_STAT_AVG        4034
#define IDC_STAT_TOP        4035
#define IDC_STAT_GOAL       4036
#define IDC_STAT_CLOSE      4037

// ── Timer ─────────────────────────────────────────────────────
#define TIMER_CLOCK         5001

// ── Layout constants ──────────────────────────────────────────
#define HEADER_H            116    // header panel height (pixels)
#define SIDEBAR_W           190    // category sidebar width
#define ITEM_H              60     // owner-draw item height
#define SEARCH_H            30     // search bar height

// =============================================================
//  Data structure
// =============================================================
struct Transaction {
    int     id;
    int     day, month, year;
    double  amount;
    wchar_t category[64];
    wchar_t description[200];
    int     type;       // 0 = Expense, 1 = Income, 2 = Transfer
    bool    recurring;
};

// =============================================================
//  Global state
// =============================================================
HINSTANCE g_hInst;
HWND      g_hMain, g_hToolbar, g_hStatus;
HWND      g_hListBox, g_hCatList, g_hSearch;

HFONT  g_hFontUI     = NULL;
HFONT  g_hFontBold   = NULL;
HFONT  g_hFontSmall  = NULL;
HFONT  g_hFontAmount = NULL;   // large amount font for header
HFONT  g_hFontList   = NULL;   // user-selected via ChooseFont

HBRUSH g_hBrushBg      = NULL;
HBRUSH g_hBrushPanel   = NULL;
HBRUSH g_hBrushHeader  = NULL;
HBRUSH g_hBrushAccent  = NULL;
HBRUSH g_hBrushIncome  = NULL;
HBRUSH g_hBrushExpense = NULL;
HBRUSH g_hBrushAlt     = NULL;

// Theme index: 0=Dark(navy), 1=Light(indigo), 2=Pink, 3=Burgundy
int    g_iTheme      = 0;
bool   g_bDarkTheme  = true;   // derived in ApplyTheme; used by painter helpers
bool   g_bShowToolbar= true;
bool   g_bShowStatus = true;

// Transaction storage (up to 2000 entries)
static Transaction g_txns[2000];
static int         g_txnCount = 0;
static int         g_nextId   = 1;

// Filtered indices (indices into g_txns[])
static int g_filtered[2000];
static int g_filteredCount = 0;

// Active category filter: 0 = All, 1..N = specific category index
static int g_filterCat  = 0;

// Savings goal
double  g_dGoalTarget = 0.0;
wchar_t g_sGoalName[128] = L"My Savings Goal";
bool    g_bHasGoal    = false;

// Totals (recalculated when needed)
double g_dTotalIncome  = 0.0;
double g_dTotalExpense = 0.0;
double g_dBalance      = 0.0;

// ChooseColor palette (persisted between calls)
COLORREF g_customColors[16] = {};

// Modeless statistics dialog handle
HWND g_hStatsDlg = NULL;

// Staging vars for Add/Edit dialog
static double   g_dlgAmount    = 0.0;
static wchar_t  g_dlgCategory[64]  = {};
static wchar_t  g_dlgDesc[200]     = {};
static int      g_dlgDay = 1, g_dlgMonth = 1, g_dlgYear = 2024;
static int      g_dlgType = 0;
static bool     g_dlgRecurring = false;
static int      g_editId = -1;  // -1 = new transaction, else = id being edited

// =============================================================
//  Categories
// =============================================================
static const wchar_t* CATEGORIES[] = {
    L"Food & Groceries",
    L"Transport",
    L"Entertainment",
    L"Shopping",
    L"Healthcare",
    L"Housing & Rent",
    L"Education",
    L"Utilities",
    L"Salary",
    L"Savings",
    L"Other",
    NULL
};
static const int CAT_COUNT = 11;

// Category colour accents for sidebar and list items
static const COLORREF CAT_COLORS[] = {
    RGB(249,115, 22),  // Food      — orange
    RGB( 59,130,246),  // Transport — blue
    RGB(168, 85,247),  // Entertainment — purple
    RGB(236, 72,153),  // Shopping  — pink
    RGB( 20,184,166),  // Healthcare— teal
    RGB(234,179,  8),  // Housing   — yellow
    RGB( 14,165,233),  // Education — sky
    RGB(100,116,139),  // Utilities — slate
    RGB( 34,197, 94),  // Salary    — green
    RGB(168,162,158),  // Savings   — stone
    RGB(148,163,184),  // Other     — grey
};

// Returns colour index for a category name, or 10 (Other)
static int CatIndex(const wchar_t* cat) {
    for (int i = 0; i < CAT_COUNT; i++)
        if (_wcsicmp(cat, CATEGORIES[i]) == 0) return i;
    return 10;
}

// Short month names for display
static const wchar_t* MONTH_SHORT[] = {
    L"", L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
    L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
};

// =============================================================
//  Theme
// =============================================================
struct Theme {
    COLORREF bg, panel, panelAlt, header;
    COLORREF accent;
    COLORREF income, expense, transfer;
    COLORREF text, textDim, border, selection;
} g_theme;

void ApplyTheme(int mode) {
    g_iTheme = mode;

    switch (mode) {
    default:
    case 0: // ── Dark Navy / Purple ──────────────────────────────
        g_theme.bg        = RGB( 13,  13,  26);
        g_theme.panel     = RGB( 22,  24,  46);
        g_theme.panelAlt  = RGB( 18,  20,  38);
        g_theme.header    = RGB( 17,  19,  43);
        g_theme.accent    = RGB(108,  99, 255);
        g_theme.income    = RGB( 74, 222, 128);
        g_theme.expense   = RGB(252, 100, 100);
        g_theme.transfer  = RGB( 96, 165, 250);
        g_theme.text      = RGB(226, 232, 240);
        g_theme.textDim   = RGB(148, 163, 184);
        g_theme.border    = RGB( 38,  42,  78);
        g_theme.selection = RGB( 48,  44, 100);
        break;
    case 1: // ── Light Indigo ────────────────────────────────────
        g_theme.bg        = RGB(245, 247, 252);
        g_theme.panel     = RGB(255, 255, 255);
        g_theme.panelAlt  = RGB(239, 242, 249);
        g_theme.header    = RGB( 79,  70, 229);
        g_theme.accent    = RGB( 79,  70, 229);
        g_theme.income    = RGB( 22, 163,  74);
        g_theme.expense   = RGB(220,  38,  38);
        g_theme.transfer  = RGB( 37,  99, 235);
        g_theme.text      = RGB( 15,  23,  42);
        g_theme.textDim   = RGB( 80,  90, 120);
        g_theme.border    = RGB(210, 215, 235);
        g_theme.selection = RGB(225, 222, 255);
        break;
    case 2: // ── Light Pink / Rose ───────────────────────────────
        g_theme.bg        = RGB(255, 240, 248);
        g_theme.panel     = RGB(255, 255, 255);
        g_theme.panelAlt  = RGB(252, 232, 243);
        g_theme.header    = RGB(194,  24,  91);
        g_theme.accent    = RGB(236,  64, 122);
        g_theme.income    = RGB( 27, 120,  60);
        g_theme.expense   = RGB(185,  28,  28);
        g_theme.transfer  = RGB( 30,  96, 200);
        g_theme.text      = RGB( 40,   5,  20);
        g_theme.textDim   = RGB(140,  60, 100);
        g_theme.border    = RGB(250, 185, 215);
        g_theme.selection = RGB(253, 218, 236);
        break;
    case 3: // ── Dark Burgundy ────────────────────────────────────
        g_theme.bg        = RGB( 20,   2,  10);
        g_theme.panel     = RGB( 42,   5,  25);
        g_theme.panelAlt  = RGB( 32,   3,  18);
        g_theme.header    = RGB( 90,  10,  50);
        g_theme.accent    = RGB(240, 100, 160);
        g_theme.income    = RGB( 80, 220, 130);
        g_theme.expense   = RGB(255, 120, 100);
        g_theme.transfer  = RGB(100, 165, 250);
        g_theme.text      = RGB(255, 220, 235);
        g_theme.textDim   = RGB(190, 130, 160);
        g_theme.border    = RGB( 90,  15,  55);
        g_theme.selection = RGB(115,  20,  68);
        break;
    }

    // Derived flag: dark background themes (0=Dark Navy, 3=Burgundy)
    g_bDarkTheme = (mode == 0 || mode == 3);

    // Recreate GDI brushes
    if (g_hBrushBg)      DeleteObject(g_hBrushBg);
    if (g_hBrushPanel)   DeleteObject(g_hBrushPanel);
    if (g_hBrushHeader)  DeleteObject(g_hBrushHeader);
    if (g_hBrushAccent)  DeleteObject(g_hBrushAccent);
    if (g_hBrushIncome)  DeleteObject(g_hBrushIncome);
    if (g_hBrushExpense) DeleteObject(g_hBrushExpense);
    if (g_hBrushAlt)     DeleteObject(g_hBrushAlt);

    g_hBrushBg      = CreateSolidBrush(g_theme.bg);
    g_hBrushPanel   = CreateSolidBrush(g_theme.panel);
    g_hBrushHeader  = CreateSolidBrush(g_theme.header);
    g_hBrushAccent  = CreateSolidBrush(g_theme.accent);
    g_hBrushIncome  = CreateSolidBrush(g_theme.income);
    g_hBrushExpense = CreateSolidBrush(g_theme.expense);
    g_hBrushAlt     = CreateSolidBrush(g_theme.panelAlt);
}

// =============================================================
//  Helper: fill rect with a colour using temp brush
// =============================================================
static void FillColor(HDC hdc, RECT r, COLORREF c) {
    HBRUSH hb = CreateSolidBrush(c);
    FillRect(hdc, &r, hb);
    DeleteObject(hb);
}

// Draw a rounded-corner filled rectangle
static void DrawCard(HDC hdc, RECT r, COLORREF fill, COLORREF border, int rx = 8) {
    HBRUSH hb  = CreateSolidBrush(fill);
    HPEN   hp  = CreatePen(PS_SOLID, 1, border);
    HBRUSH obr = (HBRUSH)SelectObject(hdc, hb);
    HPEN   opn = (HPEN)  SelectObject(hdc, hp);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rx, rx);
    SelectObject(hdc, obr); SelectObject(hdc, opn);
    DeleteObject(hb); DeleteObject(hp);
}

// Draw a single-pixel separator line
static void DrawSeparator(HDC hdc, int x1, int y, int x2, COLORREF c) {
    HPEN hp = CreatePen(PS_SOLID, 1, c);
    HPEN op = (HPEN)SelectObject(hdc, hp);
    MoveToEx(hdc, x1, y, NULL); LineTo(hdc, x2, y);
    SelectObject(hdc, op); DeleteObject(hp);
}

// =============================================================
//  Totals recalculation
// =============================================================
static void RecalcTotals() {
    g_dTotalIncome  = 0.0;
    g_dTotalExpense = 0.0;
    for (int i = 0; i < g_txnCount; i++) {
        if (g_txns[i].type == 1) g_dTotalIncome  += g_txns[i].amount;
        else if (g_txns[i].type == 0) g_dTotalExpense += g_txns[i].amount;
    }
    g_dBalance = g_dTotalIncome - g_dTotalExpense;
}

// =============================================================
//  CSV file I/O  (auto-save / import / export)
// =============================================================
static wchar_t g_autoSavePath[MAX_PATH] = L"cashflow_data.csv";

// Helper: convert wchar_t string to UTF-8 narrow char buffer.
// Returns number of bytes written (excluding null terminator).
static int WtoUTF8(const wchar_t* src, char* dst, int dstBytes) {
    if (!src || dstBytes <= 0) { if (dst && dstBytes>0) dst[0]=0; return 0; }
    int n = WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, dstBytes, NULL, NULL);
    if (n <= 0) { dst[0] = 0; return 0; }
    return n - 1; // exclude null terminator
}

// Helper: convert UTF-8 narrow string to wchar_t buffer.
static void UTF8toW(const char* src, wchar_t* dst, int dstChars) {
    if (!src || dstChars <= 0) { if (dst && dstChars>0) dst[0]=0; return; }
    int n = MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dstChars);
    if (n <= 0 && dstChars > 0) dst[0] = 0;
}

// Save all transactions as UTF-8 CSV (no wcstok/swscanf, no UTF-16).
static void SaveCSV(const wchar_t* path) {
    HANDLE hf = CreateFile(path, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return;

    DWORD wr;
    // UTF-8 BOM — makes the file readable by Excel and other tools
    WriteFile(hf, "\xEF\xBB\xBF", 3, &wr, NULL);
    // Header
    WriteFile(hf, "id,day,month,year,amount,type,recurring,category,description\r\n", 62, &wr, NULL);

    for (int i = 0; i < g_txnCount; i++) {
        char catMB[256] = {}, descMB[512] = {};
        WtoUTF8(g_txns[i].category,    catMB,  255);
        WtoUTF8(g_txns[i].description, descMB, 511);

        char lineBuf[1024];
        int len = _snprintf(lineBuf, 1023,
            "%d,%d,%d,%d,%.2f,%d,%d,\"%s\",\"%s\"\r\n",
            g_txns[i].id, g_txns[i].day, g_txns[i].month, g_txns[i].year,
            g_txns[i].amount, g_txns[i].type, (int)g_txns[i].recurring,
            catMB, descMB);
        if (len > 0)
            WriteFile(hf, lineBuf, (DWORD)len, &wr, NULL);
    }
    CloseHandle(hf);
}

// Manual UTF-8 CSV parser — no wcstok, no swscanf.
// Handles both the new UTF-8 format and the old UTF-16 LE format for migration.
static void LoadCSV(const wchar_t* path) {
    HANDLE hf = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (hf == INVALID_HANDLE_VALUE) return;

    DWORD sz = GetFileSize(hf, NULL);
    if (sz == 0 || sz == INVALID_FILE_SIZE) { CloseHandle(hf); return; }

    BYTE* raw = (BYTE*)malloc(sz + 4);
    if (!raw) { CloseHandle(hf); return; }

    DWORD rd = 0;
    ReadFile(hf, raw, sz, &rd, NULL);
    raw[rd] = raw[rd+1] = raw[rd+2] = raw[rd+3] = 0;
    CloseHandle(hf);

    // Detect encoding and normalise to UTF-8 in a plain char* buffer.
    char* data = NULL;
    bool  freeData = false;

    if (rd >= 2 && raw[0] == 0xFF && raw[1] == 0xFE) {
        // Old format: UTF-16 LE. Convert the whole file to UTF-8 first.
        wchar_t* wbuf = (wchar_t*)(raw + 2);  // skip BOM
        int wchars = (rd - 2) / 2;
        // null-terminate the wide buffer
        wbuf[wchars] = 0;
        int needed = WideCharToMultiByte(CP_UTF8, 0, wbuf, wchars, NULL, 0, NULL, NULL);
        data = (char*)malloc(needed + 4);
        if (!data) { free(raw); return; }
        WideCharToMultiByte(CP_UTF8, 0, wbuf, wchars, data, needed, NULL, NULL);
        data[needed] = 0;
        freeData = true;
    } else {
        // UTF-8 (with or without BOM)
        data = (char*)raw;
        if (rd >= 3 && (BYTE)raw[0]==0xEF && (BYTE)raw[1]==0xBB && (BYTE)raw[2]==0xBF)
            data += 3;   // skip UTF-8 BOM
    }

    g_txnCount = 0;
    g_nextId   = 1;

    // Walk line by line without wcstok (which is non-portable with 2 args).
    char* p     = data;
    bool  first = true;

    while (*p) {
        // Find end of current line
        char* lineStart = p;
        while (*p && *p != '\r' && *p != '\n') p++;
        int lineLen = (int)(p - lineStart);
        // Advance past \r\n
        if (*p == '\r') p++;
        if (*p == '\n') p++;

        if (lineLen == 0) continue;  // blank line
        if (first) { first = false; continue; }  // skip header row
        if (g_txnCount >= 2000) break;

        // ── Parse one CSV line manually ──────────────────────────
        // Format: id,day,month,year,amount,type,recurring,"cat","desc"
        char* q = lineStart;

        // Helper: read an integer field followed by ','
        auto readInt = [&]() -> int {
            int v = 0;
            bool neg = false;
            if (*q == '-') { neg = true; q++; }
            while (*q >= '0' && *q <= '9') v = v*10 + (*q++ - '0');
            if (*q == ',') q++;
            return neg ? -v : v;
        };

        // Helper: read a double field followed by ','
        auto readDbl = [&]() -> double {
            char* end = q;
            double v = strtod(q, &end);
            q = end;
            if (*q == ',') q++;
            return v;
        };

        // Helper: read a quoted string field ("...") and convert to wchar_t
        auto readStr = [&](wchar_t* out, int maxW) {
            char tmp[512] = {};
            int  ti = 0;
            if (*q == '"') q++;
            while (*q && *q != '"' && ti < 511) tmp[ti++] = *q++;
            if (*q == '"') q++;
            if (*q == ',') q++;
            UTF8toW(tmp, out, maxW);
        };

        Transaction t = {};
        t.id        = readInt();
        t.day       = readInt();
        t.month     = readInt();
        t.year      = readInt();
        t.amount    = readDbl();
        t.type      = readInt();
        int recur   = readInt();
        t.recurring = (recur != 0);
        readStr(t.category,    63);
        readStr(t.description, 199);

        // Basic sanity checks before storing
        if (t.id > 0 && t.amount > 0.0 &&
            t.day >= 1 && t.day <= 31 &&
            t.month >= 1 && t.month <= 12) {
            if (!t.category[0]) wcscpy(t.category, L"Other");
            g_txns[g_txnCount++] = t;
            if (t.id >= g_nextId) g_nextId = t.id + 1;
        }
    }

    if (freeData) free(data);
    free(raw);
    RecalcTotals();
}

// =============================================================
//  In-memory DLGTEMPLATE builder
//  (Lab 5: demonstrates resource creation without .rc files)
// =============================================================
static BYTE  g_dlgBuf[8192];
static int   g_dlgOfs;

static void DB_Align4()       { while (g_dlgOfs & 3) g_dlgBuf[g_dlgOfs++] = 0; }
static void DB_Word(WORD w)   { *(WORD*)(g_dlgBuf+g_dlgOfs) = w;  g_dlgOfs += 2; }
static void DB_Wstr(const wchar_t* s) {
    int n = (int)(wcslen(s)+1)*2;
    memcpy(g_dlgBuf+g_dlgOfs, s, n);
    g_dlgOfs += n;
}

// Begin a new in-memory dialog template.
// extraStyle: additional DLGTEMPLATE style bits.
// numItems must equal the exact count of DB_Item calls that follow.
static DLGTEMPLATE* DB_Begin(DWORD extraStyle, short x, short y,
                              short cx, short cy,
                              const wchar_t* title, WORD numItems)
{
    g_dlgOfs = 0;
    memset(g_dlgBuf, 0, sizeof(g_dlgBuf));

    DLGTEMPLATE* dt = (DLGTEMPLATE*)g_dlgBuf;
    dt->style           = WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_SETFONT|extraStyle;
    dt->dwExtendedStyle = 0;
    dt->cdit            = numItems;
    dt->x = x; dt->y = y; dt->cx = cx; dt->cy = cy;

    g_dlgOfs = sizeof(DLGTEMPLATE);
    DB_Word(0);           // no menu
    DB_Word(0);           // default dialog class
    DB_Wstr(title);       // caption text
    DB_Word(9);           // DS_SETFONT: point size
    DB_Wstr(L"Segoe UI"); // DS_SETFONT: font face
    return dt;
}

// Add one control to the in-memory template.
// classAtom: 0x0080=Button  0x0081=Edit  0x0082=Static  0x0085=ComboBox
static void DB_Item(DWORD style, short x, short y, short cx, short cy,
                    WORD id, WORD classAtom, const wchar_t* text,
                    DWORD exStyle = 0)
{
    DB_Align4();  // DLGITEMTEMPLATE must be DWORD-aligned
    DLGITEMTEMPLATE* di = (DLGITEMTEMPLATE*)(g_dlgBuf+g_dlgOfs);
    di->style           = style;
    di->dwExtendedStyle = exStyle;
    di->x = x; di->y = y; di->cx = cx; di->cy = cy;
    di->id              = id;
    g_dlgOfs           += sizeof(DLGITEMTEMPLATE);

    DB_Word(0xFFFF);      // class is identified by atom
    DB_Word(classAtom);
    DB_Wstr(text);        // initial window text
    DB_Word(0);           // no extra creation data
}

// =============================================================
//  UpdateStatsDlg — refresh values in the modeless stats panel
// =============================================================
static void UpdateStatsDlg() {
    if (!g_hStatsDlg) return;
    wchar_t buf[160];

    swprintf(buf,160, L"+ %.2f UAH", g_dTotalIncome);
    SetDlgItemText(g_hStatsDlg, IDC_STAT_INCOME, buf);

    swprintf(buf,160, L"- %.2f UAH", g_dTotalExpense);
    SetDlgItemText(g_hStatsDlg, IDC_STAT_EXPENSE, buf);

    swprintf(buf,160, L"%+.2f UAH", g_dBalance);
    SetDlgItemText(g_hStatsDlg, IDC_STAT_BALANCE, buf);

    swprintf(buf,160, L"%d", g_txnCount);
    SetDlgItemText(g_hStatsDlg, IDC_STAT_COUNT, buf);

    // Average expense per transaction
    int expCnt = 0;
    for (int i = 0; i < g_txnCount; i++)
        if (g_txns[i].type == 0) expCnt++;
    double avg = expCnt > 0 ? g_dTotalExpense / expCnt : 0.0;
    swprintf(buf,160, L"%.2f UAH", avg);
    SetDlgItemText(g_hStatsDlg, IDC_STAT_AVG, buf);

    // Top spending category
    double catAmt[CAT_COUNT] = {};
    for (int i = 0; i < g_txnCount; i++)
        if (g_txns[i].type == 0)
            catAmt[CatIndex(g_txns[i].category)] += g_txns[i].amount;
    int topIdx = 0;
    for (int i = 1; i < CAT_COUNT; i++)
        if (catAmt[i] > catAmt[topIdx]) topIdx = i;
    swprintf(buf,160, L"%ls (%.2f UAH)", CATEGORIES[topIdx], catAmt[topIdx]);
    SetDlgItemText(g_hStatsDlg, IDC_STAT_TOP, buf);

    // Goal progress
    if (g_bHasGoal && g_dGoalTarget > 0.0) {
        double pct = (g_dBalance / g_dGoalTarget) * 100.0;
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        swprintf(buf,160, L"%.1f%%  of  %.2f UAH  (\"%ls\")",
                 pct, g_dGoalTarget, g_sGoalName);
    } else {
        wcscpy(buf, L"No goal — use Tools \u2192 Set Budget Goal");
    }
    SetDlgItemText(g_hStatsDlg, IDC_STAT_GOAL, buf);
}

// Helper: colour controls inside dialogs to match the active theme
static LRESULT CALLBACK DlgColorProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc=(HDC)wp; SetTextColor(hdc,g_theme.text); SetBkColor(hdc,g_theme.panel);
        return (LRESULT)g_hBrushPanel;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc=(HDC)wp; SetTextColor(hdc,g_theme.text); SetBkColor(hdc,g_theme.panel);
        return (LRESULT)g_hBrushPanel;
    }
    case WM_CTLCOLORBTN: {
        HDC hdc=(HDC)wp; SetTextColor(hdc,g_theme.text); SetBkColor(hdc,g_theme.panel);
        return (LRESULT)g_hBrushPanel;
    }
    case WM_ERASEBKGND: {
        RECT rc; GetClientRect(hDlg,&rc); FillRect((HDC)wp,&rc,g_hBrushPanel); return 1;
    }
    }
    return -1;   // sentinel: caller should fall through to DefDlgProc
}

// =============================================================
//  Dialog Proc ①+②: Add / Edit Transaction  (modal)
//  Uses DialogBoxIndirect with in-memory DLGTEMPLATE.
// =============================================================
INT_PTR CALLBACK AddEditTxnDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT colorResult = DlgColorProc(hDlg, msg, wp, lp);
    if (colorResult != -1) return colorResult;

    switch (msg) {
    case WM_INITDIALOG: {
        // Pre-select Expense radio or restore edited transaction's type
        UINT radios[3] = {IDC_TXN_EXPENSE, IDC_TXN_INCOME, IDC_TXN_TRANSFER};
        int  type = (g_editId == -1) ? 0 : g_dlgType;
        CheckDlgButton(hDlg, radios[type], BST_CHECKED);

        // Populate category combobox
        HWND hCombo = GetDlgItem(hDlg, IDC_TXN_CAT);
        for (int i = 0; CATEGORIES[i]; i++)
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)CATEGORIES[i]);

        if (g_editId == -1) {
            // New transaction: defaults
            SendMessage(hCombo, CB_SETCURSEL, 0, 0);
            SYSTEMTIME st; GetLocalTime(&st);
            wchar_t dateBuf[20];
            swprintf(dateBuf, 20, L"%02d.%02d.%04d", st.wDay, st.wMonth, st.wYear);
            SetDlgItemText(hDlg, IDC_TXN_DATE, dateBuf);
        } else {
            // Editing existing: pre-fill fields
            wchar_t amtBuf[32]; swprintf(amtBuf,32,L"%.2f",g_dlgAmount);
            SetDlgItemText(hDlg, IDC_TXN_AMOUNT, amtBuf);
            SetDlgItemText(hDlg, IDC_TXN_DESC,   g_dlgDesc);
            wchar_t dateBuf[20];
            swprintf(dateBuf,20,L"%02d.%02d.%04d",g_dlgDay,g_dlgMonth,g_dlgYear);
            SetDlgItemText(hDlg, IDC_TXN_DATE, dateBuf);
            if (g_dlgRecurring) CheckDlgButton(hDlg, IDC_TXN_RECURRING, BST_CHECKED);

            // Select the matching category in the combobox
            int ci = CatIndex(g_dlgCategory);
            SendMessage(hCombo, CB_SETCURSEL, ci, 0);
        }
        SetFocus(GetDlgItem(hDlg, IDC_TXN_AMOUNT));
        return FALSE;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDOK: {
            // ── Validate amount ──────────────────────────────────
            wchar_t amtBuf[64]={};
            GetDlgItemText(hDlg, IDC_TXN_AMOUNT, amtBuf, 63);
            wchar_t* ep = NULL;
            double amount = wcstod(amtBuf, &ep);
            if (ep == amtBuf || amount <= 0.0) {
                MessageBox(hDlg, L"Please enter a valid positive amount.",
                           L"Validation Error", MB_OK|MB_ICONWARNING);
                SetFocus(GetDlgItem(hDlg, IDC_TXN_AMOUNT));
                return TRUE;
            }

            // ── Validate and parse date ──────────────────────────
            wchar_t dateBuf[32]={}; int dd=1,mm=1,yyyy=2024;
            GetDlgItemText(hDlg, IDC_TXN_DATE, dateBuf, 31);
            if (swscanf(dateBuf, L"%d.%d.%d", &dd, &mm, &yyyy) < 3 ||
                dd<1||dd>31||mm<1||mm>12||yyyy<2000||yyyy>2100) {
                MessageBox(hDlg, L"Date must be DD.MM.YYYY  (e.g. 15.04.2024)",
                           L"Validation Error", MB_OK|MB_ICONWARNING);
                SetFocus(GetDlgItem(hDlg, IDC_TXN_DATE));
                return TRUE;
            }

            // ── Gather rest of fields ────────────────────────────
            g_dlgAmount = amount;
            g_dlgDay = dd; g_dlgMonth = mm; g_dlgYear = yyyy;

            GetDlgItemText(hDlg, IDC_TXN_DESC, g_dlgDesc, 199);

            HWND hCombo = GetDlgItem(hDlg, IDC_TXN_CAT);
            int ci = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            if (ci < 0 || ci >= CAT_COUNT) ci = CAT_COUNT-1;
            wcsncpy(g_dlgCategory, CATEGORIES[ci], 63);

            g_dlgType = IsDlgButtonChecked(hDlg, IDC_TXN_INCOME)   == BST_CHECKED ? 1 :
                        IsDlgButtonChecked(hDlg, IDC_TXN_TRANSFER)  == BST_CHECKED ? 2 : 0;
            g_dlgRecurring = (IsDlgButtonChecked(hDlg, IDC_TXN_RECURRING) == BST_CHECKED);

            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

// =============================================================
//  Dialog Proc ③: Budget Goal  (modal)
// =============================================================
INT_PTR CALLBACK GoalDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT cr = DlgColorProc(hDlg, msg, wp, lp);
    if (cr != -1) return cr;

    switch (msg) {
    case WM_INITDIALOG:
        if (g_bHasGoal) {
            SetDlgItemText(hDlg, IDC_GOAL_NAME, g_sGoalName);
            wchar_t b[64]; swprintf(b,64,L"%.2f",g_dGoalTarget);
            SetDlgItemText(hDlg, IDC_GOAL_TARGET, b);
        }
        SetFocus(GetDlgItem(hDlg, IDC_GOAL_NAME));
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDOK: {
            wchar_t name[128]={}, targetBuf[64]={};
            GetDlgItemText(hDlg, IDC_GOAL_NAME,   name,      127);
            GetDlgItemText(hDlg, IDC_GOAL_TARGET, targetBuf, 63);
            double target = wcstod(targetBuf, NULL);
            if (target <= 0.0) {
                MessageBox(hDlg, L"Target must be greater than zero.",
                           L"Validation Error", MB_OK|MB_ICONWARNING);
                SetFocus(GetDlgItem(hDlg, IDC_GOAL_TARGET));
                return TRUE;
            }
            wcsncpy(g_sGoalName, name[0] ? name : L"Savings Goal", 127);
            g_dGoalTarget = target;
            g_bHasGoal    = true;
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL: EndDialog(hDlg, IDCANCEL); return TRUE;
        }
        break;
    case WM_CLOSE: EndDialog(hDlg, IDCANCEL); return TRUE;
    }
    return FALSE;
}

// =============================================================
//  Dialog Proc ④: Live Statistics  (modeless)
// =============================================================
INT_PTR CALLBACK StatsDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT cr = DlgColorProc(hDlg, msg, wp, lp);
    if (cr != -1) return cr;

    switch (msg) {
    case WM_CTLCOLORSTATIC: {
        HDC hdc=(HDC)wp;
        HWND hCtl=(HWND)lp;
        int id = GetDlgCtrlID(hCtl);
        if (id == IDC_STAT_BALANCE)
            SetTextColor(hdc, g_dBalance >= 0 ? g_theme.income : g_theme.expense);
        else if (id == IDC_STAT_INCOME)  SetTextColor(hdc, g_theme.income);
        else if (id == IDC_STAT_EXPENSE) SetTextColor(hdc, g_theme.expense);
        else SetTextColor(hdc, g_theme.text);
        SetBkColor(hdc, g_theme.panel);
        return (LRESULT)g_hBrushPanel;
    }
    case WM_INITDIALOG:
        UpdateStatsDlg();
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp)==IDC_STAT_CLOSE||LOWORD(wp)==IDCANCEL) {
            DestroyWindow(hDlg); g_hStatsDlg = NULL; return TRUE;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hDlg); g_hStatsDlg = NULL; return TRUE;
    }
    return FALSE;
}

// =============================================================
//  Show-dialog helpers
// =============================================================

// ── ①+②: Add Transaction (modal, in-memory template) ─────────
static void ShowAddEditDlg(HWND hOwner)
{
    // Build in-memory template with exactly 15 items:
    // 3 type radios + amount label+edit + category label+combo
    // + desc label+edit + date label+edit + recurring + OK + Cancel
    DB_Begin(DS_MODALFRAME|DS_CENTER, 0,0, 295,195,
             g_editId==-1 ? L"\u271A  Add Transaction"
                          : L"\u270E  Edit Transaction", 15);

    // Row 1 – transaction type (3 radios)
    DB_Item(WS_CHILD|WS_VISIBLE|SS_LEFT,
            10,13, 45,10, (WORD)-1, 0x0082, L"Type:");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTORADIOBUTTON|WS_GROUP,
            58,10, 70,14, IDC_TXN_EXPENSE,  0x0080, L"\u2193 Expense");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTORADIOBUTTON,
            130,10, 70,14, IDC_TXN_INCOME,  0x0080, L"\u2191 Income");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTORADIOBUTTON,
            202,10, 85,14, IDC_TXN_TRANSFER,0x0080, L"\u21C4 Transfer");

    // Row 2 – amount
    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT,
            10,32, 45,10, (WORD)-1, 0x0082, L"Amount:");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|WS_GROUP,
            58,29, 100,14, IDC_TXN_AMOUNT, 0x0081, L"");

    // Row 3 – category
    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT,
            10,52, 45,10, (WORD)-1, 0x0082, L"Category:");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,
            58,49, 150,90, IDC_TXN_CAT, 0x0085, L"");

    // Row 4 – description
    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT,
            10,72, 45,10, (WORD)-1, 0x0082, L"Note:");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,
            58,69, 227,14, IDC_TXN_DESC, 0x0081, L"");

    // Row 5 – date
    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT,
            10,92, 45,10, (WORD)-1, 0x0082, L"Date:");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,
            58,89, 90,14, IDC_TXN_DATE, 0x0081, L"");

    // Row 6 – recurring checkbox
    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
            58,110, 140,14, IDC_TXN_RECURRING, 0x0080, L"\u21BA Recurring transaction");

    // Buttons
    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
            178,171, 52,14, IDOK,     0x0080, L"Save");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            234,171, 52,14, IDCANCEL, 0x0080, L"Cancel");

    DialogBoxIndirect(g_hInst, (DLGTEMPLATE*)g_dlgBuf, hOwner, AddEditTxnDlgProc);
}

// ── ③: Budget Goal  (modal, in-memory template) ───────────────
static void ShowGoalDlg(HWND hOwner)
{
    DB_Begin(DS_MODALFRAME|DS_CENTER, 0,0, 235,110,
             L"\u2605  Set Budget Goal", 6);

    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT,
            8,15, 70,10, (WORD)-1, 0x0082, L"Goal name:");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,
            83,12, 143,14, IDC_GOAL_NAME, 0x0081, L"");

    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT,
            8,35, 70,10, (WORD)-1, 0x0082, L"Target (UAH):");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,
            83,32, 100,14, IDC_GOAL_TARGET, 0x0081, L"");

    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
            122,85, 52,14, IDOK,     0x0080, L"OK");
    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            178,85, 52,14, IDCANCEL, 0x0080, L"Cancel");

    DialogBoxIndirect(g_hInst, (DLGTEMPLATE*)g_dlgBuf, hOwner, GoalDlgProc);
}

// ── ④: Live Statistics  (modeless, in-memory template) ────────
static void ShowStatsDlg(HWND hOwner)
{
    if (g_hStatsDlg) { SetForegroundWindow(g_hStatsDlg); UpdateStatsDlg(); return; }

    // 14 items: 6 label+value pairs + goal info + close button
    DB_Begin(DS_CENTER, 0,0, 240,200, L"\u2665  Live Statistics", 14);

    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT, 10,16, 95,10,(WORD)-1,0x0082,L"Total Income:");
    DB_Item(WS_CHILD|WS_VISIBLE|SS_LEFT, 110,16,125,10,IDC_STAT_INCOME, 0x0082,L"0.00 UAH");

    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT, 10,32, 95,10,(WORD)-1,0x0082,L"Total Expense:");
    DB_Item(WS_CHILD|WS_VISIBLE|SS_LEFT, 110,32,125,10,IDC_STAT_EXPENSE,0x0082,L"0.00 UAH");

    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT, 10,48, 95,10,(WORD)-1,0x0082,L"Balance:");
    DB_Item(WS_CHILD|WS_VISIBLE|SS_LEFT, 110,48,125,10,IDC_STAT_BALANCE,0x0082,L"+0.00 UAH");

    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT, 10,64, 95,10,(WORD)-1,0x0082,L"Transactions:");
    DB_Item(WS_CHILD|WS_VISIBLE|SS_LEFT, 110,64, 60,10,IDC_STAT_COUNT,  0x0082,L"0");

    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT, 10,80, 95,10,(WORD)-1,0x0082,L"Avg. expense:");
    DB_Item(WS_CHILD|WS_VISIBLE|SS_LEFT, 110,80,125,10,IDC_STAT_AVG,    0x0082,L"0.00 UAH");

    DB_Item(WS_CHILD|WS_VISIBLE|SS_RIGHT, 10,96, 95,10,(WORD)-1,0x0082,L"Top category:");
    DB_Item(WS_CHILD|WS_VISIBLE|SS_LEFT, 110,96,125,10,IDC_STAT_TOP,    0x0082,L"—");

    DB_Item(WS_CHILD|WS_VISIBLE|SS_LEFT, 10,118,220,22,IDC_STAT_GOAL,   0x0082,L"No goal set");

    DB_Item(WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
            92,172, 56,14, IDC_STAT_CLOSE, 0x0080, L"Close");

    g_hStatsDlg = CreateDialogIndirect(g_hInst,(DLGTEMPLATE*)g_dlgBuf,hOwner,StatsDlgProc);
    if (g_hStatsDlg) { ShowWindow(g_hStatsDlg, SW_SHOW); UpdateStatsDlg(); }
}

// ── ⑤: ChooseColor  (standard dialog) ────────────────────────
static void ShowColorDlg(HWND hOwner)
{
    CHOOSECOLOR cc = {};
    cc.lStructSize  = sizeof(CHOOSECOLOR);
    cc.hwndOwner    = hOwner;
    cc.rgbResult    = g_theme.accent;
    cc.lpCustColors = g_customColors;  // persists user's custom palette squares
    cc.Flags        = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc)) {
        g_theme.accent = cc.rgbResult;
        if (g_hBrushAccent) DeleteObject(g_hBrushAccent);
        g_hBrushAccent = CreateSolidBrush(g_theme.accent);
        InvalidateRect(hOwner, NULL, TRUE);
    }
}

// ── ⑥: ChooseFont  (standard dialog) ─────────────────────────
static void ShowFontDlg(HWND hOwner)
{
    LOGFONT lf = {};
    GetObject(g_hFontList ? g_hFontList : g_hFontUI, sizeof(LOGFONT), &lf);

    CHOOSEFONT cf = {};
    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner   = hOwner;
    cf.lpLogFont   = &lf;
    cf.rgbColors   = g_theme.text;
    cf.Flags       = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS;

    if (ChooseFont(&cf)) {
        HFONT hNew = CreateFontIndirect(&lf);
        if (hNew) {
            if (g_hFontList) DeleteObject(g_hFontList);
            g_hFontList = hNew;
            // Apply to the transaction listbox immediately
            SendMessage(g_hListBox, WM_SETFONT, (WPARAM)g_hFontList, TRUE);
        }
    }
}

// =============================================================
//  Transaction management
// =============================================================

// Apply text search + category filter → rebuild g_filtered[], sorted newest-first
static void BuildFilter() {
    wchar_t searchBuf[200] = {};
    if (g_hSearch) GetWindowText(g_hSearch, searchBuf, 199);
    wchar_t* srch = searchBuf;
    // Lowercase search term
    for (wchar_t* p = srch; *p; p++) *p = towlower(*p);

    g_filteredCount = 0;
    for (int i = 0; i < g_txnCount; i++) {
        // Category filter
        if (g_filterCat > 0) {
            if (_wcsicmp(g_txns[i].category, CATEGORIES[g_filterCat-1]) != 0)
                continue;
        }
        // Text search (case-insensitive, checks description + category)
        if (srch[0]) {
            wchar_t descLow[200], catLow[64];
            wcsncpy(descLow, g_txns[i].description, 199); descLow[199]=0;
            wcsncpy(catLow,  g_txns[i].category,    63);  catLow[63]=0;
            for (wchar_t* p=descLow; *p; p++) *p=towlower(*p);
            for (wchar_t* p=catLow;  *p; p++) *p=towlower(*p);
            if (!wcsstr(descLow, srch) && !wcsstr(catLow, srch))
                continue;
        }
        g_filtered[g_filteredCount++] = i;
    }

    // Sort by date descending (newest first)
    for (int i = 0; i < g_filteredCount-1; i++) {
        for (int j = i+1; j < g_filteredCount; j++) {
            int ki = g_txns[g_filtered[i]].year*10000
                   + g_txns[g_filtered[i]].month*100
                   + g_txns[g_filtered[i]].day;
            int kj = g_txns[g_filtered[j]].year*10000
                   + g_txns[g_filtered[j]].month*100
                   + g_txns[g_filtered[j]].day;
            if (kj > ki) { int tmp=g_filtered[i]; g_filtered[i]=g_filtered[j]; g_filtered[j]=tmp; }
        }
    }
}

// Repopulate the transaction listbox from the current filter
static void RefreshList() {
    SendMessage(g_hListBox, WM_SETREDRAW, FALSE, 0);
    SendMessage(g_hListBox, LB_RESETCONTENT, 0, 0);

    for (int i = 0; i < g_filteredCount; i++) {
        // Add a placeholder string; the actual drawing is done in WM_DRAWITEM
        int idx = (int)SendMessage(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"");
        if (idx != LB_ERR)
            SendMessage(g_hListBox, LB_SETITEMDATA, idx, (LPARAM)g_filtered[i]);
    }

    SendMessage(g_hListBox, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hListBox, NULL, TRUE);
}

// Rebuild category sidebar
static void RefreshCatList() {
    SendMessage(g_hCatList, LB_RESETCONTENT, 0, 0);
    SendMessage(g_hCatList, LB_ADDSTRING, 0, (LPARAM)L"All Transactions");
    for (int i = 0; CATEGORIES[i]; i++)
        SendMessage(g_hCatList, LB_ADDSTRING, 0, (LPARAM)CATEGORIES[i]);
    SendMessage(g_hCatList, LB_SETCURSEL, g_filterCat, 0);
}

// Add a new transaction and refresh UI
static void AddTransaction(double amount, const wchar_t* cat,
                            const wchar_t* desc, int day, int month, int year,
                            int type, bool recurring)
{
    if (g_txnCount >= 2000) {
        MessageBox(g_hMain, L"Transaction limit (2000) reached.",
                   L"CashFlow", MB_OK|MB_ICONWARNING);
        return;
    }
    Transaction& t = g_txns[g_txnCount++];
    t.id = g_nextId++;
    t.amount = amount;
    t.day = day; t.month = month; t.year = year;
    t.type = type;
    t.recurring = recurring;
    wcsncpy(t.category,    cat,  63);
    wcsncpy(t.description, desc, 199);

    RecalcTotals();
    BuildFilter();
    RefreshList();
    UpdateStatsDlg();
    InvalidateRect(g_hMain, NULL, FALSE);  // repaint header cards
}

// Update an existing transaction by ID
static void UpdateTransaction(int id, double amount, const wchar_t* cat,
                               const wchar_t* desc, int day, int month, int year,
                               int type, bool recurring)
{
    for (int i = 0; i < g_txnCount; i++) {
        if (g_txns[i].id == id) {
            g_txns[i].amount    = amount;
            g_txns[i].day       = day;
            g_txns[i].month     = month;
            g_txns[i].year      = year;
            g_txns[i].type      = type;
            g_txns[i].recurring = recurring;
            wcsncpy(g_txns[i].category,    cat,  63);
            wcsncpy(g_txns[i].description, desc, 199);
            break;
        }
    }
    RecalcTotals();
    BuildFilter();
    RefreshList();
    UpdateStatsDlg();
    InvalidateRect(g_hMain, NULL, FALSE);
}

// Delete the currently selected transaction
static void DeleteSelected() {
    int sel = (int)SendMessage(g_hListBox, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) {
        MessageBox(g_hMain, L"Please select a transaction to delete.",
                   L"CashFlow", MB_OK|MB_ICONINFORMATION);
        return;
    }
    int txnIdx = (int)SendMessage(g_hListBox, LB_GETITEMDATA, sel, 0);
    if (txnIdx < 0 || txnIdx >= g_txnCount) return;

    wchar_t confirmMsg[256];
    swprintf(confirmMsg, 256,
        L"Delete this transaction?\n\n"
        L"Amount: %.2f UAH\nDescription: %ls\nDate: %02d.%02d.%04d",
        g_txns[txnIdx].amount, g_txns[txnIdx].description,
        g_txns[txnIdx].day, g_txns[txnIdx].month, g_txns[txnIdx].year);

    // MessageBox — a built-in modal system dialog
    if (MessageBox(g_hMain, confirmMsg, L"Confirm Delete",
                   MB_YESNO|MB_ICONQUESTION) != IDYES) return;

    // Remove from array by shifting
    for (int i = txnIdx; i < g_txnCount-1; i++)
        g_txns[i] = g_txns[i+1];
    g_txnCount--;

    RecalcTotals();
    BuildFilter();
    RefreshList();
    UpdateStatsDlg();
    InvalidateRect(g_hMain, NULL, FALSE);
}

// =============================================================
//  Owner-draw: draw one transaction list item
// =============================================================
static void DrawTransactionItem(DRAWITEMSTRUCT* dis)
{
    if (dis->itemID == (UINT)-1) return;
    int txnIdx = (int)dis->itemData;
    if (txnIdx < 0 || txnIdx >= g_txnCount) return;

    const Transaction& t = g_txns[txnIdx];
    HDC hdc = dis->hDC;
    RECT rc  = dis->rcItem;

    // ── Background ────────────────────────────────────────────
    bool sel = (dis->itemState & ODS_SELECTED) != 0;
    bool alt = (dis->itemID % 2 == 1);
    COLORREF bgCol = sel ? g_theme.selection : (alt ? g_theme.panelAlt : g_theme.panel);
    FillColor(hdc, rc, bgCol);

    // ── Left type stripe (4px) ───────────────────────────────
    COLORREF typeCol = (t.type==1) ? g_theme.income :
                       (t.type==2) ? g_theme.transfer : g_theme.expense;
    RECT stripe = { rc.left, rc.top, rc.left+4, rc.bottom };
    FillColor(hdc, stripe, typeCol);

    // ── Bottom separator line ─────────────────────────────────
    DrawSeparator(hdc, rc.left, rc.bottom-1, rc.right, g_theme.border);

    SetBkMode(hdc, TRANSPARENT);

    // ── Category colour dot (12x12) ───────────────────────────
    int ci = CatIndex(t.category);
    COLORREF dotCol = CAT_COLORS[ci];
    RECT dot = { rc.left+12, rc.top+13, rc.left+24, rc.top+25 };
    HBRUSH hDotBr = CreateSolidBrush(dotCol);
    HPEN   hDotPn = CreatePen(PS_SOLID, 0, dotCol);
    SelectObject(hdc, hDotBr); SelectObject(hdc, hDotPn);
    Ellipse(hdc, dot.left, dot.top, dot.right, dot.bottom);
    DeleteObject(hDotBr); DeleteObject(hDotPn);

    // ── Amount (right side, bold, coloured) ───────────────────
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBold);
    SetTextColor(hdc, typeCol);

    wchar_t amtBuf[40];
    const wchar_t* sign = (t.type==1) ? L"+" : (t.type==2) ? L"\u00B1" : L"-";
    swprintf(amtBuf, 40, L"%ls%.2f UAH", sign, t.amount);

    RECT amtRect = { rc.right-150, rc.top+6, rc.right-10, rc.top+26 };
    DrawText(hdc, amtBuf, -1, &amtRect, DT_RIGHT|DT_SINGLELINE|DT_VCENTER);

    // ── Type label under amount (small, dim) ──────────────────
    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, g_theme.textDim);
    const wchar_t* typeLbl = (t.type==1) ? L"Income" :
                              (t.type==2) ? L"Transfer" : L"Expense";
    wchar_t typeFull[32];
    swprintf(typeFull, 32, L"%ls%ls", typeLbl, t.recurring ? L" \u21BA" : L"");
    RECT typeRect = { rc.right-150, rc.top+28, rc.right-10, rc.bottom-6 };
    DrawText(hdc, typeFull, -1, &typeRect, DT_RIGHT|DT_SINGLELINE|DT_VCENTER);

    // ── Description (left, primary text) ─────────────────────
    SelectObject(hdc, g_hFontList ? g_hFontList : g_hFontUI);
    SetTextColor(hdc, g_theme.text);
    wchar_t descDisp[200];
    wcsncpy(descDisp, t.description[0] ? t.description : t.category, 199);
    RECT descRect = { rc.left+30, rc.top+6, rc.right-155, rc.top+26 };
    DrawText(hdc, descDisp, -1, &descRect, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);

    // ── Category + Date (small, muted) ────────────────────────
    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, g_theme.textDim);
    wchar_t subBuf[120];
    const wchar_t* mo = (t.month>=1&&t.month<=12) ? MONTH_SHORT[t.month] : L"?";
    swprintf(subBuf, 120, L"%ls  \u2022  %d %ls %d",
             t.category, t.day, mo, t.year);
    RECT subRect = { rc.left+30, rc.top+28, rc.right-155, rc.bottom-6 };
    DrawText(hdc, subBuf, -1, &subRect, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);

    SelectObject(hdc, oldFont);

    // Focus rect when keyboard focus is on this item
    if ((dis->itemState & ODS_FOCUS) && (dis->itemAction & (ODA_FOCUS|ODA_DRAWENTIRE)))
        DrawFocusRect(hdc, &rc);
}

// =============================================================
//  Header cards  (painted by OnPaint)
// =============================================================
// Cards always have a dark background (60% of header colour).
// Therefore ALL text inside cards must be explicitly light/white —
// never rely on g_theme.income/expense which can be dark on light themes.
static void DrawHeaderCard(HDC hdc, RECT r,
                           const wchar_t* label, const wchar_t* value,
                           COLORREF valCol, COLORREF cardBg)
{
    // Shadow: 50% of cardBg
    RECT shadow = { r.left+2, r.top+2, r.right+2, r.bottom+2 };
    COLORREF shadowCol = RGB(
        (BYTE)(GetRValue(cardBg) / 2),
        (BYTE)(GetGValue(cardBg) / 2),
        (BYTE)(GetBValue(cardBg) / 2));
    DrawCard(hdc, shadow, shadowCol, shadowCol, 8);

    // Border: cardBg + 25
    COLORREF borderCol = RGB(
        (BYTE)std::min(255, (int)GetRValue(cardBg) + 25),
        (BYTE)std::min(255, (int)GetGValue(cardBg) + 25),
        (BYTE)std::min(255, (int)GetBValue(cardBg) + 25));
    DrawCard(hdc, r, cardBg, borderCol, 8);

    SetBkMode(hdc, TRANSPARENT);

    // Label: always soft white — card bg is always dark, so this is always readable
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, RGB(190, 190, 210));
    RECT lblR = { r.left+12, r.top+10, r.right-8, r.top+24 };
    DrawText(hdc, label, -1, &lblR, DT_LEFT|DT_SINGLELINE);

    // Value: always use a BRIGHT tint of valCol so it's visible on the dark card.
    // We boost each channel toward 255 by 40% to guarantee contrast.
    COLORREF brightVal = RGB(
        (BYTE)std::min(255, (int)GetRValue(valCol) + (255 - (int)GetRValue(valCol)) * 40 / 100),
        (BYTE)std::min(255, (int)GetGValue(valCol) + (255 - (int)GetGValue(valCol)) * 40 / 100),
        (BYTE)std::min(255, (int)GetBValue(valCol) + (255 - (int)GetBValue(valCol)) * 40 / 100));
    SelectObject(hdc, g_hFontAmount);
    SetTextColor(hdc, brightVal);
    RECT valR = { r.left+10, r.top+26, r.right-8, r.bottom-8 };
    DrawText(hdc, value, -1, &valR, DT_LEFT|DT_SINGLELINE|DT_VCENTER);

    SelectObject(hdc, oldFont);
}

// =============================================================
//  CreateControls
// =============================================================
static void CreateAppControls(HWND hwnd)
{
    // ── Toolbar ───────────────────────────────────────────────
    // iString = -1: icon-only buttons; tooltips come from TTN_GETDISPINFO
    TBBUTTON tbb[] = {
        {0,IDT_ADD,   TBSTATE_ENABLED,BTNS_BUTTON,{0},0,-1},
        {1,IDT_EDIT,  TBSTATE_ENABLED,BTNS_BUTTON,{0},0,-1},
        {2,IDT_DELETE,TBSTATE_ENABLED,BTNS_BUTTON,{0},0,-1},
        {0,0,         TBSTATE_ENABLED,BTNS_SEP,   {0},0, 0},
        {3,IDT_STATS, TBSTATE_ENABLED,BTNS_BUTTON,{0},0,-1},
        {4,IDT_GOAL,  TBSTATE_ENABLED,BTNS_BUTTON,{0},0,-1},
        {0,0,         TBSTATE_ENABLED,BTNS_SEP,   {0},0, 0},
        {5,IDT_THEME, TBSTATE_ENABLED,BTNS_BUTTON,{0},0,-1},
    };
    g_hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT|TBSTYLE_TOOLTIPS|CCS_TOP,
        0,0,0,0, hwnd,(HMENU)IDC_TOOLBAR, g_hInst, NULL);
    SendMessage(g_hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(g_hToolbar, TB_SETBITMAPSIZE, 0, MAKELPARAM(16,16));

    // Create colourful icon squares for toolbar buttons
    HIMAGELIST hIL = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,6,0);
    COLORREF iconCols[] = {
        RGB(108,99,255), RGB(74,222,128), RGB(252,100,100),
        RGB(96,165,250), RGB(234,179,8), RGB(148,163,184)
    };
    for (int i = 0; i < 6; i++) {
        HDC hdc = CreateCompatibleDC(NULL);
        HBITMAP hbm = CreateBitmap(16,16,1,32,NULL);
        HBITMAP old = (HBITMAP)SelectObject(hdc, hbm);
        HBRUSH hb = CreateSolidBrush(iconCols[i]);
        RECT rr={2,2,14,14};
        HBRUSH obr=(HBRUSH)SelectObject(hdc,hb);
        HPEN   hp=CreatePen(PS_SOLID,0,iconCols[i]);
        HPEN   op=(HPEN)SelectObject(hdc,hp);
        RoundRect(hdc,rr.left,rr.top,rr.right,rr.bottom,4,4);
        SelectObject(hdc,obr); SelectObject(hdc,op);
        DeleteObject(hb); DeleteObject(hp);
        SelectObject(hdc, old); DeleteDC(hdc);
        ImageList_Add(hIL, hbm, NULL);
        DeleteObject(hbm);
    }
    SendMessage(g_hToolbar, TB_SETIMAGELIST, 0, (LPARAM)hIL);
    SendMessage(g_hToolbar, TB_ADDBUTTONS, 8, (LPARAM)tbb);
    SendMessage(g_hToolbar, TB_AUTOSIZE, 0, 0);

    // ── Status bar ─────────────────────────────────────────────
    g_hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
        WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP,
        0,0,0,0, hwnd,(HMENU)IDC_STATUSBAR, g_hInst, NULL);
    int sbParts[] = {260,500,-1};
    SendMessage(g_hStatus, SB_SETPARTS, 3, (LPARAM)sbParts);

    // ── Search bar (Edit) ─────────────────────────────────────
    g_hSearch = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
        0,0,0,0, hwnd,(HMENU)IDC_SEARCH, g_hInst, NULL);
    SendMessage(g_hSearch, EM_SETCUEBANNER, TRUE,
                (LPARAM)L"  Search transactions...");

    // ── Category filter listbox (sidebar, owner-draw for full theming) ──
    g_hCatList = CreateWindowEx(0, L"LISTBOX", NULL,
        WS_CHILD|WS_VISIBLE|LBS_NOTIFY|LBS_HASSTRINGS|
        LBS_OWNERDRAWFIXED|LBS_NOINTEGRALHEIGHT,
        0,0,0,0, hwnd,(HMENU)IDC_CAT_LIST, g_hInst, NULL);
    RefreshCatList();

    // ── Transaction listbox (owner-draw, main area) ────────────
    g_hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
        WS_CHILD|WS_VISIBLE|WS_VSCROLL|
        LBS_NOTIFY|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT,
        0,0,0,0, hwnd,(HMENU)IDC_LISTBOX, g_hInst, NULL);

    SetTimer(hwnd, TIMER_CLOCK, 1000, NULL);
}

// =============================================================
//  Menu
// =============================================================
static void CreateAppMenu(HWND hwnd)
{
    HMENU hMenu  = CreateMenu();
    HMENU hFile  = CreatePopupMenu();
    HMENU hTxn   = CreatePopupMenu();
    HMENU hTools = CreatePopupMenu();
    HMENU hView  = CreatePopupMenu();

    AppendMenu(hFile, MF_STRING, IDM_FILE_NEW,    L"&New / Clear All\tCtrl+N");
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFile, MF_STRING, IDM_FILE_IMPORT, L"&Import CSV…\tCtrl+O");
    AppendMenu(hFile, MF_STRING, IDM_FILE_EXPORT, L"E&xport CSV…\tCtrl+S");
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFile, MF_STRING, IDM_FILE_EXIT,   L"E&xit\tAlt+F4");

    // Transaction menu — demonstrates various dialog invocations
    AppendMenu(hTxn, MF_STRING, IDM_TXN_ADD,    L"\u271A Add Transaction…\tCtrl+D");
    AppendMenu(hTxn, MF_STRING, IDM_TXN_EDIT,   L"\u270E Edit Selected…\tF2");
    AppendMenu(hTxn, MF_STRING, IDM_TXN_DELETE, L"\u2716 Delete Selected\tDel");

    AppendMenu(hTools, MF_STRING, IDM_TOOLS_GOAL,  L"\u2605 Set Budget Goal…");
    AppendMenu(hTools, MF_STRING, IDM_VIEW_STATS,  L"\u2665 Live Statistics…\tCtrl+W");
    AppendMenu(hTools, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTools, MF_STRING, IDM_TOOLS_COLOR, L"\u25A0 Change Accent Colour…");
    AppendMenu(hTools, MF_STRING, IDM_TOOLS_FONT,  L"A\u0332 Change List Font…");
    AppendMenu(hTools, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTools, MF_STRING, IDM_TOOLS_ABOUT, L"About CashFlow…");

    AppendMenu(hView, MF_STRING|MF_CHECKED, IDM_VIEW_TOOLBAR, L"&Toolbar");
    AppendMenu(hView, MF_STRING|MF_CHECKED, IDM_VIEW_STATUS,  L"&Status Bar");
    AppendMenu(hView, MF_SEPARATOR, 0, NULL);
    AppendMenu(hView, MF_STRING, IDM_VIEW_THEME,
               L"Cycle &Theme (Dark / Light / Pink / Burgundy)\tCtrl+T");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFile,  L"&File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hTxn,   L"&Transaction");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hTools, L"&Tools");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hView,  L"&View");

    SetMenu(hwnd, hMenu);
}

// =============================================================
//  Layout
// =============================================================
static void LayoutControls(HWND hwnd)
{
    RECT rc; GetClientRect(hwnd, &rc);
    int W = rc.right, H = rc.bottom;

    int tbH = 0, sbH = 0;
    if (g_bShowToolbar && g_hToolbar) {
        RECT tr; GetWindowRect(g_hToolbar, &tr); tbH = tr.bottom - tr.top;
    }
    if (g_bShowStatus && g_hStatus) {
        RECT sr; GetWindowRect(g_hStatus, &sr); sbH = sr.bottom - sr.top;
    }

    int contentTop = tbH + HEADER_H + 8;
    int contentBot = H - sbH - 8;
    int contentH   = contentBot - contentTop;

    int sideX = 8;
    int sideW = SIDEBAR_W;
    int mainX = sideX + sideW + 8;
    int mainW = W - mainX - 8;

    // Category sidebar: full content height
    MoveWindow(g_hCatList, sideX, contentTop, sideW, contentH, TRUE);

    // Search bar above transaction list
    MoveWindow(g_hSearch, mainX, contentTop, mainW, SEARCH_H, TRUE);

    // Transaction listbox fills the rest
    int listTop = contentTop + SEARCH_H + 4;
    int listH   = contentBot - listTop;
    MoveWindow(g_hListBox, mainX, listTop, mainW, listH > 0 ? listH : 1, TRUE);
}

// =============================================================
//  OnPaint — custom background + header stat cards
// =============================================================
static void OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc; GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, g_hBrushBg);

    int tbH = 0;
    if (g_bShowToolbar && g_hToolbar) {
        RECT tr; GetWindowRect(g_hToolbar, &tr); tbH = tr.bottom - tr.top;
    }

    // ── Header panel ──────────────────────────────────────────
    RECT hdrRect = { 0, tbH, rc.right, tbH + HEADER_H };
    FillRect(hdc, &hdrRect, g_hBrushHeader);

    // Subtle bottom edge on header
    DrawSeparator(hdc, 0, tbH + HEADER_H - 1, rc.right, g_theme.border);

    // ── App title ─────────────────────────────────────────────
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontBold);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255,255,255));
    RECT titleR = { 14, tbH+10, 200, tbH+30 };
    DrawText(hdc, L"CashFlow \u2605", -1, &titleR, DT_LEFT|DT_SINGLELINE);

    SelectObject(hdc, g_hFontSmall);
    // Subtitle: semi-transparent white (rgba simulation: white at 70% opacity on header)
    SetTextColor(hdc, RGB(210, 210, 230));
    RECT subtitleR = { 14, tbH+28, 250, tbH+44 };
    DrawText(hdc, L"Personal Budget Tracker", -1, &subtitleR, DT_LEFT|DT_SINGLELINE);
    SelectObject(hdc, oldFont);

    // ── Stat cards ────────────────────────────────────────────
    int cardY  = tbH + 12;
    int cardH  = HEADER_H - 24;
    int W      = rc.right;
    int cardW  = 200;
    int gap    = 12;
    // Place 3 cards on the right side of the header
    int startX = W - (cardW * 3 + gap * 2) - 14;

    // Card bg: darker shade derived from header colour (no hardcoded palette)
    COLORREF cardBg = RGB(
        (BYTE)(GetRValue(g_theme.header) * 60 / 100),
        (BYTE)(GetGValue(g_theme.header) * 60 / 100),
        (BYTE)(GetBValue(g_theme.header) * 60 / 100));

    wchar_t valBuf[64];

    // Balance card
    swprintf(valBuf, 64, L"%+.2f UAH", g_dBalance);
    COLORREF balCol = g_dBalance >= 0 ? g_theme.income : g_theme.expense;
    RECT c1 = { startX, cardY, startX+cardW, cardY+cardH };
    DrawHeaderCard(hdc, c1, L"BALANCE", valBuf, balCol, cardBg);

    // Income card
    startX += cardW + gap;
    swprintf(valBuf, 64, L"+%.2f UAH", g_dTotalIncome);
    RECT c2 = { startX, cardY, startX+cardW, cardY+cardH };
    DrawHeaderCard(hdc, c2, L"INCOME", valBuf, g_theme.income, cardBg);

    // Expense card
    startX += cardW + gap;
    swprintf(valBuf, 64, L"-%.2f UAH", g_dTotalExpense);
    RECT c3 = { startX, cardY, startX+cardW, cardY+cardH };
    DrawHeaderCard(hdc, c3, L"EXPENSES", valBuf, g_theme.expense, cardBg);

    // ── Goal progress bar ─────────────────────────────────────
    if (g_bHasGoal && g_dGoalTarget > 0.0) {
        double pct = g_dBalance / g_dGoalTarget;
        if (pct < 0) pct = 0;
        if (pct > 1) pct = 1;

        int barX = 14, barY = tbH + HEADER_H - 22;
        int barW = W - 14 - (W - (W - (cardW*3+gap*2) - 14)) - 20;
        if (barW < 100) barW = 100;
        int barH = 10;

        // Track bg
        RECT barBg = { barX, barY, barX+barW, barY+barH };
        // Progress track: 40% brightness of header colour
        COLORREF trackCol = RGB(
            (BYTE)(GetRValue(g_theme.header) * 40 / 100),
            (BYTE)(GetGValue(g_theme.header) * 40 / 100),
            (BYTE)(GetBValue(g_theme.header) * 40 / 100));
        DrawCard(hdc, barBg, trackCol, trackCol, 4);

        // Fill
        int fillW = (int)(barW * pct);
        if (fillW > 4) {
            RECT barFill = { barX, barY, barX+fillW, barY+barH };
            DrawCard(hdc, barFill, g_theme.accent, g_theme.accent, 4);
        }

        // Label
        SelectObject(hdc, g_hFontSmall);
        // Goal label: always white — header is always a coloured/dark background
        SetTextColor(hdc, RGB(220, 220, 235));
        wchar_t goalBuf[128];
        swprintf(goalBuf, 128, L"\u2605 Goal: \"%ls\"  %.1f%% of %.0f UAH",
                 g_sGoalName, pct*100.0, g_dGoalTarget);
        RECT goalR = { barX, tbH + HEADER_H - 36, barX+barW+200, barY };
        DrawText(hdc, goalBuf, -1, &goalR, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    }

    // ── Sidebar title ─────────────────────────────────────────
    int sbH2 = 0;
    if (g_bShowStatus && g_hStatus) {
        RECT sr; GetWindowRect(g_hStatus, &sr); sbH2 = sr.bottom - sr.top;
    }
    int contentTop = tbH + HEADER_H + 8;

    SelectObject(hdc, g_hFontSmall);
    SetTextColor(hdc, g_theme.textDim);
    RECT catTitleR = { 8, contentTop - 16, 8+SIDEBAR_W, contentTop };
    DrawText(hdc, L"CATEGORIES", -1, &catTitleR, DT_LEFT|DT_SINGLELINE|DT_VCENTER);

    int mainX = 8 + SIDEBAR_W + 8;
    RECT txnTitleR = { mainX, contentTop - 16, mainX + 200, contentTop };
    DrawText(hdc, L"TRANSACTIONS", -1, &txnTitleR, DT_LEFT|DT_SINGLELINE|DT_VCENTER);

    EndPaint(hwnd, &ps);
}

// =============================================================
//  Update status bar text
// =============================================================
static void UpdateStatus() {
    wchar_t s[120];
    swprintf(s, 120, L"  %d transactions  |  Balance: %+.2f UAH", g_txnCount, g_dBalance);
    SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)s);

    swprintf(s, 120, L"  Showing: %d", g_filteredCount);
    SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)s);
}

// Apply dynamic theme colours to native controls that don't respond to WM_CTLCOLORx.
// Called once at startup and again on every theme switch.
static void ApplyThemeToControls() {
    if (!g_hStatus) return;
    // Status bar: SB_SETBKCOLOR sets its background (text colour uses system default,
    // but the bg change alone makes it match the theme).
    SendMessage(g_hStatus, SB_SETBKCOLOR, 0, (LPARAM)g_theme.panel);
    // Force a repaint of the statusbar so the new bg is visible immediately
    InvalidateRect(g_hStatus, NULL, TRUE);
}

// =============================================================
//  WndProc
// =============================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {

    // ── WM_MEASUREITEM: set heights for both owner-draw listboxes ─
    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT* mis = (MEASUREITEMSTRUCT*)lp;
        if (mis->CtlID == IDC_LISTBOX)
            mis->itemHeight = ITEM_H;
        if (mis->CtlID == IDC_CAT_LIST)
            mis->itemHeight = 26;   // compact sidebar items
        return TRUE;
    }

    // ── WM_DRAWITEM: render owner-draw listbox items ───────────
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lp;

        // Main transaction list
        if (dis->CtlID == IDC_LISTBOX && dis->itemAction != ODA_FOCUS)
            DrawTransactionItem(dis);

        // Category sidebar items (owner-draw for full theme support)
        if (dis->CtlID == IDC_CAT_LIST && dis->itemID != (UINT)-1) {
            HDC hdc   = dis->hDC;
            RECT rc   = dis->rcItem;
            bool sel  = (dis->itemState & ODS_SELECTED) != 0;

            // Background
            COLORREF bgCol = sel ? g_theme.selection : g_theme.panel;
            FillColor(hdc, rc, bgCol);

            // Active-filter accent bar on the left edge
            if (sel) {
                RECT bar = { rc.left, rc.top, rc.left+3, rc.bottom };
                FillColor(hdc, bar, g_theme.accent);
            }

            // Bottom separator
            DrawSeparator(hdc, rc.left, rc.bottom-1, rc.right, g_theme.border);

            // Category colour dot (index 0 = "All" has no dot)
            if (dis->itemID > 0) {
                int ci = (int)dis->itemID - 1;   // 0-based category index
                COLORREF dotCol = (ci < CAT_COUNT) ? CAT_COLORS[ci] : CAT_COLORS[10];
                HBRUSH hb = CreateSolidBrush(dotCol);
                HPEN   hp = CreatePen(PS_SOLID, 0, dotCol);
                SelectObject(hdc, hb); SelectObject(hdc, hp);
                Ellipse(hdc, rc.left+10, rc.top+8, rc.left+20, rc.top+18);
                DeleteObject(hb); DeleteObject(hp);
            }

            // Category label text
            wchar_t text[128] = {};
            SendMessage(g_hCatList, LB_GETTEXT, dis->itemID, (LPARAM)text);
            SetBkMode(hdc, TRANSPARENT);
            HFONT oldF = (HFONT)SelectObject(hdc, sel ? g_hFontBold : g_hFontUI);
            SetTextColor(hdc, sel ? g_theme.text : g_theme.textDim);
            RECT tr = { rc.left+24, rc.top, rc.right-4, rc.bottom };
            DrawText(hdc, text, -1, &tr,
                     DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
            SelectObject(hdc, oldF);
        }
        return TRUE;
    }

    case WM_CREATE:
        CreateAppMenu(hwnd);
        ApplyTheme(g_iTheme);

        g_hFontUI    = CreateFont(15,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                         DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
        g_hFontBold  = CreateFont(15,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                         DEFAULT_PITCH|FF_SWISS, L"Segoe UI Semibold");
        g_hFontSmall = CreateFont(11,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                         DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
        g_hFontAmount= CreateFont(20,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
                         DEFAULT_PITCH|FF_SWISS, L"Segoe UI");

        CreateAppControls(hwnd);

        // Apply fonts to controls
        SendMessage(g_hSearch,  WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        SendMessage(g_hCatList, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        SendMessage(g_hListBox, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        SendMessage(g_hStatus,  WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        // Auto-load saved data on startup
        LoadCSV(g_autoSavePath);
        BuildFilter();
        RefreshList();
        UpdateStatus();
        ApplyThemeToControls();
        SendMessage(g_hStatus, SB_SETTEXT, 2, (LPARAM)L"  CashFlow v1.0  \u2665");
        return 0;

    case WM_SIZE:
        if (g_hToolbar) SendMessage(g_hToolbar, TB_AUTOSIZE, 0, 0);
        if (g_hStatus)  SendMessage(g_hStatus,  WM_SIZE, wp, lp);
        LayoutControls(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_PAINT:    OnPaint(hwnd); return 0;
    case WM_ERASEBKGND: return 1;

    // ── Colour child controls to match the active theme ────────
    case WM_CTLCOLOREDIT: {
        HDC hdc=(HDC)wp;
        SetTextColor(hdc, g_theme.text);
        SetBkColor(hdc,   g_theme.panel);
        return (LRESULT)g_hBrushPanel;
    }
    case WM_CTLCOLORLISTBOX: {
        HDC hdc=(HDC)wp;
        SetTextColor(hdc, g_theme.text);
        SetBkColor(hdc,   g_theme.panel);
        return (LRESULT)g_hBrushPanel;
    }
    case WM_CTLCOLORSTATIC: {
        // Use full-brightness text colour so labels are always readable
        HDC hdc=(HDC)wp;
        SetTextColor(hdc, g_theme.text);
        SetBkColor(hdc,   g_theme.bg);
        return (LRESULT)g_hBrushBg;
    }
    case WM_CTLCOLORBTN: {
        // Checkboxes and radio buttons — background must match their parent bg
        HDC hdc=(HDC)wp;
        SetTextColor(hdc, g_theme.text);
        SetBkColor(hdc,   g_theme.panel);
        return (LRESULT)g_hBrushPanel;
    }

    // ── Toolbar tooltips + custom draw ────────────────────────
    case WM_NOTIFY: {
        NMHDR* nmh = (NMHDR*)lp;

        // Custom-draw the toolbar so its background and text follow the theme
        if (nmh->hwndFrom == g_hToolbar && nmh->code == NM_CUSTOMDRAW) {
            NMTBCUSTOMDRAW* cd = (NMTBCUSTOMDRAW*)lp;
            switch (cd->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                // Fill the entire toolbar strip with the bg colour
                FillRect(cd->nmcd.hdc, &cd->nmcd.rc, g_hBrushBg);
                return CDRF_NOTIFYITEMDRAW;

            case CDDS_ITEMPREPAINT: {
                // Highlight hovered/pressed items; rest use bg colour
                bool hot     = (cd->nmcd.uItemState & CDIS_HOT)     != 0;
                bool pressed = (cd->nmcd.uItemState & CDIS_SELECTED) != 0;
                COLORREF itemBg = pressed ? g_theme.selection :
                                  hot     ? g_theme.panel : g_theme.bg;
                HBRUSH hbItem = CreateSolidBrush(itemBg);
                FillRect(cd->nmcd.hdc, &cd->nmcd.rc, hbItem);
                DeleteObject(hbItem);
                // Force text colour — return CDRF_NEWFONT so the toolbar
                // actually uses the SetTextColor we set here
                SetTextColor(cd->nmcd.hdc, g_theme.text);
                SetBkColor(cd->nmcd.hdc, itemBg);
                SetBkMode(cd->nmcd.hdc, TRANSPARENT);
                return CDRF_NEWFONT;   // ← tells toolbar to use our HDC colours
            }
            }
        }

        if (nmh->code == TTN_GETDISPINFO) {
            NMTTDISPINFO* tt = (NMTTDISPINFO*)lp;
            switch (nmh->idFrom) {
            case IDT_ADD:    tt->lpszText=(LPWSTR)L"Add Transaction (Ctrl+D)"; break;
            case IDT_EDIT:   tt->lpszText=(LPWSTR)L"Edit Selected (F2)";       break;
            case IDT_DELETE: tt->lpszText=(LPWSTR)L"Delete Selected (Del)";    break;
            case IDT_STATS:  tt->lpszText=(LPWSTR)L"Live Statistics (Ctrl+W)"; break;
            case IDT_GOAL:   tt->lpszText=(LPWSTR)L"Set Budget Goal";          break;
            case IDT_THEME:  tt->lpszText=(LPWSTR)L"Toggle Dark/Light (Ctrl+T)"; break;
            }
        }
        return 0;
    }

    case WM_COMMAND: {
        int id  = LOWORD(wp);
        int evt = HIWORD(wp);

        // Search bar: rebuild filter on every keystroke
        if (id == IDC_SEARCH && evt == EN_CHANGE) {
            BuildFilter(); RefreshList(); UpdateStatus();
            return 0;
        }

        // Category sidebar selection
        if (id == IDC_CAT_LIST && evt == LBN_SELCHANGE) {
            int sel = (int)SendMessage(g_hCatList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                g_filterCat = sel;   // 0 = All, 1..N = category
                BuildFilter(); RefreshList(); UpdateStatus();
            }
            return 0;
        }

        switch (id) {

        // ── Add Transaction ─ modal dialog ① ───────────────────
        case IDM_TXN_ADD: case IDT_ADD:
            g_editId = -1;
            ShowAddEditDlg(hwnd);
            if (g_dlgAmount > 0.0) {
                AddTransaction(g_dlgAmount, g_dlgCategory, g_dlgDesc,
                               g_dlgDay, g_dlgMonth, g_dlgYear,
                               g_dlgType, g_dlgRecurring);
                g_dlgAmount = 0.0;  // prevent accidental double-add
                UpdateStatus();
                SaveCSV(g_autoSavePath);
                // Notify if goal reached
                if (g_bHasGoal && g_dBalance >= g_dGoalTarget)
                    MessageBox(hwnd, L"\u2605 You have reached your savings goal!",
                               L"Goal Achieved!", MB_OK|MB_ICONINFORMATION);
            }
            break;

        // ── Edit Transaction ─ modal dialog ② ──────────────────
        case IDM_TXN_EDIT: case IDT_EDIT: case VK_F2: {
            int sel = (int)SendMessage(g_hListBox, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR) {
                MessageBox(hwnd, L"Please select a transaction to edit.",
                           L"CashFlow", MB_OK|MB_ICONINFORMATION);
                break;
            }
            int txnIdx = (int)SendMessage(g_hListBox, LB_GETITEMDATA, sel, 0);
            if (txnIdx < 0 || txnIdx >= g_txnCount) break;

            // Populate staging vars from existing record
            Transaction& t = g_txns[txnIdx];
            g_editId       = t.id;
            g_dlgAmount    = t.amount;
            g_dlgDay       = t.day;  g_dlgMonth = t.month;  g_dlgYear = t.year;
            g_dlgType      = t.type;
            g_dlgRecurring = t.recurring;
            wcsncpy(g_dlgCategory, t.category,    63);
            wcsncpy(g_dlgDesc,     t.description, 199);

            ShowAddEditDlg(hwnd);

            if (g_dlgAmount > 0.0) {  // IDOK was pressed
                UpdateTransaction(g_editId, g_dlgAmount, g_dlgCategory, g_dlgDesc,
                                  g_dlgDay, g_dlgMonth, g_dlgYear,
                                  g_dlgType, g_dlgRecurring);
                UpdateStatus();
                SaveCSV(g_autoSavePath);
            }
            g_editId = -1;
            break;
        }

        // ── Delete Transaction ─ MessageBox confirmation ────────
        case IDM_TXN_DELETE: case IDT_DELETE:
            DeleteSelected();
            UpdateStatus();
            SaveCSV(g_autoSavePath);
            break;

        // ── Live Statistics ─ modeless dialog ③ ────────────────
        case IDM_VIEW_STATS: case IDT_STATS:
            ShowStatsDlg(hwnd);
            break;

        // ── Budget Goal ─ modal dialog ② ───────────────────────
        case IDM_TOOLS_GOAL: case IDT_GOAL: {
            INT_PTR wasSet = g_bHasGoal;
            ShowGoalDlg(hwnd);
            if (g_bHasGoal) {
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateStatsDlg();
            }
            break;
        }

        // ── ChooseColor ─ standard dialog ④ ────────────────────
        case IDM_TOOLS_COLOR:
            ShowColorDlg(hwnd);
            EnumChildWindows(hwnd, [](HWND hw, LPARAM) -> BOOL {
                InvalidateRect(hw, NULL, TRUE); return TRUE;
            }, 0);
            if (g_hStatsDlg) InvalidateRect(g_hStatsDlg, NULL, TRUE);
            break;

        // ── ChooseFont ─ standard dialog ⑤ ─────────────────────
        case IDM_TOOLS_FONT:
            ShowFontDlg(hwnd);
            InvalidateRect(g_hListBox, NULL, TRUE);
            break;

        // ── Import CSV ─ GetOpenFileName ⑥ ─────────────────────
        case IDM_FILE_IMPORT: {
            OPENFILENAME ofn = {};
            wchar_t file[MAX_PATH] = {};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner   = hwnd;
            ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files\0*.*\0";
            ofn.lpstrFile   = file;
            ofn.nMaxFile    = MAX_PATH;
            ofn.Flags       = OFN_FILEMUSTEXIST;
            ofn.lpstrTitle  = L"Import Transactions";
            if (GetOpenFileName(&ofn)) {
                g_txnCount = 0; g_nextId = 1;
                LoadCSV(file);
                BuildFilter(); RefreshList(); UpdateStatus();
                InvalidateRect(hwnd, NULL, FALSE);
                wchar_t msg[80];
                swprintf(msg,80,L"Imported %d transactions.", g_txnCount);
                MessageBox(hwnd, msg, L"Import Complete", MB_OK|MB_ICONINFORMATION);
            }
            break;
        }

        // ── Export CSV ─ GetSaveFileName ⑦ ─────────────────────
        case IDM_FILE_EXPORT: {
            OPENFILENAME ofn = {};
            wchar_t file[MAX_PATH] = L"cashflow_export.csv";
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner   = hwnd;
            ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files\0*.*\0";
            ofn.lpstrFile   = file;
            ofn.nMaxFile    = MAX_PATH;
            ofn.Flags       = OFN_OVERWRITEPROMPT;
            ofn.lpstrTitle  = L"Export Transactions";
            if (GetSaveFileName(&ofn)) {
                SaveCSV(file);
                MessageBox(hwnd, L"Transactions exported successfully.",
                           L"Export Complete", MB_OK|MB_ICONINFORMATION);
            }
            break;
        }

        case IDM_FILE_NEW:
            if (MessageBox(hwnd,
                    L"Clear all transactions and start fresh?\n"
                    L"The current data will be lost.",
                    L"New Budget", MB_YESNO|MB_ICONQUESTION) == IDYES)
            {
                g_txnCount=0; g_nextId=1;
                g_dTotalIncome=g_dTotalExpense=g_dBalance=0.0;
                g_bHasGoal=false;
                BuildFilter(); RefreshList(); UpdateStatus();
                InvalidateRect(hwnd, NULL, FALSE);
                SaveCSV(g_autoSavePath);
            }
            break;

        case IDM_FILE_EXIT:
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        // ── Theme toggle: cycles Dark → Light → Pink → Burgundy ─
        case IDM_VIEW_THEME: case IDT_THEME:
            ApplyTheme((g_iTheme + 1) % 4);
            ApplyThemeToControls();
            // Invalidate every child window so colours refresh immediately
            EnumChildWindows(hwnd, [](HWND hw, LPARAM) -> BOOL {
                InvalidateRect(hw, NULL, TRUE); return TRUE;
            }, 0);
            if (g_hStatsDlg) InvalidateRect(g_hStatsDlg, NULL, TRUE);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDM_VIEW_TOOLBAR:
            g_bShowToolbar = !g_bShowToolbar;
            ShowWindow(g_hToolbar, g_bShowToolbar ? SW_SHOW : SW_HIDE);
            { HMENU hm=GetMenu(hwnd); HMENU hv=GetSubMenu(hm,3);
              CheckMenuItem(hv,IDM_VIEW_TOOLBAR,g_bShowToolbar?MF_CHECKED:MF_UNCHECKED); }
            LayoutControls(hwnd); InvalidateRect(hwnd,NULL,TRUE);
            break;

        case IDM_VIEW_STATUS:
            g_bShowStatus = !g_bShowStatus;
            ShowWindow(g_hStatus, g_bShowStatus ? SW_SHOW : SW_HIDE);
            { HMENU hm=GetMenu(hwnd); HMENU hv=GetSubMenu(hm,3);
              CheckMenuItem(hv,IDM_VIEW_STATUS,g_bShowStatus?MF_CHECKED:MF_UNCHECKED); }
            LayoutControls(hwnd); InvalidateRect(hwnd,NULL,TRUE);
            break;

        case IDM_TOOLS_ABOUT:
            MessageBox(hwnd,
                L"CashFlow  \u2605  v1.0\n"
                L"Modern Personal Budget Tracker\n\n"
                L"Lab 5 \u2014 Dialog Panels (Win32 API)\n\n"
                L"Dialogs demonstrated:\n"
                L"  \u2460  Modal  \u2014  Add Transaction       (DialogBoxIndirect)\n"
                L"  \u2461  Modal  \u2014  Edit Transaction      (DialogBoxIndirect)\n"
                L"  \u2462  Modal  \u2014  Budget Goal setup     (DialogBoxIndirect)\n"
                L"  \u2463  Modeless \u2014 Live Statistics panel (CreateDialogIndirect)\n"
                L"  \u2464  Standard \u2014 ChooseColor           (accent colour)\n"
                L"  \u2465  Standard \u2014 ChooseFont           (list font)\n"
                L"  \u2466  Standard \u2014 GetOpenFileName      (import CSV)\n"
                L"  \u2467  Standard \u2014 GetSaveFileName      (export CSV)\n"
                L"  \u2468  All in-memory DLGTEMPLATE (no .rc file)\n\n"
                L"Themes (Ctrl+T to cycle):\n"
                L"  Dark Navy   \u2022  Light Indigo  \u2022  Pink Rose  \u2022  Dark Burgundy\n\n"
                L"Data saved automatically to cashflow_data.csv",
                L"About CashFlow \u2605", MB_OK|MB_ICONINFORMATION);
            break;
        }
        return 0;
    }

    // ── Keyboard shortcuts (WM_KEYDOWN on main window) ────────
    case WM_KEYDOWN:
        if (wp == VK_DELETE) {
            DeleteSelected(); UpdateStatus(); SaveCSV(g_autoSavePath);
        }
        if (wp == VK_F2) {
            SendMessage(hwnd, WM_COMMAND, IDM_TXN_EDIT, 0);
        }
        return 0;

    case WM_TIMER:
        if (wp == TIMER_CLOCK) {
            time_t t = time(NULL);
            struct tm* ti = localtime(&t);
            wchar_t s[80];
            swprintf(s,80,L"  CashFlow  \u2665  %02d:%02d:%02d",
                     ti->tm_hour, ti->tm_min, ti->tm_sec);
            SendMessage(g_hStatus, SB_SETTEXT, 2, (LPARAM)s);
        }
        return 0;

    case WM_CLOSE:
        // Auto-save before exit
        SaveCSV(g_autoSavePath);
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_CLOCK);
        if (g_hStatsDlg) { DestroyWindow(g_hStatsDlg); g_hStatsDlg = NULL; }
        // Clean up fonts
        if (g_hFontUI)     DeleteObject(g_hFontUI);
        if (g_hFontBold)   DeleteObject(g_hFontBold);
        if (g_hFontSmall)  DeleteObject(g_hFontSmall);
        if (g_hFontAmount) DeleteObject(g_hFontAmount);
        if (g_hFontList)   DeleteObject(g_hFontList);
        // Clean up brushes
        if (g_hBrushBg)      DeleteObject(g_hBrushBg);
        if (g_hBrushPanel)   DeleteObject(g_hBrushPanel);
        if (g_hBrushHeader)  DeleteObject(g_hBrushHeader);
        if (g_hBrushAccent)  DeleteObject(g_hBrushAccent);
        if (g_hBrushIncome)  DeleteObject(g_hBrushIncome);
        if (g_hBrushExpense) DeleteObject(g_hBrushExpense);
        if (g_hBrushAlt)     DeleteObject(g_hBrushAlt);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// =============================================================
//  WinMain
//  Message loop routes modeless dialog messages first,
//  then falls through to main window IsDialogMessage for
//  keyboard navigation in the main window.
// =============================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    g_hInst = hInst;

    INITCOMMONCONTROLSEX icc = { sizeof(icc),
        ICC_WIN95_CLASSES | ICC_BAR_CLASSES |
        ICC_COOL_CLASSES  | ICC_USEREX_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEX wc     = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc    = WndProc;
    wc.hInstance      = hInst;
    wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName  = L"CashFlowWnd";
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wc);

    g_hMain = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"CashFlowWnd",
        L"CashFlow  \u2605  Personal Budget Tracker",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1160, 760,
        NULL, NULL, hInst, NULL);

    ShowWindow(g_hMain, nCmdShow);
    UpdateWindow(g_hMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        // Lab 5: modeless statistics panel gets first chance at keyboard input
        if (g_hStatsDlg && IsDialogMessage(g_hStatsDlg, &msg)) continue;
        if (!IsDialogMessage(g_hMain, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}
