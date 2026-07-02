#include "ksmaudio/Sample.hpp"

#ifdef KSMAUDIO_BACKEND_WEB

#include <algorithm>
#include <vector>
#include "WebAudioEngine.hpp"

namespace ksmaudio
{
	class Sample::Impl : public web::IMixSource
	{
	private:
		struct Voice
		{
			std::size_t cursorFrame = 0U;
			float volume = 1.0f;
			bool active = false;
		};

		std::vector<float> m_pcmData; // ステレオ44100Hzのインターリーブデータ
		std::size_t m_totalFrames = 0U;
		std::vector<Voice> m_voices;

	public:
		Impl(const std::string& filePath, DWORD maxPolyphony)
			: m_voices(static_cast<std::size_t>(maxPolyphony))
		{
			ma_decoder_config config = ma_decoder_config_init(ma_format_f32, web::kEngineNumChannels, web::kEngineSampleRate);
			ma_uint64 frameCount = 0U;
			void* pData = nullptr;
			if (ma_decode_file(filePath.c_str(), &config, &frameCount, &pData) == MA_SUCCESS)
			{
				m_totalFrames = static_cast<std::size_t>(frameCount);
				const auto* pFloatData = static_cast<const float*>(pData);
				m_pcmData.assign(pFloatData, pFloatData + m_totalFrames * web::kEngineNumChannels);
				ma_free(pData, nullptr);
			}

			web::Engine().registerSource(this);
		}

		~Impl()
		{
			web::Engine().unregisterSource(this);
		}

		virtual void mixInto(float* pOut, std::size_t frameCount) override
		{
			if (m_totalFrames == 0U)
			{
				return;
			}
			for (Voice& voice : m_voices)
			{
				if (!voice.active)
				{
					continue;
				}
				const std::size_t remaining = m_totalFrames - voice.cursorFrame;
				const std::size_t mixFrames = std::min(frameCount, remaining);
				const float* pSrc = m_pcmData.data() + voice.cursorFrame * web::kEngineNumChannels;
				for (std::size_t i = 0; i < mixFrames * web::kEngineNumChannels; ++i)
				{
					pOut[i] += pSrc[i] * voice.volume;
				}
				voice.cursorFrame += mixFrames;
				if (voice.cursorFrame >= m_totalFrames)
				{
					voice.active = false;
				}
			}
		}

		void play(double volume)
		{
			if (m_totalFrames == 0U || m_voices.empty())
			{
				return;
			}

			web::Engine().lock();

			// 空きボイスを探し、なければ最も長く再生されているボイスを再利用(BASS_SAMPLE_OVER_POS相当)
			Voice* pTargetVoice = nullptr;
			for (Voice& voice : m_voices)
			{
				if (!voice.active)
				{
					pTargetVoice = &voice;
					break;
				}
			}
			if (pTargetVoice == nullptr)
			{
				const auto it = std::max_element(m_voices.begin(), m_voices.end(),
					[](const Voice& a, const Voice& b)
					{
						return a.cursorFrame < b.cursorFrame;
					});
				pTargetVoice = &(*it);
			}

			pTargetVoice->cursorFrame = 0U;
			pTargetVoice->volume = static_cast<float>(volume);
			pTargetVoice->active = true;

			web::Engine().unlock();
		}
	};

	Sample::Sample(const std::string& filePath, DWORD maxPolyphony)
		: m_impl(std::make_unique<Impl>(filePath, maxPolyphony))
	{
	}

	Sample::~Sample() = default;

	Sample::Sample(Sample&& other) noexcept = default;

	Sample& Sample::operator=(Sample&& other) noexcept = default;

	void Sample::play(double volume) const
	{
		if (m_impl)
		{
			m_impl->play(volume);
		}
	}
}

#endif
