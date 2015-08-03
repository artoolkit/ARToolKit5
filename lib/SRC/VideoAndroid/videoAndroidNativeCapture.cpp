/*
 *  videoAndroidNativeCapture.cpp
 *  ARToolKit5
 *
 *  Copyright 2012-2014 ARToolworks, Inc.. All rights reserved.
 *  Author(s): Philip Lamb
 *
 */

//
//  A light wrapper around OpenCV's CameraActivity, exposing a C API.
//  Link against libopencv_androidcamera.
//

#include "videoAndroidNativeCapture.h"

#include "camera_properties.h"
#include "camera_activity.hpp" 
#include <thread_sub.h>
#include <AR/ar.h>

class ARToolKitVideoAndroidCameraActivity; // Forward declaration.

extern "C" {

typedef enum {
    EMPTY = -1,     // Buffer is empty. Available for writing.
    READY = 0,      // Buffer is full, but client has not yet been notified.
    NOTIFIED = 1,   // Buffer is full, and client has been notified (don't notify again).
    LOCKED = 2      // Buffer is full, and client is using it.
} VIDEO_ANDROID_NATIVE_FRAMEBUFFERSTATUS;

struct _VIDEO_ANDROID_NATIVE_CAPTURE {
    ARToolKitVideoAndroidCameraActivity *ca;
    pthread_mutex_t frameLock;
    pthread_cond_t frameReadyNotifierThreadCondGo;
    pthread_t frameReadyNotifierThread;
    bool frameReadyNotifierThreadShouldQuit;
    int frameWidth;
    int frameHeight;
    int frameBufferLength;
    unsigned char *frameBuffers[2];
    VIDEO_ANDROID_NATIVE_FRAMEBUFFERSTATUS frameBuffersStatus[2];
    AR_VIDEO_FRAME_READY_CALLBACK frameReadyCallback;
    void *frameReadyCallbackUserdata;
};

// Private function prototypes.
static void *frameReadyNotifier(void *arg);

// Implements CameraActivity::onFrameBuffer callback.
class ARToolKitVideoAndroidCameraActivity : public CameraActivity
{
    
private:
    
    VIDEO_ANDROID_NATIVE_CAPTURE *m_nc;
    int m_framesReceived;
    
public:
    
    ARToolKitVideoAndroidCameraActivity(VIDEO_ANDROID_NATIVE_CAPTURE *nc)
    {
        ARLOGd("ARToolKitVideoAndroidCameraActivity CTOR\n");
        m_nc = nc;
        m_framesReceived = 0;
    }
    
    virtual bool onFrameBuffer(void *buffer, int bufferSize)
    {
        int frameIndex;
        bool ret;
        
        if (!isConnected() || !buffer || bufferSize <= 0) {
            ARLOGe("Error: onFrameBuffer() called while not connected, or called without frame.\n");
            return false;
        }
        
        ret = true;
        m_framesReceived++;
        
        pthread_mutex_lock(&m_nc->frameLock);
        if (m_nc->frameBuffers[0] && m_nc->frameBuffers[1]) { // Only do copy if capture has been started.
            if (bufferSize != m_nc->frameBufferLength) {
                ARLOGe("Error: onFrameBuffer frame size is %d but receiver expected %d.\n", bufferSize, m_nc->frameBufferLength);
                ret = false;
            } else {
                // Find a buffer to write to. Any buffer not locked by client is a candidate.
                if      (m_nc->frameBuffersStatus[0] != LOCKED) frameIndex = 0;
                else if (m_nc->frameBuffersStatus[1] != LOCKED) frameIndex = 1;
                else frameIndex = -1;
                if (frameIndex == -1) {
                    ARLOGe("Error: onFrameBuffer receiver was all full up.\n");
                    ret = false;
                } else {
                    ARLOGd("FRAME => buffer %d %p\n", frameIndex, m_nc->frameBuffers[frameIndex]);
                    memcpy(m_nc->frameBuffers[frameIndex], buffer, bufferSize);
                    m_nc->frameBuffersStatus[frameIndex] = READY;
                    if (m_nc->frameReadyCallback) pthread_cond_signal(&m_nc->frameReadyNotifierThreadCondGo);
                }
            }
        } else {
            ARLOGd("FRAME =X\n");
        }
        pthread_mutex_unlock(&m_nc->frameLock);
        
        return ret;
    }
    
    void LogFramesRate()
    {
        ARLOGi("Frames received: %d\n", m_framesReceived);
    }

};

VIDEO_ANDROID_NATIVE_CAPTURE *videoAndroidNativeCaptureOpen(int cameraIndex)
{
    CameraActivity::ErrorCode ca_err;
    
    ARLOGd("videoAndroidNativeCaptureOpen(%d).\n", cameraIndex);
    
    VIDEO_ANDROID_NATIVE_CAPTURE *nc = (VIDEO_ANDROID_NATIVE_CAPTURE *)calloc(1, sizeof(VIDEO_ANDROID_NATIVE_CAPTURE));
    if (!nc) {
        ARLOGe("Out of memory!\n");
        return (NULL);
    }
    
    nc->ca = new ARToolKitVideoAndroidCameraActivity(nc);
    if (!nc->ca) {
        ARLOGe("Unable to create native connection to camera.\n");
        goto bail;
    }
    
    // Lock manages contention between user thread, CameraActivity::onFrameBuffer thread (might be same as user thread), and frameReadyNotifierThread.
    pthread_mutex_init(&nc->frameLock, NULL);
    pthread_cond_init(&nc->frameReadyNotifierThreadCondGo, NULL);
    
    ca_err = nc->ca->connect(cameraIndex);
    if (ca_err != CameraActivity::NO_ERROR) {
        ARLOGe("Error %d opening native connection to camera.\n", ca_err);
        goto bail1;
    }
    nc->frameWidth = (int)nc->ca->getProperty(ANDROID_CAMERA_PROPERTY_FRAMEWIDTH);
    nc->frameHeight = (int)nc->ca->getProperty(ANDROID_CAMERA_PROPERTY_FRAMEHEIGHT);
    
    ARLOGd("/videoAndroidNativeCaptureOpen %dx%d.\n", nc->frameWidth, nc->frameHeight);
    
    return (nc);

bail1:
    delete(nc->ca);
    pthread_cond_destroy(&nc->frameReadyNotifierThreadCondGo);
    pthread_mutex_destroy(&nc->frameLock);
bail:
    free(nc);
    return (NULL);
}

bool videoAndroidNativeCaptureStart(VIDEO_ANDROID_NATIVE_CAPTURE *nc, AR_VIDEO_FRAME_READY_CALLBACK callback, void *userdata)
{
    int err;
    bool ret = true;
    
    ARLOGd("videoAndroidNativeCaptureStart().\n");

    if (!nc) return false;

    // Don't start if already started.
    if (nc->frameBuffers[0] || nc->frameBuffers[1]) {
        ARLOGe("videoAndroidNativeCaptureStart called again.\n");
        return false;
    }
        
    // Create the frame buffers.
    pthread_mutex_lock(&nc->frameLock);
    nc->frameBufferLength = (nc->frameWidth * nc->frameHeight * 3) / 2; // Assume NV21/NV12 format.
    nc->frameBuffersStatus[0] = nc->frameBuffersStatus[1] = EMPTY;
    nc->frameBuffers[0] = (unsigned char *)malloc(nc->frameBufferLength);
    nc->frameBuffers[1] = (unsigned char *)malloc(nc->frameBufferLength);
    if (!nc->frameBuffers[0] || !nc->frameBuffers[1]) {
        ARLOGe("Out of memory!\n");
        ret = false;
    } else {
        nc->frameReadyCallback = callback;
        if (callback) {
            // Start the frameReadyNotifierThread.
            nc->frameReadyCallbackUserdata = userdata;
            nc->frameReadyNotifierThreadShouldQuit = false;
            if ((err = pthread_create(&(nc->frameReadyNotifierThread), NULL, frameReadyNotifier, (void *)nc)) != 0) {
                ARLOGe("videoAndroidNativeCaptureOpen(): Error %d detaching thread.\n", err);
                ret = false;
            }
        }
    }
    pthread_mutex_unlock(&nc->frameLock);
    
    ARLOGd("/videoAndroidNativeCaptureStart nc->frameBufferLength=%d.\n", nc->frameBufferLength);

    return ret;
}
    

// Thread which tells the user when new frames are ready.
static void *frameReadyNotifier(void *arg)
{
    ARLOGd("Start frameReadyNotifier thread.\n");

    VIDEO_ANDROID_NATIVE_CAPTURE *nc = (VIDEO_ANDROID_NATIVE_CAPTURE *)arg;
    if (!nc) {
        ARLOGe("Error: frameReadyNotifier thread with no arg.\n");
        return NULL;
    }
    
    pthread_mutex_lock(&nc->frameLock);
    
    while (!nc->frameReadyNotifierThreadShouldQuit) {
        
        // Wait for a frame or quit signal.
        while (!nc->frameReadyNotifierThreadShouldQuit && !(nc->frameBuffersStatus[0] == READY) && !(nc->frameBuffersStatus[1] == READY)) {
            pthread_cond_wait(&nc->frameReadyNotifierThreadCondGo, &nc->frameLock); // Releases lock while waiting, reacquires it when done.
        }
        
        if (nc->frameReadyNotifierThreadShouldQuit) break;
        
        // Unlock frameLock during the notification, to allow the other frame buffer to be written concurrently, or the callee to get the frame.
        pthread_mutex_unlock(&nc->frameLock);
        (*nc->frameReadyCallback)(nc->frameReadyCallbackUserdata); // Invoke callback.
        pthread_mutex_lock(&nc->frameLock);
        
        // Mark the buffer(s) as notified, so that in the event the user doesn't pick it up, we don't keep on notifying.
        if (nc->frameBuffersStatus[0] == READY) nc->frameBuffersStatus[0] = NOTIFIED;
        if (nc->frameBuffersStatus[1] == READY) nc->frameBuffersStatus[1] = NOTIFIED;
    }
    
    pthread_mutex_unlock(&nc->frameLock);
    
    ARLOGd("End frameReadyNotifier thread.\n");
    return (NULL);
}

unsigned char *videoAndroidNativeCaptureGetFrame(VIDEO_ANDROID_NATIVE_CAPTURE *nc)
{
    int frameReadyIndex;

    if (!nc) return NULL;
    
    pthread_mutex_lock(&nc->frameLock);

    // Check back in any and all frames which were checked out.
    if (nc->frameBuffersStatus[0] == LOCKED) nc->frameBuffersStatus[0] = EMPTY;
    if (nc->frameBuffersStatus[1] == LOCKED) nc->frameBuffersStatus[1] = EMPTY;
    
    if      (nc->frameBuffersStatus[0] == READY || nc->frameBuffersStatus[0] == NOTIFIED) frameReadyIndex = 0;
    else if (nc->frameBuffersStatus[1] == READY || nc->frameBuffersStatus[1] == NOTIFIED) frameReadyIndex = 1;
    else {
        pthread_mutex_unlock(&nc->frameLock);
        return NULL;
    }

    // Check the frame out and return it to the caller.
    nc->frameBuffersStatus[frameReadyIndex] = LOCKED;
    pthread_mutex_unlock(&nc->frameLock);
    return (nc->frameBuffers[frameReadyIndex]);
}

bool videoAndroidNativeCaptureStop(VIDEO_ANDROID_NATIVE_CAPTURE *nc)
{
    ARLOGd("videoAndroidNativeCaptureStop().\n");
    
    if (!nc) return false;

    // Don't stop if not started.
    if (!nc->frameBuffers[0] && !nc->frameBuffers[1]) {
        return false;
    }
    
    pthread_mutex_lock(&nc->frameLock);
    
    // Cancel any ready frames.
    if (nc->frameBuffersStatus[0] == READY || nc->frameBuffersStatus[0] == NOTIFIED) nc->frameBuffersStatus[0] = EMPTY;
    if (nc->frameBuffersStatus[1] == READY || nc->frameBuffersStatus[1] == NOTIFIED) nc->frameBuffersStatus[1] = EMPTY;

    // Get frameReadyNotifier to exit. Once this is done, we can safely dispose of any remaining locked buffers.
    if (nc->frameReadyCallback) {
        nc->frameReadyNotifierThreadShouldQuit = true;
        pthread_cond_signal(&nc->frameReadyNotifierThreadCondGo); // Make sure thread leaves condition if its waiting there.
        pthread_mutex_unlock(&nc->frameLock);
        pthread_join(nc->frameReadyNotifierThread, NULL); // NULL -> don't return thread exit status.
        pthread_mutex_lock(&nc->frameLock);
        nc->frameReadyNotifierThreadShouldQuit = false;
        nc->frameReadyCallback = NULL;
        nc->frameReadyCallbackUserdata = NULL;
    }
    
    free(nc->frameBuffers[0]);
    free(nc->frameBuffers[1]);
    nc->frameBuffers[0] = nc->frameBuffers[1] = NULL;
    nc->frameBuffersStatus[0] = nc->frameBuffersStatus[1] = EMPTY;
    nc->frameBufferLength = 0;
    
    pthread_mutex_unlock(&nc->frameLock);
    
    ARLOGd("/videoAndroidNativeCaptureStop.\n");
    
    return true;
}

bool videoAndroidNativeCaptureClose(VIDEO_ANDROID_NATIVE_CAPTURE **nc_p)
{
    ARLOGd("videoAndroidNativeCaptureClose().\n");
    
    if (!nc_p || !*nc_p) return (false); // Sanity check.
    
    if ((*nc_p)->frameBuffers[0] || (*nc_p)->frameBuffers[1]) {
        ARLOGw("Warning: videoAndroidNativeCaptureClose called without call to videoAndroidNativeCaptureStop.\n");
        videoAndroidNativeCaptureStop(*nc_p);
    }
    
    pthread_mutex_destroy(&((*nc_p)->frameLock));
    pthread_cond_destroy(&((*nc_p)->frameReadyNotifierThreadCondGo));

    if ((*nc_p)->ca) {
        //ca->disconnect() will be automatically called inside destructor;
        delete((*nc_p)->ca); 
        (*nc_p)->ca = NULL;
    }
    
    free(*nc_p);
    *nc_p = NULL;
    
    ARLOGd("/videoAndroidNativeCaptureClose.\n");
    
    return (true);
}

bool videoAndroidNativeCaptureSetSize(VIDEO_ANDROID_NATIVE_CAPTURE *nc, int width, int height)
{
    AR_VIDEO_FRAME_READY_CALLBACK frameReadyCallback;
    void *frameReadyCallbackUserdata;

    ARLOGd("videoAndroidNativeCaptureSetSize().\n");

    if (!nc || !nc->ca) return false;
    
    pthread_mutex_lock(&nc->frameLock);
    bool capturing = (nc->frameBuffers[0] || nc->frameBuffers[0]);
    pthread_mutex_unlock(&nc->frameLock);
    
    if (capturing) {
        frameReadyCallback = nc->frameReadyCallback;
        frameReadyCallbackUserdata = nc->frameReadyCallbackUserdata;
        videoAndroidNativeCaptureStop(nc);
    }

    nc->ca->setProperty(ANDROID_CAMERA_PROPERTY_FRAMEWIDTH, width);
    nc->ca->setProperty(ANDROID_CAMERA_PROPERTY_FRAMEHEIGHT, height);
    videoAndroidNativeCaptureApplyProperties(nc);
    nc->frameWidth = (int)nc->ca->getProperty(ANDROID_CAMERA_PROPERTY_FRAMEWIDTH);
    nc->frameHeight = (int)nc->ca->getProperty(ANDROID_CAMERA_PROPERTY_FRAMEHEIGHT);

    if (capturing) videoAndroidNativeCaptureStart(nc, frameReadyCallback, frameReadyCallbackUserdata);
    
    ARLOGd("/videoAndroidNativeCaptureSetSize.\n");
    
    return true;
}

bool videoAndroidNativeCaptureGetSize(VIDEO_ANDROID_NATIVE_CAPTURE *nc, int *width, int *height)
{
    if (!nc || !nc->ca || !width || !height) return false;
    
    *width = nc->frameWidth;
    *height = nc->frameHeight;
    
    return true;
}
    
bool videoAndroidNativeCaptureSetProperty(VIDEO_ANDROID_NATIVE_CAPTURE *nc, int propIndex, int propValue)
{
    if (!nc || !nc->ca) return false;

    if (propIndex == ANDROID_CAMERA_PROPERTY_FRAMEWIDTH || propIndex == ANDROID_CAMERA_PROPERTY_FRAMEHEIGHT) {
        ARLOGe("Error: videoAndroidNativeCaptureSetProperty(): frame width/height must be set with videoAndroidNativeCaptureSetSize().\n");
        return false;
    }
    nc->ca->setProperty(propIndex, propValue);

    return true;
}

bool videoAndroidNativeCaptureGetProperty(VIDEO_ANDROID_NATIVE_CAPTURE *nc, int propIndex, int *propValue)
{
    if (!nc || !nc->ca || !propValue) return false;
    
    *propValue = (int)nc->ca->getProperty(propIndex);
    
    return true;
}

bool videoAndroidNativeCaptureApplyProperties(VIDEO_ANDROID_NATIVE_CAPTURE *nc)
{
    int ret = true;
    
    if (!nc || !nc->ca) return false;
    
    nc->ca->applyProperties();

    return ret;
}
    
} // extern "C"

