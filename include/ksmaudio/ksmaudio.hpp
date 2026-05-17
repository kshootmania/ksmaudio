#pragma once
#include "Stream.hpp"
#include "StreamWithEffects.hpp"
#include "Sample.hpp"
#include "AudioEffect/All.hpp"

namespace ksmaudio
{
	constexpr DWORD kSampleRate = 44100;
	constexpr DWORD kBufferSizeMs = 200;
	constexpr DWORD kUpdatePeriodMs = 100;
	constexpr DWORD kUpdateThreads = 2;

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
