//
//  AudioThread.h
//  Snakebite
//
//  Created by Stephen Harrison-Daly on 19/04/2017.
//  Copyright © 2017 Stephen Harrison-Daly. All rights reserved.
//

#pragma once

#include "Threading.h"
#include "AudioQueue.h"
#include "Common.h"

namespace FMOD
{
	class Sound;
	class Channel;
	class System;
	class ChannelGroup;
}

namespace shd
{
	// Represents a piece of audio
	struct AudioData
	{
		// The size of the sound buffer
		size_t bufferSize;

		// Buffer containing the sound data
		uint8_t * pBuffer;

		// FMOD sound object
		FMOD::Sound * pSound;

		// FMOD channel
		FMOD::Channel * pChannel;

		// Constructor
		AudioData() : bufferSize(0), pBuffer(nullptr), pSound(nullptr), pChannel(nullptr)
		{}

		// Destructor
		~AudioData()
		{
			if (pBuffer)
			{
				SHD_FREE(pBuffer);
			}
		}
	};

	class AudioThread
	{

	public:

		static const int MAX_VOLUME = 10;

		// Contructor
		AudioThread() : m_threadHandle(nullptr), m_pSystem(nullptr), m_pGroupSFX(nullptr), m_pGroupMusic(nullptr)
		{}

		// Destructor
		~AudioThread() {}

		// Initialise the audio system
		bool init();

		// Please stop the audio thread please
		void term();

		// Play a sound
		bool playSound(SoundDesc type, SoundAction action);

	private:

		// Disable copying
		DISABLE_COPY(AudioThread);

		// Entry func for our thread
		static void threadEntry(void * args);

		// Load all audio files into memory
		bool loadAudioFiles();

		// Audio queue
		AudioQueue m_commandQueue;

		// All of the audio data
		AudioData m_audioData[SOUNDS_MAX];

		// Handle to our thread
		Threading::ThreadHandle m_threadHandle;

		// Should the thread end?
		volatile uint32_t m_endThread;

		// FMOD audio system
		FMOD::System * m_pSystem;

		// The SFX group
		FMOD::ChannelGroup * m_pGroupSFX;

		// The music group
		FMOD::ChannelGroup * m_pGroupMusic;

		// The previous volume levels
		uint32_t m_previousVolumeSFX;
		uint32_t m_previousVolumeMusic;
	};
}