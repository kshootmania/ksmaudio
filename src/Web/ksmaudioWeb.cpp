#include "ksmaudio/ksmaudio.hpp"

#ifdef KSMAUDIO_BACKEND_WEB

#include "WebAudioEngine.hpp"

namespace ksmaudio
{
	bool Init([[maybe_unused]] void* hWnd, [[maybe_unused]] AudioBackend audioBackend)
	{
		return web::Engine().init();
	}

	void Terminate()
	{
		web::Engine().terminate();
	}

	int GetLastErrorCode()
	{
		// Webバックエンドではエラーコードは未対応
		return 0;
	}

	void SetMute(bool isMute)
	{
		web::Engine().setMuted(isMute);
	}

	void SetMasterVolume(double volume)
	{
		web::Engine().setMasterVolume(volume);
	}
}

#endif
