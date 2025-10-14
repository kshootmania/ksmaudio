#pragma once
#include "ksmaudio/audio_effect/audio_effect.hpp"
#include "ksmaudio/audio_effect/params/pitch_shift_params.hpp"
#include "ksmaudio/audio_effect/detail/biquad_filter.hpp"
#include <array>
#include <vector>

namespace ksmaudio::AudioEffect
{
	class PitchShiftDSP
	{
	private:
		static constexpr int kDelayBufferMax = 44100 * 4;
		static constexpr int kChannelLeft = 0;
		static constexpr int kChannelRight = 1;

		const DSPCommonInfo m_info;

		float m_pitch = 0.0f;
		float m_pitchPrev = 0.0f;
		std::array<std::array<float, kDelayBufferMax>, 2> m_delayBuffer{};
		float m_playSpeed = 1.0f;
		float m_overlap = 0.25f;
		int m_cursor = 0;
		int m_count = 0;
		int m_chunkSize = 1378;
		int m_sizep = 100;
		int m_start = 0;
		int m_startp = 0;
		int m_startp2 = 0;
		int m_step2 = -1;
		std::array<detail::BiquadFilter<float>, 2> m_lowpassFilter;

	public:
		explicit PitchShiftDSP(const DSPCommonInfo& info);

		void process(float* pData, std::size_t dataSize, bool bypass, const PitchShiftDSPParams& params);

		void updateParams(const PitchShiftDSPParams& params);
	};
}
