#include <windows.h>
#include <math.h>

bool g_isTileActive = false;

char g_inputText[256] = "Type here: ";
int g_textLength = 11;
int g_caretX = 420;
int g_caretY = 420;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;

    wcex.hCursor = NULL;

    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = "GdiTilesClass";

    if (!RegisterClassEx(&wcex)) return 1;

    HWND hWnd = CreateWindowEx(0, "GdiTilesClass", "GDI Pattern Tiles + Lab 4",
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 816, 839, NULL, NULL, hInstance, NULL);

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HBRUSH hNavyBrush   = CreateSolidBrush(RGB(15, 25, 50));     //CreateSolidBrush
        HBRUSH hGoldBrush   = CreateSolidBrush(RGB(255, 215, 0));
        HBRUSH hTealBrush   = CreateSolidBrush(RGB(0, 150, 150));
        HBRUSH hCrimsonBrush= CreateSolidBrush(RGB(200, 30, 50));
        HBRUSH hStockGray   = (HBRUSH)GetStockObject(DKGRAY_BRUSH);  // GetStockObject

        HPEN hGoldPen       = CreatePen(PS_SOLID, 2, RGB(255, 215, 0));
        HPEN hThickPen      = CreatePen(PS_SOLID, 4, RGB(255, 255, 255));
        HPEN hTealPen       = CreatePen(PS_SOLID, 2, RGB(0, 200, 200));
        HPEN hNullPen       = CreatePen(PS_NULL, 0, 0);

        HPEN hOldPen        = (HPEN)SelectObject(hdc, hNullPen);
        HBRUSH hOldBrush    = (HBRUSH)SelectObject(hdc, hNavyBrush);

        Rectangle(hdc, -10, -10, 850, 850);

        // ==========================================
        // TILE 1: [0, 0, 400, 400]
        // ==========================================
        RECT rectTile1 = { 0, 0, 400, 400 };
        FillRect(hdc, &rectTile1, hNavyBrush);                       // FillRect

        SelectObject(hdc, hTealBrush);
        Pie(hdc, -40, -40, 140, 140, 100, 50, 50, 100);              //Pie
        Pie(hdc, 260, 260, 440, 440, 300, 350, 350, 300);
        SelectObject(hdc, hCrimsonBrush);
        Chord(hdc, 260, -40, 440, 140, 350, 100, 300, 50);           //  Chord
        Chord(hdc, -40, 260, 140, 440, 50, 300, 100, 350);

        SelectObject(hdc, hGoldPen);
        HBRUSH hHatchBrush = CreateHatchBrush(HS_DIAGCROSS, RGB(0, 120, 120)); //  CreateHatchBrush
        SelectObject(hdc, hHatchBrush);
        Ellipse(hdc, 40, 40, 360, 360);                              // Ellipse

        SelectObject(hdc, hNavyBrush);
        Ellipse(hdc, 80, 80, 320, 320);

        POINT polyPoints[] = { {200, 100}, {270, 200}, {200, 300}, {130, 200}, {130, 130}, {270, 130}, {270, 270}, {130, 270} };
        int ppCounts[] = { 4, 4 };
        SelectObject(hdc, hStockGray);
        SelectObject(hdc, hNullPen);
        PolyPolygon(hdc, polyPoints, ppCounts, 2);                   //PolyPolygon

        SelectObject(hdc, hGoldBrush);

        // ALTERNATE
        POINT starAlt[5] = { {130, 180}, {170, 280}, {90, 220}, {170, 220}, {90, 280} };
        SetPolyFillMode(hdc, ALTERNATE);                             //  SetPolyFillMode
        Polygon(hdc, starAlt, 5);                                    // Polygon

        // WINDING
        POINT starWin[5] = { {270, 180}, {310, 280}, {230, 220}, {310, 220}, {230, 280} };
        SetPolyFillMode(hdc, WINDING);
        Polygon(hdc, starWin, 5);

        // ==========================================
        // TILE 2: [400, 0, 800, 400]
        // ==========================================
        SelectObject(hdc, hStockGray);
        SelectObject(hdc, hNullPen);
        Rectangle(hdc, 400, 0, 800, 400);                            //  Rectangle

        HRGN hRgnTop    = CreateEllipticRgn(550, 50, 650, 150);      // CreateEllipticRgn
        HRGN hRgnCenter = CreateRectRgn(500, 100, 700, 300);         // CreateRectRgn

        POINT polyPts[] = { {450, 200}, {500, 150}, {500, 250} };
        HRGN hRgnPoly   = CreatePolygonRgn(polyPts, 3, WINDING);     // CreatePolygonRgn

        HRGN hFlower = CreateRectRgn(0, 0, 1, 1);
        CombineRgn(hFlower, hRgnTop, hRgnCenter, RGN_OR);            // CombineRgn
        CombineRgn(hFlower, hFlower, hRgnPoly, RGN_OR);

        FillRgn(hdc, hFlower, hNavyBrush);                           // FillRgn

        HBRUSH hBorderBrush = CreateSolidBrush(RGB(0, 255, 200));
        FrameRgn(hdc, hFlower, hBorderBrush, 4, 4);                  //  FrameRgn

        SelectObject(hdc, hGoldBrush);
        HRGN hCenterGem = CreateEllipticRgn(570, 170, 630, 230);
        PaintRgn(hdc, hCenterGem);                                   // PaintRgn

        HRGN hCore = CreateRectRgn(590, 190, 610, 210);
        InvertRgn(hdc, hCore);                                       // InvertRgn

        // ==========================================
        // TILE 3: [0, 400, 400, 800]
        // ==========================================
        SelectObject(hdc, hTealBrush);
        Rectangle(hdc, 0, 400, 400, 800);

        SelectObject(hdc, hGoldPen);
        BeginPath(hdc);                                              // BeginPath
        for(int i = 20; i <= 380; i += 40) {
            MoveToEx(hdc, i, 420, NULL);                             // MoveToEx
            POINT wave[] = { {i + 30, 500}, {i - 30, 700}, {i, 780} };
            PolyBezierTo(hdc, wave, 3);                              //  PolyBezierTo
        }
        EndPath(hdc);                                                //  EndPath
        StrokePath(hdc);                                             //  StrokePath

        SelectObject(hdc, hNavyBrush);
        SelectObject(hdc, hNullPen);
        BeginPath(hdc);
        MoveToEx(hdc, 200, 750, NULL);
        POINT leftWing[] = { {150, 750}, {50, 650}, {100, 550}, {150, 600}, {180, 500}, {200, 600} };
        PolyBezierTo(hdc, leftWing, 6);
        CloseFigure(hdc);                                            //  CloseFigure

        MoveToEx(hdc, 200, 750, NULL);
        POINT rightWing[] = { {250, 750}, {350, 650}, {300, 550}, {250, 600}, {220, 500}, {200, 600} };
        PolyBezierTo(hdc, rightWing, 6);
        CloseFigure(hdc);
        EndPath(hdc);
        FillPath(hdc);                                               // FillPath

        SelectObject(hdc, hThickPen);
        SelectObject(hdc, hCrimsonBrush);
        BeginPath(hdc);
        MoveToEx(hdc, 200, 450, NULL);
        POINT spire[] = { {240, 500}, {240, 700}, {200, 780}, {160, 700}, {160, 500}, {200, 450} };
        for(int i = 0; i < 6; i++) LineTo(hdc, spire[i].x, spire[i].y); // LineTo
        CloseFigure(hdc);
        EndPath(hdc);
        StrokeAndFillPath(hdc);                                      // StrokeAndFillPath

        BeginPath(hdc);
        MoveToEx(hdc, 200, 530, NULL);
        LineTo(hdc, 230, 600);
        LineTo(hdc, 200, 670);
        LineTo(hdc, 170, 600);
        CloseFigure(hdc);
        EndPath(hdc);
        HRGN hPathRgn = PathToRegion(hdc);                           // PathToRegion
        FillRgn(hdc, hPathRgn, hGoldBrush);

        // ==========================================
        // TILE 4 (Text Zone): [400, 400, 800, 800]
        // ==========================================
        RECT rectTile4 = { 400, 400, 800, 800 };
        FillRect(hdc, &rectTile4, hNavyBrush);

        RECT frame4 = { 410, 410, 790, 790 };
        SelectObject(hdc, hBorderBrush);
        FrameRect(hdc, &frame4, hBorderBrush);                       //  FrameRect

        BeginPath(hdc);
        RoundRect(hdc, 500, 450, 700, 750, 100, 100);                // RoundRect
        EndPath(hdc);
        SelectClipPath(hdc, RGN_COPY);                               // SelectClipPath

        SelectObject(hdc, hGoldPen);
        for (int i = 400; i < 800; i += 15) {
            MoveToEx(hdc, 600, 600, NULL); LineTo(hdc, i, 400);
            MoveToEx(hdc, 600, 600, NULL); LineTo(hdc, i, 800);
        }

        SelectClipRgn(hdc, NULL);                                    //SelectClipRgn

        SelectObject(hdc, hThickPen);
        SelectObject(hdc, hNullPen);
        Ellipse(hdc, 550, 550, 650, 650);

        if (g_isTileActive) {
            RECT innerRect = { 575, 575, 625, 625 };
            InvertRect(hdc, &innerRect);                             // InvertRect
        }

        //Display text in the fourth tile
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOut(hdc, 420, 420, g_inputText, g_textLength);

        // Calculate exact caret position after the text
        SIZE textSize;
        GetTextExtentPoint32(hdc, g_inputText, g_textLength, &textSize);
        g_caretX = 420 + textSize.cx;
        SetCaretPos(g_caretX, g_caretY);

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hNavyBrush); DeleteObject(hGoldBrush); DeleteObject(hTealBrush);
        DeleteObject(hCrimsonBrush); DeleteObject(hStockGray); DeleteObject(hHatchBrush);
        DeleteObject(hBorderBrush);
        DeleteObject(hGoldPen); DeleteObject(hThickPen); DeleteObject(hTealPen); DeleteObject(hNullPen);
        DeleteObject(hRgnTop); DeleteObject(hRgnCenter); DeleteObject(hRgnPoly);
        DeleteObject(hFlower); DeleteObject(hCenterGem); DeleteObject(hCore);
        DeleteObject(hPathRgn);

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_MOUSEMOVE:
    {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);

        if (xPos < 400 && yPos < 400) {
            SetCursor(LoadCursor(NULL, IDC_CROSS));
        } else if (xPos >= 400 && yPos < 400) {
            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        } else if (xPos < 400 && yPos >= 400) {
            SetCursor(LoadCursor(NULL, IDC_UPARROW));
        } else {
            SetCursor(LoadCursor(NULL, IDC_IBEAM));
        }
    }
    break;

    case WM_SETFOCUS:
        CreateCaret(hWnd, NULL, 2, 16);
        SetCaretPos(g_caretX, g_caretY);
        ShowCaret(hWnd);
        break;

    case WM_KILLFOCUS:
        HideCaret(hWnd);
        DestroyCaret();
        break;

    case WM_CHAR:
    {
        if (wParam == VK_BACK) {
            if (g_textLength > 0) {
                g_textLength--;
                g_inputText[g_textLength] = '\0';
            }
        }

        else if (wParam >= 32 && wParam <= 126 && g_textLength < 255) {
            g_inputText[g_textLength] = (char)wParam;
            g_textLength++;
            g_inputText[g_textLength] = '\0';
        }

        RECT textRect = { 420, 420, 800, 450 };
        InvalidateRect(hWnd, &textRect, TRUE);
    }
    break;

    case WM_LBUTTONDOWN:
    {
        // Set focus to the window on click (required for keyboard input)
        SetFocus(hWnd);

        g_isTileActive = !g_isTileActive;

        HRGN hTile4Rgn = CreateRectRgn(400, 400, 800, 800);

        HRGN hTile1Rgn = CreateRectRgn(0, 0, 400, 400);
        HRGN hTile2Rgn = CreateRectRgn(400, 0, 800, 400);
        HRGN hTile3Rgn = CreateRectRgn(0, 400, 400, 800);

        HRGN hProtectRgn = CreateRectRgn(0, 0, 1, 1);
        CombineRgn(hProtectRgn, hTile1Rgn, hTile2Rgn, RGN_OR);
        CombineRgn(hProtectRgn, hProtectRgn, hTile3Rgn, RGN_OR);

        InvalidateRgn(hWnd, hTile4Rgn, FALSE);                       //  InvalidateRgn
        ValidateRgn(hWnd, hProtectRgn);                              //  ValidateRgn

        DeleteObject(hTile4Rgn); DeleteObject(hTile1Rgn);
        DeleteObject(hTile2Rgn); DeleteObject(hTile3Rgn); DeleteObject(hProtectRgn);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}