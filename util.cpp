#include "stdafx.h"
#include "AsioMP3.h"
#include "PlayerDlg.h"

#define MAX_A2W_LENGTH		512

WCHAR a2w_buff[MAX_A2W_LENGTH];

void* GetTempBuffer()
{
	return a2w_buff;
}

LPWSTR UpperExt(LPCTSTR pszSrc)
{
	for (int i = 0;; i++)
	{
		a2w_buff[i] = pszSrc[i];
		if (pszSrc[i] == 0)
			break;

		if (a2w_buff[i] >= 'a' && a2w_buff[i] <= 'z')
			a2w_buff[i] = (a2w_buff[i] - 'a') + 'A';
	}

	return a2w_buff;
}

LPWSTR A2W(LPCSTR pszSrc)
{
	MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, a2w_buff, MAX_A2W_LENGTH);
	return a2w_buff;
}

LPSTR W2A(LPCWSTR pszSrc)
{
	char* buff = (char*)a2w_buff;
	WideCharToMultiByte(CP_ACP, 0, pszSrc, -1, buff, MAX_A2W_LENGTH, NULL, NULL);
	return buff;
}

/*
LPWSTR U2W(LPCSTR pszUTF8)
{
	Utf8ToWide(a2w_buff, MAX_A2W_LENGTH, pszUTF8);
	return a2w_buff;
}

LPSTR W2U(LPCWSTR pszWide)
{
	char* buff = (char*)a2w_buff;
	WideToUtf8(buff, MAX_A2W_LENGTH, pszWide);
	return buff;
}

static int _UnicodeToUTF8(int c, BYTE* p)
{
	if (c < 0x80)
	{
		*p = c;
		return 1;
	}
	else if (c < 0x800)
	{
		p[0] = 0xC0 | (c >> 6);
		p[1] = 0x80 | (c & 0x3F);
		return 2;
	}
	else if (c < 0x10000)
	{
		p[0] = 0xE0 | (c >> 12);
		p[1] = 0x80 | ((c >> 6) & 0x3F);
		p[2] = 0x80 | (c & 0x3F);
		return 3;
	}
	//else if(c < 0x200000)
	else if (c <= 0x10FFFF)
	{
		p[0] = 0xF0 | (c >> 18);
		p[1] = 0x80 | ((c >> 12) & 0x3F);
		p[2] = 0x80 | ((c >> 6) & 0x3F);
		p[3] = 0x80 | (c & 0x3F);
		return 4;
	}

	ASSERT(!"invalid unicode");
	return 0;
}

int WideToUtf8(char* pDstUTF8, int cchMaxDst, const wchar_t* pszSrcUnicode)
{
	WCHAR ch;
	BYTE* pOut = (BYTE*)pDstUTF8;

	int lenStr = 0;

	while ((ch = *pszSrcUnicode++) != 0)
	{
		int len = _UnicodeToUTF8(ch, &pOut[lenStr]);
		ASSERT(len > 0);
		lenStr += len;
	}

	pOut[lenStr] = 0;
	return lenStr;
}

int Utf8ToWide(WCHAR* pDstUnicode, int cchMaxDst, const char* pszSrcUTF8)
{
	unsigned int ch0, ch1, ch2, ch3;
	WCHAR* uni0 = pDstUnicode;

	while (ch0 = *pszSrcUTF8++)
	{
		if (ch0 < 0x80)
		{
			*pDstUnicode++ = ch0;
		}
		else if ((ch0 & 0xE0) == 0xC0)
		{
			ch1 = *pszSrcUTF8++;
			*pDstUnicode++ = ((ch0 & 0x1F) << 6) | (ch1 & 0x3F);
		}
		else if ((ch0 & 0xF0) == 0xE0)
		{
			ch1 = *pszSrcUTF8++;
			ch2 = *pszSrcUTF8++;
			*pDstUnicode++ = ((ch0 & 0x0F) << 12) | ((ch1 & 0x3F) << 6) | (ch2 & 0x3F);
		}
		else if ((ch0 & 0xF8) == 0xF0)
		{
			ch1 = *pszSrcUTF8++;
			ch2 = *pszSrcUTF8++;
			ch3 = *pszSrcUTF8++;
			*pDstUnicode++ = ((ch0 & 0x07) << 18) | ((ch1 & 0x3F) << 12)
				| ((ch2 & 0x3F) << 6) | (ch3 & 0x3F);
		}
		else
		{
			ASSERT(!"invalid utf8");
			return -1;
		}
	}

	*pDstUnicode = 0;
	return (int)(pDstUnicode - uni0);
}
*/

///////////////////////////////////////////////////////////////////////////////

static LPCTSTR s_pszMsgBoxTitle = _T("ASIO MP3 Player");

int MsgBox(LPCTSTR pszMessage, UINT uType)
{
	return MessageBox(g_playerDlg.GetHwnd(), pszMessage, s_pszMsgBoxTitle, uType);
}

int MsgBoxF(UINT uType, LPCTSTR pszFormat, ...)
{
	TCHAR szBuffer[1024];

	va_list ap;
	va_start(ap, pszFormat);
	StringCbVPrintf(szBuffer, sizeof(szBuffer), pszFormat, ap);
	va_end(ap);

	return MessageBox(g_playerDlg.GetHwnd(), szBuffer, s_pszMsgBoxTitle, uType);
}
