#pragma once
#include "ksmaudio/AudioEffect/AudioEffect.hpp"
#include "ksmaudio/AudioEffect/Params/LowPassFilterParams.hpp"
#include "ksmaudio/AudioEffect/detail/BiquadFilter.hpp"
#include "ksmaudio/AudioEffect/detail/LinearEasing.hpp"

namespace ksmaudio::AudioEffect
{
	class LowPassFilterDSP
	{
	private:
		const DSPCommonInfo m_info;
		std::array<detail::BiquadFilter<float>, 2> m_lowPassFilters;
		detail::LinearEasing<float> m_vEasing;

	public:
		explicit LowPassFilterDSP(const DSPCommonInfo& info);

		void process(float* pData, std::size_t dataSize, bool bypass, const LowPassFilterDSPParams& params);

		void updateParams(const LowPassFilterDSPParams& params);
	};
}
