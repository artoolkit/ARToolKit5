//
//  EdenError.c
//
//  Copyright (c) 2004-2012 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//	
//	Rev		Date		Who		Changes
//	1.0.0	2004-06-01	PRL		Pulled together from other headers.
//
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

// ============================================================================
//	Includes
// ============================================================================

#include <string.h>
//#include <stdarg.h>				// va_list, va_start(), va_end()
#include <errno.h>                  // EINVAL, ERANGE
#include <Eden/EdenError.h>


// ============================================================================
//	Private types and defines
// ============================================================================

typedef struct _ERROR_MESSAGE_t {
	EDEN_E_t code;
	char *message;
} ERROR_MESSAGE_t;

// ============================================================================
//	Private globals.
// ============================================================================
const ERROR_MESSAGE_t gErrorMessages[] = {
	
	{   EDEN_E_NONE,				"No error"  },
	{   EDEN_E_OUT_OF_MEMORY,		"Out of memory"  },
	{   EDEN_E_OVERFLOW,			"Overflow"  },
	{   EDEN_E_NODATA,				"Data was requested but none was available"  },
	{   EDEN_E_IOERROR,				"Error during input / output operation"  },
	{   EDEN_E_EOF,					"End of file"	},
	{   EDEN_E_TIMEOUT,				"Timed out"  },
	{   EDEN_E_INVALID_COMMAND,		"Invalid command"  },
	{   EDEN_E_INVALID_ENUM,		"Invalid enumeration"  },
	{   EDEN_E_THREADS,				"An error occurred during a thread management operation"  },
	{   EDEN_E_FILE_NOT_FOUND,		"File not found"  },
	{   EDEN_E_LENGTH_UNAVAILABLE,	"Length not available"  },
	{   EDEN_E_GENERIC,				"Error"  },
	
	{	EDEN_E_LIBRARY_NOT_AVAILABLE, "A required library is not installed"},
	{	EDEN_E_LIBRARY_TOO_OLD,		"The minimum version requirement of a library was not met"},
	{	EDEN_E_LIBRARY_TOO_NEW,		"The maximum version requirement of a library was not met"},	
	{   EDEN_E_GENERIC_TOOLBOX,		"System error"  },

	{   EDEN_E_HARDWARE_NOT_AVAILABLE, "Required hardware is not available"  },
	{   EDEN_E_BIRD_CONFIGURATION,	"The bird hardware is incorrectly configured"  },
	{   EDEN_E_BIRD_PHASEERROR,		"Data from the bird arrived out-of-phase"  },
	{   EDEN_E_HARDWARE_GENERIC,	"Hardware error"	},

	{   EDEN_E_NET_NOT_AVAILABLE,   "Network not available"  },
	{   EDEN_E_NET_NOT_CONNECTED,   "Network not connected"  },
	{   EDEN_E_NET_GENERIC,			"Network error"	},
	
};
const char gErrorMessageUnknown[] = "Unknown error";
#define EDEN_ERROR_MESSAGES_SIZE (sizeof(gErrorMessages) / sizeof(gErrorMessages[0]))

// ============================================================================
//	Public functions.
// ============================================================================
 
const char *EdenError_strerror(const EDEN_E_t code)
{
	int i;
	
	// Do a lame linear search. Is there a better way?
	for (i = 0; i < EDEN_ERROR_MESSAGES_SIZE; i++) {
		if (gErrorMessages[i].code == code) {
			return (gErrorMessages[i].message);
		}
	}
	return (gErrorMessageUnknown);
}

int EdenError_strerror_r(const EDEN_E_t code, char *strerrbuf, const size_t buflen)
{
	int i;
	
	for (i = 0; i < EDEN_ERROR_MESSAGES_SIZE; i++) {
		if (gErrorMessages[i].code == code) {
			strncpy(strerrbuf, gErrorMessages[i].message, buflen);
			if ((strlen(gErrorMessages[i].message) + 1) >= buflen) {
				strerrbuf[buflen - 1] = '\0';
				return (ERANGE);
			} else {
				return (0);
			}
		}
	}
	strncpy(strerrbuf, gErrorMessageUnknown, buflen);
	if ((strlen(gErrorMessageUnknown) + 1) >= buflen) {
		strerrbuf[buflen - 1] = '\0';
		return (ERANGE);
	} else {
		return (EINVAL);
	}
}

void EdenError_perror(const EDEN_E_t code, const char *string)
{
    EDEN_LOGe((string ? "%s: %s" : "%s%s\n"), (string ? string : ""), EdenError_strerror(code));
}
