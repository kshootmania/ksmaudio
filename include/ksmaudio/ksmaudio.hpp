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

	void Init(void* hWnd);

	void Terminate();

	void SetMute(bool isMute);
}
