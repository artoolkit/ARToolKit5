/*
 *  calib_inp.h
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifdef _WIN32
#include <windows.h>
#endif
#include <math.h>
#include <stdio.h>
#include <AR/ar.h>
#include <AR/param.h>
#include <AR/matrix.h>
#include "calib_camera.h"

#ifdef ARDOUBLE_IS_FLOAT
#  define SIN sinf
#  define COS cosf
#  define ACOS acosf
#else
#  define SIN sin
#  define COS cos
#  define ACOS acos
#endif

//
// Obsoleted functions.
//

#define CHECK_CALC    1

int arGetAngle( ARdouble rot[3][3], ARdouble *wa, ARdouble *wb, ARdouble *wc )
{
    ARdouble      a, b, c;
    ARdouble      sina, cosa, sinb, cosb, sinc, cosc;
#if CHECK_CALC
	ARdouble   w[3];
	int        i;
	for(i=0;i<3;i++) w[i] = rot[i][0];
	for(i=0;i<3;i++) rot[i][0] = rot[i][1];
	for(i=0;i<3;i++) rot[i][1] = rot[i][2];
	for(i=0;i<3;i++) rot[i][2] = w[i];
#endif
	
    if( rot[2][2] > 1.0 ) {
        /* ARLOG("cos(beta) = %f\n", rot[2][2]); */
        rot[2][2] = 1.0;
    }
    else if( rot[2][2] < -1.0 ) {
        /* ARLOG("cos(beta) = %f\n", rot[2][2]); */
        rot[2][2] = -1.0;
    }
    cosb = rot[2][2];
    b = ACOS( cosb );
    sinb = SIN( b );
    if( b >= 0.000001 || b <= -0.000001) {
        cosa = rot[0][2] / sinb;
        sina = rot[1][2] / sinb;
        if( cosa > 1.0 ) {
            /* ARLOG("cos(alph) = %f\n", cosa); */
            cosa = 1.0;
            sina = 0.0;
        }
        if( cosa < -1.0 ) {
            /* ARLOG("cos(alph) = %f\n", cosa); */
            cosa = -1.0;
            sina =  0.0;
        }
        if( sina > 1.0 ) {
            /* ARLOG("sin(alph) = %f\n", sina); */
            sina = 1.0;
            cosa = 0.0;
        }
        if( sina < -1.0 ) {
            /* ARLOG("sin(alph) = %f\n", sina); */
            sina = -1.0;
            cosa =  0.0;
        }
        a = ACOS( cosa );
        if( sina < 0 ) a = -a;
		
        sinc =  (rot[2][1]*rot[0][2]-rot[2][0]*rot[1][2])
			/ (rot[0][2]*rot[0][2]+rot[1][2]*rot[1][2]);
        cosc =  -(rot[0][2]*rot[2][0]+rot[1][2]*rot[2][1])
			/ (rot[0][2]*rot[0][2]+rot[1][2]*rot[1][2]);
        if( cosc > 1.0 ) {
            /* ARLOG("cos(r) = %f\n", cosc); */
            cosc = 1.0;
            sinc = 0.0;
        }
        if( cosc < -1.0 ) {
            /* ARLOG("cos(r) = %f\n", cosc); */
            cosc = -1.0;
            sinc =  0.0;
        }
        if( sinc > 1.0 ) {
            /* ARLOG("sin(r) = %f\n", sinc); */
            sinc = 1.0;
            cosc = 0.0;
        }
        if( sinc < -1.0 ) {
            /* ARLOG("sin(r) = %f\n", sinc); */
            sinc = -1.0;
            cosc =  0.0;
        }
        c = ACOS( cosc );
        if( sinc < 0 ) c = -c;
    }
    else {
        a = b = 0.0;
        cosa = cosb = 1.0;
        sina = sinb = 0.0;
        cosc = rot[0][0];
        sinc = rot[1][0];
        if( cosc > 1.0 ) {
            /* ARLOG("cos(r) = %f\n", cosc); */
            cosc = 1.0;
            sinc = 0.0;
        }
        if( cosc < -1.0 ) {
            /* ARLOG("cos(r) = %f\n", cosc); */
            cosc = -1.0;
            sinc =  0.0;
        }
        if( sinc > 1.0 ) {
            /* ARLOG("sin(r) = %f\n", sinc); */
            sinc = 1.0;
            cosc = 0.0;
        }
        if( sinc < -1.0 ) {
            /* ARLOG("sin(r) = %f\n", sinc); */
            sinc = -1.0;
            cosc =  0.0;
        }
        c = ACOS( cosc );
        if( sinc < 0 ) c = -c;
    }
	
    *wa = a;
    *wb = b;
    *wc = c;
	
    return 0;
}

int arGetRot( ARdouble a, ARdouble b, ARdouble c, ARdouble rot[3][3] )
{
    ARdouble   sina, sinb, sinc;
    ARdouble   cosa, cosb, cosc;
#if CHECK_CALC
    double     w[3];
    int        i;
#endif
	
    sina = SIN(a); cosa = COS(a);
    sinb = SIN(b); cosb = COS(b);
    sinc = SIN(c); cosc = COS(c);
    rot[0][0] = cosa*cosa*cosb*cosc+sina*sina*cosc+sina*cosa*cosb*sinc-sina*cosa*sinc;
    rot[0][1] = -cosa*cosa*cosb*sinc-sina*sina*sinc+sina*cosa*cosb*cosc-sina*cosa*cosc;
    rot[0][2] = cosa*sinb;
    rot[1][0] = sina*cosa*cosb*cosc-sina*cosa*cosc+sina*sina*cosb*sinc+cosa*cosa*sinc;
    rot[1][1] = -sina*cosa*cosb*sinc+sina*cosa*sinc+sina*sina*cosb*cosc+cosa*cosa*cosc;
    rot[1][2] = sina*sinb;
    rot[2][0] = -cosa*sinb*cosc-sina*sinb*sinc;
    rot[2][1] = cosa*sinb*sinc-sina*sinb*cosc;
    rot[2][2] = cosb;
	
#if CHECK_CALC
    for(i=0;i<3;i++) w[i] = rot[i][2];
    for(i=0;i<3;i++) rot[i][2] = rot[i][1];
    for(i=0;i<3;i++) rot[i][1] = rot[i][0];
    for(i=0;i<3;i++) rot[i][0] = w[i];
#endif
	
    return 0;
}

int arGetNewMatrix( ARdouble a, ARdouble b, ARdouble c,
                    ARdouble trans[3], ARdouble trans2[3][4],
                    ARdouble cpara[3][4], ARdouble ret[3][4] )
{
    ARdouble   cpara2[3][4];
    ARdouble   rot[3][3];
    int        i, j;
	
    arGetRot( a, b, c, rot );
	
    if( trans2 != NULL ) {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
                cpara2[j][i] = cpara[j][0] * trans2[0][i]
				+ cpara[j][1] * trans2[1][i]
				+ cpara[j][2] * trans2[2][i];
            }
        }
    }
    else {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
                cpara2[j][i] = cpara[j][i];
            }
        }
    }
	
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 3; i++ ) {
            ret[j][i] = cpara2[j][0] * rot[0][i]
			+ cpara2[j][1] * rot[1][i]
			+ cpara2[j][2] * rot[2][i];
        }
        ret[j][3] = cpara2[j][0] * trans[0]
		+ cpara2[j][1] * trans[1]
		+ cpara2[j][2] * trans[2]
		+ cpara2[j][3];
    }
	
    return(0);
}

#define MD_PI         3.14159265358979323846
ARdouble arModifyMatrix( ARdouble rot[3][3], ARdouble trans[3],
                         ARdouble cpara[3][4], ARdouble pos3d[][3], ARdouble pos2d[][2], int num )
{
    ARdouble    factor;
    ARdouble    a, b, c;
    ARdouble    a1, b1, c1;
    ARdouble    a2, b2, c2;
    ARdouble    ma, mb, mc;
    ARdouble    combo[3][4];
    ARdouble    hx, hy, h, x, y;
    ARdouble    err, minerr;
    int         t1, t2, t3;
    int         s1, s2, s3;
    int         i, j;
	
    arGetAngle( rot, &a, &b, &c );
	
    a2 = a;
    b2 = b;
    c2 = c;
    factor = 10.0*MD_PI/180.0;
    for( j = 0; j < 10; j++ ) {
        minerr = 1000000000.0;
        for(t1=-1;t1<=1;t1++) {
			for(t2=-1;t2<=1;t2++) {
				for(t3=-1;t3<=1;t3++) {
					a1 = a2 + factor*t1;
					b1 = b2 + factor*t2;
					c1 = c2 + factor*t3;
					arGetNewMatrix( a1, b1, c1, trans, NULL, cpara, combo );
					
					err = 0.0;
					for( i = 0; i < num; i++ ) {
						hx = combo[0][0] * pos3d[i][0]
						+ combo[0][1] * pos3d[i][1]
						+ combo[0][2] * pos3d[i][2]
						+ combo[0][3];
						hy = combo[1][0] * pos3d[i][0]
							+ combo[1][1] * pos3d[i][1]
							+ combo[1][2] * pos3d[i][2]
							+ combo[1][3];
						h  = combo[2][0] * pos3d[i][0]
							+ combo[2][1] * pos3d[i][1]
							+ combo[2][2] * pos3d[i][2]
							+ combo[2][3];
						x = hx / h;
						y = hy / h;
						
						err += (pos2d[i][0] - x) * (pos2d[i][0] - x)
							+ (pos2d[i][1] - y) * (pos2d[i][1] - y);
					}
					
					if( err < minerr ) {
						minerr = err;
						ma = a1;
						mb = b1;
						mc = c1;
						s1 = t1; s2 = t2; s3 = t3;
					}
				}
			}
        }
		
        if( s1 == 0 && s2 == 0 && s3 == 0 ) factor *= 0.5;
        a2 = ma;
        b2 = mb;
        c2 = mc;
    }
	
    arGetRot( ma, mb, mc, rot );
	
    return minerr/num;
}

#define P_MAX       500
static ARdouble  pos3d[P_MAX][3];
ARdouble arGetTransMatSub( ARdouble rot[3][3], ARdouble pos2d[][2], ARdouble ppos3d[][3], int num,
                           ARdouble cpara[3][4], ARdouble conv[3][4] )
{
    ARMat     *mat_a, *mat_b, *mat_c, *mat_d, *mat_e, *mat_f;
    ARdouble  trans[3];
    ARdouble  wx, wy, wz;
    ARdouble  off[3], pmax[3], pmin[3];
    ARdouble  ret;
    int       i, j;
	
    if( num > P_MAX ) {
        ARLOGe("arGetTransMatSub: num is too big.\n");
        exit(0);
    }
	
    mat_a = arMatrixAlloc( num*2, 3 );
    mat_b = arMatrixAlloc( 3, num*2 );
    mat_c = arMatrixAlloc( num*2, 1 );
    mat_d = arMatrixAlloc( 3, 3 );
    mat_e = arMatrixAlloc( 3, 1 );
    mat_f = arMatrixAlloc( 3, 1 );
	
    pmax[0]=pmax[1]=pmax[2] = -10000000000.0;
    pmin[0]=pmin[1]=pmin[2] =  10000000000.0;
    for( i = 0; i < num; i++ ) {
        if( ppos3d[i][0] > pmax[0] ) pmax[0] = ppos3d[i][0];
        if( ppos3d[i][0] < pmin[0] ) pmin[0] = ppos3d[i][0];
        if( ppos3d[i][1] > pmax[1] ) pmax[1] = ppos3d[i][1];
        if( ppos3d[i][1] < pmin[1] ) pmin[1] = ppos3d[i][1];
        if( ppos3d[i][2] > pmax[2] ) pmax[2] = ppos3d[i][2];
        if( ppos3d[i][2] < pmin[2] ) pmin[2] = ppos3d[i][2];
    }
    off[0] = -(pmax[0] + pmin[0]) / 2.0;    
    off[1] = -(pmax[1] + pmin[1]) / 2.0;
    off[2] = -(pmax[2] + pmin[2]) / 2.0;
    for( i = 0; i < num; i++ ) {
        pos3d[i][0] = ppos3d[i][0] + off[0];
        pos3d[i][1] = ppos3d[i][1] + off[1];
        pos3d[i][2] = ppos3d[i][2] + off[2];     
    }
	
    for( j = 0; j < num; j++ ) {
        wx = rot[0][0] * pos3d[j][0]
		+ rot[0][1] * pos3d[j][1]
		+ rot[0][2] * pos3d[j][2];
        wy = rot[1][0] * pos3d[j][0]
			+ rot[1][1] * pos3d[j][1]
			+ rot[1][2] * pos3d[j][2];
        wz = rot[2][0] * pos3d[j][0]
			+ rot[2][1] * pos3d[j][1]
			+ rot[2][2] * pos3d[j][2];
        mat_a->m[j*6+0] = mat_b->m[num*0+j*2] = cpara[0][0];
        mat_a->m[j*6+1] = mat_b->m[num*2+j*2] = cpara[0][1];
        mat_a->m[j*6+2] = mat_b->m[num*4+j*2] = cpara[0][2] - pos2d[j][0];
        mat_c->m[j*2+0] = wz * pos2d[j][0]
			- cpara[0][0]*wx - cpara[0][1]*wy - cpara[0][2]*wz;
        mat_a->m[j*6+3] = mat_b->m[num*0+j*2+1] = 0.0;
        mat_a->m[j*6+4] = mat_b->m[num*2+j*2+1] = cpara[1][1];
        mat_a->m[j*6+5] = mat_b->m[num*4+j*2+1] = cpara[1][2] - pos2d[j][1];
        mat_c->m[j*2+1] = wz * pos2d[j][1]
			- cpara[1][1]*wy - cpara[1][2]*wz;
    }
    arMatrixMul( mat_d, mat_b, mat_a );
    arMatrixMul( mat_e, mat_b, mat_c );
    arMatrixSelfInv( mat_d );
    arMatrixMul( mat_f, mat_d, mat_e );
    trans[0] = mat_f->m[0];
    trans[1] = mat_f->m[1];
    trans[2] = mat_f->m[2];
	
    ret = arModifyMatrix( rot, trans, cpara, pos3d, pos2d, num );
	
    for( j = 0; j < num; j++ ) {
        wx = rot[0][0] * pos3d[j][0]
		+ rot[0][1] * pos3d[j][1]
		+ rot[0][2] * pos3d[j][2];
        wy = rot[1][0] * pos3d[j][0]
			+ rot[1][1] * pos3d[j][1]
			+ rot[1][2] * pos3d[j][2];
        wz = rot[2][0] * pos3d[j][0]
			+ rot[2][1] * pos3d[j][1]
			+ rot[2][2] * pos3d[j][2];
        mat_a->m[j*6+0] = mat_b->m[num*0+j*2] = cpara[0][0];
        mat_a->m[j*6+1] = mat_b->m[num*2+j*2] = cpara[0][1];
        mat_a->m[j*6+2] = mat_b->m[num*4+j*2] = cpara[0][2] - pos2d[j][0];
        mat_c->m[j*2+0] = wz * pos2d[j][0]
			- cpara[0][0]*wx - cpara[0][1]*wy - cpara[0][2]*wz;
        mat_a->m[j*6+3] = mat_b->m[num*0+j*2+1] = 0.0;
        mat_a->m[j*6+4] = mat_b->m[num*2+j*2+1] = cpara[1][1];
        mat_a->m[j*6+5] = mat_b->m[num*4+j*2+1] = cpara[1][2] - pos2d[j][1];
        mat_c->m[j*2+1] = wz * pos2d[j][1]
			- cpara[1][1]*wy - cpara[1][2]*wz;
    }
    arMatrixMul( mat_d, mat_b, mat_a );
    arMatrixMul( mat_e, mat_b, mat_c );
    arMatrixSelfInv( mat_d );
    arMatrixMul( mat_f, mat_d, mat_e );
    trans[0] = mat_f->m[0];
    trans[1] = mat_f->m[1];
    trans[2] = mat_f->m[2];
	
    ret = arModifyMatrix( rot, trans, cpara, pos3d, pos2d, num );
	
    arMatrixFree( mat_a );
    arMatrixFree( mat_b );
    arMatrixFree( mat_c );
    arMatrixFree( mat_d );
    arMatrixFree( mat_e );
    arMatrixFree( mat_f );
	
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 3; i++ ) conv[j][i] = rot[j][i];
        conv[j][3] = trans[j];
    }
	
    conv[0][3] = conv[0][0]*off[0] + conv[0][1]*off[1] + conv[0][2]*off[2] + conv[0][3];
    conv[1][3] = conv[1][0]*off[0] + conv[1][1]*off[1] + conv[1][2]*off[2] + conv[1][3];
    conv[2][3] = conv[2][0]*off[0] + conv[2][1]*off[1] + conv[2][2]*off[2] + conv[2][3];
	
    return ret;
}

//
// End obsoleted functions.
//

static int calc_inp2( CALIB_PATT_T *patt, CALIB_COORD_T *screen, ARdouble *pos2d, ARdouble *pos3d,
                      ARdouble x0, ARdouble y0, ARdouble f[2], ARdouble *err );
static void get_cpara( CALIB_COORD_T *world, CALIB_COORD_T *screen, int num, ARdouble para[3][3] );
static int  get_fl( ARdouble *p , ARdouble *q, int num, ARdouble f[2] );
static int  check_rotation( ARdouble rot[2][3] );

int calc_inp( CALIB_PATT_T *patt, ARdouble dist_factor[], int xsize, int ysize, ARdouble mat[3][4], int dist_function_version )
{
    CALIB_COORD_T  *screen, *sp;
    ARdouble       *pos2d, *pos3d, *pp;
    ARdouble       f[2];
    ARdouble       x0, y0;
    ARdouble       err, minerr;
    int            res;
    int            i, j, k;

    sp = screen = (CALIB_COORD_T *)malloc(sizeof(CALIB_COORD_T) * patt->h_num * patt->v_num * patt->loop_num);
    pp = pos2d = (ARdouble *)malloc(sizeof(ARdouble) * patt->h_num * patt->v_num * patt->loop_num * 2);
    pos3d = (ARdouble *)malloc(sizeof(ARdouble) * patt->h_num * patt->v_num * 2);
    for( k = 0; k < patt->loop_num; k++ ) {
        for( j = 0; j < patt->v_num; j++ ) {
            for( i = 0; i < patt->h_num; i++ ) {
                arParamObserv2Ideal( dist_factor, 
                                     patt->point[k][j*patt->h_num+i].x_coord,
                                     patt->point[k][j*patt->h_num+i].y_coord,
                                     &(sp->x_coord), &(sp->y_coord), dist_function_version );
                *(pp++) = sp->x_coord;
                *(pp++) = sp->y_coord;
                sp++;
            }
        }
    }
    pp = pos3d;
    for( j = 0; j < patt->v_num; j++ ) {
        for( i = 0; i < patt->h_num; i++ ) {
            *(pp++) = patt->world_coord[j*patt->h_num+i].x_coord;
            *(pp++) = patt->world_coord[j*patt->h_num+i].y_coord;
        }
    }

    minerr = 100000000000000000000000.0;
    for( j = -50; j <= 50; j++ ) {
        ARLOG("-- loop:%d --\n", j);
        y0 = dist_factor[1] + j;
/*      y0 = ysize/2 + j;   */
        if( y0 < 0 || y0 >= ysize ) continue;

        for( i = -50; i <= 50; i++ ) {
            x0 = dist_factor[0] + i;
/*          x0 = xsize/2 + i;  */
            if( x0 < 0 || x0 >= xsize ) continue;

            res = calc_inp2( patt, screen, pos2d, pos3d, x0, y0, f, &err );
            if( res < 0 ) continue;
            if( err < minerr ) {
                ARLOG("F = (%f,%f), Center = (%f,%f): err = %f\n", f[0], f[1], x0, y0, err);
                minerr = err;

                mat[0][0] = f[0];
                mat[0][1] = 0.0;
                mat[0][2] = x0;
                mat[0][3] = 0.0;
                mat[1][0] = 0.0;
                mat[1][1] = f[1];
                mat[1][2] = y0;
                mat[1][3] = 0.0;
                mat[2][0] = 0.0;
                mat[2][1] = 0.0;
                mat[2][2] = 1.0;
                mat[2][3] = 0.0;
            }
        }
    }

    free( screen );
    free( pos2d );
    free( pos3d );

    if( minerr >= 100.0 ) return -1;
    return 0;
}

static int calc_inp2 ( CALIB_PATT_T *patt, CALIB_COORD_T *screen, ARdouble *pos2d, ARdouble *pos3d,
                      ARdouble x0, ARdouble y0, ARdouble f[2], ARdouble *err )
{
    ARdouble  x1, y1, x2, y2;
    ARdouble  p[LOOP_MAX], q[LOOP_MAX];
    ARdouble  para[3][3];
    ARdouble  rot[3][3], rot2[3][3];
    ARdouble  cpara[3][4], conv[3][4];
    ARdouble  *ppos2d, *ppos3d;
    ARdouble  d, werr, werr2;
    int       i, j, k, l, m;

    for( i =  0; i < patt->loop_num; i++ ) {
        get_cpara( patt->world_coord, &(screen[i*patt->h_num*patt->v_num]),
                   patt->h_num*patt->v_num, para );
        x1 = para[0][0] / para[2][0];
        y1 = para[1][0] / para[2][0];
        x2 = para[0][1] / para[2][1];
        y2 = para[1][1] / para[2][1];

        p[i] = (x1 - x0)*(x2 - x0);
        q[i] = (y1 - y0)*(y2 - y0);
    }
    if( get_fl(p, q, patt->loop_num, f) < 0 ) return -1;

    cpara[0][0] = f[0];
    cpara[0][1] = 0.0;
    cpara[0][2] = x0;
    cpara[0][3] = 0.0;
    cpara[1][0] = 0.0;
    cpara[1][1] = f[1];
    cpara[1][2] = y0;
    cpara[1][3] = 0.0;
    cpara[2][0] = 0.0;
    cpara[2][1] = 0.0;
    cpara[2][2] = 1.0;
    cpara[2][3] = 0.0;

    werr = 0.0;
    arMalloc( ppos2d, ARdouble, 2*patt->h_num*patt->v_num);
    arMalloc( ppos3d, ARdouble, 3*patt->h_num*patt->v_num);
    for( i =  0; i < patt->loop_num; i++ ) {
        get_cpara( patt->world_coord, &(screen[i*patt->h_num*patt->v_num]),
                   patt->h_num*patt->v_num, para );
        rot[0][0] = (para[0][0] - x0*para[2][0]) / f[0];
        rot[0][1] = (para[1][0] - y0*para[2][0]) / f[1];
        rot[0][2] = para[2][0];
        d = sqrt( rot[0][0]*rot[0][0] + rot[0][1]*rot[0][1] + rot[0][2]*rot[0][2] );
        rot[0][0] /= d;
        rot[0][1] /= d;
        rot[0][2] /= d;
        rot[1][0] = (para[0][1] - x0*para[2][1]) / f[0];
        rot[1][1] = (para[1][1] - y0*para[2][1]) / f[1];
        rot[1][2] = para[2][1];
        d = sqrt( rot[1][0]*rot[1][0] + rot[1][1]*rot[1][1] + rot[1][2]*rot[1][2] );
        rot[1][0] /= d;
        rot[1][1] /= d;
        rot[1][2] /= d;
        check_rotation( rot );
        rot[2][0] = rot[0][1]*rot[1][2] - rot[0][2]*rot[1][1];
        rot[2][1] = rot[0][2]*rot[1][0] - rot[0][0]*rot[1][2];
        rot[2][2] = rot[0][0]*rot[1][1] - rot[0][1]*rot[1][0];
        d = sqrt( rot[2][0]*rot[2][0] + rot[2][1]*rot[2][1] + rot[2][2]*rot[2][2] );
        rot[2][0] /= d;
        rot[2][1] /= d;
        rot[2][2] /= d;
        rot2[0][0] = rot[0][0]; // transpose
        rot2[1][0] = rot[0][1];
        rot2[2][0] = rot[0][2];
        rot2[0][1] = rot[1][0];
        rot2[1][1] = rot[1][1];
        rot2[2][1] = rot[1][2];
        rot2[0][2] = rot[2][0];
        rot2[1][2] = rot[2][1];
        rot2[2][2] = rot[2][2];

        for( j = 0; j < patt->h_num*patt->v_num; j++ ) {
            ppos2d[j*2+0] = pos2d[(i*patt->h_num*patt->v_num+j)*2 + 0];
            ppos2d[j*2+1] = pos2d[(i*patt->h_num*patt->v_num+j)*2 + 1];
            ppos3d[j*3+0] = pos3d[j*2+0];
            ppos3d[j*3+1] = pos3d[j*2+1];
            ppos3d[j*3+2] = 0.0;
        }

        for( j = 0; j < 5; j++ ) {
			
			for (m = 0; m < 5; m++) {
				werr2 = arGetTransMatSub(rot2, (ARdouble (*)[2])ppos2d, (ARdouble (*)[3])ppos3d, patt->h_num*patt->v_num, cpara, conv);
				if (werr2 < 0.1) break;
			}
			
            for( k = 0; k < 3; k++ ) {
            for( l = 0; l < 3; l++ ) {
                rot2[k][l] = conv[k][l];
            }}
        }
        werr += werr2;

    }
    *err = sqrt( werr / patt->loop_num );
    free( ppos2d );
    free( ppos3d );

    return 0;
}

static void get_cpara( CALIB_COORD_T *world, CALIB_COORD_T *screen, int num, ARdouble para[3][3] )
{
    ARMat   *a, *b, *c;
    ARMat   *at, *aa, res;
    int     i;

    a = arMatrixAlloc( num*2, 8 );
    b = arMatrixAlloc( num*2, 1 );
    c = arMatrixAlloc( 8, num*2 );
    at = arMatrixAlloc( 8, num*2 );
    aa = arMatrixAlloc( 8, 8 );
    for( i = 0; i < num; i++ ) {
        a->m[i*16+0]  = world[i].x_coord;
        a->m[i*16+1]  = world[i].y_coord;
        a->m[i*16+2]  = 1.0;
        a->m[i*16+3]  = 0.0;
        a->m[i*16+4]  = 0.0;
        a->m[i*16+5]  = 0.0;
        a->m[i*16+6]  = -world[i].x_coord * screen[i].x_coord;
        a->m[i*16+7]  = -world[i].y_coord * screen[i].x_coord;
        a->m[i*16+8]  = 0.0;
        a->m[i*16+9]  = 0.0;
        a->m[i*16+10] = 0.0;
        a->m[i*16+11] = world[i].x_coord;
        a->m[i*16+12] = world[i].y_coord;
        a->m[i*16+13] = 1.0;
        a->m[i*16+14] = -world[i].x_coord * screen[i].y_coord;
        a->m[i*16+15] = -world[i].y_coord * screen[i].y_coord;
        b->m[i*2+0] = screen[i].x_coord;
        b->m[i*2+1] = screen[i].y_coord;
    }
    arMatrixTrans( at, a );
    arMatrixMul( aa, at, a );
    arMatrixSelfInv( aa );
    arMatrixMul( c, aa, at );
    res.row = 8;
    res.clm = 1;
    res.m = &(para[0][0]);
    arMatrixMul( &res, c, b );
    para[2][2] = 1.0;

    arMatrixFree( a );
    arMatrixFree( b );
    arMatrixFree( c );
    arMatrixFree( at );
    arMatrixFree( aa );
}

static int get_fl( ARdouble *p , ARdouble *q, int num, ARdouble f[2] )
{
    //COVHI10404, COVHI10368, COVHI10351, COVHI10315, COVHI10310, COVHI10309
    ARMat *a = NULL, *b = NULL, *c = NULL;
    ARMat *at = NULL, *aa = NULL, *res = NULL;
    int   i, ret = 0;

#if 1
    if (NULL == (a = arMatrixAlloc(num, 2))) {
        ret = -1;
        goto done;
    }
    if (NULL == (b = arMatrixAlloc(num, 1))) {
        ret = -1;
        goto done;
    }
    if (NULL == (c = arMatrixAlloc(2, num))) {
        ret = -1;
        goto done;
    }
    if (NULL == (at = arMatrixAlloc(2, num))) {
        ret = -1;
        goto done;
    }
    if (NULL == (aa = arMatrixAlloc(2, 2))) {
        ret = -1;
        goto done;
    }
    if (NULL == (res = arMatrixAlloc(2, 1))) {
        ret = -1;
        goto done;
    }

    for( i = 0; i < num; i++ ) {
        a->m[i*2+0] = *(p++);
        a->m[i*2+1] = *(q++);
        b->m[i]     = -1.0;
    }
#else
    a = arMatrixAlloc( num-1, 2 );
    b = arMatrixAlloc( num-1, 1 );
    c = arMatrixAlloc( 2, num-1 );
    at = arMatrixAlloc( 2, num-1 );
    aa = arMatrixAlloc( 2, 2 );
    res = arMatrixAlloc( 2, 1 );
    p++; q++;
    for( i = 0; i < num-1; i++ ) {
        a->m[i*2+0] = *(p++);
        a->m[i*2+1] = *(q++);
        b->m[i]     = -1.0;
    }
#endif
    arMatrixTrans( at, a );
    arMatrixMul( aa, at, a );
    arMatrixSelfInv( aa );
    arMatrixMul( c, aa, at );
    arMatrixMul( res, c, b );

    if (res->m[0] < 0 || res->m[1] < 0) {
        ret = -1;
        goto done;
    }

    f[0] = sqrt( 1.0 / res->m[0] );
    f[1] = sqrt( 1.0 / res->m[1] );

done:
    arMatrixFree( a );
    arMatrixFree( b );
    arMatrixFree( c );
    arMatrixFree( at );
    arMatrixFree( aa );
    arMatrixFree( res );

    return (ret);
}

static int check_rotation( ARdouble rot[2][3] )
{
    double  v1[3], v2[3], v3[3];
    double  ca, cb, k1, k2, k3, k4;
    double  a, b, c, d;
    double  p1, q1, r1;
    double  p2, q2, r2;
    double  p3, q3, r3;
    double  p4, q4, r4;
    double  w;
    double  e1, e2, e3, e4;
    int     f;

    v1[0] = rot[0][0];
    v1[1] = rot[0][1];
    v1[2] = rot[0][2];
    v2[0] = rot[1][0];
    v2[1] = rot[1][1];
    v2[2] = rot[1][2];
    v3[0] = v1[1]*v2[2] - v1[2]*v2[1];
    v3[1] = v1[2]*v2[0] - v1[0]*v2[2];
    v3[2] = v1[0]*v2[1] - v1[1]*v2[0];
    w = sqrt( v3[0]*v3[0]+v3[1]*v3[1]+v3[2]*v3[2] );
    if( w == 0.0 ) return -1;
    v3[0] /= w;
    v3[1] /= w;
    v3[2] /= w;

    cb = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    if( cb < 0 ) cb *= -1.0;
    ca = (sqrt(cb+1.0) + sqrt(1.0-cb)) * 0.5;

    if( v3[1]*v1[0] - v1[1]*v3[0] != 0.0 ) {
        f = 0;
    }
    else {
        if( v3[2]*v1[0] - v1[2]*v3[0] != 0.0 ) {
            w = v1[1]; v1[1] = v1[2]; v1[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            f = 1;
        }
        else {
            w = v1[0]; v1[0] = v1[2]; v1[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            f = 2;
        }
    }
    if( v3[1]*v1[0] - v1[1]*v3[0] == 0.0 ) return -1;
    k1 = (v1[1]*v3[2] - v3[1]*v1[2]) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k3 = (v1[0]*v3[2] - v3[0]*v1[2]) / (v3[0]*v1[1] - v1[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v1[1] - v1[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r1 = (-b + sqrt(d))/a;
    p1 = k1*r1 + k2;
    q1 = k3*r1 + k4;
    r2 = (-b - sqrt(d))/a;
    p2 = k1*r2 + k2;
    q2 = k3*r2 + k4;
    if( f == 1 ) {
        w = q1; q1 = r1; r1 = w;
        w = q2; q2 = r2; r2 = w;
        w = v1[1]; v1[1] = v1[2]; v1[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
        f = 0;
    }
    if( f == 2 ) {
        w = p1; p1 = r1; r1 = w;
        w = p2; p2 = r2; r2 = w;
        w = v1[0]; v1[0] = v1[2]; v1[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
        f = 0;
    }

    if( v3[1]*v2[0] - v2[1]*v3[0] != 0.0 ) {
        f = 0;
    }
    else {
        if( v3[2]*v2[0] - v2[2]*v3[0] != 0.0 ) {
            w = v2[1]; v2[1] = v2[2]; v2[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            f = 1;
        }
        else {
            w = v2[0]; v2[0] = v2[2]; v2[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            f = 2;
        }
    }
    if( v3[1]*v2[0] - v2[1]*v3[0] == 0.0 ) return -1;
    k1 = (v2[1]*v3[2] - v3[1]*v2[2]) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k3 = (v2[0]*v3[2] - v3[0]*v2[2]) / (v3[0]*v2[1] - v2[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v2[1] - v2[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r3 = (-b + sqrt(d))/a;
    p3 = k1*r3 + k2;
    q3 = k3*r3 + k4;
    r4 = (-b - sqrt(d))/a;
    p4 = k1*r4 + k2;
    q4 = k3*r4 + k4;
    if( f == 1 ) {
        w = q3; q3 = r3; r3 = w;
        w = q4; q4 = r4; r4 = w;
        w = v2[1]; v2[1] = v2[2]; v2[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
        f = 0;
    }
    if( f == 2 ) {
        w = p3; p3 = r3; r3 = w;
        w = p4; p4 = r4; r4 = w;
        w = v2[0]; v2[0] = v2[2]; v2[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
        f = 0;
    }

    e1 = p1*p3+q1*q3+r1*r3; if( e1 < 0 ) e1 = -e1;
    e2 = p1*p4+q1*q4+r1*r4; if( e2 < 0 ) e2 = -e2;
    e3 = p2*p3+q2*q3+r2*r3; if( e3 < 0 ) e3 = -e3;
    e4 = p2*p4+q2*q4+r2*r4; if( e4 < 0 ) e4 = -e4;
    if( e1 < e2 ) {
        if( e1 < e3 ) {
            if( e1 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }
    else {
        if( e2 < e3 ) {
            if( e2 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }

    return 0;
}
