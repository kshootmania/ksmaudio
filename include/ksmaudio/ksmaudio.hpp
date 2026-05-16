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

	/// @brief 音声処理を初期化
	/// @param hWnd Windowsの場合はメインウィンドウのHWND、それ以外ではnullptr
	/// @return 成功時true、失敗時false(失敗時はGetLastErrorCode()でBASSエラーコードを取得可能)
	[[nodiscard]]
	bool Init(void* hWnd);

	void Terminate();

	/// @brief 直前のBASS関数のエラーコード(BASS_ErrorGetCode)を取得
	[[nodiscard]]
	int GetLastErrorCode();

	/// @brief WindowsでDirectSound初期化に失敗し、WASAPI相当のデフォルト初期化にフォールバックしたか
	[[nodiscard]]
	bool IsWasapiFallbackUsed();

	void SetMute(bool isMute);

	/// @brief マスターボリュームを設定
	/// @param volume 音量(0.0〜1.0)
	void SetMasterVolume(double volume);
}
