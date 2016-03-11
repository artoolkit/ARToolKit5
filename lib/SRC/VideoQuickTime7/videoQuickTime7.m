/*
 *  videoQuickTime7.m
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
 *  Author(s): Philip Lamb
 *
 *	Rev		Date		Who		Changes
 *	1.0.0	2010-07-01	PRL		Written.
 *
 */

#include <AR/video.h>

#ifdef AR_INPUT_QUICKTIME7

#import "QTKitVideo.h"
#import "videoQuickTime7QTKitVideoGotFrameDelegate.h"

#include <string.h>

struct _AR2VideoParamQuickTime7T  {
    QTKitVideo        *qtKitVideo;
    CVImageBufferRef   currentFrame;
    UInt64             currentFrameTimestamp;
    AR2VideoBufferT    buffer;
    videoQuickTime7QTKitVideoGotFrameDelegate *qtKitVideoGotFrameDelegate;
    void (*gotImageFunc)(AR2VideoBufferT *, void *);
    void *gotImageFuncUserData;
};

int ar2VideoDispOptionQuickTime7( void )
{
    ARLOG(" -device=QuickTime7\n");
    ARLOG("\n");
    ARLOG(" -width=N\n");
    ARLOG("    Scale camera native image to width N.\n");
    ARLOG(" -height=N\n");
    ARLOG("    Scale camera native image to height N.\n");
	ARLOG(" -pixelformat=cccc\n");
    ARLOG("    Return images with pixels in format cccc, where cccc is either a\n");
    ARLOG("    numeric pixel format number or a valid 4-character-code for a\n");
    ARLOG("    pixel format.\n");
	ARLOG("    The following numeric values are supported: \n");
	ARLOG("    24 (24-bit RGB), 32 (32-bit ARGB), 40 (8-bit grey)\n");
	ARLOG("    The following 4-character-codes are supported: \n");
    ARLOG("    BGRA, RGBA, ABGR, 24BG, 2vuy, yuvs.\n");
    ARLOG("    (See http://developer.apple.com/library/mac/#technotes/tn2010/tn2273.html.)\n");
    ARLOG(" -source=N\n");
    ARLOG("    Acquire video from connected source device with index N (default = 0).\n");
    ARLOG(" -nomuxed");
    ARLOG("    Do not search for video from multiplexed video/audio devices (e.g. DV cams).\n");
    //ARLOG(" -bufferpow2\n");
    //ARLOG("    requests that images are returned in a buffer which has power-of-two dimensions.\n");
    ARLOG("\n");

    return 0;
}

AR2VideoParamQuickTime7T *ar2VideoOpenQuickTime7( const char *config )
{
    AR2VideoParamQuickTime7T      *vid;
    const char                    *a;
    char                           line[1024];
    int width = 0;
    int height = 0;
	int					showFPS = 0;
	int					showDialog = 1;
	int					singleBuffer = 0;
	int					flipH = 0, flipV = 0;
	int					err_i = 0;
#ifdef AR_BIG_ENDIAN
    OSType pixFormat = k32ARGBPixelFormat;
#else
    OSType pixFormat = k32BGRAPixelFormat;
#endif
    int source = 0;
    char *sourceuid = NULL;
    char bufferpow2 = 0;
    char noMuxed = 0;
    int i;

    arMalloc( vid, AR2VideoParamQuickTime7T, 1 );
    vid->currentFrame = NULL;
    vid->currentFrameTimestamp = 0;
    vid->buffer.buff = vid->buffer.buffLuma = NULL;
    vid->buffer.fillFlag = 0;

    a = config;
    if( a != NULL) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;
    
            if( sscanf(a, "%s", line) == 0 ) break;

            if( strncmp( line, "-width=", 7 ) == 0 ) {
                if( sscanf( &line[7], "%d", &width ) == 0 ) err_i = 1;
            } else if( strncmp( line, "-height=", 8 ) == 0 ) {
                if( sscanf( &line[8], "%d", &height ) == 0 ) err_i = 1;
            } else if (strncmp(a, "-format=", 8) == 0) {
                sscanf(a, "%s", line);
                if (strlen(line) <= 8) err_i = 1;
                else {
#ifdef AR_BIG_ENDIAN
                    if (strlen(line) == 12) err_i = (sscanf(&line[13], "%4c", (char *)&pixFormat) < 1);
#else
                    if (strlen(line) == 12) err_i = (sscanf(&line[13], "%c%c%c%c", &(((char *)&pixFormat)[3]), &(((char *)&pixFormat)[2]), &(((char *)&pixFormat)[1]), &(((char *)&pixFormat)[0])) < 4);
#endif
                    else err_i = (sscanf(&line[8], "%li", (long *)&pixFormat) < 1); // Integer.
                }
            } else if (strncmp(a, "-pixelformat=", 13) == 0) {
                sscanf(a, "%s", line);
                if (strlen(line) <= 13) err_i = 1;
                else {
#ifdef AR_BIG_ENDIAN
                    if (strlen(line) == 17) err_i = (sscanf(&line[13], "%4c", (char *)&pixFormat) < 1);
#else
                    if (strlen(line) == 17) err_i = (sscanf(&line[13], "%c%c%c%c", &(((char *)&pixFormat)[3]), &(((char *)&pixFormat)[2]), &(((char *)&pixFormat)[1]), &(((char *)&pixFormat)[0])) < 4);
#endif
                    else err_i = (sscanf(&line[13], "%li", (long *)&pixFormat) < 1); // Integer.
                }
            } else if( strncmp( line, "-source=", 8 ) == 0 ) {
                if( sscanf( &line[8], "%d", &source ) == 0 ) err_i = 1;
            } else if( strcmp( line, "-device=QuickTime7" ) == 0 )    {
            } else if (strncmp(a, "-dialog", 7) == 0) {
                showDialog = 1;
            } else if (strncmp(a, "-nodialog", 9) == 0) {
                showDialog = 0;
            } else if (strncmp(a, "-sourceuuid=", 11) == 0) {
                // Attempt to read in device unique ID, allowing for quoting of whitespace.
                a += 11; // Skip "-sourceuuid=" characters.
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
                if (!strlen(line)) err_i = 1;
                else sourceuid = strdup(line);
            // Allowed but currently ignored.
            } else if( strcmp( line, "-bufferpow2" ) == 0 )    {
                bufferpow2 = 1;
            } else if (strncmp(a, "-fps", 4) == 0) {
                showFPS = 1;
            } else if (strncmp(a, "-nofps", 6) == 0) {
                showFPS = 0;
            } else if (strncmp(a, "-fliph", 6) == 0) {
                flipH = 1;
            } else if (strncmp(a, "-nofliph", 8) == 0) {
                flipH = 0;
            } else if (strncmp(a, "-flipv", 6) == 0) {
                flipV = 1;
            } else if (strncmp(a, "-noflipv", 8) == 0) {
                flipV = 0;
            } else if (strncmp(a, "-singlebuffer", 13) == 0) {
                singleBuffer = 1;
            } else if (strncmp(a, "-nosinglebuffer", 15) == 0) {
                singleBuffer = 0;
            } else if (strncmp(a, "-nomuxed", 8) == 0) {
                noMuxed = 1;
            // Obsolete. Allowed and ignored.
            } else if (strncmp(a, "-grabber=", 9) == 0) {
            } else if (strncmp(a, "-standarddialog", 15) == 0) {
            } else if (strncmp(a, "-nostandarddialog", 17) == 0) {
            } else {
                ARLOGe("Error: unrecognised video configuration option \"%s\".\n", a);
                ar2VideoDispOptionQuickTime7();
                err_i = 1;
            }

            if (err_i) {
                if (sourceuid) free(sourceuid);
                free(vid);
                return (NULL);
            }

            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

    // Calling Objective-C from C necessitates an autorelease pool.
    @autoreleasepool {
    
        // Init the QTKitVideo object.
        vid->qtKitVideo = [[QTKitVideo alloc] init];
        if (!vid->qtKitVideo) {
            NSLog(@"Error: Unable to initialise camera.\n");
            if (sourceuid) free(sourceuid);
            free (vid);
            return (NULL);
        }
        
        if (source) vid->qtKitVideo.preferredDevice = source;
        if (sourceuid) {
            vid->qtKitVideo.preferredDeviceUID = [NSString stringWithCString:sourceuid encoding:NSUTF8StringEncoding];
            free(sourceuid);
        }
        vid->qtKitVideo.showDialogs = showDialog;
        if (noMuxed) vid->qtKitVideo.acceptMuxedVideo = FALSE;
        
        [vid->qtKitVideo startWithRequestedWidth:width height:height pixelFormat:pixFormat];
        
        if (!vid->qtKitVideo.running) {
            NSLog(@"Error: Unable to open connection to camera.\n");
            free (vid);
            return (NULL);
        }
        
        // Report video size and compression type.
        OSType formatType = vid->qtKitVideo.pixelFormat;
        if (formatType > 0x28) ARLOGi("Video formatType is %c%c%c%c, size is %ldx%ld.\n",
                                      (char)((formatType >> 24) & 0xFF),
                                      (char)((formatType >> 16) & 0xFF),
                                      (char)((formatType >>  8) & 0xFF),
                                      (char)((formatType >>  0) & 0xFF),
                                      vid->qtKitVideo.width, vid->qtKitVideo.height);
        else ARLOGi("Video formatType is %u, size is %ldx%ld.\n", (int)formatType, vid->qtKitVideo.width, vid->qtKitVideo.height);

       /*
        if (bufferpow2) {
            width = height = 1;
            while (width < vid->qtKitVideo.bufWidth) width *= 2;
            while (height < vid->qtKitVideo.bufHeight) height *= 2;
            if (ar2VideoSetBufferSizeQuickTime7(vid, width, height) != 0) {
                [vid->qtKitVideo release];
                free (vid);
                return (NULL);
            }
        }
        */
        
        vid->qtKitVideo.pause = TRUE;
        vid->qtKitVideoGotFrameDelegate = nil; // Init.

    }
    return (vid);
}

int ar2VideoCloseQuickTime7( AR2VideoParamQuickTime7T *vid )
{
    if (vid) {
        @autoreleasepool {
            if (vid->currentFrame) {
                CVPixelBufferUnlockBaseAddress(vid->currentFrame, kCVPixelBufferLock_ReadOnly);
                CVBufferRelease(vid->currentFrame);
            }
            if (vid->qtKitVideo) [vid->qtKitVideo release];
            if (vid->qtKitVideoGotFrameDelegate) [vid->qtKitVideoGotFrameDelegate release];
        }
        free(vid);
        return 0;
    }
    return (-1);    
} 

int ar2VideoCapStartQuickTime7( AR2VideoParamQuickTime7T *vid )
{
    if (vid) {
        if (vid->qtKitVideo) {
            @autoreleasepool {
                vid->qtKitVideo.pause = FALSE;
            }
            return 0;
        }
    }
    return (-1);
}

int ar2VideoCapStopQuickTime7( AR2VideoParamQuickTime7T *vid )
{
    if (vid) {
        if (vid->qtKitVideo) {
            @autoreleasepool {
                vid->qtKitVideo.pause = TRUE;
            }
            return 0; 
        }
    }
    return (-1);
}

AR2VideoBufferT *ar2VideoGetImageQuickTime7( AR2VideoParamQuickTime7T *vid )
{
    if (vid) {
        if (vid->qtKitVideo) {
            @autoreleasepool {
                UInt64 timestamp;
                CVImageBufferRef frame = [vid->qtKitVideo frameTimestamp:&timestamp ifNewerThanTimestamp:vid->currentFrameTimestamp];
                if (frame) {
                    if (vid->currentFrame) {
                        CVPixelBufferUnlockBaseAddress(vid->currentFrame, kCVPixelBufferLock_ReadOnly);
                        CVBufferRelease(vid->currentFrame);
                    }
                    
                    vid->currentFrame = frame;
                    vid->currentFrameTimestamp = timestamp;
                    CVPixelBufferLockBaseAddress(frame, kCVPixelBufferLock_ReadOnly);
                    
                    if (!CVPixelBufferGetPlaneCount(frame)) vid->buffer.buff = CVPixelBufferGetBaseAddress(frame);
                    else vid->buffer.buff = CVPixelBufferGetBaseAddressOfPlane(frame, 0);
                    
                    vid->buffer.fillFlag  = 1;
                    vid->buffer.buffLuma = NULL;
                    vid->buffer.time_sec  = 0;
                    vid->buffer.time_usec = 0;
                    
                    return &(vid->buffer);
                }
            }
        }        
    }
    return (NULL);
}

int ar2VideoGetSizeQuickTime7(AR2VideoParamQuickTime7T *vid, int *x,int *y)
{
    if (!vid || !x || !y) return (-1);
    if (!vid->qtKitVideo) return (-1);

    @autoreleasepool {
        *x = (int)((vid->qtKitVideo).width);
        if (!*x) {
#ifdef DEBUG
            NSLog(@"Unable to determine camera image width.\n");
#endif
            return (-1);
        }
        *y = (int)((vid->qtKitVideo).height);
        if (!*y) {
#ifdef DEBUG
            NSLog(@"Unable to determine camera image height.\n");
#endif
            return (-1);
        }
    
        return 0;
    }
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatQuickTime7( AR2VideoParamQuickTime7T *vid )
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
    if (!vid->qtKitVideo) return (AR_PIXEL_FORMAT_INVALID);
        
    @autoreleasepool {
        switch ((vid->qtKitVideo.pixelFormat)) {
            case kCVPixelFormatType_422YpCbCr8: // (previously k2vuyPixelFormat). Preferred YUV format on iPhone 3G.
                return (AR_PIXEL_FORMAT_2vuy);
                break;
            case kCVPixelFormatType_422YpCbCr8_yuvs: // (previously kyuvsPixelFormat)
                return (AR_PIXEL_FORMAT_yuvs);
                break; 
            case kCVPixelFormatType_24RGB: // (previously k24RGBPixelFormat)
                return (AR_PIXEL_FORMAT_RGB);
                break; 
            case kCVPixelFormatType_24BGR: // (previously k24BGRPixelFormat)
                return (AR_PIXEL_FORMAT_BGR);
                break; 
            case kCVPixelFormatType_32ARGB: // (previously k32ARGBPixelFormat)
                return (AR_PIXEL_FORMAT_ARGB);
                break; 
            case kCVPixelFormatType_32BGRA: // (previously k32BGRAPixelFormat)
                return (AR_PIXEL_FORMAT_BGRA);
                break; 
            case kCVPixelFormatType_32ABGR: // (previously k32ABGRPixelFormat)
                return (AR_PIXEL_FORMAT_ABGR);
                break; 
            case kCVPixelFormatType_32RGBA: // (previously k32RGBAPixelFormat)
                return (AR_PIXEL_FORMAT_RGBA);
                break; 
            case kCVPixelFormatType_8IndexedGray_WhiteIsZero: // (previously k8IndexedGrayPixelFormat)
                return (AR_PIXEL_FORMAT_MONO);
                break;
            case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
                return (AR_PIXEL_FORMAT_420v);
                break;
            case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
                return (AR_PIXEL_FORMAT_420f);
                break;
            default:
                return (AR_PIXEL_FORMAT_INVALID);
                break; 
        }
    }
}

int ar2VideoGetIdQuickTime7( AR2VideoParamQuickTime7T *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiQuickTime7( AR2VideoParamQuickTime7T *vid, int paramName, int *value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamiQuickTime7( AR2VideoParamQuickTime7T *vid, int paramName, int  value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamdQuickTime7( AR2VideoParamQuickTime7T *vid, int paramName, double *value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamdQuickTime7( AR2VideoParamQuickTime7T *vid, int paramName, double  value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamsQuickTime7( AR2VideoParamQuickTime7T *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsQuickTime7( AR2VideoParamQuickTime7T *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

id ar2VideoGetQTKitVideoInstanceQuickTime7(AR2VideoParamQuickTime7T *vid)
{
    if (!vid) return (nil);
    return (vid->qtKitVideo);
}

/*
 int ar2VideoSetBufferSizeQuickTime7(AR2VideoParamQuickTime7T *vid, const int width, const int height)
 {
 if (!vid) return (-1);
 if (!vid->qtKitVideo) return (-1);
 
 vid->qtKitVideo.bufWidth = width;
 vid->qtKitVideo.bufHeight = height;
 
 return (0);
 }
 
 int ar2VideoGetBufferSizeQuickTime7(AR2VideoParamQuickTime7T *vid, int *width, int *height)
 {
 if (!vid) return (-1);
 if (!vid->qtKitVideo) return (-1);
 
 if (width) *width = (int)(vid->qtKitVideo.bufWidth);
 if (height) *height = (int)(vid->qtKitVideo.bufHeight);
 
 return (0);
 }
 */

@implementation videoQuickTime7QTKitVideoGotFrameDelegate

- (void) QTKitVideoGotFrame:(id)sender userData:(void *)data
{
    AR2VideoParamQuickTime7T *vid = (AR2VideoParamQuickTime7T *)data; // Cast to correct type;
    if (vid) {
        if (vid->gotImageFunc) (*vid->gotImageFunc)(ar2VideoGetImageQuickTime7(vid), vid->gotImageFuncUserData);
    }
}

@end

void ar2VideoSetGotImageFunctionQuickTime7(AR2VideoParamQuickTime7T *vid, void (*gotImageFunc)(AR2VideoBufferT *, void *), void *userData)
{
    if (vid) {
        if (vid->qtKitVideo) {
            if (gotImageFunc != vid->gotImageFunc || userData != vid->gotImageFuncUserData) {
                @autoreleasepool {
                    vid->gotImageFunc = gotImageFunc;
                    vid->gotImageFuncUserData = userData;
                    if (gotImageFunc) {
                        // Setting or changing; the videoQuickTime7QTKitVideoGotFrameDelegate class implements the appropriate delegate.
                        if (!vid->qtKitVideoGotFrameDelegate) vid->qtKitVideoGotFrameDelegate = [[videoQuickTime7QTKitVideoGotFrameDelegate alloc] init]; // Create an instance of delegate.
                        [vid->qtKitVideo setGotFrameDelegate:vid->qtKitVideoGotFrameDelegate];
                        [vid->qtKitVideo setGotFrameDelegateUserData:vid];
                    } else {
                        // Cancellation message; unset delegate.
                        [vid->qtKitVideo setGotFrameDelegate:nil];
                        [vid->qtKitVideo setGotFrameDelegateUserData:NULL];
                        [vid->qtKitVideoGotFrameDelegate release]; // Destroy instance of delegate.
                        vid->qtKitVideoGotFrameDelegate = nil;
                    }
                }
            }
        }
    }    
}

void (*ar2VideoGetGotImageFunctionQuickTime7(AR2VideoParamQuickTime7T *vid))(AR2VideoBufferT *, void *)
{
    if (vid) return (vid->gotImageFunc);
    else return (NULL);
}

#endif //  AR_INPUT_QUICKTIME7
