/*
 *  videoV4L.c
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
 *  Author(s): Atsushi Nakazawa, Hirokazu Kato, Philip Lamb
 *
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
#include <time.h>
#include <pthread.h>
#include <linux/types.h>
#include <AR/ar.h>
#include <AR/video.h>



#define MAXCHANNEL   10

static void ar2VideoBufferInitV4L( AR2VideoBufferV4LT *buffer, int size );
static void ar2VideoCaptureV4L(AR2VideoParamV4LT *vid);
static void ar2VideoGetTimeStampV4L(ARUint32 *t_sec, ARUint32 *t_usec);


int ar2VideoDispOptionV4L( void )
{
    ARLOG(" -device=LinuxV4L\n");
    ARLOG("\n");
    ARLOG(" -width=N\n");
    ARLOG("    specifies expected width of image.\n");
    ARLOG(" -height=N\n");
    ARLOG("    specifies expected height of image.\n");
    ARLOG(" -contrast=N\n");
    ARLOG("    specifies contrast. (0.0 <-> 1.0)\n");
    ARLOG(" -brightness=N\n");
    ARLOG("    specifies brightness. (0.0 <-> 1.0)\n");
    ARLOG(" -color=N\n");
    ARLOG("    specifies color. (0.0 <-> 1.0)\n");
    ARLOG(" -hue=N\n");
    ARLOG("    specifies hue. (0.0 <-> 1.0)\n");
    ARLOG(" -whiteness=N\n");
    ARLOG("    specifies whiteness. (0.0 <-> 1.0)\n");
    ARLOG(" -channel=N\n");
    ARLOG("    specifies source channel.\n");
    ARLOG(" -dev=filepath\n");
    ARLOG("    specifies device file.\n");
    ARLOG(" -mode=[PAL|NTSC|SECAM]\n");
    ARLOG("    specifies TV signal mode.\n");
    ARLOG(" -format=[BGR|BGRA]\n");
    ARLOG("    specifies pixel format.\n");
    ARLOG("\n");
    
    return 0;
}

AR2VideoParamV4LT *ar2VideoOpenV4L( const char *config )
{
    AR2VideoParamV4LT        *vid;
    struct video_capability   vd;
    struct video_channel      vc[MAXCHANNEL];
    struct video_picture      vp;
    struct video_window       grab_win;
    const char               *a;
    char                      b[256];
    int                       i;
    
    arMalloc( vid, AR2VideoParamV4LT, 1 );
    strcpy( vid->dev, AR_VIDEO_V4L_DEFAULT_DEVICE );
    vid->width      = AR_VIDEO_V4L_DEFAULT_WIDTH;
    vid->height     = AR_VIDEO_V4L_DEFAULT_HEIGHT;
    vid->channel    = AR_VIDEO_V4L_DEFAULT_CHANNEL;
    vid->mode       = AR_VIDEO_V4L_DEFAULT_MODE;
    vid->format     = AR_INPUT_V4L_DEFAULT_PIXEL_FORMAT;
    vid->debug      = 0;
    vid->contrast   = 0.5;
    vid->brightness = 0.5;
    vid->hue        = 0.5;
    vid->whiteness  = 0.5;
    vid->color      = 0.5;
    
    a = config;
    if( a != NULL) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;
            
            if( sscanf(a, "%s", b) == 0 ) break;
            if( strncmp( b, "-width=", 7 ) == 0 ) {
                if( sscanf( &b[7], "%d", &vid->width ) == 0 ) {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-height=", 8 ) == 0 ) {
                if( sscanf( &b[8], "%d", &vid->height ) == 0 ) {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-contrast=", 10 ) == 0 ) {
                if( sscanf( &b[10], "%lf", &vid->contrast ) == 0 ) {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-brightness=", 12 ) == 0 ) {
                if( sscanf( &b[12], "%lf", &vid->brightness ) == 0 ) {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-hue=", 5 ) == 0 ) {
                if( sscanf( &b[5], "%lf", &vid->hue ) == 0 ) {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-whiteness=", 11 ) == 0 ) {
                if( sscanf( &b[11], "%lf", &vid->whiteness ) == 0 ) {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-color=", 7 ) == 0 ) {
                if( sscanf( &b[7], "%lf", &vid->color ) == 0 ) {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-channel=", 9 ) == 0 ) {
                if( sscanf( &b[9], "%d", &vid->channel ) == 0 ) {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-dev=", 5 ) == 0 ) {
                if( sscanf( &b[5], "%s", vid->dev ) == 0 ) {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-mode=", 6 ) == 0 ) {
                if( strcmp( &b[6], "PAL" ) == 0 )        vid->mode = VIDEO_MODE_PAL;
                else if( strcmp( &b[6], "NTSC" ) == 0 )  vid->mode = VIDEO_MODE_NTSC;
                else if( strcmp( &b[6], "SECAM" ) == 0 ) vid->mode = VIDEO_MODE_SECAM;
                else {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strncmp( b, "-format=", 8 ) == 0 ) {
                if( strcmp( &b[8], "BGR" ) == 0 )        vid->format = AR_PIXEL_FORMAT_BGR;
                else if( strcmp( &b[8], "BGRA" ) == 0 )  vid->format = AR_PIXEL_FORMAT_BGRA;
                else {
                    ar2VideoDispOptionV4L();
                    free( vid );
                    return 0;
                }
            }
            else if( strcmp( b, "-debug" ) == 0 ) {
                vid->debug = 1;
            }
            else if( strcmp( b, "-device=LinuxV4L" ) == 0 )    {
            }
            else {
                ar2VideoDispOptionV4L();
                free( vid );
                return 0;
            }
            
            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }
    
    vid->fd = open(vid->dev, O_RDWR);
    if(vid->fd < 0){
        ARLOGe("video device (%s) open failed\n",vid->dev);
        free( vid );
        return 0;
    }
    
    if(ioctl(vid->fd,VIDIOCGCAP,&vd) < 0){
        ARLOGe("ioctl failed\n");
        free( vid );
        return 0;
    }
    
    if( vid->debug ) {
        ARLOG("=== debug info ===\n");
        ARLOG("  vd.name      =   %s\n",vd.name);
        ARLOG("  vd.channels  =   %d\n",vd.channels);
        ARLOG("  vd.maxwidth  =   %d\n",vd.maxwidth);
        ARLOG("  vd.maxheight =   %d\n",vd.maxheight);
        ARLOG("  vd.minwidth  =   %d\n",vd.minwidth);
        ARLOG("  vd.minheight =   %d\n",vd.minheight);
    }
    
    /* check capture size */
    if(vd.maxwidth  < vid->width  || vid->width  < vd.minwidth ||
       vd.maxheight < vid->height || vid->height < vd.minheight ) {
        ARLOGe("arVideoOpen: width or height oversize \n");
        free( vid );
        return 0;
    }
    
    /* check channel */
    if(vid->channel < 0 || vid->channel >= vd.channels){
        ARLOGe("arVideoOpen: channel# is not valid. \n");
        free( vid );
        return 0;
    }
    
    if( vid->debug ) {
        ARLOG("==== capture device channel info ===\n");
    }
    
    for(i = 0; i < vd.channels && i < MAXCHANNEL; i++){
        vc[i].channel = i;
        if(ioctl(vid->fd,VIDIOCGCHAN,&vc[i]) < 0){
            ARLOGe("error: acquireing channel(%d) info\n",i);
            free( vid );
            return 0;
        }
        
        if( vid->debug ) {
            ARLOG("    channel = %d\n",  vc[i].channel);
            ARLOG("       name = %s\n",  vc[i].name);
            ARLOG("     tuners = %d",    vc[i].tuners);
            
            ARLOG("       flag = 0x%08x",vc[i].flags);
            if(vc[i].flags & VIDEO_VC_TUNER)
                ARLOG(" TUNER");
            if(vc[i].flags & VIDEO_VC_AUDIO)
                ARLOG(" AUDIO");
            ARLOG("\n");
            
            ARLOG("     vc[%d].type = 0x%08x", i, vc[i].type);
            if(vc[i].type & VIDEO_TYPE_TV)
                ARLOG(" TV");
            if(vc[i].type & VIDEO_TYPE_CAMERA)
                ARLOG(" CAMERA");
            ARLOG("\n");
        }
    }
    
    /* select channel */
    vc[vid->channel].norm = vid->mode;       /* 0: PAL 1: NTSC 2:SECAM 3:AUTO */
    if(ioctl(vid->fd, VIDIOCSCHAN, &vc[vid->channel]) < 0){
        ARLOGe("error: selecting channel %d\n", vid->channel);
        free( vid );
        return 0;
    }
    
    /* set video picture */
    vp.brightness = 32767 * 2.0 * vid->brightness;
    vp.hue        = 32767 * 2.0 * vid->hue;
    vp.colour     = 32767 * 2.0 * vid->color;
    vp.contrast   = 32767 * 2.0 * vid->contrast;
    vp.whiteness  = 32767 * 2.0 * vid->whiteness;
    vp.depth      = 24;                      /* color depth    */
    if( vid->format == AR_PIXEL_FORMAT_BGR ) {
        vp.palette    = VIDEO_PALETTE_RGB24;     /* palette format */
    }
    else if( vid->format == AR_PIXEL_FORMAT_BGRA ) {
        vp.palette    = VIDEO_PALETTE_RGB32;     /* palette format */
    }
    else {
        free( vid );
        return 0;
    }
    if(ioctl(vid->fd, VIDIOCSPICT, &vp)) {
        ARLOGe("error: setting palette\n");
        free( vid );
        return 0;
    }
    
    memset( &grab_win, 0, sizeof( struct video_window ) );
    grab_win.width = vid->width;
    grab_win.height = vid->height;
    if( ioctl( vid->fd, VIDIOCSWIN, &grab_win ) < 0 ) {
        ARLOGe("error: VIDIOCSWIN\n");
        free( vid );
        return 0;
    }
    
    /* get mmap info */
    if(ioctl(vid->fd,VIDIOCGMBUF,&vid->vm) < 0){
        ARLOGe("error: videocgmbuf\n");
        free( vid );
        return 0;
    }
    
    if( vid->debug ) {
        ARLOG("===== Image Buffer Info =====\n");
        ARLOG("   size   =  %d[bytes]\n", vid->vm.size);
        ARLOG("   frames =  %d\n", vid->vm.frames);
    }
    if(vid->vm.frames < 2){
        ARLOGe("this device can not be supported by libARvideo.\n");
        ARLOGe("(vm.frames < 2)\n");
        free( vid );
        return 0;
    }
    
    
    /* get memory mapped io */
    if((vid->map = (ARUint8 *)mmap(0, vid->vm.size, PROT_READ|PROT_WRITE, MAP_SHARED, vid->fd, 0)) < 0){
        ARLOGe("error: mmap\n");
        free( vid );
        return 0;
    }
    
    /* setup for vmm */
    vid->vmm.frame  = 0;
    vid->vmm.width  = vid->width;
    vid->vmm.height = vid->height;
    if( vid->format == AR_PIXEL_FORMAT_BGR ) {
        vid->vmm.format = VIDEO_PALETTE_RGB24;
        ar2VideoBufferInitV4L( &(vid->buffer), vid->width*vid->height*3 );
    }
    else if( vid->format == AR_PIXEL_FORMAT_BGRA ) {
        vid->vmm.format = VIDEO_PALETTE_RGB32;
        ar2VideoBufferInitV4L( &(vid->buffer), vid->width*vid->height*4 );
    }
    else {
        free( vid );
        return 0;
    }
    
    vid->status = AR2VIDEO_V4L_STATUS_IDLE;
    pthread_create(&(vid->capture), NULL, (void * (*)(void *))ar2VideoCaptureV4L, vid);
    
    return vid;
}

int ar2VideoCloseV4L( AR2VideoParamV4LT *vid )
{
    vid->status = AR2VIDEO_V4L_STATUS_STOP;
    
    pthread_join( vid->capture, NULL );
    
    free(vid->buffer.in.buff  );
    free(vid->buffer.wait.buff);
    free(vid->buffer.out.buff );
    
    close(vid->fd);
    free( vid );
    
    return 0;
}

int ar2VideoGetIdV4L( AR2VideoParamV4LT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    if( vid == NULL ) return -1;
    
    if (id0) *id0 = 0;
    if (id1) *id1 = 0;
    
    return -1;
}

int ar2VideoGetSizeV4L(AR2VideoParamV4LT *vid, int *x,int *y)
{
    if( vid == NULL ) return -1;
    
    if (x) *x = vid->vmm.width;
    if (y) *y = vid->vmm.height;
    
    return 0;
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatV4L( AR2VideoParamV4LT *vid )
{
    if( vid == NULL ) return AR_PIXEL_FORMAT_INVALID;
    
    return vid->format;
}

int ar2VideoCapStartV4L( AR2VideoParamV4LT *vid )
{
    if(vid->status == AR2VIDEO_V4L_STATUS_RUN){
        ARLOGe("arVideoCapStart has already been called.\n");
        return -1;
    }
    
    vid->status = AR2VIDEO_V4L_STATUS_RUN;
    
    return 0;
}

int ar2VideoCapStopV4L( AR2VideoParamV4LT *vid )
{
    if( vid->status != AR2VIDEO_V4L_STATUS_RUN ) return -1;
    vid->status = AR2VIDEO_V4L_STATUS_IDLE;
    
    return 0;
}

AR2VideoBufferT *ar2VideoGetImageV4L( AR2VideoParamV4LT *vid )
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

static void ar2VideoCaptureV4L(AR2VideoParamV4LT *vid)
{
    AR2VideoBufferT   tmp;
    ARUint8          *img;
    int               pixSize;
    
    if( vid->format == AR_PIXEL_FORMAT_BGR )       pixSize = 3;
    else if( vid->format == AR_PIXEL_FORMAT_BGRA ) pixSize = 4;
    else return;
    
    vid->video_cont_num = 0;
    vid->vmm.frame = vid->video_cont_num;
    if(ioctl(vid->fd, VIDIOCMCAPTURE, &vid->vmm) < 0) {
        ARLOGe("error: videocapture\n");
        return;
    }
    vid->vmm.frame = 1 - vid->vmm.frame;
    if( ioctl(vid->fd, VIDIOCMCAPTURE, &vid->vmm) < 0) {
        ARLOGe("error: videocapture\n");
        return;
    }
    
    while(vid->status != AR2VIDEO_V4L_STATUS_STOP) {
        if( vid->status == AR2VIDEO_V4L_STATUS_RUN ) {
            if(ioctl(vid->fd, VIDIOCSYNC, &vid->video_cont_num) < 0){
                ARLOGe("error: videosync\n");
                return;
            }
            img = vid->map + vid->vm.offsets[vid->video_cont_num];
            
            ar2VideoGetTimeStampV4L( &(vid->buffer.in.time_sec), &(vid->buffer.in.time_usec) );
            vid->buffer.in.fillFlag = 1;
            vid->buffer.in.buffLuma = NULL;
            memcpy(vid->buffer.in.buff, img, vid->width*vid->height*pixSize);
            
            pthread_mutex_lock(&(vid->buffer.mutex));
            tmp = vid->buffer.wait;
            vid->buffer.wait = vid->buffer.in;
            vid->buffer.in = tmp;
            pthread_mutex_unlock(&(vid->buffer.mutex));
            
            vid->vmm.frame = vid->video_cont_num;
            ioctl(vid->fd, VIDIOCMCAPTURE, &vid->vmm);
            vid->video_cont_num = 1 - vid->video_cont_num;
        }
        else if( vid->status == AR2VIDEO_V4L_STATUS_IDLE ) usleep(100);
    }
    
    if(ioctl(vid->fd, VIDIOCSYNC, &vid->video_cont_num) < 0){
        ARLOGe("error: videosync\n");
        return;
    }
    
    return;
}

static void ar2VideoBufferInitV4L( AR2VideoBufferV4LT *buffer, int size )
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

static void ar2VideoGetTimeStampV4L(ARUint32 *t_sec, ARUint32 *t_usec)
{
    struct timeval     time;
    double             tt;
    int                s1, s2;
    
    gettimeofday( &time, NULL );
    *t_sec  = time.tv_sec;
    *t_usec = time.tv_usec;
    
    return;
}

int ar2VideoGetParamiV4L( AR2VideoParamV4LT *vid, int paramName, int *value )
{
    return -1;
}
int ar2VideoSetParamiV4L( AR2VideoParamV4LT *vid, int paramName, int  value )
{
    return -1;
}
int ar2VideoGetParamdV4L( AR2VideoParamV4LT *vid, int paramName, double *value )
{
    return -1;
}
int ar2VideoSetParamdV4L( AR2VideoParamV4LT *vid, int paramName, double  value )
{
    return -1;
}
