// gcc main.c -o main -lgdi32 -lcomctl32 -luxtheme -luuid -lole32 -loleaut32

#include "w32d.h"
#include "webbrowser.h"
#include "window.h"

WebBrowser2 web;

enum StatusBarID { IDS_LEFT = 2001, IDS_ONE, IDS_TWO, IDS_THREE };
enum MenuBarID { IDM_FILE = 3001, IDM_FILE_EXIT, IDM_TOOLS, IDM_TOOLS_DIALOG, IDM_TOOLS_THEME, IDM_TOOLS_THEME_DEFAULT, IDM_TOOLS_THEME_LIGHT, IDM_TOOLS_THEME_DARK };

void ApplyTheme(Window* win, W32DMode mode) {
  w32d_set_theme(win->hWnd, mode);

  mode = (int[]){2, 3, 1}[mode];
  window_menubar_check_radio(win, IDM_TOOLS_THEME_DEFAULT, IDM_TOOLS_THEME_DARK, mode);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
  Window* win = (Window*)dwRefData;
  if (!win) return DefSubclassProc(hWnd, uMsg, wParam, lParam);

  if (hWnd == win->StatusBar.hWnd) {
    if (uMsg == WM_LBUTTONDOWN) {
      WindowStatusBarPart* part = window_statusbar_get_active(win, lParam);
      if (part) MessageBox(win->hWnd, part->text, "Clicked StatusBar Item", 0);
    }
  } else {
    switch (uMsg) {
      case WM_COMMAND: {
        switch (LOWORD(wParam)) {
          case IDM_FILE_EXIT: {
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
          }
          case IDM_TOOLS_DIALOG: {
            MessageBox(hWnd, "Hello World!", "Info Message", MB_ICONINFORMATION);
            break;
          }
          case IDM_TOOLS_THEME_DEFAULT:
          case IDM_TOOLS_THEME_LIGHT:
          case IDM_TOOLS_THEME_DARK: {
            int mode = (int[]){2, 0, 1}[wParam - IDM_TOOLS_THEME_DEFAULT];
            ApplyTheme(win, mode);
            break;
          }
        }
        break;
      }
      case WM_SIZE: {
        RECT rc;
        GetClientRect(win->hWnd, &rc);
        rc.bottom -= 22;
        webbrowser_resize(&web, rc);
        break;
      }
    }
  }

  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  Window win;
  window_init(&win, hInstance, "fanta window", 640, 480);
  if (!win.hWnd) return 0;

  window_set_background(&win, (HBRUSH)CreateSolidBrush(RGB(132, 34, 34)));
  window_set_proc(&win, WndProc);

  window_statusbar_init(&win);
  window_statusbar_set_proc(&win, WndProc);
  window_statusbar_add_item(&win, IDS_LEFT, "Left (Fill)", -1);
  window_statusbar_add_item(&win, IDS_ONE, "One (100)", 100);
  window_statusbar_add_item(&win, IDS_TWO, "Two (200)", 200);
  window_statusbar_add_item(&win, IDS_THREE, "Three (100)", 100);

  window_menubar_init(&win);

  window_menubar_add_menu(&win, IDM_FILE, "&File", 0);
  window_menubar_add_item(&win, IDM_FILE_EXIT, "&Exit", IDM_FILE);

  window_menubar_add_menu(&win, IDM_TOOLS, "&Tools", 0);
  window_menubar_add_item(&win, IDM_TOOLS_DIALOG, "&Open Dialog", IDM_TOOLS);
  window_menubar_add_menu(&win, IDM_TOOLS_THEME, "&Theme", IDM_TOOLS);
  window_menubar_add_item(&win, IDM_TOOLS_THEME_DEFAULT, "&Use System Default", IDM_TOOLS_THEME);
  window_menubar_add_item(&win, IDM_TOOLS_THEME_LIGHT, "&Light Theme", IDM_TOOLS_THEME);
  window_menubar_add_item(&win, IDM_TOOLS_THEME_DARK, "&Dark Theme", IDM_TOOLS_THEME);

  ApplyTheme(&win, W32D_DEFAULT);

  webbrowser_init(&web, win.hWnd);
  webbrowser_navigate(&web, L"https://example.com");

  window_show(&win, nCmdShow);

  return window_global_msg_loop();
}