#include "SDLSoundService.h"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <string>

namespace dae
{
	// Sound request pushed onto the event queue
	struct SoundRequest
	{
		enum class Type { Play, StopAll, PlayMusic, StopMusic, SetMuted };
		Type type{};
		std::string filePath;
		int volume{ 128 };
		bool loop{ false };
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

			// Destroy the music track before the mixer that owns it
			if (m_pMusicTrack)
			{
				// Detach the stopped-callback so it can't fire mid-teardown
				m_musicShouldPlay = false;
				MIX_SetTrackStoppedCallback(m_pMusicTrack, nullptr, nullptr);
				MIX_DestroyTrack(m_pMusicTrack);
				m_pMusicTrack = nullptr;
			}

			// Audio objects are freed by MIX_Quit, but clear the cache maps
			m_loadedSounds.clear();
			m_loadedMusic.clear();

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
			m_queue.push({ SoundRequest::Type::StopAll, "", 0, false });
			m_cv.notify_one();
		}

		void QueuePlayMusic(const std::string& filePath, bool loop)
		{
			std::lock_guard lock(m_mutex);
			m_queue.push({ SoundRequest::Type::PlayMusic, filePath, 128, loop });
			m_cv.notify_one();
		}

		void QueueStopMusic()
		{
			std::lock_guard lock(m_mutex);
			m_queue.push({ SoundRequest::Type::StopMusic, "", 0, false });
			m_cv.notify_one();
		}

		void QueueSetMuted(bool muted)
		{
			std::lock_guard lock(m_mutex);
			m_queue.push({ SoundRequest::Type::SetMuted, "", 0, muted });
			m_cv.notify_one();
		}

	private:
		// Worker thread loop
		void ProcessQueue()
		{
			while (true)
			{
				SoundRequest request;
				{
					std::unique_lock lock(m_mutex);
					
					m_cv.wait_for(lock, std::chrono::milliseconds(500),
						[this] { return !m_queue.empty() || !m_running; });

					if (!m_running && m_queue.empty())
						break;

					if (m_queue.empty())
					{
						lock.unlock();
						CheckMusicWatchdog();
						continue;
					}

					request = std::move(m_queue.front());
					m_queue.pop();
				}

				switch (request.type)
				{
				case SoundRequest::Type::Play:
					HandlePlay(request.filePath, request.volume);
					break;
				case SoundRequest::Type::StopAll:
					
					m_musicShouldPlay = false;
					if (m_pMixer)
						MIX_StopAllTracks(m_pMixer, 0);
					break;
				case SoundRequest::Type::PlayMusic:
					HandlePlayMusic(request.filePath, request.loop);
					break;
				case SoundRequest::Type::StopMusic:
					m_musicShouldPlay = false;
					if (m_pMusicTrack)
						MIX_StopTrack(m_pMusicTrack, 0);
					break;
				case SoundRequest::Type::SetMuted:
					// Mute by silencing the mixer
					if (m_pMixer)
						MIX_SetMixerGain(m_pMixer, request.loop ? 0.f : 1.f);
					break;
				}

				CheckMusicWatchdog();
			}
		}

		// The streamed music track can die silently (e.g. a failed loop seek),
		// and nothing in SDL_mixer restarts it. If music is supposed to be
		// playing but the track went quiet, start it again.
		void CheckMusicWatchdog()
		{
			if (!m_musicShouldPlay || !m_pMusicTrack) return;
			if (MIX_TrackPlaying(m_pMusicTrack) || MIX_TrackPaused(m_pMusicTrack)) return;

			SDL_Log("SDLSoundService: music track stopped unexpectedly — restarting %s",
				m_currentMusicPath.c_str());
			HandlePlayMusic(m_currentMusicPath, m_currentMusicLoop);
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
				
				audio = MIX_LoadAudio(m_pMixer, filePath.c_str(), true);
				if (audio)
					m_loadedSounds[filePath] = audio;
				else
					SDL_Log("SDLSoundService: failed to load %s: %s",
						filePath.c_str(), SDL_GetError());
			}

			if (audio)
			{
				(void)volume; 
				MIX_PlayAudio(m_pMixer, audio);
			}
		}

		void HandlePlayMusic(const std::string& filePath, bool loop)
		{
			if (!m_pMixer) return;

			
			MIX_Audio* audio = nullptr;
			auto it = m_loadedMusic.find(filePath);
			if (it != m_loadedMusic.end())
			{
				audio = it->second;
			}
			else
			{
				audio = MIX_LoadAudio(m_pMixer, filePath.c_str(), true);
				if (audio)
					m_loadedMusic[filePath] = audio;
				else
				{
					SDL_Log("SDLSoundService: failed to load music %s: %s",
						filePath.c_str(), SDL_GetError());
					return;
				}
			}

			m_currentMusicPath = filePath;
			m_currentMusicLoop = loop;

			
			if (!m_pMusicTrack)
			{
				m_pMusicTrack = MIX_CreateTrack(m_pMixer);
				if (!m_pMusicTrack)
				{
					SDL_Log("SDLSoundService: MIX_CreateTrack failed: %s", SDL_GetError());
					return;
				}

				
				MIX_SetTrackStoppedCallback(m_pMusicTrack,
					[](void* userdata, MIX_Track* track)
					{
						auto* self = static_cast<Impl*>(userdata);
						if (!self->m_musicShouldPlay) return; // intentional stop

						SDL_Log("SDLSoundService: music track stopped unexpectedly — restarting in place");
						SDL_PropertiesID props = SDL_CreateProperties();
						SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER,
							self->m_currentMusicLoop ? -1 : 0);
						if (!MIX_PlayTrack(track, props))
							SDL_Log("SDLSoundService: in-place music restart failed: %s", SDL_GetError());
						SDL_DestroyProperties(props);
					}, this);
			}

			// Suppress the stopped-callback while we restart the track ourselves
			m_musicShouldPlay = false;
			MIX_StopTrack(m_pMusicTrack, 0);
			if (!MIX_SetTrackAudio(m_pMusicTrack, audio))
			{
				SDL_Log("SDLSoundService: MIX_SetTrackAudio failed for %s: %s",
					filePath.c_str(), SDL_GetError());
				return;
			}

			SDL_PropertiesID props = SDL_CreateProperties();
			SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loop ? -1 : 0);
			bool started = MIX_PlayTrack(m_pMusicTrack, props);
			SDL_DestroyProperties(props);

			if (!started)
			{
				// Leave m_musicShouldPlay true: the 500ms watchdog will retry
				SDL_Log("SDLSoundService: MIX_PlayTrack failed for %s: %s",
					filePath.c_str(), SDL_GetError());
			}
			m_musicShouldPlay = true;
		}

		MIX_Mixer* m_pMixer{ nullptr };
		MIX_Track* m_pMusicTrack{ nullptr };

		
		std::string m_currentMusicPath;
		std::atomic<bool> m_currentMusicLoop{ true };
		std::atomic<bool> m_musicShouldPlay{ false };

		std::queue<SoundRequest> m_queue;
		std::mutex m_mutex;
		std::condition_variable m_cv;
		std::thread m_workerThread;
		bool m_running{ false };

		// Sound caches — only accessed from the worker thread
		std::unordered_map<std::string, MIX_Audio*> m_loadedSounds;
		std::unordered_map<std::string, MIX_Audio*> m_loadedMusic;
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

	void SDLSoundService::PlayMusic(const std::string& filePath, bool loop)
	{
		m_pImpl->QueuePlayMusic(filePath, loop);
	}

	void SDLSoundService::StopMusic()
	{
		m_pImpl->QueueStopMusic();
	}

	void SDLSoundService::SetMuted(bool muted)
	{
		m_pImpl->QueueSetMuted(muted);
	}
}
