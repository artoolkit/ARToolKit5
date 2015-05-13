/*
 *  argDrawMode.c
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

static void argConvGLcpara( const ARParam *cparam, ARGClipPlane clipPlane, ARdouble m[16] );


int argDrawMode2D( ARGViewportHandle *vp )
{
    ARGViewport     *viewport;
    int              winXsize, winYsize;

    argSetCurrentVPHandle(vp);

    viewport = &(vp->viewport);
    argGetWindowSize( &winXsize, &winYsize );

    glViewport(viewport->sx, winYsize-viewport->sy-viewport->ysize, viewport->xsize, viewport->ysize);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    switch( vp->flipMode ) {
      case AR_GL_FLIP_H | AR_GL_FLIP_V:
        glOrtho(viewport->xsize-0.5, -0.5, -0.5, viewport->ysize-0.5, 1.0, -1.0);
        break;
      case AR_GL_FLIP_H:
        glOrtho(viewport->xsize-0.5, -0.5, viewport->ysize-0.5, -0.5, 1.0, -1.0);
        break;
      case AR_GL_FLIP_V:
        glOrtho(-0.5, viewport->xsize-0.5, -0.5, viewport->ysize-0.5, 1.0, -1.0);
        break;
      default:
        glOrtho(-0.5, viewport->xsize-0.5, viewport->ysize-0.5, -0.5,  1.0, -1.0);
        break;
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    return 0;
}

int argDrawMode3D( ARGViewportHandle *vp )
{
    ARGViewport     *viewport;
    int              winXsize, winYsize;
    ARdouble           sx, sy, offx, offy;
    ARdouble           ix, iy;
    ARdouble           m[16];

    argSetCurrentVPHandle(vp);

    if( argGetCurrentScale(vp, &sx, &sy, &offx, &offy) < 0 ) return -1;
    ix = vp->cparam->xsize;
    iy = vp->cparam->ysize;

    viewport = &(vp->viewport);
    argGetWindowSize( &winXsize, &winYsize );

    if( vp->cparam == NULL ) return -1;
    argConvGLcpara( vp->cparam, vp->clipPlane, m );
    switch( vp->flipMode ) {
      case AR_GL_FLIP_H | AR_GL_FLIP_V:
        m[0]  *= -1.0;
        m[4]  *= -1.0;
        m[8]  *= -1.0;
        m[12] *= -1.0;
        m[1]  *= -1.0;
        m[5]  *= -1.0;
        m[9]  *= -1.0;
        m[13] *= -1.0;
        break;
      case AR_GL_FLIP_H:
        m[0]  *= -1.0;
        m[4]  *= -1.0;
        m[8]  *= -1.0;
        m[12] *= -1.0;
        break;
      case AR_GL_FLIP_V:
        m[1]  *= -1.0;
        m[5]  *= -1.0;
        m[9]  *= -1.0;
        m[13] *= -1.0;
        break;
    }

    //glViewport(viewport->sx, winYsize-viewport->sy-viewport->ysize, viewport->xsize, viewport->ysize);
    glViewport(viewport->sx + (int)offx, winYsize - viewport->sy - (GLint)(iy*sy) - (GLint)offy, (GLint)(ix*sx), (GLint)(iy*sy));
    glMatrixMode(GL_PROJECTION);
#ifdef ARDOUBLE_IS_FLOAT
    glLoadMatrixf( m );
#else
    glLoadMatrixd( m );
#endif
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    return 0;
}

void argConvGlpara( ARdouble para[3][4], ARdouble gl_para[16] )
{
    int     i, j;

    for( j = 0; j < 3; j++ ) { 
        for( i = 0; i < 4; i++ ) {
            gl_para[i*4+j] = para[j][i];
        }
    }
    gl_para[0*4+3] = gl_para[1*4+3] = gl_para[2*4+3] = 0.0;
    gl_para[3*4+3] = 1.0;
}

static void argConvGLcpara( const ARParam *cparam, ARGClipPlane clipPlane, ARdouble m[16] )
{
    ARdouble   icpara[3][4];
    ARdouble   trans[3][4];
    ARdouble   p[3][3], q[4][4];
    ARdouble   farClip, nearClip;
    int      width, height;
    int      i, j;

    width  = cparam->xsize;
    height = cparam->ysize;
    farClip    = clipPlane.farClip;
    nearClip   = clipPlane.nearClip;

    if( arParamDecompMat(cparam->mat, icpara, trans) < 0 ) {
        ARLOGe("gConvGLcpara: Parameter error!!\n");
        exit(0);
    }
    for( i = 0; i < 4; i++ ) {
        icpara[1][i] = (height-1)*(icpara[2][i]) - icpara[1][i];
    }


    for( i = 0; i < 3; i++ ) {
        for( j = 0; j < 3; j++ ) {
            p[i][j] = icpara[i][j] / icpara[2][2];
        }
    }
    q[0][0] = (2.0 * p[0][0] / (width-1));
    q[0][1] = (2.0 * p[0][1] / (width-1));
    q[0][2] = ((2.0 * p[0][2] / (width-1))  - 1.0);
    q[0][3] = 0.0;

    q[1][0] = 0.0;
    q[1][1] = (2.0 * p[1][1] / (height-1));
    q[1][2] = ((2.0 * p[1][2] / (height-1)) - 1.0);
    q[1][3] = 0.0;

    q[2][0] = 0.0;
    q[2][1] = 0.0;
    q[2][2] = (farClip + nearClip)/(farClip - nearClip);
    q[2][3] = -2.0 * farClip * nearClip / (farClip - nearClip);

    q[3][0] = 0.0;
    q[3][1] = 0.0;
    q[3][2] = 1.0;
    q[3][3] = 0.0;

    for( i = 0; i < 4; i++ ) {
        for( j = 0; j < 3; j++ ) {
            m[i+j*4] = q[i][0] * trans[0][j]
                     + q[i][1] * trans[1][j]
                     + q[i][2] * trans[2][j];
        }
        m[i+3*4] = q[i][0] * trans[0][3]
                 + q[i][1] * trans[1][3]
                 + q[i][2] * trans[2][3]
                 + q[i][3];
    }
}
