//
//  EdenSound.h
//  The Eden Library
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

#ifndef __EdenSound_h__
#define __EdenSound_h__

#ifndef __Eden_h__
#  include <Eden/Eden.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _EdenSound EdenSound_t;

EdenSound_t *EdenSoundLoad(const char *pathname);
void        EdenSoundUnload(EdenSound_t **sound_p);
void        EdenSoundPlay(EdenSound_t *sound);
EDEN_BOOL   EdenSoundIsPlaying(EdenSound_t *sound);
void        EdenSoundPause(EdenSound_t *sound);
EDEN_BOOL   EdenSoundIsPaused(EdenSound_t *sound);
void        EdenSoundSetLooping(EdenSound_t *sound, EDEN_BOOL looping);
EDEN_BOOL   EdenSoundIsLooping(EdenSound_t *sound);
void        EdenSoundStop(EdenSound_t *sound);
EDEN_BOOL   EdenSoundIsStopped(EdenSound_t *sound);
void        EdenSoundRewind(EdenSound_t *sound);
float       EdenSoundGetPlaybackTimeInSecs(EdenSound_t *sound);

#ifdef __cplusplus
}
#endif //__cplusplus
#endif // !__EdenSound_h__
