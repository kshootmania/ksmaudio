#pragma once
#include <cstddef>
#include <mutex>
#include <vector>
#include "miniaudio.h"

namespace ksmaudio::web
{
	constexpr ma_uint32 kEngineSampleRate = 44100;
	constexpr ma_uint32 kEngineNumChannels = 2;

	// 1コールバックあたりのフレーム数(miniaudioのWebAudioデフォルトは2048=約46msと粗く、
	// エフェクトパラメータの反映がこの単位に量子化されるため、BASS版の更新周期(約16ms)に近い512=約11.6msを指定)
	constexpr ma_uint32 kEnginePeriodSizeInFrames = 512;

	/// @brief エンジンのミックス対象インタフェース(StreamやSampleの実装が継承)
	class IMixSource
	{
	public:
		virtual ~IMixSource() = default;

		/// @brief ステレオ44100Hzの出力バッファへ加算ミックス
		/// @param pOut 出力バッファ(インターリーブ、frameCount * 2要素)
		/// @param frameCount フレーム数
		virtual void mixInto(float* pOut, std::size_t frameCount) = 0;
	};

	/// @brief miniaudioデバイスを保持し、登録されたソースをミックスして出力するエンジン
	class WebAudioEngine
	{
	private:
		ma_device m_device;
		bool m_deviceInitialized = false;
		std::vector<IMixSource*> m_sources;
		double m_masterVolume = 1.0;
		bool m_muted = false;
		std::recursive_mutex m_mutex;

		static void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

	public:
		WebAudioEngine() = default;

		~WebAudioEngine();

		[[nodiscard]]
		bool init();

		void terminate();

		void registerSource(IMixSource* pSource);

		void unregisterSource(IMixSource* pSource);

		void setMasterVolume(double volume);

		void setMuted(bool muted);

		void lock();

		void unlock();

		/// @brief 出力デバイスのレイテンシ(秒)を取得
		[[nodiscard]]
		double outputLatencySec() const;

		[[nodiscard]]
		bool isInitialized() const;
	};

	/// @brief エンジンのシングルトンを取得
	[[nodiscard]]
	WebAudioEngine& Engine();
}
