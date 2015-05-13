/*
 *  argBase.c
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
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AR/config.h>
#include "argPrivate.h"
#define ARG_BASE_DO_NOT_INIT_GLUT

static int                  initWinPosX = 0;
static int                  initWinPosY = 0;
static ARGWindowHandle     *winHandle = NULL;
static ARGViewportHandle   *currentVP = NULL;
#ifndef ARG_BASE_DO_NOT_INIT_GLUT
static int                  initF = 0;
#endif

int argSetWindow( ARGWindowHandle *w )
{
        winHandle = w;
        glutSetWindow( w->winID );
        return 0;
}

int argInitWindowPos( int xpos, int ypos )
{
    initWinPosX = xpos;
    initWinPosY = ypos;
    return 0;
}

ARGWindowHandle *argCreateWindow( int xsize, int ysize )
{
    ARGWindowHandle  *w;
#ifndef ARG_BASE_DO_NOT_INIT_GLUT
    int               argc = 1;
    char             *argv[1] = {"ARToolKit"};

    if( initF == 0 ) { glutInit(&argc, argv); initF = 1; }
#endif
	
    w = (ARGWindowHandle *)malloc(sizeof(ARGWindowHandle));
    if( w == NULL ) return NULL;

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowPosition(initWinPosX, initWinPosY);
    glutInitWindowSize(xsize, ysize);
    w->winID = glutCreateWindow("");
    w->xsize = xsize;
    w->ysize = ysize;

    if (glutExtensionSupported("GL_ARB_texture_rectangle") || glutExtensionSupported("GL_EXT_texture_rectangle") || glutExtensionSupported("GL_NV_texture_rectangle")) {
        w->possibleTextureRectangle = 1;
        glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &(w->maxTextureRectangleSize));
    } else {
		w->possibleTextureRectangle = 0;
	}

    glutKeyboardFunc( argDefaultKeyFunc );

    winHandle = w;
    glutSetWindow( w->winID );

    return w;
}

ARGWindowHandle *argCreateFullWindow( void )
{
    ARGWindowHandle  *w;
    int               xsize, ysize;
#ifndef ARG_BASE_DO_NOT_INIT_GLUT
    int               argc = 1;
    char             *argv[1] = {"ARToolKit"};

    if( initF == 0 ) { glutInit(&argc, argv); initF = 1; }
#endif
	
    w = (ARGWindowHandle *)malloc(sizeof(ARGWindowHandle));
    if( w == NULL ) return (NULL);

    xsize = glutGet(GLUT_SCREEN_WIDTH);
    ysize = glutGet(GLUT_SCREEN_HEIGHT);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowPosition(initWinPosX, initWinPosY);
    glutInitWindowSize(xsize, ysize);
    w->winID = glutCreateWindow("");
    w->xsize = xsize;
    w->ysize = ysize;
    glutFullScreen();

    if( glutExtensionSupported("GL_ARB_texture_rectangle") || glutExtensionSupported("GL_EXT_texture_rectangle") || glutExtensionSupported("GL_NV_texture_rectangle") ) {
        w->possibleTextureRectangle = 1;
        glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &(w->maxTextureRectangleSize));
    } else {
		w->possibleTextureRectangle = 0;
	}

    glutKeyboardFunc( argDefaultKeyFunc );

    winHandle = w;
    glutSetWindow( w->winID );

    return w;
}

int argGetScreenSize( int *xsize, int *ysize )
{
    if( winHandle == NULL ) return -1;

    *xsize = glutGet(GLUT_SCREEN_WIDTH);
    *ysize = glutGet(GLUT_SCREEN_HEIGHT);

    return 0;
}

int argGetWindowSize( int *xsize, int *ysize )
{
    if( winHandle == NULL ) return -1;

    *xsize = winHandle->xsize;
    *ysize = winHandle->ysize;

    return 0;
}

int argSetWindowSize( int xsize, int ysize )
{
    if( winHandle == NULL ) return -1;

    winHandle->xsize = xsize;
    winHandle->ysize = ysize;

    return 0;
}

ARGViewportHandle *argCreateViewport( ARGViewport *viewport )
{
    ARGViewportHandle *handle;

    if( winHandle == NULL ) {
        if( argCreateWindow(viewport->sx+viewport->xsize, viewport->sy+viewport->ysize) < 0 ) return NULL;
    }

    handle = (ARGViewportHandle *)malloc(sizeof(ARGViewportHandle));
    if( handle == NULL ) return NULL;

    handle->viewport               = *viewport;
    handle->cparam                 = NULL;
    handle->clipPlane.nearClip     = (float)AR_GL_DEFAULT_CLIP_NEAR;
    handle->clipPlane.farClip      = (float)AR_GL_DEFAULT_CLIP_FAR;
    handle->dispMethod             = AR_GL_DEFAULT_DISP_METHOD;
    handle->dispMode               = AR_GL_DEFAULT_DISP_MODE;
    handle->flipMode               = AR_GL_DEFAULT_FLIP_MODE;
    handle->distortionMode         = AR_GL_DEFAULT_DISTORTION_MODE;
    handle->pixFormat              = AR_DEFAULT_PIXEL_FORMAT;
    handle->scale                  = 1.0f;
    handle->dispList.texId         = -1;
    handle->dispList.listIndex     = -1;
    handle->dispListHalf.texId     = -1;
    handle->dispListHalf.listIndex = -1;

    currentVP = handle;

    return handle;
}

int argViewportSetViewport( ARGViewportHandle *argVPhandle, ARGViewport *viewport )
{
    if( argVPhandle == NULL ) return -1;

    argVPhandle->viewport = *viewport;

    return 0;
}

int argViewportSetCparam( ARGViewportHandle *argVPhandle, ARParam *cparam )
{
    if( argVPhandle == NULL ) return -1;

    argClearDisplayList( &(argVPhandle->dispList) );
    argClearDisplayList( &(argVPhandle->dispListHalf) );

    if( argVPhandle->cparam == NULL ) {
        argVPhandle->cparam = (ARParam *)malloc(sizeof(ARParam));
        if( argVPhandle->cparam == NULL ) return -1;
    }

    *(argVPhandle->cparam) = *cparam;

    return 0;
}

int argViewportSetImageSize( ARGViewportHandle *argVPhandle, int xsize, int ysize )
{
    if( argVPhandle == NULL ) return -1;

    argClearDisplayList( &(argVPhandle->dispList) );
    argClearDisplayList( &(argVPhandle->dispListHalf) );

    if( argVPhandle->cparam == NULL ) {
        argVPhandle->cparam = (ARParam *)malloc(sizeof(ARParam));
        if( argVPhandle->cparam == NULL ) return -1;
    }
    arParamClear( argVPhandle->cparam, xsize, ysize, AR_DIST_FUNCTION_VERSION_DEFAULT );

    return 0;
}

int argViewportSetClipPlane( ARGViewportHandle *argVPhandle, ARGClipPlane *clipPlane )
{
    if( argVPhandle == NULL ) return -1;

    argVPhandle->clipPlane = *clipPlane;

    return 0;
}

int argViewportSetDispMethod( ARGViewportHandle *argVPhandle, int dispMethod )
{
    if( argVPhandle == NULL ) return -1;

    argClearDisplayList( &(argVPhandle->dispList) );
    argClearDisplayList( &(argVPhandle->dispListHalf) );

    argVPhandle->dispMethod = dispMethod;

    return 0;
}

int argViewportSetDispMode( ARGViewportHandle *argVPhandle, int dispMode )
{
    if( argVPhandle == NULL ) return -1;

    argVPhandle->dispMode = dispMode;

    return 0;
}

int argViewportSetFlipMode( ARGViewportHandle *argVPhandle, int flipMode )
{
    if( argVPhandle == NULL ) return -1;

    argVPhandle->flipMode = flipMode;

    return 0;
}

int argViewportSetDistortionMode( ARGViewportHandle *argVPhandle, int distortionMode )
{
    if( argVPhandle == NULL ) return -1;

    argClearDisplayList( &(argVPhandle->dispList) );
    argClearDisplayList( &(argVPhandle->dispListHalf) );

    argVPhandle->distortionMode = distortionMode;

    return 0;
}

int argViewportSetPixFormat( ARGViewportHandle *argVPhandle, int pixFormat )
{
    if( argVPhandle == NULL ) return -1;

    argVPhandle->pixFormat = pixFormat;

    return 0;
}

int argViewportSetDispScale( ARGViewportHandle *argVPhandle, ARdouble scale )
{
    if( argVPhandle == NULL ) return -1;

    argVPhandle->scale = scale;

    return 0;
}

ARGViewportHandle *argGetCurrentVPHandle( void )
{
    return currentVP;
}

int argSetCurrentVPHandle( ARGViewportHandle *vp )
{
    currentVP = vp;
    return 0;
}

void argClearDisplayList( ARGDisplayList *dispList )
{
    GLuint    texId;

    if( dispList->texId != -1 ) {
        texId = dispList->texId;
        glDeleteTextures( 1, &(texId) );
        dispList->texId = -1;
    }

    if( dispList->listIndex != -1 ) {
        glDeleteLists( dispList->listIndex, 1 );
        dispList->listIndex = -1;
    }

    return;
}

ARGWindowHandle *argGetWinHandle( void )
{
    return winHandle;
}

