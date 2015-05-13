/*
 *  argDrawImage.c
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
#include "argPrivate.h"

static int    argDrawImageDrawPixels( ARGViewportHandle *vp, ARUint8 *image, int xsize, int ysize );
static int    argDrawImageTexMap    ( ARGViewportHandle *vp, ARUint8 *image, int xsize, int ysize, ARGDisplayList *dl );
static int    argDrawImageTexMapRect( ARGViewportHandle *vp, ARUint8 *image, int xsize, int ysize, ARGDisplayList *dl );
static int    argGetImageScale      ( ARGViewportHandle *vp, int xsize, int ysize,
                                      float *sx, float *sy, float *offx, float *offy );
static GLuint argUpdateImageTexture ( ARGViewportHandle *vp, ARUint8 *image, int xsize, int ysize,
                                      int pixFormat, int full_half, GLenum target, GLuint glid );
static GLuint argGenImageTexture    ( int tx, int ty, int full_half, GLenum target );


int argDrawImage( ARUint8 *image )
{
    ARGViewportHandle    *vp;
    int                   xsize, ysize;
    ARGDisplayList       *dl;
	int                   ret;
	//GLint                 texEnvModeSave;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;
    if( vp->cparam == NULL ) return -1;
    xsize = vp->cparam->xsize;
    ysize = vp->cparam->ysize;
    dl = &(vp->dispList);

	if (!image) return (-1);
    if( vp->dispMethod == AR_GL_DISP_METHOD_GL_DRAW_PIXELS ) {
        ret = argDrawImageDrawPixels( vp, image, xsize, ysize );
    }
    else {
		//glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &texEnvModeSave); // Save GL texture environment mode.
		//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        if( argGetWinHandle()->possibleTextureRectangle == 1 ) {
            ret = argDrawImageTexMapRect( vp, image, xsize, ysize, dl );
        }
        else {
            ret = argDrawImageTexMap( vp, image, xsize, ysize, dl );
        }
		//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnvModeSave); // Restore GL texture environment mode.

    }
	return (ret);
}

int argDrawImageHalf( ARUint8 *image )
{
    ARGViewportHandle    *vp;
    int                   xsize, ysize;
    ARGDisplayList       *dl;

	if (!image) return (-1);
    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;
    if( vp->cparam == NULL ) return -1;
    xsize = vp->cparam->xsize / 2;
    ysize = vp->cparam->ysize / 2;
    dl = &(vp->dispListHalf);

    if( vp->dispMethod == AR_GL_DISP_METHOD_GL_DRAW_PIXELS ) {
        return argDrawImageDrawPixels( vp, image, xsize, ysize );
    }
    else {
        if( argGetWinHandle()->possibleTextureRectangle == 1 ) {
            return argDrawImageTexMapRect( vp, image, xsize, ysize, dl );
        }
        else {
            return argDrawImageTexMap( vp, image, xsize, ysize, dl );
        }
    }
}



static int argDrawImageDrawPixels( ARGViewportHandle *vp, ARUint8 *image, int xsize, int ysize )
{
    float                 s1, s2, offx, offy;
    int                   pixFormat;

    if( vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_ENABLE ) return -1;

    if( argGetImageScale(vp, xsize, ysize, &s1, &s2, &offx, &offy) < 0 ) return -1;
    pixFormat = vp->pixFormat;

    offx -= 0.5f;
    offy -= 0.5f;
    if( offy < 0.0f ) offy = 0.0f;
    glRasterPos3f( offx, offy, 1.0f );
    switch( vp->flipMode ) {
      case AR_GL_FLIP_H | AR_GL_FLIP_V:
        glPixelZoom( -s1, s2);
        break;
      case AR_GL_FLIP_H:
        glPixelZoom( -s1, -s2);
        break;
      case AR_GL_FLIP_V:
        glPixelZoom( s1, s2);
        break;
      default:
        glPixelZoom( s1, -s2);
        break;
    }

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    switch( pixFormat ) {
        case AR_PIXEL_FORMAT_RGB:
            glDrawPixels( xsize, ysize, GL_RGB, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_BGR:
            glDrawPixels( xsize, ysize, GL_BGR, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_RGBA:
            glDrawPixels( xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_BGRA:
            glDrawPixels( xsize, ysize, GL_BGRA, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_ABGR:
            glDrawPixels( xsize, ysize, GL_ABGR_EXT, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_ARGB:
#ifdef AR_BIG_ENDIAN
            glDrawPixels( xsize, ysize, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image );
#else
            glDrawPixels( xsize, ysize, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, image );
#endif
            break;
        case AR_PIXEL_FORMAT_MONO:
        case AR_PIXEL_FORMAT_420v:
        case AR_PIXEL_FORMAT_420f:
            glDrawPixels( xsize, ysize, GL_LUMINANCE, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_2vuy:
#ifdef AR_BIG_ENDIAN
            glDrawPixels( xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, image );
#else
            glDrawPixels( xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, image );
#endif
            break;
        case AR_PIXEL_FORMAT_yuvs:
#ifdef AR_BIG_ENDIAN
            glDrawPixels( xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, image );
#else
            glDrawPixels( xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, image );
#endif
            break;
		default: return -1;
    }

    return 0;

}
static int argDrawImageTexMap( ARGViewportHandle *vp, ARUint8 *image, int xsize, int ysize, ARGDisplayList *dl )
{
    int                   dispMethod;
    int                   distMode;
    int                   tx, ty;
    int                   pixFormat;
    float                 s1, s2, offx, offy;
    ARdouble                px, py, qy, z;
    ARdouble                x1, x2;
    ARdouble                y1, y2;
    int                   i, j;

    dispMethod = (vp->dispMethod == AR_GL_DISP_METHOD_TEXTURE_MAPPING_FIELD)? 0: 1;
    distMode = (vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_DISABLE)? 0: 1;
    if( argGetImageScale(vp, xsize, ysize, &s1, &s2, &offx, &offy) < 0 ) return -1;
    pixFormat = vp->pixFormat;
    tx = 1;
    while( tx < xsize ) tx *= 2;
    ty = 1;
    while( ty < ysize ) ty *= 2;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef( offx, offy, 0.0f );
    glScalef( s1, s2, 1.0f );
    glEnable( GL_TEXTURE_2D );

    if( dl->texId < 0 ) {
        dl->texId = argGenImageTexture( tx, ty, dispMethod, GL_TEXTURE_2D );
    }
    argUpdateImageTexture( vp, image, xsize, ysize, pixFormat, dispMethod, GL_TEXTURE_2D, dl->texId );

    glBindTexture( GL_TEXTURE_2D, dl->texId );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    if( dl->listIndex < 0 ) {
        dl->listIndex = glGenLists(1);
        glNewList(dl->listIndex, GL_COMPILE_AND_EXECUTE);
        z = 1.0;
        if( distMode == 1 ) {
            qy = ysize * 0 / (ARdouble)AR_GL_TEXTURE_MESH_NUM;
            for( j = 1; j <= AR_GL_TEXTURE_MESH_NUM; j++ ) {
                py = qy;
                qy = ysize * j / (ARdouble)AR_GL_TEXTURE_MESH_NUM;

                glBegin( GL_QUAD_STRIP );
                for( i = 0; i <= AR_GL_TEXTURE_MESH_NUM; i++ ) {
                    px = xsize * i / (ARdouble)AR_GL_TEXTURE_MESH_NUM;

                    arParamObserv2Ideal( vp->cparam->dist_factor, px, py, &x1, &y1, vp->cparam->dist_function_version );
                    arParamObserv2Ideal( vp->cparam->dist_factor, px, qy, &x2, &y2, vp->cparam->dist_function_version );

                    glTexCoord2d( px/(ARdouble)tx, py/(ARdouble)ty ); glVertex3d( x1-0.5, y1-0.5, z );
                    glTexCoord2d( px/(ARdouble)tx, qy/(ARdouble)ty ); glVertex3d( x2-0.5, y2-0.5, z );
                }
                glEnd();
            }
        }
        else {
            glBegin( GL_QUAD_STRIP );
            glTexCoord2d( 0.0,                      0.0 );                      glVertex3d( -0.5,           -0.5,           z );
            glTexCoord2d( 0.0,                      (ARdouble)ysize/(ARdouble)ty ); glVertex3d( -0.5,           (ARdouble)ysize-0.5, z );
            glTexCoord2d( (ARdouble)xsize/(ARdouble)tx, 0.0 );                      glVertex3d( (ARdouble)xsize-0.5, -0.5,           z );
            glTexCoord2d( (ARdouble)xsize/(ARdouble)tx, (ARdouble)ysize/(ARdouble)ty ); glVertex3d( (ARdouble)xsize-0.5, (ARdouble)ysize-0.5, z );
            glEnd();
        }
        glEndList();
    }
    else {
        glCallList( dl->listIndex );
    }
    glBindTexture( GL_TEXTURE_2D, 0 );

    glDisable( GL_TEXTURE_2D );
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    return 0;
}

static int argDrawImageTexMapRect( ARGViewportHandle *vp, ARUint8 *image, int xsize, int ysize, ARGDisplayList *dl )
{
    ARParam               wparam;
    int                   dispMethod;
    int                   distMode;
    int                   tx, ty;
    int                   pixFormat;
    float                 s1, s2, offx, offy;
    ARdouble                px, py, qy, z, dy;
    ARdouble                x1, x2;
    ARdouble                y1, y2;
    int                   i, j;

    dispMethod = (vp->dispMethod == AR_GL_DISP_METHOD_TEXTURE_MAPPING_FIELD)? 0: 1;
    distMode = (vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_DISABLE)? 0: 1;
    if( argGetImageScale(vp, xsize, ysize, &s1, &s2, &offx, &offy) < 0 ) return -1;
    pixFormat = vp->pixFormat;
    tx = xsize;
    ty = ysize;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef( offx, offy, 0.0f );
    glScalef( s1, s2, 1.0f );
    glEnable( GL_TEXTURE_RECTANGLE );

    if( dl->texId < 0 ) {
        dl->texId = argGenImageTexture( tx, ty, dispMethod,
        GL_TEXTURE_RECTANGLE );
    }
    argUpdateImageTexture( vp, image, xsize, ysize, pixFormat, dispMethod, GL_TEXTURE_RECTANGLE, dl->texId );

    glBindTexture( GL_TEXTURE_RECTANGLE, dl->texId );
    glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP );
    //glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    if( dl->listIndex < 0 ) {
        arParamChangeSize( vp->cparam, xsize, ysize, &wparam );
        dl->listIndex = glGenLists(1);
        glNewList(dl->listIndex, GL_COMPILE_AND_EXECUTE);
        z = 1.0;
        dy = (dispMethod)? 1.0: 2.0;
        if( distMode == 1 ) {
            qy = ysize * 0 / (ARdouble)AR_GL_TEXTURE_MESH_NUM;
            for( j = 1; j <= AR_GL_TEXTURE_MESH_NUM; j++ ) {
                py = qy;
                qy = ysize * j / (ARdouble)AR_GL_TEXTURE_MESH_NUM;

                glBegin( GL_QUAD_STRIP );
                for( i = 0; i <= AR_GL_TEXTURE_MESH_NUM; i++ ) {
                    px = xsize * i / (ARdouble)AR_GL_TEXTURE_MESH_NUM;

                    arParamObserv2Ideal( wparam.dist_factor, px, py, &x1, &y1, wparam.dist_function_version );
                    arParamObserv2Ideal( wparam.dist_factor, px, qy, &x2, &y2, wparam.dist_function_version );
    
                    glTexCoord2d( px, py/dy ); glVertex3d( x1-0.5, y1-0.5, z );
                    glTexCoord2d( px, qy/dy ); glVertex3d( x2-0.5, y2-0.5, z );
                }
                glEnd();
            }
        }
        else {
            glBegin( GL_QUAD_STRIP );
            glTexCoord2d( 0.0,           0.0 );              glVertex3d(-0.5,           -0.5,           z );
            glTexCoord2d( 0.0,           (ARdouble)ysize/dy ); glVertex3d(-0.5,           (ARdouble)ysize-0.5, z );
            glTexCoord2d( (ARdouble)xsize, 0.0 );              glVertex3d( (ARdouble)xsize-0.5, -0.5,           z );
            glTexCoord2d( (ARdouble)xsize, (ARdouble)ysize/dy ); glVertex3d( (ARdouble)xsize-0.5, (ARdouble)ysize-0.5, z );
            glEnd();
        }
        glEndList();
    }
    else {
        glCallList( dl->listIndex );
    }
    glBindTexture( GL_TEXTURE_RECTANGLE, 0 );

    glDisable( GL_TEXTURE_RECTANGLE );
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

	return 0;
}

static int argGetImageScale( ARGViewportHandle *vp, int xsize, int ysize,
                             float *sx, float *sy, float *offx, float *offy )
{
    int    wx, wy;

    wx = vp->viewport.xsize;
    wy = vp->viewport.ysize;
    if( vp->dispMode == AR_GL_DISP_MODE_FIT_TO_VIEWPORT ) {
        *sx = (float)wx / (float)xsize;
        *sy = (float)wy / (float)ysize;
        *offx = 0.0f;
        *offy = 0.0f;
    }
    else if( vp->dispMode == AR_GL_DISP_MODE_FIT_TO_VIEWPORT_KEEP_ASPECT_RATIO ) {
        *sx = (float)wx / (float)xsize;
        *sy = (float)wy / (float)ysize;
        if( *sx < *sy ) {
            *sy = *sx;
            *offx = 0.0f;
            *offy = ((float)wy - (*sy * (float)ysize)) / 2.0f;
        }
        else {
            *sx = *sy;
            *offx = ((float)wx - (*sx * (float)xsize)) / 2.0f;
            *offy = 0.0f;
        }
    }
    else if( vp->dispMode == AR_GL_DISP_MODE_FIT_TO_VIEWPORT_HEIGHT_KEEP_ASPECT_RATIO ) {
        *sx = *sy = (float)wy / (float)ysize;
        *offx = ((float)wx - (*sx * (float)xsize)) / 2.0f;
        *offy = 0.0f;
    }
    else if( vp->dispMode == AR_GL_DISP_MODE_FIT_TO_VIEWPORT_WIDTH_KEEP_ASPECT_RATIO ) {
        *sx = *sy = (float)wx / (float)xsize;
        *offx = 0.0f;
        *offy = ((float)wy - (*sy * (float)ysize)) / 2.0f;
    }
    else if( vp->dispMode == AR_GL_DISP_MODE_USE_SPECIFIED_SCALE ) {
        *sx = *sy = vp->scale;
        *offx = ((float)wx - (*sx * (float)xsize)) / 2.0f;
        *offy = ((float)wy - (*sy * (float)ysize)) / 2.0f;
    }
    else return -1;

    return 0;
}


static GLuint argGenImageTexture( int tx, int ty, int full_half, GLenum target )
{
    static ARUint8 *tmpImage = NULL;
    GLuint          glid;

    glGenTextures(1, &glid);
    glBindTexture( target, glid );

    glTexParameterf( target, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( target, GL_TEXTURE_WRAP_T, GL_CLAMP );
    //glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    if( full_half == 0 ) {
        ty /= 2;
    }
#ifdef __APPLE__
    //glTexParameterf(target, GL_TEXTURE_PRIORITY, 0.0);
    //glTexParameteri(target, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);
    //glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 1);
#endif

    if( tmpImage != NULL ) free( tmpImage );
    arMalloc( tmpImage, ARUint8, tx*ty );
    glTexImage2D( target, 0, 3, tx, ty, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tmpImage );

    glBindTexture( target, 0 );

    return glid;
}

static GLuint argUpdateImageTexture( ARGViewportHandle *vp, ARUint8 *image, int xsize, int ysize,
                                     int pixFormat, int full_half, GLenum target, GLuint glid )
{
    GLint    saveMatrixMode;

    glBindTexture( target, glid );

    glGetIntegerv( GL_MATRIX_MODE, &saveMatrixMode );
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(saveMatrixMode);

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    if( full_half == 0 ) {
        glPixelStorei( GL_UNPACK_ROW_LENGTH,   xsize*2 );
        ysize /= 2;
    }
    else {
        glPixelStorei( GL_UNPACK_ROW_LENGTH,   xsize );
    }
#ifdef __APPLE__
    //glTexParameterf(target, GL_TEXTURE_PRIORITY, 0.0);
    //glTexParameteri(target, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);
    //glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 1);
#endif

    switch( pixFormat ) {
        case AR_PIXEL_FORMAT_RGB:
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_RGB, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_BGR:
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_BGR, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_RGBA:
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_BGRA:
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_BGRA, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_ABGR:
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_ABGR_EXT, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_ARGB:
#ifdef AR_BIG_ENDIAN
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, image );
#else
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, image );
#endif
            break;
        case AR_PIXEL_FORMAT_MONO:
        case AR_PIXEL_FORMAT_420v:
        case AR_PIXEL_FORMAT_420f:
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_LUMINANCE, GL_UNSIGNED_BYTE, image );
            break;
        case AR_PIXEL_FORMAT_2vuy:
#ifdef AR_BIG_ENDIAN
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, image );
#else
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, image );
#endif
            break;
        case AR_PIXEL_FORMAT_yuvs:
#ifdef AR_BIG_ENDIAN
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, image );
#else
            glTexSubImage2D( target, 0, 0, 0, xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, image );
#endif
            break;
    }

    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

    glBindTexture( target, 0 );

    return 0;
}
