#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>

#define MAX_LOADSTRING 100
#define SafeDeleteObject(gdiobj) gdiobj != NULL && ::DeleteObject(gdiobj)
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
TCHAR szTitle[MAX_LOADSTRING]=L"selector1";
TCHAR szWindowClass[MAX_LOADSTRING]=L"selector2";

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndProc2(HWND, UINT, WPARAM, LPARAM);

void ShowLastError(void){
	LPVOID lpMessageBuffer;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMessageBuffer,
		0,
		NULL );

	MessageBox(NULL, (LPCWSTR)lpMessageBuffer, TEXT("Error"), MB_OK);
	LocalFree( lpMessageBuffer );
}

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

	// ��ʂ�������̂ŁA�ŏ���0,0�ŃE�C���h�E�쐬���Ƃ���
	// �����ł����炲����Ɗg��
	hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_COMPOSITED | WS_EX_TOOLWINDOW, szWindowClass, szTitle, WS_POPUP,
		0,0,0,0, NULL, NULL, hInstance, NULL);
	::SetLayeredWindowAttributes(hWnd, RGB(255,0,0), 100, LWA_COLORKEY | LWA_ALPHA);
	if(hWnd == NULL){
		::ShowLastError();
		return FALSE;
	}
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// �I��̈�
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

void TextFormatOut(HDC hdc, int x, int y, LPCTSTR format, ...)
{
	va_list arg;
	va_start(arg, format);

	TCHAR buffer[256];
	::_vsnwprintf_s(buffer, 256, _TRUNCATE, format, arg);
	::TextOut(hdc, x, y, buffer, lstrlen(buffer));
	va_end(arg);
}

void ShadowTextFormatOut(HDC hdc, int x, int y, int w, COLORREF shadow, COLORREF color, LPCTSTR format, ...)
{
	va_list arg;
	va_start(arg, format);

	TCHAR buffer[256];
	::SetBkMode(hdc, TRANSPARENT);
	::_vsnwprintf_s(buffer, 256, _TRUNCATE, format, arg);

	// �e�̕`��
	::SetTextColor(hdc, shadow);
	::TextOut(hdc, x + w, y + w, buffer, lstrlen(buffer));

	// �{�̂̕`��
	::SetTextColor(hdc, color);
	::TextOut(hdc, x, y, buffer, lstrlen(buffer));

	va_end(arg);
}

#define BUFFER_SIZE 256
void trace(LPCTSTR format, ...)
{
	va_list arg;
	va_start(arg, format);

	TCHAR buffer[BUFFER_SIZE];
	::_vsnwprintf_s(buffer, BUFFER_SIZE, _TRUNCATE, format, arg);
	::OutputDebugString(buffer);	
	va_end(arg);
}

void RectangleNormalize(RECT *rect)
{
	// ��ɍ����_�̍\���̂ɕϊ�
	if(rect->right - rect->left < 0){
		// ���E�t
		int tmp = rect->left;
		rect->left = rect->right;
		rect->right = tmp;
	}
	if(rect->bottom - rect->top < 0){
		int tmp = rect->top;
		rect->top = rect->bottom;
		rect->bottom = tmp;
	}
}

void StickRect(RECT *selected, RECT *target, int w_px, int h_px)
{
	// ����
	if(target->left <= selected->left && selected->left <= w_px){
		selected->right = target->left + (selected->right - selected->left);
		selected->left = target->left;
	}
	// �㑤
	if(target->top <= selected->top && selected->top <= h_px){
		selected->bottom = target->top + (selected->bottom - selected->top);
		selected->top = target->top;
	}
	// ����
	if(target->bottom - h_px <= selected->bottom && selected->bottom <= target->bottom){
		selected->top = target->bottom - (selected->bottom - selected->top);
		selected->bottom = target->bottom;
	}
	// �E��
	if(target->right - w_px <= selected->right && selected->right <= target->right){
		selected->left = target->right - (selected->right - selected->left);
		selected->right = target->right;
	}

	// �㉺���]�����Ƃ��̏㑤
	if(selected->bottom < target->top + h_px){
		selected->top = target->top + (selected->top - selected->bottom);
		selected->bottom = target->top;
	}
	// �㉺���]�����Ƃ��̉���
	if(target->bottom - h_px <= selected->top){
		selected->bottom = target->bottom - (selected->top - selected->bottom);
		selected->top = target->bottom;
	}
	// ���E���]�����Ƃ��̍���
	if(selected->right < target->left + w_px){
		selected->left = selected->left - selected->right;
		selected->right = target->left;
	}
	// ���E���]�����Ƃ��̉E��
	if(target->right - w_px < selected->left){
		selected->right = target->right - (selected->left - selected->right);
		selected->left = target->right;
	}
}

// �w�肳�ꂽ�E�C���h�E�͈͂���o���Ȃ����܂�
void CorrectRect(RECT *selected, RECT *target)
{
	// ����
	if(selected->left < target->left){
		selected->right = target->left + (selected->right - selected->left);
		selected->left = target->left;
	}
	// �㑤
	if(selected->top < target->top){
		selected->bottom = target->top + (selected->bottom - selected->top);
		selected->top = target->top;
	}
	// �E��
	// ...

	// �E��(�t��)
	if(selected->left > target->right){
		int w = selected->left - selected->right;
		selected->left = target->right;
		selected->right = selected->left - w;
	}

	// ����
	/*
	if(selected->bottom > target->bottom){
	int h = selected->bottom - selected->top;
	selected->bottom = target->bottom;
	selected->top = selected->bottom - h;
	}
	*/

	// ����(�t��)
	if(selected->top > target->bottom){
		int h = selected->top - selected->bottom;
		selected->top = target->bottom;
		selected->bottom = selected->top - h;
	}
}

// �w�肳�ꂽ�E�C���h�E���������܂�
BOOL HighlightWindow(HWND hWnd)
{
	HDC hdc = ::GetWindowDC(hWnd);
	if(hdc == NULL){
		return FALSE;
	}

	HPEN hPen = CreatePen(PS_SOLID, 5, RGB(255, 0, 0));
	HBRUSH hBrush = (HBRUSH)::GetStockObject(HOLLOW_BRUSH);

	HGDIOBJ hPrevPen = ::SelectObject(hdc, hPen);
	HGDIOBJ hPrevBrush = ::SelectObject(hdc, hBrush);

	RECT rect;
	::GetWindowRect(hWnd, &rect);
	::Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

	::SelectObject(hdc, hPrevPen);
	::SelectObject(hdc, hPrevBrush);

	::DeleteObject(hPen);
	::DeleteObject(hBrush);

	::ReleaseDC(hWnd, hdc);
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

		hFont = CreateFont(18,    //�t�H���g����
			0,                    //������
			0,                    //�e�L�X�g�̊p�x
			0,                    //�x�[�X���C���Ƃ����Ƃ̊p�x
			FW_REGULAR,            //�t�H���g�̏d���i�����j
			FALSE,                //�C�^���b�N��
			FALSE,                //�A���_�[���C��
			FALSE,                //�ł�������
			ANSI_CHARSET,    //�����Z�b�g
			OUT_DEFAULT_PRECIS,    //�o�͐��x
			CLIP_DEFAULT_PRECIS,//�N���b�s���O���x
			PROOF_QUALITY,        //�o�͕i��
			FIXED_PITCH | FF_MODERN,//�s�b�`�ƃt�@�~���[
			L"Tahoma");    //���̖�

		// �������f�o�C�X�R���e�L�X�g�̍쐬
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
			// ���K�����[�hON
			bNormalize = TRUE;
			return TRUE;
		}

		if(wParam == VK_SHIFT){
			bStick = TRUE;
			return TRUE;
		}

		// �Ȃ񂩃L�[�����ꂽ�璆�f���܂�
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

		// �Ȃ񂩃L�[�����ꂽ�璆�f���܂�
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

			// �`��\�ȍŏ��P�ʂ����傫�������Ƃ������B�e�\
			int w = abs(rect.right - rect.left);
			int h = abs(rect.bottom - rect.top);

			if(CAPTURE_MIN_WIDTH < w && CAPTURE_MIN_HEIGHT < h){
				::FillRect(::g_hMemDC, &rect, colorBrush);

				// �E�C���h�E�̃T�C�Y��`��
				HFONT hOldFont = SelectFont(g_hMemDC, hFont);

				// �}�E�X�J�[�\���̉E���ɍ��W��`�悷��
				POINT pt;
				::GetCursorPos(&pt);

				//int parentMode = ::SetBkMode(::g_hMemDC, TRANSPARENT);
				::SelectObject(::g_hMemDC, ::CreateSolidBrush(RGB(255,255,255)));
				::TextFormatOut(::g_hMemDC, pt.x + 10, pt.y, L"%d,%d",
					rect.right - rect.left,
					rect.bottom - rect.top);

				// font��߂�
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

			// �`��\�ȍŏ��P�ʂ����傫�������Ƃ������B�e�\
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
		// ���_������������
		::mousePressed.x = ::selected.left;
		::mousePressed.y = ::selected.top;
		break;

	case WM_MOUSEWHEEL: // �I��̈�̊g��E�k��
		{
			int v =  GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * 20; // �}�E�X�z�C�[���̈ړ���

			// ���ׂĂ̕����ɋϓ��Ɋg�傷��
			selected.left += -v;
			selected.top += -v;
			selected.right += v;
			selected.bottom += v;

			// �}�E�X���_�������Ȃ��Ƒ��Ɛ��������Ƃ�񂭂Ȃ�
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

			// �}�E�X�������|�C���g���猻�݂̈ʒu�ւ̈ړ��ʕ��Aleft/top���ړ�
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

			// �E�C���h�E�c���T�C�Y�̐��K��
			if(bNormalize){
				// ���̃T�C�Y�Ɠ��������̏c�̃T�C�Y�ɂ���
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

		// ��ʊO�ɃE�C���h�E���o�Ȃ��悤�ɂ���␳����
		CorrectRect(&selected, &windowRect);

		// �V�X�e���E�C���h�E�Ƃ̋z������
		// �ړ����ɂ����z�������͂��Ȃ�
		if(bDrag){
			if(bStick){
				// �J�[�\���̈ʒu�ɃE�C���h�E������΂��̃E�C���h�E�ɋz������
				//POINT pt;
				//::GetCursorPos(&pt);

				// ���ׂẴE�C���h�E�̂����A�������g�ȊO�ŉ��ȃE�C���h�E���璲��
				// �J�[�\�������Ԃ��Ă�������Ƃ����߂ȃE�C���h�E��T���Ď����ő傫���𒲐�����
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
