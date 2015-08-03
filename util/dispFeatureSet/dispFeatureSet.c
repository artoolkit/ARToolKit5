/*
 *  dispFeatureSet.c
 *  ARToolKit5
 *
 *  Displays feature set files.
 *
 *  Run with "--help" parameter to see usage.
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
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR2/config.h>
#include <AR2/imageSet.h>
#include <AR2/featureSet.h>
#include <AR2/util.h>
#include <KPM/kpm.h>

static int                      winXsize = 1000;
static int                      winYsize = 800;
static ARGViewportHandle      **vp;

static AR2ImageSetT            *imageSet;
static AR2FeatureSetT          *featureSet;
static KpmRefDataSet           *refDataSet;
static int                      page = 0;

static int                      display_fset = 0;
static int                      display_fset3 = 0;

static int  init( int argc, char *argv[] );
static void reportCurrentDPI(void);
static void usage( char *com );
static void dispFunc( void );
static void keyFunc( unsigned char key, int x, int y );
static void drawFeatureRect( int x, int y, int ts1, int ts2 );


int main( int argc, char *argv[] )
{
    glutInit(&argc, argv);
    init( argc, argv );
    argSetKeyFunc( keyFunc );
    argSetDispFunc( dispFunc, 0 );
    argMainLoop();
}

static int init( int argc, char *argv[] )
{
    ARGViewport		viewport;
    char		*filename = NULL;
    int			xmax, ymax;
    float		xzoom, yzoom;
    float       zoom;
    int			i, display_defaults = 1;


    for( i = 1; i < argc; i++ ) {
        if (strcmp(argv[i], "-fset") == 0) {
            display_defaults = 0;
            display_fset = 1;
        } else if( strcmp(argv[i], "-fset2") == 0 ) {
            ARLOGe("Error: -fset2 option no longer supported as of ARToolKit v5.3.\n");
            exit(-1);
        } else if( strcmp(argv[i], "-fset3") == 0 ) {
            display_defaults = 0;
            display_fset3 = 1;
        } else if( filename == NULL ) filename = argv[i];
        else usage(argv[0] );
    }
    if (!filename || !filename[0]) usage(argv[0]);
    
    if (display_defaults) display_fset = display_fset3 = 1;

    ARLOG("Read ImageSet.\n");
    ar2UtilRemoveExt( filename );
    imageSet = ar2ReadImageSet( filename );
    if( imageSet == NULL ) {
        ARLOGe("file open error: %s.iset\n", filename );
        exit(0);
    }
    ARLOG("  end.\n");

    if (display_fset) {
        ARLOG("Read FeatureSet.\n");
        featureSet = ar2ReadFeatureSet( filename, "fset" );
        if( featureSet == NULL ) {
            ARLOGe("file open error: %s.fset\n", filename );
            exit(0);
        }
        ARLOG("  end.\n");
    }
 
    if (display_fset3) {
        ARLOG("Read FeatureSet3.\n");
        kpmLoadRefDataSet( filename, "fset3", &refDataSet );
        if( refDataSet == NULL ) {
            ARLOGe("file open error: %s.fset3\n", filename );
            exit(0);
        }
        ARLOG("  end.\n");
        ARLOG("num = %d\n", refDataSet->num);
    }

    arMalloc(vp, ARGViewportHandle *, imageSet->num);

    xmax = ymax = 0;
    for( i = 0; i < imageSet->num; i++ ) {
        if( imageSet->scale[i]->xsize > xmax ) xmax = imageSet->scale[i]->xsize;
        if( imageSet->scale[i]->ysize > ymax ) ymax = imageSet->scale[i]->ysize;
    }
    xzoom = yzoom = 1.0;
    while( xmax > winXsize*xzoom ) xzoom += 1.0;
    while( ymax > winYsize*yzoom ) yzoom += 1.0;
    if( xzoom > yzoom ) zoom = 1.0/xzoom;
    else                zoom = 1.0/yzoom;
    winXsize = xmax * zoom;
    winYsize = ymax * zoom;
    ARLOG("Size = (%d,%d) Zoom = %f\n", xmax, ymax, zoom);
    argCreateWindow( winXsize, winYsize );

    for( i = 0; i < imageSet->num; i++ ) {
        viewport.sx = viewport.sy = 0;
        viewport.xsize = imageSet->scale[i]->xsize * zoom;
        viewport.ysize = imageSet->scale[i]->ysize * zoom;
        vp[i] = argCreateViewport( &viewport );
        argViewportSetImageSize( vp[i], imageSet->scale[i]->xsize, imageSet->scale[i]->ysize );
        argViewportSetDispMethod( vp[i], AR_GL_DISP_METHOD_TEXTURE_MAPPING_FRAME );
        //argViewportSetDispMethod( vp[i], AR_GL_DISP_METHOD_GL_DRAW_PIXELS );
        argViewportSetDispMode( vp[i], AR_GL_DISP_MODE_FIT_TO_VIEWPORT );
        argViewportSetDistortionMode( vp[i], AR_GL_DISTORTION_COMPENSATE_DISABLE );
    }

    reportCurrentDPI();
    
    return 0;
}

static void reportCurrentDPI(void)
{
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
	ARLOG("%f[dpi] image. Size = (%d,%d)\n", imageSet->scale[page/(AR2_BLUR_IMAGE_MAX)]->dpi,
	                                         imageSet->scale[page/(AR2_BLUR_IMAGE_MAX)]->xsize,
	                                         imageSet->scale[page/(AR2_BLUR_IMAGE_MAX)]->ysize);
#else
	ARLOG("%f[dpi] image. Size = (%d,%d)\n", imageSet->scale[page]->dpi,
	                                         imageSet->scale[page]->xsize,
	                                         imageSet->scale[page]->ysize);
#endif
}

static void usage( char *com )
{
    ARLOG("%s <filename>\n", com);
    ARLOG("    -fset     Show fset features.\n");
    ARLOG("    -fset3    Show fset3 features.\n");
    ARLOG("%s <filename>\n", com);
    exit(0);
}

static void dispFunc( void )
{
    int     x, y;
    int     i, j;
    char    str[32];

    glClear( GL_COLOR_BUFFER_BIT );
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
    argViewportSetPixFormat( vp[page/AR2_BLUR_IMAGE_MAX], AR_PIXEL_FORMAT_MONO );
    argDrawMode2D( vp[page/AR2_BLUR_IMAGE_MAX] );
    argDrawImage( imageSet->scale[page/AR2_BLUR_IMAGE_MAX]->imgBWBlur[page%AR2_BLUR_IMAGE_MAX] );
#else
    argViewportSetPixFormat( vp[page], AR_PIXEL_FORMAT_MONO );
    argDrawMode2D( vp[page] );
    argDrawImage( imageSet->scale[page]->imgBW );
#endif

    if (display_fset) {
        for( i = 0; i < featureSet->list[page].num; i++ ) {
            x = featureSet->list[page].coord[i].x;
            y = featureSet->list[page].coord[i].y;
            drawFeatureRect( x, y, AR2_DEFAULT_TS1, AR2_DEFAULT_TS2 );
            sprintf(str, "%d", i);
            glColor3f( 0.0f, 0.0f, 1.0f );
            argDrawStringsByObservedPos(str, x, y);
        }
        ARLOG("fset:  Num of feature points: %d\n", featureSet->list[page].num);
    }

    if (display_fset3) {
        for( i = j = 0; i < refDataSet->num; i++ ) {
            if( refDataSet->refPoint[i].refImageNo != page ) continue;
            x = refDataSet->refPoint[i].coord2D.x;
            y = refDataSet->refPoint[i].coord2D.y;
            glColor3f( 0.0f, 1.0f, 0.0f );
            argDrawLineByObservedPos(x-5, y-5, x+5, y+5);
            argDrawLineByObservedPos(x+5, y-5, x-5, y+5);
            j++;
        }
        ARLOG("fset3: Num of feature points: %d\n", j);
#if 0
        for (i = 0; i < refDataSet->pageNum; i++) {
            for (j = 0; j < refDataSet->pageInfo[i].imageNum; j++) {
                if (refDataSet->pageInfo[i].imageInfo[j].imageNo == page) {
                    ARLOG("fset3: Image size: %dx%d\n", refDataSet->pageInfo[i].imageInfo[j].width, refDataSet->pageInfo[i].imageInfo[j].height);
                }
            }
        }
#endif
    }

    argSwapBuffers();
}

static void keyFunc( unsigned char key, int x, int y )
{
    if( key == 0x1b ) {
        argCleanup();
        exit(0);
    }

    if( key == ' ' ) {
        page++;
#if AR2_CAPABLE_ADAPTIVE_TEMPLATE
        page = page % (imageSet->num*(AR2_BLUR_IMAGE_MAX));
#else
        page = page % (imageSet->num);
#endif
        reportCurrentDPI();
        dispFunc();
    }
}

static void drawFeatureRect( int x, int y, int ts1, int ts2 )
{
    glColor3f( 1.0f, 0.0f, 0.0f );
    glLineWidth( 2.0f );

    argDrawLineByObservedPos( x-ts1, y-ts1, x+ts2, y-ts1 );
    argDrawLineByObservedPos( x+ts2, y-ts1, x+ts2, y+ts2 );
    argDrawLineByObservedPos( x+ts2, y+ts2, x-ts1, y+ts2 );
    argDrawLineByObservedPos( x-ts1, y+ts2, x-ts1, y-ts1 );
}
