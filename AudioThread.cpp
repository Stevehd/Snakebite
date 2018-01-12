//
//  AudioThread.cpp
//  Snakebite
//
//  Created by Stephen Harrison-Daly on 19/04/2017.
//  Copyright © 2017 Stephen Harrison-Daly. All rights reserved.
//

#include "AudioThread.h"
#include "FileIO.h"
#include "Application.h"
#include <fmod.hpp>
#include <string.h>

#define SHD_AUDIO_THREAD_SLEEP_TIME_MS 16.667f

using namespace shd;


const char * shdGameSounds[SOUNDS_MAX] = 
{	
	"./Assets/Audio/audio_cooked/match_goal_net.ogg",
	"./Assets/Audio/audio_cooked/match_kick_1.ogg",
	"./Assets/Audio/audio_cooked/match_kick_2.ogg",
	"./Assets/Audio/audio_cooked/match_kick_3.ogg",
	"./Assets/Audio/audio_cooked/match_kick_4.ogg",
	"./Assets/Audio/audio_cooked/match_kick_5.ogg",
	"./Assets/Audio/audio_cooked/match_kick_6.ogg",

	"./Assets/Audio/audio_cooked/match_slide_tackle.ogg",
	"./Assets/Audio/audio_cooked/match_ball_bounce.ogg",

	"./Assets/Audio/audio_cooked/menu_click_1.ogg",
	"./Assets/Audio/audio_cooked/menu_click_2.ogg",
	"./Assets/Audio/audio_cooked/menu_click_3.ogg",

	"./Assets/Audio/audio_cooked/referee_whistle_1.ogg",
	"./Assets/Audio/audio_cooked/referee_whistle_2.ogg",
	"./Assets/Audio/audio_cooked/referee_whistle_3.ogg",
	"./Assets/Audio/audio_cooked/referee_whistle_4.ogg",
	"./Assets/Audio/audio_cooked/referee_whistle_5.ogg",

	"./Assets/Audio/audio_cooked/stadium_air_horn.ogg",

	"./Assets/Audio/audio_cooked/stadium_ambient_1.ogg",

	"./Assets/Audio/audio_cooked/stadium_boo_1.ogg",
	"./Assets/Audio/audio_cooked/stadium_boo_2.ogg",
	"./Assets/Audio/audio_cooked/stadium_boo_3.ogg",

	"./Assets/Audio/audio_cooked/stadium_cheer_1.ogg",
	"./Assets/Audio/audio_cooked/stadium_cheer_2.ogg",
	"./Assets/Audio/audio_cooked/stadium_cheer_3.ogg",
	"./Assets/Audio/audio_cooked/stadium_cheer_4.ogg",
	"./Assets/Audio/audio_cooked/stadium_cheer_5.ogg",
	"./Assets/Audio/audio_cooked/stadium_cheer_6.ogg",

	"./Assets/Audio/audio_cooked/stadium_ohh_1.ogg",
	"./Assets/Audio/audio_cooked/stadium_ohh_2.ogg",
	"./Assets/Audio/audio_cooked/stadium_ohh_3.ogg",
	"./Assets/Audio/audio_cooked/stadium_ohh_4.ogg",
	"./Assets/Audio/audio_cooked/stadium_ohh_5.ogg",

	"./Assets/Audio/audio_cooked/stadium_random_cheer_1.ogg",
	"./Assets/Audio/audio_cooked/stadium_random_cheer_2.ogg",
	"./Assets/Audio/audio_cooked/stadium_random_cheer_3.ogg",

	"./Assets/Audio/audio_cooked/weapon_pickup_1.ogg",

	"./Assets/Audio/audio_cooked/weapon_shoot_AK47_1.ogg",
	"./Assets/Audio/audio_cooked/weapon_shoot_handgun_1.ogg",
	"./Assets/Audio/audio_cooked/weapon_shoot_handgun_2.ogg",
	"./Assets/Audio/audio_cooked/weapon_shoot_shotgun_1.ogg",
	"./Assets/Audio/audio_cooked/weapon_shoot_uzi_1.ogg",
};

bool AudioThread::init()
{
	bool ret = false;
	FMOD_RESULT fmodResult;
	Threading::ThreadStartParams threadParams;

	m_threadHandle = 0;
	m_endThread = 0;
	m_previousVolumeSFX = Application::getInstance().globalSettings.audioVolumeSFX;
	m_previousVolumeMusic = Application::getInstance().globalSettings.audioVolumeMusic;

	// init the command queue
	ret = m_commandQueue.init();
	if (ret == false)
	{
		return false;
	}

	// Create the main system object
	fmodResult = FMOD::System_Create(&m_pSystem);
	if (fmodResult != FMOD_OK)
	{
		return false;
	}

	// Initialize FMOD
	fmodResult = m_pSystem->init(64, FMOD_INIT_THREAD_UNSAFE, 0);
	if (fmodResult != FMOD_OK)
	{
		return false;
	}

	// Create the groups
	fmodResult = m_pSystem->createChannelGroup("GroupSFX", &m_pGroupSFX);
	if (fmodResult != FMOD_OK)
	{
		return false;
	}

	fmodResult = m_pSystem->createChannelGroup("GroupMusic", &m_pGroupMusic);
	if (fmodResult != FMOD_OK)
	{
		return false;
	}

	threadParams.entryPoint = &threadEntry;
	threadParams.userArgs = this;

    ret = Threading::startThread(threadParams, &m_threadHandle);
	if (ret == false)
	{
		return false;
	}

	return true;
}

void AudioThread::term()
{
	Atomic::exchange32((volatile uint32_t *)&m_endThread, 1);

	// Wait for the thread to end before returning
	Threading::joinThread(m_threadHandle);
}

bool AudioThread::playSound(SoundDesc type, SoundAction action)
{
	return m_commandQueue.push(AudioCommand(type, action));
}

void AudioThread::threadEntry(void * args)
{
	bool ret = false;
    AudioCommand cmd;
    FMOD_RESULT result;
	AudioThread * thread = (AudioThread *)args;

	ret = thread->loadAudioFiles();
	if (ret == false)
	{
		SHD_ASSERT(false);
		return;
	}
    
	while (thread->m_endThread == 0)
	{
		// See if volume has been modified since last time, the update the groups accordingly
		if (thread->m_previousVolumeSFX != Application::getInstance().globalSettings.audioVolumeSFX)
		{
			thread->m_previousVolumeSFX = Application::getInstance().globalSettings.audioVolumeSFX;
			thread->m_pGroupSFX->setVolume((float)thread->m_previousVolumeSFX / (float)AudioThread::MAX_VOLUME);
		}

		if (thread->m_previousVolumeMusic != Application::getInstance().globalSettings.audioVolumeMusic)
		{
			thread->m_previousVolumeMusic = Application::getInstance().globalSettings.audioVolumeMusic;
			thread->m_pGroupMusic->setVolume((float)thread->m_previousVolumeMusic / (float)AudioThread::MAX_VOLUME);
		}

        // Check for new audio commands
        if(thread->m_commandQueue.size())
        {
            cmd = thread->m_commandQueue.pop();
            
			// TODO - other actions, like looping, stopping, etc
            if(cmd.action == SOUND_ACTION_START)
            {
				if (cmd.type >= 0 && cmd.type < SOUNDS_MAX)
				{
					result = thread->m_pSystem->playSound(thread->m_audioData[cmd.type].pSound, thread->m_pGroupSFX, false, &(thread->m_audioData[cmd.type].pChannel));
					if (result != FMOD_OK)
					{
						SHD_PRINTF("FMOD error playing sounds: %i\n", result);
						SHD_ASSERT(false);
					}
				}
            }
        }
        
        // Poll the underlying system
        thread->m_pSystem->update();
        
        // Sleep the thread
        Threading::sleep(SHD_AUDIO_THREAD_SLEEP_TIME_MS);
    }
}

bool AudioThread::loadAudioFiles()
{
	size_t bytesRead = 0;
	int64_t fileSize = 0;
	FMOD_RESULT fmodResult;
	FMOD_CREATESOUNDEXINFO exinfo;

	for (int i = 0; i < SOUNDS_MAX; i++)
	{
		// Get file size
		fileSize = FileIO::getFileSize(shdGameSounds[i]);
		if (fileSize < 1)
		{
			return false;
		}

		uint8_t * buf = (uint8_t *)SHD_MALLOC(fileSize);

		// Load file
		int ret = FileIO::readFile(shdGameSounds[i], buf, (size_t)fileSize, &bytesRead);
		if (ret < 0)
		{
			SHD_FREE(buf);
			return false;
		}

		if (m_audioData[i].pBuffer)
		{
			SHD_FREE(m_audioData[i].pBuffer);
			m_audioData[i].pBuffer = nullptr;
		}

		m_audioData[i].pBuffer = buf;
		m_audioData[i].bufferSize = fileSize;

		memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
		exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		exinfo.length = m_audioData[i].bufferSize;

		// FMOD_CREATESAMPLE instead of FMOD_CREATESTREAM to play the same sound multiple times without resetting
		fmodResult = m_pSystem->createSound((const char *)m_audioData[i].pBuffer, FMOD_OPENMEMORY | FMOD_CREATESAMPLE | FMOD_DEFAULT, &exinfo, &m_audioData[i].pSound);
		if (fmodResult != FMOD_OK)
		{
			return false;
		}

		// For ambient noise - set loop count to infinite
		if (i == SOUND_STADIUM_AMBIENT_1)
		{
			m_audioData[i].pSound->setMode(FMOD_LOOP_NORMAL);
			m_audioData[i].pSound->setLoopCount(-1);
		}
	}

	return true;
}
