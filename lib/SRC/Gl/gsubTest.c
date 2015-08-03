/*
 *  gsubTest.c
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
 *  Author(s): Hirokazu Kato
 *
 */

#ifdef _WIN32
#  include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <AR/ar.h>
#include <AR/gsub.h>

#define             PIXEL_FORMAT       AR_PIXEL_FORMAT_RGB
#define             PIXEL_SIZE         3

#define             SQUARE_WIDTH       100.0

ARHandle           *arHandle;
ARPattHandle       *arPattHandle;
AR3DHandle         *ar3DHandle;
ARGViewportHandle  *vp;
int                 xsize, ysize;
int                 imageMode = 0;
int                 dispMode = 0;
int                 distMode = 0;

static void   init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static void   draw( ARdouble trans[3][4] );

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
    init();
    argSetKeyFunc( keyEvent );
    argSetDispFunc( mainLoop, 1 );
    argMainLoop();
    return (0);
}

static void   keyEvent( unsigned char key, int x, int y)
{
    if( key == 'i' ) imageMode = (imageMode + 1) % 4;
    if( key == 'd' ) dispMode = (dispMode + 1) % 3;
    if( key == 'c' ) distMode = (distMode + 1) % 2;

    argDefaultKeyFunc( key, x, y );
}

// Report state of ARToolKit tracker.
static void debugReportMode(ARGViewportHandle *vp)
{
	if (vp->dispMethod == AR_GL_DISP_METHOD_GL_DRAW_PIXELS) {
		ARLOGe("dispMode (d)   : GL_DRAW_PIXELS\n");
	} else if (vp->dispMethod == AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME) {
		ARLOGe("dispMode (d)   : TEXTURE MAPPING (FULL RESOLUTION)\n");
	} else {
		ARLOGe("dispMode (d)   : TEXTURE MAPPING (HALF RESOLUTION)\n");
	}
}

static void mainLoop(void)
{
    static ARUint8     *dataPtr = NULL;
    static int          oldImageMode = -1;
    static int          oldDispMode  = -1;
    static int          oldDistMode  = -1;
    ARdouble            patt_trans[3][4];
    int                 i, j;

    if( dataPtr == NULL ) {
        arMalloc( dataPtr, ARUint8, xsize*ysize*PIXEL_SIZE );
    }

    if( oldImageMode != 0 && imageMode == 0 ) {
        for( i = 0; i < xsize*ysize; i++ ) {
            dataPtr[i*PIXEL_SIZE+0] = 200;
            dataPtr[i*PIXEL_SIZE+1] = 200;
            dataPtr[i*PIXEL_SIZE+2] = 200;
        }
        for( j = 190; j < 291; j++ ) {
            for( i = 280; i < 381; i++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 20;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 20;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 20;
            }
        }
        i = 0;
        for( j = 0; j < ysize; j++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 255;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 0;
        }
        i = 639;
        for( j = 0; j < ysize; j++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 255;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 0;
        }
        j = 0;
        for( i = 0; i < xsize; i++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 255;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 0;
        }
        j = 479;
        for( i = 0; i < xsize; i++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 255;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 0;
        }
        oldImageMode = 0;
    }

    if( oldImageMode != 1 && imageMode == 1 ) {
        for( j = 0; j < 480; j += 2 ) {
            for( i = 0; i < 640; i++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 255;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 0;
            }
        }
        for( j = 1; j < 480; j += 2 ) {
            for( i = 0; i < 640; i++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 255;
            }
        }
        oldImageMode = 1;
    }
    if( oldImageMode != 2 && imageMode == 2 ) {
        for( i = 0; i < 640; i += 2 ) {
            for( j = 0; j < 480; j++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 255;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 0;
            }
        }
        for( i = 1; i < 640; i += 2 ) {
            for( j = 0; j < 480; j++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 0;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 255;
            }
        }
        oldImageMode = 2;
    }
    if( oldImageMode != 3 && imageMode == 3 ) {
        for( i = 0; i < xsize*ysize; i++ ) {
            dataPtr[i*PIXEL_SIZE+0] = 200;
            dataPtr[i*PIXEL_SIZE+1] = 200;
            dataPtr[i*PIXEL_SIZE+2] = 200;
        }
        for( j = 190; j < 291; j++ ) {
            for( i = 280; i < 381; i++ ) {
                dataPtr[(j*xsize+i)*PIXEL_SIZE+0] = 20;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+1] = 20;
                dataPtr[(j*xsize+i)*PIXEL_SIZE+2] = 20;
            }
        }
        oldImageMode = 3;
    }

    /* detect the markers in the video frame */
    if( arDetectMarker(arHandle, dataPtr) < 0 ) {
        cleanup();
        exit(0);
    }

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClearDepth( 1.0f );
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    if( oldDispMode != 0 && dispMode == 0 ) {
        argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_GL_DRAW_PIXELS );
        oldDispMode = 0;
        debugReportMode(vp);
    }
    else if( oldDispMode != 1 && dispMode == 1 ) {
        argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
        oldDispMode = 1;
        debugReportMode(vp);
    }
    else if( oldDispMode != 2 && dispMode == 2 ) {
        argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FIELD );
        oldDispMode = 2;
        debugReportMode(vp);
   }

    if( oldDistMode != 0 && distMode == 0 ) {
        argViewportSetDistortionMode( vp, AR_GL_DISTORTION_COMPENSATE_DISABLE );
        oldDistMode = 0;
    }
    if( oldDistMode != 1 && distMode == 1 ) {
        argViewportSetDistortionMode( vp, AR_GL_DISTORTION_COMPENSATE_ENABLE );
        oldDistMode = 1;
    }
   
    argDrawMode2D(vp);
    argDrawImage( dataPtr );

    if( imageMode == 3 ) {
        glLineWidth( 1.0f );
        glColor3f( 0.0f, 1.0f, 0.0f );
        argDrawSquareByIdealPos( arHandle->markerInfo[0].vertex );

        glColor3f( 1.0f, 0.0f, 0.0f );
        argDrawLineByIdealPos( 0.0, 0.0, 640.0, 0.0 );
        argDrawLineByIdealPos( 0.0, 479.0, 640.0, 479.0 );
        argDrawLineByIdealPos( 0.0, -1.0, 0.0, 479.0 );
        argDrawLineByIdealPos( 639.0, -1.0, 639.0, 479.0 );

        argDrawLineByIdealPos( 0.0, 188.0, 639.0, 188.0 );
        argDrawLineByIdealPos( 0.0, 292.0, 639.0, 292.0 );
        argDrawLineByIdealPos( 278.0, 0.0, 278.0, 479.0 );
        argDrawLineByIdealPos( 382.0, 0.0, 382.0, 479.0 );
    }

    if( arHandle->marker_num == 0 ) {
        argSwapBuffers();
        return;
    }

    arGetTransMatSquare(ar3DHandle, &(arHandle->markerInfo[0]), SQUARE_WIDTH, patt_trans);
    draw(patt_trans);
    argSwapBuffers();
}

static void init( void )
{
    ARParam          cparam;
    ARParamLT       *cparamLT;
    ARGViewport      viewport;

    xsize = 640;
    ysize = 480;
    ARLOGi("Image size (x,y) = (%d,%d)\n", xsize, ysize);

    /* set the initial camera parameters */
    arParamClear(&cparam, xsize, ysize, AR_DIST_FUNCTION_VERSION_DEFAULT);
    ARLOG("*** Camera Parameter ***\n");
    arParamDisp( &cparam );
    //COVHI10445 ignored as false positive, i.e. cparam->m[3][4] uninitialized.
    cparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET);
    
    arHandle = arCreateHandle(cparamLT);
    if( arHandle == NULL ) {
        ARLOGe("Error: arCreateHandle.\n");
        exit(0);
    }
    arSetPixelFormat( arHandle, PIXEL_FORMAT );
    arSetLabelingMode( arHandle, AR_LABELING_BLACK_REGION );
    arSetImageProcMode( arHandle, AR_IMAGE_PROC_FRAME_IMAGE );

    ar3DHandle = ar3DCreateHandle( &cparam );
    if( ar3DHandle == NULL ) {
        ARLOGe("Error: ar3DCreateHandle.\n");
        exit(0);
    }

    /* open the graphics window */
    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = xsize;
    viewport.ysize = ysize;
    vp = argCreateViewport( &viewport );
    if( vp == NULL ) exit(0);
    argViewportSetCparam( vp, &cparam );
    argViewportSetPixFormat( vp, PIXEL_FORMAT );
    argViewportSetDispMode( vp, AR_GL_DISP_MODE_FIT_TO_VIEWPORT );

    argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_GL_DRAW_PIXELS );
    //argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
    //argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_TEXTURE_MAPPING_FIELD );

    argViewportSetDistortionMode( vp, AR_GL_DISTORTION_COMPENSATE_DISABLE );
    //argViewportSetDistortionMode( vp, AR_GL_DISTORTION_COMPENSATE_ENABLE );
  
#if 0
    if( argSetFullScreenConfig("1024x768") == 0 ) {
        ARLOGe("Full screen is not possible.\n");
        exit(0);
    }
    //argGetWindowSizeFullScreen( &viewport.xsize, &viewport.ysize );
    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = 1024;
    viewport.ysize = 768;
    argViewportSetViewportFullScreen( vpL, &viewport );
    viewport.sx = 1024;
    argViewportSetViewportFullScreen( vpR, &viewport );
#endif
}

/* cleanup function called when program exits */
static void cleanup(void)
{
    argCleanup();
}

static void draw( ARdouble trans[3][4] )
{
    ARdouble    gl_para[16];
    
    argDrawMode3D(vp);

    /* load the camera transformation matrix */
    argConvGlpara(trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
#ifdef ARDOUBLE_IS_FLOAT
    glLoadMatrixf( gl_para );
#else
    glLoadMatrixd( gl_para );
#endif
    
    glColor3f( 1.0f, 1.0f, 0.0f );
    glBegin(GL_LINES);
    glVertex2f( -50.0f, -50.0f );
    glVertex2f( -50.0f,  50.0f );
    glVertex2f(  50.0f, -50.0f );
    glVertex2f(  50.0f,  50.0f );
    glVertex2f( -50.0f, -50.0f );
    glVertex2f(  50.0f, -50.0f );
    glVertex2f( -50.0f,  50.0f );
    glVertex2f(  50.0f,  50.0f );
    glEnd();
}
