/*
 *  multiTest.c
 *
 *  gsub-based example code to demonstrate use of ARToolKit
 *  with multi-marker tracking.
 *
 *  Press '?' while running for help on available key commands.
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not use, install, modify or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive
 *  license, under Daqri's copyrights in this original Daqri software (the
 *  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
 *  Software, with or without modifications, in source and/or binary forms;
 *  provided that if you redistribute the Daqri Software in its entirety and
 *  without modifications, you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri LLC. All Rights Reserved.
 *  Copyright 2002-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#  ifdef _WIN32
#    include <windows.h>
#    define _USE_MATH_DEFINES
#  endif
#  include <GL/glut.h>
#else
#  include <GLUT/glut.h>
#endif
#include <math.h>
#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/arMulti.h>
#include <ARUtil/time.h>

#define                 CONFIG_NAME      "Data/multi/marker.dat"

ARHandle               *arHandle;
AR3DHandle             *ar3DHandle;
ARGViewportHandle      *vp;
ARMultiMarkerInfoT     *config;
int                     robustFlag = 0;
int                     count;
ARParamLT              *gCparamLT = NULL;

static void             init(int argc, char *argv[]);
static void             cleanup(void);
static void             mainLoop(void);
static void             draw( ARdouble trans1[3][4], ARdouble trans2[3][4], int mode );
static void             keyEvent( unsigned char key, int x, int y);


int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
    init(argc, argv);

    argSetDispFunc( mainLoop, 1 );
    argSetKeyFunc( keyEvent );
    count = 0;
    arVideoCapStart();
    arUtilTimerReset();
    argMainLoop();
	return (0);
}

static void   keyEvent( unsigned char key, int x, int y)
{
    int     debug;
    int     thresh;

    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        ARLOG("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        cleanup();
        exit(0);
    }

    if( key == 'd' ) {
        arGetDebugMode( arHandle, &debug );
        debug = 1 - debug;
        arSetDebugMode( arHandle, debug );
    }

    if( key == '1' ) {
        arGetDebugMode( arHandle, &debug );
        if( debug ) {
            arGetLabelingThresh( arHandle, &thresh );
            thresh -= 5;
            if( thresh < 0 ) thresh = 0;
            arSetLabelingThresh( arHandle, thresh );
            ARLOG("thresh = %d\n", thresh);
        }
    }
    if( key == '2' ) {
        arGetDebugMode( arHandle, &debug );
        if( debug ) {
            arGetLabelingThresh( arHandle, &thresh );
            thresh += 5;
            if( thresh > 255 ) thresh = 255;
            arSetLabelingThresh( arHandle, thresh );
            ARLOG("thresh = %d\n", thresh);
        }
    }

    if( key == ' ' ) {
        robustFlag = 1 - robustFlag;
        if( robustFlag ) ARLOG("Robust estimation mode.\n");
        else             ARLOG("Normal estimation mode.\n");
    }
}

/* main loop */
static void mainLoop(void)
{
    AR2VideoBufferT *buff;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             imageProcMode;
    int             debugMode;
    double          err;
    int             i;

    /* grab a video frame */
    buff = arVideoGetImage();
    if (!buff || !buff->fillFlag) {
        arUtilSleep(2);
        return;
    }

    if( count == 100 ) {
        ARLOG("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        arUtilTimerReset();
        count = 0;
    }
    count++;

    /* detect the markers in the video frame */
    if (arDetectMarker(arHandle, buff) < 0) {
        cleanup();
        exit(0);
    }
    marker_num = arGetMarkerNum( arHandle );
    marker_info =  arGetMarker( arHandle );

    arGetDebugMode(arHandle, &debugMode);
    if (debugMode == AR_DEBUG_ENABLE) {
        argViewportSetPixFormat(vp, AR_PIXEL_FORMAT_MONO); // Drawing the debug image.
        argDrawMode2D(vp);
        arGetImageProcMode(arHandle, &imageProcMode);
        if (imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE) argDrawImage(arHandle->labelInfo.bwImage);
        else argDrawImageHalf(arHandle->labelInfo.bwImage);
        glColor3f( 1.0f, 0.0f, 0.0f );
        glLineWidth( 2.0f);
        for( i = 0; i < marker_num; i++ ) {
            argDrawSquareByIdealPos( marker_info[i].vertex );
        }
        glLineWidth( 1.0f );
    } else {
        AR_PIXEL_FORMAT pixFormat;
        arGetPixelFormat(arHandle, &pixFormat);
        argViewportSetPixFormat(vp, pixFormat); // Drawing the input image.
        argDrawMode2D(vp);
        argDrawImage(buff->buff);
    }

    if( robustFlag ) {
        err = arGetTransMatMultiSquareRobust( ar3DHandle, marker_info, marker_num, config);
    }
    else {
        err = arGetTransMatMultiSquare( ar3DHandle, marker_info, marker_num, config);
    }
    if( config->prevF == 0 ) {
        argSwapBuffers();
        return;
    }
    //ARLOGd("err = %f\n", err);

    argDrawMode3D(vp);
    glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    for( i = 0; i < config->marker_num; i++ ) {
        if( config->marker[i].visible >= 0 ) draw( config->trans, config->marker[i].trans, 0 );
        else                                 draw( config->trans, config->marker[i].trans, 1 );
    }

    argSwapBuffers();
}

static void   init(int argc, char *argv[])
{
    char           *cparam_name = NULL;
    ARParam         cparam;
    ARGViewport     viewport;
    ARPattHandle   *arPattHandle;
    char            vconf[512];
    char            configName[512];
    int             xsize, ysize;
    AR_PIXEL_FORMAT pixFormat;
    int             i;

    configName[0] = '\0';
    vconf[0] = '\0';
    for( i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "-config=", 8) == 0 ) {
            strcpy(configName, &argv[i][8]);
        }
        else {
            if( vconf[0] != '\0' ) strcat(vconf, " ");
            strcat(vconf, argv[i]);
        }
    }
    if( configName[0] == '\0' ) strcpy(configName, CONFIG_NAME);

    arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL);
    
    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    /* find the size of the window */
    if( arVideoGetSize(&xsize, &ysize) < 0 ) exit(0);
    ARLOGi("Image size (x,y) = (%d,%d)\n", xsize, ysize);
    if( (pixFormat=arVideoGetPixelFormat()) < 0 ) exit(0);

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
    if ((gCparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
        ARLOGe("Error: arParamLTCreate.\n");
        exit(-1);
    }

    if( (arHandle=arCreateHandle(gCparamLT)) == NULL ) {
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
    arPattAttach( arHandle, arPattHandle );

    if( (config = arMultiReadConfigFile(configName, arPattHandle)) == NULL ) {
        ARLOGe("config data load error !!\n");
        exit(0);
    }
    if( config->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE ) {
        arSetPatternDetectionMode( arHandle, AR_TEMPLATE_MATCHING_COLOR );
    } else if( config->patt_type == AR_MULTI_PATTERN_DETECTION_MODE_MATRIX ) {
        arSetPatternDetectionMode( arHandle, AR_MATRIX_CODE_DETECTION );
    } else { // AR_MULTI_PATTERN_DETECTION_MODE_TEMPLATE_AND_MATRIX
        arSetPatternDetectionMode( arHandle, AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX );
    }

    /* open the graphics window */
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
    arParamLTFree(&gCparamLT);
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}

static void draw( ARdouble trans1[3][4], ARdouble trans2[3][4], int mode )
{
    ARdouble  gl_para[16];
    GLfloat   light_position[]  = {100.0f, -200.0f, 200.0f, 0.0f};
    GLfloat   light_ambi[]      = {0.1f, 0.1f, 0.1f, 0.0f};
    GLfloat   light_color[]     = {1.0f, 1.0f, 1.0f, 0.0f};
    GLfloat   mat_flash[]       = {1.0f, 1.0f, 1.0f, 0.0f};
    GLfloat   mat_flash_shiny[] = {50.0f};
    GLfloat   mat_diffuse[]     = {0.0f, 0.0f, 1.0f, 1.0f};
    GLfloat   mat_diffuse1[]    = {1.0f, 0.0f, 0.0f, 1.0f};
    int       debugMode;
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* load the camera transformation matrix */
    glMatrixMode(GL_MODELVIEW);
    argConvGlpara(trans1, gl_para);
#ifdef ARDOUBLE_IS_FLOAT
    glLoadMatrixf( gl_para );
#else
    glLoadMatrixd( gl_para );
#endif
    argConvGlpara(trans2, gl_para);
#ifdef ARDOUBLE_IS_FLOAT
    glMultMatrixf( gl_para );
#else
    glMultMatrixd( gl_para );
#endif

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_color);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_color);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
    if( mode == 0 ) {
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_diffuse);
    }
    else {
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse1);
        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_diffuse1);
    }
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef( 0.0f, 0.0f, 20.0f );
    arGetDebugMode( arHandle, &debugMode );
    if( debugMode == 0 ) glutSolidCube(40.0);
     else                glutWireCube(40.0);
    glPopMatrix();

    glDisable( GL_LIGHT0 );
    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
}
