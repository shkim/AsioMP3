#include "stdafx.h"
#include "AsioMP3.h"
#include "PlayerDlg.h"
#include "DecodeMP3.h"

#define MAX_QUEUE_ITEM_COUNT	16

DecodeMP3::DecodeMP3()
{
	m_nQueueItemCount = 0;
	m_pDecoder = NULL;
	m_pRawFile = NULL;
}

DecodeMP3::~DecodeMP3()
{
	ASSERT(m_queue.empty());
	ASSERT(m_freeItems.empty());
	ASSERT(m_pDecoder == NULL);
	ASSERT(m_pRawFile == NULL);
}

static TCHAR* GetID3Tag(BYTE* p, int len)
{
	char buff[128];

	strncpy(buff, (char*)p, len);
	buff[len] = 0;

	return A2W(buff);
}

bool DecodeMP3::Open(LPCTSTR pszPathname)
{
	FILE* fp;
	_tfopen_s(&fp, pszPathname, _T("rb"));
	if (fp == NULL)
	{
		g_playerDlg.LogPrint(_T("MP3 File open failed: %s"), pszPathname);
		return false;
	}

	g_playerDlg.LogPrint(_T("Open MP3 file: %s"), pszPathname);

	fseek(fp, 0, SEEK_END);
	m_cbFileSize = ftell(fp);
	if (m_cbFileSize > 0)
	{
		rewind(fp);
		m_pRawFile = (BYTE*)malloc(m_cbFileSize);
		fread(m_pRawFile, 1, m_cbFileSize, fp);
	}
	fclose(fp);

	if (m_cbFileSize < 256)
	{
		g_playerDlg.LogPrint(_T("Invalid MP3 file size: %d bytes"), m_cbFileSize);
		return false;
	}

	// check for a ID3 tag
	BYTE* pID3 = m_pRawFile + m_cbFileSize - 128;
	if (((*(unsigned long *)pID3) & 0xFFFFFF) == 0x474154)
	{
		g_playerDlg.LogPrint(_T("MP3 Title: %s"), GetID3Tag(pID3 + 3, 30));
		g_playerDlg.LogPrint(_T("MP3 Artist: %s"), GetID3Tag(pID3 + 33, 30));
		g_playerDlg.LogPrint(_T("MP3 Album: %s"), GetID3Tag(pID3 + 63, 30));
		g_playerDlg.LogPrint(_T("MP3 Year: %s"), GetID3Tag(pID3 + 93, 4));
		g_playerDlg.LogPrint(_T("MP3 Comment: %s"), GetID3Tag(pID3 + 97, 30));
	}

	m_pDecoder = mp3_create();
	
	m_pNextDecodePos = m_pRawFile;
	m_cbLeftMP3Size = m_cbFileSize;// -128;

	mp3_info_t mp3info;
	if (!DecodeMore(true, &mp3info))
	{
		g_playerDlg.LogPrint(_T("Could not decode MP3 file: %s"), pszPathname);
		return false;
	}

	g_playerDlg.LogPrint(_T("MP3 FileSize=%d bytes, SampleRate=%d Hz, channels=%d"), m_cbFileSize, mp3info.sample_rate, mp3info.channels);

	m_bStopThread = false;
	m_pThread = new std::thread(ThreadProc, this);

	return true;
}

void DecodeMP3::Close()
{
	m_bStopThread = true;

	if (m_pThread != NULL)
	{
		m_pThread->join();
		delete m_pThread;
	}

	if (m_pDecoder != NULL)
	{
		mp3_done(m_pDecoder);
		m_pDecoder = NULL;
	}

	while (!m_queue.empty())
	{
		BufferItem* pItem = m_queue.front();
		m_queue.pop();
		delete pItem;
	}

	while (!m_freeItems.empty())
	{
		BufferItem* pItem = m_freeItems.back();
		m_freeItems.pop_back();
		delete pItem;
	}

	m_nQueueItemCount = 0;

	free(m_pRawFile);
	m_pRawFile = NULL;
}

bool DecodeMP3::DecodeMore(bool isFirst, mp3_info_t* pInfoOut)
{
	signed short buff[MP3_MAX_SAMPLES_PER_FRAME];

	int cbDecoded = mp3_decode(m_pDecoder, m_pNextDecodePos, m_cbLeftMP3Size, buff, pInfoOut);
	if (cbDecoded <= 0)
	{
		if (!isFirst)
		{
			// in decoder thread
			m_bStopThread = true;
			PostMessage(g_playerDlg.GetHwnd(), WM_USER_LOGMSG, 0, (LPARAM)_T("MP3 Decoder error"));
		}

		return false;
	}
	
	m_pNextDecodePos += cbDecoded;
	m_cbLeftMP3Size -= cbDecoded;

	int nWords = pInfoOut->audio_bytes / sizeof(short);
	ASSERT((nWords & 1) == 0);

	// get buffer item
	BufferItem* pItem;
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_freeItems.empty())
		{
			pItem = new BufferItem();
		}
		else
		{
			pItem = m_freeItems.back();
			m_freeItems.pop_back();
		}
	}

	int cbReq = nWords * sizeof(int);
	if (pItem->cbAllocated < cbReq)
	{
		pItem->cbAllocated = cbReq;
		pItem->pAudioData = (int*)realloc(pItem->pAudioData, cbReq);
	}

	pItem->nFilled = nWords;
	pItem->nUsed = 0;

	signed short* pDecodedMP3 = buff;
	signed int* pDst = pItem->pAudioData;
	while (--nWords >= 0)
	{
		signed short val = *pDecodedMP3++;
		*pDst++ = val << 16;
	}

	// add item to queue
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(pItem);
	}

	_InterlockedIncrement(&m_nQueueItemCount);
	//TRACE("MP3 decoded stream %d bytes added: count=%d\n", cbReq, m_nQueueItemCount);
	return true;
}

void DecodeMP3::FillBuffer(int32_t* pLeft, int32_t* pRight, int nWords)
{
	while (nWords > 0)
	{
		// pop item from queue
		BufferItem* pItem;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_queue.empty())
			{
				pItem = NULL;
			}
			else
			{
				pItem = m_queue.front();
			}
		}

		if (pItem == NULL)
		{
			TRACE("!!! mp3 audio source not available\n");
			if (m_bStopThread)
			{
				PostMessage(g_playerDlg.GetHwnd(), WM_STOP_ASIO, 0, 0);
			}

			memset(pLeft, 0, nWords * sizeof(int));
			memset(pRight, 0, nWords * sizeof(int));
			return;
		}

		// assumes two channel STEREO!
		int32_t* pData = &pItem->pAudioData[pItem->nUsed];
		while (pItem->nUsed < pItem->nFilled)
		{
			*pLeft++ = *pData++;
			*pRight++ = *pData++;
			pItem->nUsed += 2;
			if (--nWords <= 0)
				break;
		}

		//TRACE("MP3 Data used %d (fill=%d), nWords=%d\n", pItem->nUsed, pItem->nFilled, nWords);

		if (pItem->nUsed >= pItem->nFilled)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_queue.pop();
			m_freeItems.push_back(pItem);
			_InterlockedDecrement(&m_nQueueItemCount);
		}
	}

}

void DecodeMP3::ThreadProc(DecodeMP3* pThis)
{
	while (!pThis->m_bStopThread)
	{
		if (pThis->m_nQueueItemCount > MAX_QUEUE_ITEM_COUNT)
		{
			//TRACE("MP3 buffer to many, sleep...\n");
			Sleep(250);
			continue;
		}

		mp3_info_t info;
		pThis->DecodeMore(false, &info);
	}

	TRACE("MP3 Decoder thread finished.\n");
}