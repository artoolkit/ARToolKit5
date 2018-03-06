/*
 *  getInput.c
 *  ARToolKit5
 *
 *  This file is part of ARToolKit.
 *
 *  ARToolKit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ARToolKit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2013-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */


#include "getInput.h"
#include <AR/ar.h>
#include <string.h>
#ifdef _WIN32
#  include <sys/timeb.h>
#  include <windows.h>
#  define MAXPATHLEN MAX_PATH
#else
#  include <time.h>
#  include <sys/time.h>
#endif
#include <pthread.h>

// ASCII keycodes.
#define ASCII_ESC	27
#define ASCII_HT	9
#define ASCII_BS	8
#define ASCII_CR	13
#define ASCII_DEL	127

static unsigned char *inputBuf = NULL;
static unsigned int promptLength = 0;
static unsigned char *inputBufInputPtr = NULL;
static unsigned int inputLength = 0;
static unsigned int inputComplete = FALSE;

static unsigned int inputMinLength;
static unsigned int inputMaxLength;
static unsigned int inputIntOnly;
static unsigned int inputFPOnly;
static unsigned int inputAlphaOnly;

static pthread_mutex_t inputCompleteLock;
static pthread_cond_t inputCompleteCond;

int getInputStart(const unsigned char *prompt, unsigned int minLength, unsigned int maxLength, int intOnly, int fpOnly, int alphaOnly)
{
    if (maxLength == 0) inputMaxLength = INPUT_MAX_LENGTH_DEFAULT;
    else inputMaxLength = maxLength;
    
    if (minLength > inputMaxLength) return (FALSE);
    inputMinLength = minLength;
    
    if (prompt) {
        promptLength = (unsigned int)strlen((const char *)prompt);
    }
    
    if (inputBuf) free(inputBuf);
    inputBuf = (unsigned char *)malloc(promptLength + inputMaxLength + 2); // +1 for cursor and +1 for nul-terminator.
    if (!inputBuf) {
        ARLOGe("Out of memory!!\n");
        return (FALSE);
    }
    if (prompt) {
        strncpy((char *)inputBuf, (const char *)prompt, promptLength);
    }
    inputBufInputPtr = inputBuf + promptLength;
    inputBufInputPtr[0] = '\0';
    inputLength = 0;
    inputComplete = FALSE;
    
    inputIntOnly = intOnly;
    inputFPOnly = fpOnly;
    inputAlphaOnly = alphaOnly;
    
    pthread_mutex_init(&inputCompleteLock, NULL);
    pthread_cond_init(&inputCompleteCond, NULL);
    
    return (TRUE);
}

unsigned char *getInput()
{
    if (inputBufInputPtr) {
        inputBufInputPtr[inputLength] = '\0'; // Overwrite any cursor character.
        return (inputBufInputPtr);
    } else return (NULL);
}

unsigned char *getInputPromptAndInputAndCursor()
{
#ifdef _WIN32
    struct _timeb sys_time;
#else
    struct timeval time;
#endif
    int showCursor;
    
    if (inputBuf) {
#ifdef _WIN32
        _ftime(&sys_time);
        if (sys_time.millitm < 500ul) showCursor = TRUE;
        else showCursor = FALSE;
#else
#  if defined(__linux) || defined(__APPLE__)
        gettimeofday( &time, NULL );
#  else
        gettimeofday( &time );
#  endif
        if (time.tv_usec < 500000) showCursor = TRUE;
        else showCursor = FALSE;
#endif
        if (showCursor) {
            inputBufInputPtr[inputLength] = '|';
            inputBufInputPtr[inputLength + 1] = '\0';
        } else {
            inputBufInputPtr[inputLength] = ' ';
            inputBufInputPtr[inputLength + 1] = '\0';
        }
        return (inputBuf);
    } else return (NULL);
}

int getInputIsComplete()
{
    int ret;
    
    pthread_mutex_lock(&inputCompleteLock);
    ret = inputComplete;
    pthread_mutex_unlock(&inputCompleteLock);
    return (ret);
}

void getInputWaitUntilComplete()
{
    pthread_mutex_lock(&inputCompleteLock);
    if (!inputComplete) {
        pthread_cond_wait(&inputCompleteCond, &inputCompleteLock);
    }
    pthread_mutex_unlock(&inputCompleteLock);
}

static void getInputComplete()
{
    pthread_mutex_lock(&inputCompleteLock);
    inputComplete = TRUE;
    pthread_cond_signal(&inputCompleteCond);
    pthread_mutex_unlock(&inputCompleteLock);
}

void getInputProcessKey(const unsigned char keyAsciiCode)
{
    if (!inputBufInputPtr || inputComplete) return;
    
    switch (keyAsciiCode) {
		case ASCII_ESC:
            free(inputBuf);
            inputBuf = inputBufInputPtr = NULL;
            inputLength = 0;
            getInputComplete();
            break;
		case ASCII_CR:
			if (inputLength >= inputMinLength) {
                getInputComplete();
            }
			break;
        case ASCII_BS:
		case ASCII_DEL:
			if (inputLength > 0) {
				inputLength--;
				inputBufInputPtr[inputLength] = '\0';
			}
			break;
		default:
            if (inputLength == inputMaxLength) break;
			if (keyAsciiCode < 0x20) break; // Throw away all other control characters.
            if (inputIntOnly && (keyAsciiCode < '0' || keyAsciiCode > '9')) break;
            if (inputFPOnly && (keyAsciiCode < '0' || keyAsciiCode > '9') && keyAsciiCode != '.') break;
            if (inputAlphaOnly && (keyAsciiCode < 'A' || keyAsciiCode > 'Z') && (keyAsciiCode < 'a' || keyAsciiCode > 'z')) break;
            inputBufInputPtr[inputLength++] = keyAsciiCode;
            inputBufInputPtr[inputLength] = '\0';
			break;
	}
}

void getInputFinish()
{
    pthread_mutex_destroy(&inputCompleteLock);
    pthread_cond_destroy(&inputCompleteCond);

    if (inputBufInputPtr) {
        free(inputBuf);
        inputBuf = inputBufInputPtr = NULL;
        inputLength = 0;
    }
    inputComplete = FALSE;
}
