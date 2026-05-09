#include "SDLSoundService.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>
#include <string>

namespace dae
{
	// Sound request pushed onto the event queue
	struct SoundRequest
	{
		enum class Type { Play, StopAll };
		Type type{};
		std::string filePath;
		int volume{ 128 };
	};

	// Pimpl — all SDL_mixer usage is contained here
	class SDLSoundService::Impl
	{
	public:
		Impl()
		{
			if (!MIX_Init())
			{
				SDL_Log("SDLSoundService: MIX_Init failed: %s", SDL_GetError());
				return;
			}

			m_pMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
			if (!m_pMixer)
			{
				SDL_Log("SDLSoundService: MIX_CreateMixerDevice failed: %s", SDL_GetError());
				return;
			}

			m_running = true;
			m_workerThread = std::thread([this] { ProcessQueue(); });
		}

		~Impl()
		{
			// Signal the worker thread to stop
			{
				std::lock_guard lock(m_mutex);
				m_running = false;
			}
			m_cv.notify_one();

			if (m_workerThread.joinable())
				m_workerThread.join();

			// Audio objects are freed by MIX_Quit, but clear the cache map
			m_loadedSounds.clear();

			if (m_pMixer)
				MIX_DestroyMixer(m_pMixer);

			MIX_Quit();
		}

		void QueuePlay(const std::string& filePath, int volume)
		{
			std::lock_guard lock(m_mutex);
			m_queue.push({ SoundRequest::Type::Play, filePath, volume });
			m_cv.notify_one();
		}

		void QueueStopAll()
		{
			std::lock_guard lock(m_mutex);
			m_queue.push({ SoundRequest::Type::StopAll, "", 0 });
			m_cv.notify_one();
		}

	private:
		// Worker thread loop — processes sound requests from the event queue
		void ProcessQueue()
		{
			while (true)
			{
				SoundRequest request;
				{
					std::unique_lock lock(m_mutex);
					m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });

					if (!m_running && m_queue.empty())
						break;

					request = std::move(m_queue.front());
					m_queue.pop();
				}

				switch (request.type)
				{
				case SoundRequest::Type::Play:
					HandlePlay(request.filePath, request.volume);
					break;
				case SoundRequest::Type::StopAll:
					if (m_pMixer)
						MIX_StopAllTracks(m_pMixer, 0);
					break;
				}
			}
		}

		void HandlePlay(const std::string& filePath, int volume)
		{
			if (!m_pMixer) return;

			MIX_Audio* audio = nullptr;

			// Check cache first
			auto it = m_loadedSounds.find(filePath);
			if (it != m_loadedSounds.end())
			{
				audio = it->second;
			}
			else
			{
				// Load from disk (predecode = true for sound effects)
				audio = MIX_LoadAudio(m_pMixer, filePath.c_str(), true);
				if (audio)
					m_loadedSounds[filePath] = audio;
				else
					SDL_Log("SDLSoundService: failed to load %s: %s",
						filePath.c_str(), SDL_GetError());
			}

			if (audio)
			{
				(void)volume; // Volume control requires track-based playback
				MIX_PlayAudio(m_pMixer, audio);
			}
		}

		MIX_Mixer* m_pMixer{ nullptr };

		std::queue<SoundRequest> m_queue;
		std::mutex m_mutex;
		std::condition_variable m_cv;
		std::thread m_workerThread;
		bool m_running{ false };

		// Sound cache — only accessed from the worker thread
		std::unordered_map<std::string, MIX_Audio*> m_loadedSounds;
	};

	// SDLSoundService forwards everything to the Impl

	SDLSoundService::SDLSoundService()
		: m_pImpl(std::make_unique<Impl>())
	{
	}

	SDLSoundService::~SDLSoundService() = default;

	void SDLSoundService::PlaySound(const std::string& filePath, int volume)
	{
		m_pImpl->QueuePlay(filePath, volume);
	}

	void SDLSoundService::StopAllSounds()
	{
		m_pImpl->QueueStopAll();
	}
}
