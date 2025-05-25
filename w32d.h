// w32d - win32 dark theme support

/*
MIT License

Copyright (c) 2025 pear

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef W32D_H
#define W32D_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#endif

#include <windows.h>
//
#include <uxtheme.h>
#include <vsstyle.h>

typedef enum { W32D_DISABLE, W32D_ENABLE, W32D_DEFAULT } W32DMode;

typedef struct {
  BOOL _init;
  HBRUSH hbrMenu;
  HBRUSH hbrMenuItem;
  HBRUSH hbrMenuItemSelected;
  COLORREF clrMenuItemText;
  COLORREF clrMenuItemSelectedText;
  HBRUSH hbrStatus;
  HBRUSH hbrStatusGapLines;
  HBRUSH hbrStatusIcon;
  COLORREF clrStatusText;
} W32DTheme;

W32DTheme w32d_theme;

BOOL w32d_is_darkmode();
void w32d_set_theme(HWND hWnd, W32DMode mode);

void w32d_dark_titlebar(HWND hWnd, BOOL enable);
void w32d_dark_menubar(HWND hWnd, BOOL enable);
void w32d_dark_statusbar(HWND hWnd, BOOL enable);
void w32d_dark_contextmenu(BOOL enable);

// W32D_IMPLEMENTATION

BOOL w32d_is_darkmode() {
  DWORD value = 1;
  HKEY hKey;
  if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    DWORD size = sizeof(DWORD);
    RegQueryValueEx(hKey, "AppsUseLightTheme", 0, 0, (PBYTE)&value, &size);
    RegCloseKey(hKey);
  }
  return value == 0;
}

void _w32d_init_theme() {
  if (w32d_theme._init) return;
  w32d_theme._init = TRUE;

  w32d_theme.hbrMenu = CreateSolidBrush(RGB(32, 32, 32));
  w32d_theme.hbrMenuItem = CreateSolidBrush(RGB(32, 32, 32));
  w32d_theme.hbrMenuItemSelected = CreateSolidBrush(RGB(64, 64, 64));

  w32d_theme.clrMenuItemText = RGB(242, 242, 242);
  w32d_theme.clrMenuItemSelectedText = RGB(242, 242, 242);

  w32d_theme.hbrStatus = CreateSolidBrush(RGB(32, 32, 32));
  w32d_theme.hbrStatusIcon = CreateSolidBrush(RGB(200, 200, 200));
  w32d_theme.hbrStatusGapLines = CreateSolidBrush(RGB(128, 128, 128));
  w32d_theme.clrStatusText = RGB(242, 242, 242);
}

// #region w32d_dark_titlebar
#define WCA_USEDARKMODECOLORS 26

typedef struct {
  INT attribute;
  PVOID data;
  SIZE_T sizeOfData;
} WINDOWCOMPOSITIONATTRIBDATA;

void w32d_dark_titlebar(HWND hWnd, BOOL enable) {
  typedef BOOL(WINAPI * SetWindowCompositionAttributeFn)(HWND, void *);

  static SetWindowCompositionAttributeFn SetWindowCompositionAttribute = NULL;

  if (!SetWindowCompositionAttribute) {
    HMODULE hUser32 = GetModuleHandle("user32.dll");
    if (!hUser32) return;

    SetWindowCompositionAttribute = (SetWindowCompositionAttributeFn)GetProcAddress(hUser32, "SetWindowCompositionAttribute");
  }

  WINDOWCOMPOSITIONATTRIBDATA data = {WCA_USEDARKMODECOLORS, &enable, sizeof(enable)};
  SetWindowCompositionAttribute(hWnd, &data);
}
// #endregion

// #region w32d_dark_menubar
#define WM_UAHDRAWMENU 0x0091
#define WM_UAHDRAWMENUITEM 0x0092
#define WM_UAHMEASUREMENUITEM 0x0094

typedef struct {
  HMENU hmenu;
  HDC hdc;
  DWORD dwFlags;
} UAHMENU;

typedef struct {
  INT iPosition;
  // UAHMENUITEMMETRICS umim;
  // UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

typedef struct {
  DRAWITEMSTRUCT dis;
  UAHMENU um;
  UAHMENUITEM umi;
} UAHDRAWMENUITEM;

LRESULT CALLBACK _w32d_subclass_menubar(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
  switch (uMsg) {
    case WM_UAHDRAWMENU: {
      MENUBARINFO mbi;
      mbi.cbSize = sizeof(mbi);
      if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi)) return TRUE;

      RECT rcWindow;
      GetWindowRect(hWnd, &rcWindow);
      OffsetRect(&mbi.rcBar, -rcWindow.left, -rcWindow.top);

      UAHMENU *pUDM = (UAHMENU *)lParam;
      FillRect(pUDM->hdc, &mbi.rcBar, w32d_theme.hbrMenu);

      return TRUE;
    }
    case WM_UAHDRAWMENUITEM: {
      CHAR menuString[256];

      MENUITEMINFO mii = {0};
      mii.cbSize = sizeof(mii);
      mii.fMask = MIIM_STRING;
      mii.dwTypeData = menuString;
      mii.cch = ARRAYSIZE(menuString) - 1;

      UAHDRAWMENUITEM *pUDMI = (UAHDRAWMENUITEM *)lParam;
      GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);

      HBRUSH *bgBrush = &w32d_theme.hbrMenuItem;
      COLORREF *clrTxt = &w32d_theme.clrMenuItemText;

      if (pUDMI->dis.itemState & ODS_HOTLIGHT || pUDMI->dis.itemState & ODS_SELECTED) {
        bgBrush = &w32d_theme.hbrMenuItemSelected;
        clrTxt = &w32d_theme.clrMenuItemSelectedText;
      }

      DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
      if (pUDMI->dis.itemState & ODS_NOACCEL) dwFlags |= DT_HIDEPREFIX;

      FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, *bgBrush);

      SetTextColor(pUDMI->um.hdc, *clrTxt);
      SetBkMode(pUDMI->um.hdc, TRANSPARENT);
      DrawText(pUDMI->um.hdc, menuString, mii.cch, &pUDMI->dis.rcItem, dwFlags);

      return TRUE;
    }
    case WM_NCPAINT:
    case WM_NCACTIVATE: {
      DefWindowProc(hWnd, uMsg, wParam, lParam);

      RECT rcClient, rcWindow;
      GetClientRect(hWnd, &rcClient);
      MapWindowPoints(hWnd, 0, (POINT *)&rcClient, 2);
      GetWindowRect(hWnd, &rcWindow);
      OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);

      rcClient.bottom = rcClient.top;
      rcClient.top--;

      HDC hdc = GetWindowDC(hWnd);
      FillRect(hdc, &rcClient, w32d_theme.hbrMenu);
      ReleaseDC(hWnd, hdc);

      return TRUE;
    }
  }
  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void w32d_dark_menubar(HWND hWnd, BOOL enable) {
  _w32d_init_theme();

  RemoveWindowSubclass(hWnd, _w32d_subclass_menubar, 0);
  if (enable) SetWindowSubclass(hWnd, _w32d_subclass_menubar, 0, 0);

  SetWindowTheme(hWnd, 0, 0);
};
// #endregion

// #region w32d_dark_statusbar
LRESULT CALLBACK _w32d_subclass_statusbar(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
  switch (uMsg) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      if (!BeginPaint(hWnd, &ps)) break;

      static HFONT hFont;
      if (!hFont) {
        NONCLIENTMETRICS metrics;
        metrics.cbSize = sizeof(NONCLIENTMETRICS);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0)) {
          hFont = CreateFontIndirect(&metrics.lfMessageFont);
        }
      }
      SelectObject(ps.hdc, hFont);

      RECT rcClient;
      GetClientRect(hWnd, &rcClient);
      FillRect(ps.hdc, &rcClient, w32d_theme.hbrStatus);

      SetBkMode(ps.hdc, TRANSPARENT);
      SetTextColor(ps.hdc, w32d_theme.clrStatusText);

      INT blockCoord[128];
      INT blockCount = SendMessage(hWnd, SB_GETPARTS, ARRAYSIZE(blockCoord), (LPARAM)blockCoord);
      for (INT i = 0; i < blockCount; i++) {
        RECT rcBlock;
        CHAR blockText[MAX_PATH] = "";
        if (!SendMessage(hWnd, SB_GETRECT, (WPARAM)i, (LPARAM)&rcBlock)) continue;
        if (!SendMessage(hWnd, SB_GETTEXT, (WPARAM)i, (LPARAM)blockText)) continue;

        rcBlock.left += 4;
        DrawText(ps.hdc, blockText, -1, &rcBlock, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        rcBlock.right -= 2;
        rcBlock.left = rcBlock.right;
        rcBlock.left++;
        rcBlock.top += 2;
        rcBlock.bottom -= 3;
        FillRect(ps.hdc, &rcBlock, w32d_theme.hbrStatusGapLines);
        rcBlock.left++;
        rcBlock.right++;
        FillRect(ps.hdc, &rcBlock, w32d_theme.hbrStatus);
      }

      RECT rcGrip;
      rcGrip.left = rcClient.right - GetSystemMetrics(SM_CXHSCROLL);
      rcGrip.top = rcClient.bottom - GetSystemMetrics(SM_CYVSCROLL);
      rcGrip.right = rcClient.right;
      rcGrip.bottom = rcClient.bottom;

      for (INT row = 0; row < 3; ++row) {
        for (INT col = 0; col <= row; ++col) {
          INT x = rcGrip.right - 2 - (col * 3);
          INT y = rcGrip.bottom - 2 - ((row - col) * 3);
          RECT rc = {x - 2, y - 2, x, y};
          FillRect(ps.hdc, &rc, w32d_theme.hbrStatusIcon);
        }
      }

      EndPaint(hWnd, &ps);

      return TRUE;
    }
  }

  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void w32d_dark_statusbar(HWND hWnd, BOOL enable) {
  HWND status = FindWindowEx(hWnd, 0, STATUSCLASSNAME, 0);
  if (!status) return;

  _w32d_init_theme();

  RemoveWindowSubclass(status, _w32d_subclass_statusbar, 0);
  if (enable) SetWindowSubclass(status, _w32d_subclass_statusbar, 0, 0);

  InvalidateRect(status, 0, TRUE);
}
// #endregion

// #region w32d_dark_contextmenu
#define APPMODE_DEFAULT 0
#define APPMODE_FORCE_DARK 2

void w32d_dark_contextmenu(BOOL enable) {
  typedef int(__stdcall * SetPreferredAppModeFn)(int);
  typedef void(__stdcall * FlushMenuThemesFn)(void);

  static SetPreferredAppModeFn SetPreferredAppMode;
  static FlushMenuThemesFn FlushMenuThemes;

  if (!SetPreferredAppMode || !FlushMenuThemes) {
    HMODULE hUxTheme = LoadLibrary("uxtheme.dll");
    if (!hUxTheme) return;

    SetPreferredAppMode = (SetPreferredAppModeFn)GetProcAddress(hUxTheme, (LPCSTR)135);
    FlushMenuThemes = (FlushMenuThemesFn)GetProcAddress(hUxTheme, (LPCSTR)136);
    FreeLibrary(hUxTheme);
  }

  SetPreferredAppMode(enable ? APPMODE_FORCE_DARK : APPMODE_DEFAULT);
  FlushMenuThemes();
}
// #endregion

// #region w32d_set_theme
void _w32d_dark_toggle(HWND hWnd, BOOL enable) {
  w32d_dark_titlebar(hWnd, enable);
  w32d_dark_menubar(hWnd, enable);
  w32d_dark_statusbar(hWnd, enable);
  w32d_dark_contextmenu(enable);
}

LRESULT CALLBACK _w32d_subclass_autotheme(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
  switch (uMsg) {
    case WM_SETTINGCHANGE: {
      if (lParam && wcscmp((LPCWSTR)lParam, L"ImmersiveColorSet") == 0) _w32d_dark_toggle(hWnd, w32d_is_darkmode());
      break;
    }
  }
  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void w32d_set_theme(HWND hWnd, W32DMode mode) {
  RemoveWindowSubclass(hWnd, _w32d_subclass_autotheme, 0);

  BOOL enable = mode;

  if (mode == W32D_DEFAULT) {
    SetWindowSubclass(hWnd, _w32d_subclass_autotheme, 0, 0);
    enable = w32d_is_darkmode();
  }

  _w32d_dark_toggle(hWnd, enable);
}
// #endregion

#ifdef __cplusplus
}
#endif

#endif  // W32D_H
