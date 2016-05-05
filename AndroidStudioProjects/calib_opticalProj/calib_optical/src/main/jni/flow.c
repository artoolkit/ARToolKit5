/*
 *  flow.c
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
 *  Copyright 2015 Daqri LLC. All Rights Reserved.
 *  Copyright 2013-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "flow.h"

#include <pthread.h>
#include <Eden/EdenMessage.h>
#include "calib_optical.h"


//
// Globals.
//

static bool gInited = false;
static FLOW_STATE gState = FLOW_STATE_NOT_INITED;
static pthread_mutex_t gStateLock;
static pthread_mutex_t gEventLock;
static pthread_cond_t gEventCond;
static EVENT_t gEvent = EVENT_NONE;
static EVENT_t gEventMask = EVENT_NONE;
static pthread_t gThread;
static int gThreadExitStatus;
static bool gStop;

static bool gStereoDisplay = false;
static int gEyeSelection = 0;


// Status bar.
#define STATUS_BAR_MESSAGE_BUFFER_LEN 128
unsigned char statusBarMessage[STATUS_BAR_MESSAGE_BUFFER_LEN] = "";
#define HELP_MESSAGE_BUFFER_LEN 1024
unsigned char helpMessage[HELP_MESSAGE_BUFFER_LEN] = "";

//
// Function prototypes.
//

static void *flowThread(void *arg);

//
// Functions.
//

bool flowInitAndStart(bool stereoDisplay, int eyeSelection)
{
	 pthread_mutex_init(&gStateLock, NULL);
	 pthread_mutex_init(&gEventLock, NULL);
	 pthread_cond_init(&gEventCond, NULL);

     // Inputs.
	 gStereoDisplay = stereoDisplay;
	 gEyeSelection = eyeSelection;

	 gStop = false;
	 pthread_create(&gThread, NULL, flowThread, NULL);

	 gInited = true;

	 return (true);
}

bool flowStopAndFinal()
{
	void *exit_status_p;		 // Pointer to return value from thread, will be filled in by pthread_join().

	if (!gInited) return (false);

	// Request stop and wait for join.
	gStop = true;
#ifndef ANDROID
	pthread_cancel(gThread); // Not implemented on Android.
#endif
#ifdef DEBUG
	LOGE("flowStopAndFinal(): Waiting for flowThread() to exit...\n");
#endif
	pthread_join(gThread, &exit_status_p);
#ifdef DEBUG
#  ifndef ANDROID
	LOGE("  done. Exit status was %d.\n",((exit_status_p == PTHREAD_CANCELED) ? 0 : *(int *)(exit_status_p))); // Contents of gThreadExitStatus.
#  else
	LOGE("  done. Exit status was %d.\n", *(int *)(exit_status_p)); // Contents of gThreadExitStatus.
#  endif
#endif

	// Clean up.
	pthread_mutex_destroy(&gStateLock);
	pthread_mutex_destroy(&gEventLock);
	pthread_cond_destroy(&gEventCond);
	gState = FLOW_STATE_NOT_INITED;
	gInited = false;

	return true;
}

FLOW_STATE flowStateGet()
{
	FLOW_STATE ret;

	if (!gInited) return (FLOW_STATE_NOT_INITED);

	pthread_mutex_lock(&gStateLock);
	ret = gState;
	pthread_mutex_unlock(&gStateLock);
	return (ret);
}

static void flowStateSet(FLOW_STATE state)
{
	if (!gInited) return;

	pthread_mutex_lock(&gStateLock);
	gState = state;
	pthread_mutex_unlock(&gStateLock);
}

static void flowSetEventMask(const EVENT_t eventMask)
{
	pthread_mutex_lock(&gEventLock);
	gEventMask = eventMask;
	pthread_mutex_unlock(&gEventLock);
}

bool flowHandleEvent(const EVENT_t event)
{
	bool ret;

	if (!gInited) return false;

	pthread_mutex_lock(&gEventLock);
	if ((event & gEventMask) == EVENT_NONE) {
		ret = false; // not handled (discarded).
	} else {
		gEvent = event;
		pthread_cond_signal(&gEventCond);
		ret = true;
	}
	pthread_mutex_unlock(&gEventLock);

	return (ret);
}

static EVENT_t flowWaitForEvent(void)
{
	EVENT_t ret;

	pthread_mutex_lock(&gEventLock);
	while (gEvent == EVENT_NONE && !gStop) {
#ifdef ANDROID
        // Android "Bionic" libc doesn't implement cancelation, so need to let wait expire somewhat regularly.
        const struct timespec twoSeconds = {2, 0};
        pthread_cond_timedwait_relative_np(&gEventCond, &gEventLock, &twoSeconds);
#else
		pthread_cond_wait(&gEventCond, &gEventLock);
#endif
	}
	ret = gEvent;
	gEvent = EVENT_NONE; // Clear wait state.
	pthread_mutex_unlock(&gEventLock);

	return (ret);
}

static void flowThreadCleanup(void *arg)
{
	pthread_mutex_unlock(&gStateLock);
}

static void *flowThread(void *arg)
{
	int 	 calibrationEyeCount;
	VIEW_EYE_t calibrationEye;
	int co1, co2;

	bool captureDoneSinceBackButtonLastPressed;
	EVENT_t event;
	// TYPE* TYPE_INSTANCE = (TYPE *)arg; // Cast the thread start arg to the correct type.

    ARLOG("Start flow thread.\n");

    // Register our cleanup function, with no arg.
	pthread_cleanup_push(flowThreadCleanup, NULL);

	// Welcome.
	flowStateSet(FLOW_STATE_WELCOME);

	while (!gStop) {

		for (calibrationEyeCount = 0; calibrationEyeCount < (gStereoDisplay ? 2 : 1); calibrationEyeCount++) {

			if (calibrationEyeCount == 0) calibrationEye = (gEyeSelection ? VIEW_RIGHTEYE : VIEW_LEFTEYE);
			else calibrationEye = (gEyeSelection ? VIEW_LEFTEYE : VIEW_RIGHTEYE);

			char *buf;
			asprintf(&buf, "%s"
					"Touch to begin a calibration run for the %s eye.\n\n"
					"Press Menu for settings and help.",
					(flowStateGet() == FLOW_STATE_WELCOME ? "Welcome to ARToolKit Optical See-Through Calibrator\n(c)2014 ARToolworks, Inc.\n\n" : ""),
					(calibrationEye == VIEW_LEFTEYE ? "left" : "right")
					);
			EdenMessageShow((unsigned char *)buf);
			free(buf);
			flowSetEventMask(EVENT_TOUCH);
			event = flowWaitForEvent();
			if (gStop) break;
			EdenMessageHide();

			// Start capturing.
			captureInit(calibrationEye);

			captureDoneSinceBackButtonLastPressed = false;
			flowStateSet(FLOW_STATE_CAPTURING);
			flowSetEventMask(EVENT_TOUCH|EVENT_BACK_BUTTON);

			bool complete = false;
			bool cancelled = false;


			do {
				snprintf((char *)statusBarMessage, STATUS_BAR_MESSAGE_BUFFER_LEN, "Hold target %s, align with %s eye", (co2 == 1 ? "up close" : "far away"), (calibrationEye == VIEW_LEFTEYE ? "left" : "right"));

				snprintf((char *)helpMessage, HELP_MESSAGE_BUFFER_LEN,
						"Hold the optical calibration marker in view of the camera.\n"
						"(A coloured box will show when marker is in camera's view.)\n"
						"Hold the marker as %s as possible, and using your %s eye\n"
						"visually align the centre of the marker with the crosshairs on the display\n"
						"then touch the screen to capture the position, or press [back] to cancel."
						, (co2 == 1 ? "near" : "far"), (calibrationEye == VIEW_LEFTEYE ? "left" : "right"));

				event = flowWaitForEvent();
				if (gStop) break;
				if (event == EVENT_TOUCH) {

					complete = capture(&co1, &co2);

				    captureDoneSinceBackButtonLastPressed = true;

				} else if (event == EVENT_BACK_BUTTON) {

					if (!captureDoneSinceBackButtonLastPressed) {
						cancelled = true;
					} else {
						cancelled = captureUndo(&co1, &co2);
					}
					captureDoneSinceBackButtonLastPressed = false;
				}


			} while (!complete && !cancelled);

			// Clear status bar and help.
			statusBarMessage[0] = '\0';
			helpMessage[0] = '\0';

			if (cancelled) {

				flowSetEventMask(EVENT_TOUCH);
	            flowStateSet(FLOW_STATE_DONE);
				EdenMessageShow((const unsigned char *)"Calibration canceled");
				flowWaitForEvent();
				if (gStop) break;
				EdenMessageHide();

			} else {

			    const char calibCompleteStringBad[] = "ERROR: calculation of optical display parameters and transformation matrix failed!";
			    const char calibCompleteStringOK[] = "Optical parameters calculated (fovy=%.3f, aspect=%.3f, offset={%.0f, %.0f, %.0f}). Saved to file '%s'.";


			    ARParam param;
				ARdouble fovy, aspect, m[16];

				flowSetEventMask(EVENT_NONE);
				flowStateSet(FLOW_STATE_CALIBRATING);
				EdenMessageShow((const unsigned char *)"Calculating optical parameters...");
				bool ok = (calib(&fovy, &aspect, m) != -1);
	    		EdenMessageHide();

				// Calibration complete. Post results as status.
				flowSetEventMask(EVENT_TOUCH);
				flowStateSet(FLOW_STATE_DONE);

				unsigned char *buf;

				if (!ok) {
	            	asprintf((char **)&buf, calibCompleteStringBad);
				} else {
					char *dir;
				    //const char calibFilenameMono[] = "optical_param.dat";
				    const char calibFilenameL[] = "optical_param_left.dat";
				    const char calibFilenameR[] = "optical_param_right.dat";
					char *paramPathname;

					// Save the parameter file.
					dir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_USER_ROOT, NULL);
				    asprintf(&paramPathname, "%s/%s", dir, (calibrationEye == VIEW_LEFTEYE ? calibFilenameL : calibFilenameR));
				    free(dir);
	            	saveParam(paramPathname, fovy, aspect, m);

	    			asprintf((char **)&buf, calibCompleteStringOK, fovy, aspect, m[12], m[13], m[14], paramPathname);
	            	free(paramPathname);
	            }

				EdenMessageShow(buf);
				free(buf);
				flowWaitForEvent();
				if (gStop) break;
				EdenMessageHide();


			} // cancelled/complete.

			//pthread_testcancel(); // Not implemented on Android.

		} // for (calibrationEyeCount)
	} // while (!gStop);

	pthread_cleanup_pop(1); // Unlocks gStateLock.

    ARLOG("End flow thread.\n");

	gThreadExitStatus = 1; // Put the exit status into a global
	return (&gThreadExitStatus); // Pass a pointer to the global as our exit status.
}


