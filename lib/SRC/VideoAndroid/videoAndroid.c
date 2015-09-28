/*
 *  videoAndroid.c
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
 *  Copyright 2012-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include <AR/video.h>
#include <stdbool.h>
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
#  include <sys/time.h> // gettimeofday()
#endif

#include "android_os_build_codes.h"
#include "cparamSearch.h"
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
#  include "videoAndroidNativeCapture.h"
#  include "color_convert_common.h"
#endif
#include <dlfcn.h>

struct _AR2VideoParamAndroidT {
    char               device_id[PROP_VALUE_MAX*3+2]; // From <sys/system_properties.h>. 3 properties plus separators.
    int                width;
    int                height;
    AR_PIXEL_FORMAT    format;
    int                camera_index; // 0 = first camera, 1 = second etc.
    int                camera_face; // 0 = camera is rear facing, 1 = camera is front facing.
    float              focal_length; // metres.
    int                cparamSearchInited;
    void             (*cparamSearchCallback)(const ARParam *, void *);
    void              *cparamSearchUserdata;
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
    VIDEO_ANDROID_NATIVE_CAPTURE *nativeCapture;
    AR2VideoBufferT    buffer;
    bool               capturing;
#endif
};

int ar2VideoDispOptionAndroid( void )
{
    ARLOG(" -device=Android\n");
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
    ARLOG(" -source=N\n");
    ARLOG("    Acquire video from connected source device with index N (default = 0).\n");
#endif
    ARLOG(" -width=N\n");
    ARLOG("    specifies desired width of image.\n");
    ARLOG(" -height=N\n");
    ARLOG("    specifies desired height of image.\n");
    ARLOG(" -cachedir=/path/to/cache\n");
    ARLOG("    Specifies the path in which to look for/store camera parameter cache files.\n");
    ARLOG("    Default is working directory.\n");
    ARLOG("\n");
    
    return 0;
}

AR2VideoParamAndroidT *ar2VideoOpenAndroid( const char *config )
{
    char                     *cacheDir = NULL;
    AR2VideoParamAndroidT    *vid;
    const char               *a;
    char                      line[1024];
    int err_i = 0;
    int i;
    int width = 0, height = 0;
    
    arMallocClear( vid, AR2VideoParamAndroidT, 1 );
    
    a = config;
    if( a != NULL) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;
            
            if (sscanf(a, "%s", line) == 0) break;
            if( strcmp( line, "-device=Android" ) == 0 ) {
            } else if( strncmp( line, "-width=", 7 ) == 0 ) {
                if( sscanf( &line[7], "%d", &width ) == 0 ) {
                    ARLOGe("Error: Configuration option '-width=' must be followed by width in integer pixels.\n");
                    err_i = 1;
                }
            } else if( strncmp( line, "-height=", 8 ) == 0 ) {
                if( sscanf( &line[8], "%d", &height ) == 0 ) {
                    ARLOGe("Error: Configuration option '-height=' must be followed by height in integer pixels.\n");
                    err_i = 1;
                }
            } else if( strncmp( line, "-format=", 8 ) == 0 ) {
                if (strcmp(line+8, "0") == 0) {
                    vid->format = 0;
                    ARLOGi("Requesting images in system default format.\n");
                } else if (strcmp(line+8, "RGBA") == 0) {
                    vid->format = AR_PIXEL_FORMAT_RGBA;
                    ARLOGi("Requesting images in RGBA format.\n");
                } else if (strcmp(line+8, "NV21") == 0) {
                    vid->format = AR_PIXEL_FORMAT_NV21;
                    ARLOGi("Requesting images in NV21 format.\n");
                } else if (strcmp(line+8, "420f") == 0 || strcmp(line+8, "NV12") == 0) {
                    vid->format = AR_PIXEL_FORMAT_420f;
                    ARLOGi("Requesting images in 420f/NV12 format.\n");
                } else {
                    ARLOGe("Ignoring request for unsupported video format '%s'.\n", line+8);
                }
            } else if (strncmp(a, "-cachedir=", 10) == 0) {
                // Attempt to read in pathname, allowing for quoting of whitespace.
                a += 10; // Skip "-cachedir=" characters.
                if (*a == '"') {
                    a++;
                    // Read all characters up to next '"'.
                    i = 0;
                    while (i < (sizeof(line) - 1) && *a != '\0') {
                        line[i] = *a;
                        a++;
                        if (line[i] == '"') break;
                        i++;
                    }
                    line[i] = '\0';
                } else {
                    sscanf(a, "%s", line);
                }
                if (!strlen(line)) {
                    ARLOGe("Error: Configuration option '-cachedir=' must be followed by path (optionally in double quotes).\n");
                    err_i = 1;
                } else {
                    free(cacheDir);
                    cacheDir = strdup(line);
                }
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
            } else if( strncmp( line, "-source=", 8 ) == 0 ) {
                if( sscanf( &line[8], "%d", &vid->camera_index ) == 0 ) err_i = 1;
#endif
            } else {
                err_i = 1;
            }
            
            if (err_i) {
				ARLOGe("Error: Unrecognised configuration option '%s'.\n", a);
                ar2VideoDispOptionAndroid();
                goto bail;
			}
            
            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }
    
    
    // Initial state.
    if (!vid->format) vid->format = AR_INPUT_ANDROID_PIXEL_FORMAT;
    if (!vid->focal_length) vid->focal_length = AR_VIDEO_ANDROID_FOCAL_LENGTH_DEFAULT;
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
    vid->capturing = false;
#endif
    
    // In lieu of identifying the actual camera, we use manufacturer/model/board to identify a device,
    // and assume that identical devices have identical cameras.
    // Handset ID, via <sys/system_properties.h>.
    int len;
    len = __system_property_get(ANDROID_OS_BUILD_MANUFACTURER, vid->device_id); // len = (int)strlen(device_id).
    vid->device_id[len] = '/';
    len++;
    len += __system_property_get(ANDROID_OS_BUILD_MODEL, vid->device_id + len);
    vid->device_id[len] = '/';
    len++;
    len += __system_property_get(ANDROID_OS_BUILD_BOARD, vid->device_id + len);
    
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
    // Open the camera connection. Until this is done, we can't set any properties of the stream.
    if (!(vid->nativeCapture = videoAndroidNativeCaptureOpen(vid->camera_index))) {
        ARLOGe("Unable to initialise native Android video capture.\n");
        goto bail;
    }
#endif

    // Set width and height if specified.
#if !AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
    if (width && height) {
        vid->width = width;
        vid->height = height;
    }
#else
    if (width && height) {
        if (!videoAndroidNativeCaptureSetSize(vid->nativeCapture, width, height)) {
            ARLOGe("Error: Unable to set native Android video frame size.\n");
        }
    }
    // Read back the actual width and height.
    if (!videoAndroidNativeCaptureGetSize(vid->nativeCapture, &vid->width, &vid->height)) {
        ARLOGe("Error: Unable to get native Android video frame size.\n");
        goto bail1;
    }
#endif
    
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
    // Add any required changes to other properties here.
    //videoAndroidNativeCaptureSetProperty(ANDROID_CAMERA_PROPERTY_FOCUS_MODE, ANDROID_CAMERA_FOCUS_MODE_AUTO);
    //if (!videoAndroidNativeCaptureApplyProperties(vid->nativeCapture)) {
    //    ARLOGe("Unable to apply changes to native Android video.\n");
    //}
    
    // Check format and prepare the vid->buffer structure for later use.
    if (vid->format == AR_PIXEL_FORMAT_NV21 || vid->format == AR_PIXEL_FORMAT_420f) {
        // For non-planar formats, incoming buffers will be served directly,
        // so no allocation for buff, but do need to allocate an array for thr bufPlanes pointers.
        vid->buffer.buff = NULL;
        vid->buffer.bufPlanes = (ARUint8 **)calloc(2, sizeof(ARUint8 *));
        vid->buffer.bufPlaneCount = 2;
    } else if (vid->format == AR_PIXEL_FORMAT_RGBA) {
        vid->buffer.buff = malloc(vid->width * vid->height * 4);
        vid->buffer.bufPlanes = NULL;
        vid->buffer.bufPlaneCount = 0; // This will be checked for in ar2VideoCloseAndroid when deciding whether to free(buff).
    } else {
        ARLOGe("Error opening Android video: Unsupported video format %s (%d).\n", arVideoUtilGetPixelFormatName(vid->format), vid->format);
        goto bail1;
    }
    
    ARLOGi("Opened native video %dx%d, %s.\n", vid->width, vid->height, arVideoUtilGetPixelFormatName(vid->format));
#endif

	// Initialisation required before cparamSearch can be used.
    if (cparamSearchInit(cacheDir, false) < 0) {
        ARLOGe("Unable to initialise cparamSearch.\n");
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
        goto bail1;
#else
        goto bail;
#endif
    };

    goto done;

#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
bail1:
    videoAndroidNativeCaptureClose(&vid->nativeCapture);
#endif
bail:
    free(vid);
    vid = NULL;
done:
    free(cacheDir);
    return (vid);
}

int ar2VideoCloseAndroid( AR2VideoParamAndroidT *vid )
{
    if (!vid) return (-1); // Sanity check.
    
#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
    // Free image stuff.
    if (vid->capturing) ar2VideoCapStopAndroid(vid);
    if (vid->buffer.bufPlaneCount == 0) free(vid->buffer.buff);
    else free(vid->buffer.bufPlanes);
    
    // Clean up native capture;
    if (!videoAndroidNativeCaptureClose(&vid->nativeCapture)) {
        ARLOGe("Error shutting down native Android video.\n");
    }
#endif
    
    if (cparamSearchFinal() < 0) {
        ARLOGe("Unable to finalise cparamSearch.\n");
    }
    
    free( vid );
    
    return 0;
} 

int ar2VideoGetIdAndroid( AR2VideoParamAndroidT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetSizeAndroid(AR2VideoParamAndroidT *vid, int *x,int *y)
{
    if (!vid) return -1;
    
    if (x) *x = vid->width;
    if (y) *y = vid->height;
    
    return 0;
}

#if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA

AR2VideoBufferT *ar2VideoGetImageAndroid( AR2VideoParamAndroidT *vid )
{
    if (!vid) return NULL;
    
    unsigned char *frame = videoAndroidNativeCaptureGetFrame(vid->nativeCapture);
    if (!frame) return NULL;
    
    if (vid->format == AR_PIXEL_FORMAT_NV21 || vid->format == AR_PIXEL_FORMAT_420f) {
        vid->buffer.bufPlanes[0] = vid->buffer.buff = (ARUint8 *)frame; // Luma plane.
        vid->buffer.bufPlanes[1] = (ARUint8 *)(frame + vid->width*vid->height); // Chroma plane.
    } else if (vid->format == AR_PIXEL_FORMAT_RGBA) {
        color_convert_common(frame, frame + vid->width*vid->height, vid->width, vid->height, vid->buffer.buff);
    } else {
        return NULL;
    }
    
    vid->buffer.fillFlag = 1;
    
    // Get time of capture.
    struct timeval time;
    gettimeofday(&time, NULL);
    vid->buffer.time_sec = time.tv_sec;
    vid->buffer.time_usec = time.tv_usec;
    
    return (&vid->buffer);
}

int ar2VideoCapStartAndroid(AR2VideoParamAndroidT *vid)
{
    return (ar2VideoCapStartAsyncAndroid(vid, NULL, NULL));
}

int ar2VideoCapStartAsyncAndroid(AR2VideoParamAndroidT *vid, AR_VIDEO_FRAME_READY_CALLBACK callback, void *userdata)
{
    if (!vid) return -1;
    if (vid->capturing) return -1; // Already capturing.
    
    if (!videoAndroidNativeCaptureStart(vid->nativeCapture, callback, userdata)) {
        ARLOGe("Error starting native frame capture.\n");
        return -1;
    }
    vid->capturing = true;
    
    return 0;
}

int ar2VideoCapStopAndroid(AR2VideoParamAndroidT *vid)
{
    int ret = 0;
    
    if (!vid) return -1;
    if (!vid->capturing) return -1; // Not capturing.
    
    if (!videoAndroidNativeCaptureStop(vid->nativeCapture)) {
        ARLOGe("Error stopping native frame capture.\n");
        ret = -1;
    }
    vid->capturing = false;
    
    return ret;
}

#endif // AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA

AR_PIXEL_FORMAT ar2VideoGetPixelFormatAndroid( AR2VideoParamAndroidT *vid )
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID;
    
    return vid->format;
}

int ar2VideoGetParamiAndroid( AR2VideoParamAndroidT *vid, int paramName, int *value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
#if !AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
        case AR_VIDEO_PARAM_ANDROID_WIDTH:          *value = vid->width; break;
        case AR_VIDEO_PARAM_ANDROID_HEIGHT:         *value = vid->height; break;
        case AR_VIDEO_PARAM_ANDROID_PIXELFORMAT:    *value = (int)vid->format; break;
#endif
        case AR_VIDEO_PARAM_ANDROID_CAMERA_INDEX:   *value = vid->camera_index; break;
        case AR_VIDEO_PARAM_ANDROID_CAMERA_FACE:    *value = vid->camera_face; break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamiAndroid( AR2VideoParamAndroidT *vid, int paramName, int  value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
#if !AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
        case AR_VIDEO_PARAM_ANDROID_WIDTH:          vid->width = value; break;
        case AR_VIDEO_PARAM_ANDROID_HEIGHT:         vid->height = value; break;
        case AR_VIDEO_PARAM_ANDROID_PIXELFORMAT:    vid->format = (AR_PIXEL_FORMAT)value; break;
#endif
        case AR_VIDEO_PARAM_ANDROID_CAMERA_INDEX:   vid->camera_index = value; break;
        case AR_VIDEO_PARAM_ANDROID_CAMERA_FACE:    vid->camera_face = value; break;
        case AR_VIDEO_PARAM_ANDROID_INTERNET_STATE: return (cparamSearchSetInternetState(value)); break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamdAndroid( AR2VideoParamAndroidT *vid, int paramName, double *value )
{
    if (!vid || !value) return (-1);

    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:   *value = (double)vid->focal_length; break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamdAndroid( AR2VideoParamAndroidT *vid, int paramName, double  value )
{
    if (!vid) return (-1);

    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_FOCAL_LENGTH:   vid->focal_length = (float)value; break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamsAndroid( AR2VideoParamAndroidT *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_DEVICEID:       *value = vid->device_id; break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsAndroid( AR2VideoParamAndroidT *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

static void cparamSeachCallback(CPARAM_SEARCH_STATE state, float progress, const ARParam *cparam, void *userdata)
{
    int final = false;
    AR2VideoParamAndroidT *vid = (AR2VideoParamAndroidT *)userdata;
    if (!vid) return;
    
    switch (state) {
        case CPARAM_SEARCH_STATE_INITIAL:
        case CPARAM_SEARCH_STATE_IN_PROGRESS:
            break;
        case CPARAM_SEARCH_STATE_RESULT_NULL:
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
        case CPARAM_SEARCH_STATE_OK:
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(cparam, vid->cparamSearchUserdata);
            final = true;
            break;
        case CPARAM_SEARCH_STATE_FAILED_NO_NETWORK:
            ARLOGe("Error during cparamSearch. Internet connection unavailable.\n");
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
        default: // Errors.
            ARLOGe("Error %d returned from cparamSearch.\n", (int)state);
            if (vid->cparamSearchCallback) (*vid->cparamSearchCallback)(NULL, vid->cparamSearchUserdata);
            final = true;
            break;
    }
    if (final) vid->cparamSearchCallback = vid->cparamSearchUserdata = NULL;
}

#if (__ANDROID_API__ >= 21) 
// Android 'L' makes __system_property_get a non-global symbol.
// Here we provide a stub which loads the symbol from libc via dlsym.
typedef int (*PFN_SYSTEM_PROP_GET)(const char *, char *);
int __system_property_get(const char* name, char* value)
{
    static PFN_SYSTEM_PROP_GET __real_system_property_get = NULL;
    if (!__real_system_property_get) {
        // libc.so should already be open, get a handle to it.
        void *handle = dlopen("libc.so", RTLD_NOLOAD);
        if (!handle) {
            ARLOGe("Cannot dlopen libc.so: %s.\n", dlerror());
        } else {
            __real_system_property_get = (PFN_SYSTEM_PROP_GET)dlsym(handle, "__system_property_get");
        }
        if (!__real_system_property_get) {
            ARLOGe("Cannot resolve __system_property_get(): %s.\n", dlerror());
        }
    }
    return (*__real_system_property_get)(name, value);
} 
#endif // __ANDROID_API__ >= 21

int ar2VideoGetCParamAsyncAndroid(AR2VideoParamAndroidT *vid, void (*callback)(const ARParam *, void *), void *userdata)
{
    if (!vid) return (-1);
    if (!callback) {
        ARLOGw("Warning: cparamSearch requested without callback.\n");
    }

    vid->cparamSearchCallback = callback;
    vid->cparamSearchUserdata = userdata;
    
    CPARAM_SEARCH_STATE initialState = cparamSearch(vid->device_id, vid->camera_index, vid->width, vid->height, vid->focal_length, &cparamSeachCallback, (void *)vid);
    if (initialState != CPARAM_SEARCH_STATE_INITIAL) {
        ARLOGe("Error %d returned from cparamSearch.\n", initialState);
        vid->cparamSearchCallback = vid->cparamSearchUserdata = NULL;
        return (-1);
    }
    
    return (0);
}
