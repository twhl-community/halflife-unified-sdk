/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "IMusicSystem.h"

#include "OpenALSoundSystem.h"

class OpenALMusicSystem final : public IMusicSystem
{
private:
	//This is tuned to work pretty well with the original Quake soundtrack, but needs testing to make sure it's good for any input.
	static constexpr std::size_t BufferSize = 1024 * 4;

	//Wrapper around FILE* that implements copy behavior as move behavior.
	//This is needed because std::function is copyable, and std::unique_ptr captured by lambda disables the lambda's copy constructor.
	//C++23 has std::move_only_function which solves this problem, but it's not available yet.
	struct FileWrapper
	{
		mutable FILE* File = nullptr;

		FileWrapper(FILE* file) noexcept
			: File(file)
		{
		}

		FileWrapper(FileWrapper&& other) noexcept
			: File(other.File)
		{
			other.File = nullptr;
		}

		FileWrapper& operator=(FileWrapper&& other) noexcept
		{
			if (this != &other)
			{
				Close();
				File = other.File;
				other.File = nullptr;
			}

			return *this;
		}

		FileWrapper(const FileWrapper& other) noexcept
			: File(other.File)
		{
			other.File = nullptr;
		}

		FileWrapper& operator=(const FileWrapper& other) noexcept
		{
			if (this != &other)
			{
				Close();
				File = other.File;
				other.File = nullptr;
			}

			return *this;
		}

		~FileWrapper() noexcept
		{
			Close();
		}

		FILE* get() noexcept
		{
			return File;
		}

		void release() noexcept
		{
			File = nullptr;
		}

		void reset() noexcept
		{
			Close();
		}

	private:
		void Close() noexcept
		{
			if (File)
			{
				fclose(File);
				File = nullptr;
			}
		}
	};

	struct PlaybackInfo
	{
		std::unique_ptr<nqr::AudioData> Data;
		ALenum Format = 0;
		std::size_t NextSampleIndex = 0;
	};

public:
	~OpenALMusicSystem() override;

	bool Create(OpenALSoundSystem& soundSystem);

	void Play(std::string&& fileName, bool looping) override;
	void Stop() override;
	void Pause() override;
	void Resume() override;

private:
	/**
	*	@brief If the current thread is not the worker thread, queues the given function and arguments for execution on the worker thread.
	*	@return @c true if the function was queued, false otherwise.
	*/
	template <typename Func, typename... Args>
	bool RunOnWorkerThread(Func func, Args&&... args);

	void StartPlaying(bool looping, FileWrapper file);

	std::size_t Read(std::byte* dest, std::size_t bufferSize);

	void Run();
	void Update();

	void Music_Command(const CCommandArgs& args);

private:
	cvar_t* m_VolumeCvar{};
	cvar_t* m_FadeTimeCvar{};

	std::string m_FileName;

	std::atomic<bool> m_Enabled = true;
	std::atomic<bool> m_Playing = false;
	std::atomic<bool> m_Paused = false;
	std::atomic<bool> m_Looping = false;

	std::atomic<float> m_Volume = -1; //Set to -1 to force initialization.
	std::atomic<std::chrono::system_clock::time_point> m_FadeStartTime;
	std::atomic<float> m_FadeDuration;

	std::unique_ptr<ALCcontext, DeleterWrapper<alcDestroyContext>> m_Context;

	std::vector<OpenALBuffer> m_Buffers;
	OpenALSource m_Source;

	std::unique_ptr<nqr::NyquistIO> m_Loader;

	PlaybackInfo m_Info;

	std::thread m_Thread;

	std::atomic<bool> m_Quit;

	std::mutex m_JobMutex;

	std::vector<std::function<void()>> m_Jobs;
	std::vector<std::function<void()>> m_JobsToExecute;
};
