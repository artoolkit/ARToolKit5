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

#include "android_os_build_codes.h"
#include "cparamSearch.h"

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
};

int ar2VideoDispOptionAndroid( void )
{
    ARLOG(" -device=Android\n");
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
    
    // Set width and height if specified.
    if (width && height) {
        vid->width = width;
        vid->height = height;
    }
    
    if (cparamSearchInit(cacheDir, false) < 0) {
        ARLOGe("Unable to initialise cparamSearch.\n");
        goto bail;
    };

    goto done;

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

AR_PIXEL_FORMAT ar2VideoGetPixelFormatAndroid( AR2VideoParamAndroidT *vid )
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID;
    
    return vid->format;
}

int ar2VideoGetParamiAndroid( AR2VideoParamAndroidT *vid, int paramName, int *value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_ANDROID_WIDTH:          *value = vid->width; break;
        case AR_VIDEO_PARAM_ANDROID_HEIGHT:         *value = vid->height; break;
        case AR_VIDEO_PARAM_ANDROID_PIXELFORMAT:    *value = (int)vid->format; break;
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
        case AR_VIDEO_PARAM_ANDROID_WIDTH:          vid->width = value; break;
        case AR_VIDEO_PARAM_ANDROID_HEIGHT:         vid->height = value; break;
        case AR_VIDEO_PARAM_ANDROID_PIXELFORMAT:    vid->format = (AR_PIXEL_FORMAT)value; break;
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
