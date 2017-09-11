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

#define _GNU_SOURCE   // asprintf()/vasprintf() on Linux.
#define _XOPEN_SOURCE 500 // realpath()

#include "videoV4L2.h"

#ifdef ARVIDEO_INPUT_V4L2

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h> // gettimeofday(), struct timeval
#include <sys/param.h> // MAXPATHLEN
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h> // asprintf()
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h> // memset()
#include <errno.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <libudev.h>
#include "../cparamSearch.h"

#define   AR2VIDEO_V4L2_STATUS_IDLE    0
#define   AR2VIDEO_V4L2_STATUS_RUN     1
#define   AR2VIDEO_V4L2_STATUS_STOP    2

typedef struct {
    AR2VideoBufferT     in;
    AR2VideoBufferT     wait;
    AR2VideoBufferT     out;
    pthread_mutex_t     mutex;
} AR2VideoBufferV4L2T;

struct buffer_ar_v4l2 {
    ARUint8   *start;
    size_t    length;
};

struct _AR2VideoParamV4L2T {
    char                   dev[MAXPATHLEN];
    int                    width;
    int                    height;
    int                    channel;
    int                    mode;
    AR_PIXEL_FORMAT        format;
    int                    debug;
    int                    palette;
    int                    saturation;
    int                    exposure;
    int                    gain;
    int                    gamma;
    int                    contrast;
    int                    brightness;
    int                    hue;
    double                 whiteness;
    double                 color;

    int                    fd;
    int                    status;
    int                    video_cont_num;
    ARUint8                *videoBuffer;

    pthread_t              capture;
    AR2VideoBufferV4L2T    buffer;
    
    struct buffer_ar_v4l2 *buffers;
    int                    n_buffers;
    
    void                 (*cparamSearchCallback)(const ARParam *, void *);
    void                  *cparamSearchUserdata;
    char                  *device_id;
};

// V4L2 code from https://gist.github.com/jayrambhia/5866483
static int xioctl(int fd, int request, void *arg)
{
    int r;
    do r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);
    return r;
}

#define MAXCHANNEL   10

static void printPalette(int p)
{
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

static int getControl(int fd, int type, int *value)
{
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

static int setControl(int fd, int type, int value)
{
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

int ar2VideoDispOptionV4L2(void)
{
    ARLOG(" -module=V4L2\n");
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

ARVideoSourceInfoListT *ar2VideoCreateSourceInfoListV4L2(const char *config_in)
{
    struct udev *udev;
    ARVideoSourceInfoListT *sil = NULL;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev, *dev_parent;
	const char udev_err_msg[] = "Unable to query udev for V4L device list.\n"; 
	
	udev = udev_new();
    if (!udev) {
        ARLOGe(udev_err_msg);
        return NULL;
    }
    enumerate = udev_enumerate_new(udev);
    if (!enumerate) {
        ARLOGe(udev_err_msg);
        goto bail;
    }
    if (udev_enumerate_add_match_subsystem(enumerate, "video4linux") < 0) {
        ARLOGe(udev_err_msg);
        goto bail1;
    }
	if (udev_enumerate_scan_devices(enumerate) < 0) {
        ARLOGe(udev_err_msg);
        goto bail1;
    }
    devices = udev_enumerate_get_list_entry(enumerate);
    if (!devices) {
        goto bail1;
    }
    
    // Count devices, and create list.
    int count = 0;
    udev_list_entry_foreach(dev_list_entry, devices) {
        count++;
    } 
    arMallocClear(sil, ARVideoSourceInfoListT, 1);
    sil->count = count;
    arMallocClear(sil->info, ARVideoSourceInfoT, count);
    
    int i = 0;
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
        
        const char *path = udev_list_entry_get_name(dev_list_entry);
        //ARLOGd("System Name: %s\n", path);                         // e.g. '/sys/devices/pci0000:00/0000:00:1d.7/usb1/1-1/1-1:1.0/video4linux/video0'
		dev = udev_device_new_from_syspath(udev, path);
		//ARLOGd("Device Node: %s\n", udev_device_get_devnode(dev)); // e.g. '/dev/video0'
		//ARLOGd("System Name: %s\n", udev_device_get_sysname(dev)); // e.g. 'video0'
		//ARLOGd("Device Path: %s\n", udev_device_get_devpath(dev)); // e.g. '/devices/pci0000:00/0000:00:1d.7/usb1/1-1/1-1:1.0/video4linux/video0'

        sil->info[i].flags |= AR_VIDEO_POSITION_UNKNOWN;
        //sil->info[i].flags |= AR_VIDEO_SOURCE_INFO_FLAG_OPEN_ASYNC; // If we require async opening.
        sil->info[i].name = strdup(udev_device_get_sysattr_value(dev, "name"));
        //ARLOGd("name: %s\n", sil->info[i].name);                   // e.g. 'Logitech Camera'
        if (asprintf(&sil->info[i].open_token, "-dev=%s", udev_device_get_devnode(dev)) < 0) {
            ARLOGperror(NULL);
            sil->info[i].open_token = NULL;
        }
        
        dev_parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (dev_parent) {
            sil->info[i].UID = strdup(udev_device_get_sysattr_value(dev_parent, "serial"));
            //ARLOGd("UID: %s\n", sil->info[i].UID);
            const char *idVendor = udev_device_get_sysattr_value(dev_parent, "idVendor");
            const char *idProduct = udev_device_get_sysattr_value(dev_parent, "idProduct");
            const char *product = udev_device_get_sysattr_value(dev_parent, "product");
            if (idProduct && idVendor) {
                if (asprintf(&sil->info[i].model, "usb %s:%s%s%s", idVendor, idProduct, (product ? " " : ""), (product ? product : "")) == -1) {
                    ARLOGperror(NULL);
                    sil->info[i].model = NULL;
                }
            }
        }
        
        udev_device_unref(dev);
        
        i++;
    }

bail1:
    udev_enumerate_unref(enumerate);
bail:   
    udev_unref(udev);
    
    return (sil);
}

AR2VideoParamV4L2T *ar2VideoOpenV4L2(const char *config)
{
    char                     *cacheDir = NULL;
    char                     *cacheInitDir = NULL;
    AR2VideoParamV4L2T       *vid;
    struct v4l2_capability   vd;
    struct v4l2_format fmt;
    struct v4l2_input  ipt;
    struct v4l2_requestbuffers req;
    
    const char *a;
    char line[1024];
    int value;
    int err_i = 0;
    int i;
    
    arMallocClear(vid, AR2VideoParamV4L2T, 1);
    strcpy(vid->dev, AR_VIDEO_V4L2_DEFAULT_DEVICE);
    vid->width      = AR_VIDEO_V4L2_DEFAULT_WIDTH;
    vid->height     = AR_VIDEO_V4L2_DEFAULT_HEIGHT;
    vid->channel    = AR_VIDEO_V4L2_DEFAULT_CHANNEL;
    vid->mode       = AR_VIDEO_V4L2_DEFAULT_MODE;
    vid->format     = ARVIDEO_INPUT_V4L2_DEFAULT_PIXEL_FORMAT;
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
    vid->videoBuffer = NULL;
    
    a = config;
    if (a != NULL) {
        for(;;) {
            while(*a == ' ' || *a == '\t') a++;
            if (*a == '\0') break;

            if (sscanf(a, "%s", line) == 0) break;

            if (strncmp(a, "-dev=", 5) == 0) {
                if (sscanf(&line[5], "%s", vid->dev) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-channel=", 9) == 0) {
                if (sscanf(&line[9], "%d", &vid->channel) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-width=", 7) == 0) {
                if (sscanf(&line[7], "%d", &vid->width) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-height=", 8) == 0) {
                if (sscanf(&line[8], "%d", &vid->height) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-palette=", 9) == 0) {
                if (strncmp(&a[9], "GREY", 4) == 0) {
                    vid->palette = V4L2_PIX_FMT_GREY;
                } else if (strncmp(&a[9], "HI240", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_HI240;
                } else if (strncmp(&a[9], "RGB565", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_RGB565;
                } else if (strncmp(&a[9], "RGB555", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_RGB555;
                } else if (strncmp(&a[9], "BGR24", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_BGR24;
                } else if (strncmp(&a[9], "BGR32", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_BGR32;
                } else if (strncmp(&a[9], "YUYV", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_YUYV;
                } else if (strncmp(&a[9], "UYVY", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_UYVY;
                } else if (strncmp(&a[9], "Y41P", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_Y41P;
                } else if (strncmp(&a[9], "YUV422P", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_YUV422P;
                } else if (strncmp(&a[9], "YUV411P", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_YUV411P;
                } else if (strncmp(&a[9], "YVU420", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_YVU420;
                } else if (strncmp(&a[9], "YVU410", 3) == 0) {
                    vid->palette = V4L2_PIX_FMT_YVU410;
                }
            } else if (strncmp(a, "-contrast=", 10) == 0) {
                if (sscanf(&line[10], "%d", &vid->contrast) == 0) {
                    err_i = 1;
                    goto bail;
                }
            } else if (strncmp(a, "-brightness=", 12) == 0) {
                if (sscanf(&line[12], "%d", &vid->brightness) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-saturation=", 12) == 0) {
                if (sscanf(&line[12], "%d", &vid->saturation) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-hue=", 5) == 0) {
                if (sscanf(&line[5], "%d", &vid->hue) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-gamma=", 7) == 0) {
                if (sscanf(&line[7], "%d", &vid->gamma) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-exposure=", 10) == 0) {
                if (sscanf(&line[10], "%d", &vid->exposure) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-gain=", 6) == 0) {
                if (sscanf(&line[6], "%d", &vid->gain) == 0) {
                    err_i = 1;
                }
            } else if (strncmp(a, "-mode=", 6) == 0) {
                if (strncmp(&a[6], "PAL", 3) == 0)        vid->mode = V4L2_STD_PAL;
                else if (strncmp(&a[6], "NTSC", 4) == 0)  vid->mode = V4L2_STD_NTSC;
                else if (strncmp(&a[6], "SECAM", 5) == 0) vid->mode = V4L2_STD_SECAM;
                else {
                    err_i = 1;
                }
            } else if (strcmp(line, "-debug") == 0) {
                vid->debug = 1;
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
            } else if (strncmp(a, "-cacheinitdir=", 14) == 0) {
                // Attempt to read in pathname, allowing for quoting of whitespace.
                a += 14; // Skip "-cacheinitdir=" characters.
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
                    ARLOGe("Error: Configuration option '-cacheinitdir=' must be followed by path (optionally in double quotes).\n");
                    err_i = 1;
                } else {
                    free(cacheInitDir);
                    cacheInitDir = strdup(line);
                }
            } else if (strcmp(line, "-module=V4L2") == 0)    {
            } else {
                err_i = 1;
            }
            
            if (err_i) {
				ARLOGe("Error: Unrecognised configuration option '%s'.\n", a);
                ar2VideoDispOptionV4L2();
                goto bail;
			}
            
            while (*a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }
    
#if USE_CPARAM_SEARCH
	// Initialisation required before cparamSearch can be used.
    if (!cacheDir) {
        cacheDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_APP_CACHE_DIR);
    }
    if (!cacheInitDir) {
        cacheInitDir = arUtilGetResourcesDirectoryPath(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_USE_BUNDLE_RESOURCES_DIR);
    }
    if (cparamSearchInit(cacheDir, cacheInitDir, false) < 0) {
        ARLOGe("Unable to initialise cparamSearch.\n");
        goto bail;
    };
#endif
    free(cacheDir);
    cacheDir = NULL;
    free(cacheInitDir);
    cacheInitDir = NULL;

    vid->fd = open(vid->dev, O_RDWR);// O_RDONLY ?
    if (vid->fd < 0) {
        ARLOGe("video device (%s) open failed\n",vid->dev);
        goto bail;
    }
    
    if (xioctl(vid->fd,VIDIOC_QUERYCAP, &vd) < 0) {
        ARLOGe("xioctl failed\n");
        goto bail1;
    }
    
    if (!(vd.capabilities & V4L2_CAP_STREAMING)) {
        ARLOGe("Device does not support streaming i/o\n");
    }
    
    if (vid->debug) {
        ARLOGi("=== debug info ===\n");
        ARLOGi("  vd.driver        =   %s\n",vd.driver);
        ARLOGi("  vd.card          =   %s\n",vd.card);
        ARLOGi("  vd.bus_info      =   %s\n",vd.bus_info);
        ARLOGi("  vd.version       =   %d\n",vd.version);
        ARLOGi("  vd.capabilities  =   %d\n",vd.capabilities);
    }

    // Get the sysname of the device requested by the user.
    // Since vid->dev might be a symbolic link, it has to be resolved first.
    char *sysname = NULL;
    char *dev_real = realpath(vid->dev, NULL);
    if (!dev_real) {
        ARLOGe("Unable to resolve device path '%s'.\n", vid->dev);
        ARLOGperror(NULL);
    } else {
        if (strncmp(dev_real, "/dev/", 5) != 0) {
            ARLOGe("Resolved device path '%s' is not in /dev.\n", dev_real);
            free(dev_real);
        } else {
            sysname = &dev_real[5];
        }
    }
    
    if (sysname) {
        // Get udev device for the passed-in node.
        struct udev *udev = udev_new();
        if (!udev) {
            ARLOGe("Unable to query udev.\n");
        } else {
            struct udev_device *dev = udev_device_new_from_subsystem_sysname(udev, "video4linux", sysname);
            if (!dev) {
                ARLOGe("Unable to locate udev video4linux device '%s'.\n", sysname);
            } else {
                //const char *name = udev_device_get_sysattr_value(dev, "name");
                //ARLOGd("name: %s\n", name); // e.g. 'Logitech Camera'
                struct udev_device *dev_parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
                if (dev_parent) {
                    //char *UID = strdup(udev_device_get_sysattr_value(dev_parent, "serial"));
                    //ARLOGd("UID: %s\n", UID);
                    const char *idVendor = udev_device_get_sysattr_value(dev_parent, "idVendor");
                    const char *idProduct = udev_device_get_sysattr_value(dev_parent, "idProduct");
                    const char *product = udev_device_get_sysattr_value(dev_parent, "product");
                    if (idProduct && idVendor) {
                        if (asprintf(&vid->device_id, "/usb %s:%s%s%s/", idVendor, idProduct, (product ? " " : ""), (product ? product : "")) == -1) {
                            ARLOGperror(NULL);
                            vid->device_id = NULL;
                        }
                    }
                }
                udev_device_unref(dev);            
            }
            udev_unref(udev);
        }
        free(dev_real);
    }
    if (!vid->device_id) ARLOGe("Unable to obtain device_id.\n");
    else ARLOGi("device_id: '%s'.\n", vid->device_id);
    
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
        ARLOGe("ar2VideoOpen: Error setting video format (%d)\n", errno);
        goto bail1;
    }
    
    // Get actual camera settings
    vid->palette = fmt.fmt.pix.pixelformat;
    vid->width = fmt.fmt.pix.width;
    vid->height = fmt.fmt.pix.height;
    
    if (vid->debug) {
        ARLOGi("  Width: %d\n", fmt.fmt.pix.width);
        ARLOGi("  Height: %d\n", fmt.fmt.pix.height);
        printPalette(fmt.fmt.pix.pixelformat);
    }
    
    memset(&ipt, 0, sizeof(ipt));
    
    ipt.index = vid->channel;
    ipt.std = vid->mode;
    
    if (xioctl(vid->fd,VIDIOC_ENUMINPUT,&ipt) < 0) {
        ARLOGe("arVideoOpen: Error querying input device type\n");
        goto bail1;
    }
    
    if (vid->debug) {
        if (ipt.type == V4L2_INPUT_TYPE_TUNER) {
            ARLOGi("  Type: Tuner\n");
        }
        if (ipt.type == V4L2_INPUT_TYPE_CAMERA) {
            ARLOGi("  Type: Camera\n");
        }
    }
    
    // Set channel
    if (xioctl(vid->fd, VIDIOC_S_INPUT, &ipt)) {
        ARLOGe("arVideoOpen: Error setting video input\n");
        goto bail1;
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
    if (vid->debug) {
        if (!getControl(vid->fd, V4L2_CID_BRIGHTNESS, &value)) {
            ARLOGi("Brightness: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_CONTRAST, &value)) {
            ARLOGi("Contrast: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_SATURATION, &value)) {
            ARLOGi("Saturation: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_HUE, &value)) {
            ARLOGi("Hue: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_GAMMA, &value)) {
            ARLOGi("Gamma: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_EXPOSURE, &value)) {
            ARLOGi("Exposure: %d\n", value);
        }
        if (!getControl(vid->fd, V4L2_CID_GAIN, &value)) {
            ARLOGi("Gain: %d\n", value);
        }
    }
    
    //    if (vid->palette==V4L2_PIX_FMT_YUYV)
#if defined(AR_PIX_FORMAT_BGRA)
    arMalloc(vid->videoBuffer, ARUint8, vid->width*vid->height*4);
#else
    arMalloc(vid->videoBuffer, ARUint8, vid->width*vid->height*3);
#endif
    // Setup memory mapping
    memset(&req, 0, sizeof(req));
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (xioctl(vid->fd, VIDIOC_REQBUFS, &req)) {
        ARLOGe("Error calling VIDIOC_REQBUFS\n");
        goto bail2;
    }
    
    if (req.count < 2) {
        ARLOGe("This device can not be supported by libARvideo.\n");
        ARLOGe("(req.count < 2)\n");
        goto bail2;
    }
    
    vid->buffers = (struct buffer_ar_v4l2 *)calloc(req.count , sizeof(*vid->buffers));
    
    if (vid->buffers == NULL) {
        ARLOGe("ar2VideoOpen: Error allocating buffer memory\n");
        goto bail2;
    }
    
    for (vid->n_buffers = 0; vid->n_buffers < req.count; ++vid->n_buffers) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = vid->n_buffers;
        
        if (xioctl(vid->fd, VIDIOC_QUERYBUF, &buf)) {
            ARLOGe("error VIDIOC_QUERYBUF\n");
            goto bail2;
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
            goto bail2;
        }
    }
    
    vid->video_cont_num = -1;
    
    return vid;

bail2:
    free(vid->videoBuffer);
bail1:
    close(vid->fd);
bail:
    free(vid);
    return (NULL);
}

int ar2VideoCloseV4L2(AR2VideoParamV4L2T *vid)
{
    if (vid->video_cont_num >= 0) {
        ar2VideoCapStopV4L2(vid);
    }
    free(vid->videoBuffer);
    close(vid->fd);

#if USE_CPARAM_SEARCH
    if (cparamSearchFinal() < 0) {
        ARLOGe("Unable to finalise cparamSearch.\n");
    }
#endif

    free(vid);
    
    return 0;
}

int ar2VideoGetIdV4L2(AR2VideoParamV4L2T *vid, ARUint32 *id0, ARUint32 *id1)
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

AR_PIXEL_FORMAT ar2VideoGetPixelFormatV4L2(AR2VideoParamV4L2T *vid)
{
    if (!vid) return AR_PIXEL_FORMAT_INVALID;
    
    return vid->format;
}

int ar2VideoCapStartV4L2(AR2VideoParamV4L2T *vid)
{
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;
    int i;
    
    if (vid->video_cont_num >= 0) {
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

int ar2VideoCapStopV4L2(AR2VideoParamV4L2T *vid)
{
    if (!vid) return -1;

    if (vid->video_cont_num < 0) {
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
    else if (*value > max_val) *value = max_val;
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
        for (x = 0; x < width*2; x += 4) { // Y1 U Y2 V
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

AR2VideoBufferT *ar2VideoGetImageV4L2(AR2VideoParamV4L2T *vid)
{
    if (!vid) return NULL;
   
    if (vid->video_cont_num < 0) {
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
        out->time.sec = (uint64_t)buf.timestamp.tv_sec;
        out->time.usec = (uint32_t)buf.timestamp.tv_usec; 
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

int ar2VideoGetParamiV4L2(AR2VideoParamV4L2T *vid, int paramName, int *value)
{
    return -1;
}

int ar2VideoSetParamiV4L2(AR2VideoParamV4L2T *vid, int paramName, int  value)
{
    return -1;
}

int ar2VideoGetParamdV4L2(AR2VideoParamV4L2T *vid, int paramName, double *value)
{
    return -1;
}

int ar2VideoSetParamdV4L2(AR2VideoParamV4L2T *vid, int paramName, double  value)
{
    return -1;
}

int ar2VideoGetParamsV4L2( AR2VideoParamV4L2T *vid, const int paramName, char **value )
{
    if (!vid || !value) return (-1);
    
    switch (paramName) {
        case AR_VIDEO_PARAM_DEVICEID:
            *value = (vid->device_id ? strdup(vid->device_id) : NULL);
            break;
        default:
            return (-1);
    }
    return (0);
}

int ar2VideoSetParamsV4L2( AR2VideoParamV4L2T *vid, const int paramName, const char  *value )
{
    if (!vid) return (-1);
    
    switch (paramName) {
        default:
            return (-1);
    }
    return (0);
}

#if USE_CPARAM_SEARCH
static void cparamSeachCallback(CPARAM_SEARCH_STATE state, float progress, const ARParam *cparam, void *userdata)
{
    int final = false;
    AR2VideoParamV4L2T *vid = (AR2VideoParamV4L2T *)userdata;
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

int ar2VideoGetCParamAsyncV4L2(AR2VideoParamV4L2T *vid, void (*callback)(const ARParam *, void *), void *userdata)
{
    if (!vid) return (-1);
    if (!callback) {
        ARLOGw("Warning: cparamSearch requested without callback.\n");
    }
    
    if (!vid->device_id) {
        ARLOGe("Error: device identification not available.\n");
        return (-1);
    }
    
    int camera_index = 0;
    float focal_length = 0.0f;
    int width = 0, height = 0;
    if (ar2VideoGetSizeV4L2(vid, &width, &height) < 0) {
        ARLOGe("Error: request for camera parameters, but video size is unknown.\n");
        return (-1);
    };
    
    vid->cparamSearchCallback = callback;
    vid->cparamSearchUserdata = userdata;
    
    CPARAM_SEARCH_STATE initialState = cparamSearch(vid->device_id, camera_index, width, height, focal_length, &cparamSeachCallback, (void *)vid);
    if (initialState != CPARAM_SEARCH_STATE_INITIAL) {
        ARLOGe("Error %d returned from cparamSearch.\n", initialState);
        vid->cparamSearchCallback = vid->cparamSearchUserdata = NULL;
        return (-1);
    }
    
    return (0);
}

#endif // USE_CPARAM_SEARCH

#endif // ARVIDEO_INPUT_V4L2
