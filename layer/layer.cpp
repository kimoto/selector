﻿#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include "Util.h"

#define CAPTURE_MIN_WIDTH 1
#define CAPTURE_MIN_HEIGHT 1

POINT mousePressed = {0};
POINT mousePressedR = {0};
BOOL bCapture = FALSE;
BOOL bDrag = FALSE;
BOOL bNormalize = FALSE;
BOOL bStick = FALSE;
RECT selected = {0};
RECT lastSelected = {0};
HDC g_hMemDC;

HINSTANCE hInst;
TCHAR szTitle[]=L"selector1";
TCHAR szWindowClass[]=L"selector2";

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndProc2(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	MSG msg;

	MyRegisterClass(hInstance);
	if (!InitInstance (hInstance, nCmdShow)) {
		return FALSE;
	}

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex, wcex2;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= 0;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

	wcex2.cbSize = sizeof(WNDCLASSEX);
	wcex2.style			= 0;
	wcex2.lpfnWndProc	= WndProc2;
	wcex2.cbClsExtra		= 0;
	wcex2.cbWndExtra		= 0;
	wcex2.hInstance		= hInstance;
	wcex2.hIcon			= NULL;
	wcex2.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex2.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex2.lpszMenuName	= NULL;
	wcex2.lpszClassName	= szWindowClass;
	wcex2.hIconSm		= NULL;
	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	hInst = hInstance;

	RECT rect;
	::GetWindowRect(::GetDesktopWindow(), &rect);
	rect.right = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	rect.bottom = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// 画面がちらつくので、最初は0,0でウインドウ作成しといて
	// 準備できたらごりっと拡大
	hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_COMPOSITED | WS_EX_TOOLWINDOW, szWindowClass, szTitle, WS_POPUP,
		0,0,0,0, NULL, NULL, hInstance, NULL);
	::SetLayeredWindowAttributes(hWnd, RGB(255,0,0), 100, LWA_COLORKEY | LWA_ALPHA);
	if(hWnd == NULL){
		::ShowLastError();
		return FALSE;
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// 選択領域
	HWND hWnd2 = ::CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, szWindowClass, szTitle, WS_POPUP,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
	if(hWnd == NULL){
		::ShowLastError();
		return FALSE;
	}
	ShowWindow(hWnd2, SW_SHOW);
	UpdateWindow(hWnd2);

	::MoveWindow(hWnd, rect.left, rect.top, rect.right, rect.bottom, TRUE);
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	static RECT windowRect;
	static HBRUSH transparentBrush = NULL;
	static HBRUSH colorBrush = NULL;
	static HBRUSH colorBrush2 = NULL;
	static HBITMAP hOldBitmap = NULL;
	static HBITMAP hBitmap = NULL;
	static HFONT hFont;

	switch (message)
	{
	case WM_CREATE:
		::GetClientRect(hWnd, &windowRect);
		transparentBrush = ::CreateSolidBrush(RGB(255,0,0));
		colorBrush = ::CreateSolidBrush(RGB(254,0,0));
		colorBrush2 = ::CreateSolidBrush(RGB(0,254,0));

		hFont = CreateFont(18,    //フォント高さ
			0,                    //文字幅
			0,                    //テキストの角度
			0,                    //ベースラインとｘ軸との角度
			FW_REGULAR,            //フォントの重さ（太さ）
			FALSE,                //イタリック体
			FALSE,                //アンダーライン
			FALSE,                //打ち消し線
			ANSI_CHARSET,    //文字セット
			OUT_DEFAULT_PRECIS,    //出力精度
			CLIP_DEFAULT_PRECIS,//クリッピング精度
			PROOF_QUALITY,        //出力品質
			FIXED_PITCH | FF_MODERN,//ピッチとファミリー
			L"Tahoma");    //書体名

		// メモリデバイスコンテキストの作成
		{
			g_hMemDC = ::CreateCompatibleDC(::GetDC(hWnd));
			hBitmap = ::CreateCompatibleBitmap(::GetDC(hWnd), windowRect.right, windowRect.bottom);
			hOldBitmap = (HBITMAP)::SelectObject(g_hMemDC, hBitmap);
		}
		return TRUE;
	case WM_COMMAND:
		return DefWindowProc(hWnd, message, wParam, lParam);

	case WM_KEYDOWN:
		if(wParam == VK_CONTROL){
			// 正規化モードON
			bNormalize = TRUE;
			return TRUE;
		}

		if(wParam == VK_SHIFT){
			bStick = TRUE;
			return TRUE;
		}

		// なんかキーおされたら中断します
		bCapture = FALSE;
		::InvalidateRect(hWnd, NULL, FALSE);
		::DestroyWindow(hWnd);
		break;

	case WM_KEYUP:
		if(wParam == VK_CONTROL){
			bNormalize = FALSE;
			return TRUE;
		}

		if(wParam == VK_SHIFT){
			bStick = FALSE;
			return TRUE;
		}

		// なんかキーおされたら中断します
		bCapture = FALSE;
		::InvalidateRect(hWnd, NULL, FALSE);
		::DestroyWindow(hWnd);
		break;

	case WM_ERASEBKGND:
		return FALSE;

	case WM_PAINT:
		::FillRect(::g_hMemDC, &windowRect, transparentBrush);
		if(bCapture){
			RECT rect = selected;
			RectangleNormalize(&rect);

			// 描画可能な最小単位よりも大きかったときだけ撮影可能
			int w = abs(rect.right - rect.left);
			int h = abs(rect.bottom - rect.top);

			if(CAPTURE_MIN_WIDTH < w && CAPTURE_MIN_HEIGHT < h){
				::FillRect(::g_hMemDC, &rect, colorBrush);

				// ウインドウのサイズを描画
				HFONT hOldFont = SelectFont(g_hMemDC, hFont);

				// マウスカーソルの右下に座標を描画する
				POINT pt;
				::GetCursorPos(&pt);

				//int parentMode = ::SetBkMode(::g_hMemDC, TRANSPARENT);
				::SelectObject(::g_hMemDC, ::CreateSolidBrush(RGB(255,255,255)));
				::TextFormatOut(::g_hMemDC, pt.x + 10, pt.y, L"%d,%d",
					rect.right - rect.left,
					rect.bottom - rect.top);

				// fontを戻す
				SelectFont(g_hMemDC, hOldFont);
			}else{
				::FillRect(::g_hMemDC, &selected, colorBrush2);
			}
		}

		hdc = BeginPaint(hWnd, &ps);
		::BitBlt(hdc, 0, 0, windowRect.right, windowRect.bottom, ::g_hMemDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		return TRUE;
	case WM_DESTROY:
		::SelectObject(::g_hMemDC, hOldBitmap);
		SafeDeleteObject(::g_hMemDC);
		SafeDeleteObject(transparentBrush);
		SafeDeleteObject(colorBrush);
		SafeDeleteObject(colorBrush2);
		SafeDeleteObject(hFont);
		PostQuitMessage(0);
		return TRUE;
	case WM_LBUTTONDOWN:
		if(!bCapture)
		{
			POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			mousePressed = point;
			bCapture = TRUE;
		}
		return TRUE;
	case WM_LBUTTONUP:
		if(bCapture){
			POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::InvalidateRect(hWnd, NULL, FALSE);

			// 描画可能な最小単位よりも大きかったときだけ撮影可能
			int w = abs(selected.right - selected.left);
			int h = abs(selected.bottom - selected.top);

			if(CAPTURE_MIN_WIDTH < w && CAPTURE_MIN_HEIGHT < h){
				::InvalidateRect(hWnd, NULL, FALSE);
				::MessageBeep(MB_ICONASTERISK);
				::DestroyWindow(hWnd);
				return TRUE;
			}

			::DestroyWindow(hWnd);
		}
		bCapture = FALSE;
		return TRUE;
	case WM_RBUTTONDOWN:
		bDrag = TRUE;
		::mousePressedR.x = GET_X_LPARAM(lParam);
		::mousePressedR.y = GET_Y_LPARAM(lParam);
		lastSelected = selected;
		break;
	case WM_RBUTTONUP:
		bDrag = FALSE;
		// 原点を書き換える
		::mousePressed.x = ::selected.left;
		::mousePressed.y = ::selected.top;
		break;

	case WM_MOUSEWHEEL: // 選択領域の拡大・縮小
		{
			int v =  GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * 20; // マウスホイールの移動量

			// すべての方向に均等に拡大する
			selected.left += -v;
			selected.top += -v;
			selected.right += v;
			selected.bottom += v;

			// マウス原点もかえないと他と整合性がとれんくなる
			::mousePressed.x = selected.left;
			::mousePressed.y = selected.top;

			::InvalidateRect(hWnd, NULL, FALSE);
		}
		break;

	case WM_MOUSEMOVE:
		if(bDrag){
			POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			int w = lastSelected.right - lastSelected.left;
			int h = lastSelected.bottom - lastSelected.top;

			// マウス押したポイントから現在の位置への移動量分、left/topを移動
			int x = (point.x - mousePressedR.x);
			int y = (point.y - mousePressedR.y);

			selected.left = lastSelected.left + point.x - mousePressedR.x;
			selected.top = lastSelected.top + point.y - mousePressedR.y;
			selected.right = selected.left + w;
			selected.bottom = selected.top + h;

		}else if(bCapture){
			POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			selected.left = mousePressed.x;
			selected.top = mousePressed.y;
			selected.right = point.x;
			selected.bottom = point.y;

			// ウインドウ縦横サイズの正規化
			if(bNormalize){
				// 横のサイズと同じだけの縦のサイズにする
				if(selected.left < selected.right){
					if(selected.top < selected.bottom){
						selected.bottom = selected.top + (selected.right - selected.left);
					}else{
						selected.bottom = selected.top - (selected.right - selected.left);
					}
				}else{
					if(selected.top < selected.bottom){
						selected.bottom = selected.top + (selected.left - selected.right);
					}else{
						selected.bottom = selected.top + (selected.right - selected.left);
					}
				}
			}
		}

		// 画面外にウインドウが出ないようにする補正処理
		CorrectRect(&selected, &windowRect);

		// システムウインドウとの吸着処理
		// 移動中にしか吸着処理はしない
		if(bDrag){
			if(bStick){
				// カーソルの位置にウインドウがあればそのウインドウに吸着する
				//POINT pt;
				//::GetCursorPos(&pt);

				// すべてのウインドウのうち、自分自身以外で可視なウインドウから調査
				// カーソルがかぶっているもっとも直近なウインドウを探して自動で大きさを調整する
				//selected = rect;
				StickRect(&selected, &windowRect, 50, 50);
			}
		}

		::InvalidateRect(hWnd, NULL, FALSE);
		return TRUE;
	case WM_SIZE:
		return TRUE;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return TRUE;
}

LRESULT CALLBACK WndProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message){
	case WM_ERASEBKGND:
		return FALSE;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
