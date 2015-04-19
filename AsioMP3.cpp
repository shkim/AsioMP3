#include "stdafx.h"
#include "AsioMP3.h"
#include "PlayerDlg.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' " \
	"version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "winmm.lib")

HINSTANCE g_hInstance;
PlayerDlg g_playerDlg;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HMODULE hRichEdit = NULL;

	g_hInstance = hInstance;

	hRichEdit = LoadLibrary(_T("RICHED32"));

	g_playerDlg.Show();

	if (hRichEdit)
		FreeLibrary(hRichEdit);
	
	return 0;
}