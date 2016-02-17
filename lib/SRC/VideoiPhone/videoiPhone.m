/*
 *  videoiPhone.m
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
 *  Copyright 2008-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *	
 *	Rev		Date		Who		Changes
 *	1.0.0	2008-05-04	PRL		Written.
 *
 */

#include <AR/video.h>    // arParamLoadFromBuffer().

#ifdef AR_INPUT_IPHONE

#import <Foundation/Foundation.h>
#import <AR/sys/CameraVideo.h>
#import <AR/sys/MovieVideo.h>
#import "videoiPhoneCameraVideoTookPictureDelegate.h"
#import "cparams.h"

#include <string.h>

struct _AR2VideoParamiPhoneT  {
    CameraVideo       *cameraVideo;
    MovieVideo        *movieVideo;
    AR2VideoBufferT    buffer;
    videoiPhoneCameraVideoTookPictureDelegate *cameraVideoTookPictureDelegate;
    void (*gotImageFunc)(AR2VideoBufferT *, void *);
    void *gotImageFuncUserData;
    AR_VIDEO_IOS_FOCUS focus;
    float              focusPointOfInterestX;
    float              focusPointOfInterestY;
    BOOL               itsAMovie;
    UInt64             currentFrameTimestamp;
    UInt64             hostClockFrequency;
};

int getFrameParameters(AR2VideoParamiPhoneT *vid);

int ar2VideoDispOptioniPhone( void )
{
    ARLOG(" -device=iPhone\n");
    ARLOG("\n");
    ARLOG(" -preset=(cif|480p|vga|720p|1080p|low|medium|high)");
    ARLOG("     specify camera settings preset to use. cif=352x288, vga/480p=640x480,\n");
    ARLOG("     720p=1280x720, 1080p=1920x1080. cif and 1080p require iOS 5 or newer.\n");
    ARLOG("     default value is 'medium'.\n");
    ARLOG(" -position=(rear|back|front)\n");
    ARLOG("    choose between rear/back and front-mounted camera (where available).\n");
    ARLOG("    default value is 'rear'.\n");
    ARLOG(" -format=(BGRA|420v|420f|2vuy|yuvs|RGBA)\n");
    ARLOG("    choose format of pixels returned by arVideoGetImage().\n");
    ARLOG("    default value is '420f'.\n");
    ARLOG(" -width=N\n");
    ARLOG("    specifies expected width of image. N.B. IGNORED IN THIS RELEASE.\n");
    ARLOG(" -height=N\n");
    ARLOG("    specifies expected height of image. N.B. IGNORED IN THIS RELEASE.\n");
    ARLOG(" -bufferpow2\n");
    ARLOG("    requests that images are returned in a buffer which has power-of-two dimensions. N.B. IGNORED IN THIS RELEASE.\n");
    ARLOG(" -[no]flipv\n");
    ARLOG("    Flip camera image vertically.\n");
    ARLOG(" -[no]mt\n");
    ARLOG("    \"Multithreaded\", i.e. allow new frame callbacks on non-main thread.\n");
    //ARLOG(" -[no]fliph\n");
    //ARLOG("    Flip camera image horizontally.\n");
    ARLOG("\n");

    return 0;
}

AR2VideoParamiPhoneT *ar2VideoOpeniPhone( const char *config )
{
    return (ar2VideoOpenAsynciPhone(config, NULL, NULL));
}

AR2VideoParamiPhoneT *ar2VideoOpenAsynciPhone(const char *config, void (*callback)(void *), void *userdata)
{
	int					err_i = 0;
    AR2VideoParamiPhoneT *vid;
    const char         *a;
    char                b[1024];
    int                 itsAMovie = 0;
    char				movieConf[256] = "-pause -loop";
	int					i;
    int                 width = 0;
    int                 height = 0;
    uint32_t            format = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
	int                 flipH = -1, flipV = -1;
    char                bufferpow2 = 0;
    AVCaptureDevicePosition position = AVCaptureDevicePositionBack;
    NSString            *preset = AVCaptureSessionPresetMedium;
    int                 mt = 0;

    if (config) {
        a = config;
        err_i = 0;
        for (;;) {
            
            if (itsAMovie) {
                NSURL *url;
                NSString *pathnameOrURL = [NSString stringWithCString:b encoding:NSUTF8StringEncoding];
                //url = [NSURL URLWithString:pathnameOrURL];
                url = [[NSBundle mainBundle] URLForResource:pathnameOrURL withExtension:nil];
                if (!url) {
                    NSLog(@"Unable to locate requested movie resource %@.\n", pathnameOrURL);
                    return (NULL);
                }

                strncat(movieConf, a, sizeof(movieConf) - strlen(movieConf) - 1);
                
                MovieVideo *movieVideo = [[MovieVideo alloc] initWithURL:url config:movieConf];
                [movieVideo start];
                
                // Allocate the parameters structure and fill it in.
				arMallocClear(vid, AR2VideoParamiPhoneT, 1);
				vid->itsAMovie = TRUE;
				vid->movieVideo = movieVideo;
				return (vid);

            } else {
                while( *a == ' ' || *a == '\t' ) a++; // Skip whitespace.
                if( *a == '\0' ) break;
                
                if( sscanf(a, "%s", b) == 0 ) break;
                
                if (strncmp(a, "-movie=", 7) == 0) {
                    // Attempt to read in movie pathname or URL, allowing for quoting of whitespace.
                    a += 7; // Skip "-movie=" characters.
                    if (*a == '"') {
                        a++;
                        // Read all characters up to next '"'.
                        i = 0;
                        while (i < (sizeof(b) - 1) && *a != '\0') {
                            b[i] = *a;
                            a++;
                            if (b[i] == '"') break;
                            i++;
                        }
                        b[i] = '\0';
                    } else {
                        sscanf(a, "%s", b);
                    }
                    if (!strlen(b)) err_i = 1;
                    else itsAMovie = 1;
                } else if( strncmp( b, "-width=", 7 ) == 0 ) {
                    if( sscanf( &b[7], "%d", &width ) == 0 ) {
                        ar2VideoDispOptioniPhone();
                        return (NULL);
                    }   
                }
                else if( strncmp( b, "-height=", 8 ) == 0 ) {
                    if( sscanf( &b[8], "%d", &height ) == 0 ) {
                        ar2VideoDispOptioniPhone();
                        return (NULL);
                    }   
                }
                else if( strncmp( b, "-format=", 8 ) == 0 ) {
                    if (strcmp(b+8, "0") == 0) {
                        format = 0;
                        ARLOGi("Requesting images in system default format.\n");
                    } else if (strcmp(b+8, "BGRA") == 0) {
                        format = kCVPixelFormatType_32BGRA;
                        ARLOGi("Requesting images in BGRA format.\n");
                    } else if (strcmp(b+8, "420v") == 0) {
                        format = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
                        ARLOGi("Requesting images in 420v format.\n");
                    } else if (strcmp(b+8, "420f") == 0) {
                        format = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
                        ARLOGi("Requesting images in 420f format.\n");
                    } else if (strcmp(b+8, "2vuy") == 0) {
                        format = kCVPixelFormatType_422YpCbCr8;
                        ARLOGi("Requesting images in 2vuy format.\n");
                    } else if (strcmp(b+8, "yuvs") == 0) {
                        format = kCVPixelFormatType_422YpCbCr8_yuvs;
                        ARLOGi("Requesting images in yuvs format.\n");
                    } else if (strcmp(b+8, "RGBA") == 0) {
                        format = kCVPixelFormatType_32RGBA;
                        ARLOGi("Requesting images in RGBA format.\n");
                    } else {
                        NSLog(@"Ignoring request for unsupported video format '%s'.\n", b+8);
                    }
                }
                else if( strncmp( b, "-preset=", 8 ) == 0 ) {
                    if (strcmp(b+8, "1080p") == 0) {
                        preset = AVCaptureSessionPreset1920x1080;
                    } else if (strcmp(b+8, "720p") == 0) {
                        preset = AVCaptureSessionPreset1280x720;
                    } else if (strcmp(b+8, "480p") == 0 || strcmp(b+8, "vga") == 0) {
                        preset = AVCaptureSessionPreset640x480;
                    } else if (strcmp(b+8, "cif") == 0) {
                        preset = AVCaptureSessionPreset352x288;
                    } else if (strcmp(b+8, "high") == 0) {
                        preset = AVCaptureSessionPresetHigh;
                    } else if (strcmp(b+8, "medium") == 0) {
                        preset = AVCaptureSessionPresetMedium;
                    } else if (strcmp(b+8, "low") == 0) {
                        preset = AVCaptureSessionPresetLow;
                    } else if (strcmp(b+8, "photo") == 0) {
                        preset = AVCaptureSessionPresetPhoto;
                    } else {
                        NSLog(@"Error: unsupported video preset requested. Using default.\n");
                    }
                }
                else if( strncmp( b, "-position=", 10 ) == 0 ) {
                    if (strcmp(b+10, "rear") == 0 || strcmp(b+10, "back") == 0) {
                        position = AVCaptureDevicePositionBack;
                    } else if (strcmp(b+10, "front") == 0) {
                        position = AVCaptureDevicePositionFront;
                    } else {
                        NSLog(@"Error: unsupported video device position requested. Using default.\n");
                    }
                } else if (strcmp(b, "-flipv") == 0) {
                    flipV = 1;
                } else if (strcmp(b, "-noflipv") == 0) {
                    flipV = 0;
                } else if (strcmp(b, "-fliph") == 0) {
                    flipH = 1;
                } else if (strcmp(b, "-nofliph") == 0) {
                    flipH = 0;
                } else if (strcmp(b, "-mt") == 0) {
                    mt = 1;
                } else if (strcmp(b, "-nomt") == 0) {
                    mt = 0;
                } else if (strcmp(b, "-bufferpow2") == 0) {
                    bufferpow2 = 1;
                } else if (strcmp(b, "-device=iPhone") == 0) { // Ignored.
                } else {
                    err_i = 1;
                }
                
                if (err_i) {
					ARLOGe("Error: unrecognised video configuration option \"%s\".\n", a);
                    ar2VideoDispOptioniPhone();
                    return (NULL);
                }
                
                while( *a != ' ' && *a != '\t' && *a != '\0') a++;
            }
        }
    }

    arMallocClear(vid, AR2VideoParamiPhoneT, 1);
    vid->focusPointOfInterestX = vid->focusPointOfInterestY = -1.0f;
     
    // Init the CameraVideo object.
    vid->cameraVideo = [[CameraVideo alloc] init];
    if (!vid->cameraVideo) {
        NSLog(@"Error: Unable to open connection to iPhone camera.\n");
        free (vid);
        return (NULL);
    }
    vid->cameraVideoTookPictureDelegate = nil; // Init.
    vid->cameraVideo.pause = TRUE; // Start paused. ar2VideoCapStart will unpause.
    [vid->cameraVideo setCaptureDevicePosition:position];
    if (flipV == 1) vid->cameraVideo.flipV = TRUE;
    if (flipH == 1) vid->cameraVideo.flipH = TRUE;
    [vid->cameraVideo setCaptureSessionPreset:preset];
    if (format) [vid->cameraVideo setPixelFormat:format];
    
    if (!callback) {
        
        [vid->cameraVideo start];
        
    } else {
        
        [vid->cameraVideo startAsync:^() {
            
            // This block only gets called if vid->cameraVideo.running == TRUE.
            if (!vid->cameraVideo.width || !vid->cameraVideo.height || !vid->cameraVideo.bytesPerRow || getFrameParameters(vid) < 0) {
                NSLog(@"Error: Unable to open connection to camera.\n");  // Callback must check for error state and call arVideoClose() in this case.
            }
            
            (*callback)(userdata);
        }];
        
    }
    
    if (!vid->cameraVideo.running) {
        NSLog(@"Error: Unable to open connection to camera.\n");
        [vid->cameraVideo release];
        vid->cameraVideo = nil;
        free (vid);
        return (NULL);
    }

    // If doing synchronous opening, check parameters right now.
    if (!callback) {
        if (!vid->cameraVideo.width || !vid->cameraVideo.height || !vid->cameraVideo.bytesPerRow || getFrameParameters(vid) < 0) {
            NSLog(@"Error: Unable to open connection to camera.\n");
            [vid->cameraVideo release];
            vid->cameraVideo = nil;
            free (vid);
            return (NULL);
        }
        
    }
    
    return (vid);
}

int getFrameParameters(AR2VideoParamiPhoneT *vid)
{
#ifdef DEBUG
    // Report video size and compression type.
    OSType formatType = vid->cameraVideo.pixelFormat;
    ARLOGd("Video formatType is ");
    if (formatType > 0x28) ARLOGd("%c%c%c%c", (char)((formatType >> 24) & 0xFF),
                                  (char)((formatType >> 16) & 0xFF),
                                  (char)((formatType >>  8) & 0xFF),
                                  (char)((formatType >>  0) & 0xFF));
    else ARLOGd("%u", (int)formatType);
    ARLOGd(", size is %ldx%ld.\n", vid->cameraVideo.width, vid->cameraVideo.height);
#endif
    
    // Allocate structures for multi-planar frames.
    vid->buffer.bufPlaneCount = (unsigned int)vid->cameraVideo.planeCount;
    if (vid->buffer.bufPlaneCount) {
        vid->buffer.bufPlanes = (ARUint8 **)calloc(sizeof(ARUint8 *), vid->buffer.bufPlaneCount);
        if (!vid->buffer.bufPlanes) {
            ARLOGe("Out of memory!\n");
            return (-1);
        }
    } else vid->buffer.bufPlanes = NULL;
    
    vid->hostClockFrequency = vid->cameraVideo.timestampsPerSecond;
    
    return (0);
}

int ar2VideoCloseiPhone( AR2VideoParamiPhoneT *vid )
{
    if (vid) {
        if (vid->buffer.bufPlanes) free(vid->buffer.bufPlanes);
        if (vid->itsAMovie && vid->movieVideo) {
            [vid->movieVideo stop];
            [vid->movieVideo release];
        } else if (vid->cameraVideo) {
            [vid->cameraVideo stop];
            [vid->cameraVideo release];
        }
        if (vid->cameraVideoTookPictureDelegate) [vid->cameraVideoTookPictureDelegate release];
        free( vid );
        return 0;
    }
    return (-1);    
} 

int ar2VideoCapStartiPhone( AR2VideoParamiPhoneT *vid )
{
    if (vid) {
        if (vid->itsAMovie && vid->movieVideo) {
            [vid->movieVideo setPaused:FALSE];
            return 0;
        } else if (vid->cameraVideo){
            vid->cameraVideo.pause = FALSE;
            return 0;
        }
    }
    return (-1);
}

int ar2VideoCapStopiPhone( AR2VideoParamiPhoneT *vid )
{
    if (vid) {
        if (vid->itsAMovie && vid->movieVideo) {
            [vid->movieVideo setPaused:TRUE];
            return 0;
        } else if (vid->cameraVideo) {
            vid->cameraVideo.pause = TRUE;
            return 0; 
        }
    }
    return (-1);
}

AR2VideoBufferT *ar2VideoGetImageiPhone( AR2VideoParamiPhoneT *vid )
{
    if (!vid) return (NULL);
    
    if (vid->cameraVideo) {
        
        UInt64 timestamp;
        if (!vid->buffer.bufPlaneCount) {
            unsigned char *bufDataPtr = [vid->cameraVideo frameTimestamp:&timestamp ifNewerThanTimestamp:vid->currentFrameTimestamp];
            if (!bufDataPtr) return (NULL);
            vid->buffer.buff = bufDataPtr;
        } else {
            BOOL ret = [vid->cameraVideo framePlanes:vid->buffer.bufPlanes count:vid->buffer.bufPlaneCount timestamp:&timestamp ifNewerThanTimestamp:vid->currentFrameTimestamp];
            if (!ret) return (NULL);
            vid->buffer.buff = vid->buffer.bufPlanes[0];
        }
        vid->currentFrameTimestamp = timestamp;
        vid->buffer.time_sec  = (ARUint32)(timestamp / vid->hostClockFrequency);
        vid->buffer.time_usec = (ARUint32)((timestamp % vid->hostClockFrequency) / (vid->hostClockFrequency / 1000000ull));
        
    } else if (vid->itsAMovie && vid->movieVideo) {
        
        unsigned char *bufDataPtr = (vid->movieVideo).bufDataPtr;
        if (!bufDataPtr) return (NULL);
        vid->buffer.buff = bufDataPtr;
        vid->buffer.time_sec  = 0;
        vid->buffer.time_usec = 0;
       
    } else return (NULL);

    vid->buffer.fillFlag  = 1;
    
    return &(vid->buffer);
}

int ar2VideoGetSizeiPhone(AR2VideoParamiPhoneT *vid, int *x,int *y)
{
    if (!vid || !x || !y) return (-1);
    
    if (vid->itsAMovie) {
        *x = (int)((vid->movieVideo).contentWidth);
        *y = (int)((vid->movieVideo).contentHeight);
    } else {
        *x = (int)((vid->cameraVideo).width);
        *y = (int)((vid->cameraVideo).height);
    }
    if (!*x) {
#ifdef DEBUG
        NSLog(@"Unable to determine video image width.\n");
#endif
        return (-1);
    }
    if (!*y) {
#ifdef DEBUG
        NSLog(@"Unable to determine video image height.\n");
#endif
        return (-1);
    }
    
    return 0;
}

int ar2VideoSetBufferSizeiPhone(AR2VideoParamiPhoneT *vid, const int width, const int height)
{
    if (!vid) return (-1);
    //
    return (0);
}

int ar2VideoGetBufferSizeiPhone(AR2VideoParamiPhoneT *vid, int *width, int *height)
{
    if (!vid) return (-1);
    if (vid->itsAMovie) {
        if (width) *width = (int)((vid->movieVideo).bufWidth);
        if (height) *height = (int)((vid->movieVideo).bufHeight);
    } else {
        if (width) *width = (int)((vid->cameraVideo).width);
        if (height) *height = (int)((vid->cameraVideo).height);
    }
    return (0);
}

int ar2VideoGetPixelFormatiPhone( AR2VideoParamiPhoneT *vid )
{
    if (!vid) return (-1);
    if (vid->itsAMovie && vid->movieVideo) {
        return (vid->movieVideo.ARPixelFormat);
    } else if (vid->cameraVideo) {
        switch ((vid->cameraVideo.pixelFormat)) {
            case kCVPixelFormatType_32BGRA:
                return (AR_PIXEL_FORMAT_BGRA);
                break; 
            case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange: // All devices except iPhone 3G recommended.
                return (AR_PIXEL_FORMAT_420v);
                break;
            case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
                return (AR_PIXEL_FORMAT_420f);
                break;
            case kCVPixelFormatType_422YpCbCr8: // iPhone 3G recommended.
                return (AR_PIXEL_FORMAT_2vuy);
                break;
            case kCVPixelFormatType_422YpCbCr8_yuvs:
                return (AR_PIXEL_FORMAT_yuvs);
                break; 
            case kCVPixelFormatType_24RGB:
                return (AR_PIXEL_FORMAT_RGB);
                break; 
            case kCVPixelFormatType_24BGR:
                return (AR_PIXEL_FORMAT_BGR);
                break; 
            case kCVPixelFormatType_32ARGB:
                return (AR_PIXEL_FORMAT_ARGB);
                break; 
            case kCVPixelFormatType_32ABGR:
                return (AR_PIXEL_FORMAT_ABGR);
                break; 
            case kCVPixelFormatType_32RGBA:
                return (AR_PIXEL_FORMAT_RGBA);
                break; 
            case kCVPixelFormatType_8IndexedGray_WhiteIsZero:
                return (AR_PIXEL_FORMAT_MONO);
                break;
            default:
                return (AR_PIXEL_FORMAT)-1;
                break; 
        }
    }
    return (AR_PIXEL_FORMAT)-1;
}

int ar2VideoGetIdiPhone( AR2VideoParamiPhoneT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    return -1;
}

int ar2VideoGetParamiiPhone( AR2VideoParamiPhoneT *vid, int paramName, int *value )
{
    NSString *iOSDevice;
    AVCaptureDevicePosition position;
    
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_IPHONE_IS_USING_COREVIDEO:
            *value = TRUE; // Always true for iOS v4.0 and later.
        case AR_VIDEO_PARAM_IOS_ASYNC:
            if (vid->cameraVideo) *value = TRUE;
            else *value = FALSE; 
            break;
        case AR_VIDEO_PARAM_IPHONE_WILL_CAPTURE_NEXT_FRAME:
            if (vid->cameraVideo) *value = vid->cameraVideo.willSaveNextFrame;
            else return (-1);
            break;
        case AR_VIDEO_PARAM_IOS_DEVICE:
            if (vid->cameraVideo) {
                iOSDevice = vid->cameraVideo.iOSDevice;
                if      (iOSDevice == CameraVideoiOSDeviceiPhone3G)   *value = AR_VIDEO_IOS_DEVICE_IPHONE3G;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone3GS)  *value = AR_VIDEO_IOS_DEVICE_IPHONE3GS;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone4)    *value = AR_VIDEO_IOS_DEVICE_IPHONE4;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone4S)   *value = AR_VIDEO_IOS_DEVICE_IPHONE4S;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone5)    *value = AR_VIDEO_IOS_DEVICE_IPHONE5;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone5c)   *value = AR_VIDEO_IOS_DEVICE_IPHONE5C;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone5s)   *value = AR_VIDEO_IOS_DEVICE_IPHONE5S;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone6)    *value = AR_VIDEO_IOS_DEVICE_IPHONE6;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone6Plus) *value = AR_VIDEO_IOS_DEVICE_IPHONE6PLUS;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone6S)    *value = AR_VIDEO_IOS_DEVICE_IPHONE6S;
                else if (iOSDevice == CameraVideoiOSDeviceiPhone6SPlus) *value = AR_VIDEO_IOS_DEVICE_IPHONE6SPLUS;
                else if (iOSDevice == CameraVideoiOSDeviceiPodTouch4) *value = AR_VIDEO_IOS_DEVICE_IPODTOUCH4;
                else if (iOSDevice == CameraVideoiOSDeviceiPodTouch5) *value = AR_VIDEO_IOS_DEVICE_IPODTOUCH5;
                else if (iOSDevice == CameraVideoiOSDeviceiPad2)      *value = AR_VIDEO_IOS_DEVICE_IPAD2;
                else if (iOSDevice == CameraVideoiOSDeviceiPad3)      *value = AR_VIDEO_IOS_DEVICE_IPAD3;
                else if (iOSDevice == CameraVideoiOSDeviceiPad4)      *value = AR_VIDEO_IOS_DEVICE_IPAD4;
                else if (iOSDevice == CameraVideoiOSDeviceiPadAir)    *value = AR_VIDEO_IOS_DEVICE_IPADAIR;
                else if (iOSDevice == CameraVideoiOSDeviceiPadAir2)   *value = AR_VIDEO_IOS_DEVICE_IPADAIR2;
                else if (iOSDevice == CameraVideoiOSDeviceiPadMini)   *value = AR_VIDEO_IOS_DEVICE_IPADMINI;
                else if (iOSDevice == CameraVideoiOSDeviceiPadMini2)  *value = AR_VIDEO_IOS_DEVICE_IPADMINI2;
                else if (iOSDevice == CameraVideoiOSDeviceiPadMini3)  *value = AR_VIDEO_IOS_DEVICE_IPADMINI3;
                else if (iOSDevice == CameraVideoiOSDeviceiPadMini4)  *value = AR_VIDEO_IOS_DEVICE_IPADMINI4;
                else if (iOSDevice == CameraVideoiOSDeviceiPhoneX)    *value = AR_VIDEO_IOS_DEVICE_IPHONE_GENERIC;
                else if (iOSDevice == CameraVideoiOSDeviceiPodX)      *value = AR_VIDEO_IOS_DEVICE_IPOD_GENERIC;
                else if (iOSDevice == CameraVideoiOSDeviceiPadX)      *value = AR_VIDEO_IOS_DEVICE_IPAD_GENERIC;
                else if (iOSDevice == CameraVideoiOSDeviceAppleTVX)   *value = AR_VIDEO_IOS_DEVICE_APPLETV_GENERIC;
                else *value = -1;
            } else return (-1);
            break;
        case AR_VIDEO_PARAM_IOS_FOCUS:
            *value = vid->focus;
            break;
        case AR_VIDEO_PARAM_IOS_CAMERA_POSITION:
            if (vid->cameraVideo) {
                position = vid->cameraVideo.captureDevicePosition;
                if      (position == AVCaptureDevicePositionUnspecified) *value = AR_VIDEO_IOS_CAMERA_POSITION_UNSPECIFIED;
                else if (position == AVCaptureDevicePositionBack)        *value = AR_VIDEO_IOS_CAMERA_POSITION_REAR;
                else if (position == AVCaptureDevicePositionFront)       *value = AR_VIDEO_IOS_CAMERA_POSITION_FRONT;
                else *value = -1;
            } else return (-1);
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamiiPhone( AR2VideoParamiPhoneT *vid, int paramName, int  value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_IPHONE_WILL_CAPTURE_NEXT_FRAME:
            vid->cameraVideo.willSaveNextFrame = (BOOL)value;
#ifdef DEBUG
            NSLog(@"willSaveNextFrame.\n");
#endif
            break;
        case AR_VIDEO_PARAM_IOS_FOCUS:
            if (value < 0) return (-1);
            vid->focus = value;
            break;
        case AR_VIDEO_FOCUS_MODE:
            if (value == AR_VIDEO_FOCUS_MODE_FIXED) {
                if (![vid->cameraVideo setFocus:AVCaptureFocusModeLocked atPixelCoords:CGPointMake(0.0f, 0.0f)]) {
                    return (-1);
                };
            } else if (value == AR_VIDEO_FOCUS_MODE_AUTO) {
                if (![vid->cameraVideo setFocus:AVCaptureFocusModeContinuousAutoFocus atPixelCoords:CGPointMake(0.0f, 0.0f)]) {
                    return (-1);
                };
            } else if (value == AR_VIDEO_FOCUS_MODE_POINT_OF_INTEREST) {
                if (vid->focusPointOfInterestX < 0.0f || vid->focusPointOfInterestY < 0.0f) {
                    ARLOGw("Warning: request for focus on point-of-interest, but point of interest not yet set.\n");
                } else {
                    if (![vid->cameraVideo setFocus:AVCaptureFocusModeAutoFocus atPixelCoords:CGPointMake(vid->focusPointOfInterestX, vid->focusPointOfInterestY)]) {
                        return (-1);
                    };
                }
            } else if (value == AR_VIDEO_FOCUS_MODE_MANUAL) {
                ARLOGe("Error: request for manual focus but this mode not currently supported on iOS.\n");
                return (-1);
            } else {
                ARLOGe("Error: request for focus mode %d but this mode not currently supported on iOS.\n", value);
                return (-1);
            }
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamdiPhone( AR2VideoParamiPhoneT *vid, int paramName, double *value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamdiPhone( AR2VideoParamiPhoneT *vid, int paramName, double  value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_FOCUS_MANUAL_DISTANCE:
            ARLOGe("Error: request for manual focus but this mode not currently supported on iOS.\n");
            return (-1);
            break;
        case AR_VIDEO_FOCUS_POINT_OF_INTEREST_X:
            vid->focusPointOfInterestX = (float)value;
            break;
        case AR_VIDEO_FOCUS_POINT_OF_INTEREST_Y:
            vid->focusPointOfInterestY = (float)value;
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoGetParamsiPhone( AR2VideoParamiPhoneT *vid, const int paramName, char **value )
{
   if (!vid || !value) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_IOS_RECOMMENDED_CPARAM_NAME:
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsiPhone( AR2VideoParamiPhoneT *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

id ar2VideoGetNativeVideoInstanceiPhone(AR2VideoParamiPhoneT *vid)
{
    if (!vid) return (nil);
    if (vid->itsAMovie) return (vid->movieVideo);
    else return (vid->cameraVideo);
}

// A class that implements the CameraVideoTookPictureDelegate protocol.
@implementation videoiPhoneCameraVideoTookPictureDelegate

- (void) cameraVideoTookPicture:(id)sender userData:(void *)data
{
    AR2VideoParamiPhoneT *vid = (AR2VideoParamiPhoneT *)data; // Cast to correct type;
    if (vid) {
        if (vid->gotImageFunc) (vid->gotImageFunc)(ar2VideoGetImageiPhone(vid), vid->gotImageFuncUserData);
    }
}

@end

void ar2VideoSetGotImageFunctioniPhone(AR2VideoParamiPhoneT *vid, void (*gotImageFunc)(AR2VideoBufferT *, void *), void *userData)
{
    if (vid) {
        if (vid->cameraVideo) {
            if (gotImageFunc != vid->gotImageFunc || userData != vid->gotImageFuncUserData) {
                vid->gotImageFunc = gotImageFunc;
                vid->gotImageFuncUserData = userData;
                if (gotImageFunc) {
                    // Setting or changing; the videoiPhoneCameraVideoTookPictureDelegate class implements the appropriate delegate method.
                    if (!vid->cameraVideoTookPictureDelegate) vid->cameraVideoTookPictureDelegate = [[videoiPhoneCameraVideoTookPictureDelegate alloc] init]; // Create an instance of delegate.
                    [vid->cameraVideo setTookPictureDelegate:vid->cameraVideoTookPictureDelegate];
                    [vid->cameraVideo setTookPictureDelegateUserData:vid];
                } else {
                    // Cancellation message; unset delegate.
                    [vid->cameraVideo setTookPictureDelegate:nil];
                    [vid->cameraVideo setTookPictureDelegateUserData:NULL];
                    [vid->cameraVideoTookPictureDelegate release]; // Destroy instance of delegate.
                    vid->cameraVideoTookPictureDelegate = nil;
                }
            }
        }
    }
}

void (*ar2VideoGetGotImageFunctioniPhone(AR2VideoParamiPhoneT *vid))(AR2VideoBufferT *, void *)
{
    if (vid) return (vid->gotImageFunc);
    else return (NULL);
}

int ar2VideoGetCParamiPhone(AR2VideoParamiPhoneT *vid, ARParam *cparam)
{
    NSString *iOSDevice;
    const unsigned char *cparambytes;
    const char *cparamname;
    
    if (!vid || !cparam) return (-1);

    if (vid->cameraVideo) {
        iOSDevice = vid->cameraVideo.iOSDevice;
        if (iOSDevice == CameraVideoiOSDeviceiPhone3G ||
            iOSDevice == CameraVideoiOSDeviceiPhone3GS) {
            cparambytes = camera_para_iPhone;
            cparamname = "camera_para_iPhone.dat";
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone4) {
            if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                cparambytes = camera_para_iPhone_4_front;
                cparamname = "camera_para_iPhone_4_front.dat";
            } else {
                if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_4_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_4_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_4_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_4_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_4_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_4_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_4_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_4_rear_1280x720_0_3m.dat";
                            break;
                    }
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_4_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_4_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_4_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_4_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_4_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_4_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_4_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_4_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone4S ||
                   iOSDevice == CameraVideoiOSDeviceiPad3 ||
                   iOSDevice == CameraVideoiOSDeviceiPadMini) {
            if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                cparambytes = camera_para_iPhone_4S_front;
                cparamname = "camera_para_iPhone_4S_front.dat";
            }  else {
                if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_4S_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_4S_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_4S_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_4S_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_4S_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_4S_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_4S_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_4S_rear_1280x720_0_3m.dat";
                            break;
                    }
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_4S_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_4S_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_4S_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_4S_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_4S_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_4S_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_4S_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_4S_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPad4 ||
                   iOSDevice == CameraVideoiOSDeviceiPadAir ||
                   iOSDevice == CameraVideoiOSDeviceiPhone5 ||
                   iOSDevice == CameraVideoiOSDeviceiPodTouch5 ||
                   iOSDevice == CameraVideoiOSDeviceiPhone5c) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_5_front_1280x720;
                    cparamname = "camera_para_iPhone_5_front_1280x720.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_5_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_5_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_5_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_5_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_5_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_5_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_5_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_5_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_5_front_640x480;
                    cparamname = "camera_para_iPhone_5_front_640x480.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_5_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_5_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_5_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_5_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_5_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_5_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_5_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_5_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone5s) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_5s_front_1280x720;
                    cparamname = "camera_para_iPhone_5s_front_1280x720.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_5s_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_5s_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_5s_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_5s_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_5s_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_5s_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_5s_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_5s_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_5s_front_640x480;
                    cparamname = "camera_para_iPhone_5s_front_640x480.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_5s_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_5s_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_5s_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_5s_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_5s_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_5s_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_5s_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_5s_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPodTouch4) {
            if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                cparambytes = camera_para_iPod_touch_4G_front;
                cparamname = "camera_para_iPod_touch_4G_front.dat";
            } else {
                if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                    cparambytes = camera_para_iPod_touch_4G_rear_1280x720;
                    cparamname = "camera_para_iPod_touch_4G_rear_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPod_touch_4G_rear_640x480;
                    cparamname = "camera_para_iPod_touch_4G_rear_640x480.dat";
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPad2) {
            if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                cparambytes = camera_para_iPad_2_front;
                cparamname = "camera_para_iPad_2_front.dat";
            } else {
                if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                    vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                    cparambytes = camera_para_iPad_2_rear_1280x720;
                    cparamname = "camera_para_iPad_2_rear_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPad_2_rear_640x480;
                    cparamname = "camera_para_iPad_2_rear_640x480.dat";
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPadMini2 || iOSDevice == CameraVideoiOSDeviceiPadMini3) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_mini_3_front_1280x720;
                    cparamname = "camera_para_iPad_mini_3_front_1280x720.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPad_mini_3_rear_1280x720_inf;
                            cparamname = "camera_para_iPad_mini_3_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPad_mini_3_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPad_mini_3_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPad_mini_3_rear_1280x720_macro;
                            cparamname = "camera_para_iPad_mini_3_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPad_mini_3_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPad_mini_3_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_mini_3_front_640x480;
                    cparamname = "camera_para_iPad_mini_3_front_640x480.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPad_mini_3_rear_640x480_inf;
                            cparamname = "camera_para_iPad_mini_3_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPad_mini_3_rear_640x480_1_0m;
                            cparamname = "camera_para_iPad_mini_3_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPad_mini_3_rear_640x480_macro;
                            cparamname = "camera_para_iPad_mini_3_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPad_mini_3_rear_640x480_0_3m;
                            cparamname = "camera_para_iPad_mini_3_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone6Plus) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6_Plus_front_1280x720;
                    cparamname = "camera_para_iPhone_6_Plus_front_1280x720.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_6_Plus_rear_1280x720_inf;
                            cparamname = "camera_para_iPhone_6_Plus_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_6_Plus_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPhone_6_Plus_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_6_Plus_rear_1280x720_macro;
                            cparamname = "camera_para_iPhone_6_Plus_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_6_Plus_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPhone_6_Plus_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6_Plus_front_640x480;
                    cparamname = "camera_para_iPhone_6_Plus_front_640x480.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPhone_6_Plus_rear_640x480_inf;
                            cparamname = "camera_para_iPhone_6_Plus_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPhone_6_Plus_rear_640x480_1_0m;
                            cparamname = "camera_para_iPhone_6_Plus_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPhone_6_Plus_rear_640x480_macro;
                            cparamname = "camera_para_iPhone_6_Plus_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPhone_6_Plus_rear_640x480_0_3m;
                            cparamname = "camera_para_iPhone_6_Plus_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone6) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6_front_1280x720;
                    cparamname = "camera_para_iPhone_6_front_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPhone_6_rear_1280x720;
                    cparamname = "camera_para_iPhone_6_rear_1280x720_0_3m.dat";
                }
            } else {
                cparambytes = NULL;
                cparamname = NULL;
            }
        }
        else if (iOSDevice == CameraVideoiOSDeviceiPhone6SPlus) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6s_Plus_front_1280x720;
                    cparamname = "camera_para_iPhone_6s_Plus_front_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPhone_6s_Plus_rear_1280x720;
                    cparamname = "camera_para_iPhone_6s_Plus_rear_1280x720.dat";
                }
            } else {
                cparambytes = NULL;
                cparamname = NULL;
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPhone6S) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPhone_6s_front_1280x720;
                    cparamname = "camera_para_iPhone_6s_front_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPhone_6s_rear_1280x720;
                    cparamname = "camera_para_iPhone_6s_rear_1280x720.dat";
                }
            } else {
                cparambytes = NULL;
                cparamname = NULL;
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPadMini4) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_mini_4_front_1280x720;
                    cparamname = "camera_para_iPad_mini_4_front_1280x720.dat";
                } else {
                    cparambytes = camera_para_iPad_mini_4_rear_1280x720;
                    cparamname = "camera_para_iPad_mini_4_rear_1280x720.dat";
                }
            } else {
                cparambytes = NULL;
                cparamname = NULL;
            }
        } else if (iOSDevice == CameraVideoiOSDeviceiPadAir2) {
            if (vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetHigh ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1280x720 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPreset1920x1080 ||
                vid->cameraVideo.captureSessionPreset == AVCaptureSessionPresetiFrame960x540) {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_Air_2_front_1280x720;
                    cparamname = "camera_para_iPad_Air_2_front_1280x720.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPad_Air_2_rear_1280x720_inf;
                            cparamname = "camera_para_iPad_Air_2_rear_1280x720_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPad_Air_2_rear_1280x720_1_0m;
                            cparamname = "camera_para_iPad_Air_2_rear_1280x720_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPad_Air_2_rear_1280x720_macro;
                            cparamname = "camera_para_iPad_Air_2_rear_1280x720_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPad_Air_2_rear_1280x720_0_3m;
                            cparamname = "camera_para_iPad_Air_2_rear_1280x720_0_3m.dat";
                            break;
                    }
                }
            } else {
                if (vid->cameraVideo.captureDevicePosition == AVCaptureDevicePositionFront) {
                    cparambytes = camera_para_iPad_Air_2_front_640x480;
                    cparamname = "camera_para_iPad_Air_2_front_640x480.dat";
                } else {
                    switch (vid->focus) {
                        case AR_VIDEO_IOS_FOCUS_INF:
                            cparambytes = camera_para_iPad_Air_2_rear_640x480_inf;
                            cparamname = "camera_para_iPad_Air_2_rear_640x480_inf.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_1_0M:
                            cparambytes = camera_para_iPad_Air_2_rear_640x480_1_0m;
                            cparamname = "camera_para_iPad_Air_2_rear_640x480_1_0m.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_MACRO:
                            cparambytes = camera_para_iPad_Air_2_rear_640x480_macro;
                            cparamname = "camera_para_iPad_Air_2_rear_640x480_macro.dat";
                            break;
                        case AR_VIDEO_IOS_FOCUS_0_3M:
                        default:
                            cparambytes = camera_para_iPad_Air_2_rear_640x480_0_3m;
                            cparamname = "camera_para_iPad_Air_2_rear_640x480_0_3m.dat";
                            break;
                    }
                }
            }
        } else {
            cparambytes = NULL;
            cparamname = NULL;
        }
    } else {
        // !vid->cameraVideo
        cparambytes = NULL;
        cparamname = NULL;
    }

    if (cparambytes) {
        ARLOGe("Using %s for device %s.\n", cparamname, [iOSDevice cStringUsingEncoding:NSUTF8StringEncoding]);
        return (arParamLoadFromBuffer(cparambytes, cparam_size, cparam));
    } else {
        ARLOGe("Unable to find suitable camera parameters for device %s.\n", [iOSDevice cStringUsingEncoding:NSUTF8StringEncoding]);
        return (-1);
    }
}

#endif //  AR_INPUT_IPHONE
