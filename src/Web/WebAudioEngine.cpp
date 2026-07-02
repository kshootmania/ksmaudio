#include "WebAudioEngine.hpp"
#include <algorithm>
#include <cstring>

namespace ksmaudio::web
{
	void WebAudioEngine::DataCallback(ma_device* pDevice, void* pOutput, [[maybe_unused]] const void* pInput, ma_uint32 frameCount)
	{
		auto* pEngine = static_cast<WebAudioEngine*>(pDevice->pUserData);
		auto* pOut = static_cast<float*>(pOutput);
		const std::size_t sampleCount = static_cast<std::size_t>(frameCount) * kEngineNumChannels;
		std::memset(pOut, 0, sampleCount * sizeof(float));

		std::lock_guard<std::recursive_mutex> lock(pEngine->m_mutex);
		for (IMixSource* pSource : pEngine->m_sources)
		{
			pSource->mixInto(pOut, frameCount);
		}

		const float masterGain = pEngine->m_muted ? 0.0f : static_cast<float>(pEngine->m_masterVolume);
		if (masterGain != 1.0f)
		{
			for (std::size_t i = 0; i < sampleCount; ++i)
			{
				pOut[i] *= masterGain;
			}
		}
	}

	WebAudioEngine::~WebAudioEngine()
	{
		terminate();
	}

	bool WebAudioEngine::init()
	{
		if (m_deviceInitialized)
		{
			return true;
		}

		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_f32;
		config.playback.channels = kEngineNumChannels;
		config.sampleRate = kEngineSampleRate;
		config.periodSizeInFrames = kEnginePeriodSizeInFrames;
		config.dataCallback = DataCallback;
		config.pUserData = this;

		if (ma_device_init(nullptr, &config, &m_device) != MA_SUCCESS)
		{
			return false;
		}
		if (ma_device_start(&m_device) != MA_SUCCESS)
		{
			ma_device_uninit(&m_device);
			return false;
		}
		m_deviceInitialized = true;
		return true;
	}

	void WebAudioEngine::terminate()
	{
		if (m_deviceInitialized)
		{
			ma_device_uninit(&m_device);
			m_deviceInitialized = false;
		}
	}

	void WebAudioEngine::registerSource(IMixSource* pSource)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		m_sources.push_back(pSource);
	}

	void WebAudioEngine::unregisterSource(IMixSource* pSource)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		m_sources.erase(std::remove(m_sources.begin(), m_sources.end(), pSource), m_sources.end());
	}

	void WebAudioEngine::setMasterVolume(double volume)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		m_masterVolume = volume;
	}

	void WebAudioEngine::setMuted(bool muted)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		m_muted = muted;
	}

	void WebAudioEngine::lock()
	{
		m_mutex.lock();
	}

	void WebAudioEngine::unlock()
	{
		m_mutex.unlock();
	}

	double WebAudioEngine::outputLatencySec() const
	{
		if (!m_deviceInitialized)
		{
			return 0.0;
		}
		const ma_uint32 sampleRate = m_device.playback.internalSampleRate != 0 ? m_device.playback.internalSampleRate : kEngineSampleRate;
		return static_cast<double>(m_device.playback.internalPeriodSizeInFrames) / sampleRate;
	}

	bool WebAudioEngine::isInitialized() const
	{
		return m_deviceInitialized;
	}

	WebAudioEngine& Engine()
	{
		static WebAudioEngine s_engine;
		return s_engine;
	}
}
