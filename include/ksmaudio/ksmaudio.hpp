#pragma once
#include "Stream.hpp"
#include "StreamWithEffects.hpp"
#include "Sample.hpp"
#include "AudioEffect/All.hpp"

namespace ksmaudio
{
	constexpr DWORD kSampleRate = 44100;

	// 再生バッファ長(BGM等のHSTREAM/HMUSIC用、サンプルには影響しない)
	constexpr DWORD kBufferSizeMs = 100;

	// 再生バッファの更新周期
	constexpr DWORD kUpdatePeriodMs = 5;

	constexpr DWORD kUpdateThreads = 2;

	// デバイスバッファの更新周期(BASS_Initより前に設定する必要がある)
	// (macOSではデバイスバッファ長がこの2倍になる)
	constexpr DWORD kDevicePeriodMs = 5;

	// デバイスバッファ長(Windowsのみ有効、BASS_Initより前に設定する必要がある)
	constexpr DWORD kDeviceBufferMs = 10;

	/// @brief 音声出力バックエンド
	enum class AudioBackend
	{
		/// @brief BASSのデフォルト出力を使用
		Default,

		/// @brief DirectSound出力を使用(Windowsのみ)
		DirectSound,
	};

	/// @brief 音声処理を初期化
	/// @param hWnd Windowsの場合はメインウィンドウのHWND、それ以外ではnullptr
	/// @param audioBackend Windowsで使用する音声出力バックエンド。それ以外の環境では無視される
	/// @return 成功時true、失敗時false(失敗時はGetLastErrorCode()でBASSエラーコードを取得可能)
	[[nodiscard]]
	bool Init(void* hWnd, AudioBackend audioBackend = AudioBackend::Default);

	void Terminate();

	/// @brief 直前のBASS関数のエラーコード(BASS_ErrorGetCode)を取得
	[[nodiscard]]
	int GetLastErrorCode();

	void SetMute(bool isMute);

	/// @brief マスターボリュームを設定
	/// @param volume 音量(0.0〜1.0)
	void SetMasterVolume(double volume);
}
