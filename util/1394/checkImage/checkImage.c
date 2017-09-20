/*
 *  checkImage.c
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
 *  Copyright 2006-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
 
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif
#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR/video.h>

#define             PATT_NAME        "../share/checkImage/Data/hiro.patt"

AR_PIXEL_FORMAT     pixFormat;
ARParam             cparam;
ARParamLT          *cparamLT;
ARHandle           *arHandle;
ARPattHandle       *arPattHandle;
AR3DHandle         *ar3DHandle;
ARGViewportHandle  *vp;
ARUint32            id0, id1;
int                 debug = 0;
int                 patt_id;
double              patt_width = 80.0;
int                 count;
char                fps[256];

static void   init(int argc, char *argv[]);
static void   cleanup(void);
static void   mainLoop(void);
static void   draw( double trans[3][4] );
static void   keyEvent( unsigned char key, int x, int y);


main(int argc, char *argv[])
{
    init(argc, argv);

    arVideoCapStart();
    argSetDispFunc( mainLoop, 1 );
    argSetKeyFunc( keyEvent );
    count = 0;
    fps[0] = '\0';
    arUtilTimer();
    argMainLoop();
}

static void   keyEvent( unsigned char key, int x, int y)
{
    int     value;
    char    filename[512];

    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        cleanup();
        exit(0);
    }

    if( key == '1' ) {
        arVideoSetParami( AR_VIDEO_1394_EXPOSURE_FEATURE_ON, 1 );
        arVideoSetParami( AR_VIDEO_1394_EXPOSURE_AUTO_ON, 0 );
        arVideoGetParami( AR_VIDEO_1394_EXPOSURE, &value );
        value--;
        arVideoSetParami( AR_VIDEO_1394_EXPOSURE, value );
        arVideoGetParami( AR_VIDEO_1394_EXPOSURE, &value );
        ARLOG("Exposure: %d\n", value);
    }
    if( key == '2' ) {
        arVideoSetParami( AR_VIDEO_1394_EXPOSURE_FEATURE_ON, 1 );
        arVideoSetParami( AR_VIDEO_1394_EXPOSURE_AUTO_ON, 0 );
        arVideoGetParami( AR_VIDEO_1394_EXPOSURE, &value );
        value++;
        arVideoSetParami( AR_VIDEO_1394_EXPOSURE, value );
        arVideoGetParami( AR_VIDEO_1394_EXPOSURE, &value );
        ARLOG("Exposure: %d\n", value);
    }

    if( key == '3' ) {
        arVideoSetParami( AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON, 1 );
        arVideoSetParami( AR_VIDEO_1394_BRIGHTNESS_AUTO_ON, 0 );
        arVideoGetParami( AR_VIDEO_1394_BRIGHTNESS, &value );
        value--;
        arVideoSetParami( AR_VIDEO_1394_BRIGHTNESS, value );
        arVideoGetParami( AR_VIDEO_1394_BRIGHTNESS, &value );
        ARLOG("Brightness: %d\n", value);
    }
    if( key == '4' ) {
        arVideoSetParami( AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON, 1 );
        arVideoSetParami( AR_VIDEO_1394_BRIGHTNESS_AUTO_ON, 0 );
        arVideoGetParami( AR_VIDEO_1394_BRIGHTNESS, &value );
        value++;
        arVideoSetParami( AR_VIDEO_1394_BRIGHTNESS, value );
        arVideoGetParami( AR_VIDEO_1394_BRIGHTNESS, &value );
        ARLOG("Brightness: %d\n", value);
    }

    if( key == '5' ) {
        arVideoSetParami( AR_VIDEO_1394_GAIN_FEATURE_ON, 1 );
        arVideoSetParami( AR_VIDEO_1394_GAIN_AUTO_ON, 0 );
        arVideoGetParami( AR_VIDEO_1394_GAIN, &value );
        value--;
        arVideoSetParami( AR_VIDEO_1394_GAIN, value );
        arVideoGetParami( AR_VIDEO_1394_GAIN, &value );
        ARLOG("Gain: %d\n", value);
    }
    if( key == '6' ) {
        arVideoSetParami( AR_VIDEO_1394_GAIN_FEATURE_ON, 1 );
        arVideoSetParami( AR_VIDEO_1394_GAIN_AUTO_ON, 0 );
        arVideoGetParami( AR_VIDEO_1394_GAIN, &value );
        value++;
        arVideoSetParami( AR_VIDEO_1394_GAIN, value );
        arVideoGetParami( AR_VIDEO_1394_GAIN, &value );
        ARLOG("Gain: %d\n", value);
    }

    if( key == '7' ) {
        arVideoSetParami( AR_VIDEO_1394_FOCUS_FEATURE_ON, 1 );
        arVideoSetParami( AR_VIDEO_1394_FOCUS_AUTO_ON, 0 );
        arVideoGetParami( AR_VIDEO_1394_FOCUS, &value );
        value--;
        arVideoSetParami( AR_VIDEO_1394_FOCUS, value );
        arVideoGetParami( AR_VIDEO_1394_FOCUS, &value );
        ARLOG("Focus: %d\n", value);
    }
    if( key == '8' ) {
        arVideoSetParami( AR_VIDEO_1394_FOCUS_FEATURE_ON, 1 );
        arVideoSetParami( AR_VIDEO_1394_FOCUS_AUTO_ON, 0 );
        arVideoGetParami( AR_VIDEO_1394_FOCUS, &value );
        value++;
        arVideoSetParami( AR_VIDEO_1394_FOCUS, value );
        arVideoGetParami( AR_VIDEO_1394_FOCUS, &value );
        ARLOG("Focus: %d\n", value);
    }

    if( key == '-' ) {
        arGetLabelingThresh(arHandle, &value);
        value -= 5;
        if( value < 0 ) value = 0;
        arSetLabelingThresh( arHandle, value );
        ARLOG("thresh = %d\n", value);
    }
    if( key == '+' ) {
        arGetLabelingThresh(arHandle, &value);
        value += 5;
        if( value > 255 ) value = 255;
        arSetLabelingThresh( arHandle, value );
        ARLOG("thresh = %d\n", value);
    }

    if( key == 'd' ) {
        debug = 1 - debug;
        arSetDebugMode( arHandle, debug );
    }

    if( key == 's' ) {
        sprintf(filename, "cameraSetting-%08x%08x.dat", id1, id0);
        if( arVideoSaveParam( filename ) < 0 ) {
            ARLOGe("File write error!!\n");
        }
    }

    ARLOG("(+,-): Threshold.\n");
    ARLOG("(1,2): Exposure.\n");
    ARLOG("(3,4): Brightness.\n");
    ARLOG("(5,6): Gain.\n");
    ARLOG("(7,8): Focus.\n");
    ARLOG("s: save parameter.\n");
    ARLOG("d: toggle debug mode.\n");
}

static void mainLoop(void)
{
    AR2VideoBufferT  *buff;
    ARMarkerInfo     *markerInfo;
    int               markerNum;
    double            patt_trans[3][4];
    int               j, k;
    int               mode;

    /* grab a video frame */
    buff = arVideoGetImage();
    if (!buff || !buff->fillFlag) {
        arUtilSleep(2);
        return;
    }

    arGetDebugMode(arHandle, &mode);
    if (mode == AR_DEBUG_ENABLE) {
        argViewportSetPixFormat(vp, AR_PIXEL_FORMAT_MONO); // Drawing the debug image.
        argDrawMode2D(vp);
        arGetImageProcMode(arHandle, &mode);
        if (mode == AR_IMAGE_PROC_FRAME_IMAGE) argDrawImage(arHandle->labelInfo.bwImage);
        else argDrawImageHalf(arHandle->labelInfo.bwImage);
    } else {
        argViewportSetPixFormat(vp, pixFormat); // Drawing the input image.
        argDrawMode2D(vp);
        argDrawImage(buff->buff);
    }

    if( count % 10 == 0 ) {
        sprintf(fps, "%f[fps]", 10.0/arUtilTimer());
        arUtilTimerReset();
    }
    count++;
    glColor3f(0.0, 1.0, 0.0);
    argDrawStringsByIdealPos(fps, 10, cparam.ysize-30);

    /* detect the markers in the video frame */
    if( arDetectMarker(arHandle, buff) < 0 ) {
        cleanup();
        exit(0);
    }

    markerNum = arGetMarkerNum( arHandle );
    if( markerNum == 0 ) {
        argSwapBuffers();
        return;
    }

    /* check for object visibility */
    markerInfo =  arGetMarker( arHandle ); 
    k = -1;
    for( j = 0; j < markerNum; j++ ) {
        if( patt_id == markerInfo[j].id ) {
            if( k == -1 ) k = j;
            else if( markerInfo[k].cf < markerInfo[j].cf ) k = j;
        }
    }
    if( markerInfo[k].cf < 0.7 ) k = -1;
    if( k == -1 ) {
        argSwapBuffers();
        return;
    }
    arGetTransMatSquare(ar3DHandle, &(markerInfo[k]), patt_width, patt_trans);

    draw(patt_trans);
    argSwapBuffers();
}

static void   init(int argc, char *argv[])
{
    char           *cparam_name = NULL;
    ARParam         cparam;
    char            vconf[512];
    int             xsize, ysize;
    ARGViewport     viewport;
    int             i;

    if( argc == 1 ) vconf[0] = '\0';
    else {
        strcpy( vconf, argv[1] );
        for( i = 2; i < argc; i++ ) {strcat(vconf, " "); strcat(vconf,argv[i]);}
    }

    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    if( arVideoGetSize(&xsize, &ysize) < 0 ) exit(0);
    ARLOGi("Image size (x,y) = (%d,%d)\n", xsize, ysize);
    if( (pixFormat=arVideoGetPixelFormat()) < 0 ) exit(0);
    arVideoGetId( &id0, &id1 );
    ARLOGi("Camera ID = (%08x, %08x)\n", id1, id0);

    /* set the initial camera parameters */
    if (cparam_name && *cparam_name) {
        if (arParamLoad(cparam_name, 1, &cparam) < 0) {
            ARLOGe("Camera parameter load error !!\n");
            exit(0);
        }
    } else {
        arParamClearWithFOVy(&cparam, xsize, ysize, M_PI_4); // M_PI_4 radians = 45 degrees.
        ARLOGw("Using default camera parameters for %dx%d image size, 45 degrees vertical field-of-view.\n", xsize, ysize);
    }
    arParamChangeSize( &cparam, xsize, ysize, &cparam );
    ARLOG("*** Camera Parameter ***\n");
    arParamDisp( &cparam );

    if ((cparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
        ARLOGe("Error: arParamLTCreate.\n");
        exit(0);
    }
    if( (arHandle=arCreateHandle(cparamLT)) == NULL ) {
        ARLOGe("Error: arCreateHandle.\n");
        exit(0);
    }
    if( arSetPixelFormat(arHandle, pixFormat) < 0 ) {
        ARLOGe("Error: arSetPixelFormat.\n");
        exit(0);
    }

    if( (ar3DHandle=ar3DCreateHandle(&cparam)) == NULL ) {
        ARLOGe("Error: ar3DCreateHandle.\n");
        exit(0);
    }

    if( (arPattHandle=arPattCreateHandle()) == NULL ) {
        ARLOGe("Error: arPattCreateHandle.\n");
        exit(0);
    }
    if( (patt_id=arPattLoad(arPattHandle, PATT_NAME)) < 0 ) {
        ARLOGe("pattern load error !!\n");
        exit(0);
    }
    arPattAttach( arHandle, arPattHandle );

    /* open the graphics window */
    //argCreateFullWindow();
    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = xsize;
    viewport.ysize = ysize;
    if( (vp=argCreateViewport(&viewport)) == NULL ) exit(0);
    argViewportSetCparam( vp, &cparam );
    argViewportSetPixFormat( vp, pixFormat );
}

/* cleanup function called when program exits */
static void cleanup(void)
{
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}

static void draw( double trans[3][4] )
{
    double    gl_para[16];
    GLfloat   mat_diffuse[]     = {0.0, 0.0, 1.0, 1.0};
    GLfloat   mat_flash[]       = {1.0, 1.0, 1.0, 1.0};
    GLfloat   mat_flash_shiny[] = {50.0};
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   light_ambi[]      = {0.1, 0.1, 0.1, 0.1};
    GLfloat   light_color[]     = {0.9, 0.9, 0.9, 0.1};
    
    argDrawMode3D(vp);
    glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* load the camera transformation matrix */
    argConvGlpara(trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd( gl_para );

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_color);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_flash);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_diffuse);

    glTranslatef( 0.0, 0.0, 40.0 );
    glutSolidCube(80.0);
    glDisable(GL_LIGHT0);
    glDisable( GL_LIGHTING );

    glDisable( GL_DEPTH_TEST );
}
