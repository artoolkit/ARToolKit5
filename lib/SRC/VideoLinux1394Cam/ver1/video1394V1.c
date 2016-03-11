/*
 *  video1394V1.c
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
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/* 
 *   Video capture subrutine for Linux/libdc1394 devices
 *   author: Hirokazu Kato ( kato@sys.im.hiroshima-cu.ac.jp )
 *
 *   Revision: 3.0   Date: 2004/01/01
 */

#include <AR/ar.h>
#ifndef AR_INPUT_1394CAM_USE_LIBDC1394_V2
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <linux/types.h>
#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>
#include <AR/video.h>
#include "video1394V1Private.h"


static ARVideo1394   arV1394[AR2VIDEO_1394_PORT_MAX];
static int           initFlag = 0;

static int  ar2VideoInit1394( int port, int debug );
static int  ar2VideGetCameraEUID1394( int port, int node, quadlet_t euid[2] );
static void ar2VideoBufferInit1394( AR2VideoBuffer1394T *buffer, int size );
static void ar2VideoGetTimeStamp1394(ARUint32 *t_sec, ARUint32 *t_usec);
static void ar2VideoCapture1394(AR2VideoParam1394T *vid);

ARVideo1394 *ar2VideoGetPortParam( int portNum )
{
    if( portNum < 0 || portNum >= AR2VIDEO_1394_PORT_MAX ) return NULL;
    return &arV1394[portNum];
}

int ar2VideoDispOption1394( void )
{
    ARLOG(" -device=Linux1394Cam\n");
    ARLOG("\n");
    ARLOG(" -port=N\n");
    ARLOG("    specifies a FireWire adaptor port (-1: Any).\n");
    ARLOG(" -euid=N\n");
    ARLOG("    specifies EUID of a FireWire camera (-1: Any).\n");
    ARLOG(" -mode=[320x240_YUV422|640x480_YUV422|640x480_RGB|\n");
    ARLOG("        640x480_YUV411|640x480_YUV411_HALF|640x480_MONO|\n");
    ARLOG("        640x480_MONO_COLOR|640x480_MONO_COLOR2|640x480_MONO_COLOR3\n");
    ARLOG("        640x480_MONO_COLOR_HALF|640x480_MONO_COLOR_HALF2|640x480_MONO_COLOR_HALF3\n");
    ARLOG("        1024x768_MONO|1024x768_MONO_COLOR|1024x768_MONO_COLOR2|1024x768_MONO_COLOR3]\n");
    ARLOG("    specifies input image format.\n");
    ARLOG(" -rate=N\n");
    ARLOG("    specifies desired framerate of a FireWire camera. \n");
    ARLOG("    (1.875, 3.75, 7.5, 15, 30, 60, 120, 240)\n");
    ARLOG(" -speed=[400|800]\n");
    ARLOG("    specifies interface speed.\n");
    ARLOG(" -format7\n");
    ARLOG("    use format7 camera mode.\n");
    ARLOG(" -reset\n");
    ARLOG("    resets camera to factory default settings.\n");
    ARLOG("    This is required for DFK21AF04 when it has been connected.\n");
    ARLOG("\n");

    return 0;
}

AR2VideoParam1394T *ar2VideoOpen1394( const char *config )
{
    AR2VideoParam1394T       *vid;
    ARUint32                  p1,p2;
    quadlet_t                 euid[2];
    quadlet_t                 value;
    int                       resetFlag;
    const char               *a;
    char                      b[256];
    int                       i, j;

    arMalloc( vid, AR2VideoParam1394T, 1 );
    vid->port         = AR_VIDEO_1394_DEFAULT_PORT;
    vid->euid[0]      = 0;
    vid->euid[1]      = 0;
    vid->mode         = AR_VIDEO_1394_DEFAULT_MODE;
    vid->rate         = AR_VIDEO_1394_DEFAULT_FRAME_RATE;
    vid->speed        = AR_VIDEO_1394_DEFAULT_SPEED;
    vid->dma_buf_num  = 3;
    vid->drop_frames  = 0;
    vid->format7      = 0;
    vid->debug        = 0;
    resetFlag         = 0;

    a = config;
    if( a != NULL) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;

            if( sscanf(a, "%s", b) == 0 ) break;
            if( strncmp( b, "-mode=", 6 ) == 0 ) {
                if ( strcmp( &b[6], "320x240_YUV422" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_320x240_YUV422;
                }
                else if ( strcmp( &b[6], "640x480_YUV422" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_YUV422;
                }
                else if ( strcmp( &b[6], "640x480_YUV411_HALF" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_YUV411_HALF;
                }
                else if ( strcmp( &b[6], "640x480_YUV411" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_YUV411;
                }
                else if ( strcmp( &b[6], "640x480_RGB" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_RGB;
                }
                else if ( strcmp( &b[6], "640x480_MONO_COLOR" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR;
                }
                else if ( strcmp( &b[6], "640x480_MONO_COLOR2" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR2;
                }
                else if ( strcmp( &b[6], "640x480_MONO_COLOR3" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR3;
                }
                else if ( strcmp( &b[6], "640x480_MONO_COLOR_HALF" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF;
                }
                else if ( strcmp( &b[6], "640x480_MONO_COLOR_HALF2" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF2;
                }
                else if ( strcmp( &b[6], "640x480_MONO_COLOR_HALF3" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF3;
                }
                else if ( strcmp( &b[6], "640x480_MONO" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_640x480_MONO;
                }
                else if ( strcmp( &b[6], "1024x768_MONO" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_1024x768_MONO;
                }
                else if ( strcmp( &b[6], "1024x768_MONO_COLOR" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_1024x768_MONO_COLOR;
                }
                else if ( strcmp( &b[6], "1024x768_MONO_COLOR2" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_1024x768_MONO_COLOR2;
                }
                else if ( strcmp( &b[6], "1024x768_MONO_COLOR3" ) == 0 ) {
                    vid->mode = AR_VIDEO_1394_MODE_1024x768_MONO_COLOR3;
                }
                else {
                    ar2VideoDispOption1394();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-port=", 6 ) == 0 ) {
                if( sscanf( &b[6], "%d", &vid->port ) == 0 ) {
                    ar2VideoDispOption1394();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-euid=", 6 ) == 0 ) {
                if( sscanf( &b[6], "%08x%08x", &vid->euid[1], &vid->euid[0] ) != 2 ) {
                    ar2VideoDispOption1394();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-rate=", 6 ) == 0 ) {
                if ( strcmp( &b[6], "1.875" ) == 0 ) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_1_875;
                }
                else if ( strcmp( &b[6], "3.75" ) == 0 ) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_3_75;
                }
                else if ( strcmp( &b[6], "7.5" ) == 0 ) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_7_5;
                }
                else if ( strcmp( &b[6], "15" ) == 0 ) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_15;
                }
                else if ( strcmp( &b[6], "30" ) == 0 ) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_30;
                }
                else if ( strcmp( &b[6], "60" ) == 0 ) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_60;
                }
                else if ( strcmp( &b[6], "120" ) == 0 ) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_120;
                }
                else if ( strcmp( &b[6], "240" ) == 0 ) {
                    vid->rate = AR_VIDEO_1394_FRAME_RATE_240;
                }
                else {
                    ar2VideoDispOption1394();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-speed=", 7 ) == 0 ) {
                if ( strcmp( &b[7], "400" ) == 0 ) {
                    vid->speed = AR_VIDEO_1394_SPEED_400;
                }
                else if ( strcmp( &b[7], "800" ) == 0 ) {
                    vid->speed = AR_VIDEO_1394_SPEED_800;
                }
                else {
                    ar2VideoDispOption1394();
                    free( vid );
                    return 0;
                }
            }
            else if( strcmp( b, "-format7" ) == 0 ) {
                vid->format7 = 1;
            }
            else if( strcmp( b, "-debug" ) == 0 ) {
                vid->debug = 1;
            }
            else if( strcmp( b, "-reset" ) == 0 ) {
                resetFlag = 1;
            }
            else if( strcmp( b, "-device=Linux1394Cam" ) == 0 )    {
            }
            else {
                ar2VideoDispOption1394();
                free( vid );
                return 0;
            }

            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }

    if( vid->format7 ) {
        vid->format = FORMAT_SCALABLE_IMAGE_SIZE;
    }
    else if( vid->mode == AR_VIDEO_1394_MODE_1024x768_MONO
     || vid->mode == AR_VIDEO_1394_MODE_1024x768_MONO_COLOR 
     || vid->mode == AR_VIDEO_1394_MODE_1024x768_MONO_COLOR2
     || vid->mode == AR_VIDEO_1394_MODE_1024x768_MONO_COLOR3 ) {
        vid->format = FORMAT_SVGA_NONCOMPRESSED_1;
    }
    else {
        vid->format = FORMAT_VGA_NONCOMPRESSED;
    }

    if( initFlag == 0 ) {
        for( i=0; i<AR2VIDEO_1394_PORT_MAX; i++ ) arV1394[i].isOpen = 0;
        initFlag = 1;
    }
    if( vid->port < 0 || vid->port >= AR2VIDEO_1394_PORT_MAX ) {
        ARLOGe("illegal port number!!  %d\n", vid->port);
        exit(0);
    }
    if( arV1394[vid->port].isOpen == 0 ) {
        if( ar2VideoInit1394( vid->port, vid->debug) < 0 ) return NULL;
        arV1394[vid->port].isOpen = 1;
    }
    if( arV1394[vid->port].handle == NULL ) {
        ARLOGe("Port #%d is not available.\n", vid->port);
        exit(0);
    }
    for( j=0; j<arV1394[vid->port].numCameras; j++ ) {
        if( vid->euid[0] == 0 && vid->euid[1] == 0 && arV1394[vid->port].activeFlag[j] == 0 ) {
            if( vid->debug ) {
                ARLOGi("Using a camera #%d (Node ID = %d) on port #%d\n", j, arV1394[vid->port].camera_nodes[j], vid->port);
            }
            vid->node        = arV1394[vid->port].camera_nodes[j];
            vid->internal_id = j;
            if( ar2VideGetCameraEUID1394(vid->port, arV1394[vid->port].camera_nodes[j], euid) == 0 ) {
                vid->euid[0] = euid[0];
                vid->euid[1] = euid[1];
            }
            break;
        }
        if( vid->euid[0] != 0 || vid->euid[1] != 0 ) {
            if( ar2VideGetCameraEUID1394(vid->port, arV1394[vid->port].camera_nodes[j], euid) < 0 ) continue;
            if( vid->euid[0] != euid[0] || vid->euid[1] != euid[1] ) continue;
            if( arV1394[vid->port].activeFlag[j] == 1 ) {
                ARLOGe("The camera(euid;%08x%08x) on port #%d is already used.\n", euid[1], euid[0], vid->port);
                exit(0);
            }
            vid->node        = arV1394[vid->port].camera_nodes[j];
            vid->internal_id = j;
            vid->euid[0]     = euid[0];
            vid->euid[1]     = euid[1];
            break;
        }
    }
    if( j >= arV1394[vid->port].numCameras ) {
        if( vid->debug ) ARLOGe("cound not find the specified camera.\n");
        return NULL;
    }
    vid->channel = vid->internal_id;
  
    if( resetFlag ) {
        dc1394_init_camera(arV1394[vid->port].handle, vid->node);
    }

    switch( vid->mode ) {
        case AR_VIDEO_1394_MODE_320x240_YUV422:
          vid->int_mode = MODE_320x240_YUV422;
          break;
        case AR_VIDEO_1394_MODE_640x480_YUV422:
          vid->int_mode = MODE_640x480_YUV422;
          break;
        case AR_VIDEO_1394_MODE_640x480_YUV411:
        case AR_VIDEO_1394_MODE_640x480_YUV411_HALF:
          vid->int_mode = MODE_640x480_YUV411;
          break;
        case AR_VIDEO_1394_MODE_640x480_RGB:
          vid->int_mode = MODE_640x480_RGB;
          break;
        case AR_VIDEO_1394_MODE_640x480_MONO:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR3:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF2:
        case AR_VIDEO_1394_MODE_640x480_MONO_COLOR_HALF3:
          vid->int_mode = MODE_640x480_MONO;
          break;
        case AR_VIDEO_1394_MODE_1024x768_MONO:
        case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR:
        case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR2:
        case AR_VIDEO_1394_MODE_1024x768_MONO_COLOR3:
          vid->int_mode = MODE_1024x768_MONO;
          break;
        default:
          ARLOGe("Sorry, Unsupported Video Format for IEEE1394 Camera.\n");
          exit(1);
    }
    switch( vid->rate ) {
        case AR_VIDEO_1394_FRAME_RATE_1_875:
          vid->int_rate = FRAMERATE_1_875;
          break;
        case AR_VIDEO_1394_FRAME_RATE_3_75:
          vid->int_rate = FRAMERATE_3_75;
          break;
        case AR_VIDEO_1394_FRAME_RATE_7_5:
          vid->int_rate = FRAMERATE_7_5;
          break;
        case AR_VIDEO_1394_FRAME_RATE_15:
          vid->int_rate = FRAMERATE_15;
          break;
        case AR_VIDEO_1394_FRAME_RATE_30:
          vid->int_rate = FRAMERATE_30;
          break;
        case AR_VIDEO_1394_FRAME_RATE_60:
          vid->int_rate = FRAMERATE_60;
          break;
        case AR_VIDEO_1394_FRAME_RATE_120:
          vid->int_rate = FRAMERATE_120;
          break;
        case AR_VIDEO_1394_FRAME_RATE_240:
          vid->int_rate = FRAMERATE_240;
          break;
        default:
          ARLOGe("Sorry, Unsupported Frame Rate for IEEE1394 Camera.\n");
          exit(1);
    }
    switch( vid->speed ) {
        case AR_VIDEO_1394_SPEED_400:
          vid->int_speed = SPEED_400;
          break;
        case AR_VIDEO_1394_SPEED_800:
          vid->int_speed = SPEED_800;
          break;
        default:
          ARLOGe("Sorry, Unsupported Frame Rate for IEEE1394 Camera.\n");
          exit(1);
    }
  
    /*-----------------------------------------------------------------------*/
    /*  report camera's features                                             */
    /*-----------------------------------------------------------------------*/
    if( dc1394_get_camera_feature_set(arV1394[vid->port].handle, 
				      vid->node,
				      &(vid->features)) != DC1394_SUCCESS ) {
        ARLOGe("unable to get feature set\n");
    }
    else if( vid->debug ) {
        dc1394_print_feature_set( &(vid->features) );
    }

    /*-----------------------------------------------------------------------*/
    /*  check parameters                                                     */
    /*-----------------------------------------------------------------------*/
    if( dc1394_query_supported_formats(arV1394[vid->port].handle, vid->node, &value) != DC1394_SUCCESS ) {
        ARLOGe("unable to query_supported_formats\n");
    }
    i = 31 - (vid->format - FORMAT_MIN);
    p1 = 1 << i;
    p2 = value & p1;
    if( p2 == 0 ) {
        ARLOGe("unable to use this camera with format%d. %x\n", vid->format-FORMAT_MIN, value);
        exit(0);
    }

    if( vid->format == FORMAT_VGA_NONCOMPRESSED ) {
        dc1394_query_supported_modes(arV1394[vid->port].handle, vid->node,  FORMAT_VGA_NONCOMPRESSED, &value);
        i = 31 - (vid->int_mode - MODE_FORMAT0_MIN);
        p1 = 1 << i;
        p2 = value & p1;
        if( p2 == 0 ) {
            ARLOGe("Unsupported Mode for the specified camera.\n");
            ar2VideoDispOption1394();
            exit(0);
        }
        dc1394_query_supported_framerates(arV1394[vid->port].handle, vid->node, FORMAT_VGA_NONCOMPRESSED, vid->int_mode, &value);
        i = 31 - (vid->int_rate - FRAMERATE_MIN);
        p1 = 1 << i;
        p2 = value & p1;
        if( p2 == 0 ) {
            ARLOGe("Unsupported Framerate for the specified mode.\n");
            ar2VideoDispOption1394();
            exit(0);
        }
    }
    else if( vid->format == FORMAT_SVGA_NONCOMPRESSED_1 ) {
        dc1394_query_supported_modes(arV1394[vid->port].handle, vid->node,  FORMAT_SVGA_NONCOMPRESSED_1, &value);
        i = 31 - (vid->int_mode - MODE_FORMAT0_MIN);
        p1 = 1 << i;
        p2 = value & p1;
        if( p2 == 0 ) {
            ARLOGe("Unsupported Mode for the specified camera.\n");
            ar2VideoDispOption1394();
            exit(0);
        }
        dc1394_query_supported_framerates(arV1394[vid->port].handle, vid->node, FORMAT_SVGA_NONCOMPRESSED_1, vid->int_mode, &value);
        i = 31 - (vid->int_rate - FRAMERATE_MIN);
        p1 = 1 << i;
        p2 = value & p1;
        if( p2 == 0 ) {
            ARLOGe("Unsupported Framerate for the specified mode.\n");
            ar2VideoDispOption1394();
            exit(0);
        }
    }
    else if( vid->format == FORMAT_SCALABLE_IMAGE_SIZE ) {
        if( vid->int_mode != MODE_640x480_MONO ) {
            ARLOGe("Unsupported Mode with Format 7.\n");
            ar2VideoDispOption1394();
            exit(0);
        }
    }
    else {
        ARLOGe("Unsupported camera image format.\n");
        ar2VideoDispOption1394();
        exit(0);
    }

    /*-----------------------------------------------------------------------*/
    /*  setup capture                                                        */
    /*-----------------------------------------------------------------------*/
    if( vid->speed == AR_VIDEO_1394_SPEED_400 ) {
        dc1394_set_operation_mode(arV1394[vid->port].handle, vid->node, OPERATION_MODE_LEGACY);
    }
    else {
        dc1394_set_operation_mode(arV1394[vid->port].handle, vid->node, OPERATION_MODE_1394B);
    }

    if( vid->format7 == 0 ) {
        if( dc1394_dma_setup_capture(arV1394[vid->port].handle,
			             vid->node,
			             vid->channel,
			             vid->format,
			             vid->int_mode,
			             vid->int_speed,
			             vid->int_rate,
			             vid->dma_buf_num,
                                     //0,
                                     vid->drop_frames,
                                     //vid->dma_channel,
                                     NULL,
			             &(vid->camera)) != DC1394_SUCCESS ) {
            ARLOGe("unable to setup camera-\n"
                    "check if you did 'insmod video1394' or,\n"
                    "check line %d of %s to make sure\n"
                    "that the video mode,framerate and format are\n"
                    "supported by your camera\n",
                    __LINE__,__FILE__);
            exit(0);
        }
    }
    else {
        if( dc1394_dma_setup_format7_capture(arV1394[vid->port].handle,
                                             vid->node,
                                             vid->channel, 
                                             MODE_FORMAT7_0,
                                             vid->int_speed,
                                             USE_MAX_AVAIL,  //unsigned int bytes_per_packet,
                                             0, 0,           //unsigned int left, unsigned int top,
                                             640, 480,       //unsigned int width, unsigned int height,
                                             vid->dma_buf_num,
                                             vid->drop_frames,
                                             NULL,
                                             &(vid->camera)) != DC1394_SUCCESS ) {
            ARLOGe("unable to setup camera-\n"
                    "check if you did 'insmod video1394' or,\n"
                    "check line %d of %s to make sure\n"
                    "that the video mode,framerate and format are\n"
                    "supported by your camera\n",
                    __LINE__,__FILE__);
            exit(0);
        }
    }
  
    /* set trigger mode */
    if( dc1394_set_trigger_mode(arV1394[vid->port].handle, vid->node, TRIGGER_MODE_0) != DC1394_SUCCESS ) {
        ARLOGe("unable to set camera trigger mode (ignored)\n");
    }

    arV1394[vid->port].activeFlag[vid->internal_id] = 1;

    if( vid->mode == AR_VIDEO_1394_MODE_640x480_MONO 
     || vid->mode == AR_VIDEO_1394_MODE_1024x768_MONO ) {
        ar2VideoBufferInit1394( &(vid->buffer), vid->camera.frame_width * vid->camera.frame_height * 1 );
    }
    else {
        ar2VideoBufferInit1394( &(vid->buffer), vid->camera.frame_width * vid->camera.frame_height * 3 );
    }
    vid->status = AR2VIDEO_1394_STATUS_IDLE;
    pthread_create(&(vid->capture), NULL, (void * (*)(void *))ar2VideoCapture1394, vid);

    return vid;
}


int ar2VideoClose1394( AR2VideoParam1394T *vid )
{
    vid->status = AR2VIDEO_1394_STATUS_STOP;

    pthread_join( vid->capture, NULL );

    dc1394_dma_release_camera(arV1394[vid->port].handle, &(vid->camera));
    arV1394[vid->port].activeFlag[vid->internal_id] = 0;
    free(vid->buffer.in.buff  );
    free(vid->buffer.wait.buff);
    free(vid->buffer.out.buff );
    free( vid );

    arUtilSleep(100);

    return 0;
} 

int ar2VideoGetId1394( AR2VideoParam1394T *vid, ARUint32 *id0, ARUint32 *id1 )     
{
    if( vid == NULL ) return -1;

    *id0 = (ARUint32)vid->euid[0];
    *id1 = (ARUint32)vid->euid[1];

    return 0;
}

int ar2VideoGetSize1394(AR2VideoParam1394T *vid, int *x,int *y)
{
    if( vid == NULL ) return -1;

    *x = vid->camera.frame_width;
    *y = vid->camera.frame_height;

    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormat1394( AR2VideoParam1394T *vid )
{
    if( vid == NULL ) return AR_PIXEL_FORMAT_INVALID;

    if( vid->mode == AR_VIDEO_1394_MODE_640x480_MONO
     || vid->mode == AR_VIDEO_1394_MODE_1024x768_MONO ) {
        return AR_PIXEL_FORMAT_MONO;
    }
    else {
        return AR_PIXEL_FORMAT_RGB;
    }
}


int ar2VideoCapStart1394( AR2VideoParam1394T *vid )
{
    if(vid->status == AR2VIDEO_1394_STATUS_RUN){
        ARLOGe("arVideoCapStart has already been called.\n");
        return -1;  
    }

    vid->status = AR2VIDEO_1394_STATUS_RUN;

    return 0;
}


int ar2VideoCapStop1394( AR2VideoParam1394T *vid )
{
    if( vid->status != AR2VIDEO_1394_STATUS_RUN ) return -1;
    vid->status = AR2VIDEO_1394_STATUS_IDLE;   
    arUtilSleep(100);

    return 0;
}

AR2VideoBufferT *ar2VideoGetImage1394( AR2VideoParam1394T *vid )
{
    AR2VideoBufferT   tmp;

    pthread_mutex_lock(&(vid->buffer.mutex));
    tmp = vid->buffer.wait;
    vid->buffer.wait = vid->buffer.out;
    vid->buffer.out = tmp;

    vid->buffer.wait.fillFlag = 0;
    pthread_mutex_unlock(&(vid->buffer.mutex));

    return &(vid->buffer.out);
}

static void ar2VideoCapture1394(AR2VideoParam1394T *vid)
{
    AR2VideoBufferT   tmp;
    int               startFlag = 0;

    while(vid->status != AR2VIDEO_1394_STATUS_STOP) {
        if( vid->status == AR2VIDEO_1394_STATUS_RUN && startFlag == 0 ) {
            if( dc1394_start_iso_transmission(arV1394[vid->port].handle, vid->node) != DC1394_SUCCESS ) {
                ARLOGe("error: unable to start camera iso transmission\n");
                return;
            }
            startFlag = 1;
        }

        if( vid->status == AR2VIDEO_1394_STATUS_IDLE && startFlag == 1 ) {
            if( dc1394_stop_iso_transmission(arV1394[vid->port].handle, vid->node) != DC1394_SUCCESS ) {
                ARLOGe("error: couldn't stop the camera?\n");
                return;
            }
            startFlag = 0;
        }

        if( vid->status == AR2VIDEO_1394_STATUS_IDLE ) {
            usleep(100);
            continue;
        }

        if( startFlag ) {
            if( dc1394_dma_single_capture( &(vid->camera) ) != DC1394_SUCCESS ) {
                ARLOGe("error: unable to capture a frame\n");
                return;
            }

            ar2VideoGetTimeStamp1394( &(vid->buffer.in.time_sec), &(vid->buffer.in.time_usec) );
            vid->buffer.in.fillFlag = 1;
            vid->buffer.in.buffLuma = NULL;

            ar2Video1394FormatConversion( (ARUint8 *)vid->camera.capture_buffer,
                                          vid->buffer.in.buff,
                                          vid->mode,
                                          vid->camera.frame_width,
                                          vid->camera.frame_height );

            dc1394_dma_done_with_buffer( &(vid->camera) );

            pthread_mutex_lock(&(vid->buffer.mutex));
            tmp = vid->buffer.wait;
            vid->buffer.wait = vid->buffer.in;
            vid->buffer.in = tmp;
            pthread_mutex_unlock(&(vid->buffer.mutex));
        }
    }

    if( startFlag == 1 ) {
        if( dc1394_stop_iso_transmission(arV1394[vid->port].handle, vid->node) != DC1394_SUCCESS ) {
            ARLOGe("error: couldn't stop the camera?\n");
            return;
        }
        startFlag = 0;
    }

    return;
}


static int ar2VideoInit1394( int port, int debug )
{
    int     i;

    /*-----------------------------------------------------------------------*/
    /*  Open ohci and asign handle to it                                     */
    /*-----------------------------------------------------------------------*/
    arV1394[port].handle = dc1394_create_handle(port);
    if (arV1394[port].handle==NULL) return -1;

    /*-----------------------------------------------------------------------*/
    /*  get the camera nodes and describe them as we find them               */
    /*-----------------------------------------------------------------------*/
    arV1394[port].numNodes = raw1394_get_nodecount(arV1394[port].handle);
    arV1394[port].camera_nodes = dc1394_get_camera_nodes(arV1394[port].handle,&arV1394[port].numCameras,((debug)? 1: 0));
    if (arV1394[port].numCameras<1) {
        ARLOGe("no cameras found on port #%d\n", port);
        raw1394_destroy_handle(arV1394[port].handle);
        arV1394[port].handle = NULL;
        return -1;
    }
    if( debug ) {
        ARLOGi("%d camera(s) found on the port #%d\n", arV1394[port].numCameras, port);
        ARLOGi("Node IDs are: ");
        for (i=0; i<arV1394[port].numCameras; i++) {
	    ARLOGi("%d, ", arV1394[port].camera_nodes[i]);
        }
        ARLOGi("\n");
    }

    arMalloc( arV1394[port].activeFlag, int, arV1394[port].numCameras );
    for( i=0; i<arV1394[port].numCameras; i++ ) {
        if( arV1394[port].camera_nodes[i] == arV1394[port].numNodes-1) {
            ARLOGe("\n"
                    "If ohci1394 is not working as root, please do:\n"
                    "\n"
                    "   rmmod ohci1394\n"
                    "   insmod ohci1394 attempt_root=1\n"
                    "\n"
                    "Otherwise, try to change FireWire connections so that\n"
                    "the highest number is not given to any camera.\n");
            exit(0);
        }

        arV1394[port].activeFlag[i] = 0;
    }

    return 0;
}

static int ar2VideGetCameraEUID1394( int port, int node, quadlet_t euid[2] )
{
    dc1394_camerainfo caminfo;
                                                                                
    if(dc1394_get_camera_info(arV1394[port].handle, node, &caminfo) != DC1394_SUCCESS) return -1;
    //dc1394_print_camera_info( &caminfo );

    euid[0]= caminfo.euid_64 & 0xffffffff;
    euid[1]= (caminfo.euid_64 >>32) & 0xffffffff;
 
    return 0;
}

static void ar2VideoBufferInit1394( AR2VideoBuffer1394T *buffer, int size )
{
    arMalloc( buffer->in.buff,   ARUint8, size );
    arMalloc( buffer->wait.buff, ARUint8, size );
    arMalloc( buffer->out.buff,  ARUint8, size );
    buffer->in.fillFlag   = 0;
    buffer->wait.fillFlag = 0;
    buffer->out.fillFlag  = 0;
    buffer->in.buffLuma   = NULL;
    buffer->wait.buffLuma = NULL;
    buffer->out.buffLuma  = NULL;
    pthread_mutex_init(&(buffer->mutex), NULL);

    return;
}

static void ar2VideoGetTimeStamp1394(ARUint32 *t_sec, ARUint32 *t_usec)
{
#ifdef _WIN32
    struct _timeb sys_time;
    double             tt;
    int                s1, s2;

    _ftime(&sys_time);
    *t_sec  = sys_time.time;
    *t_usec = sys_time.millitm * 1000;
#else
    struct timeval     time;
    double             tt;
    int                s1, s2;

#if defined(__linux) || defined(__APPLE__)
    gettimeofday( &time, NULL );
#else
    gettimeofday( &time );
#endif
    *t_sec  = time.tv_sec;
    *t_usec = time.tv_usec;
#endif

    return;
}

int ar2VideoGetParami1394( AR2VideoParam1394T *vid, int paramName, int *value )
{
    int    min, max;
    int    ub, vr;
    int    ret;

    if( paramName == AR_VIDEO_1394_BRIGHTNESS ) {
        return ar2VideoGetValue1394( vid, AR_VIDEO_1394_BRIGHTNESS, value );
    }
    else if( paramName == AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON ) {
        return ar2VideoGetFeatureOn1394( vid, AR_VIDEO_1394_BRIGHTNESS, value );
    }
    else if( paramName == AR_VIDEO_1394_BRIGHTNESS_AUTO_ON ) {
        return ar2VideoGetAutoOn1394( vid, AR_VIDEO_1394_BRIGHTNESS, value );
    }
    else if( paramName == AR_VIDEO_1394_BRIGHTNESS_MAX_VAL ) {
        return ar2VideoGetMaxValue1394( vid, AR_VIDEO_1394_BRIGHTNESS, value );
    }
    else if( paramName == AR_VIDEO_1394_BRIGHTNESS_MIN_VAL ) {
        return ar2VideoGetMinValue1394( vid, AR_VIDEO_1394_BRIGHTNESS, value );
    }
    else if( paramName == AR_VIDEO_1394_EXPOSURE ) {
        return ar2VideoGetValue1394( vid, AR_VIDEO_1394_EXPOSURE, value );
    }
    else if( paramName == AR_VIDEO_1394_EXPOSURE_FEATURE_ON ) {
        return ar2VideoGetFeatureOn1394( vid, AR_VIDEO_1394_EXPOSURE, value );
    }
    else if( paramName == AR_VIDEO_1394_EXPOSURE_AUTO_ON ) {
        return ar2VideoGetAutoOn1394( vid, AR_VIDEO_1394_EXPOSURE, value );
    }
    else if( paramName == AR_VIDEO_1394_EXPOSURE_MAX_VAL ) {
        return ar2VideoGetMaxValue1394( vid, AR_VIDEO_1394_EXPOSURE, value );
    }
    else if( paramName == AR_VIDEO_1394_EXPOSURE_MIN_VAL ) {
        return ar2VideoGetMinValue1394( vid, AR_VIDEO_1394_EXPOSURE, value );
    }
    else if( paramName == AR_VIDEO_1394_SHUTTER_SPEED ) {
        return ar2VideoGetValue1394( vid, AR_VIDEO_1394_SHUTTER_SPEED, value );
    }
    else if( paramName == AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON ) {
        return ar2VideoGetFeatureOn1394( vid, AR_VIDEO_1394_SHUTTER_SPEED, value );
    }
    else if( paramName == AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON ) {
        return ar2VideoGetAutoOn1394( vid, AR_VIDEO_1394_SHUTTER_SPEED, value );
    }
    else if( paramName == AR_VIDEO_1394_SHUTTER_SPEED_MAX_VAL ) {
        return ar2VideoGetMaxValue1394( vid, AR_VIDEO_1394_SHUTTER_SPEED, value );
    }
    else if( paramName == AR_VIDEO_1394_SHUTTER_SPEED_MIN_VAL ) {
        return ar2VideoGetMinValue1394( vid, AR_VIDEO_1394_SHUTTER_SPEED, value );
    }
    else if( paramName == AR_VIDEO_1394_GAIN ) {
        return ar2VideoGetValue1394( vid, AR_VIDEO_1394_GAIN, value );
    }
    else if( paramName == AR_VIDEO_1394_GAIN_FEATURE_ON ) {
        return ar2VideoGetFeatureOn1394( vid, AR_VIDEO_1394_GAIN, value );
    }
    else if( paramName == AR_VIDEO_1394_GAIN_AUTO_ON ) {
        return ar2VideoGetAutoOn1394( vid, AR_VIDEO_1394_GAIN, value );
    }
    else if( paramName == AR_VIDEO_1394_GAIN_MAX_VAL ) {
        return ar2VideoGetMaxValue1394( vid, AR_VIDEO_1394_GAIN, value );
    }
    else if( paramName == AR_VIDEO_1394_GAIN_MIN_VAL ) {
        return ar2VideoGetMinValue1394( vid, AR_VIDEO_1394_GAIN, value );
    }
    else if( paramName == AR_VIDEO_1394_FOCUS ) {
        return ar2VideoGetValue1394( vid, AR_VIDEO_1394_FOCUS, value );
    }
    else if( paramName == AR_VIDEO_1394_FOCUS_FEATURE_ON ) {
        return ar2VideoGetFeatureOn1394( vid, AR_VIDEO_1394_FOCUS, value );
    }
    else if( paramName == AR_VIDEO_1394_FOCUS_AUTO_ON ) {
        return ar2VideoGetAutoOn1394( vid, AR_VIDEO_1394_FOCUS, value );
    }
    else if( paramName == AR_VIDEO_1394_FOCUS_MAX_VAL ) {
        return ar2VideoGetMaxValue1394( vid, AR_VIDEO_1394_FOCUS, value );
    }
    else if( paramName == AR_VIDEO_1394_FOCUS_MIN_VAL ) {
        return ar2VideoGetMinValue1394( vid, AR_VIDEO_1394_FOCUS, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_UB ) {
        return ar2VideoGetValue1394( vid, AR_VIDEO_1394_WHITE_BALANCE_UB, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_VR ) {
        return ar2VideoGetValue1394( vid, AR_VIDEO_1394_WHITE_BALANCE_VR, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON ) {
        return ar2VideoGetFeatureOn1394( vid, AR_VIDEO_1394_WHITE_BALANCE, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON ) {
        return ar2VideoGetAutoOn1394( vid, AR_VIDEO_1394_WHITE_BALANCE, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_MAX_VAL ) {
        return ar2VideoGetMaxValue1394( vid, AR_VIDEO_1394_WHITE_BALANCE, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_MIN_VAL ) {
        return ar2VideoGetMinValue1394( vid, AR_VIDEO_1394_WHITE_BALANCE, value );
    }

    return -1;
}

int ar2VideoSetParami1394( AR2VideoParam1394T *vid, int paramName, int  value )
{
    int    min, max;
    int    ub, vr;
    int    ret;

    if( paramName == AR_VIDEO_1394_BRIGHTNESS ) {
        return ar2VideoSetValue1394( vid, AR_VIDEO_1394_BRIGHTNESS, value );
    }
    else if( paramName == AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON ) {
        return ar2VideoSetFeatureOn1394( vid, AR_VIDEO_1394_BRIGHTNESS, value );
    }
    else if( paramName == AR_VIDEO_1394_BRIGHTNESS_AUTO_ON ) {
        return ar2VideoSetAutoOn1394( vid, AR_VIDEO_1394_BRIGHTNESS, value );
    }
    else if( paramName == AR_VIDEO_1394_EXPOSURE ) {
        return ar2VideoSetValue1394( vid, AR_VIDEO_1394_EXPOSURE, value );
    }
    else if( paramName == AR_VIDEO_1394_EXPOSURE_FEATURE_ON ) {
        return ar2VideoSetFeatureOn1394( vid, AR_VIDEO_1394_EXPOSURE, value );
    }
    else if( paramName == AR_VIDEO_1394_EXPOSURE_AUTO_ON ) {
        return ar2VideoSetAutoOn1394( vid, AR_VIDEO_1394_EXPOSURE, value );
    }
    else if( paramName == AR_VIDEO_1394_SHUTTER_SPEED ) {
        return ar2VideoSetValue1394( vid, AR_VIDEO_1394_SHUTTER_SPEED, value );
    }
    else if( paramName == AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON ) {
        return ar2VideoSetFeatureOn1394( vid, AR_VIDEO_1394_SHUTTER_SPEED, value );
    }
    else if( paramName == AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON ) {
        return ar2VideoSetAutoOn1394( vid, AR_VIDEO_1394_SHUTTER_SPEED, value );
    }
    else if( paramName == AR_VIDEO_1394_GAIN ) {
        return ar2VideoSetValue1394( vid, AR_VIDEO_1394_GAIN, value );
    }
    else if( paramName == AR_VIDEO_1394_GAIN_FEATURE_ON ) {
        return ar2VideoSetFeatureOn1394( vid, AR_VIDEO_1394_GAIN, value );
    }
    else if( paramName == AR_VIDEO_1394_GAIN_AUTO_ON ) {
        return ar2VideoSetAutoOn1394( vid, AR_VIDEO_1394_GAIN, value );
    }
    else if( paramName == AR_VIDEO_1394_FOCUS ) {
        return ar2VideoSetValue1394( vid, AR_VIDEO_1394_FOCUS, value );
    }
    else if( paramName == AR_VIDEO_1394_FOCUS_FEATURE_ON ) {
        return ar2VideoSetFeatureOn1394( vid, AR_VIDEO_1394_FOCUS, value );
    }
    else if( paramName == AR_VIDEO_1394_FOCUS_AUTO_ON ) {
        return ar2VideoSetAutoOn1394( vid, AR_VIDEO_1394_FOCUS, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_UB ) {
        return ar2VideoSetValue1394( vid, AR_VIDEO_1394_WHITE_BALANCE_UB, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_VR ) {
        return ar2VideoSetValue1394( vid, AR_VIDEO_1394_WHITE_BALANCE_VR, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON ) {
        return ar2VideoSetFeatureOn1394( vid, AR_VIDEO_1394_WHITE_BALANCE, value );
    }
    else if( paramName == AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON ) {
        return ar2VideoSetAutoOn1394( vid, AR_VIDEO_1394_WHITE_BALANCE, value );
    }

    return -1;
}

int ar2VideoGetParamd1394( AR2VideoParam1394T *vid, int paramName, double *value )
{
    return -1;
}

int ar2VideoSetParamd1394( AR2VideoParam1394T *vid, int paramName, double  value )
{
    return -1;
}
#endif
