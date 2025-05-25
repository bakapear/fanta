#ifndef WEBVIEW2_EMBED_H
#define WEBVIEW2_EMBED_H

#define WM_WEBVIEW2_READY (WM_USER + 100)

#define COBJMACROS
#include <stdlib.h>
#include <windows.h>

#include "webview/WebView2.h"

typedef struct {
  ICoreWebView2Controller* controller;
  ICoreWebView2* webview;
} WebView2;

typedef struct {
  ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl* lpVtbl;
  ULONG refCount;
  HWND hwnd;
  WebView2* web;
} ControllerHandler;

static HRESULT STDMETHODCALLTYPE Controller_QueryInterface(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* this, REFIID riid, void** ppvObject) {
  if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)) {
    *ppvObject = this;
    this->lpVtbl->AddRef((IUnknown*)this);
    return S_OK;
  }
  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Controller_AddRef(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* this) { return ++((ControllerHandler*)this)->refCount; }

static ULONG STDMETHODCALLTYPE Controller_Release(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* this) {
  ULONG ref = --((ControllerHandler*)this)->refCount;
  if (ref == 0) free(this);
  return ref;
}

static HRESULT STDMETHODCALLTYPE Controller_Invoke(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* this, HRESULT result, ICoreWebView2Controller* controller) {
  if (controller) {
    ControllerHandler* self = (ControllerHandler*)this;
    self->web->controller = controller;
    controller->lpVtbl->AddRef(controller);

    controller->lpVtbl->get_CoreWebView2(controller, &self->web->webview);

    RECT bounds;
    GetClientRect(self->hwnd, &bounds);
    controller->lpVtbl->put_Bounds(controller, bounds);

    PostMessage(self->hwnd, WM_WEBVIEW2_READY, 0, 0);
  }
  return S_OK;
}

static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl g_controller_vtbl = {Controller_QueryInterface, Controller_AddRef, Controller_Release, Controller_Invoke};

typedef struct {
  ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl* lpVtbl;
  ULONG refCount;
  HWND hwnd;
  WebView2* web;
} EnvironmentHandler;

static HRESULT STDMETHODCALLTYPE Env_QueryInterface(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* this, REFIID riid, void** ppvObject) {
  if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)) {
    *ppvObject = this;
    this->lpVtbl->AddRef((IUnknown*)this);
    return S_OK;
  }
  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE Env_AddRef(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* this) { return ++((EnvironmentHandler*)this)->refCount; }

static ULONG STDMETHODCALLTYPE Env_Release(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* this) {
  ULONG ref = --((EnvironmentHandler*)this)->refCount;
  if (ref == 0) free(this);
  return ref;
}

static HRESULT STDMETHODCALLTYPE Env_Invoke(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* this, HRESULT result, ICoreWebView2Environment* env) {
  if (env) {
    EnvironmentHandler* self = (EnvironmentHandler*)this;

    ControllerHandler* controllerHandler = (ControllerHandler*)malloc(sizeof(ControllerHandler));
    controllerHandler->lpVtbl = &g_controller_vtbl;
    controllerHandler->refCount = 1;
    controllerHandler->hwnd = self->hwnd;
    controllerHandler->web = self->web;

    env->lpVtbl->CreateCoreWebView2Controller(env, self->hwnd, (ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*)controllerHandler);
  }
  return S_OK;
}

static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl g_env_vtbl = {Env_QueryInterface, Env_AddRef, Env_Release, Env_Invoke};

static void webview2_init(WebView2* web, HWND hwnd, LPCSTR args) {
  EnvironmentHandler* envHandler = (EnvironmentHandler*)malloc(sizeof(EnvironmentHandler));
  envHandler->lpVtbl = &g_env_vtbl;
  envHandler->refCount = 1;
  envHandler->hwnd = hwnd;
  envHandler->web = web;

  wchar_t tempPath[MAX_PATH];
  GetTempPathW(MAX_PATH, tempPath);
  wcscat_s(tempPath, MAX_PATH, L"fanta_webview_userdata");
  CreateDirectoryW(tempPath, NULL);

  ICoreWebView2EnvironmentOptions* opts = NULL;
  SetEnvironmentVariable("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", args);

  CreateCoreWebView2EnvironmentWithOptions(NULL, tempPath, opts, (ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*)envHandler);
}

static void webview2_resize(WebView2* web, RECT rc) {
  if (web->controller) {
    web->controller->lpVtbl->put_Bounds(web->controller, rc);
  }
}

static void webview2_cleanup(WebView2* web) {
  if (web->webview) {
    web->webview->lpVtbl->Release(web->webview);
    web->webview = NULL;
  }
  if (web->controller) {
    web->controller->lpVtbl->Release(web->controller);
    web->controller = NULL;
  }
}

static void webview2_navigate(WebView2* web, const wchar_t* url) {
  if (web->webview) {
    web->webview->lpVtbl->Navigate(web->webview, url);
  }
}

static void webview2_navigate_str(WebView2* web, const wchar_t* str) {
  if (web->webview) {
    web->webview->lpVtbl->NavigateToString(web->webview, str);
  }
}

#endif  // WEBVIEW2_EMBED_H
