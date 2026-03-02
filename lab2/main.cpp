#include <windows.h>
#include <tchar.h>
#include <cmath>

const TCHAR szClassName[] = _T("Lab2GDIClass");
const TCHAR szWindowTitle[] = _T("Lab 2: GDI Graphics Primitives");

POINT g_ClickPoint = { -1, -1 };
bool g_bClicked = false;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawRedDotInCenter(HDC hdc, HWND hwnd);
void DrawClickPoint(HDC hdc);
void DrawPolylines(HDC hdc, HWND hwnd);
void DrawCurvesAndBezier(HDC hdc, HWND hwnd);
void DrawWithDifferentStyles(HDC hdc, HWND hwnd);
void DrawOnDifferentBackgrounds(HDC hdc, HWND hwnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szClassName;

    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL, _T("Window Registration Failed!"), _T("Error"), MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        0, szClassName, szWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 750,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            DrawRedDotInCenter(hdc, hwnd);
            DrawClickPoint(hdc);
            DrawPolylines(hdc, hwnd);
            DrawCurvesAndBezier(hdc, hwnd);
            DrawWithDifferentStyles(hdc, hwnd);
            DrawOnDifferentBackgrounds(hdc, hwnd);

            EndPaint(hwnd, &ps);
        } break;

        case WM_LBUTTONDOWN: {
            // 2
            g_ClickPoint.x = LOWORD(lParam);
            g_ClickPoint.y = HIWORD(lParam);
            g_bClicked = true;
            InvalidateRect(hwnd, NULL, TRUE);
        } break;

        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 1
void DrawRedDotInCenter(HDC hdc, HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    int centerX = (rect.right - rect.left) / 2;
    int centerY = (rect.bottom - rect.top) / 2;

    HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));
    // Save the previously selected brush so we can restore it later,
    // preventing resource leaks and unintended style changes.
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    Ellipse(hdc, centerX - 5, centerY - 5, centerX + 5, centerY + 5);
    SetPixel(hdc, centerX, centerY, RGB(0, 0, 0));

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOut(hdc, centerX + 10, centerY - 10, _T("Task 1: Center dot"), 18);
}

// 2
void DrawClickPoint(HDC hdc) {
    if (g_bClicked) {
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 255));
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        Ellipse(hdc, g_ClickPoint.x - 5, g_ClickPoint.y - 5, g_ClickPoint.x + 5, g_ClickPoint.y + 5);
        SetPixel(hdc, g_ClickPoint.x, g_ClickPoint.y, RGB(255, 255, 255));

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hBrush);
        DeleteObject(hPen);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 255));
        TextOut(hdc, g_ClickPoint.x + 10, g_ClickPoint.y - 10, _T("Task 2: Clicked dot"), 19);
    }
}

// Task 3: Polylines — Mountain Landscape
// Demonstrates three related GDI line-drawing functions:
//   • LineTo      — draws a single straight segment from the current position
//                   to the given endpoint, then updates the current position.
//                   Used here for snow caps, house walls, roof, door and river banks.
//   • Polyline    — draws a connected sequence of straight line segments through
//                   an array of POINT structures in a single call. Unlike LineTo it
//                   does NOT update the DC's current position. Used here for the
//                   main mountain silhouette and river bank edges.
//   • PolyPolyline — draws multiple independent polylines in one call by accepting
//                   a flat POINT array, an array of per-polyline vertex counts, and
//                   the total number of polylines. Ideal for drawing many disconnected
//                   shapes (e.g., the layered tree triangles) without repeated
//                   SelectObject/MoveToEx calls.
void DrawPolylines(HDC hdc, HWND hwnd) {
    HPEN hPenMountain = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));

    HPEN hPenSnow = CreatePen(PS_SOLID, 2, RGB(200, 200, 220));

    HPEN hPenTree = CreatePen(PS_SOLID, 2, RGB(34, 139, 34));

    HPEN hPenHouse = CreatePen(PS_SOLID, 2, RGB(139, 69, 19));

    HPEN hPenRiver = CreatePen(PS_SOLID, 2, RGB(65, 105, 225));

    HPEN hOldPen = (HPEN)SelectObject(hdc, hPenMountain);

    POINT mountains[] = {
        {50, 250}, {120, 100}, {180, 190}, {250, 70},
        {320, 140}, {400, 50}, {470, 180}, {530, 120}, {600, 250}
    };
    Polyline(hdc, mountains, 9);

    SelectObject(hdc, hPenSnow);


    // --- LineTo: snow caps on three peaks ---
    // MoveToEx repositions the DC's current point without drawing.
    // Each subsequent LineTo(hdc, x, y) draws a segment from the current
    // position to (x, y) and moves the current position there.
    // Three V-shapes trace snow coverage just below each summit.
    // Перша гора (пік 120, 100)
    MoveToEx(hdc, 101, 140, NULL); LineTo(hdc, 120, 100); LineTo(hdc, 147, 140);
    // Друга гора (пік 250, 70)
    MoveToEx(hdc, 227, 110, NULL); LineTo(hdc, 250, 70);  LineTo(hdc, 290, 110);
    // Третя гора (пік 400, 50)
    MoveToEx(hdc, 364, 90, NULL);  LineTo(hdc, 400, 50);  LineTo(hdc, 422, 90);

    SelectObject(hdc, hPenTree);

    // --- PolyPolyline: layered tree triangles ---
    // All vertices for both trees (left and right) are stored in one flat array.
    // treeCounts specifies how many vertices belong to each sub-polyline:
    //   4 vertices → base triangle layer
    //   4 vertices → middle triangle layer
    //   4 vertices → top triangle layer
    //   2 vertices → trunk (vertical segment)
    // The last argument (8) is the total number of sub-polylines.
    // PolyPolyline draws each sub-polyline independently — no lines connect them.
    POINT trees[] = {
        {100, 250}, {120, 210}, {140, 250}, {100, 250},
        {105, 210}, {120, 170}, {135, 210}, {105, 210},
        {110, 170}, {120, 140}, {130, 170}, {110, 170},
        {120, 250}, {120, 270},
        {450, 250}, {470, 210}, {490, 250}, {450, 250},
        {455, 210}, {470, 170}, {485, 210}, {455, 210},
        {460, 170}, {470, 140}, {480, 170}, {460, 170},
        {470, 250}, {470, 270}
    };
    DWORD treeCounts[] = {4, 4, 4, 2,   4, 4, 4, 2};
    PolyPolyline(hdc, trees, treeCounts, 8);

    SelectObject(hdc, hPenHouse);

    // --- LineTo: house walls, roof, and door ---
    // Three groups of LineTo sequences build:
    //   1. Rectangular house body (closed quadrilateral).
    //   2. Triangular roof above it.
    //   3. Rectangular door recess.
    MoveToEx(hdc, 270, 250, NULL); LineTo(hdc, 270, 200); LineTo(hdc, 330, 200); LineTo(hdc, 330, 250); LineTo(hdc, 270, 250);
    MoveToEx(hdc, 260, 200, NULL); LineTo(hdc, 300, 160); LineTo(hdc, 340, 200); LineTo(hdc, 260, 200);
    MoveToEx(hdc, 290, 250, NULL); LineTo(hdc, 290, 220); LineTo(hdc, 310, 220); LineTo(hdc, 310, 250);

    SelectObject(hdc, hPenRiver);

    // --- Polyline: left and right river banks ---
    // Two separate Polyline calls draw the left and right edges of the river.
    // Using Polyline (rather than LineTo) keeps the DC current position unchanged,
    // which is preferable when the next drawing step uses MoveToEx anyway.
    POINT riverLeft[]  = { {250, 250}, {230, 280}, {260, 310}, {210, 350} };
    Polyline(hdc, riverLeft, 4);
    POINT riverRight[] = { {270, 250}, {250, 280}, {280, 310}, {240, 350} };
    Polyline(hdc, riverRight, 4);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPenMountain); DeleteObject(hPenSnow); DeleteObject(hPenTree);
    DeleteObject(hPenHouse);    DeleteObject(hPenRiver);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOut(hdc, 50, 20, _T("Task 3: Landscape (LineTo, Polyline, PolyPolyline)"), 50);
}

// Task 4: Curves — Arc and PolyBezier
// Demonstrates two GDI curve-drawing functions:
//   • Arc        — draws an elliptical arc (a portion of an ellipse outline).
//                  The arc is defined by a bounding rectangle, a start radial
//                  point and an end radial point. GDI draws counter-clockwise
//                  by default (unless SetArcDirection is used). No fill is applied.
//   • PolyBezier — draws one or more cubic Bézier curves through a flat POINT array.
//                  The first curve requires 4 points (start + 2 control + end).
//                  Each additional curve shares the previous end-point and adds
//                  3 more points (2 control + 1 end). Total count = 1 + 3*n segments.
//                  Control points shape the curvature without being passed through.
void DrawCurvesAndBezier(HDC hdc, HWND hwnd) {
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(128, 0, 128));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    // --- Arc ---
    // Arc(hdc, left, top, right, bottom, xStart, yStart, xEnd, yEnd)
    //   Bounding rectangle: (50,350)–(150,430) — defines the ellipse shape.
    //   Start radial: (140, 390) — rightmost midpoint, arc starts here.
    //   End radial:   (60,  390) — leftmost midpoint, arc ends here.
    // The result is the upper half of the ellipse, drawn counter-clockwise.
    Arc(hdc, 50, 350, 150, 430, 140, 390, 60, 390);

    // --- PolyBezier ---
    // 7 points define 2 cubic Bézier segments (1 + 3*2 = 7):
    //   Segment 1: points[0]–[3]  → anchor, ctrl1, ctrl2, anchor
    //   Segment 2: points[3]–[6]  → (shared anchor), ctrl1, ctrl2, anchor
    // The S-like wave is achieved by alternating control points above/below
    // the baseline, producing smooth inflection transitions.
    POINT bezier[] = {
        {180, 400}, {220, 350}, {260, 450}, {300, 400},
        {340, 350}, {380, 450}, {420, 400}
    };
    PolyBezier(hdc, bezier, 7);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    SetTextColor(hdc, RGB(128, 0, 128));
    TextOut(hdc, 50, 320, _T("Task 4: Arc & PolyBezier"), 24);
}

//5
void DrawWithDifferentStyles(HDC hdc, HWND hwnd) {
    int startY = 530;
    int spacing = 30;

    struct LineStyle { int style; int width; COLORREF color; const TCHAR* name; };
    LineStyle styles[] = {
        { PS_SOLID, 3, RGB(255, 0, 0), _T("PS_SOLID (3px)") },
        { PS_DASH, 1, RGB(0, 128, 255), _T("PS_DASH (1px)") },
        { PS_DOT, 1, RGB(255, 128, 0), _T("PS_DOT (1px)") },
        { PS_DASHDOT, 1, RGB(0, 200, 0), _T("PS_DASHDOT (1px)") },
        { PS_DASHDOTDOT, 1, RGB(128, 0, 128), _T("PS_DASHDOTDOT (1px)") }
    };

    for (int i = 0; i < 5; ++i) {
        HPEN hPen = CreatePen(styles[i].style, styles[i].width, styles[i].color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        MoveToEx(hdc, 50, startY + (i * spacing), NULL);
        LineTo(hdc, 250, startY + (i * spacing));

        SetTextColor(hdc, styles[i].color);
        TextOut(hdc, 260, startY + (i * spacing) - 8, styles[i].name, _tcslen(styles[i].name));

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }
    TextOut(hdc, 50, startY - 25, _T("Task 5: Line Styles"), 19);
}

// 6,7
void DrawOnDifferentBackgrounds(HDC hdc, HWND hwnd) {
    int oldROP = SetROP2(hdc, R2_COPYPEN);
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOut(hdc, 650, 30, _T("Task 6: Drawing Modes (ROP2)"), 28);

    RECT whiteRect = { 650, 60, 850, 140 };
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &whiteRect, whiteBrush);
    DeleteObject(whiteBrush);

    // Solid red pen, 5px wide, drawn with R2_COPYPEN mode.
    // R2_COPYPEN simply copies the pen color to the destination pixels —
    // the most straightforward drawing mode. On a white background the red
    // line is rendered exactly as specified.
    HPEN hPen1 = CreatePen(PS_SOLID, 5, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen1);
    SetROP2(hdc, R2_COPYPEN);
    MoveToEx(hdc, 630, 100, NULL); LineTo(hdc, 870, 100);
    TextOut(hdc, 660, 65, _T("R2_COPYPEN (White BG)"), 21);

    RECT blackRect = { 650, 160, 850, 240 };
    // Solid black brush fills the background rectangle for the R2_XORPEN demo.
    // The black background makes the XOR effect clearly visible: XOR-ing the
    // green pen (0,255,0) with black (0,0,0) yields green, while XOR-ing with
    // white (255,255,255) would yield magenta — demonstrating the ROP2 behavior.
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &blackRect, blackBrush);
    DeleteObject(blackBrush);

    // Solid green pen, 5px wide, drawn with R2_XORPEN mode.
    // R2_XORPEN XORs the pen color bits with the destination pixel bits.
    // This is classically used for rubber-band drawing and highlighting because
    // drawing the same line twice restores the original background exactly.
    HPEN hPen2 = CreatePen(PS_SOLID, 5, RGB(0, 255, 0));
    SelectObject(hdc, hPen2);
    SetROP2(hdc, R2_XORPEN);
    MoveToEx(hdc, 630, 200, NULL); LineTo(hdc, 870, 200);
    SetTextColor(hdc, RGB(255, 255, 255));
    TextOut(hdc, 660, 165, _T("R2_XORPEN (Black BG)"), 20);

    SetROP2(hdc, oldROP);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen1);
    DeleteObject(hPen2);

    HDC hdcScreen = GetDC(NULL);

    // Solid magenta pen used to draw a decorative frame around the application
    // window directly on the screen DC (hdcScreen). Drawing on the screen DC
    // (GetDC(NULL)) makes the rectangle visible on the desktop behind the window.
    HPEN hScreenPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 255));
    HPEN hOldScreenPen = (HPEN)SelectObject(hdcScreen, hScreenPen);

    // NULL_BRUSH (stock object) sets a transparent fill for the Rectangle call below,
    // so only the outline is drawn without painting over desktop content inside the frame.
    HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH hOldBrush  = (HBRUSH)SelectObject(hdcScreen, hNullBrush);

    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    Rectangle(hdcScreen, windowRect.left - 5, windowRect.top - 5, windowRect.right + 5, windowRect.bottom + 5);

    SelectObject(hdcScreen, hOldScreenPen); SelectObject(hdcScreen, hOldBrush);
    DeleteObject(hScreenPen); ReleaseDC(NULL, hdcScreen);

    SetTextColor(hdc, RGB(255, 0, 255));
    TextOut(hdc, 650, 280, _T("Task 7: Look at desktop (Magenta frame)"), 39);
}