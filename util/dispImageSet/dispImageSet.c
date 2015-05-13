/*
 *  dispImageSet.c
 *  ARToolKit5
 *
 *  Displays image sets
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
#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR2/imageSet.h>
#include <AR2/util.h>

static int                      winXsize = 1000;
static int                      winYsize = 800;
static ARGViewportHandle      **vp;

static AR2ImageSetT            *imageSet;
static int                      page = 0;

static int  init( int argc, char *argv[] );
static void reportCurrentDPI(void);
static void usage( char *com );
static void dispFunc( void );
static void keyFunc( unsigned char key, int x, int y );


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
    float		zoom;
    int			i;

    for( i = 1; i < argc; i++ ) {
        if( filename == NULL ) filename = argv[i];
        else usage(argv[0] );
    }
    if (!filename || !filename[0]) usage(argv[0]);

    ARLOG("Read ImageSet.\n");
    ar2UtilRemoveExt( filename );
    imageSet = ar2ReadImageSet( filename );
    if( imageSet == NULL ) {
        ARLOGe("file open error: %s.iset\n", filename );
        exit(0);
    }
    ARLOG("  end.\n");

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
    exit(0);
}

static void dispFunc( void )
{
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
