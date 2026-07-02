#pragma once
#include <string>
#include <memory>
#include "ksmaudio/Backend.hpp"

namespace ksmaudio
{
	class Sample
	{
	private:
#ifdef KSMAUDIO_BACKEND_BASS
		HSAMPLE m_hSample = 0;
#else
		class Impl;
		std::unique_ptr<Impl> m_impl;
#endif

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
