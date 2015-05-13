/*
 *  argDraw.c
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

static int argDrawPointDirect( ARdouble x1, ARdouble y1 );
static int argDrawLineDirect( ARdouble x1, ARdouble y1, ARdouble x2, ARdouble y2 );
static int argDrawLineConversion( ARdouble x1, ARdouble y1, ARdouble x2, ARdouble y2,
                                  int (*conv)(const ARdouble dist_factor[AR_DIST_FACTOR_NUM_MAX], const ARdouble ix, const ARdouble iy, ARdouble *ox, ARdouble *oy, const int dist_function_version) );
static int argDrawSquare2Direct( ARdouble vertex[4][2] );
static int argDrawSquare2Conversion( ARdouble vertex[4][2],
                                     int (*conv)(const ARdouble dist_factor[AR_DIST_FACTOR_NUM_MAX], const ARdouble ix, const ARdouble iy, ARdouble *ox, ARdouble *oy, const int dist_function_version) );
static int argDrawStringsDirect( char *string, ARdouble x1, ARdouble y1 );


int argDrawSquare2ByIdealPos( ARdouble vertex[4][2] )
{
    ARGViewportHandle    *vp;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_ENABLE ) {
        argDrawSquare2Direct( vertex );
    }
    else if( vp->cparam != NULL ) {
        argDrawSquare2Conversion( vertex, arParamIdeal2Observ );
    }
    return 0;
}

int argDrawSquare2ByObservedPos( ARdouble vertex[4][2] )
{
    ARGViewportHandle    *vp;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_DISABLE ) {
        argDrawSquare2Direct( vertex );
    }
    else if( vp->cparam != NULL ) {
        argDrawSquare2Conversion( vertex, arParamObserv2Ideal );
    }
    return 0;
}

int argDrawSquareByIdealPos( ARdouble vertex[4][2] )
{
    argDrawLineByIdealPos( vertex[0][0], vertex[0][1], vertex[1][0], vertex[1][1] );
    argDrawLineByIdealPos( vertex[1][0], vertex[1][1], vertex[2][0], vertex[2][1] );
    argDrawLineByIdealPos( vertex[2][0], vertex[2][1], vertex[3][0], vertex[3][1] );
    argDrawLineByIdealPos( vertex[3][0], vertex[3][1], vertex[0][0], vertex[0][1] );

    return 0;
}

int argDrawSquareByObservedPos( ARdouble vertex[4][2] )
{
    argDrawLineByObservedPos( vertex[0][0], vertex[0][1], vertex[1][0], vertex[1][1] );
    argDrawLineByObservedPos( vertex[1][0], vertex[1][1], vertex[2][0], vertex[2][1] );
    argDrawLineByObservedPos( vertex[2][0], vertex[2][1], vertex[3][0], vertex[3][1] );
    argDrawLineByObservedPos( vertex[3][0], vertex[3][1], vertex[0][0], vertex[0][1] );

    return 0;
}

int argDrawLineByIdealPos( ARdouble x1, ARdouble y1, ARdouble x2, ARdouble y2 )
{
    ARGViewportHandle    *vp;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_ENABLE ) {
        argDrawLineDirect( x1, y1, x2, y2 );
    }
    else if( vp->cparam != NULL ) {
        argDrawLineConversion( x1, y1, x2, y2, arParamIdeal2Observ );
    }

    return 0;
}

int argDrawLineByObservedPos( ARdouble x1, ARdouble y1, ARdouble x2, ARdouble y2 )
{
    ARGViewportHandle    *vp;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_DISABLE ) {
        argDrawLineDirect( x1, y1, x2, y2 );
    }
    else if( vp->cparam != NULL ) {
        argDrawLineConversion( x1, y1, x2, y2, arParamObserv2Ideal );
    }

    return 0;
}

int argDrawPointByIdealPos( ARdouble x1, ARdouble y1 )
{
    ARGViewportHandle    *vp;
    ARdouble                xx1, yy1;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_ENABLE ) {
        xx1 = x1;  yy1 = y1;
    }
    else if( vp->cparam != NULL ) {
        arParamIdeal2Observ( vp->cparam->dist_factor, x1, y1, &xx1, &yy1, vp->cparam->dist_function_version );
    }
    else return -1;

    return argDrawPointDirect( xx1, yy1 );
}

int argDrawPointByObservedPos( ARdouble x1, ARdouble y1 )
{
    ARGViewportHandle    *vp;
    ARdouble                xx1, yy1;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_DISABLE ) {
        xx1 = x1;  yy1 = y1;
    }
    else if( vp->cparam != NULL ) {
        arParamObserv2Ideal( vp->cparam->dist_factor, x1, y1, &xx1, &yy1, vp->cparam->dist_function_version );
    }
    else return -1;

    return argDrawPointDirect( xx1, yy1 );
}

static int argDrawPointDirect( ARdouble x1, ARdouble y1 )
{
    ARGViewportHandle    *vp;
    ARdouble                xx1, yy1;
    ARdouble                s1, s2, offx, offy;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( argGetCurrentScale(vp, &s1, &s2, &offx, &offy) < 0 ) return -1;

    xx1 = x1 * s1 + offx;
    yy1 = y1 * s2 + offy;

    glBegin(GL_POINTS);
    glVertex2f( (float)xx1, (float)yy1 );
    glEnd();

    return 0;
}

static int argDrawLineDirect( ARdouble x1, ARdouble y1, ARdouble x2, ARdouble y2 )
{
    ARGViewportHandle    *vp;
    ARdouble                xx1, yy1, xx2, yy2;
    ARdouble                s1, s2, offx, offy;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( argGetCurrentScale(vp, &s1, &s2, &offx, &offy) < 0 ) return -1;

    xx1 = x1 * s1 + offx;
    yy1 = y1 * s2 + offy;
    xx2 = x2 * s1 + offx;
    yy2 = y2 * s2 + offy;

    glBegin(GL_LINES);
    glVertex2f( (float)xx1, (float)yy1 );
    glVertex2f( (float)xx2, (float)yy2 );
    glEnd();

    return 0;
}

static int argDrawLineConversion( ARdouble x1, ARdouble y1, ARdouble x2, ARdouble y2,
                                  int (*conv)(const ARdouble dist_factor[AR_DIST_FACTOR_NUM_MAX], const ARdouble ix, const ARdouble iy, ARdouble *ox, ARdouble *oy, const int dist_function_version) )
{
    ARGViewportHandle    *vp;
    ARdouble                s1, s2, offx, offy;
    ARdouble                dx, dy;
    ARdouble                xx1, yy1, xx2, yy2;
    int                   n;
    int                   i;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( argGetCurrentScale(vp, &s1, &s2, &offx, &offy) < 0 ) return -1;

    dx = x1 - x2;
    if( dx < 0 ) dx = -dx;
    dy = y1 - y2;
    if( dy < 0 ) dy = -dy;

    if( dx > dy ) {
        if( x1 > x2 ) {
            xx1 = x1;
            x1 = x2;
            x2 = xx1;
            yy1 = y1;
            y1 = y2;
            y2 = yy1;
        }
        n = (int)dx / AR_GL_MIN_LINE_SEGMENT + 1;
        glBegin(GL_LINE_STRIP);
        for( i = 0; i <= n; i++ ) {
            xx1 = (x2 - x1)*(ARdouble)i/(ARdouble)n + x1;
            yy1 = (y2 - y1)*(ARdouble)i/(ARdouble)n + y1;
            (*conv)( vp->cparam->dist_factor, xx1, yy1, &xx2, &yy2, vp->cparam->dist_function_version );
            glVertex2f( (float)(xx2*s1 + offx), (float)(yy2*s2 + offy) );
        }
        glEnd();
    }
    else {
        if( y1 > y2 ) {
            xx1 = x1;
            x1 = x2;
            x2 = xx1;
            yy1 = y1;
            y1 = y2;
            y2 = yy1;
        }
        n = (int)dy / AR_GL_MIN_LINE_SEGMENT + 1;
        glBegin(GL_LINE_STRIP);
        for( i = 0; i <= n; i++ ) {
            xx1 = (x2 - x1)*(ARdouble)i/(ARdouble)n + x1;
            yy1 = (y2 - y1)*(ARdouble)i/(ARdouble)n + y1;
            (*conv)( vp->cparam->dist_factor, xx1, yy1, &xx2, &yy2, vp->cparam->dist_function_version );
            glVertex2f( (float)(xx2*s1+offx), (float)(yy2*s2+offy) );
        }
        glEnd();
    }

    return 0;
}

static int argDrawSquare2Direct( ARdouble vertex[4][2] )
{
    ARGViewportHandle    *vp;
    ARdouble                xx1, yy1;
    ARdouble                s1, s2, offx, offy;
    int                   i;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( argGetCurrentScale(vp, &s1, &s2, &offx, &offy) < 0 ) return -1;

    glBegin(GL_QUADS);
    for( i = 0; i < 4; i++ ) {
        xx1 = vertex[i][0] * s1 + offx;
        yy1 = vertex[i][1] * s2 + offy;
        glVertex2f( (float)xx1, (float)yy1 );
    }
    glEnd();

    return 0;
}

static int argDrawSquare2Conversion( ARdouble vertex[4][2],
                                     int (*conv)(const ARdouble dist_factor[AR_DIST_FACTOR_NUM_MAX], const ARdouble ix, const ARdouble iy, ARdouble *ox, ARdouble *oy, const int dist_function_version) )
{
    ARGViewportHandle    *vp;
    ARdouble                s1, s2, offx, offy;
    ARdouble                xx1, yy1;
    int                   i;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( argGetCurrentScale(vp, &s1, &s2, &offx, &offy) < 0 ) return -1;

    glBegin(GL_QUADS);
    for( i = 0; i < 4; i++ ) {
        (*conv)( vp->cparam->dist_factor, vertex[i][0], vertex[i][1], &xx1, &yy1, vp->cparam->dist_function_version );
        glVertex2f( (float)(xx1*s1+offx), (float)(yy1*s2+offy) );
    }
    glEnd();

    return 0;
}

int argDrawStringsByIdealPos( char *string, ARdouble sx, ARdouble sy )
{
    ARGViewportHandle    *vp;
    ARdouble                xx1, yy1;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_ENABLE ) {
        xx1 = sx;  yy1 = sy;
    }
    else if( vp->cparam != NULL ) {
        arParamIdeal2Observ( vp->cparam->dist_factor, sx, sy, &xx1, &yy1, vp->cparam->dist_function_version );
    }
    else return -1;

    return argDrawStringsDirect( string, xx1, yy1 );
}

int argDrawStringsByObservedPos( char *string, ARdouble sx, ARdouble sy )
{
    ARGViewportHandle    *vp;
    ARdouble                xx1, yy1;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( vp->distortionMode == AR_GL_DISTORTION_COMPENSATE_DISABLE ) {
        xx1 = sx;  yy1 = sy;
    }
    else if( vp->cparam != NULL ) {
        arParamObserv2Ideal( vp->cparam->dist_factor, sx, sy, &xx1, &yy1, vp->cparam->dist_function_version );
    }
    else return -1;

    return argDrawStringsDirect( string, xx1, yy1 );
}

static int argDrawStringsDirect( char *string, ARdouble x1, ARdouble y1 )
{
    ARGViewportHandle    *vp;
    ARdouble                s1, s2, offx, offy;
    int                   i;

    if( (vp=argGetCurrentVPHandle()) == NULL ) return-1;

    if( argGetCurrentScale(vp, &s1, &s2, &offx, &offy) < 0 ) return -1;

    x1 = x1 * s1 + offx;
    y1 = y1 * s2 + offy;

    glRasterPos2d(x1, y1);

    for( i = 0; i < (int)strlen(string); i++ ) {
        if(string[i] != '\n' ) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string[i]);
        }
        else {
            y1+=24;
            glRasterPos2d(x1, y1);
        }
    }

    return 0;
}

int argGetCurrentScale(ARGViewportHandle *vp, ARdouble *sx, ARdouble *sy, ARdouble *offx, ARdouble *offy )
{
    int     wx, wy;
    int     ix, iy;

    if( vp == NULL ) return -1;
    if( vp->cparam == NULL ) return -1;

    ix = vp->cparam->xsize;
    iy = vp->cparam->ysize;
    wx = vp->viewport.xsize;
    wy = vp->viewport.ysize;

    if( vp->dispMode == AR_GL_DISP_MODE_FIT_TO_VIEWPORT ) {
        *sx = (ARdouble)wx / (ARdouble)ix;
        *sy = (ARdouble)wy / (ARdouble)iy;
        *offx = *offy = 0.0;
    }
    else if( vp->dispMode == AR_GL_DISP_MODE_FIT_TO_VIEWPORT_KEEP_ASPECT_RATIO ) {
        *sx = (ARdouble)wx / (ARdouble)ix;
        *sy = (ARdouble)wy / (ARdouble)iy;
        if( *sx < *sy ) {
            *sy = *sx;
            *offx = 0.0;
            *offy = ((ARdouble)wy - (*sy * (ARdouble)iy)) / 2.0;
        }
        else {
            *sx = *sy;
            *offx = ((ARdouble)wx - (*sx * (ARdouble)ix)) / 2.0;
            *offy = 0.0;
        }
    }
    else if( vp->dispMode == AR_GL_DISP_MODE_FIT_TO_VIEWPORT_HEIGHT_KEEP_ASPECT_RATIO ) {
        *sx = *sy = (ARdouble)wy / (ARdouble)iy;
        *offx = ((ARdouble)wx - (*sx * (ARdouble)ix)) / 2.0;
        *offy = 0.0;
    }
    else if( vp->dispMode == AR_GL_DISP_MODE_FIT_TO_VIEWPORT_WIDTH_KEEP_ASPECT_RATIO ) {
        *sx = *sy = (ARdouble)wx / (ARdouble)ix;
        *offx = 0.0;
        *offy = ((ARdouble)wy - (*sy * (ARdouble)iy)) / 2.0;
    }
    else if( vp->dispMode == AR_GL_DISP_MODE_USE_SPECIFIED_SCALE ) {
        *sx = *sy = vp->scale;
        *offx = ((ARdouble)wx - (*sx * (ARdouble)ix)) / 2.0;
        *offy = ((ARdouble)wy - (*sy * (ARdouble)iy)) / 2.0;
    }
    else return -1;

    return 0;
}
