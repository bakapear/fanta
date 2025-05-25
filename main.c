#include "w32d.h"
#include "webview.h"
#include "window.h"

Window g_Win;
WebView2 g_Web;

enum StatusBarID { IDS_LEFT = 2001, IDS_ONE, IDS_TWO, IDS_THREE };
enum MenuBarID { IDM_FILE = 3001, IDM_FILE_EXIT, IDM_TOOLS, IDM_TOOLS_DIALOG, IDM_TOOLS_THEME, IDM_TOOLS_THEME_DEFAULT, IDM_TOOLS_THEME_LIGHT, IDM_TOOLS_THEME_DARK };

void ApplyTheme(Window* win, W32DMode mode) {
  w32d_set_theme(win->hWnd, mode);

  mode = (int[]){2, 3, 1}[mode];
  window_menubar_check_radio(win, IDM_TOOLS_THEME_DEFAULT, IDM_TOOLS_THEME_DARK, mode);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
  if (hWnd == g_Win.StatusBar.hWnd) {
    if (uMsg == WM_LBUTTONDOWN) {
      WindowStatusBarPart* part = window_statusbar_get_active(&g_Win, lParam);
      if (part) MessageBox(g_Win.hWnd, part->text, "Clicked StatusBar Item", 0);
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
            ApplyTheme(&g_Win, mode);
            break;
          }
        }
        break;
      }
      case WM_SIZE: {
        RECT rc;
        GetClientRect(g_Win.hWnd, &rc);
        rc.bottom -= 22;
        webview2_resize(&g_Web, rc);
        break;
      }
      case WM_WEBVIEW2_READY: {
        webview2_navigate(&g_Web, L"https://example.com");
        break;
      }
    }
  }

  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  window_init(&g_Win, hInstance, "fanta window", 640, 480);
  if (!g_Win.hWnd) return 0;

  window_set_background(&g_Win, (HBRUSH)CreateSolidBrush(RGB(132, 34, 34)));
  window_set_proc(&g_Win, WndProc);

  window_statusbar_init(&g_Win);
  window_statusbar_set_proc(&g_Win, WndProc);
  window_statusbar_add_item(&g_Win, IDS_LEFT, "Left (Fill)", -1);
  window_statusbar_add_item(&g_Win, IDS_ONE, "One (100)", 100);
  window_statusbar_add_item(&g_Win, IDS_TWO, "Two (200)", 200);
  window_statusbar_add_item(&g_Win, IDS_THREE, "Three (100)", 100);

  window_menubar_init(&g_Win);

  window_menubar_add_menu(&g_Win, IDM_FILE, "&File", 0);
  window_menubar_add_item(&g_Win, IDM_FILE_EXIT, "&Exit", IDM_FILE);

  window_menubar_add_menu(&g_Win, IDM_TOOLS, "&Tools", 0);
  window_menubar_add_item(&g_Win, IDM_TOOLS_DIALOG, "&Open Dialog", IDM_TOOLS);
  window_menubar_add_menu(&g_Win, IDM_TOOLS_THEME, "&Theme", IDM_TOOLS);
  window_menubar_add_item(&g_Win, IDM_TOOLS_THEME_DEFAULT, "&Use System Default", IDM_TOOLS_THEME);
  window_menubar_add_item(&g_Win, IDM_TOOLS_THEME_LIGHT, "&Light Theme", IDM_TOOLS_THEME);
  window_menubar_add_item(&g_Win, IDM_TOOLS_THEME_DARK, "&Dark Theme", IDM_TOOLS_THEME);

  ApplyTheme(&g_Win, W32D_DEFAULT);

  window_show(&g_Win, nCmdShow);

  webview2_init(&g_Web, g_Win.hWnd, "--disable-gpu");

  return window_global_msg_loop();
}
