#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#ifdef _MSC_VER
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#endif

#include <windows.h>
//
#include <exdisp.h>
#include <mshtml.h>
#include <ocidl.h>
#include <oleidl.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct MyClientSite {
  IOleClientSite clientSite;
  IOleInPlaceSite inplaceSite;
  ULONG refCount;
  HWND hwnd;
} MyClientSite;

HRESULT STDMETHODCALLTYPE Client_QueryInterface(IOleClientSite *This, REFIID riid, void **ppvObject) {
  MyClientSite *site = (MyClientSite *)This;
  if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleClientSite)) {
    *ppvObject = This;
  } else if (IsEqualIID(riid, &IID_IOleInPlaceSite)) {
    *ppvObject = &site->inplaceSite;
  } else {
    *ppvObject = NULL;
    return E_NOINTERFACE;
  }
  ((IUnknown *)*ppvObject)->lpVtbl->AddRef((IUnknown *)*ppvObject);
  return S_OK;
}

ULONG STDMETHODCALLTYPE Client_AddRef(IOleClientSite *This) { return ++((MyClientSite *)This)->refCount; }

ULONG STDMETHODCALLTYPE Client_Release(IOleClientSite *This) {
  MyClientSite *site = (MyClientSite *)This;
  if (--site->refCount == 0) {
    free(site);
    return 0;
  }
  return site->refCount;
}

HRESULT STDMETHODCALLTYPE Client_SaveObject(IOleClientSite *This) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE Client_GetMoniker(IOleClientSite *This, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE Client_GetContainer(IOleClientSite *This, IOleContainer **ppContainer) { return E_NOINTERFACE; }
HRESULT STDMETHODCALLTYPE Client_ShowObject(IOleClientSite *This) { return S_OK; }
HRESULT STDMETHODCALLTYPE Client_OnShowWindow(IOleClientSite *This, BOOL fShow) { return S_OK; }
HRESULT STDMETHODCALLTYPE Client_RequestNewObjectLayout(IOleClientSite *This) { return E_NOTIMPL; }

IOleClientSiteVtbl g_ClientSiteVtbl = {Client_QueryInterface, Client_AddRef, Client_Release, Client_SaveObject, Client_GetMoniker, Client_GetContainer, Client_ShowObject, Client_OnShowWindow, Client_RequestNewObjectLayout};

HRESULT STDMETHODCALLTYPE InPlace_QueryInterface(IOleInPlaceSite *This, REFIID riid, void **ppvObject) { return Client_QueryInterface((IOleClientSite *)((char *)This - offsetof(MyClientSite, inplaceSite)), riid, ppvObject); }

ULONG STDMETHODCALLTYPE InPlace_AddRef(IOleInPlaceSite *This) { return Client_AddRef((IOleClientSite *)((char *)This - offsetof(MyClientSite, inplaceSite))); }

ULONG STDMETHODCALLTYPE InPlace_Release(IOleInPlaceSite *This) { return Client_Release((IOleClientSite *)((char *)This - offsetof(MyClientSite, inplaceSite))); }

HRESULT STDMETHODCALLTYPE InPlace_GetWindow(IOleInPlaceSite *This, HWND *phwnd) {
  *phwnd = ((MyClientSite *)((char *)This - offsetof(MyClientSite, inplaceSite)))->hwnd;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE InPlace_ContextSensitiveHelp(IOleInPlaceSite *This, BOOL fEnterMode) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE InPlace_CanInPlaceActivate(IOleInPlaceSite *This) { return S_OK; }
HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceActivate(IOleInPlaceSite *This) { return S_OK; }
HRESULT STDMETHODCALLTYPE InPlace_OnUIActivate(IOleInPlaceSite *This) { return S_OK; }

HRESULT STDMETHODCALLTYPE InPlace_GetWindowContext(IOleInPlaceSite *This, IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, OLEINPLACEFRAMEINFO *lpFrameInfo) {
  HWND hwnd = ((MyClientSite *)((char *)This - offsetof(MyClientSite, inplaceSite)))->hwnd;
  GetClientRect(hwnd, lprcPosRect);
  GetClientRect(hwnd, lprcClipRect);
  return S_OK;
}

HRESULT STDMETHODCALLTYPE InPlace_Scroll(IOleInPlaceSite *This, SIZE scrollExtent) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE InPlace_OnUIDeactivate(IOleInPlaceSite *This, BOOL fUndoable) { return S_OK; }
HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceDeactivate(IOleInPlaceSite *This) { return S_OK; }
HRESULT STDMETHODCALLTYPE InPlace_DiscardUndoState(IOleInPlaceSite *This) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE InPlace_DeactivateAndUndo(IOleInPlaceSite *This) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE InPlace_OnPosRectChange(IOleInPlaceSite *This, LPCRECT lprcPosRect) { return S_OK; }

IOleInPlaceSiteVtbl g_InPlaceSiteVtbl = {InPlace_QueryInterface, InPlace_AddRef, InPlace_Release, InPlace_GetWindow, InPlace_ContextSensitiveHelp, InPlace_CanInPlaceActivate, InPlace_OnInPlaceActivate, InPlace_OnUIActivate, InPlace_GetWindowContext, InPlace_Scroll, InPlace_OnUIDeactivate, InPlace_OnInPlaceDeactivate, InPlace_DiscardUndoState, InPlace_DeactivateAndUndo, InPlace_OnPosRectChange};

MyClientSite *CreateClientSite(HWND hwnd) {
  MyClientSite *site = (MyClientSite *)calloc(1, sizeof(MyClientSite));
  site->clientSite.lpVtbl = &g_ClientSiteVtbl;
  site->inplaceSite.lpVtbl = &g_InPlaceSiteVtbl;
  site->refCount = 1;
  site->hwnd = hwnd;
  return site;
}

typedef struct {
  IOleObject *oleObject;
  IWebBrowser2 *webBrowser2;
  MyClientSite *clientSite;
  HWND containerHwnd;
} WebBrowser2;

HRESULT webbrowser_init(WebBrowser2 *browser, HWND hwnd) {
  if (!browser) return E_POINTER;
  OleInitialize(NULL);

  MyClientSite *site = CreateClientSite(hwnd);
  IOleObject *oleObj = NULL;
  IWebBrowser2 *web = NULL;

  HRESULT hr = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER, &IID_IOleObject, (void **)&oleObj);
  if (FAILED(hr)) return hr;

  hr = oleObj->lpVtbl->SetClientSite(oleObj, (IOleClientSite *)site);
  if (FAILED(hr)) return hr;

  RECT rc;
  GetClientRect(hwnd, &rc);
  hr = oleObj->lpVtbl->DoVerb(oleObj, OLEIVERB_SHOW, NULL, (IOleClientSite *)site, 0, hwnd, &rc);
  if (FAILED(hr)) return hr;

  hr = oleObj->lpVtbl->QueryInterface(oleObj, &IID_IWebBrowser2, (void **)&web);
  if (FAILED(hr)) return hr;

  web->lpVtbl->put_Silent(web, VARIANT_TRUE);  // suppress script error popups

  browser->containerHwnd = hwnd;
  browser->oleObject = oleObj;
  browser->webBrowser2 = web;
  browser->clientSite = site;

  return S_OK;
}

void webbrowser_resize(WebBrowser2 *browser, RECT rc) {
  if (!browser || !browser->oleObject) return;
  IOleInPlaceObject *inPlace = NULL;
  if (SUCCEEDED(browser->oleObject->lpVtbl->QueryInterface(browser->oleObject, &IID_IOleInPlaceObject, (void **)&inPlace))) {
    inPlace->lpVtbl->SetObjectRects(inPlace, &rc, &rc);
    inPlace->lpVtbl->Release(inPlace);
  }
}

HRESULT webbrowser_navigate(WebBrowser2 *browser, const wchar_t *url) {
  if (!browser || !browser->webBrowser2) return E_POINTER;

  VARIANT empty = {0};
  return browser->webBrowser2->lpVtbl->Navigate(browser->webBrowser2, SysAllocString(url), &empty, &empty, &empty, &empty);
}

void webbrowser_uninit(WebBrowser2 *browser) {
  if (!browser) return;

  if (browser->webBrowser2) {
    browser->webBrowser2->lpVtbl->Quit(browser->webBrowser2);
    browser->webBrowser2->lpVtbl->Release(browser->webBrowser2);
    browser->webBrowser2 = NULL;
  }

  if (browser->oleObject) {
    browser->oleObject->lpVtbl->Close(browser->oleObject, OLECLOSE_NOSAVE);
    browser->oleObject->lpVtbl->Release(browser->oleObject);
    browser->oleObject = NULL;
  }

  if (browser->clientSite) {
    browser->clientSite->clientSite.lpVtbl->Release((IOleClientSite *)browser->clientSite);
    browser->clientSite = NULL;
  }

  OleUninitialize();
}

#endif  // WEBBROWSER_H
