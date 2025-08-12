module;

#include <dsound.h>
#include "..\devon\DevonMachine.h"

export module SoundTools;

static const unsigned int SO_PLAYBACK_FREQ = 44100;
static const unsigned int SO_PRIMARY_BUFFER_SIZE = (40000 * 2 * 2);

IDirectSound*			g_DS = nullptr;
LPDIRECTSOUNDBUFFER		pDSB = nullptr;
char *					JKevOutBuf = nullptr;

namespace DSoundTools
{
	export void Init(HWND  hWnd, DevonMachine & Machine)
	{
		// Init DSound_____________
		DirectSoundCreate( nullptr, &g_DS, nullptr );
		g_DS->SetCooperativeLevel( hWnd, DSSCL_PRIORITY );

		LPDIRECTSOUNDBUFFER pDSBPrimary = nullptr;
		DSBUFFERDESC dsbd;
		ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
		dsbd.dwSize        = sizeof(DSBUFFERDESC);
		dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
		dsbd.dwBufferBytes = 0;
		dsbd.lpwfxFormat   = nullptr;

		g_DS->CreateSoundBuffer( &dsbd, &pDSBPrimary, nullptr ); 

		WAVEFORMATEX wfx;
		ZeroMemory( &wfx, sizeof(WAVEFORMATEX) );
		wfx.wFormatTag      = (WORD) WAVE_FORMAT_PCM;
		wfx.nChannels       = (WORD) 2;
		wfx.nSamplesPerSec  = (DWORD) SO_PLAYBACK_FREQ;
		wfx.wBitsPerSample  = (WORD) 16;
		wfx.nBlockAlign     = (WORD) (wfx.wBitsPerSample / 8 * wfx.nChannels);
		wfx.nAvgBytesPerSec = (DWORD) (wfx.nSamplesPerSec * wfx.nBlockAlign);

		pDSBPrimary->SetFormat(&wfx);

		ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
		dsbd.dwSize          = sizeof(DSBUFFERDESC);
		dsbd.dwFlags         = DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes   = SO_PRIMARY_BUFFER_SIZE;
		dsbd.lpwfxFormat     = &wfx;

		// Init Secondary Buffer_____________    
		HRESULT Res = g_DS->CreateSoundBuffer( &dsbd, &pDSB, nullptr );

		void *P1 = nullptr, *P2 = nullptr;
		DWORD N1 = 0, N2 = 0;
		pDSB->Lock(0, SO_PRIMARY_BUFFER_SIZE, &P1, &N1, &P2, &N2, 0);

		if(P1 && N1)
			memset(P1, 0, N1);

		if(P2 && N2)
			memset(P2, 0, N2);

		pDSB->Unlock(P1, N1, P2, N2);
	
		pDSB->Play( 0, 0, DSBPLAY_LOOPING );	

		JKevOutBuf = new char[SO_PRIMARY_BUFFER_SIZE];
		Machine.JKev.SetOutputSurface(JKevOutBuf, SO_PRIMARY_BUFFER_SIZE);
	}

	export void Render(DevonMachine & Machine, float Volume)
	{
		DWORD PlayCursor, WriteCursor;

		static const DWORD NbChunks = 8;
		static const DWORD ChunkSize = SO_PRIMARY_BUFFER_SIZE / NbChunks;
		assert(ChunkSize*NbChunks == SO_PRIMARY_BUFFER_SIZE);
		static DWORD NextChunkToWrite = 0;

		pDSB->GetCurrentPosition(&PlayCursor, &WriteCursor);

		DWORD  CurChunk = PlayCursor / ChunkSize;

		while (NextChunkToWrite != ((CurChunk + NbChunks - 1) % NbChunks))
		{
			DWORD Cursor = NextChunkToWrite * ChunkSize;

			void* P[2] = {nullptr, nullptr};
			DWORD N[2] = {0, 0};
			pDSB->Lock(Cursor, ChunkSize, &P[0], &N[0], &P[1], &N[1], 0);

			auto Output = [&](int BufIdx) 
			{
				assert(N[BufIdx] % 4 == 0);
			    unsigned int sampleCount16 = N[BufIdx] / sizeof(short);
				short * Buf = static_cast<short *>(P[BufIdx]);
				for(unsigned int i = 0; i < sampleCount16; i++)
				{
					char Val;
					if(Machine.JKev.Pop(Val))
					{
						// safe sign extension then scale
						const int s = static_cast<int>(static_cast<signed char>(Val));
						int sample = static_cast<int>(s * 256);      // 8-bit -> 16-bit
						sample = static_cast<int>(sample * Volume);  // apply volume
						Buf[i] = static_cast<short>(std::clamp(sample, -32768, 32767));
					}
					else
					{
						Buf[i] = 0;
					}
				}
			};

			Output(0);
			Output(1);
		
			pDSB->Unlock(P[0], N[0], P[1], N[1]);

		    NextChunkToWrite = (NextChunkToWrite + 1) % NbChunks;
		}
	}

	export void Release()
	{
		g_DS->Release();

		delete[] JKevOutBuf;
	}
};
