#pragma once
#include <string>
#include "bass.h"

namespace ksmaudio
{
	class Sample
	{
	private:
		HSAMPLE m_hSample = 0;

	public:
		// Note: filePath must be in UTF-8
		Sample(const std::string& filePath, DWORD maxPolyphony = 1U);

		~Sample();

		Sample(const Sample&) = delete;

		Sample& operator=(const Sample&) = delete;

		Sample(Sample&& other) noexcept;

		Sample& operator=(Sample&& other) noexcept;

		void play(double volume = 1.0) const;
	};
}
