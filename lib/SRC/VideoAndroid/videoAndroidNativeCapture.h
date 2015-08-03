#ifndef VIDEO_ANDROID_NATIVE_CAPTURE_H
#define VIDEO_ANDROID_NATIVE_CAPTURE_H

#include <AR/video.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _VIDEO_ANDROID_NATIVE_CAPTURE VIDEO_ANDROID_NATIVE_CAPTURE; // Opaque type forward declaration.

// Open a connection to camera cameraIndex.
// After this call returns successfully, the video frame size can be set and/or queried.
VIDEO_ANDROID_NATIVE_CAPTURE *videoAndroidNativeCaptureOpen(int cameraIndex);

// Start frame capture, and optionally notify the user when frames are ready.
// If callback is non-NULL, when a new frame is ready the callback will be invoked
// on a separate dedicated thread, and passed the userdata. It is recommended that the
// callee immediately fetch the ready frame by calling videoAndroidNativeCaptureGetFrame().
bool videoAndroidNativeCaptureStart(VIDEO_ANDROID_NATIVE_CAPTURE *nc, AR_VIDEO_FRAME_READY_CALLBACK callback, void *userdata);

// Get a frame, if one is ready. If a frame is returned, the caller has exclusive use of the
// frame until the next call to videoAndroidNativeCaptureGetFrame, or until videoAndroidNativeCaptureStop is called.
unsigned char *videoAndroidNativeCaptureGetFrame(VIDEO_ANDROID_NATIVE_CAPTURE *nc);

bool videoAndroidNativeCaptureStop(VIDEO_ANDROID_NATIVE_CAPTURE *nc);

bool videoAndroidNativeCaptureClose(VIDEO_ANDROID_NATIVE_CAPTURE **cap_p);

bool videoAndroidNativeCaptureSetSize(VIDEO_ANDROID_NATIVE_CAPTURE *nc, int width, int height);

bool videoAndroidNativeCaptureGetSize(VIDEO_ANDROID_NATIVE_CAPTURE *nc, int *width, int *height);

bool videoAndroidNativeCaptureSetProperty(VIDEO_ANDROID_NATIVE_CAPTURE *nc, int propIndex, int propValue);

bool videoAndroidNativeCaptureGetProperty(VIDEO_ANDROID_NATIVE_CAPTURE *nc, int propIndex, int *propValue);

bool videoAndroidNativeCaptureApplyProperties(VIDEO_ANDROID_NATIVE_CAPTURE *nc);

#ifdef __cplusplus
}
#endif
#endif // !VIDEO_ANDROID_NATIVE_CAPTURE_H