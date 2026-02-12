#include "ksmaudio/ksmaudio.hpp"
#include "bass.h"

namespace ksmaudio
{
	namespace
	{
		double s_masterVolume = 1.0;
		bool s_isMuted = false;

		void ApplyVolume()
		{
			const DWORD bassVolume = s_isMuted ? 0 : static_cast<DWORD>(s_masterVolume * 10000.0);
			BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, bassVolume);
			BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, bassVolume);
		}
	}

	void Init(void* hWnd)
	{
#ifdef _WIN32
		BASS_Init(-1/* default device */, kSampleRate, 0, static_cast<HWND>(hWnd), nullptr);
#else
		(void)hWnd;
		BASS_Init(-1/* default device */, kSampleRate, 0, 0, nullptr);
#endif
		BASS_SetConfig(BASS_CONFIG_BUFFER, kBufferSizeMs);
		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, kUpdatePeriodMs);
		BASS_SetConfig(BASS_CONFIG_FLOATDSP, TRUE);
		BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, kUpdateThreads);

		BASS_FX_GetVersion(); // bass_fx.dllをロードするために呼ぶ必要あり
	}

	void Terminate()
	{
		BASS_Free();
	}

	void SetMute(bool isMute)
	{
		s_isMuted = isMute;
		ApplyVolume();
	}

	void SetMasterVolume(double volume)
	{
		s_masterVolume = volume;
		ApplyVolume();
	}
}
