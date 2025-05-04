#include <windows.h>
//
#include <commctrl.h>
#include <windowsx.h>

#define WINDOW_MAX_STATUSBAR_PARTS 16
#define WINDOW_MAX_MENUBAR_MENUS 16

typedef struct {
  const char* text;
  int width;
  int id;
} WindowStatusBarPart;

typedef struct {
  HWND hWnd;
  int size;
  WindowStatusBarPart parts[WINDOW_MAX_STATUSBAR_PARTS];
} WindowStatusBar;

typedef struct {
  int id;
  HMENU handle;
} WindowMenubarMenu;

typedef struct {
  HMENU menu;
  int size;
  WindowMenubarMenu menus[WINDOW_MAX_MENUBAR_MENUS];
} WindowMenuBar;

typedef struct {
  HINSTANCE hInstance;
  HWND hWnd;
  WNDPROC WndProc;
  WindowStatusBar StatusBar;
  WindowMenuBar MenuBar;
} Window;

int window_global_msg_loop() {
  MSG msg;
  while (GetMessage(&msg, 0, 0, 0)) TranslateMessage(&msg), DispatchMessage(&msg);
  return (int)msg.wParam;
}

void window_statusbar_update(Window* win) {
  if (!win || !win->StatusBar.hWnd) return;

  SendMessage(win->StatusBar.hWnd, WM_SIZE, 0, 0);

  if (win->StatusBar.size == 0) return;

  RECT rc;
  GetClientRect(win->hWnd, &rc);
  rc.right -= 18;

  int count = win->StatusBar.size;
  int edges[ARRAYSIZE(win->StatusBar.parts)];

  for (int i = count - 1; i >= 0; --i) {
    int next = i + 1;
    if (next < count) rc.right -= win->StatusBar.parts[next].width;
    edges[i] = rc.right;
  }

  SendMessage(win->StatusBar.hWnd, SB_SETPARTS, (WPARAM)count, (LPARAM)edges);

  for (int i = 0; i < count; ++i) {
    SendMessage(win->StatusBar.hWnd, SB_SETTEXT, i, (LPARAM)win->StatusBar.parts[i].text);
  }
}

WindowStatusBarPart* window_statusbar_add_item(Window* win, int id, const char* text, int width) {
  if (!win || !win->StatusBar.hWnd) return 0;
  if (win->StatusBar.size >= WINDOW_MAX_STATUSBAR_PARTS) return 0;

  int i = win->StatusBar.size++;
  win->StatusBar.parts[i].text = text;
  win->StatusBar.parts[i].width = width;
  win->StatusBar.parts[i].id = id;

  window_statusbar_update(win);

  return &win->StatusBar.parts[i];
}

WindowStatusBarPart* window_statusbar_get_item(Window* win, int id) {
  if (!win || !win->StatusBar.hWnd) return 0;

  for (int i = 0; i < win->StatusBar.size; i++) {
    if (win->StatusBar.parts[i].id == id) return &win->StatusBar.parts[i];
  }

  return 0;
}

void window_statusbar_remove_item(Window* win, int id) {
  if (!win || !win->StatusBar.hWnd) return;

  for (int i = 0; i < win->StatusBar.size; i++) {
    if (win->StatusBar.parts[i].id != id) continue;

    win->StatusBar.size--;
    for (int j = i; j < win->StatusBar.size; j++) win->StatusBar.parts[j] = win->StatusBar.parts[j + 1];
    window_statusbar_update(win);
    return;
  }
}

void window_statusbar_init(Window* win) {
  if (!win || !win->hWnd) return;

  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_BAR_CLASSES;
  InitCommonControlsEx(&icex);

  win->StatusBar.hWnd = CreateWindowEx(0, STATUSCLASSNAME, 0, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, win->hWnd, (HMENU)1, win->hInstance, 0);
  win->StatusBar.size = 0;

  window_statusbar_update(win);
}

HMENU _window_menubar_find_menu(Window* win, int id) {
  if (id == 0) return win->MenuBar.menu;
  for (int i = 0; i < win->MenuBar.size; i++) {
    if (win->MenuBar.menus[i].id == id) return win->MenuBar.menus[i].handle;
  }
  return 0;
}

int window_menubar_add_menu(Window* win, int id, const char* text, int parentId) {
  if (!win || !win->MenuBar.menu) return 0;
  if (win->MenuBar.size >= WINDOW_MAX_MENUBAR_MENUS) return 0;

  HMENU parent = _window_menubar_find_menu(win, parentId);
  if (!parent) return 0;

  int i = win->MenuBar.size++;
  win->MenuBar.menus[i].handle = CreatePopupMenu();
  win->MenuBar.menus[i].id = id;

  AppendMenu(parent, MF_POPUP, (UINT_PTR)win->MenuBar.menus[i].handle, text);

  return id;
}

int window_menubar_add_item(Window* win, int id, const char* text, int parentId) {
  if (!win || !win->MenuBar.menu) return 0;

  HMENU parent = _window_menubar_find_menu(win, parentId);
  if (!parent) return 0;

  AppendMenu(parent, MF_STRING, id, text);

  return id;
}

void window_menubar_check_radio(Window* win, int startId, int endId, int check) {
  check = startId + check - 1;
  CheckMenuRadioItem(win->MenuBar.menu, startId, endId, check, MF_BYCOMMAND);
}

void window_menubar_init(Window* win) {
  if (!win || !win->hWnd) return;

  win->MenuBar.menu = CreateMenu();
  SetMenu(win->hWnd, win->MenuBar.menu);
}

LRESULT CALLBACK _window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  Window* win = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

  switch (uMsg) {
    case WM_SIZE: {
      window_statusbar_update(win);
      break;
    }
    case WM_DESTROY: {
      PostQuitMessage(0);
      break;
    }
  }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void window_init(Window* win, HINSTANCE inst, const char* title, INT width, INT height) {
  memset(win, 0, sizeof(Window));

  WNDCLASS wc = {0};
  wc.lpfnWndProc = _window_proc;
  wc.hInstance = inst;
  wc.lpszClassName = title;
  wc.hCursor = LoadCursor(0, IDC_ARROW);

  if (RegisterClass(&wc)) {
    win->hWnd = CreateWindow(wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, inst, 0);
    win->hInstance = inst;
    win->WndProc = _window_proc;

    SetWindowLongPtr(win->hWnd, GWLP_USERDATA, (LONG_PTR)win);
  }
}

void window_show(Window* win, BOOL show) {
  if (!win || !win->hWnd) return;
  ShowWindow(win->hWnd, show);
  UpdateWindow(win->hWnd);
}

void window_set_background(Window* win, HBRUSH bg) {
  if (!win || !win->hWnd) return;
  SetClassLongPtr(win->hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)bg);
  InvalidateRect(win->hWnd, 0, TRUE);
}

void window_set_proc(Window* win, SUBCLASSPROC proc) {
  if (!win || !win->hWnd) return;
  SetWindowSubclass(win->hWnd, proc, 0, (DWORD_PTR)win);
}

void window_remove_proc(Window* win, SUBCLASSPROC proc) {
  if (!win || !win->hWnd) return;
  RemoveWindowSubclass(win->hWnd, proc, 0);
}

void window_statusbar_set_proc(Window* win, SUBCLASSPROC proc) {
  if (!win || !win->StatusBar.hWnd) return;
  SetWindowSubclass(win->StatusBar.hWnd, proc, 0, (DWORD_PTR)win);
}

void window_statusbar_remove_proc(Window* win, SUBCLASSPROC proc) {
  if (!win || !win->StatusBar.hWnd) return;
  RemoveWindowSubclass(win->StatusBar.hWnd, proc, 0);
}

WindowStatusBarPart* window_statusbar_get_active(Window* win, LPARAM lParam) {
  POINT pt;
  pt.x = GET_X_LPARAM(lParam);
  pt.y = GET_Y_LPARAM(lParam);

  for (int i = 0; i < win->StatusBar.size; ++i) {
    RECT rc;
    if (SendMessage(win->StatusBar.hWnd, SB_GETRECT, i, (LPARAM)&rc)) {
      if (PtInRect(&rc, pt)) return &win->StatusBar.parts[i];
    }
  }

  return 0;
}