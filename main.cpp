#include <windows.h>
#include <tchar.h>

// -------------------------------------------------------------------------
// Global Constants and Variables
// -------------------------------------------------------------------------
constexpr UINT IDT_FADE_TIMER = 1001; // IDT = "ID for Timer" (an app-chosen number to identify this timer)
const TCHAR szClassName[] = _T("SeniorWin32LabClass"); // TCHAR/_T: text that can be ANSI or Unicode depending on project settings
const TCHAR szWindowTitle[] = _T("Lab 1: Advanced Win32 API Demonstration");

BYTE g_Alpha = 230;             // Alpha/transparency (0 = fully invisible, 255 = fully opaque)
bool g_IsFading = false;        // Flag: are we currently running the fade-out animation?
COLORREF g_CurrentBgColor = RGB(200, 200, 255); // COLORREF = Windows color value, RGB(r,g,b) builds it

// WndProc = "Window Procedure": Windows calls this function for each window message/event.
// LRESULT  = "Long RESULT": the return type expected by Windows for message handling (success/error/info).
// CALLBACK = calling convention macro (tells the compiler how parameters are passed on the stack/registers for WinAPI calls).
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void DynamicallyChangeWindowProperties(HWND hwnd);

// -------------------------------------------------------------------------
// Entry Point (Task 1: Win32 GUI Project creation)
// -------------------------------------------------------------------------
// WinMain = the starting function for a classic Windows GUI app (no console window).
// HINSTANCE = "handle to instance" (an ID for this running program).
// nCmdShow tells how the window should be shown (normal/minimized/maximized).
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    WNDCLASSEX wcex;                // Describes a "window class" (rules/appearance shared by windows of this type)
    wcex.cbSize = sizeof(WNDCLASSEX); // Must be set so Windows knows which struct version we are using

    // Task 3: Window class style (redraw on resize, drop shadow)
    // CS_* = "Class Style" flags (how the window class behaves).
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW; //horizontal, vertical redraw

    wcex.lpfnWndProc = WndProc;     // Pointer to our message handler function (WndProc)
    wcex.cbClsExtra = 0;            // Extra bytes reserved for the class (not used here)
    wcex.cbWndExtra = 0;            // Extra bytes reserved per window instance (not used here)
    wcex.hInstance = hInstance;     // Program instance that owns this class

    // Task 4: Change initial icon, cursor, and background
    // LoadIcon/LoadCursor with NULL uses standard built-in resources (IDI_*/IDC_*).
    wcex.hIcon = LoadIcon(NULL, IDI_WARNING);                  // Big icon (e.g., shown in Alt+Tab)
    wcex.hCursor = LoadCursor(NULL, IDC_CROSS);               // Mouse cursor when over the client area
    wcex.hbrBackground = CreateSolidBrush(g_CurrentBgColor);  // Brush used to paint background (solid color fill)

    wcex.lpszMenuName = NULL;          // No menu resource
    wcex.lpszClassName = szClassName;  // Name of this window class (used later in CreateWindowEx)
    wcex.hIconSm = LoadIcon(NULL, IDI_INFORMATION); // Small icon (e.g., title bar)

    // RegisterClassEx tells Windows about our window class so it can create windows of this type.
    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL, _T("Window Registration Failed!"), _T("Critical Error"), MB_ICONERROR | MB_OK);
        return 0;
    }

    // Task 5 & 11 & 12: Initial size/position, Always on top, Layered
    // HWND = "handle to window" (a unique ID used to refer to the created window).
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,   // WS_EX_* = "extended window styles": topmost + supports per-window transparency
        szClassName,
        szWindowTitle,
        WS_OVERLAPPEDWINDOW,             // Standard resizable window with title bar/borders
        100, 100, 800, 600,              // Initial X, Y, Width, Height (screen coordinates, pixels)
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, _T("Window Creation Failed!"), _T("Critical Error"), MB_ICONERROR | MB_OK);
        return 0;
    }

    // SetLayeredWindowAttributes applies transparency to a WS_EX_LAYERED window.
    // LWA_ALPHA means "use the alpha value" (not color-key transparency).
    SetLayeredWindowAttributes(hwnd, 0, g_Alpha, LWA_ALPHA);

    // Task 6: Initial view state (show the window on screen)
    ShowWindow(hwnd, SW_SHOWNORMAL); // SW_* = "Show Window" command (normal state here)
    UpdateWindow(hwnd);             // Forces an initial paint right away

    // Message Loop: pulls events/messages from Windows and sends them to WndProc.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) { // GetMessage blocks until a message arrives; returns 0 when quitting
        TranslateMessage(&msg);                // Converts key presses into character messages (WM_CHAR)
        DispatchMessage(&msg);                 // Sends the message to WndProc
        //WndProc stands for Window Procedure. It’s a callback function that Windows calls
        //whenever something happens to the window.
    }

    return static_cast<int>(msg.wParam); // Exit code for the process
}

// -------------------------------------------------------------------------
// Main Window Procedure
// -------------------------------------------------------------------------
// LRESULT/CALLBACK explained above.
//It’s the function header (declaration/signature) for the Window Procedure — the function
//Windows calls to deliver events/messages to your window.
// Parameters (quick meaning):
//   HWND   hwnd  : handle/ID of the window receiving the message
//   UINT   msg   : message ID (what happened)
//   WPARAM wParam: extra info #1 (meaning depends on msg)
//   LPARAM lParam: extra info #2 (meaning depends on msg)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

        case WM_PAINT: { // WM_PAINT: Windows is asking us to draw the window's client area
            // Drawing opaque objects on a transparent (layered) window background
            PAINTSTRUCT ps;                 // Paint information filled by BeginPaint
            HDC hdc = BeginPaint(hwnd, &ps); // HDC = "handle to device context" (a drawing surface)

            // Draw a solid red rectangle.
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 100, 100)); // Brush = fill style used for shapes
            RECT rect = { 50, 50, 450, 150 };                     // Rectangle coordinates inside the window
            FillRect(hdc, &rect, hBrush);                         // Paint the rectangle
            DeleteObject(hBrush);                                 // Free GDI object to avoid resource leaks

            SetBkMode(hdc, TRANSPARENT);              // Text background is transparent (doesn't paint a box behind text)
            SetTextColor(hdc, RGB(255, 255, 255));    // White text
            TextOut(hdc, 70, 80, _T("LEFT CLICK: Show Custom MessageBox"), 34);
            TextOut(hdc, 70, 100, _T("RIGHT CLICK: Change Window Properties Dynamically"), 49);
            TextOut(hdc, 70, 120, _T("CLOSE WINDOW: Trigger Fade-Out Animation"), 40);

            EndPaint(hwnd, &ps); // Must match BeginPaint; tells Windows painting is done
        } break;

        case WM_LBUTTONDOWN: { // Left mouse button pressed inside the window
            MessageBox(hwnd,
                _T("Left mouse button was pressed!\n\nDid you expect this?"),
                _T("Mouse Event Registered"),
                MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON1); // MB_* = MessageBox options (icons/buttons/default)
        } break;

        case WM_RBUTTONDOWN: { // Right mouse button pressed
            DynamicallyChangeWindowProperties(hwnd);
        } break;

        case WM_CLOSE: { // User clicked the X button (request to close)
            // Instead of closing instantly, start a timer-driven fade-out animation.
            if (!g_IsFading) {
                g_IsFading = true;
                SetTimer(hwnd, IDT_FADE_TIMER, 20, NULL); // 20ms tick; WM_TIMER will fire repeatedly
            }
            return 0; // We handled it (prevents default immediate destruction)
        } break;

        case WM_TIMER: { // Fired when a timer set by SetTimer ticks
            if (wParam == IDT_FADE_TIMER) {
                if (g_Alpha > 10) {
                    g_Alpha -= 10; // Reduce opacity step-by-step to fade out
                    SetLayeredWindowAttributes(hwnd, 0, g_Alpha, LWA_ALPHA);
                } else {
                    KillTimer(hwnd, IDT_FADE_TIMER); // Stop the repeating timer
                    DestroyWindow(hwnd);              // Now actually close the window
                }
            }
        } break;

        case WM_DESTROY: { // Window is being destroyed; last cleanup step
            PostQuitMessage(0); // Tells the message loop to stop (GetMessage returns 0)
        } break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam); // Default handling for messages we don't process
    }
    return 0;
}

// -------------------------------------------------------------------------
// Helper Function for Task 10
// -------------------------------------------------------------------------
// Changes several "look and behavior" settings while the program is running.
void DynamicallyChangeWindowProperties(HWND hwnd) {
    // SetClassLongPtr changes attributes of the *window class* (affects behavior/appearance).
    // GCL_STYLE = "Get/Set Class Long: STYLE" (class-level style flags).
    // CS_DBLCLKS enables double-click messages (e.g., WM_LBUTTONDBLCLK).
    SetClassLongPtr(hwnd, GCL_STYLE, CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS);

    // Change the class icon and cursor to different standard system ones.
    // reinterpret_cast<LONG_PTR>(...) is needed because SetClassLongPtr stores a pointer-sized integer.
    SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(LoadIcon(NULL, IDI_ERROR)));
    SetClassLongPtr(hwnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_WAIT)));

    g_CurrentBgColor = RGB(100, 255, 100);                // New background color (light green)
    HBRUSH hNewBrush = CreateSolidBrush(g_CurrentBgColor); // Create the new brush used for painting the background
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(hNewBrush));

    // Move and resize the window; keep it topmost.
    // SetWindowPos updates position/size/Z-order in one call.
    SetWindowPos(hwnd, HWND_TOPMOST, 300, 250, 640, 480, SWP_SHOWWINDOW);

    // Keep alpha behavior consistent after changing properties.
    SetLayeredWindowAttributes(hwnd, 0, g_Alpha, LWA_ALPHA);

    InvalidateRect(hwnd, NULL, TRUE); // Force a repaint (TRUE = erase background too)

    MessageBox(hwnd,
        _T("Window properties (Icon, Cursor, Background, Size, Position) have been updated!"),
        _T("Dynamic Update Complete"),
        MB_ICONWARNING | MB_OK);
}