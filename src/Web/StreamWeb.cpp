#include "ksmaudio/Stream.hpp"

#ifdef KSMAUDIO_BACKEND_WEB

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include "WebAudioEngine.hpp"

namespace
{
	using namespace ksmaudio;

	// BASS版のコンプレッサー(BASS_FX_BFX_COMPRESSOR2)と同等のパラメータ値
	constexpr float kCompressorGainDb = 18.0f;
	constexpr float kCompressorThresholdDb = -24.0f;
	constexpr float kCompressorRatio = 5.0f;
	constexpr float kCompressorAttackMs = 1.0f;
	constexpr float kCompressorReleaseMs = 20.0f;

	// 各組み込みDSPの優先順位(BASS版のStream.cppと同じ値)
	constexpr int kCompressorFXPriority = 0;
	constexpr int kVolumeAmplifyFXPriority = 5;
	constexpr int kVolumeFXPriority = 10;

	// コンプレッサーの前段の音量変更の値(BASS版のkVolumeFXParamsと同じ値)
	constexpr float kPreCompressorVolume = 0.1f;

	/// @brief BASS_FX_BFX_COMPRESSOR2相当のコンプレッサー
	class Compressor
	{
	private:
		float m_gainReductionDb = 0.0f;
		float m_attackCoeff;
		float m_releaseCoeff;

	public:
		Compressor()
			: m_attackCoeff(std::exp(-1.0f / (kCompressorAttackMs * 0.001f * web::kEngineSampleRate)))
			, m_releaseCoeff(std::exp(-1.0f / (kCompressorReleaseMs * 0.001f * web::kEngineSampleRate)))
		{
		}

		void process(float* pData, std::size_t frameCount)
		{
			const float makeupGain = std::pow(10.0f, kCompressorGainDb / 20.0f);
			for (std::size_t i = 0; i < frameCount; ++i)
			{
				float* pFrame = pData + i * web::kEngineNumChannels;

				// 全チャンネル共通のピークレベルで検出(BASS_BFX_CHANALL相当)
				float peak = 0.0f;
				for (std::size_t ch = 0; ch < web::kEngineNumChannels; ++ch)
				{
					peak = std::max(peak, std::abs(pFrame[ch]));
				}
				const float levelDb = 20.0f * std::log10(std::max(peak, 1e-10f));
				const float overDb = levelDb - kCompressorThresholdDb;
				const float targetReductionDb = overDb > 0.0f ? overDb * (1.0f - 1.0f / kCompressorRatio) : 0.0f;

				// アタック/リリースで平滑化
				const float coeff = targetReductionDb > m_gainReductionDb ? m_attackCoeff : m_releaseCoeff;
				m_gainReductionDb = targetReductionDb + coeff * (m_gainReductionDb - targetReductionDb);

				const float gain = makeupGain * std::pow(10.0f, -m_gainReductionDb / 20.0f);
				for (std::size_t ch = 0; ch < web::kEngineNumChannels; ++ch)
				{
					pFrame[ch] *= gain;
				}
			}
		}
	};

	/// @brief DSPチェーンの要素の種類
	enum class DSPType
	{
		UserEffect,
		PreCompressorVolume,
		VolumeAmplify,
		Compressor,
	};

	struct DSPEntry
	{
		HDSP handle;
		int priority;
		DSPType type;
		AudioEffect::IAudioEffect* pAudioEffect; // UserEffectの場合のみ使用
	};
}

namespace ksmaudio
{
	class Stream::Impl : public web::IMixSource
	{
	private:
		ma_decoder m_decoder;
		bool m_decoderOk = false;
		const bool m_loop;
		Compressor m_compressor;

		enum class State
		{
			Stopped,
			Playing,
		};
		State m_state = State::Stopped;

		double m_volume;
		bool m_muted = false;

		// BASS_ATTRIB_VOL相当のフェード係数(スライド対象)
		float m_attribVol = 1.0f;
		bool m_sliding = false;
		float m_slideTarget = 1.0f;
		float m_slideDeltaPerFrame = 0.0f;

		// BASS_FX_BFX_VOLUME相当の増幅ゲイン(100%超音量用)
		float m_amplifyGain = 1.0f;

		std::vector<DSPEntry> m_dspChain;
		HDSP m_nextDSPHandle = 1;

		// シーク基準位置(秒)とシーク後の出力フレーム数
		double m_seekBaseSec = 0.0;
		std::uint64_t m_outputFrames = 0;

		// 最後にミックスした時刻(posSecはコールバック単位でしか進まないため、コールバック間は実時間で補間する)
		std::chrono::steady_clock::time_point m_lastMixTime{};
		bool m_lastMixTimeValid = false;

		std::vector<float> m_frameBuffer;

		void insertDSPEntry(const DSPEntry& entry)
		{
			// 優先度の降順を保ち、同順位では後から追加した方を後ろに挿入(BASSと同じ呼び出し順)
			const auto it = std::find_if(m_dspChain.begin(), m_dspChain.end(),
				[&entry](const DSPEntry& e)
				{
					return e.priority < entry.priority;
				});
			m_dspChain.insert(it, entry);
		}

		std::size_t readDecodedFrames(float* pDst, std::size_t frameCount)
		{
			if (!m_decoderOk)
			{
				return 0U;
			}
			std::size_t total = 0U;
			while (total < frameCount)
			{
				ma_uint64 framesRead = 0U;
				ma_decoder_read_pcm_frames(&m_decoder, pDst + total * web::kEngineNumChannels, frameCount - total, &framesRead);
				total += static_cast<std::size_t>(framesRead);
				if (total >= frameCount)
				{
					break;
				}
				if (m_loop)
				{
					if (ma_decoder_seek_to_pcm_frame(&m_decoder, 0U) != MA_SUCCESS || framesRead == 0U)
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			return total;
		}

	public:
		Impl(const std::string& filePath, double volume, bool enableCompressor, bool loop)
			: m_loop(loop)
			, m_volume(volume)
		{
			ma_decoder_config config = ma_decoder_config_init(ma_format_f32, web::kEngineNumChannels, web::kEngineSampleRate);
			m_decoderOk = ma_decoder_init_file(filePath.c_str(), &config, &m_decoder) == MA_SUCCESS;

			// 組み込みDSPを登録(音量増幅は常時、コンプレッサー系は有効時のみ)
			insertDSPEntry(DSPEntry{ .handle = 0U, .priority = kVolumeAmplifyFXPriority, .type = DSPType::VolumeAmplify, .pAudioEffect = nullptr });
			if (enableCompressor)
			{
				insertDSPEntry(DSPEntry{ .handle = 0U, .priority = kVolumeFXPriority, .type = DSPType::PreCompressorVolume, .pAudioEffect = nullptr });
				insertDSPEntry(DSPEntry{ .handle = 0U, .priority = kCompressorFXPriority, .type = DSPType::Compressor, .pAudioEffect = nullptr });
			}

			applyVolume();

			web::Engine().registerSource(this);
		}

		~Impl()
		{
			web::Engine().unregisterSource(this);
			if (m_decoderOk)
			{
				ma_decoder_uninit(&m_decoder);
			}
		}

		virtual void mixInto(float* pOut, std::size_t frameCount) override
		{
			if (m_state != State::Playing || !m_decoderOk)
			{
				return;
			}

			m_frameBuffer.resize(frameCount * web::kEngineNumChannels);
			const std::size_t framesRead = readDecodedFrames(m_frameBuffer.data(), frameCount);
			m_outputFrames += framesRead;
			m_lastMixTime = std::chrono::steady_clock::now();
			m_lastMixTimeValid = true;

			// DSPチェーンを優先度順に適用
			const std::size_t sampleCount = framesRead * web::kEngineNumChannels;
			for (const DSPEntry& entry : m_dspChain)
			{
				switch (entry.type)
				{
				case DSPType::UserEffect:
					entry.pAudioEffect->process(m_frameBuffer.data(), sampleCount);
					break;

				case DSPType::PreCompressorVolume:
					for (std::size_t i = 0; i < sampleCount; ++i)
					{
						m_frameBuffer[i] *= kPreCompressorVolume;
					}
					break;

				case DSPType::VolumeAmplify:
					if (m_amplifyGain != 1.0f)
					{
						for (std::size_t i = 0; i < sampleCount; ++i)
						{
							m_frameBuffer[i] *= m_amplifyGain;
						}
					}
					break;

				case DSPType::Compressor:
					m_compressor.process(m_frameBuffer.data(), framesRead);
					break;
				}
			}

			// フェード係数を適用しながら出力バッファへ加算ミックス
			for (std::size_t i = 0; i < framesRead; ++i)
			{
				if (m_sliding)
				{
					m_attribVol += m_slideDeltaPerFrame;
					if ((m_slideDeltaPerFrame >= 0.0f && m_attribVol >= m_slideTarget) ||
						(m_slideDeltaPerFrame < 0.0f && m_attribVol <= m_slideTarget))
					{
						m_attribVol = m_slideTarget;
						m_sliding = false;
					}
				}
				for (std::size_t ch = 0; ch < web::kEngineNumChannels; ++ch)
				{
					pOut[i * web::kEngineNumChannels + ch] += m_frameBuffer[i * web::kEngineNumChannels + ch] * m_attribVol;
				}
			}

			if (framesRead < frameCount && !m_loop)
			{
				m_state = State::Stopped;
			}
		}

		void play()
		{
			web::Engine().lock();
			m_state = State::Playing;
			m_lastMixTimeValid = false;
			web::Engine().unlock();
		}

		void pause()
		{
			web::Engine().lock();
			m_state = State::Stopped;
			web::Engine().unlock();
		}

		void stop()
		{
			pause();
		}

		bool isPlaying() const
		{
			return m_state == State::Playing;
		}

		/// @brief 現在再生位置(秒)を取得(コールバック間は実時間で補間)
		double posSec() const
		{
			const double baseSec = m_seekBaseSec + static_cast<double>(m_outputFrames) / web::kEngineSampleRate;
			if (m_state != State::Playing || !m_lastMixTimeValid)
			{
				return baseSec;
			}
			const double periodSec = static_cast<double>(web::kEnginePeriodSizeInFrames) / web::kEngineSampleRate;
			const double elapsedSec = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_lastMixTime).count();
			return baseSec + std::clamp(elapsedSec, 0.0, periodSec);
		}

		/// @brief 指定位置(秒)へシーク
		void seekPosSec(double sec)
		{
			web::Engine().lock();
			if (m_decoderOk)
			{
				const auto frame = static_cast<ma_uint64>(std::max(sec, 0.0) * web::kEngineSampleRate);
				ma_decoder_seek_to_pcm_frame(&m_decoder, frame);
			}
			m_seekBaseSec = sec;
			m_outputFrames = 0U;
			m_lastMixTimeValid = false;
			web::Engine().unlock();
		}

		/// @brief 全体の長さ(秒)を取得
		double durationSec()
		{
			if (!m_decoderOk)
			{
				return 0.0;
			}
			ma_uint64 lengthFrames = 0U;
			if (ma_decoder_get_length_in_pcm_frames(&m_decoder, &lengthFrames) != MA_SUCCESS)
			{
				return 0.0;
			}
			return static_cast<double>(lengthFrames) / web::kEngineSampleRate;
		}

		HDSP addAudioEffect(AudioEffect::IAudioEffect* pAudioEffect, int priority)
		{
			web::Engine().lock();
			const HDSP handle = m_nextDSPHandle++;
			insertDSPEntry(DSPEntry{ .handle = handle, .priority = priority, .type = DSPType::UserEffect, .pAudioEffect = pAudioEffect });
			web::Engine().unlock();
			return handle;
		}

		void removeAudioEffect(HDSP hDSP)
		{
			web::Engine().lock();
			m_dspChain.erase(
				std::remove_if(m_dspChain.begin(), m_dspChain.end(),
					[hDSP](const DSPEntry& e)
					{
						return e.type == DSPType::UserEffect && e.handle == hDSP;
					}),
				m_dspChain.end());
			web::Engine().unlock();
		}

		void applyVolume()
		{
			web::Engine().lock();
			if (m_volume > 1.0)
			{
				// 100%超の場合は増幅ゲインで対応し、フェード係数は1.0にする(BASS版と同じ挙動)
				m_amplifyGain = static_cast<float>(m_volume);
				m_attribVol = 1.0f;
			}
			else
			{
				m_amplifyGain = 1.0f;
				m_attribVol = static_cast<float>(m_volume);
			}
			web::Engine().unlock();
		}

		void setVolume(double volume)
		{
			m_volume = volume;
			if (!m_muted)
			{
				applyVolume();
			}
		}

		double volume() const
		{
			return m_volume;
		}

		bool muted() const
		{
			return m_muted;
		}

		void setMuted(bool muted)
		{
			web::Engine().lock();
			m_muted = muted;
			if (muted)
			{
				m_attribVol = 0.0f;
			}
			else
			{
				m_attribVol = m_volume > 1.0 ? 1.0f : static_cast<float>(m_volume);
			}
			web::Engine().unlock();
		}

		void slideAttribVol(float from, float to, double durationSec)
		{
			web::Engine().lock();
			m_attribVol = from;
			m_slideTarget = to;
			const double frames = std::max(durationSec, 0.001) * web::kEngineSampleRate;
			m_slideDeltaPerFrame = static_cast<float>((to - from) / frames);
			m_sliding = true;
			web::Engine().unlock();
		}

		bool isSliding() const
		{
			return m_sliding;
		}

		float attribVol() const
		{
			return m_attribVol;
		}
	};

	// Webバックエンドは再生速度変更(playbackSpeed)未対応のため、1.0以外が渡されても無視して等速で再生する
	Stream::Stream(const std::string& filePath, double volume, bool enableCompressor, [[maybe_unused]] bool preload, bool loop, [[maybe_unused]] double playbackSpeed, [[maybe_unused]] bool decode)
		: m_impl(std::make_unique<Impl>(filePath, volume, enableCompressor, loop))
	{
	}

	Stream::~Stream() = default;

	Stream::Stream(Stream&&) = default;

	Stream& Stream::operator=(Stream&&) = default;

	void Stream::applyVolume()
	{
		m_impl->applyVolume();
	}

	void Stream::play() const
	{
		m_impl->play();
	}

	void Stream::pause() const
	{
		m_impl->pause();
	}

	void Stream::stop() const
	{
		m_impl->stop();
	}

	void Stream::updateManually() const
	{
		// Webバックエンドではデバイスのコールバックで常時処理されるため何もしない
	}

	SecondsF Stream::posSec() const
	{
		return SecondsF{ m_impl->posSec() };
	}

	void Stream::seekPosSec(SecondsF time) const
	{
		m_impl->seekPosSec(time.count());
	}

	Duration Stream::duration() const
	{
		return Duration{ m_impl->durationSec() };
	}

	HDSP Stream::addAudioEffect(AudioEffect::IAudioEffect* pAudioEffect, int priority) const
	{
		return m_impl->addAudioEffect(pAudioEffect, priority);
	}

	void Stream::removeAudioEffect(HDSP hDSP) const
	{
		m_impl->removeAudioEffect(hDSP);
	}

	void Stream::setFadeIn(Duration duration) const
	{
		// 音量を0から本来の音量まで推移させる(100%超の増幅は増幅ゲイン側で行うため上限は1.0)
		const double volume = m_impl->volume();
		const float targetVolume = volume > 1.0 ? 1.0f : static_cast<float>(volume);
		m_impl->slideAttribVol(0.0f, targetVolume, duration.count());
	}

	void Stream::setFadeIn(Duration duration, double volume)
	{
		m_impl->setVolume(volume);
		setFadeIn(duration);
	}

	void Stream::setFadeOut(Duration duration) const
	{
		// 音量を現在値から0まで推移させる
		m_impl->slideAttribVol(m_impl->attribVol(), 0.0f, duration.count());
	}

	void Stream::setFadeOut(Duration duration, double volume)
	{
		m_impl->setVolume(volume);
		setFadeOut(duration);
	}

	bool Stream::isPlaying() const
	{
		return m_impl->isPlaying();
	}

	bool Stream::isFading() const
	{
		return m_impl->isSliding();
	}

	void Stream::setVolume(double volume)
	{
		m_impl->setVolume(volume);
	}

	double Stream::volume() const
	{
		return m_impl->volume();
	}

	bool Stream::muted() const
	{
		return m_impl->muted();
	}

	void Stream::setMuted(bool muted)
	{
		m_impl->setMuted(muted);
	}

	std::size_t Stream::sampleRate() const
	{
		return static_cast<std::size_t>(web::kEngineSampleRate);
	}

	std::size_t Stream::numChannels() const
	{
		return static_cast<std::size_t>(web::kEngineNumChannels);
	}

	SecondsF Stream::latency() const
	{
		return SecondsF{ web::Engine().outputLatencySec() };
	}

	void Stream::lockBegin() const
	{
		web::Engine().lock();
	}

	void Stream::lockEnd() const
	{
		web::Engine().unlock();
	}

	DWORD Stream::getData([[maybe_unused]] void* buffer, [[maybe_unused]] DWORD length)
	{
		// Webバックエンドでは未対応(アプリ側からは未使用)
		return 0U;
	}
}

#endif
