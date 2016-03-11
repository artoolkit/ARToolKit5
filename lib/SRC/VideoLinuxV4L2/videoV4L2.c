/*
 *  videoV4L2.c
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
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Atsushi Nakazawa, Hirokazu Kato, Philip Lamb, Simon Goodall
 *
 */
/*
 *   Video capture subrutine for Linux/Video4Linux2 devices
 *   Based upon the V4L 1 artoolkit code and v4l2 spec example
 *   at http://v4l2spec.bytesex.org/spec/a13010.htm
 *   Simon Goodall <sg@ecs.soton.ac.uk>
 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h> // gettimeofday(), struct timeval
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset()
#include <errno.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <AR/ar.h>
#include <AR/video.h>



// V4L2 code from https://gist.github.com/jayrambhia/5866483
static int xioctl(int fd, int request, void *arg)
{
    int r;
    do r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);
    return r;
}

#define MAXCHANNEL   10

static void printPalette(int p) {
    switch (p) {
            //YUV formats
        case (V4L2_PIX_FMT_GREY): ARLOGi("  Pix Fmt: Grey\n"); break;
        case (V4L2_PIX_FMT_YUYV): ARLOGi("  Pix Fmt: YUYV\n"); break;
        case (V4L2_PIX_FMT_UYVY): ARLOGi("  Pix Fmt: UYVY\n"); break;
        case (V4L2_PIX_FMT_Y41P): ARLOGi("  Pix Fmt: Y41P\n"); break;
        case (V4L2_PIX_FMT_YVU420): ARLOGi("  Pix Fmt: YVU420\n"); break;
        case (V4L2_PIX_FMT_YVU410): ARLOGi("  Pix Fmt: YVU410\n"); break;
        case (V4L2_PIX_FMT_YUV422P): ARLOGi("  Pix Fmt: YUV422P\n"); break;
        case (V4L2_PIX_FMT_YUV411P): ARLOGi("  Pix Fmt: YUV411P\n"); break;
        case (V4L2_PIX_FMT_NV12): ARLOGi("  Pix Fmt: NV12\n"); break;
        case (V4L2_PIX_FMT_NV21): ARLOGi("  Pix Fmt: NV21\n"); break;
            // RGB formats
        case (V4L2_PIX_FMT_RGB332): ARLOGi("  Pix Fmt: RGB332\n"); break;
        case (V4L2_PIX_FMT_RGB555): ARLOGi("  Pix Fmt: RGB555\n"); break;
        case (V4L2_PIX_FMT_RGB565): ARLOGi("  Pix Fmt: RGB565\n"); break;
        case (V4L2_PIX_FMT_RGB555X): ARLOGi("  Pix Fmt: RGB555X\n"); break;
        case (V4L2_PIX_FMT_RGB565X): ARLOGi("  Pix Fmt: RGB565X\n"); break;
        case (V4L2_PIX_FMT_BGR24): ARLOGi("  Pix Fmt: BGR24\n"); break;
        case (V4L2_PIX_FMT_RGB24): ARLOGi("  Pix Fmt: RGB24\n"); break;
        case (V4L2_PIX_FMT_BGR32): ARLOGi("  Pix Fmt: BGR32\n"); break;
        case (V4L2_PIX_FMT_RGB32): ARLOGi("  Pix Fmt: RGB32\n"); break;
    };
    
}

static int getControl(int fd, int type, int *value) {
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;
    
    memset (&queryctrl, 0, sizeof (queryctrl));
    // TODO: Manke sure this is a correct value
    queryctrl.id = type;
    
    if (-1 == xioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
        if (errno != EINVAL) {
            ARLOGe("Error calling VIDIOC_QUERYCTRL\n");
            return 1;
        } else {
            ARLOGe("Control %d is not supported\n", type);
            return 1;
        }
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        ARLOGe("Control %s is not supported\n", queryctrl.name);
        return 1;
    } else {
        memset (&control, 0, sizeof (control));
        control.id = type;
        
        if (-1 == xioctl (fd, VIDIOC_G_CTRL, &control)) {
            ARLOGe("Error getting control %s value\n", queryctrl.name);
            return 1;
        }
        *value = control.value;
    }
    return 0;
}

static int setControl(int fd, int type, int value) {
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;
    
    memset (&queryctrl, 0, sizeof (queryctrl));
    // TODO: Manke sure this is a correct value
    queryctrl.id = type;
    
    if (-1 == xioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) {
        if (errno != EINVAL) {
            ARLOGe("Error calling VIDIOC_QUERYCTRL\n");
            return 1;
        } else {
            ARLOGe("Control %d is not supported\n", type);
            return 1;
        }
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        ARLOGe("Control %s is not supported\n", queryctrl.name);
        return 1;
    } else {
        memset (&control, 0, sizeof (control));
        control.id = type;
        // TODO check min/max range
        // If value is -1, then we use the default value
        control.value = (value == -1) ? (queryctrl.default_value) : (value);
        
        if (-1 == xioctl (fd, VIDIOC_S_CTRL, &control)) {
            ARLOGe("Error setting control %s to %d\n", queryctrl.name, value);
            return 1;
        }
    }
    return 0;
}

/*-------------------------------------------*/

int ar2VideoDispOptionV4L2( void )
{
    ARLOG(" -device=LinuxV4L2\n");
    ARLOG("\n");
    ARLOG("DEVICE CONTROLS:\n");
    ARLOG(" -dev=filepath\n");
    ARLOG("    specifies device file.\n");
    ARLOG(" -channel=N\n");
    ARLOG("    specifies source channel.\n");
    ARLOG(" -width=N\n");
    ARLOG("    specifies expected width of image.\n");
    ARLOG(" -height=N\n");
    ARLOG("    specifies expected height of image.\n");
    ARLOG(" -palette=[GREY|HI240|RGB565|RGB555|BGR24|BGR32|YUYV|UYVY|\n");
    ARLOG("    Y41P|YUV422P|YUV411P|YVU420|YVU410]\n");
    ARLOG("    specifies the camera palette (WARNING: not all options are supported by\n");
    ARLOG("    every camera).\n");
    ARLOG("IMAGE CONTROLS (WARNING: not all options are not supported by every camera):\n");
    ARLOG(" -brightness=N\n");
    ARLOG("    specifies brightness. (0.0 <-> 1.0)\n");
    ARLOG(" -contrast=N\n");
    ARLOG("    specifies contrast. (0.0 <-> 1.0)\n");
    ARLOG(" -saturation=N\n");
    ARLOG("    specifies saturation (color). (0.0 <-> 1.0) (for color camera only)\n");
    ARLOG(" -hue=N\n");
    ARLOG("    specifies hue. (0.0 <-> 1.0) (for color camera only)\n");
    ARLOG("OPTION CONTROLS:\n");
    ARLOG(" -mode=[PAL|NTSC|SECAM]\n");
    ARLOG("    specifies TV signal mode (for tv/capture card).\n");
    ARLOG("\n");
    
    return 0;
}

AR2VideoParamV4L2T *ar2VideoOpenV4L2(const char *config)
{
    
    // Warning, this function leaks badly when an error occurs.
    AR2VideoParamV4L2T            *vid;
    struct v4l2_capability   vd;
    struct v4l2_format fmt;
    struct v4l2_input  ipt;
    struct v4l2_requestbuffers req;
    
    const char *a;
    char line[256];
    int value;
    
    arMalloc( vid, AR2VideoParamV4L2T, 1 );
    strcpy( vid->dev, AR_VIDEO_V4L2_DEFAULT_DEVICE );
    vid->width      = AR_VIDEO_V4L2_DEFAULT_WIDTH;
    vid->height     = AR_VIDEO_V4L2_DEFAULT_HEIGHT;
    vid->channel    = AR_VIDEO_V4L2_DEFAULT_CHANNEL;
    vid->mode       = AR_VIDEO_V4L2_DEFAULT_MODE;
    vid->format     = AR_INPUT_V4L2_DEFAULT_PIXEL_FORMAT;
    vid->palette = V4L2_PIX_FMT_YUYV;     /* palette format */
    vid->contrast   = -1;
    vid->brightness = -1;
    vid->saturation = -1;
    vid->hue        = -1;
    vid->gamma  = -1;
    vid->exposure  = -1;
    vid->gain  = 1;
    //vid->debug      = 0;
    vid->debug      = 1;
    vid->videoBuffer=NULL;
    
    a = config;
    if( a != NULL) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;

            if (sscanf( a, "%s", line ) == 0) break;

            if( strncmp( a, "-dev=", 5 ) == 0 ) {
                if( sscanf( &line[5], "%s", vid->dev ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-channel=", 9 ) == 0 ) {
                if( sscanf( &line[9], "%d", &vid->channel ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-width=", 7 ) == 0 ) {
                if( sscanf( &line[7], "%d", &vid->width ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-height=", 8 ) == 0 ) {
                if( sscanf( &line[8], "%d", &vid->height ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-palette=", 9 ) == 0 ) {
                if( strncmp( &a[9], "GREY", 4) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_GREY;
                } else if( strncmp( &a[9], "HI240", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_HI240;
                } else if( strncmp( &a[9], "RGB565", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_RGB565;
                } else if( strncmp( &a[9], "RGB555", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_RGB555;
                } else if( strncmp( &a[9], "BGR24", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_BGR24;
                } else if( strncmp( &a[9], "BGR32", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_BGR32;
                } else if( strncmp( &a[9], "YUYV", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_YUYV;
                } else if( strncmp( &a[9], "UYVY", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_UYVY;
                } else if( strncmp( &a[9], "Y41P", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_Y41P;
                } else if( strncmp( &a[9], "YUV422P", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_YUV422P;
                } else if( strncmp( &a[9], "YUV411P", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_YUV411P;
                } else if( strncmp( &a[9], "YVU420", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_YVU420;
                } else if( strncmp( &a[9], "YVU410", 3) == 0 ) {
                    vid->palette = V4L2_PIX_FMT_YVU410;
                }
            }
            else if( strncmp( a, "-contrast=", 10 ) == 0 ) {
                if( sscanf( &line[10], "%d", &vid->contrast ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-brightness=", 12 ) == 0 ) {
                if( sscanf( &line[12], "%d", &vid->brightness ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-saturation=", 12 ) == 0 ) {
                if( sscanf( &line[12], "%d", &vid->saturation ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-hue=", 5 ) == 0 ) {
                if( sscanf( &line[5], "%d", &vid->hue ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-gamma=", 7 ) == 0 ) {
                if( sscanf( &line[7], "%d", &vid->gamma ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-exposure=", 10 ) == 0 ) {
                if( sscanf( &line[10], "%d", &vid->exposure ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-gain=", 6 ) == 0 ) {
                if( sscanf( &line[6], "%d", &vid->gain ) == 0 ) {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( a, "-mode=", 6 ) == 0 ) {
                if( strncmp( &a[6], "PAL", 3 ) == 0 )        vid->mode = V4L2_STD_PAL;
                else if( strncmp( &a[6], "NTSC", 4 ) == 0 )  vid->mode = V4L2_STD_NTSC;
                else if( strncmp( &a[6], "SECAM", 5 ) == 0 ) vid->mode = V4L2_STD_SECAM;
                else {
                    ar2VideoDispOptionV4L2();
                    free( vid );
                    return 0;
                }
            }
            else if( strcmp( line, "-debug" ) == 0 ) {
                vid->debug = 1;
            }
            else if( strcmp( line, "-device=LinuxV4L2" ) == 0 )    {
            }
            else {
                ARLOGe("Error: unrecognised configuration option '%s'.\n", a);
                ar2VideoDispOptionV4L2();
                free( vid );
                return 0;
            }
            
            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }
    
    vid->fd = open(vid->dev, O_RDWR);// O_RDONLY ?
    if(vid->fd < 0){
        ARLOGe("video device (%s) open failed\n",vid->dev);
        free( vid );
        return 0;
    }
    
    if(xioctl(vid->fd,VIDIOC_QUERYCAP,&vd) < 0){
        ARLOGe("xioctl failed\n");
        free( vid );
        return 0;
    }
    
    if (!(vd.capabilities & V4L2_CAP_STREAMING)) {
        ARLOGe("Device does not support streaming i/o\n");
    }
    
    if (vid->debug) {
        ARLOGe("=== debug info ===\n");
        ARLOGe("  vd.driver        =   %s\n",vd.driver);
        ARLOGe("  vd.card          =   %s\n",vd.card);
        ARLOGe("  vd.bus_info      =   %s\n",vd.bus_info);
        ARLOGe("  vd.version       =   %d\n",vd.version);
        ARLOGe("  vd.capabilities  =   %d\n",vd.capabilities);
    }
    
    memset(&fmt, 0, sizeof(fmt));
    
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#if 1
    fmt.fmt.pix.width       = vid->width;
    fmt.fmt.pix.height      = vid->height;
    fmt.fmt.pix.pixelformat = vid->palette;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
#else
    fmt.fmt.pix.width       = 640;
    fmt.fmt.pix.height      = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
#endif
    
    if (xioctl (vid->fd, VIDIOC_S_FMT, &fmt) < 0) {
        close(vid->fd);
        free( vid );
        ARLOGe("ar2VideoOpen: Error setting video format (%d)\n", errno);
        return 0;
    }
    
    // Get actual camera settings
    vid->palette = fmt.fmt.pix.pixelformat;
    vid->width = fmt.fmt.pix.width;
    vid->height = fmt.fmt.pix.height;
    
    if (vid->debug) {
        ARLOGe("  Width: %d\n", fmt.fmt.pix.width);
        ARLOGe("  Height: %d\n", fmt.fmt.pix.height);
        printPalette(fmt.fmt.pix.pixelformat);
    }
    
    memset(&ipt, 0, sizeof(ipt));
    
    ipt.index = vid->channel;
    ipt.std = vid->mode;
    
    if (xioctl(vid->fd,VIDIOC_ENUMINPUT,&ipt) < 0) {
        ARLOGe("arVideoOpen: Error querying input device type\n");
        close(vid->fd);
        free( vid );
        return 0;
    }
    
    if (vid->debug) {
        if (ipt.type == V4L2_INPUT_TYPE_TUNER) {
            ARLOGe("  Type: Tuner\n");
        }
        if (ipt.type == V4L2_INPUT_TYPE_CAMERA) {
            ARLOGe("  Type: Camera\n");
        }
    }
    
    // Set channel
    if (xioctl(vid->fd, VIDIOC_S_INPUT, &ipt)) {
        ARLOGe("arVideoOpen: Error setting video input\n");
        close(vid->fd);
        free( vid );
        return 0;
    }
    
    
    // Attempt to set some camera controls
    setControl(vid->fd, V4L2_CID_BRIGHTNESS, vid->brightness);
    setControl(vid->fd, V4L2_CID_CONTRAST, vid->contrast);
    setControl(vid->fd, V4L2_CID_SATURATION, vid->saturation);
    setControl(vid->fd, V4L2_CID_HUE, vid->hue);
    setControl(vid->fd, V4L2_CID_GAMMA, vid->gamma);
    setControl(vid->fd, V4L2_CID_EXPOSURE, vid->exposure);
    setControl(vid->fd, V4L2_CID_GAIN, vid->gain);
    
    // Print out current control values
    if (vid->debug ) {
        if (!getControl(vid->fd, V4L2_CID_BRIGHTNESS, &value)) {
            ARLOGe("Brightness: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_CONTRAST, &value)) {
            ARLOGe("Contrast: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_SATURATION, &value)) {
            ARLOGe("Saturation: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_HUE, &value)) {
            ARLOGe("Hue: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_GAMMA, &value)) {
            ARLOGe("Gamma: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_EXPOSURE, &value)) {
            ARLOGe("Exposure: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_GAIN, &value)) {
            ARLOGe("Gain: %d\n", value);
        }
    }
    
    //    if (vid->palette==V4L2_PIX_FMT_YUYV)
#if defined(AR_PIX_FORMAT_BGRA)
    arMalloc( vid->videoBuffer, ARUint8, vid->width*vid->height*4 );
#else
    arMalloc( vid->videoBuffer, ARUint8, vid->width*vid->height*3 );
#endif
    // Setup memory mapping
    memset(&req, 0, sizeof(req));
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (xioctl(vid->fd, VIDIOC_REQBUFS, &req)) {
        ARLOGe("Error calling VIDIOC_REQBUFS\n");
        close(vid->fd);
        if(vid->videoBuffer!=NULL) free(vid->videoBuffer);
        free( vid );
        return 0;
    }
    
    if (req.count < 2) {
        ARLOGe("this device can not be supported by libARvideo.\n");
        ARLOGe("(req.count < 2)\n");
        close(vid->fd);
        if(vid->videoBuffer!=NULL) free(vid->videoBuffer);
        free( vid );
        
        return 0;
    }
    
    vid->buffers = (struct buffer_ar_v4l2 *)calloc(req.count , sizeof(*vid->buffers));
    
    if (vid->buffers == NULL ) {
        ARLOGe("ar2VideoOpen: Error allocating buffer memory\n");
        close(vid->fd);
        if(vid->videoBuffer!=NULL) free(vid->videoBuffer);
        free( vid );
        return 0;
    }
    
    for (vid->n_buffers = 0; vid->n_buffers < req.count; ++vid->n_buffers) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = vid->n_buffers;
        
        if (xioctl (vid->fd, VIDIOC_QUERYBUF, &buf)) {
            ARLOGe("error VIDIOC_QUERYBUF\n");
            close(vid->fd);
            if(vid->videoBuffer!=NULL) free(vid->videoBuffer);
            free( vid );
            return 0;
        }
        
        vid->buffers[vid->n_buffers].length = buf.length;
        vid->buffers[vid->n_buffers].start =
        mmap (NULL /* start anywhere */,
              buf.length,
              PROT_READ | PROT_WRITE /* required */,
              MAP_SHARED /* recommended */,
              vid->fd, buf.m.offset);
        
        if (MAP_FAILED == vid->buffers[vid->n_buffers].start) {
            ARLOGe("Error mmap\n");
            close(vid->fd);
            if(vid->videoBuffer!=NULL) free(vid->videoBuffer);
            free( vid );
            return 0;
        }
    }
    
    vid->video_cont_num = -1;
    
    return vid;
}

int ar2VideoCloseV4L2( AR2VideoParamV4L2T *vid )
{
    if (vid->video_cont_num >= 0){
        ar2VideoCapStopV4L2( vid );
    }
    close(vid->fd);
    free(vid->videoBuffer);
    free(vid);
    
    return 0;
}

int ar2VideoGetIdV4L2( AR2VideoParamV4L2T *vid, ARUint32 *id0, ARUint32 *id1 )
{
    if (!vid) return -1;
    
    if (id0) *id0 = 0;
    if (id1) *id1 = 0;
    
    return -1;
}

int ar2VideoGetSizeV4L2(AR2VideoParamV4L2T *vid, int *x,int *y)
{
    if (!vid) return -1;
    
    if (x) *x = vid->width;
    if (y) *y = vid->height;
    
    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatV4L2( AR2VideoParamV4L2T *vid )
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID;
    
    return vid->format;
}

int ar2VideoCapStartV4L2( AR2VideoParamV4L2T *vid )
{
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;
    int i;
    
    if (vid->video_cont_num >= 0){
        ARLOGe("arVideoCapStart has already been called.\n");
        return -1;
    }
    
    vid->video_cont_num = 0;
    
    for (i = 0; i < vid->n_buffers; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(vid->fd, VIDIOC_QBUF, &buf)) {
            ARLOGe("ar2VideoCapStart: Error calling VIDIOC_QBUF: %d\n", errno);
            return -1;
        }
    }
    
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(vid->fd, VIDIOC_STREAMON, &type)) {
        ARLOGe("ar2VideoCapStart: Error calling VIDIOC_STREAMON\n");
        return -1;
    }
    
    return 0;
}

int ar2VideoCapStopV4L2( AR2VideoParamV4L2T *vid )
{
    if (!vid) return -1;

    if (vid->video_cont_num < 0){
        ARLOGe("arVideoCapStart has never been called.\n");
        return -1;
    }
    
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (xioctl(vid->fd, VIDIOC_STREAMOFF, &type)) {
        ARLOGe("Error calling VIDIOC_STREAMOFF\n");
        return -1;
    }
    
    vid->video_cont_num = -1;
    
    return 0;
}

// YUYV, aka YUV422, to RGB
// from http://pastebin.com/mDcwqJV3
static inline
void saturate(int* value, int min_val, int max_val)
{
    if (*value < min_val) *value = min_val;
    if (*value > max_val) *value = max_val;
}

// destination format AR_PIX_FORMAT_BGRA
static void yuyv_to_rgb32(int width, int height, const void *src, void *dst)
{
    unsigned char *yuyv_image = (unsigned char*) src;
    unsigned char *rgb_image = (unsigned char*) dst;
    const int K1 = (int)(1.402f * (1 << 16));
    const int K2 = (int)(0.714f * (1 << 16));
    const int K3 = (int)(0.334f * (1 << 16));
    const int K4 = (int)(1.772f * (1 << 16));
    
    typedef unsigned char T;
    T* out_ptr = &rgb_image[0];
    const T a = 0xff;
    const int pitch = width * 2; // 2 bytes per one YU-YV pixel
    int x, y;
    for (y=0; y<height; y++) {
        const T* src = yuyv_image + pitch * y;
        for (x=0; x<width*2; x+=4) { // Y1 U Y2 V
            T Y1 = src[x + 0];
            T U  = src[x + 1];
            T Y2 = src[x + 2];
            T V  = src[x + 3];
            
            char uf = U - 128;
            char vf = V - 128;
            
            int R = Y1 + (K1*vf >> 16);
            int G = Y1 - (K2*vf >> 16) - (K3*uf >> 16);
            int B = Y1 + (K4*uf >> 16);
            
            saturate(&R, 0, 255);
            saturate(&G, 0, 255);
            saturate(&B, 0, 255);
            
            *out_ptr++ = (T)(B);
            *out_ptr++ = (T)(G);
            *out_ptr++ = (T)(R);
            *out_ptr++ = a;
            
            R = Y2 + (K1*vf >> 16);
            G = Y2 - (K2*vf >> 16) - (K3*uf >> 16);
            B = Y2 + (K4*uf >> 16);
            
            saturate(&R, 0, 255);
            saturate(&G, 0, 255);
            saturate(&B, 0, 255);
            
            *out_ptr++ = (T)(B);
            *out_ptr++ = (T)(G);
            *out_ptr++ = (T)(R);
            *out_ptr++ = a;
        }
        
    }
    
}

// destination format AR_PIX_FORMAT_BGR
static void yuyv_to_rgb24(int width, int height, const void *src, void *dst)
{
    unsigned char *yuyv_image = (unsigned char*) src;
    unsigned char *rgb_image = (unsigned char*) dst;
    const int K1 = (int)(1.402f * (1 << 16));
    const int K2 = (int)(0.714f * (1 << 16));
    const int K3 = (int)(0.334f * (1 << 16));
    const int K4 = (int)(1.772f * (1 << 16));
    
    typedef unsigned char T;
    T* out_ptr = &rgb_image[0];
    const int pitch = width * 2; // 2 bytes per one YU-YV pixel
    int x, y;
    for (y=0; y<height; y++) {
        const T* src = yuyv_image + pitch * y;
        for (x=0; x<width*2; x+=4) { // Y1 U Y2 V
            T Y1 = src[x + 0];
            T U  = src[x + 1];
            T Y2 = src[x + 2];
            T V  = src[x + 3];
            
            char uf = U - 128;
            char vf = V - 128;
            
            int R = Y1 + (K1*vf >> 16);
            int G = Y1 - (K2*vf >> 16) - (K3*uf >> 16);
            int B = Y1 + (K4*uf >> 16);
            
            saturate(&R, 0, 255);
            saturate(&G, 0, 255);
            saturate(&B, 0, 255);
            
            *out_ptr++ = (T)(B);
            *out_ptr++ = (T)(G);
            *out_ptr++ = (T)(R);
            
            R = Y2 + (K1*vf >> 16);
            G = Y2 - (K2*vf >> 16) - (K3*uf >> 16);
            B = Y2 + (K4*uf >> 16);
            
            saturate(&R, 0, 255);
            saturate(&G, 0, 255);
            saturate(&B, 0, 255);
            
            *out_ptr++ = (T)(B);
            *out_ptr++ = (T)(G);
            *out_ptr++ = (T)(R);
        }
        
    }
}

AR2VideoBufferT *ar2VideoGetImageV4L2( AR2VideoParamV4L2T *vid )
{
    if (!vid) return NULL;
   
    if (vid->video_cont_num < 0){
        ARLOGe("arVideoCapStart has never been called.\n");
        return NULL;
    }
    
    ARUint8 *buffer;
    AR2VideoBufferT *out = &(vid->buffer.out);
    memset(out, 0, sizeof(*out));
    
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    
    if (xioctl(vid->fd, VIDIOC_DQBUF, &buf) < 0) {
        ARLOGe("Error calling VIDIOC_DQBUF: %d\n", errno);
        return out;
    }
    
    buffer = (ARUint8*)vid->buffers[buf.index].start;
    vid->video_cont_num = buf.index;
    
    // TODO: Add other video format conversions.
    if (vid->palette == V4L2_PIX_FMT_YUYV) {
#if defined(AR_PIX_FORMAT_BGRA)
        yuyv_to_rgb32(vid->width, vid->height, buffer, vid->videoBuffer);
#else
        yuyv_to_rgb24(vid->width, vid->height, buffer, vid->videoBuffer);
#endif
        out->buff = vid->videoBuffer;
        out->time_sec = buf.timestamp.tv_sec;
        out->time_usec = buf.timestamp.tv_usec; 
        out->fillFlag = 1;
        out->buffLuma = NULL;
    }
    
    struct v4l2_buffer buf_next;
    memset(&buf_next, 0, sizeof(buf_next));
    buf_next.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_next.memory = V4L2_MEMORY_MMAP;
    buf_next.index = vid->video_cont_num;
    
    if (xioctl(vid->fd, VIDIOC_QBUF, &buf_next)) {
        ARLOGe("ar2VideoCapNext: Error calling VIDIOC_QBUF: %d\n", errno);
    }

    return out;
}

int ar2VideoGetParamiV4L2( AR2VideoParamV4L2T *vid, int paramName, int *value )
{
    return -1;
}
int ar2VideoSetParamiV4L2( AR2VideoParamV4L2T *vid, int paramName, int  value )
{
    return -1;
}
int ar2VideoGetParamdV4L2( AR2VideoParamV4L2T *vid, int paramName, double *value )
{
    return -1;
}
int ar2VideoSetParamdV4L2( AR2VideoParamV4L2T *vid, int paramName, double  value )
{
    return -1;
}
