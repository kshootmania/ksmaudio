#pragma once
#include "ksmaudio/AudioEffect/AudioEffect.hpp"
#include "ksmaudio/AudioEffect/Params/HighPassFilterParams.hpp"
#include "ksmaudio/AudioEffect/detail/BiquadFilter.hpp"
#include "ksmaudio/AudioEffect/detail/LinearEasing.hpp"

namespace ksmaudio::AudioEffect
{
	class HighPassFilterDSP
	{
	private:
		const DSPCommonInfo m_info;
		std::array<detail::BiquadFilter<float>, 2> m_highPassFilters;
		detail::LinearEasing<float> m_vEasing;

	public:
		explicit HighPassFilterDSP(const DSPCommonInfo& info);

		void process(float* pData, std::size_t dataSize, bool bypass, const HighPassFilterDSPParams& params);

		void updateParams(const HighPassFilterDSPParams& params);
	};
}
