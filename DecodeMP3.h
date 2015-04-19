#pragma once

extern "C"
{
#include "mp3dec/minimp3.h"
}

class DecodeMP3
{
public:
	DecodeMP3();
	~DecodeMP3();

	bool Open(LPCTSTR pszPathname);
	void Close();

	void FillBuffer(int32_t* pLeft, int32_t* pRight, int nWords);

private:
	static void ThreadProc(DecodeMP3* pThis);

	void PushDecoded(signed short* pDecodedMP3, int cbDecoded);
	
	struct BufferItem
	{
		BufferItem()
		{
			pAudioData = NULL;
			cbAllocated = 0;
		}

		~BufferItem()
		{
			free(pAudioData);
		}

		int32_t* pAudioData;	// L,R interleaved
		int cbAllocated;
		int nFilled;
		int nUsed;	// audio word (int32) count
	};

	bool DecodeMore(bool isFirst, mp3_info_t* pInfoOut);

	LONG m_nQueueItemCount;

	std::mutex m_mutex;
	std::thread* m_pThread;
	std::queue<BufferItem*> m_queue;
	std::vector<BufferItem*> m_freeItems;

	bool m_bStopThread;

	BYTE* m_pRawFile;
	long m_cbFileSize;

	BYTE* m_pNextDecodePos;
	int m_cbLeftMP3Size;

	mp3_decoder_t* m_pDecoder;

};