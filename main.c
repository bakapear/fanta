// gcc main.c -mwindows -lgdi32 -lcomctl32 -luxtheme -o main

#include <windows.h>
//
#include <commctrl.h>

#include "w32d.h"

HINSTANCE g_hInst;
HWND g_hStatus;

#define IDM_FILE_EXIT 1001

#define IDM_TOOLS_THEME_DEFAULT 1002
#define IDM_TOOLS_THEME_LIGHT 1003
#define IDM_TOOLS_THEME_DARK 1004

HMENU hThemeMenu;

int CurrentTheme = IDM_TOOLS_THEME_DEFAULT;

void ApplyTheme(HWND hWnd, int theme) {
  CurrentTheme = theme;
  if (CurrentTheme == IDM_TOOLS_THEME_LIGHT) {
    w32d_set_theme(hWnd, W32D_DISABLE);
  } else if (CurrentTheme == IDM_TOOLS_THEME_DARK) {
    w32d_set_theme(hWnd, W32D_ENABLE);
  } else if (CurrentTheme == IDM_TOOLS_THEME_DEFAULT) {
    w32d_set_theme(hWnd, W32D_DEFAULT);
  }
}

void CreateMenus(HWND hWnd) {
  HMENU hMenu = CreateMenu();

  HMENU hFileMenu = CreatePopupMenu();
  AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "&File");
  AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, "&Exit");

  HMENU hToolsMenu = CreatePopupMenu();
  AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hToolsMenu, "&Tools");

  hThemeMenu = CreatePopupMenu();
  AppendMenu(hToolsMenu, MF_POPUP, (UINT_PTR)hThemeMenu, "&Theme");
  AppendMenu(hThemeMenu, MF_STRING, IDM_TOOLS_THEME_DEFAULT, "&Use System Default");
  AppendMenu(hThemeMenu, MF_STRING, IDM_TOOLS_THEME_LIGHT, "&Light Theme");
  AppendMenu(hThemeMenu, MF_STRING, IDM_TOOLS_THEME_DARK, "&Dark Theme");
  CheckMenuRadioItem(hThemeMenu, IDM_TOOLS_THEME_DEFAULT, IDM_TOOLS_THEME_DARK, CurrentTheme, MF_BYCOMMAND);

  SetMenu(hWnd, hMenu);
}

void HandleMenus(HWND hWnd, WPARAM wParam) {
  switch (LOWORD(wParam)) {
    case IDM_FILE_EXIT: {
      PostMessage(hWnd, WM_CLOSE, 0, 0);
      break;
    }
    case IDM_TOOLS_THEME_DEFAULT:
    case IDM_TOOLS_THEME_LIGHT:
    case IDM_TOOLS_THEME_DARK: {
      CheckMenuRadioItem(hThemeMenu, IDM_TOOLS_THEME_DEFAULT, IDM_TOOLS_THEME_DARK, wParam, MF_BYCOMMAND);
      ApplyTheme(hWnd, wParam);
      break;
    }
  }
}

void CreateStatusBar(HWND hWnd) {
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_BAR_CLASSES;
  InitCommonControlsEx(&icex);

  g_hStatus = CreateWindowEx(0, STATUSCLASSNAME, 0, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, (HMENU)1, g_hInst, 0);
}

void HandleStatusBar(HWND hWnd) {
  static int partWidths[] = {100, 200, 100};

  RECT rc;
  GetClientRect(hWnd, &rc);
  int partEdges[ARRAYSIZE(partWidths)];
  for (int i = ARRAYSIZE(partEdges); i >= 0; --i) partEdges[i] = (rc.right -= partWidths[i]);

  SendMessage(g_hStatus, WM_SIZE, 0, 0);

  SendMessage(g_hStatus, SB_SETPARTS, ARRAYSIZE(partEdges) + 1, (LPARAM)partEdges);

  SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM) "Left (Fill)");
  SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM) "One (100)");
  SendMessage(g_hStatus, SB_SETTEXT, 2, (LPARAM) "Two (200)");
  SendMessage(g_hStatus, SB_SETTEXT, 3, (LPARAM) "Three (100)");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE: {
      CreateMenus(hWnd);
      CreateStatusBar(hWnd);
      break;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      FillRect(hdc, &ps.rcPaint, (HBRUSH)CreateSolidBrush(RGB(0, 132, 44)));
      EndPaint(hWnd, &ps);
      break;
    }
    case WM_COMMAND: {
      HandleMenus(hWnd, wParam);
      break;
    }
    case WM_SIZE: {
      HandleStatusBar(hWnd);
      break;
    }
    case WM_DESTROY: {
      PostQuitMessage(0);
      break;
    }
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  g_hInst = hInstance;

  WNDCLASS wc = {0};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = "win32dark";
  wc.hCursor = LoadCursor(0, IDC_ARROW);
  if (!RegisterClass(&wc)) return 0;

  HWND hWnd = CreateWindow(wc.lpszClassName, "win32dark", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, 0, 0, hInstance, 0);
  if (!hWnd) return 0;

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  ApplyTheme(hWnd, CurrentTheme);

  MSG msg;
  while (GetMessage(&msg, 0, 0, 0)) TranslateMessage(&msg), DispatchMessage(&msg);
  return (int)msg.wParam;
}