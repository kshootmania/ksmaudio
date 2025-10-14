#include "ksmaudio/audio_effect/dsp/pitch_shift_dsp.hpp"
#include <cmath>
#include <algorithm>
#include <cassert>

namespace ksmaudio::AudioEffect
{
	PitchShiftDSP::PitchShiftDSP(const DSPCommonInfo& info)
		: m_info(info)
		, m_delayBuffer(info.numChannels)
		, m_lowpassFilter(info.numChannels)
	{
		m_start = kDelayBufferMax - m_chunkSize;
		m_prevStart = m_prevPrevStart = kDelayBufferMax - m_chunkSize - static_cast<std::size_t>(m_overlap * m_chunkSize);
	}

	void PitchShiftDSP::process(float* pData, std::size_t dataSize, bool bypass, const PitchShiftDSPParams& params)
	{
		if (m_info.isUnsupported || bypass)
		{
			return;
		}

		assert(dataSize % m_info.numChannels == 0);
		const std::size_t numFrames = dataSize / m_info.numChannels;

		m_pitch = params.pitch;
		const float mix = params.mix;
		m_playSpeed = std::pow(2.0f, m_pitch / 12.0f);
		m_chunkSize = static_cast<std::size_t>(params.chunkSize * m_info.sampleRateScale);
		m_overlap = params.overlap;

		if (m_pitch != m_pitchPrev)
		{
			const float cutoffFreq = m_playSpeed > 1.0f ? m_info.sampleRateFloat / 2 / m_playSpeed : m_info.sampleRateFloat / 2;
			for (std::size_t ch = 0; ch < m_lowpassFilter.size(); ++ch)
			{
				m_lowpassFilter[ch].setLowPassFilter(cutoffFreq, 0.707f, m_info.sampleRateFloat);
			}
		}

		const std::size_t overlapSample = static_cast<std::size_t>(m_overlap * m_chunkSize);
		if (overlapSample == 0)
		{
			return;
		}

		assert(m_delayBuffer.size() == m_info.numChannels);
		assert(m_lowpassFilter.size() == m_info.numChannels);
		for (std::size_t i = 0; i < numFrames; ++i)
		{
			for (std::size_t ch = 0; ch < m_info.numChannels; ++ch)
			{
				m_delayBuffer[ch][m_cursor] = *pData;
				
				const std::size_t countTimesPlaySpeed = static_cast<std::size_t>(m_count * m_playSpeed);
				const std::size_t step = countTimesPlaySpeed % (m_chunkSize - overlapSample);
				
				if (m_pitch == 0.0f || mix == 0.0f)
				{
					++pData;
					continue;
				}

				float output;
				if (countTimesPlaySpeed <= overlapSample)
				{
					const float rate = static_cast<float>(step) / overlapSample;
					const std::size_t currentIdx = (kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax;
					const std::size_t prevIdx = (m_prevStart + step) % kDelayBufferMax;
					if (!m_thirdChunkBlendStep.has_value())
					{
						output = m_delayBuffer[ch][currentIdx] * rate + m_delayBuffer[ch][prevIdx] * (1.0f - rate);
					}
					else
					{
						const std::size_t prevPrevIdx = (m_prevPrevStart + step) % kDelayBufferMax;
						const float rate2 = std::min(static_cast<float>(m_thirdChunkBlendStep.value()) / overlapSample, 1.0f);
						output = m_delayBuffer[ch][currentIdx] * rate + (m_delayBuffer[ch][prevIdx] * rate2 + m_delayBuffer[ch][prevPrevIdx] * (1.0f - rate2)) * (1.0f - rate);
						if (ch == m_info.numChannels - 1)
						{
							++m_thirdChunkBlendStep.value();
						}
					}
				}
				else if (step <= overlapSample)
				{
					const float rate = static_cast<float>(step) / overlapSample;
					const std::size_t currentIdx = (kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax;
					const std::size_t overlapIdx = (kDelayBufferMax + m_start - overlapSample + step) % kDelayBufferMax;
					output = m_delayBuffer[ch][currentIdx] * rate + m_delayBuffer[ch][overlapIdx] * (1.0f - rate);
				}
				else
				{
					const std::size_t currentIdx = (kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax;
					output = m_delayBuffer[ch][currentIdx];
				}

				output = m_lowpassFilter[ch].process(output) * mix + *pData * (1.0f - mix);
				*pData = output;
				++pData;
			}

			++m_count;
			if (m_count > m_chunkSize - overlapSample)
			{
				const std::size_t countTimesPlaySpeed = static_cast<std::size_t>((m_count - 1) * m_playSpeed);
				const std::size_t step = countTimesPlaySpeed % (m_chunkSize - overlapSample);
				if (step <= overlapSample)
				{
					m_prevPrevStart = (kDelayBufferMax + m_start - overlapSample + step) % kDelayBufferMax;
					m_prevStart = (kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax;
					m_thirdChunkBlendStep = step + 1;
				}
				else
				{
					m_prevPrevStart = m_prevStart = (kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax;
					m_thirdChunkBlendStep = std::nullopt;
				}
				m_count = 0;
				m_start = m_cursor;
			}
			++m_cursor;
			if (m_cursor >= kDelayBufferMax)
			{
				m_cursor = 0;
			}
		}
		m_pitchPrev = m_pitch;
	}

	void PitchShiftDSP::updateParams(const PitchShiftDSPParams& params)
	{
	}
}
