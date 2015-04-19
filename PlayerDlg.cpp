#include "stdafx.h"
#include "AsioMP3.h"
#include "PlayerDlg.h"
#include "DecodeMP3.h"

#define IDT_REFRESH 1234

PlayerDlg::PlayerDlg()
{
	m_hWnd = NULL;
}

void PlayerDlg::Show()
{
	m_hSysFont = NULL;
	m_pMP3 = NULL;

	m_bAsioDriverLoaded = false;
	m_bAsioInitialized = false;
	m_bAsioBufferCreated = false;
	m_bAsioStarted = false;

	DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_PLAYER), NULL, DlgProc, (LPARAM) this);

	OnDestroy();
}

void PlayerDlg::OnInitDialog()
{
	m_hLogBox = GetDlgItem(m_hWnd, IDC_MESSAGE);
	m_hOutputDeviceList = GetDlgItem(m_hWnd, IDC_OUTDEV);

	m_hSysFont = CreateFont(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));

	SendMessage(m_hLogBox, WM_SETFONT, (WPARAM)m_hSysFont, TRUE);

	CreateAsio();

	EnableWindow(GetDlgItem(m_hWnd, IDC_LOAD), m_bAsioBufferCreated);
	EnableWindow(GetDlgItem(m_hWnd, IDC_PLAY), FALSE);
}

void PlayerDlg::OnDestroy()
{
	if (m_pMP3 != NULL)
	{
		m_pMP3->Close();
		delete m_pMP3;
		m_pMP3 = NULL;
	}

	DestroyAsio();

	if (m_hSysFont)
	{
		DeleteObject(m_hSysFont);
	}

	m_hWnd = NULL;
}

void PlayerDlg::LogPrint(LPCTSTR szFormat, ...)
{
	TCHAR szBuffer[1024];
	
	va_list ap;
	va_start(ap, szFormat);
	StringCchVPrintf(szBuffer, sizeof(szBuffer) / sizeof(TCHAR), szFormat, ap);
	va_end(ap);

	int n = lstrlen(szBuffer);
	szBuffer[n] = '\r';
	szBuffer[n + 1] = '\n';
	szBuffer[n + 2] = 0;

	int len = GetWindowTextLength(m_hLogBox);
	SendMessage(m_hLogBox, EM_SETSEL, len, len);
	SendMessage(m_hLogBox, EM_REPLACESEL, FALSE, (LPARAM)szBuffer);
	SendMessage(m_hLogBox, EM_SCROLL, SB_PAGEDOWN, 0);
}

void PlayerDlg::OnTimer()
{
	//UpdateAsioState();
}

void PlayerDlg::OnLoadFile()
{
	OPENFILENAME ofn;
	TCHAR szPathname[MAX_PATH] = { 0 };
	TCHAR szFilename[MAX_PATH] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.nMaxFile = MAX_PATH;	// max file name length

	ofn.lpstrFile = szPathname;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = szFilename;
	ofn.nMaxFileTitle = MAX_PATH;

	ofn.lpstrFilter = _T("MP3 Audio File (*.mp3)\0*.mp3\0\0");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrTitle = _T("Select MP3 file...");

	if (!GetOpenFileName(&ofn))
	{
		return;	// canceled
	}

	ASSERT(m_pMP3 == NULL);

	m_pMP3 = new DecodeMP3();
	if (!m_pMP3->Open(szPathname))
	{
		delete m_pMP3;
		m_pMP3 = NULL;
		return;
	}

	OnPlayOrStop();
}

void PlayerDlg::OnStop()
{
	// stop
	KillTimer(m_hWnd, IDT_REFRESH);
	StopAsio();

	if (m_pMP3)
	{
		m_pMP3->Close();
		delete m_pMP3;
		m_pMP3 = NULL;
	}

	EnableWindow(GetDlgItem(m_hWnd, IDC_PLAY), FALSE);
	EnableWindow(GetDlgItem(m_hWnd, IDC_LOAD), TRUE);
}

void PlayerDlg::OnPlayOrStop()
{
	if (m_bAsioStarted)
	{
		OnStop();
		return;
	}

	StartAsio();

	if (m_bAsioStarted)
	{
		SetTimer(m_hWnd, IDT_REFRESH, 1000, NULL);

		EnableWindow(GetDlgItem(m_hWnd, IDC_LOAD), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_PLAY), TRUE);
	}
}

void PlayerDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_LOAD:
		OnLoadFile();
		break;

	case IDC_PLAY:
		OnPlayOrStop();
		break;

	case IDCANCEL:
		EndDialog(m_hWnd, IDCANCEL);
		break;
	}
}

INT_PTR CALLBACK PlayerDlg::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static PlayerDlg* pWnd;

	switch (message)
	{
	case WM_TIMER:
		if (wParam == IDT_REFRESH)
			pWnd->OnTimer();
		break;

	case WM_INITDIALOG:
		pWnd = (PlayerDlg*)lParam;
		pWnd->m_hWnd = hWnd;
		pWnd->OnInitDialog();
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		pWnd->OnCommand(wParam, lParam);
		break;

	case WM_USER_LOGMSG:
		pWnd->LogPrint(_T("%s"), (LPCTSTR)lParam);
		break;

	case WM_STOP_ASIO:
		pWnd->OnStop();
		break;
	}

	return (INT_PTR)FALSE;
}

