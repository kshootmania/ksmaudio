#include "ksmaudio/audio_effect/dsp/pitch_shift_dsp.hpp"
#include <cmath>
#include <algorithm>
#include <cassert>
#include <vector>

namespace ksmaudio::AudioEffect
{
	PitchShiftDSP::PitchShiftDSP(const DSPCommonInfo& info)
		: m_info(info)
	{
		m_start = kDelayBufferMax - m_chunkSize;
		m_startp = kDelayBufferMax - m_chunkSize - static_cast<int>(m_overlap * m_chunkSize);
		m_startp2 = kDelayBufferMax - m_chunkSize - static_cast<int>(m_overlap * m_chunkSize);
	}

	void PitchShiftDSP::process(float* pData, std::size_t dataSize, bool bypass, const PitchShiftDSPParams& params)
	{
		if (m_info.isUnsupported || bypass)
		{
			return;
		}

		assert(dataSize % m_info.numChannels == 0);
		const std::size_t numFrames = dataSize / m_info.numChannels;

		// TODO: 持ち方を変更して不要化
		std::vector<float> leftChannel(numFrames);
		std::vector<float> rightChannel(numFrames);
		std::vector<float> leftChannelOut(numFrames);
		std::vector<float> rightChannelOut(numFrames);

		for (std::size_t i = 0; i < numFrames; ++i)
		{
			leftChannel[i] = pData[i * m_info.numChannels + 0];
			rightChannel[i] = pData[i * m_info.numChannels + 1];
		}

		float* inputs[2] = { leftChannel.data(), rightChannel.data() };
		float* outputs[2] = { leftChannelOut.data(), rightChannelOut.data() };

		m_pitch = params.pitch;
		const float mix = params.mix;
		m_playSpeed = std::pow(2.0f, m_pitch / 12.0f);
		m_chunkSize = static_cast<int>(params.chunkSize);
		m_overlap = params.overlap;

		if (m_pitch != m_pitchPrev)
		{
			if (m_playSpeed > 1.0f)
			{
				m_lowpassFilter[kChannelLeft].setLowPassFilter(m_info.sampleRateFloat / 2 / m_playSpeed, 0.707f, m_info.sampleRateFloat);
				m_lowpassFilter[kChannelRight].setLowPassFilter(m_info.sampleRateFloat / 2 / m_playSpeed, 0.707f, m_info.sampleRateFloat);
			}
			else
			{
				m_lowpassFilter[kChannelLeft].setLowPassFilter(m_info.sampleRateFloat / 2, 0.707f, m_info.sampleRateFloat);
				m_lowpassFilter[kChannelRight].setLowPassFilter(m_info.sampleRateFloat / 2, 0.707f, m_info.sampleRateFloat);
			}
		}

		const int overlapSample = static_cast<int>(m_overlap * m_chunkSize);
		for (int i = 0; i < numFrames; i++)
		{
			m_delayBuffer[kChannelLeft][m_cursor] = inputs[kChannelLeft][i];
			m_delayBuffer[kChannelRight][m_cursor] = inputs[kChannelRight][i];
			const int step = static_cast<int>(m_count * m_playSpeed) % (m_chunkSize - overlapSample);
			if ((m_pitch == 0.0f) || (mix == 0.0f))
			{
				outputs[kChannelLeft][i] = inputs[kChannelLeft][i];
				outputs[kChannelRight][i] = inputs[kChannelRight][i];
			}
			else
			{
				if (static_cast<int>(m_count * m_playSpeed) <= overlapSample)
				{
					if (m_step2 < 0)
					{
						const float rate = static_cast<float>(step) / overlapSample;
						outputs[kChannelLeft][i] = m_delayBuffer[kChannelLeft][(kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax] * rate + m_delayBuffer[kChannelLeft][(m_startp + step) % kDelayBufferMax] * (1.0f - rate);
						outputs[kChannelRight][i] = m_delayBuffer[kChannelRight][(kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax] * rate + m_delayBuffer[kChannelRight][(m_startp + step) % kDelayBufferMax] * (1.0f - rate);
					}
					else
					{
						const float rate = static_cast<float>(step) / overlapSample;
						const float rate2 = std::min(static_cast<float>(m_step2) / overlapSample, 1.0f);
						outputs[kChannelLeft][i] = m_delayBuffer[kChannelLeft][(kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax] * rate + (m_delayBuffer[kChannelLeft][(m_startp + step) % kDelayBufferMax] * rate2 + m_delayBuffer[kChannelLeft][(m_startp2 + step) % kDelayBufferMax] * (1.0f - rate2)) * (1.0f - rate);
						outputs[kChannelRight][i] = m_delayBuffer[kChannelRight][(kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax] * rate + (m_delayBuffer[kChannelRight][(m_startp + step) % kDelayBufferMax] * rate2 + m_delayBuffer[kChannelRight][(m_startp2 + step) % kDelayBufferMax] * (1.0f - rate2)) * (1.0f - rate);
						m_step2++;
					}
				}
				else if (step <= overlapSample)
				{
					const float rate = static_cast<float>(step) / overlapSample;
					outputs[kChannelLeft][i] = m_delayBuffer[kChannelLeft][(kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax] * rate + m_delayBuffer[kChannelLeft][(kDelayBufferMax + m_start - overlapSample + step) % kDelayBufferMax] * (1.0f - rate);
					outputs[kChannelRight][i] = m_delayBuffer[kChannelRight][(kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax] * rate + m_delayBuffer[kChannelRight][(kDelayBufferMax + m_start - overlapSample + step) % kDelayBufferMax] * (1.0f - rate);
				}
				else
				{
					outputs[kChannelLeft][i] = m_delayBuffer[kChannelLeft][(kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax];
					outputs[kChannelRight][i] = m_delayBuffer[kChannelRight][(kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax];
				}

				outputs[kChannelLeft][i] = m_lowpassFilter[kChannelLeft].process(outputs[kChannelLeft][i]) * mix + inputs[kChannelLeft][i] * (1.0f - mix);
				outputs[kChannelRight][i] = m_lowpassFilter[kChannelRight].process(outputs[kChannelRight][i]) * mix + inputs[kChannelRight][i] * (1.0f - mix);
			}
			m_count++;
			if (m_count > m_chunkSize - overlapSample)
			{
				if (step <= overlapSample)
				{
					m_startp2 = (kDelayBufferMax + m_start - overlapSample + step) % kDelayBufferMax;
					m_startp = (kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax;
					m_step2 = step + 1;
				}
				else
				{
					m_startp2 = m_startp = (kDelayBufferMax + m_start - m_chunkSize + step) % kDelayBufferMax;
					m_step2 = -1;
				}
				m_count = 0;
				m_start = m_cursor;
			}
			m_cursor++;
			if (m_cursor >= kDelayBufferMax)
			{
				m_cursor = 0;
			}
		}
		m_pitchPrev = m_pitch;

		for (std::size_t i = 0; i < numFrames; ++i)
		{
			pData[i * m_info.numChannels + 0] = outputs[kChannelLeft][i];
			pData[i * m_info.numChannels + 1] = outputs[kChannelRight][i];
		}
	}

	void PitchShiftDSP::updateParams(const PitchShiftDSPParams& params)
	{
	}
}
