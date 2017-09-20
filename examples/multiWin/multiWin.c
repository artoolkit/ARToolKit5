/*
 *  multiWin.c
 *
 *  gsub-based example code to demonstrate use of ARToolKit
 *  with multiple windows.
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

#ifdef _WIN32
#  include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#  include <GL/glut.h>
#else
#  include <GLUT/glut.h>
#endif
#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR/video.h>
#include <ARUtil/time.h>

#define             VPARA_NAME       "Data/cameraSetting-%08x%08x.dat"
#define             PATT_NAME        "Data/hiro.patt"

ARHandle           *arHandle;
AR3DHandle         *ar3DHandle;
ARGWindowHandle    *w1;
ARGWindowHandle    *w2;
ARGViewportHandle  *vp1;
ARGViewportHandle  *vp2;
int                 xsize, ysize;
int                 patt_id;
double              patt_width = 80.0;
int                 count;
char                fps[256];
char                errValue[256];
ARParamLT          *gCparamLT = NULL;

static void         init(int argc, char *argv[]);
static void         cleanup(void);
static void         mainLoop(void);
static void         draw( ARdouble trans[3][4] );
static void         keyEvent( unsigned char key, int x, int y);


int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    init(argc, argv);

    argSetWindow(w1);
    argSetDispFunc( mainLoop, 1 );
    argSetKeyFunc( keyEvent );

    argSetWindow(w2);
    argSetDispFunc( mainLoop, 1 );
    argSetKeyFunc( keyEvent );

    count = 0;
    fps[0] = '\0';
    arVideoCapStart();
    arUtilTimerReset();
    argMainLoop();
    return (0);
}

static void   keyEvent( unsigned char key, int x, int y)
{
    int   value;

    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        cleanup();
        exit(0);
    }

    if( key == '1' ) {
        arGetLabelingThresh( arHandle, &value );
        value -= 5;
        if( value < 0 ) value = 0;
        arSetLabelingThresh( arHandle, value );
        ARLOG("thresh = %d\n", value);
    }
    if( key == '2' ) {
        arGetLabelingThresh( arHandle, &value );
        value += 5;
        if( value > 255 ) value = 255;
        arSetLabelingThresh( arHandle, value );
        ARLOG("thresh = %d\n", value);
    }

    if( key == 'd' ) {
        arGetDebugMode( arHandle, &value );
        value = 1 - value;
        arSetDebugMode( arHandle, value );
    }
}

static void mainLoop(void)
{
    AR2VideoBufferT *buff;
    ARMarkerInfo   *markerInfo;
    int             markerNum;
    ARdouble        patt_trans[3][4];
    ARdouble        err;
    int             debugMode;
    int             j, k;

    /* grab a video frame */
    buff = arVideoGetImage();
    if (!buff || !buff->fillFlag) {
        arUtilSleep(2);
        return;
    }

    /* detect the markers in the video frame */
    if( arDetectMarker(arHandle, buff) < 0 ) {
        cleanup();
        exit(0);
    }

    argSetWindow(w1);
    
    arGetDebugMode(arHandle, &debugMode);
    if (debugMode == AR_DEBUG_ENABLE) {
        int imageProcMode;
        argViewportSetPixFormat(vp1, AR_PIXEL_FORMAT_MONO); // Drawing the debug image.
        argDrawMode2D(vp1);
        arGetImageProcMode(arHandle, &imageProcMode);
        if (imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE) argDrawImage(arHandle->labelInfo.bwImage);
        else argDrawImageHalf(arHandle->labelInfo.bwImage);
    } else {
        AR_PIXEL_FORMAT pixFormat;
        arGetPixelFormat(arHandle, &pixFormat);
        argViewportSetPixFormat(vp1, pixFormat); // Drawing the input image.
        argDrawMode2D(vp1);
        argDrawImage(buff->buff);
    }

    argSetWindow(w2);
    argDrawMode2D(vp2);
    argDrawImage(buff->buff);
    argSetWindow(w1);

    if( count % 10 == 0 ) {
        sprintf(fps, "%f[fps]", 10.0/arUtilTimer());
        arUtilTimerReset();
    }
    count++;
    glColor3f(0.0f, 1.0f, 0.0f);
    argDrawStringsByIdealPos(fps, 10, ysize-30);

    markerNum = arGetMarkerNum( arHandle );
    if( markerNum == 0 ) {
        argSetWindow(w1);
        argSwapBuffers();
        argSetWindow(w2);
        argSwapBuffers();
        return;
    }

    /* check for object visibility */
    markerInfo =  arGetMarker( arHandle ); 
    k = -1;
    for( j = 0; j < markerNum; j++ ) {
        //ARLOG("ID=%d, CF = %f\n", markerInfo[j].id, markerInfo[j].cf);
        if( patt_id == markerInfo[j].id ) {
            if( k == -1 ) {
                if (markerInfo[j].cf > 0.7) k = j;
            } else if (markerInfo[j].cf > markerInfo[k].cf) k = j;
        }
    }
    if( k == -1 ) {
        argSetWindow(w1);
        argSwapBuffers();
        argSetWindow(w2);
        argSwapBuffers();
        return;
    }

    err = arGetTransMatSquare(ar3DHandle, &(markerInfo[k]), patt_width, patt_trans);
    sprintf(errValue, "err = %f", err);
    glColor3f(0.0f, 1.0f, 0.0f);
    argDrawStringsByIdealPos(fps, 10, ysize-30);
    argDrawStringsByIdealPos(errValue, 10, ysize-60);
    //ARLOG("err = %f\n", err);

    draw(patt_trans);

    argSetWindow(w1);
    argSwapBuffers();
    argSetWindow(w2);
    argSwapBuffers();
}

static void   init(int argc, char *argv[])
{
    char           *cparam_name = NULL;
    ARParam         cparam;
    ARGViewport     viewport;
    ARPattHandle   *arPattHandle;
    char            vconf[512];
    AR_PIXEL_FORMAT pixFormat;
    ARUint32        id0, id1;
    int             i;

    if( argc == 1 ) vconf[0] = '\0';
    else {
        strcpy( vconf, argv[1] );
        for( i = 2; i < argc; i++ ) {strcat(vconf, " "); strcat(vconf,argv[i]);}
    }

    arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL);
    
   /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    if( arVideoGetSize(&xsize, &ysize) < 0 ) exit(0);
    ARLOG("Image size (x,y) = (%d,%d)\n", xsize, ysize);
    if( (pixFormat=arVideoGetPixelFormat()) < 0 ) exit(0);
    if( arVideoGetId( &id0, &id1 ) == 0 ) {
        ARLOG("Camera ID = (%08x, %08x)\n", id1, id0);
        sprintf(vconf, VPARA_NAME, id1, id0);
        if( arVideoLoadParam(vconf) < 0 ) {
            ARLOGe("No camera setting data!!\n");
        }
    }

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
    if( (patt_id=arPattLoad(arPattHandle, PATT_NAME)) < 0 ) {
        ARLOGe("pattern load error !!\n");
        exit(0);
    }
    arPattAttach( arHandle, arPattHandle );

    /* open the graphics window */
    w1 = argCreateWindow(xsize, ysize);
    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = xsize;
    viewport.ysize = ysize;
    if( (vp1=argCreateViewport(&viewport)) == NULL ) exit(0);
    argViewportSetCparam( vp1, &cparam );
    argViewportSetPixFormat( vp1, pixFormat );
    argViewportSetDispMethod( vp1, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
    argViewportSetDispMode( vp1, AR_GL_DISP_MODE_FIT_TO_VIEWPORT );
    argViewportSetDistortionMode( vp1, AR_GL_DISTORTION_COMPENSATE_ENABLE );

    w2 = argCreateWindow(xsize, ysize);
    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = xsize;
    viewport.ysize = ysize;
    if( (vp2=argCreateViewport(&viewport)) == NULL ) exit(0);
    argViewportSetCparam( vp2, &cparam );
    argViewportSetPixFormat( vp2, pixFormat );
    argViewportSetDispMethod( vp2, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
    argViewportSetDispMode( vp2, AR_GL_DISP_MODE_FIT_TO_VIEWPORT );
    argViewportSetDistortionMode( vp2, AR_GL_DISTORTION_COMPENSATE_DISABLE );

    return;
}

/* cleanup function called when program exits */
static void cleanup(void)
{
    arParamLTFree(&gCparamLT);
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}

static void draw( ARdouble trans[3][4] )
{
    ARdouble  gl_para[16];
    GLfloat   mat_diffuse[]     = {0.0f, 0.0f, 1.0f, 0.0f};
    GLfloat   mat_flash[]       = {1.0f, 1.0f, 1.0f, 0.0f};
    GLfloat   mat_flash_shiny[] = {50.0f};
    GLfloat   light_position[]  = {100.0f, -200.0f, 200.0f, 0.0f};
    GLfloat   light_ambi[]      = {0.1f, 0.1f, 0.1f, 0.0f};
    GLfloat   light_color[]     = {1.0f, 1.0f, 1.0f, 0.0f};

    argSetWindow(w1);
    argDrawMode3D(vp1);
    glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* load the camera transformation matrix */
    argConvGlpara(trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
#ifdef ARDOUBLE_IS_FLOAT
    glLoadMatrixf( gl_para );
#else
    glLoadMatrixd( gl_para );
#endif

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_color);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_color);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_flash);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_diffuse);

#if 0
    glTranslatef( 0.0f, 0.0f, 40.0f );
    glutSolidCube(80.0);
#else
    glTranslatef( 0.0f, 0.0f, 20.0f );
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidTeapot(40.0);
#endif
    glDisable(GL_LIGHT0);
    glDisable( GL_LIGHTING );

    glDisable( GL_DEPTH_TEST );
}
