/*
 *  video2.c
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/* 
 *   author: Hirokazu Kato ( kato@sys.im.hiroshima-cu.ac.jp )
 *
 *   Revision: 6.0   Date: 2003/09/29
 */

#include <stdio.h>
#include <string.h>
#include <AR/video.h>
#include <AR/config.h>

static const char *ar2VideoGetConfig(const char *config_in)
{
    const char *config = NULL;
    
    /* If no config string is supplied, we should use the environment variable, otherwise set a sane default */
    if (!config_in || !(config_in[0])) {
        /* None supplied, lets see if the user supplied one from the shell */
#ifndef _WINRT
        char *envconf = getenv("ARTOOLKIT5_VCONF");
        if (envconf && envconf[0]) {
            config = envconf;
            ARLOGi("Using video config from environment \"%s\".\n", envconf);
        } else {
#endif // !_WINRT
            config = NULL;
            ARLOGi("Using default video config.\n");
#ifndef _WINRT
        }
#endif // !_WINRT
    } else {
        config = config_in;
        ARLOGi("Using supplied video config \"%s\".\n", config_in);
    }
    
    return config;
}

static int ar2VideoGetDeviceWithConfig(const char *config, const char **configStringFollowingDevice_p)
{
    int                        device;
    const char                *a;
    char                       b[256];
    
    device = arVideoGetDefaultDevice();
    
    if (configStringFollowingDevice_p) *configStringFollowingDevice_p = NULL;
    
    a = config;
    if (a) {
        for(;;) {
            while( *a == ' ' || *a == '\t' ) a++;
            if( *a == '\0' ) break;
            
            if( sscanf(a, "%s", b) == 0 ) break;
            
            if( strcmp( b, "-device=Dummy" ) == 0 )             {
                device = AR_VIDEO_DEVICE_DUMMY;
            }
            else if( strcmp( b, "-device=LinuxV4L" ) == 0 )     {
                device = AR_VIDEO_DEVICE_V4L;
            }
            else if( strcmp( b, "-device=LinuxV4L2" ) == 0 )     {
                device = AR_VIDEO_DEVICE_V4L2;
            }
            else if( strcmp( b, "-device=LinuxDV" ) == 0 )      {
                device = AR_VIDEO_DEVICE_DV;
            }
            else if( strcmp( b, "-device=Linux1394Cam" ) == 0 ) {
                device = AR_VIDEO_DEVICE_1394CAM;
            }
            else if( strcmp( b, "-device=SGI" ) == 0 )          {
                device = AR_VIDEO_DEVICE_SGI;
            }
            else if( strcmp( b, "-device=WinDS" ) == 0 )      {
                device = AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW;
            }
            else if( strcmp( b, "-device=WinDF" ) == 0 )      {
                device = AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY;
            }
            else if( strcmp( b, "-device=WinDSVL" ) == 0 )      {
                device = AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB;
                if (configStringFollowingDevice_p) *configStringFollowingDevice_p = a;
            }
            else if( strcmp( b, "-device=QUICKTIME" ) == 0 )    {
                device = AR_VIDEO_DEVICE_QUICKTIME;
            }
            else if( strcmp( b, "-device=GStreamer" ) == 0 )    {
                device = AR_VIDEO_DEVICE_GSTREAMER;
                if (configStringFollowingDevice_p) *configStringFollowingDevice_p = a;
            }
            else if( strcmp( b, "-device=iPhone" ) == 0 )    {
                device = AR_VIDEO_DEVICE_IPHONE;
            }
            else if( strcmp( b, "-device=QuickTime7" ) == 0 )    {
                device = AR_VIDEO_DEVICE_QUICKTIME7;
            }
            else if( strcmp( b, "-device=Image" ) == 0 )    {
                device = AR_VIDEO_DEVICE_IMAGE;
            }
            else if( strcmp( b, "-device=Android" ) == 0 )    {
                device = AR_VIDEO_DEVICE_ANDROID;
            }
            else if( strcmp( b, "-device=WinMF" ) == 0 )    {
                device = AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION;
            }
            else if( strcmp( b, "-device=WinMC" ) == 0 )    {
                device = AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE;
            }
            
            while( *a != ' ' && *a != '\t' && *a != '\0') a++;
        }
    }
    
    if (configStringFollowingDevice_p) {
        if (*configStringFollowingDevice_p) {
            while( **configStringFollowingDevice_p != ' ' && **configStringFollowingDevice_p != '\t' && **configStringFollowingDevice_p != '\0') (*configStringFollowingDevice_p)++;
            while( **configStringFollowingDevice_p == ' ' || **configStringFollowingDevice_p == '\t') (*configStringFollowingDevice_p)++;
        } else {
            *configStringFollowingDevice_p = config;
        }
    }
    
    return (device);
}

ARVideoSourceInfoListT *ar2VideoCreateSourceInfoList(const char *config_in)
{
    int device = ar2VideoGetDeviceWithConfig(ar2VideoGetConfig(config_in), NULL);
#ifdef AR_INPUT_DUMMY
    if (device == AR_VIDEO_DEVICE_DUMMY) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_V4L
    if (device == AR_VIDEO_DEVICE_V4L) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_V4L2
    if (device == AR_VIDEO_DEVICE_V4L2) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_DV
    if (device == AR_VIDEO_DEVICE_DV) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_1394CAM
    if (device == AR_VIDEO_DEVICE_1394CAM) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if (device == AR_VIDEO_DEVICE_GSTREAMER) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_SGI
    if (device == AR_VIDEO_DEVICE_SGI) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if (device == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW) {
		return ar2VideoCreateSourceInfoListWinDS(config_in);
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if (device == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if (device == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if (device == AR_VIDEO_DEVICE_QUICKTIME) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_IPHONE
    if (device == AR_VIDEO_DEVICE_IPHONE) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if (device == AR_VIDEO_DEVICE_QUICKTIME7) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_IMAGE
    if (device == AR_VIDEO_DEVICE_IMAGE) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_ANDROID
    if (device == AR_VIDEO_DEVICE_ANDROID) {
        return (NULL);
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if (device == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION) {
        return ar2VideoCreateSourceInfoListWinMF(config_in);
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if (device == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE) {
        return (NULL);
    }
#endif
    return (NULL);
}

void ar2VideoDeleteSourceInfoList(ARVideoSourceInfoListT **p)
{
    int i;
    
    if (!p || !*p) return;
    
    for (i = 0; i < (*p)->count; i++) {
        free((*p)->info[i].name);
        free((*p)->info[i].UID);
    }
    free((*p)->info);
    free(*p);
    
    *p = NULL;
}

AR2VideoParamT *ar2VideoOpen( const char *config_in )
{
    AR2VideoParamT            *vid;
    const char                *config;
    // Some devices won't understand the "-device=" option, so we need to pass
    // only the portion following that option to them.
    const char                *configStringFollowingDevice = NULL;

    arMalloc( vid, AR2VideoParamT, 1 );
    config = ar2VideoGetConfig(config_in);
    vid->deviceType = ar2VideoGetDeviceWithConfig(config, &configStringFollowingDevice);

    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
#ifdef AR_INPUT_DUMMY
        if( (vid->device.dummy = ar2VideoOpenDummy(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"Dummy\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
#ifdef AR_INPUT_V4L
        if( (vid->device.v4l = ar2VideoOpenV4L(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"LinuxV4L\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
#ifdef AR_INPUT_V4L2
        if( (vid->device.v4l2 = ar2VideoOpenV4L2(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"LinuxV4L2\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
#ifdef AR_INPUT_DV
        if( (vid->device.dv = ar2VideoOpenDv(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"LinuxDV\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
#ifdef AR_INPUT_1394CAM
        if( (vid->device.cam1394 = ar2VideoOpen1394(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"Linux1394Cam\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
#ifdef AR_INPUT_GSTREAMER
        if( (vid->device.gstreamer = ar2VideoOpenGStreamer(configStringFollowingDevice)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"GStreamer\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
#ifdef AR_INPUT_SGI
        if( (vid->device.sgi = ar2VideoOpenSGI(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"SGI\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
        if( (vid->device.winDS = ar2VideoOpenWinDS(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"WinDS\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
        if( (vid->device.winDSVL = ar2VideoOpenWinDSVL(configStringFollowingDevice)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"WinDSVL\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
        if( (vid->device.winDF = ar2VideoOpenWinDF(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"WinDF\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
#ifdef AR_INPUT_QUICKTIME
        if( (vid->device.quickTime = ar2VideoOpenQuickTime(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"QUICKTIME\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
#ifdef AR_INPUT_IPHONE
        if ((vid->device.iPhone = ar2VideoOpeniPhone(config)) != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"iPhone\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
#ifdef AR_INPUT_QUICKTIME7
        if( (vid->device.quickTime7 = ar2VideoOpenQuickTime7(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"QuickTime7\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
#ifdef AR_INPUT_IMAGE
        if( (vid->device.image = ar2VideoOpenImage(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"Image\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
#ifdef AR_INPUT_ANDROID
        if( (vid->device.android = ar2VideoOpenAndroid(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"Android\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
        if( (vid->device.winMF = ar2VideoOpenWinMF(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"WinMF\" not supported on this build/architecture/system.\n");
#endif
    }
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
        if( (vid->device.winMC = ar2VideoOpenWinMC(config)) != NULL ) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"WinMC\" not supported on this build/architecture/system.\n");
#endif
    }
    
    free( vid );
    return NULL;
}

AR2VideoParamT *ar2VideoOpenAsync(const char *config_in, void (*callback)(void *), void *userdata)
{
    AR2VideoParamT            *vid;
    const char                *config;
    // Some devices won't understand the "-device=" option, so we need to pass
    // only the portion following that option to them.
    const char                *configStringFollowingDevice = NULL;
    
    arMalloc( vid, AR2VideoParamT, 1 );
    config = ar2VideoGetConfig(config_in);
    vid->deviceType = ar2VideoGetDeviceWithConfig(config, &configStringFollowingDevice);
    
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
#ifdef AR_INPUT_IPHONE
        if (callback) vid->device.iPhone = ar2VideoOpenAsynciPhone(config, callback, userdata);
        else vid->device.iPhone = NULL;
        
        if (vid->device.iPhone != NULL) return vid;
#else
        ARLOGe("ar2VideoOpen: Error: device \"iPhone\" not supported on this build/architecture/system.\n");
#endif
    }

    free( vid );
    return NULL;
}

int ar2VideoClose( AR2VideoParamT *vid )
{
    int ret;
    
    if (!vid) return -1;
    ret = -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        ret = ar2VideoCloseDummy( vid->device.dummy );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        ret = ar2VideoCloseV4L( vid->device.v4l );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        ret = ar2VideoCloseV4L2( vid->device.v4l2 );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        ret = ar2VideoCloseDv( vid->device.dv );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        ret = ar2VideoClose1394( vid->device.cam1394 );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        ret = ar2VideoCloseGStreamer( vid->device.gstreamer );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        ret = ar2VideoCloseSGI( vid->device.sgi );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        ret = ar2VideoCloseWinDS( vid->device.winDS );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        ret = ar2VideoCloseWinDSVL( vid->device.winDSVL );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        ret = ar2VideoCloseWinDF( vid->device.winDF );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        ret = ar2VideoCloseQuickTime( vid->device.quickTime );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        ret = ar2VideoCloseiPhone( vid->device.iPhone );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        ret = ar2VideoCloseQuickTime7( vid->device.quickTime7 );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        ret = ar2VideoCloseImage( vid->device.image );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        ret = ar2VideoCloseAndroid( vid->device.android );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        ret = ar2VideoCloseWinMF( vid->device.winMF );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        ret = ar2VideoCloseWinMC( vid->device.winMC );
    }
#endif
    free (vid);
    return (ret);
} 

int ar2VideoDispOption( AR2VideoParamT *vid )
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoDispOptionDummy();
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoDispOptionV4L();
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoDispOptionV4L2();
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoDispOptionDv();
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoDispOption1394();
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoDispOptionGStreamer();
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoDispOptionSGI();
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoDispOptionWinDS();
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoDispOptionWinDSVL();
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoDispOptionWinDF();
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoDispOptionQuickTime();
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoDispOptioniPhone();
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoDispOptionQuickTime7();
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoDispOptionImage();
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoDispOptionAndroid();
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoDispOptionWinMF();
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoDispOptionWinMC();
    }
#endif
    return (-1);
}

int ar2VideoGetDevice( AR2VideoParamT *vid )
{
    if (!vid) return -1;
    return vid->deviceType;
}

int ar2VideoGetId( AR2VideoParamT *vid, ARUint32 *id0, ARUint32 *id1 )
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoGetIdDummy( vid->device.dummy, id0, id1 );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoGetIdV4L( vid->device.v4l, id0, id1 );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoGetIdV4L2( vid->device.v4l2, id0, id1 );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoGetIdDv( vid->device.dv, id0, id1 );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoGetId1394( vid->device.cam1394, id0, id1 );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoGetIdGStreamer( vid->device.gstreamer, id0, id1 );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoGetIdSGI( vid->device.sgi, id0, id1 );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoGetIdWinDS( vid->device.winDS, id0, id1 );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoGetIdWinDSVL( vid->device.winDSVL, id0, id1 );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoGetIdWinDF( vid->device.winDF, id0, id1 );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoGetIdQuickTime( vid->device.quickTime, id0, id1 );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoGetIdiPhone( vid->device.iPhone, id0, id1 );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoGetIdQuickTime7( vid->device.quickTime7, id0, id1 );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoGetIdImage( vid->device.image, id0, id1 );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoGetIdAndroid( vid->device.android, id0, id1 );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoGetIdWinMF( vid->device.winMF, id0, id1 );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoGetIdWinMC( vid->device.winMC, id0, id1 );
    }
#endif
    return (-1);
}

int ar2VideoGetSize(AR2VideoParamT *vid, int *x,int *y)
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoGetSizeDummy( vid->device.dummy, x, y );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoGetSizeV4L( vid->device.v4l, x, y );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoGetSizeV4L2( vid->device.v4l2, x, y );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoGetSizeDv( vid->device.dv, x, y );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoGetSize1394( vid->device.cam1394, x, y );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoGetSizeGStreamer( vid->device.gstreamer, x, y );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoGetSizeSGI( vid->device.sgi, x, y );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoGetSizeWinDS( vid->device.winDS, x, y );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoGetSizeWinDSVL( vid->device.winDSVL, x, y );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoGetSizeWinDF( vid->device.winDF, x, y );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoGetSizeQuickTime( vid->device.quickTime, x, y );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoGetSizeiPhone( vid->device.iPhone, x, y );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoGetSizeQuickTime7( vid->device.quickTime7, x, y );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoGetSizeImage( vid->device.image, x, y );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoGetSizeAndroid( vid->device.android, x, y );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoGetSizeWinMF( vid->device.winMF, x, y );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoGetSizeWinMC( vid->device.winMC, x, y );
    }
#endif
    return (-1);
}

int ar2VideoGetPixelSize( AR2VideoParamT *vid )
{
    return (arVideoUtilGetPixelSize(ar2VideoGetPixelFormat(vid)));
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormat( AR2VideoParamT *vid )
{
    if (!vid) return (AR_PIXEL_FORMAT_INVALID);
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoGetPixelFormatDummy( vid->device.dummy );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoGetPixelFormatV4L( vid->device.v4l );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoGetPixelFormatV4L2( vid->device.v4l2 );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoGetPixelFormatDv( vid->device.dv );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoGetPixelFormat1394( vid->device.cam1394 );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoGetPixelFormatGStreamer( vid->device.gstreamer );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoGetPixelFormatSGI( vid->device.sgi );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoGetPixelFormatWinDS( vid->device.winDS );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoGetPixelFormatWinDSVL( vid->device.winDSVL );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoGetPixelFormatWinDF( vid->device.winDF );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoGetPixelFormatQuickTime( vid->device.quickTime );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoGetPixelFormatiPhone( vid->device.iPhone );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoGetPixelFormatQuickTime7( vid->device.quickTime7 );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoGetPixelFormatImage( vid->device.image );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoGetPixelFormatAndroid( vid->device.android );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoGetPixelFormatWinMF( vid->device.winMF );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoGetPixelFormatWinMC( vid->device.winMC );
    }
#endif
    return (AR_PIXEL_FORMAT_INVALID);
}

AR2VideoBufferT *ar2VideoGetImage( AR2VideoParamT *vid )
{
    if (!vid) return (NULL);
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoGetImageDummy( vid->device.dummy );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoGetImageV4L( vid->device.v4l );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoGetImageV4L2( vid->device.v4l2 );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoGetImageDv( vid->device.dv );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoGetImage1394( vid->device.cam1394 );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoGetImageGStreamer( vid->device.gstreamer );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoGetImageSGI( vid->device.sgi );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoGetImageWinDS( vid->device.winDS );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoGetImageWinDSVL( vid->device.winDSVL );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoGetImageWinDF( vid->device.winDF );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoGetImageQuickTime( vid->device.quickTime );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoGetImageiPhone( vid->device.iPhone );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoGetImageQuickTime7( vid->device.quickTime7 );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoGetImageImage( vid->device.image );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
#  if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
        return ar2VideoGetImageAndroid( vid->device.android );
#  else
        return (NULL); // NOT IMPLEMENTED.
#  endif
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoGetImageWinMF( vid->device.winMF );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoGetImageWinMC( vid->device.winMC );
    }
#endif
    return (NULL);
}

int ar2VideoCapStart( AR2VideoParamT *vid )
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoCapStartDummy( vid->device.dummy );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoCapStartV4L( vid->device.v4l );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoCapStartV4L2( vid->device.v4l2 );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoCapStartDv( vid->device.dv );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoCapStart1394( vid->device.cam1394 );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoCapStartGStreamer( vid->device.gstreamer );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoCapStartSGI( vid->device.sgi );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoCapStartWinDS( vid->device.winDS );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoCapStartWinDSVL( vid->device.winDSVL );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoCapStartWinDF( vid->device.winDF );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoCapStartQuickTime( vid->device.quickTime );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoCapStartiPhone( vid->device.iPhone );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoCapStartQuickTime7( vid->device.quickTime7 );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoCapStartImage( vid->device.image );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
#  if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
        return ar2VideoCapStartAndroid( vid->device.android );
#  else
        return (-1); // NOT IMPLEMENTED.
#  endif
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoCapStartWinMF( vid->device.winMF );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoCapStartWinMC( vid->device.winMC );
    }
#endif
    return (-1);
}

int ar2VideoCapStartAsync (AR2VideoParamT *vid, AR_VIDEO_FRAME_READY_CALLBACK callback, void *userdata)
{
    if (!vid) return -1;
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
#  if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
        return ar2VideoCapStartAsyncAndroid( vid->device.android, callback, userdata );
#  else
        return (-1); // NOT IMPLEMENTED.
#  endif
    }
#endif
    return (-1);
}

int ar2VideoCapStop( AR2VideoParamT *vid )
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoCapStopDummy( vid->device.dummy );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoCapStopV4L( vid->device.v4l );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoCapStopV4L2( vid->device.v4l2 );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoCapStopDv( vid->device.dv );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoCapStop1394( vid->device.cam1394 );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoCapStopGStreamer( vid->device.gstreamer );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoCapStopSGI( vid->device.sgi );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoCapStopWinDS( vid->device.winDS );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoCapStopWinDSVL( vid->device.winDSVL );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoCapStopWinDF( vid->device.winDF );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoCapStopQuickTime( vid->device.quickTime );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoCapStopiPhone( vid->device.iPhone );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoCapStopQuickTime7( vid->device.quickTime7 );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoCapStopImage( vid->device.image );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
#  if AR_VIDEO_ANDROID_ENABLE_NATIVE_CAMERA
		return ar2VideoCapStopAndroid( vid->device.android );
#  else
        return (-1); // NOT IMPLEMENTED.
#  endif
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoCapStopWinMF( vid->device.winMF );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoCapStopWinMC( vid->device.winMC );
    }
#endif
    return (-1);
}

int ar2VideoGetParami( AR2VideoParamT *vid, int paramName, int *value )
{
    if (paramName == AR_VIDEO_GET_VERSION) {
#if (AR_HEADER_VERSION_MAJOR >= 10)
        return (-1);
#else
        return (0x01000000 * ((unsigned int)AR_HEADER_VERSION_MAJOR) +
                0x00100000 * ((unsigned int)AR_HEADER_VERSION_MINOR / 10) +
                0x00010000 * ((unsigned int)AR_HEADER_VERSION_MINOR % 10) +
                0x00001000 * ((unsigned int)AR_HEADER_VERSION_TINY / 10) +
                0x00000100 * ((unsigned int)AR_HEADER_VERSION_TINY % 10) +
                0x00000010 * ((unsigned int)AR_HEADER_VERSION_BUILD / 10) +
                0x00000001 * ((unsigned int)AR_HEADER_VERSION_BUILD % 10)
                );
#endif
    }
    
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoGetParamiDummy( vid->device.dummy, paramName, value );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoGetParamiV4L( vid->device.v4l, paramName, value );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoGetParamiV4L2( vid->device.v4l2, paramName, value );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoGetParamiDv( vid->device.dv, paramName, value );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoGetParami1394( vid->device.cam1394, paramName, value );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoGetParamiGStreamer( vid->device.gstreamer, paramName, value );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoGetParamiSGI( vid->device.sgi, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoGetParamiWinDS( vid->device.winDS, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoGetParamiWinDSVL( vid->device.winDSVL, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoGetParamiWinDF( vid->device.winDF, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoGetParamiQuickTime( vid->device.quickTime, paramName, value );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoGetParamiiPhone( vid->device.iPhone, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoGetParamiQuickTime7( vid->device.quickTime7, paramName, value );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoGetParamiImage( vid->device.image, paramName, value );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoGetParamiAndroid( vid->device.android, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoGetParamiWinMF( vid->device.winMF, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoGetParamiWinMC( vid->device.winMC, paramName, value );
    }
#endif
    return (-1);
}

int ar2VideoSetParami( AR2VideoParamT *vid, int paramName, int  value )
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoSetParamiDummy( vid->device.dummy, paramName, value );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoSetParamiV4L( vid->device.v4l, paramName, value );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoSetParamiV4L2( vid->device.v4l2, paramName, value );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoSetParamiDv( vid->device.dv, paramName, value );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoSetParami1394( vid->device.cam1394, paramName, value );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoSetParamiGStreamer( vid->device.gstreamer, paramName, value );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoSetParamiSGI( vid->device.sgi, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoSetParamiWinDS( vid->device.winDS, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoSetParamiWinDSVL( vid->device.winDSVL, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoSetParamiWinDF( vid->device.winDF, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoSetParamiQuickTime( vid->device.quickTime, paramName, value );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoSetParamiiPhone( vid->device.iPhone, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoSetParamiQuickTime7( vid->device.quickTime7, paramName, value );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoSetParamiImage( vid->device.image, paramName, value );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoSetParamiAndroid( vid->device.android, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoSetParamiWinMF( vid->device.winMF, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoSetParamiWinMC( vid->device.winMC, paramName, value );
    }
#endif
    return (-1);
}

int ar2VideoGetParamd( AR2VideoParamT *vid, int paramName, double *value )
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoGetParamdDummy( vid->device.dummy, paramName, value );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoGetParamdV4L( vid->device.v4l, paramName, value );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoGetParamdV4L2( vid->device.v4l2, paramName, value );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoGetParamdDv( vid->device.dv, paramName, value );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoGetParamd1394( vid->device.cam1394, paramName, value );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoGetParamdGStreamer( vid->device.gstreamer, paramName, value );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoGetParamdSGI( vid->device.sgi, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoGetParamdWinDS( vid->device.winDS, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoGetParamdWinDSVL( vid->device.winDSVL, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoGetParamdWinDF( vid->device.winDF, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoGetParamdQuickTime( vid->device.quickTime, paramName, value );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoGetParamdiPhone( vid->device.iPhone, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoGetParamdQuickTime7( vid->device.quickTime7, paramName, value );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoGetParamdImage( vid->device.image, paramName, value );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoGetParamdAndroid( vid->device.android, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoGetParamdWinMF( vid->device.winMF, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoGetParamdWinMC( vid->device.winMC, paramName, value );
    }
#endif
    return (-1);
}

int ar2VideoSetParamd( AR2VideoParamT *vid, int paramName, double  value )
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoSetParamdDummy( vid->device.dummy, paramName, value );
    }
#endif
#ifdef AR_INPUT_V4L
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L ) {
        return ar2VideoSetParamdV4L( vid->device.v4l, paramName, value );
    }
#endif
#ifdef AR_INPUT_V4L2
    if( vid->deviceType == AR_VIDEO_DEVICE_V4L2 ) {
        return ar2VideoSetParamdV4L2( vid->device.v4l2, paramName, value );
    }
#endif
#ifdef AR_INPUT_DV
    if( vid->deviceType == AR_VIDEO_DEVICE_DV ) {
        return ar2VideoSetParamdDv( vid->device.dv, paramName, value );
    }
#endif
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoSetParamd1394( vid->device.cam1394, paramName, value );
    }
#endif
#ifdef AR_INPUT_GSTREAMER
    if( vid->deviceType == AR_VIDEO_DEVICE_GSTREAMER ) {
        return ar2VideoSetParamdGStreamer( vid->device.gstreamer, paramName, value );
    }
#endif
#ifdef AR_INPUT_SGI
    if( vid->deviceType == AR_VIDEO_DEVICE_SGI ) {
        return ar2VideoSetParamdSGI( vid->device.sgi, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DIRECTSHOW
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DIRECTSHOW ) {
        return ar2VideoSetParamdWinDS( vid->device.winDS, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DSVIDEOLIB
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DSVIDEOLIB ) {
        return ar2VideoSetParamdWinDSVL( vid->device.winDSVL, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_DRAGONFLY
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_DRAGONFLY ) {
        return ar2VideoSetParamdWinDF( vid->device.winDF, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME ) {
        return ar2VideoSetParamdQuickTime( vid->device.quickTime, paramName, value );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoSetParamdiPhone( vid->device.iPhone, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoSetParamdQuickTime7( vid->device.quickTime7, paramName, value );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoSetParamdImage( vid->device.image, paramName, value );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoSetParamdAndroid( vid->device.android, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoSetParamdWinMF( vid->device.winMF, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoSetParamdWinMC( vid->device.winMC, paramName, value );
    }
#endif
    return (-1);
}


int ar2VideoGetParams( AR2VideoParamT *vid, const int paramName, char **value )
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoGetParamsDummy( vid->device.dummy, paramName, value );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoGetParamsiPhone( vid->device.iPhone, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoGetParamsQuickTime7(vid->device.quickTime7, paramName, value );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoGetParamsImage( vid->device.image, paramName, value );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoGetParamsAndroid( vid->device.android, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoGetParamsWinMF( vid->device.winMF, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoGetParamsWinMC( vid->device.winMC, paramName, value );
    }
#endif
    return (-1);
}

int ar2VideoSetParams( AR2VideoParamT *vid, const int paramName, const char  *value )
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoSetParamsDummy( vid->device.dummy, paramName, value );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoSetParamsiPhone( vid->device.iPhone, paramName, value );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        return ar2VideoSetParamsQuickTime7( vid->device.quickTime7, paramName, value );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoSetParamsImage( vid->device.image, paramName, value );
    }
#endif
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoSetParamsAndroid( vid->device.android, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_FOUNDATION
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_FOUNDATION ) {
        return ar2VideoSetParamsWinMF( vid->device.winMF, paramName, value );
    }
#endif
#ifdef AR_INPUT_WINDOWS_MEDIA_CAPTURE
    if( vid->deviceType == AR_VIDEO_DEVICE_WINDOWS_MEDIA_CAPTURE ) {
        return ar2VideoSetParamsWinMC( vid->device.winMC, paramName, value );
    }
#endif
    return (-1);
}

int ar2VideoSaveParam( AR2VideoParamT *vid, char *filename )
{
    if (!vid) return -1;
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoSaveParam1394( vid->device.cam1394, filename );
    }
#endif
    return (-1);
}

int ar2VideoLoadParam( AR2VideoParamT *vid, char *filename )
{
    if (!vid) return -1;
#ifdef AR_INPUT_1394CAM
    if( vid->deviceType == AR_VIDEO_DEVICE_1394CAM ) {
        return ar2VideoLoadParam1394( vid->device.cam1394, filename );
    }
#endif
    return (-1);
}

int ar2VideoSetBufferSize(AR2VideoParamT *vid, const int width, const int height)
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoSetBufferSizeDummy( vid->device.dummy, width, height );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoSetBufferSizeiPhone( vid->device.iPhone, width, height );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        //return ar2VideoSetBufferSizeQuickTime7( vid->device.quickTime7, width, height );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoSetBufferSizeImage( vid->device.image, width, height );
    }
#endif
    return (-1);
}

int ar2VideoGetBufferSize(AR2VideoParamT *vid, int *width, int *height)
{
    if (!vid) return -1;
#ifdef AR_INPUT_DUMMY
    if( vid->deviceType == AR_VIDEO_DEVICE_DUMMY ) {
        return ar2VideoGetBufferSizeDummy( vid->device.dummy, width, height );
    }
#endif
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoGetBufferSizeiPhone( vid->device.iPhone, width, height );
    }
#endif
#ifdef AR_INPUT_QUICKTIME7
    if( vid->deviceType == AR_VIDEO_DEVICE_QUICKTIME7 ) {
        //return ar2VideoGetBufferSizeQuickTime7( vid->device.quickTime7, width, height );
    }
#endif
#ifdef AR_INPUT_IMAGE
    if( vid->deviceType == AR_VIDEO_DEVICE_IMAGE ) {
        return ar2VideoGetBufferSizeImage( vid->device.image, width, height );
    }
#endif
    return (-1);
}

int ar2VideoGetCParam(AR2VideoParamT *vid, ARParam *cparam)
{
    if (!vid) return -1;
#ifdef AR_INPUT_IPHONE
    if( vid->deviceType == AR_VIDEO_DEVICE_IPHONE ) {
        return ar2VideoGetCParamiPhone( vid->device.iPhone, cparam );
    }
#endif
    return (-1);
}

int ar2VideoGetCParamAsync(AR2VideoParamT *vid, void (*callback)(const ARParam *, void *), void *userdata)
{
    if (!vid) return -1;
#ifdef AR_INPUT_ANDROID
    if( vid->deviceType == AR_VIDEO_DEVICE_ANDROID ) {
        return ar2VideoGetCParamAsyncAndroid( vid->device.android, callback, userdata);
    }
#endif
    return (-1);
}

