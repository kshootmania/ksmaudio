#include "ksmaudio/ksmaudio.hpp"
#include <iostream>
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

	bool Init(void* hWnd, AudioBackend audioBackend)
	{
		// デバイスバッファ関連はBASS_Initより前に設定する必要がある
		BASS_SetConfig(BASS_CONFIG_DEV_PERIOD, kDevicePeriodMs);
#ifdef _WIN32
		// macOSではBASS_CONFIG_DEV_BUFFERは利用不可(デバイスバッファ長はBASS_CONFIG_DEV_PERIODの2倍になる)
		BASS_SetConfig(BASS_CONFIG_DEV_BUFFER, kDeviceBufferMs);
#endif

		// 無音区間の後でも発音までの遅延が生じないようデバイスを止めない
		BASS_SetConfig(BASS_CONFIG_DEV_NONSTOP, TRUE);

#ifdef _WIN32
		const DWORD flags = audioBackend == AudioBackend::DirectSound ? BASS_DEVICE_DSOUND : 0;
		BOOL ok = BASS_Init(-1/* default device */, kSampleRate, flags, static_cast<HWND>(hWnd), nullptr);
		if (!ok)
		{
			if (audioBackend != AudioBackend::DirectSound)
			{
				return false;
			}
			// 初期化失敗時はDirectSound指定を外して再試行
			ok = BASS_Init(-1/* default device */, kSampleRate, 0, static_cast<HWND>(hWnd), nullptr);
		}
#else
		(void)hWnd;
		(void)audioBackend;
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

		// デバイスのレイテンシとBASS推奨最小バッファ長を診断用に出力する
		BASS_INFO info{};
		if (BASS_GetInfo(&info))
		{
			std::cerr << "[ksmaudio] device latency=" << info.latency << " ms, minbuf=" << info.minbuf << " ms, freq=" << info.freq << " Hz" << std::endl;
		}

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
