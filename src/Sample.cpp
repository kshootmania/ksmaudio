#include "ksmaudio/Sample.hpp"
#include <cassert>
#include <utility>

namespace
{
	HSAMPLE LoadSample(const std::string& filePath, DWORD maxPolyphony)
	{
		assert(1 <= maxPolyphony && maxPolyphony <= 65535);
		return BASS_SampleLoad(FALSE, filePath.c_str(), 0, 0, maxPolyphony, BASS_SAMPLE_OVER_POS);
	}
}

namespace ksmaudio
{
	Sample::Sample(const std::string& filePath, DWORD maxPolyphony)
		: m_hSample(LoadSample(filePath, maxPolyphony))
	{
	}

	Sample::~Sample()
	{
		if (m_hSample != 0)
		{
			BASS_SampleFree(m_hSample);
		}
	}

	Sample::Sample(Sample&& other) noexcept
		: m_hSample(std::exchange(other.m_hSample, 0))
	{
	}

	Sample& Sample::operator=(Sample&& other) noexcept
	{
		if (this != &other)
		{
			if (m_hSample != 0)
			{
				BASS_SampleFree(m_hSample);
			}
			m_hSample = std::exchange(other.m_hSample, 0);
		}
		return *this;
	}

	void Sample::play(double volume) const
	{
		if (m_hSample == 0)
		{
			return;
		}
		const HCHANNEL hChannel = BASS_SampleGetChannel(m_hSample, FALSE);
		if (hChannel == 0)
		{
			return;
		}
		BASS_ChannelSetAttribute(hChannel, BASS_ATTRIB_VOL, static_cast<float>(volume));
		BASS_ChannelPlay(hChannel, TRUE);
	}
}
