#include "resource.h"
#include "utils.h"

#include <shlobj.h>
#include <shellapi.h>
#include <pathcch.h>
#include <commdlg.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.Foundation.h>
#pragma comment(lib, "RuntimeObject")
#pragma comment(lib, "Pathcch")

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

CHOOSECOLORW cc{ 0 };

HANDLE hProcess{};
BYTE* address1;
BYTE* address2;

winrt::Windows::UI::ViewManagement::UISettings ui;

HANDLE hFile;
Config config{ 0 };

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int) {

	HANDLE hMutex = CreateMutexW(nullptr, FALSE, L"ColorfulDesktopSelectMutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		HWND hwnd = FindWindowW(nullptr, APP_NAME);
		if (hwnd) { SendMessageW(hwnd, WM_APP_VIEW, 0, WM_LBUTTONUP); }
		return 0;
	}

	winrt::init_apartment();

	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_SOLIDCOLOR;
	cc.lpCustColors = config.customColors;
	cc.rgbResult = config.color;

	if (!Initialize(&hProcess, &address1, &address2, &config, &hFile)) { return 1; }

	HWND hwnd = CreateDialogW(hInstance, MAKEINTRESOURCE(IDD_MAIN), nullptr, DialogProc);
	if (hwnd == INVALID_HANDLE_VALUE) {
		CloseHandle(hProcess);
		return 2;
	}
	wchar_t path[MAX_PATH] = { 0 };
	GetModuleFileNameW(nullptr, path, MAX_PATH);
	PathCchRemoveFileSpec(path, MAX_PATH);

	KNOWNFOLDERID kfID = FOLDERID_Startup;
	wchar_t* startup;
	if (SHGetKnownFolderPath(kfID, 0, nullptr, &startup) == S_OK) {
		if (wcscmp(path, startup) == 0) {
			ShowWindow(hwnd, SW_HIDE);
			PostMessageW(hwnd, WM_COMMAND, IDC_APPLY, 0);
		}
		else { ShowWindow(hwnd, SW_SHOW); }
		CoTaskMemFree(startup);
	}

	ui.ColorValuesChanged(
		[hwnd](winrt::Windows::UI::ViewManagement::UISettings const&, winrt::Windows::Foundation::IInspectable const&)
		{ if (config.useAccentColor) { PostMessageW(hwnd, WM_COMMAND, IDC_APPLY, 0); }});

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		if (!IsDialogMessageW(hwnd, &msg)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	CloseHandle(hProcess);
	CloseHandle(hFile);

	return 0;
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
	switch (msg) {
	case WM_INITDIALOG:
	{
		HICON hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCE(IDI_APP_ICON));
		SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		NOTIFYICONDATAW nid = { 0 };
		nid.cbSize = sizeof(NOTIFYICONDATAW);
		nid.hWnd = hwnd;
		nid.uID = 1;
		nid.uCallbackMessage = WM_APP_VIEW;
		nid.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;
		nid.hIcon = hIcon;
		wcscpy_s(nid.szTip, L"Colorful Desktop Select");

		Shell_NotifyIconW(NIM_ADD, &nid);
		cc.rgbResult = config.color;
		if (config.useAccentColor) { CheckDlgButton(hwnd, IDC_ACCENT, BST_CHECKED); }
		EnableWindow(GetDlgItem(hwnd, IDC_APPLY), FALSE);
	}
	return TRUE;

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)l;

        if (dis->CtlID == IDC_COLOR)
        {
            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;

            HBRUSH brush = CreateSolidBrush(config.color);
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(186, 186, 186));

            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            HGDIOBJ oldPen = SelectObject(hdc, pen);

            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);

            DeleteObject(brush);
            DeleteObject(pen);
            return TRUE;
        }
    }
    break;

	case WM_APP_VIEW:
		if (l == WM_LBUTTONUP || l == WM_RBUTTONUP) {
			if (IsWindowVisible(hwnd)) { ShowWindow(hwnd, SW_HIDE); }
			else { ShowWindow(hwnd, SW_SHOW); }
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(w))
		{
		case IDC_ACCENT:
			config.useAccentColor = IsDlgButtonChecked(hwnd, IDC_ACCENT);
			EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
			return TRUE;

		case IDC_COLOR:
			if (ChooseColorW(&cc)) {
				EnableWindow(GetDlgItem(hwnd, IDC_APPLY), TRUE);
				InvalidateRect((HWND)l, NULL, TRUE);
				config.color = cc.rgbResult;
			}
			return TRUE;

		case IDC_HIDE:
			ShowWindow(hwnd, SW_HIDE);
			return TRUE;

		case IDC_APPLY:
			if (config.useAccentColor) {
				auto ac = ui.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Accent);
				config.color = RGB(ac.R, ac.G, ac.B);
				InvalidateRect(GetDlgItem(hwnd, IDC_COLOR), NULL, TRUE);
			}
			SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
			WriteFile(hFile, &config, sizeof(Config), nullptr, nullptr);
			if (!ApplyPatch(hProcess, address1, address2, config.color)) { PostQuitMessage(0); }
			EnableWindow(GetDlgItem(hwnd, IDC_APPLY), FALSE);
			return TRUE;
		}
		break;		

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return TRUE;

	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
	}
	return FALSE;
}