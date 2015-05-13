//
//  EdenSound.c
//
//	Copyright (c) 2012-2013 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//
//	Rev		Date		Who		Changes
//  1.0     2012-12-23  PRL     Initial version
//

// @@BEGIN_EDEN_LICENSE_HEADER@@
//
//  This file is part of The Eden Library.
//
//  The Eden Library is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  The Eden Library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with The Eden Library.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
// @@END_EDEN_LICENSE_HEADER@@

#include <Eden/EdenSound.h>
#include <stdlib.h>

#ifdef EDEN_HAVE_OPENAL
#  include "ALFramework.h"
#endif // EDEN_HAVE_OPENAL

static unsigned int refCount = 0;

struct _EdenSound {
#ifdef EDEN_HAVE_OPENAL
    ALuint      uiBuffer;
    ALuint      uiSource;
#else
	int dummy;
#endif // EDEN_HAVE_OPENAL
};

EdenSound_t *EdenSoundLoad(const char *pathname)
{
    EdenSound_t *sound = NULL;

#ifdef EDEN_HAVE_OPENAL
    // One-time initialisation.
    if (refCount == 0) {
#ifdef EDEN_HAVE_OPENAL
        ALFWInit();
        if (!ALFWInitOpenAL()) {
            ALFWprintf("Failed to initialize OpenAL\n");
            ALFWShutdown();
        }
#endif // EDEN_HAVE_OPENAL
    }
    
    if (!(sound = (EdenSound_t *)calloc(1, sizeof(EdenSound_t)))) {
        EDEN_LOGe("Out of memory!\n");
        return (NULL);
    }
    
#ifdef EDEN_HAVE_OPENAL
    alGenBuffers(1, &sound->uiBuffer);
#ifdef _WIN32
    if (!ALFWLoadWaveToBuffer(pathname, sound->uiBuffer)) {
        EDEN_LOGe("Failed to load .wav file '%s'.\n", pathname);
    }
#endif
#ifdef __APPLE__
    if (!ALFWLoadFileToBuffer(pathname, sound->uiBuffer)) {
        EDEN_LOGe("Failed to load audio file '%s'.\n", pathname);
    }
#endif
    alGenSources(1, &sound->uiSource);
    alSourcei(sound->uiSource, AL_BUFFER, sound->uiBuffer);
#endif // EDEN_HAVE_OPENAL

    refCount++;
#endif // EDEN_HAVE_OPENAL
    
    return (sound);
}

void EdenSoundUnload(EdenSound_t **sound_p)
{
    if (!sound_p) return;
    if (!(*sound_p)) return;
    
#ifdef EDEN_HAVE_OPENAL
    alSourceStop((*sound_p)->uiSource);
    alDeleteSources(1, &(*sound_p)->uiSource);
    alDeleteBuffers(1, &(*sound_p)->uiBuffer);
#endif // EDEN_HAVE_OPENAL
    
    free(*sound_p);
    *sound_p = NULL;

    // One-time cleanup.
    refCount--;
    if (refCount == 0) {
#ifdef EDEN_HAVE_OPENAL
        ALFWShutdownOpenAL();
        ALFWShutdown();
#endif // EDEN_HAVE_OPENAL
    }
}

void EdenSoundPlay(EdenSound_t *sound)
{
    if (!sound) return;

#ifdef EDEN_HAVE_OPENAL
    alSourcePlay(sound->uiSource);
#endif // EDEN_HAVE_OPENAL
}

void EdenSoundPause(EdenSound_t *sound)
{
    if (!sound) return;
    
#ifdef EDEN_HAVE_OPENAL
    alSourcePause(sound->uiSource);
#endif // EDEN_HAVE_OPENAL
}

void EdenSoundRewind(EdenSound_t *sound)
{
    if (!sound) return;
    
#ifdef EDEN_HAVE_OPENAL
    alSourceRewind(sound->uiSource);
#endif // EDEN_HAVE_OPENAL
}

void EdenSoundStop(EdenSound_t *sound)
{
    if (!sound) return;
    
#ifdef EDEN_HAVE_OPENAL
    alSourceStop(sound->uiSource);
#endif // EDEN_HAVE_OPENAL
}

/*
 Source State Query
 The application can query the current state of any source using alGetSource with the parameter name AL_SOURCE_STATE. Each source can be in one of four possible execution states: AL_INITIAL, AL_PLAYING, AL_PAUSED, AL_STOPPED. Sources that are either AL_PLAYING or AL_PAUSED are considered active. Sources that are AL_STOPPED or AL_INITIAL are considered inactive. Only AL_PLAYING sources are included in the processing. The implementation is free to skip those processing stages for sources that have no effect on the output (e.g. mixing for a source muted by zero GAIN, but not sample offset increments). Depending on the current state of a source certain (e.g. repeated) state transition commands are legal NOPs: they will be ignored, no error is generated.
 State Transition Commands
 The default state of any source is INITIAL. From this state it can be propagated to any other state by appropriate use of the commands below. There are no irreversible state transitions.
 
 void alSourcePlay (ALuint sName);
 void alSourcePause (ALuint sName);
 void alSourceStop (ALuint sName);
 void alSourceRewind (ALuint sName);
 
 The following state/command/state transitions are defined:
 
 alSourcePlay applied to an AL_INITIAL source will promote the source to AL_PLAYING, thus the data found in the buffer will be fed into the processing, starting at the beginning. alSourcePlay applied to a AL_PLAYING source will restart the source from the beginning. It will not affect the configuration, and will leave the source in AL_PLAYING state, but reset the sampling offset to the beginning. alSourcePlay applied to a AL_PAUSED source will resume processing using the source state as preserved at the alSourcePause operation. alSourcePlay applied to a AL_STOPPED source will propagate it to AL_INITIAL then to AL_PLAYING immediately.
 alSourcePause applied to an AL_INITIAL source is a legal NOP. alSourcePause applied to a AL_PLAYING source will change its state to AL_PAUSED. The source is exempt from processing, its current state is preserved. alSourcePause applied to a AL_PAUSED source is a legal NOP. alSourcePause applied to a AL_STOPPED source is a legal NOP.
 alSourceStop applied to an AL_INITIAL source is a legal NOP. alSourceStop applied to a AL_PLAYING source will change its state to AL_STOPPED. The source is exempt from processing, its current state is preserved. alSourceStop applied to a AL_PAUSED source will change its state to AL_STOPPED, with the same consequences as on a AL_PLAYING source. alSourceStop applied to a AL_STOPPED source is a legal NOP.
 alSourceRewind applied to an AL_INITIAL source is a legal NOP. alSourceRewind applied to a AL_PLAYING source will change its state to AL_STOPPED then AL_INITIAL. The source is exempt from processing: its current state is preserved, with the exception of the sampling offset, which is reset to the beginning. alSourceRewind applied to a AL_PAUSED source will change its state to AL_INITIAL, with the same consequences as on a AL_PLAYING source. alSourceRewind applied to an AL_STOPPED source promotes the source to AL_INITIAL, resetting the sampling offset to the beginning.
 Resetting Configuration
 Promoting a source to the AL_INITIAL state using alSourceRewind will not reset the source's properties. AL_INITIAL merely indicates that the source can be executed using the alSourcePlay command. An AL_STOPPED or AL_INITIAL source can be reset into the default configuration by using a sequence of source commands as necessary. As the application has to specify all relevant state anyway to create a useful source configuration, no reset command is provided.
 */
EDEN_BOOL EdenSoundIsPlaying(EdenSound_t *sound)
{
#ifdef EDEN_HAVE_OPENAL
    ALint iState;
#endif // EDEN_HAVE_OPENAL

    if (!sound) return (FALSE);

#ifdef EDEN_HAVE_OPENAL
    alGetSourcei(sound->uiSource, AL_SOURCE_STATE, &iState);
    return (iState == AL_PLAYING);
#else
    return (FALSE);
#endif // EDEN_HAVE_OPENAL
}

EDEN_BOOL EdenSoundIsPaused(EdenSound_t *sound)
{
#ifdef EDEN_HAVE_OPENAL
    ALint iState;
#endif // EDEN_HAVE_OPENAL

    if (!sound) return (FALSE);

#ifdef EDEN_HAVE_OPENAL
    alGetSourcei(sound->uiSource, AL_SOURCE_STATE, &iState);
    return (iState == AL_PAUSED);
#else
    return (FALSE);
#endif // EDEN_HAVE_OPENAL
}

void EdenSoundSetLooping(EdenSound_t *sound, EDEN_BOOL looping)
{
    if (!sound) return;

#ifdef EDEN_HAVE_OPENAL
    alSourcei(sound->uiSource, AL_LOOPING, looping);
#endif // EDEN_HAVE_OPENAL
}

EDEN_BOOL EdenSoundIsLooping(EdenSound_t *sound)
{
#ifdef EDEN_HAVE_OPENAL
    ALint looping;
#endif // EDEN_HAVE_OPENAL

    if (!sound) return (FALSE);

#ifdef EDEN_HAVE_OPENAL
    alGetSourcei(sound->uiSource, AL_LOOPING, &looping);
    return (looping == AL_TRUE);
#else
    return (FALSE);
#endif // EDEN_HAVE_OPENAL
}

EDEN_BOOL EdenSoundIsStopped(EdenSound_t *sound)
{
#ifdef EDEN_HAVE_OPENAL
    ALint iState;
#endif // EDEN_HAVE_OPENAL

    if (!sound) return (FALSE);

#ifdef EDEN_HAVE_OPENAL
    alGetSourcei(sound->uiSource, AL_SOURCE_STATE, &iState);
    return (iState == AL_STOPPED);
#else
    return (FALSE);
#endif // EDEN_HAVE_OPENAL
}

float EdenSoundGetPlaybackTimeInSecs(EdenSound_t *sound)
{
#ifdef EDEN_HAVE_OPENAL
    ALfloat time;
#endif // EDEN_HAVE_OPENAL

    if (!sound) return (0.0f);

#ifdef EDEN_HAVE_OPENAL
    alGetSourcef(sound->uiSource, AL_SEC_OFFSET, &time);
    return (time);
#else
    return (0.0f);
#endif // EDEN_HAVE_OPENAL
}

