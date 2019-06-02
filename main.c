#pragma comment(linker, "/OPT:NOWIN98")
#include <windows.h>
#include "resource.h"

IStream* CreateStreamFromResource(LPCWSTR lpName, LPCWSTR lpType)
{
	HRSRC hrsrc = FindResourceW(NULL, lpName, lpType);
	if (hrsrc) {
		DWORD dwResourceSize = SizeofResource(NULL, hrsrc);
		HGLOBAL hResource = LoadResource(NULL, hrsrc);
		if (hResource) {
			LPVOID pvResource = LockResource(hResource);
			if (pvResource) {
				HGLOBAL hMemory = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
				if (hMemory) {
					LPVOID pvMemory = GlobalLock(hMemory);
					if (pvMemory) {
						CopyMemory(pvMemory, pvResource, dwResourceSize);
						GlobalUnlock(hMemory);
						{
							IStream *stream;
							if (CreateStreamOnHGlobal(hMemory, TRUE, &stream) == S_OK)
								return stream;
						}
					}
					GlobalFree(hMemory);
				}
			}
		}
	}
	return NULL;
}

HBITMAP ConvertDIBtoDDB(HBITMAP hBitmap)
{
	HBITMAP hbitmap = NULL;
	HDC hdc = GetDC(NULL);
	if (hdc) {
		DIBSECTION ds;
		GetObject(hBitmap, sizeof(ds), &ds);
		ds.dsBmih.biCompression = BI_RGB;
		hbitmap = CreateDIBitmap(hdc, &ds.dsBmih, CBM_INIT, ds.dsBm.bmBits, (BITMAPINFO *)&ds.dsBmih, DIB_RGB_COLORS);
		ReleaseDC(NULL, hdc);
	}
	return hbitmap;
}

void DisplayTime(HBITMAP hBitmap)
{
	HFONT hfont = CreateFont(11, 0, 0, 0, FW_BOLD, 0, 0, 0, HANGEUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, "Tahoma");
	if (hfont) {
		HDC hdc = CreateCompatibleDC(NULL);
		if (hdc) {
			SelectObject(hdc, hBitmap);
			SelectObject(hdc, hfont);
			{
				char a[128];
				SYSTEMTIME t;
				GetLocalTime(&t);
				wsprintf(a, "%04u.%02u.%02u %02u:%02u:%02u", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
				{
					int x = 2, y = 0, n = (int)strlen(a);
					SetBkMode(hdc, TRANSPARENT);
					SetTextColor(hdc, RGB(255, 255, 255));
					TextOut(hdc, x, y - 1, a, n);
					TextOut(hdc, x + 1, y, a, n);
					TextOut(hdc, x, y + 1, a, n);
					TextOut(hdc, x - 1, 0, a, n);
					SetTextColor(hdc, RGB(0, 0, 0));
					TextOut(hdc, x, y, a, n);
				}
			}
			DeleteDC(hdc);
		}
		DeleteObject(hfont);
	}
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nSnowCmd)
{
	HMODULE m = LoadLibraryW(L"GDIPLUS.DLL");
	if (m) {
		PROC GdiplusStartup_;
		PROC GdiplusShutdown_;
		PROC GdipCreateBitmapFromStream_;
		PROC GdipCreateHBITMAPFromBitmap_;
		PROC GdipDisposeImage_;
		if ((GdiplusStartup_ = GetProcAddress(m, "GdiplusStartup")) &&
			(GdiplusShutdown_ = GetProcAddress(m, "GdiplusShutdown")) &&
			(GdipCreateBitmapFromStream_ = GetProcAddress(m, "GdipCreateBitmapFromStream")) &&
			(GdipCreateHBITMAPFromBitmap_ = GetProcAddress(m, "GdipCreateHBITMAPFromBitmap")) &&
			(GdipDisposeImage_ = GetProcAddress(m, "GdipDisposeImage"))) {
			ULONG_PTR token;
			int a[4] = {1, 0, 0, 0};
			if (!GdiplusStartup_(&token, a, 0)) {
				IStream *stream = CreateStreamFromResource((wchar_t *)1, L"PNG");
				if (stream) {
					void *bitmap;
					if (GdipCreateBitmapFromStream_(stream, &bitmap) == S_OK) {
						HBITMAP hbitmap;
						if (GdipCreateHBITMAPFromBitmap_(bitmap, &hbitmap, 0) == S_OK) {
							// Gdiplus gives handle to DIB. so convert it to DDB
							HBITMAP hbitmap2 = ConvertDIBtoDDB(hbitmap);
							if (hbitmap2) {
								DisplayTime(hbitmap2);
								if (OpenClipboard(NULL)) {
									EmptyClipboard();
									SetClipboardData(CF_BITMAP, hbitmap2);
									CloseClipboard();
									MessageBox(NULL, "Image Copied", 0, MB_ICONINFORMATION | MB_SYSTEMMODAL);
								}
								DeleteObject(hbitmap2);
							}
							DeleteObject(hbitmap);
						}
						GdipDisposeImage_(bitmap);
					}
					stream->lpVtbl->Release(stream);
				}
				GdiplusShutdown_(token);
			}
		}
		FreeLibrary(m);
	}
	return 0;
}