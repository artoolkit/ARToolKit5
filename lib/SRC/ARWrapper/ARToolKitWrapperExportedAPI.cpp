/*
 *  ARToolKitWrapperExportedAPI.cpp
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
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb, Julian Looser.
 *
 */

// ----------------------------------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------------------------------

#include <ARWrapper/ARToolKitWrapperExportedAPI.h>
#if TARGET_PLATFORM_ANDROID
#  include <ARWrapper/AndroidFeatures.h>
#endif
#include <ARWrapper/ARController.h>
#ifdef DEBUG
#  ifdef _WIN32
#    define MAXPATHLEN MAX_PATH
#    include <direct.h>               // _getcwd()
#    define getcwd _getcwd
#  else
#    include <unistd.h>
#    include <sys/param.h>
#  endif
#endif
#include <stdio.h>
#if !TARGET_PLATFORM_WINDOWS && !TARGET_PLATFORM_WINRT
#  include <pthread.h>
#endif

// ----------------------------------------------------------------------------------------------------

#if defined(_MSC_VER) && !defined(NAN)
#  define __nan_bytes { 0, 0, 0xc0, 0x7f }
static union { unsigned char __c[4]; float __d; } __nan_union = { __nan_bytes };
#  define NAN (__nan_union.__d)
#endif

// ----------------------------------------------------------------------------------------------------

static PFN_LOGCALLBACK logCallback = NULL;
#if !TARGET_PLATFORM_WINDOWS && !TARGET_PLATFORM_WINRT
static pthread_t logThread;
#else
static DWORD logThreadID = 0;
#endif
static int logDumpedWrongThreadCount = 0;

static ARController *gARTK = NULL;

#if !TARGET_PLATFORM_WINRT
static int arwUnityRenderEventUpdateTextureGLTextureID_L = 0;
static int arwUnityRenderEventUpdateTextureGLTextureID_R = 0;
#endif // !TARGET_PLATFORM_WINRT

// ----------------------------------------------------------------------------------------------------

// When handed a logging callback, install it for use by our own log function,
// and pass our own log function as the callback instead.
// This allows us to use buffering to ensure that logging occurs only on the
// same thread that registered the log callback, as required e.g. by C# interop.

void CALL_CONV log(const char *msg);
void CALL_CONV log(const char *msg)
{
	if (logCallback) {
#if !TARGET_PLATFORM_WINDOWS && !TARGET_PLATFORM_WINRT
		if (!pthread_equal(logThread, pthread_self()))
#else
		if (GetCurrentThreadId() != logThreadID)
#endif
		{
			logDumpedWrongThreadCount++;
			return;
		}
		if (logDumpedWrongThreadCount) {
			char s[80];
			sprintf(s, "%d log messages on non-main thread were dumped.\n", logDumpedWrongThreadCount);
			logDumpedWrongThreadCount = 0;
			logCallback(s);
		}
		logCallback(msg);
	} else {
		LOGE("%s\n", msg);
	}
}

EXPORT_API void arwRegisterLogCallback(PFN_LOGCALLBACK callback)
{
	logCallback = callback;
    arLogSetLogger(callback, 1); // 1 -> only callback on same thread.
#if !TARGET_PLATFORM_WINDOWS && !TARGET_PLATFORM_WINRT
	logThread = pthread_self();
#else
	logThreadID = GetCurrentThreadId();
    logDumpedWrongThreadCount = 0;
	//char buf[256];
	//_snprintf(buf, 256, "Registering log callback on thread %d.\n", logThreadID);
	//log(buf);
#endif
}

EXPORT_API void arwSetLogLevel(const int logLevel)
{
    if (logLevel >= 0) {
        arLogLevel = logLevel;
    }
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  ARToolKit lifecycle functions
// ----------------------------------------------------------------------------------------------------

EXPORT_API bool arwInitialiseAR()
{
    if (!gARTK) gARTK = new ARController;
    gARTK->logCallback = log;
	return gARTK->initialiseBase();
}

EXPORT_API bool arwInitialiseARWithOptions(const int pattSize, const int pattCountMax)
{
    if (!gARTK) gARTK = new ARController;
    gARTK->logCallback = log;
    return gARTK->initialiseBase(pattSize, pattCountMax);
}

EXPORT_API bool arwGetARToolKitVersion(char *buffer, int length)
{
	if (!buffer) return false;
    if (!gARTK) return false;
    
	if (const char *version = gARTK->getARToolKitVersion()) {
		strncpy(buffer, version, length - 1); buffer[length - 1] = '\0';
		return true;
	}
	return false;
}

EXPORT_API int arwGetError()
{
    if (!gARTK) return ARW_ERROR_NONE;
    return gARTK->getError();
}

EXPORT_API bool arwChangeToResourcesDir(const char *resourcesDirectoryPath)
{
    bool ok;
#if TARGET_PLATFORM_ANDROID
    if (resourcesDirectoryPath) ok = (arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_SUPPLIED_PATH, resourcesDirectoryPath, NULL) == 0);
	else ok = (arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL, NULL) == 0);
#elif TARGET_PLATFORM_WINRT
	ok = false; // No current working directory in WinRT.
#else
    if (resourcesDirectoryPath) ok = (arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_SUPPLIED_PATH, resourcesDirectoryPath) == 0);
	else ok = (arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL) == 0);
#endif
#ifdef DEBUG
    char buf[MAXPATHLEN];
    gARTK->logv("CWD is '%s'.", getcwd(buf, sizeof(buf)));
#endif
    return (ok);
}

EXPORT_API bool arwStartRunning(const char *vconf, const char *cparaName, const float nearPlane, const float farPlane)
{
    if (!gARTK) return false;
    gARTK->setProjectionNearPlane(nearPlane);
    gARTK->setProjectionFarPlane(farPlane);
	return gARTK->startRunning(vconf, cparaName, NULL, 0);
}

EXPORT_API bool arwStartRunningB(const char *vconf, const char *cparaBuff, const int cparaBuffLen, const float nearPlane, const float farPlane)
{
    if (!gARTK) return false;
    //gARTK->logv("ARWrapper::arwStartRunningB(): called, (ThdID-%d)", GetCurrentThreadId());
    gARTK->setProjectionNearPlane(nearPlane);
    gARTK->setProjectionFarPlane(farPlane);
	return gARTK->startRunning(vconf, NULL, cparaBuff, cparaBuffLen);
}

EXPORT_API bool arwStartRunningStereo(const char *vconfL, const char *cparaNameL, const char *vconfR, const char *cparaNameR, const char *transL2RName, const float nearPlane, const float farPlane)
{
    if (!gARTK) return false;
    //gARTK->logv("ARWrapper::arwStartRunningStereo(): called, (ThdID-%d)", GetCurrentThreadId());
    gARTK->setProjectionNearPlane(nearPlane);
    gARTK->setProjectionFarPlane(farPlane);
	return gARTK->startRunningStereo(vconfL, cparaNameL, NULL, 0L, vconfR, cparaNameR, NULL, 0L, transL2RName, NULL, 0L);
}

EXPORT_API bool arwStartRunningStereoB(const char *vconfL, const char *cparaBuffL, const int cparaBuffLenL, const char *vconfR, const char *cparaBuffR, const int cparaBuffLenR, const char *transL2RBuff, const int transL2RBuffLen, const float nearPlane, const float farPlane)
{
    if (!gARTK) return false;
    //gARTK->logv("ARWrapper::arwStartRunningStereoB(): called, (ThdID-%d)", GetCurrentThreadId());
    gARTK->setProjectionNearPlane(nearPlane);
    gARTK->setProjectionFarPlane(farPlane);
	return gARTK->startRunningStereo(vconfL, NULL, cparaBuffL, cparaBuffLenL, vconfR, NULL, cparaBuffR, cparaBuffLenR, NULL, transL2RBuff, transL2RBuffLen);
}

EXPORT_API bool arwIsRunning()
{
    if (!gARTK) return false;
	return gARTK->isRunning();
}

EXPORT_API bool arwStopRunning()
{
    if (!gARTK) return false;
    //gARTK->logv("ARWrapper::arwStopRunning: called, (ThdID-%d)", GetCurrentThreadId());
	return gARTK->stopRunning();
}

EXPORT_API bool arwShutdownAR()
{
    if (gARTK) {
        delete gARTK; // Delete the ARToolKit instance to initiate shutdown.
        gARTK = NULL;
    }

    // Clean up statics.
#if !TARGET_PLATFORM_WINRT
    arwUnityRenderEventUpdateTextureGLTextureID_L = arwUnityRenderEventUpdateTextureGLTextureID_R = 0;
#endif // !TARGET_PLATFORM_WINRT
    return (true);
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Video stream management
// -------------------------------------------------------------------------------------------------


EXPORT_API bool arwGetProjectionMatrix(float p[16])
{
    if (!gARTK) return false;
    
#ifdef ARDOUBLE_IS_FLOAT
    return gARTK->getProjectionMatrix(0, p);
#else
	ARdouble p0[16];
	if (!gARTK->getProjectionMatrix(0, p0)) {
        return false;
	}
    for (int i = 0; i < 16; i++) p[i] = (float)p0[i];
    return true;
#endif
}

EXPORT_API bool arwGetProjectionMatrixStereo(float pL[16], float pR[16])
{
    if (!gARTK) return false;
    
#ifdef ARDOUBLE_IS_FLOAT
    return (gARTK->getProjectionMatrix(0, pL) && gARTK->getProjectionMatrix(1, pR));
#else
	ARdouble p0L[16];
	ARdouble p0R[16];
	if (!gARTK->getProjectionMatrix(0, p0L) || !gARTK->getProjectionMatrix(1, p0R)) {
        return false;
	}
    for (int i = 0; i < 16; i++) pL[i] = (float)p0L[i];
    for (int i = 0; i < 16; i++) pR[i] = (float)p0R[i];
    return true;
#endif
}

EXPORT_API bool arwGetVideoParams(int *width, int *height, int *pixelSize, char *pixelFormatStringBuffer, int pixelFormatStringBufferLen)
{
    AR_PIXEL_FORMAT pf;
    
    if (!gARTK) return false;
	if (!gARTK->videoParameters(0, width, height, &pf)) return false;
    if (pixelSize) *pixelSize = arUtilGetPixelSize(pf);
    if (pixelFormatStringBuffer && pixelFormatStringBufferLen > 0) {
        strncpy(pixelFormatStringBuffer, arUtilGetPixelFormatName(pf), pixelFormatStringBufferLen);
        pixelFormatStringBuffer[pixelFormatStringBufferLen - 1] = '\0'; // guarantee nul termination.
    }
    return true;
}

EXPORT_API bool arwGetVideoParamsStereo(int *widthL, int *heightL, int *pixelSizeL, char *pixelFormatStringBufferL, int pixelFormatStringBufferLenL, int *widthR, int *heightR, int *pixelSizeR, char *pixelFormatStringBufferR, int pixelFormatStringBufferLenR)
{
    AR_PIXEL_FORMAT pfL, pfR;
    
    if (!gARTK) return false;
	if (!gARTK->videoParameters(0, widthL, heightL, &pfL)) return false;
	if (!gARTK->videoParameters(1, widthR, heightR, &pfR)) return false;
    if (pixelSizeL) *pixelSizeL = arUtilGetPixelSize(pfL);
    if (pixelSizeR) *pixelSizeR = arUtilGetPixelSize(pfR);
    if (pixelFormatStringBufferL && pixelFormatStringBufferLenL > 0) {
        strncpy(pixelFormatStringBufferL, arUtilGetPixelFormatName(pfL), pixelFormatStringBufferLenL);
        pixelFormatStringBufferL[pixelFormatStringBufferLenL - 1] = '\0'; // guarantee nul termination.
    }
    if (pixelFormatStringBufferR && pixelFormatStringBufferLenR > 0) {
        strncpy(pixelFormatStringBufferR, arUtilGetPixelFormatName(pfR), pixelFormatStringBufferLenR);
        pixelFormatStringBufferR[pixelFormatStringBufferLenR - 1] = '\0'; // guarantee nul termination.
    }
    return true;
}

EXPORT_API bool arwCapture()
{
    if (!gARTK) return false;
    return (gARTK->capture());
}

EXPORT_API bool arwUpdateAR()
{
    if (!gARTK) return false;
    return gARTK->update();
}

EXPORT_API bool arwUpdateTexture(Color *buffer)
{
    if (!gARTK) return false;
    return gARTK->updateTexture(0, buffer);
}

EXPORT_API bool arwUpdateTexture32(unsigned int *buffer)
{
    if (!gARTK) return false;
    return gARTK->updateTexture32(0, buffer);
}

EXPORT_API bool arwUpdateTextureStereo(Color *bufferL, Color *bufferR)
{
    if (!gARTK) return false;
    return (gARTK->updateTexture(0, bufferL) && gARTK->updateTexture(1, bufferR));
}

EXPORT_API bool arwUpdateTexture32Stereo(unsigned int *bufferL, unsigned int *bufferR)
{
    if (!gARTK) return false;
    return (gARTK->updateTexture32(0, bufferL) && gARTK->updateTexture32(1, bufferR));
}

EXPORT_API void arwSetVideoDebugMode(bool debug)
{
    if (!gARTK) return;
	gARTK->setDebugMode(debug);
}

EXPORT_API bool arwGetVideoDebugMode()
{
    if (!gARTK) return false;
	return gARTK->getDebugMode();
}

EXPORT_API bool arwUpdateDebugTexture(Color *buffer)
{
    if (!gARTK) return false;
	return gARTK->updateDebugTexture(0, buffer);
}

EXPORT_API bool arwUpdateDebugTexture32(unsigned int *buffer)
{
    if (!gARTK) return false;
    return gARTK->updateDebugTexture32(0, buffer);
}

#if !TARGET_PLATFORM_WINRT
EXPORT_API bool arwUpdateTextureGL(const int textureID)
{
    if (!gARTK) return false;
    return gARTK->updateTextureGL(0, textureID);
}

EXPORT_API bool arwUpdateTextureGLStereo(const int textureID_L, const int textureID_R)
{
    if (!gARTK) return false;
    return (gARTK->updateTextureGL(0, textureID_L) && gARTK->updateTextureGL(1, textureID_R));
}
#endif // !TARGET_PLATFORM_WINRT

// ----------------------------------------------------------------------------------------------------
#pragma mark  Unity-specific API
// ----------------------------------------------------------------------------------------------------

EXPORT_API void UnityRenderEvent(int eventID)
{
    switch (eventID) {
#if !TARGET_PLATFORM_WINRT
        case ARW_UNITY_RENDER_EVENTID_UPDATE_TEXTURE_GL:
            if (arwUnityRenderEventUpdateTextureGLTextureID_L) arwUpdateTextureGL(arwUnityRenderEventUpdateTextureGLTextureID_L);
            break;
        case ARW_UNITY_RENDER_EVENTID_UPDATE_TEXTURE_GL_STEREO:
            if (arwUnityRenderEventUpdateTextureGLTextureID_L && arwUnityRenderEventUpdateTextureGLTextureID_R) arwUpdateTextureGLStereo(arwUnityRenderEventUpdateTextureGLTextureID_L, arwUnityRenderEventUpdateTextureGLTextureID_R);
            break;
#endif // !TARGET_PLATFORM_WINRT
        case ARW_UNITY_RENDER_EVENTID_NOP:
        default:
            break;
    }
}

#if !TARGET_PLATFORM_WINRT
EXPORT_API void arwSetUnityRenderEventUpdateTextureGLTextureID(int textureID)
{
    arwUnityRenderEventUpdateTextureGLTextureID_L = textureID;
}

EXPORT_API void arwSetUnityRenderEventUpdateTextureGLStereoTextureIDs(int textureID_L, int textureID_R)
{
    arwUnityRenderEventUpdateTextureGLTextureID_L = textureID_L;
    arwUnityRenderEventUpdateTextureGLTextureID_R = textureID_R;
}
#endif // !TARGET_PLATFORM_WINRT

// ----------------------------------------------------------------------------------------------------
#pragma mark  Tracking configuration
// ----------------------------------------------------------------------------------------------------

EXPORT_API void arwSetVideoThreshold(int threshold)
{
    if (!gARTK) return;
	gARTK->setThreshold(threshold);
}

EXPORT_API int arwGetVideoThreshold()
{
    if (!gARTK) return 0;
	return gARTK->getThreshold();
}

EXPORT_API void arwSetVideoThresholdMode(int mode)
{
    if (!gARTK) return;
	gARTK->setThresholdMode(mode);
}

EXPORT_API int arwGetVideoThresholdMode()
{
    if (!gARTK) return 0;
	return gARTK->getThresholdMode();
}

EXPORT_API void arwSetLabelingMode(int mode)
{
    if (!gARTK) return;
	gARTK->setLabelingMode(mode);
}

EXPORT_API int arwGetLabelingMode()
{
    if (!gARTK) return 0;
	return gARTK->getLabelingMode();
}

EXPORT_API void arwSetPatternDetectionMode(int mode)
{
    if (!gARTK) return;
	gARTK->setPatternDetectionMode(mode);
}

EXPORT_API int arwGetPatternDetectionMode()
{
    if (!gARTK) return 0;
	return gARTK->getPatternDetectionMode();
}

EXPORT_API void arwSetBorderSize(float size)
{
    if (!gARTK) return;
	gARTK->setPattRatio(1.0f - 2.0f*size);
}

EXPORT_API float arwGetBorderSize()
{
    if (!gARTK) return 0.0f;
	return ((1.0f - gARTK->getPattRatio()) * 0.5f);
}

EXPORT_API void arwSetMatrixCodeType(int type)
{
    if (!gARTK) return;
	gARTK->setMatrixCodeType(type);
}

EXPORT_API int arwGetMatrixCodeType()
{
    if (!gARTK) return 0;
	return gARTK->getMatrixCodeType();
}

EXPORT_API void arwSetImageProcMode(int mode)
{
    if (!gARTK) return;
	gARTK->setImageProcMode(mode);
}

EXPORT_API int arwGetImageProcMode()
{
    if (!gARTK) return 0;
	return gARTK->getImageProcMode();
}

EXPORT_API void arwSetNFTMultiMode(bool on)
{
    if (!gARTK) return;
    gARTK->setNFTMultiMode(on);
}

EXPORT_API bool arwGetNFTMultiMode()
{
    if (!gARTK) return false;
    return gARTK->getNFTMultiMode();
}


// ----------------------------------------------------------------------------------------------------
#pragma mark  Marker management
// ---------------------------------------------------------------------------------------------

EXPORT_API int arwAddMarker(const char *cfg)
{
    if (!gARTK) return -1;
	return gARTK->addMarker(cfg);
}

EXPORT_API bool arwRemoveMarker(int markerUID)
{
    if (!gARTK) return false;
	return gARTK->removeMarker(markerUID);
}

EXPORT_API int arwRemoveAllMarkers()
{
    if (!gARTK) return 0;
	return gARTK->removeAllMarkers();
}

EXPORT_API bool arwQueryMarkerVisibility(int markerUID)
{
    ARMarker *marker;
    
    if (!gARTK) return false;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwQueryMarkerVisibility(): Couldn't locate marker with UID %d.", markerUID);
        return false;
    }
    return marker->visible;
}

EXPORT_API bool arwQueryMarkerTransformation(int markerUID, float matrix[16])
{
    ARMarker *marker;

    if (!gARTK) return false;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwQueryMarkerTransformation(): Couldn't locate marker with UID %d.", markerUID);
        return false;
    }
    for (int i = 0; i < 16; i++) matrix[i] = (float)marker->transformationMatrix[i];
    return marker->visible;
}

EXPORT_API bool arwQueryMarkerTransformationStereo(int markerUID, float matrixL[16], float matrixR[16])
{
    ARMarker *marker;
    
    if (!gARTK) return false;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwQueryMarkerTransformation(): Couldn't locate marker with UID %d.", markerUID);
        return false;
    }
    for (int i = 0; i < 16; i++) matrixL[i] = (float)marker->transformationMatrix[i];
    for (int i = 0; i < 16; i++) matrixR[i] = (float)marker->transformationMatrixR[i];
    return marker->visible;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Marker patterns
// ---------------------------------------------------------------------------------------------

EXPORT_API int arwGetMarkerPatternCount(int markerUID)
{
    ARMarker *marker;
    
    if (!gARTK) return 0;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerPatternCount(): Couldn't locate marker with UID %d.", markerUID);
        return 0;
    }
    return marker->patternCount;
}

EXPORT_API bool arwGetMarkerPatternConfig(int markerUID, int patternID, float matrix[16], float *width, float *height, int *imageSizeX, int *imageSizeY)
{
    ARMarker *marker;
    ARPattern *p;
    
    if (!gARTK) return false;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerPatternConfig(): Couldn't locate marker with UID %d.", markerUID);
        return false;
    }

    if (!(p = marker->getPattern(patternID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerPatternConfig(): Marker with UID %d has no pattern with ID %d.", markerUID, patternID);
        return false;
    }

    if (matrix) {
        for (int i = 0; i < 16; i++) matrix[i] = (float)p->m_matrix[i];
    }
    if (width) *width = (float)p->m_width;
    if (height) *height = (float)p->m_height;
    if (imageSizeX) *imageSizeX = p->m_imageSizeX;
    if (imageSizeY) *imageSizeY = p->m_imageSizeY;
    return true;
}

EXPORT_API bool arwGetMarkerPatternImage(int markerUID, int patternID, Color *buffer)
{
    ARMarker *marker;
    ARPattern *p;
    
    if (!gARTK) return false;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerPatternImage(): Couldn't locate marker with UID %d.", markerUID);
        return false;
    }
    
    if (!(p = marker->getPattern(patternID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerPatternImage(): Marker with UID %d has no pattern with ID %d.", markerUID, patternID);
        return false;
    }

    if (!p->m_image) {
        return false;
    }
    
    memcpy(buffer, p->m_image, sizeof(Color) * p->m_imageSizeX * p->m_imageSizeY);
    return true;

}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Marker options
// ---------------------------------------------------------------------------------------------

EXPORT_API bool arwGetMarkerOptionBool(int markerUID, int option)
{
    ARMarker *marker;
    
    if (!gARTK) return false;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerOptionBool(): Couldn't locate marker with UID %d.", markerUID);
        return false;
    }
    
    switch (option) {
        case ARW_MARKER_OPTION_FILTERED:
            return(marker->isFiltered());
            break;
        case ARW_MARKER_OPTION_SQUARE_USE_CONT_POSE_ESTIMATION:
            if (marker->type == ARMarker::SINGLE) return (((ARMarkerSquare *)marker)->useContPoseEstimation);
            break;
        default:
            gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerOptionBool(): Unrecognised option %d.", option);
            break;
    }
    return(false);
}

EXPORT_API void arwSetMarkerOptionBool(int markerUID, int option, bool value)
{
    ARMarker *marker;
    
    if (!gARTK) return;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwSetMarkerOptionBool(): Couldn't locate marker with UID %d.", markerUID);
        return;
    }

    switch (option) {
        case ARW_MARKER_OPTION_FILTERED:
            marker->setFiltered(value);
            break;
        case ARW_MARKER_OPTION_SQUARE_USE_CONT_POSE_ESTIMATION:
            if (marker->type == ARMarker::SINGLE) ((ARMarkerSquare *)marker)->useContPoseEstimation = value;
            break;
        default:
            gARTK->logv(AR_LOG_LEVEL_ERROR, "arwSetMarkerOptionBool(): Unrecognised option %d.", option);
            break;
    }
}

EXPORT_API int arwGetMarkerOptionInt(int markerUID, int option)
{
    ARMarker *marker;
    
    if (!gARTK) return INT_MIN;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerOptionBool(): Couldn't locate marker with UID %d.", markerUID);
        return (INT_MIN);
    }
    
    switch (option) {
        case ARW_MARKER_OPTION_MULTI_MIN_SUBMARKERS:
            if (marker->type == ARMarker::MULTI) return ((ARMarkerMulti *)marker)->config->min_submarker;
            break;
        default:
            gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerOptionInt(): Unrecognised option %d.", option);
            break;
    }
    return (INT_MIN);
}

EXPORT_API void arwSetMarkerOptionInt(int markerUID, int option, int value)
{
    ARMarker *marker;
    
    if (!gARTK) return;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwSetMarkerOptionInt(): Couldn't locate marker with UID %d.", markerUID);
        return;
    }

    switch (option) {
        case ARW_MARKER_OPTION_MULTI_MIN_SUBMARKERS:
            if (marker->type == ARMarker::MULTI) ((ARMarkerMulti *)marker)->config->min_submarker = value;
            break;
        default:
            gARTK->logv(AR_LOG_LEVEL_ERROR, "arwSetMarkerOptionInt(): Unrecognised option %d.", option);
            break;
    }
}

EXPORT_API float arwGetMarkerOptionFloat(int markerUID, int option)
{
    ARMarker *marker;
    
    if (!gARTK) return (NAN);
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerOptionBool(): Couldn't locate marker with UID %d.", markerUID);
        return (NAN);
    }
    
    switch (option) {
        case ARW_MARKER_OPTION_FILTER_SAMPLE_RATE:
            return ((float)marker->filterSampleRate());
            break;
        case ARW_MARKER_OPTION_FILTER_CUTOFF_FREQ:
            return ((float)marker->filterCutoffFrequency());
            break;
        case ARW_MARKER_OPTION_SQUARE_CONFIDENCE:
            if (marker->type == ARMarker::SINGLE) return ((float)((ARMarkerSquare *)marker)->getConfidence());
            else return (NAN);
            break;
        case ARW_MARKER_OPTION_SQUARE_CONFIDENCE_CUTOFF:
            if (marker->type == ARMarker::SINGLE) return ((float)((ARMarkerSquare *)marker)->getConfidenceCutoff());
            else return (NAN);
            break;
        case ARW_MARKER_OPTION_NFT_SCALE:
#if HAVE_NFT
            if (marker->type == ARMarker::NFT) return ((float)((ARMarkerNFT *)marker)->getNFTScale());
            else return (NAN);
#else
            return (NAN);
#endif
            break;
        case ARW_MARKER_OPTION_MULTI_MIN_CONF_MATRIX:
            if (marker->type == ARMarker::MULTI) return (float)((ARMarkerMulti *)marker)->config->cfMatrixCutoff;
            else return (NAN);
            break;
        case ARW_MARKER_OPTION_MULTI_MIN_CONF_PATTERN:
            if (marker->type == ARMarker::MULTI) return (float)((ARMarkerMulti *)marker)->config->cfPattCutoff;
            else return (NAN);
            break;
        default:
            gARTK->logv(AR_LOG_LEVEL_ERROR, "arwGetMarkerOptionFloat(): Unrecognised option %d.", option);
            break;
    }
    return (NAN);
}

EXPORT_API void arwSetMarkerOptionFloat(int markerUID, int option, float value)
{
    ARMarker *marker;
    
    if (!gARTK) return;
	if (!(marker = gARTK->findMarker(markerUID))) {
        gARTK->logv(AR_LOG_LEVEL_ERROR, "arwSetMarkerOptionFloat(): Couldn't locate marker with UID %d.", markerUID);
        return;
    }

    switch (option) {
        case ARW_MARKER_OPTION_FILTER_SAMPLE_RATE:
            marker->setFilterSampleRate(value);
            break;
        case ARW_MARKER_OPTION_FILTER_CUTOFF_FREQ:
            marker->setFilterCutoffFrequency(value);
            break;
        case ARW_MARKER_OPTION_SQUARE_CONFIDENCE_CUTOFF:
            if (marker->type == ARMarker::SINGLE) ((ARMarkerSquare *)marker)->setConfidenceCutoff(value);
            break;
        case ARW_MARKER_OPTION_NFT_SCALE:
#if HAVE_NFT
            if (marker->type == ARMarker::NFT) ((ARMarkerNFT *)marker)->setNFTScale(value);
#endif
            break;
        case ARW_MARKER_OPTION_MULTI_MIN_CONF_MATRIX:
            if (marker->type == ARMarker::MULTI) ((ARMarkerMulti *)marker)->config->cfMatrixCutoff = value;
            break;
        case ARW_MARKER_OPTION_MULTI_MIN_CONF_PATTERN:
            if (marker->type == ARMarker::MULTI) ((ARMarkerMulti *)marker)->config->cfPattCutoff = value;
            break;
        default:
            gARTK->logv(AR_LOG_LEVEL_ERROR, "arwSetMarkerOptionFloat(): Unrecognised option %d.", option);
            break;
    }
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Utility
// ----------------------------------------------------------------------------------------------------
EXPORT_API bool arwLoadOpticalParams(const char *optical_param_name, const char *optical_param_buff, const int optical_param_buffLen, float *fovy_p, float *aspect_p, float m[16], float p[16])
{
    if (!gARTK) return false;
    
#ifdef ARDOUBLE_IS_FLOAT
    return gARTK->loadOpticalParams(optical_param_name, optical_param_buff, optical_param_buffLen, fovy_p, aspect_p, m, p);
#else
    ARdouble fovy, aspect, m0[16], p0[16];
	if (!gARTK->loadOpticalParams(optical_param_name, optical_param_buff, optical_param_buffLen, &fovy, &aspect, m0, (p ? p0 : NULL))) {
        return false;
    }
    *fovy_p = (float)fovy;
    *aspect_p = (float)aspect;
    for (int i = 0; i < 16; i++) m[i] = (float)m0[i];
    if (p) for (int i = 0; i < 16; i++) p[i] = (float)p0[i];
    return true;
#endif
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Java API
// ----------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------
// Java API
// 
// The following functions provide a JNI compatible wrapper around the first set of 
// exported functions.
// --------------------------------------------------------------------------------------
#if TARGET_PLATFORM_ANDROID

// Utility function to create a Java float array from a C float array
jfloatArray glArrayToJava(JNIEnv *env, ARdouble *arr, int len) {
	jfloatArray result = NULL;
	if ((result = env->NewFloatArray(len))) env->SetFloatArrayRegion(result, 0, len, arr);
	return result;
}

extern "C" {
	JNIEXPORT jstring JNICALL JNIFUNCTION(arwGetARToolKitVersion(JNIEnv *env, jobject obj));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwGetError(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwInitialiseAR(JNIEnv *env, jobject obj));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwInitialiseARWithOptions(JNIEnv *env, jobject obj, jint pattSize, jint pattCountMax));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwChangeToResourcesDir(JNIEnv *env, jobject obj, jstring resourcesDirectoryPath));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwShutdownAR(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStartRunning(JNIEnv *env, jobject obj, jstring vconf, jstring cparaName, float nearPlane, float farPlane));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStartRunningStereo(JNIEnv *env, jobject obj, jstring vconfL, jstring cparaNameL, jstring vconfR, jstring cparaNameR, jstring transL2RName, float nearPlane, float farPlane));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwIsRunning(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStopRunning(JNIEnv *env, jobject obj));
	JNIEXPORT jfloatArray JNICALL JNIFUNCTION(arwGetProjectionMatrix(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetProjectionMatrixStereo(JNIEnv *env, jobject obj, jfloatArray projL, jfloatArray projR));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoParams(JNIEnv *env, jobject obj, jintArray width, jintArray height, jintArray pixelSize, jobjectArray pixelFormatStringBuffer));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoParamsStereo(JNIEnv *env, jobject obj, jintArray widthL, jintArray heightL, jintArray pixelSizeL, jobjectArray pixelFormatStringL, jintArray widthR, jintArray heightR, jintArray pixelSizeR, jobjectArray  pixelFormatStringR));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwCapture(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwUpdateAR(JNIEnv *env, jobject obj));
	JNIEXPORT jint JNICALL JNIFUNCTION(arwAddMarker(JNIEnv *env, jobject obj, jstring cfg));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwRemoveMarker(JNIEnv *env, jobject obj, jint markerUID));
	JNIEXPORT jint JNICALL JNIFUNCTION(arwRemoveAllMarkers(JNIEnv *env, jobject obj));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwQueryMarkerVisibility(JNIEnv *env, jobject obj, jint markerUID));
	JNIEXPORT jfloatArray JNICALL JNIFUNCTION(arwQueryMarkerTransformation(JNIEnv *env, jobject obj, jint markerUID));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwQueryMarkerTransformationStereo(JNIEnv *env, jobject obj, jint markerUID, jfloatArray matrixL, jfloatArray matrixR));
	JNIEXPORT jint JNICALL JNIFUNCTION(arwGetMarkerPatternCount(JNIEnv *env, jobject obj, int markerUID));
	JNIEXPORT void JNICALL JNIFUNCTION(arwSetVideoDebugMode(JNIEnv *env, jobject obj, jboolean debug));
	JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoDebugMode(JNIEnv *env, jobject obj));
	JNIEXPORT void JNICALL JNIFUNCTION(arwSetVideoThreshold(JNIEnv *env, jobject obj, jint threshold));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwGetVideoThreshold(JNIEnv *env, jobject obj));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetVideoThresholdMode(JNIEnv *env, jobject obj, jint mode));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwGetVideoThresholdMode(JNIEnv *env, jobject obj));
	JNIEXPORT void JNICALL JNIFUNCTION(arwSetLabelingMode(JNIEnv *env, jobject obj, jint mode));
	JNIEXPORT jint JNICALL JNIFUNCTION(arwGetLabelingMode(JNIEnv *env, jobject obj));
	JNIEXPORT void JNICALL JNIFUNCTION(arwSetPatternDetectionMode(JNIEnv *env, jobject obj, jint mode));
	JNIEXPORT jint JNICALL JNIFUNCTION(arwGetPatternDetectionMode(JNIEnv *env, jobject obj));
	JNIEXPORT void JNICALL JNIFUNCTION(arwSetBorderSize(JNIEnv *env, jobject obj, jfloat size));
	JNIEXPORT jfloat JNICALL JNIFUNCTION(arwGetBorderSize(JNIEnv *env, jobject obj));
	JNIEXPORT void JNICALL JNIFUNCTION(arwSetMatrixCodeType(JNIEnv *env, jobject obj, jint type));
	JNIEXPORT jint JNICALL JNIFUNCTION(arwGetMatrixCodeType(JNIEnv *env, jobject obj));
	JNIEXPORT void JNICALL JNIFUNCTION(arwSetImageProcMode(JNIEnv *env, jobject obj, jint mode));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwGetImageProcMode(JNIEnv *env, jobject obj));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetNFTMultiMode(JNIEnv *env, jobject obj, jboolean on));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetNFTMultiMode(JNIEnv *env, jobject obj));

    JNIEXPORT void JNICALL JNIFUNCTION(arwSetMarkerOptionBool(JNIEnv *env, jobject obj, jint markerUID, jint option, jboolean value));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetMarkerOptionInt(JNIEnv *env, jobject obj, jint markerUID, jint option, jint value));
    JNIEXPORT void JNICALL JNIFUNCTION(arwSetMarkerOptionFloat(JNIEnv *env, jobject obj, jint markerUID, jint option, jfloat value));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetMarkerOptionBool(JNIEnv *env, jobject obj, jint markerUID, jint option));
    JNIEXPORT jint JNICALL JNIFUNCTION(arwGetMarkerOptionInt(JNIEnv *env, jobject obj, jint markerUID, jint option));
    JNIEXPORT jfloat JNICALL JNIFUNCTION(arwGetMarkerOptionFloat(JNIEnv *env, jobject obj, jint markerUID, jint option));

	// Additional Java-specific function not found in the C-API
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwAcceptVideoImage(JNIEnv *env, jobject obj, jbyteArray pinArray, jint width, jint height, jint cameraIndex, jboolean cameraIsFrontFacing));
    JNIEXPORT jboolean JNICALL JNIFUNCTION(arwAcceptVideoImageStereo(JNIEnv *env, jobject obj, jbyteArray pinArrayL, jint widthL, jint heightL, jint cameraIndexL, jboolean cameraIsFrontFacingL, jbyteArray pinArrayR, jint widthR, jint heightR, jint cameraIndexR, jboolean cameraIsFrontFacingR));

    JNIEXPORT bool JNICALL JNIFUNCTION(arwUpdateDebugTexture32(JNIEnv *env, jobject obj, jbyteArray pinArray));

	// ------------------------------------------------------------------------------------
	// JNI Functions Not Yet Implemented
    //EXPORT_API bool arwStartRunningB(const char *vconf, const char *cparaBuff, const int cparaBuffLen, const float nearPlane, const float farPlane);
    //EXPORT_API bool arwStartRunningStereoB(const char *vconfL, const char *cparaBuffL, const int cparaBuffLenL, const char *vconfR, const char *cparaBuffR, const int cparaBuffLenR, const char *transL2RBuff, const int transL2RBuffLen, const float nearPlane, const float farPlane);
	//EXPORT_API bool arwGetMarkerPatternConfig(int markerUID, int patternID, float matrix[16], float *width, float *height);
	//EXPORT_API bool arwGetMarkerPatternImage(int markerUID, int patternID, Color *buffer);
	//EXPORT_API bool arwUpdateTexture(Color *buffer);
    //EXPORT_API bool arwUpdateTexture32(unsigned int *buffer);
	//EXPORT_API bool arwUpdateTextureStereo(Color *bufferL, Color *bufferR);
    //EXPORT_API bool arwUpdateTexture32Stereo(unsigned int *bufferL, unsigned int *bufferR);
	//EXPORT_API bool arwUpdateTextureGL(const int textureID);
	//EXPORT_API bool arwUpdateTextureGLStereo(const int textureID_L, const int textureID_R);
    //EXPORT_API bool arwUpdateDebugTexture(Color *buffer);

    //EXPORT_API bool arwLoadOpticalParams(const char *optical_param_name, const char *optical_param_buff, const int optical_param_buffLen, float *fovy_p, float *aspect_p, float m[16], float p[16]);
	// ------------------------------------------------------------------------------------
}


JNIEXPORT jstring JNICALL JNIFUNCTION(arwGetARToolKitVersion(JNIEnv *env, jobject obj)) 
{
	char versionString[1024];
    
	if (arwGetARToolKitVersion(versionString, 1024)) return env->NewStringUTF(versionString);		
	return env->NewStringUTF("unknown version");
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetError(JNIEnv *env, jobject obj))
{
    return arwGetError();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwInitialiseAR(JNIEnv *env, jobject obj)) 
{
	return arwInitialiseAR();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwInitialiseARWithOptions(JNIEnv *env, jobject obj, jint pattSize, jint pattCountMax))
{
    return arwInitialiseARWithOptions(pattSize, pattCountMax);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwChangeToResourcesDir(JNIEnv *env, jobject obj, jstring resourcesDirectoryPath)) 
{
    bool ok;
    
    if (resourcesDirectoryPath != NULL) {
        const char *resourcesDirectoryPathC = env->GetStringUTFChars(resourcesDirectoryPath, NULL);
        ok = arwChangeToResourcesDir(resourcesDirectoryPathC);
        env->ReleaseStringUTFChars(resourcesDirectoryPath, resourcesDirectoryPathC);
    } else ok = arwChangeToResourcesDir(NULL);
    
    return ok;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwShutdownAR(JNIEnv *env, jobject obj)) 
{
	return arwShutdownAR();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStartRunning(JNIEnv *env, jobject obj, jstring vconf, jstring cparaName, float nearPlane, float farPlane)) 
{
	const char *vconfC = env->GetStringUTFChars(vconf, NULL);
	const char *cparaNameC = env->GetStringUTFChars(cparaName, NULL);

	bool running = arwStartRunning(vconfC, cparaNameC, nearPlane, farPlane);

	env->ReleaseStringUTFChars(vconf, vconfC);
	env->ReleaseStringUTFChars(cparaName, cparaNameC);

	return running;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStartRunningStereo(JNIEnv *env, jobject obj, jstring vconfL, jstring cparaNameL, jstring vconfR, jstring cparaNameR, jstring transL2RName, float nearPlane, float farPlane)) 
{
	const char *vconfLC = env->GetStringUTFChars(vconfL, NULL);
	const char *cparaNameLC = env->GetStringUTFChars(cparaNameL, NULL);
	const char *vconfRC = env->GetStringUTFChars(vconfR, NULL);
	const char *cparaNameRC = env->GetStringUTFChars(cparaNameR, NULL);
	const char *transL2RNameC = env->GetStringUTFChars(transL2RName, NULL);
    
	bool running = arwStartRunningStereo(vconfLC, cparaNameLC, vconfRC, cparaNameRC, transL2RNameC, nearPlane, farPlane);
    
	env->ReleaseStringUTFChars(vconfL, vconfLC);
	env->ReleaseStringUTFChars(cparaNameL, cparaNameLC);
	env->ReleaseStringUTFChars(vconfR, vconfRC);
	env->ReleaseStringUTFChars(cparaNameR, cparaNameRC);
	env->ReleaseStringUTFChars(transL2RName, transL2RNameC);
    
	return running;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwIsRunning(JNIEnv *env, jobject obj)) 
{
	return arwIsRunning();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwStopRunning(JNIEnv *env, jobject obj)) 
{
	return arwStopRunning();
}

#define PIXEL_FORMAT_BUFFER_SIZE 1024

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoParams(JNIEnv *env, jobject obj, jintArray width, jintArray height, jintArray pixelSize, jobjectArray pixelFormatString))
{
    int w, h, ps;
    char pf[PIXEL_FORMAT_BUFFER_SIZE];
    
    if (!arwGetVideoParams(&w, &h, &ps, pf, PIXEL_FORMAT_BUFFER_SIZE)) return false;
    if (width) env->SetIntArrayRegion(width, 0, 1, &w);
    if (height) env->SetIntArrayRegion(height, 0, 1, &h);
    if (pixelSize) env->SetIntArrayRegion(pixelSize, 0, 1, &ps);
    if (pixelFormatString) env->SetObjectArrayElement(pixelFormatString, 0, env->NewStringUTF(pf));
    return true;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoParamsStereo(JNIEnv *env, jobject obj, jintArray widthL, jintArray heightL, jintArray pixelSizeL, jobjectArray pixelFormatStringL, jintArray widthR, jintArray heightR, jintArray pixelSizeR, jobjectArray  pixelFormatStringR))
{
    int wL, hL, psL, wR, hR, psR;
    char pfL[PIXEL_FORMAT_BUFFER_SIZE], pfR[PIXEL_FORMAT_BUFFER_SIZE];

    if (!arwGetVideoParamsStereo(&wL, &hL, &psL, pfL, PIXEL_FORMAT_BUFFER_SIZE, &wR, &hR, &psR, pfR, PIXEL_FORMAT_BUFFER_SIZE)) return false;
    if (widthL) env->SetIntArrayRegion(widthL, 0, 1, &wL);
    if (heightL) env->SetIntArrayRegion(heightL, 0, 1, &hL);
    if (pixelSizeL) env->SetIntArrayRegion(pixelSizeL, 0, 1, &psL);
    if (pixelFormatStringL) env->SetObjectArrayElement(pixelFormatStringL, 0, env->NewStringUTF(pfL));
    if (widthR) env->SetIntArrayRegion(widthR, 0, 1, &wR);
    if (heightR) env->SetIntArrayRegion(heightR, 0, 1, &hR);
    if (pixelSizeR) env->SetIntArrayRegion(pixelSizeR, 0, 1, &psR);
    if (pixelFormatStringR) env->SetObjectArrayElement(pixelFormatStringR, 0, env->NewStringUTF(pfR));
    return true;
}

JNIEXPORT jfloatArray JNICALL JNIFUNCTION(arwGetProjectionMatrix(JNIEnv *env, jobject obj)) 
{
	float proj[16];
    
	if (arwGetProjectionMatrix(proj)) return glArrayToJava(env, proj, 16);	
	return NULL;
}
	
JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetProjectionMatrixStereo(JNIEnv *env, jobject obj, jfloatArray projL, jfloatArray projR))
{
	float pL[16];
	float pR[16];
    
	if (!arwGetProjectionMatrixStereo(pL, pR)) return false;
    env->SetFloatArrayRegion(projL, 0, 16, pL);
    env->SetFloatArrayRegion(projR, 0, 16, pR);
	return true;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwCapture(JNIEnv *env, jobject obj))
{
	return arwCapture();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwUpdateAR(JNIEnv *env, jobject obj)) 
{
	return arwUpdateAR();
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwAddMarker(JNIEnv *env, jobject obj, jstring cfg)) 
{
	jboolean isCopy;

	const char *cfgC = env->GetStringUTFChars(cfg, &isCopy);
	int markerUID = arwAddMarker(cfgC);
	env->ReleaseStringUTFChars(cfg, cfgC);
	return markerUID;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwRemoveMarker(JNIEnv *env, jobject obj, jint markerUID)) 
{
	return arwRemoveMarker(markerUID);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwRemoveAllMarkers(JNIEnv *env, jobject obj)) 
{
	return arwRemoveAllMarkers();
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwQueryMarkerVisibility(JNIEnv *env, jobject obj, jint markerUID)) 
{
	return arwQueryMarkerVisibility(markerUID);
}

JNIEXPORT jfloatArray JNICALL JNIFUNCTION(arwQueryMarkerTransformation(JNIEnv *env, jobject obj, jint markerUID)) 
{
	float trans[16];
    
	if (arwQueryMarkerTransformation(markerUID, trans)) return glArrayToJava(env, trans, 16);
	return NULL;
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwQueryMarkerTransformationStereo(JNIEnv *env, jobject obj, jint markerUID, jfloatArray matrixL, jfloatArray matrixR)) 
{
	float mL[16];
	float mR[16];
    
	if (!arwQueryMarkerTransformationStereo(markerUID, mL, mR)) return false;
    env->SetFloatArrayRegion(matrixL, 0, 16, mL);
    env->SetFloatArrayRegion(matrixR, 0, 16, mR);
	return true;
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetMarkerPatternCount(JNIEnv *env, jobject obj, int markerUID)) 
{
	return arwGetMarkerPatternCount(markerUID);
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetVideoDebugMode(JNIEnv *env, jobject obj, jboolean debug)) 
{
	arwSetVideoDebugMode(debug);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetVideoDebugMode(JNIEnv *env, jobject obj)) 
{
	return arwGetVideoDebugMode();
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetVideoThreshold(JNIEnv *env, jobject obj, jint threshold)) 
{
	arwSetVideoThreshold(threshold);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetVideoThreshold(JNIEnv *env, jobject obj)) 
{
	return arwGetVideoThreshold();
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetVideoThresholdMode(JNIEnv *env, jobject obj, jint mode))
{
    arwSetVideoThresholdMode(mode);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetVideoThresholdMode(JNIEnv *env, jobject obj))
{
    return (arwGetVideoThresholdMode());
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetLabelingMode(JNIEnv *env, jobject obj, jint mode))
{
    arwSetLabelingMode(mode);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetLabelingMode(JNIEnv *env, jobject obj)) 
{
    return arwGetLabelingMode();
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetPatternDetectionMode(JNIEnv *env, jobject obj, jint mode)) 
{
    arwSetPatternDetectionMode(mode);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetPatternDetectionMode(JNIEnv *env, jobject obj)) 
{
    return arwGetPatternDetectionMode();
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetBorderSize(JNIEnv *env, jobject obj, jfloat size)) 
{
    arwSetBorderSize(size);
}

JNIEXPORT jfloat JNICALL JNIFUNCTION(arwGetBorderSize(JNIEnv *env, jobject obj)) 
{
    return arwGetBorderSize();
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetMatrixCodeType(JNIEnv *env, jobject obj, jint type)) 
{
    arwSetMatrixCodeType(type);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetMatrixCodeType(JNIEnv *env, jobject obj)) 
{
    return arwGetMatrixCodeType();
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetImageProcMode(JNIEnv *env, jobject obj, jint mode)) 
{
    arwSetImageProcMode(mode);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetImageProcMode(JNIEnv *env, jobject obj)) 
{
    return arwGetImageProcMode();
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetMarkerOptionBool(JNIEnv *env, jobject obj, jint markerUID, jint option, jboolean value)) 
{
    return arwSetMarkerOptionBool(markerUID, option, value);
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetNFTMultiMode(JNIEnv *env, jobject obj, jboolean on))
{
    arwSetNFTMultiMode(on);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetNFTMultiMode(JNIEnv *env, jobject obj))
{
    return arwGetNFTMultiMode();
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetMarkerOptionInt(JNIEnv *env, jobject obj, jint markerUID, jint option, jint value))
{
    return arwSetMarkerOptionInt(markerUID, option, value);
}

JNIEXPORT void JNICALL JNIFUNCTION(arwSetMarkerOptionFloat(JNIEnv *env, jobject obj, jint markerUID, jint option, jfloat value)) 
{
    return arwSetMarkerOptionFloat(markerUID, option, value);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwGetMarkerOptionBool(JNIEnv *env, jobject obj, jint markerUID, jint option)) 
{
    return arwGetMarkerOptionBool(markerUID, option);
}

JNIEXPORT jint JNICALL JNIFUNCTION(arwGetMarkerOptionInt(JNIEnv *env, jobject obj, jint markerUID, jint option)) 
{
    return arwGetMarkerOptionInt(markerUID, option);
}

JNIEXPORT jfloat JNICALL JNIFUNCTION(arwGetMarkerOptionFloat(JNIEnv *env, jobject obj, jint markerUID, jint option)) 
{
    return arwGetMarkerOptionFloat(markerUID, option);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwAcceptVideoImage(JNIEnv *env, jobject obj, jbyteArray pinArray, jint width, jint height, jint cameraIndex, jboolean cameraIsFrontFacing)) 
{
    if (!gARTK) return false;
    return gARTK->videoAcceptImage(env, obj, 0, pinArray, width, height, cameraIndex, cameraIsFrontFacing);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION(arwAcceptVideoImageStereo(JNIEnv *env, jobject obj, jbyteArray pinArrayL, jint widthL, jint heightL, jint cameraIndexL, jboolean cameraIsFrontFacingL, jbyteArray pinArrayR, jint widthR, jint heightR, jint cameraIndexR, jboolean cameraIsFrontFacingR))
{
    if (!gARTK) return false;
    return (gARTK->videoAcceptImage(env, obj, 0, pinArrayL, widthL, heightL, cameraIndexL, cameraIsFrontFacingL) &&
            gARTK->videoAcceptImage(env, obj, 1, pinArrayR, widthR, heightR, cameraIndexR, cameraIsFrontFacingR)
            );
}

JNIEXPORT bool  JNICALL JNIFUNCTION(arwUpdateDebugTexture32(JNIEnv *env, jobject obj, jbyteArray pinArray))
{
    if (!gARTK) return false;
    
	bool updated = false;

	if (jbyte *inArray = env->GetByteArrayElements(pinArray, NULL)) {
		updated = arwUpdateDebugTexture32((unsigned int *)inArray);
		env->ReleaseByteArrayElements(pinArray, inArray, 0); // 0 -> copy back the changes on the native side to the Java side.
	}

	return updated;
}

#endif // TARGET_PLATFORM_ANDROID
