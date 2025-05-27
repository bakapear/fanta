#ifndef WEBVIEW2_EMBED_H
#define WEBVIEW2_EMBED_H

#define WM_WEBVIEW2_READY (WM_USER + 100)

#define COBJMACROS
#include <windows.h>
//
#include <shlwapi.h>
#include <stdlib.h>
#include <wchar.h>

#include "webview/WebView2.h"

typedef struct {
  ICoreWebView2Controller* controller;
  ICoreWebView2* webview;
  ICoreWebView2Environment* env;
} WebView2;

typedef struct {
  ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl* lpVtbl;
  ULONG refCount;
  HWND hwnd;
  WebView2* web;
} ControllerHandler;

typedef struct {
  ICoreWebView2WebResourceRequestedEventHandlerVtbl* lpVtbl;
  ULONG refCount;
  HWND hwnd;
  WebView2* web;
  ICoreWebView2Environment* env;
} WebResourceRequestedHandler;

HRESULT LoadResourceStreamFromAppAssets(LPCWSTR path, IStream** stream) {
  WCHAR pathQuoted[256];
  swprintf_s(pathQuoted, 256, L"\"%s\"", path);

  HRSRC hrsrc = FindResourceW(NULL, pathQuoted, RT_HTML);
  if (!hrsrc) return HRESULT_FROM_WIN32(GetLastError());

  HGLOBAL hglob = LoadResource(NULL, hrsrc);
  if (!hglob) return HRESULT_FROM_WIN32(GetLastError());

  void* data = LockResource(hglob);
  DWORD size = SizeofResource(NULL, hrsrc);
  if (!data || size == 0) return E_FAIL;

  *stream = SHCreateMemStream((const BYTE*)data, size);
  return (*stream != NULL) ? S_OK : E_FAIL;
}

static HRESULT STDMETHODCALLTYPE WebResourceRequested_QueryInterface(ICoreWebView2WebResourceRequestedEventHandler* this, REFIID riid, void** ppvObject) {
  if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2WebResourceRequestedEventHandler)) {
    *ppvObject = this;
    this->lpVtbl->AddRef(this);
    return S_OK;
  }
  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE WebResourceRequested_AddRef(ICoreWebView2WebResourceRequestedEventHandler* this) { return ++((WebResourceRequestedHandler*)this)->refCount; }

static ULONG STDMETHODCALLTYPE WebResourceRequested_Release(ICoreWebView2WebResourceRequestedEventHandler* this) {
  WebResourceRequestedHandler* self = (WebResourceRequestedHandler*)this;
  ULONG ref = --self->refCount;
  if (ref == 0) {
    if (self->env) {
      self->env->lpVtbl->Release(self->env);
    }
    free(self);
  }
  return ref;
}

static LPCWSTR GetMimeType(LPCWSTR path) {
  LPCWSTR ext = PathFindExtensionW(path);
  if (_wcsicmp(ext, L".js") == 0) return L"text/javascript";
  if (_wcsicmp(ext, L".css") == 0) return L"text/css";
  if (_wcsicmp(ext, L".html") == 0 || _wcsicmp(ext, L".htm") == 0) return L"text/html";
  if (_wcsicmp(ext, L".json") == 0) return L"application/json";
  if (_wcsicmp(ext, L".png") == 0) return L"image/png";
  if (_wcsicmp(ext, L".jpg") == 0 || _wcsicmp(ext, L".jpeg") == 0) return L"image/jpeg";
  if (_wcsicmp(ext, L".svg") == 0) return L"image/svg+xml";
  return L"application/octet-stream";
}

static HRESULT STDMETHODCALLTYPE WebResourceRequested_Invoke(ICoreWebView2WebResourceRequestedEventHandler* this, ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args) {
  WebResourceRequestedHandler* self = (WebResourceRequestedHandler*)this;
  ICoreWebView2WebResourceRequest* request = NULL;
  args->lpVtbl->get_Request(args, &request);

  LPWSTR uri = NULL;
  request->lpVtbl->get_Uri(request, &uri);

  if (uri && wcsncmp(uri, L"file://app/", 11) == 0) {
    LPCWSTR path = uri + 11;

    IStream* stream = NULL;
    HRESULT hr = LoadResourceStreamFromAppAssets(path, &stream);

    WCHAR header[128];

    if (SUCCEEDED(hr) && stream) {
      LPCWSTR mimeType = GetMimeType(path);
      swprintf_s(header, 128, L"Content-Type: %s\r\n", mimeType);

      ICoreWebView2WebResourceResponse* response = NULL;
      hr = self->env->lpVtbl->CreateWebResourceResponse(self->env, stream, 200, L"OK", header, &response);

      if (SUCCEEDED(hr)) {
        args->lpVtbl->put_Response(args, response);
        response->lpVtbl->Release(response);
      }

      stream->lpVtbl->Release(stream);
    } else {
      IStream* emptyStream = SHCreateMemStream(NULL, 0);
      if (emptyStream) {
        swprintf_s(header, 128, L"Content-Type: text/plain\r\n");

        ICoreWebView2WebResourceResponse* response = NULL;
        hr = self->env->lpVtbl->CreateWebResourceResponse(self->env, emptyStream, 404, L"Not Found", header, &response);

        if (SUCCEEDED(hr)) {
          args->lpVtbl->put_Response(args, response);
          response->lpVtbl->Release(response);
        }

        emptyStream->lpVtbl->Release(emptyStream);
      }
    }
  }

  if (uri) CoTaskMemFree(uri);
  if (request) request->lpVtbl->Release(request);
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE Controller_QueryInterface(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* this, REFIID riid, void** ppvObject) {
  if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)) {
    *ppvObject = this;
    this->lpVtbl->AddRef(this);
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

static ICoreWebView2WebResourceRequestedEventHandlerVtbl g_webResourceRequestedVtbl = {WebResourceRequested_QueryInterface, WebResourceRequested_AddRef, WebResourceRequested_Release, WebResourceRequested_Invoke};

static HRESULT STDMETHODCALLTYPE Controller_Invoke(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* this, HRESULT result, ICoreWebView2Controller* controller) {
  if (controller) {
    ControllerHandler* self = (ControllerHandler*)this;
    self->web->controller = controller;
    controller->lpVtbl->AddRef(controller);

    controller->lpVtbl->get_CoreWebView2(controller, &self->web->webview);

    RECT bounds;
    GetClientRect(self->hwnd, &bounds);
    controller->lpVtbl->put_Bounds(controller, bounds);

    self->web->webview->lpVtbl->AddWebResourceRequestedFilter(self->web->webview, L"file://app/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

    WebResourceRequestedHandler* resourceHandler = (WebResourceRequestedHandler*)malloc(sizeof(WebResourceRequestedHandler));
    resourceHandler->lpVtbl = &g_webResourceRequestedVtbl;
    resourceHandler->refCount = 1;
    resourceHandler->hwnd = self->hwnd;
    resourceHandler->web = self->web;
    resourceHandler->env = self->web->env;
    resourceHandler->env->lpVtbl->AddRef(resourceHandler->env);

    EventRegistrationToken token;
    self->web->webview->lpVtbl->add_WebResourceRequested(self->web->webview, (ICoreWebView2WebResourceRequestedEventHandler*)resourceHandler, &token);

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
    this->lpVtbl->AddRef(this);
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

    self->web->env = env;
    env->lpVtbl->AddRef(env);

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
