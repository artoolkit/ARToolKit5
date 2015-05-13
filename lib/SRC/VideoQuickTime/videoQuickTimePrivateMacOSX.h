/*
 *	Copyright (c) 2005-2012 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
 *	
 *	Rev		Date		Who		Changes
 *	1.0.0	2005-03-08	PRL		Written.
 *
 */

#ifndef __videoQuickTimePrivateMacOSX_h__
#define __videoQuickTimePrivateMacOSX_h__

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#ifdef __cplusplus
extern "C" {
#endif

OSStatus RequestSGSettings(const int inputIndex, SeqGrabComponent seqGrab, SGChannel sgchanVideo, const int showDialog, const int standardDialog);

#ifdef __cplusplus
}
#endif

#endif // __videoQuickTimePrivateMacOSX_h__