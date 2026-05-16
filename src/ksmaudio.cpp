#include "ksmaudio/ksmaudio.hpp"
#include "bass.h"

namespace ksmaudio
{
	namespace
	{
		double s_masterVolume = 1.0;
		bool s_isMuted = false;
		bool s_isWasapiFallbackUsed = false;

		void ApplyVolume()
		{
			const DWORD bassVolume = s_isMuted ? 0 : static_cast<DWORD>(s_masterVolume * 10000.0);
			BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, bassVolume);
			BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, bassVolume);
		}
	}

	bool Init(void* hWnd)
	{
		s_isWasapiFallbackUsed = false;
#ifdef _WIN32
		// BASS 2.4.13以降はWindowsでWASAPIがデフォルトになったが、音声がズレるためDirectSoundを明示的に指定
		BOOL ok = BASS_Init(-1/* default device */, kSampleRate, BASS_DEVICE_DSOUND, static_cast<HWND>(hWnd), nullptr);
		if (!ok)
		{
			// 初期化失敗時はDirectSound指定を外して再試行
			ok = BASS_Init(-1/* default device */, kSampleRate, 0, static_cast<HWND>(hWnd), nullptr);
			if (ok)
			{
				s_isWasapiFallbackUsed = true;
			}
		}
#else
		(void)hWnd;
		const BOOL ok = BASS_Init(-1/* default device */, kSampleRate, 0, 0, nullptr);
#endif
		if (!ok)
		{
			return false;
		}
		BASS_SetConfig(BASS_CONFIG_BUFFER, kBufferSizeMs);
		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, kUpdatePeriodMs);
		BASS_SetConfig(BASS_CONFIG_FLOATDSP, TRUE);
		BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, kUpdateThreads);

		// bass_fx.dllをロードするために呼ぶ必要あり(失敗時は0が返る)
		if (BASS_FX_GetVersion() == 0)
		{
			BASS_Free();
			return false;
		}
		return true;
	}

	void Terminate()
	{
		BASS_Free();
	}

	int GetLastErrorCode()
	{
		return BASS_ErrorGetCode();
	}

	bool IsWasapiFallbackUsed()
	{
		return s_isWasapiFallbackUsed;
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
