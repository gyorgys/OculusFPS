// OculusFPS.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "OculusFPS.h"
#include <stdlib.h>
#include <string> 
#include <time.h>
#include "OculusPerfInfo.h"

#define MAX_LOADSTRING 100

const int TITLE_HEIGHT = 12;
const int DATA_HEIGHT = 72;
const int CELL_WIDTH = 100;
const int PADDING = 4;

const int CLIENT_WIDTH = PADDING * 2 + CELL_WIDTH * 3;
const int CLIENT_HEIGHT = PADDING * 3 + TITLE_HEIGHT + DATA_HEIGHT;


const COLORREF WINDOW_BACKGROUND = RGB(64, 64, 64);
const COLORREF TITLE_COLOR = RGB(148, 148, 148);
const COLORREF DATA_COLOR = RGB(192, 192, 192);
const COLORREF AWS_DATA_COLOR = RGB(255, 220, 192);

const WCHAR STR_NO_INIT[] = L"Unable to initialize Oculus Library";
const WCHAR STR_NO_HMD[] = L"Headset Inactive"; 
const WCHAR STR_FPS_TITLE[] = L"FPS";
const WCHAR STR_LATENCY_TITLE[] = L"Latency (ms)";
const WCHAR STR_DROPPED_FRAMES_TITLE[] = L"Dropped frames";
const WCHAR STR_AWS_ACTIVE[] = L"AWS";

// Global Variables:
HINSTANCE _hInst;                                // current instance
WCHAR _szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR _szWindowClass[MAX_LOADSTRING];            // the main window class name
UINT_PTR _idTimer = 0;
HWND _hPerfInfoWnd;
OculusPerfInfo* _perfInfoClass;

// Perf data
bool _perfInfoInitialized;
opi_PerfInfo _perfInfo;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL                InitializeTimer(HWND);
void                UpdatePerfData(opi_PerfInfo&);
void                DisplayPerfData(HDC);
VOID CALLBACK       TimerCallback(HWND, UINT, UINT, DWORD);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    srand(time(NULL));
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, _szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_OCULUSFPS, _szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OCULUSFPS));
   
    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    delete(_perfInfoClass);
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.lpszMenuName   = NULL;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OCULUSFPS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = CreateSolidBrush(WINDOW_BACKGROUND);
    wcex.lpszClassName  = _szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   _hInst = hInstance; // Store instance handle in our global variable
   const DWORD dwStyle = WS_DLGFRAME | WS_SYSMENU;

   RECT rcWindow = { 0, 0, CLIENT_WIDTH, CLIENT_HEIGHT};
   AdjustWindowRect(&rcWindow, dwStyle, TRUE);
   HWND hWnd = CreateWindowW(_szWindowClass, _szTitle, dwStyle,
      CW_USEDEFAULT, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   _perfInfoClass = new OculusPerfInfo();
   _perfInfoInitialized = _perfInfoClass->init();
   _perfInfoClass->setCallback(UpdatePerfData);
   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        _hPerfInfoWnd = hWnd;
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
    case WM_DISPLAYCHANGE:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            DisplayPerfData(hdc);
            EndPaint(hWnd, &ps);
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

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void UpdatePerfData(opi_PerfInfo& newPerfInfo)
{
    _perfInfo = newPerfInfo;
    if (_hPerfInfoWnd != NULL) {
        InvalidateRect(_hPerfInfoWnd, NULL, TRUE);
    }
}

void DisplayPerfData(HDC hdc)
{
    HFONT hTitleFont, hDataFont, hOldFont;
    SetMapMode(hdc, MM_TEXT);

    // Retrieve a handle to the variable stock font.  
    hTitleFont = CreateFont(TITLE_HEIGHT, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Arial"));
    hDataFont = CreateFont(DATA_HEIGHT, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Arial"));
    // Select the variable stock font into the specified device context. 
    if (hOldFont = (HFONT)SelectObject(hdc, hTitleFont))
    {
        SetTextAlign(hdc, TA_TOP | TA_CENTER);
        SetTextColor(hdc, TITLE_COLOR);
        SetBkColor(hdc, WINDOW_BACKGROUND);

        if (!_perfInfoInitialized) {
            int x = CLIENT_WIDTH / 2;
            int y = (CLIENT_HEIGHT - TITLE_HEIGHT) / 2;
            TextOut(hdc, x, y, STR_NO_INIT, wcslen(STR_NO_INIT));
        }
        else if (!_perfInfo.fHeadsetActive) {
            int x = CLIENT_WIDTH / 2;
            int y = (CLIENT_HEIGHT - TITLE_HEIGHT) / 2;
            TextOut(hdc, x, y, STR_NO_HMD, wcslen(STR_NO_HMD));
        }
        else
        {
            std::wstring strFps = std::to_wstring(_perfInfo.nAppFps);
            std::wstring strLatency = std::to_wstring(_perfInfo.nLatencyMs);
            std::wstring strDroppedFrames = std::to_wstring(_perfInfo.nDroppedFrames);
            
            int y = PADDING;
            int x = PADDING + CELL_WIDTH / 2;

            // Display the text string.  
            TextOut(hdc, x, y, STR_FPS_TITLE, wcslen(STR_FPS_TITLE));
            TextOut(hdc, x + CELL_WIDTH, y, STR_LATENCY_TITLE, wcslen(STR_LATENCY_TITLE));
            TextOut(hdc, x + CELL_WIDTH * 2, y, STR_DROPPED_FRAMES_TITLE, wcslen(STR_DROPPED_FRAMES_TITLE));

            SetTextColor(hdc, _perfInfo.fAwsActive ? AWS_DATA_COLOR : DATA_COLOR);
            y += TITLE_HEIGHT;
            SelectObject(hdc, hDataFont);
            TextOut(hdc, x, y, strFps.c_str(), strFps.length());
            TextOut(hdc, x + CELL_WIDTH, y, strLatency.c_str(), strLatency.length());
            TextOut(hdc, x + CELL_WIDTH * 2, y, strDroppedFrames.c_str(), strDroppedFrames.length());
        }
        // Restore the original font.        
        SelectObject(hdc, hOldFont);
    }
}
