#pragma once

// 音声バックエンドの選択
// ネイティブ環境ではBASSを使用し、Web(Emscripten)環境ではBASSが利用できないためminiaudioベースの自前実装を使用する
#if defined(__EMSCRIPTEN__)
#define KSMAUDIO_BACKEND_WEB
#else
#define KSMAUDIO_BACKEND_BASS
#endif

#ifdef KSMAUDIO_BACKEND_BASS
#include "bass.h"
#include "bass_fx.h"
#else
#include <cstdint>

// BASSヘッダと互換の整数型・ハンドル型(公開APIのシグネチャをバックエンド間で揃えるために定義)
typedef std::uint32_t DWORD;
typedef std::uint64_t QWORD;
typedef DWORD HSTREAM;
typedef DWORD HSAMPLE;
typedef DWORD HCHANNEL;
typedef DWORD HDSP;
typedef DWORD HFX;
#endif
