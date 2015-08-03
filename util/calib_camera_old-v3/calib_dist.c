/*
 *  calib_dist.c
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
#include <stdlib.h>
#include <AR/ar.h>
#include <AR/matrix.h>
#include "calib_camera.h"

#ifdef ARDOUBLE_IS_FLOAT
#  define SQRT sqrtf
#else
#  define SQRT sqrt
#endif


static ARdouble   get_fitting_error( CALIB_PATT_T *patt, ARdouble dist_factor[], int dist_function_version );
static ARdouble   check_error( ARdouble *x, ARdouble *y, int num, ARdouble dist_factor[], int dist_function_version );
static ARdouble   calc_distortion2( CALIB_PATT_T *patt, ARdouble dist_factor[], int dist_function_version );
static ARdouble   get_size_factor( ARdouble dist_factor[], int xsize, int ysize, int dist_function_version );

void calc_distortion( CALIB_PATT_T *patt, int xsize, int ysize, ARdouble aspect_ratio, ARdouble dist_factor[],
                      int dist_function_version )
{
    int       i, j;
    ARdouble  bx, by;
    ARdouble  bf[AR_DIST_FACTOR_NUM_MAX];
    ARdouble  error, min;
    ARdouble  factor[AR_DIST_FACTOR_NUM_MAX];

    // Sanity check. COVHI10443
    if ((dist_function_version < 1) || (dist_function_version > (AR_DIST_FUNCTION_VERSION_MAX - 1))) {
        ARLOGe("calc_distortion: dist_function_version out of bounds.\n");
        exit (-1);
    }

    bx = xsize / 2;
    by = ysize / 2;
    factor[0] = bx;
    factor[1] = by;
    factor[2] = 1.0;
    if (dist_function_version == 3) {
        factor[3] = aspect_ratio;
    }
    min = calc_distortion2( patt, factor, dist_function_version );
    if (dist_function_version == 3) {
        bf[0] = factor[0];
        bf[1] = factor[1];
        bf[2] = factor[2];
        bf[3] = factor[3];
        bf[4] = factor[4];
        bf[5] = factor[5];
        ARLOG("[%5.1f, %5.1f, %5.1f %5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], bf[3], bf[4], bf[5], min);
    } else if (dist_function_version == 2) {
        bf[0] = factor[0];
        bf[1] = factor[1];
        bf[2] = factor[2];
        bf[3] = factor[3];
        bf[4] = factor[4];
        ARLOG("[%5.1f, %5.1f, %5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], bf[3], bf[4], min);
    } else if (dist_function_version == 1) {
        bf[0] = factor[0];
        bf[1] = factor[1];
        bf[2] = factor[2];
        bf[3] = factor[3];
        ARLOG("[%5.1f, %5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], bf[3], min);
    }
    for( j = -10; j <= 10; j++ ) {
        factor[1] = by + j*5;
        for( i = -10; i <= 10; i++ ) {
            factor[0] = bx + i*5;
            error = calc_distortion2( patt, factor, dist_function_version );
            if (dist_function_version == 3) {
                if( error < min ) {
                    bf[0] = factor[0];
                    bf[1] = factor[1];
                    bf[2] = factor[2];
                    bf[3] = factor[3];
                    bf[4] = factor[4];
                    bf[5] = factor[5];
                    min = error;
                }
            } else if (dist_function_version == 2) {
                if( error < min ) {
                    bf[0] = factor[0];
                    bf[1] = factor[1];
                    bf[2] = factor[2];
                    bf[3] = factor[3];
                    bf[4] = factor[4];
                    min = error;
                }
            } else if (dist_function_version == 1) {
                if( error < min ) {
                    bf[0] = factor[0];
                    bf[1] = factor[1];
                    bf[2] = factor[2];
                    bf[3] = factor[3];
                    min = error;
                }
            }
        }
        if (dist_function_version == 3) {
            ARLOG("[%5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], bf[3], bf[4], bf[5], min);
        } else if (dist_function_version == 2) {
            ARLOG("[%5.1f, %5.1f, %5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], bf[3], bf[4], min);
        } else if (dist_function_version == 1) {
            ARLOG("[%5.1f, %5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], bf[3], min);
        }
    }

    bx = bf[0];
    by = bf[1];
    for( j = -10; j <= 10; j++ ) {
        factor[1] = by + 0.5 * j;
        for( i = -10; i <= 10; i++ ) {
            factor[0] = bx + 0.5 * i;
            error = calc_distortion2( patt, factor, dist_function_version );
            if (dist_function_version == 3) {
                if( error < min ) {
                    bf[0] = factor[0];
                    bf[1] = factor[1];
                    bf[2] = factor[2];
                    bf[3] = factor[3];
                    bf[4] = factor[4];
                    bf[5] = factor[5];
                    min = error;
                }
            } else if (dist_function_version == 2) {
                if( error < min ) {
                    bf[0] = factor[0];
                    bf[1] = factor[1];
                    bf[2] = factor[2];
                    bf[3] = factor[3];
                    bf[4] = factor[4];
                    min = error;
                }
            } else if (dist_function_version == 1) {
                if( error < min ) {
                    bf[0] = factor[0];
                    bf[1] = factor[1];
                    bf[2] = factor[2];
                    bf[3] = factor[3];
                    min = error;
                }
            }
        }
        if (dist_function_version == 3) {
            ARLOG("[%5.1f, %5.1f, %5.1f, %5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], bf[3], bf[4], bf[5], min);
        } else if (dist_function_version == 2) {
            ARLOG("[%5.1f, %5.1f, %5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], bf[3], bf[4], min);
        } else if (dist_function_version == 1) {
            ARLOG("[%5.1f, %5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], bf[3], min);
        }
    }

    if (dist_function_version == 3) {
        dist_factor[0] = bf[0];
        dist_factor[1] = bf[1];
        dist_factor[2] = get_size_factor( bf, xsize, ysize, dist_function_version );
        dist_factor[3] = bf[3];
        dist_factor[4] = bf[4];
        dist_factor[5] = bf[5];
    } else if (dist_function_version == 2) {
        dist_factor[0] = bf[0];
        dist_factor[1] = bf[1];
        dist_factor[2] = get_size_factor( bf, xsize, ysize, dist_function_version );
        dist_factor[3] = bf[3];
        dist_factor[4] = bf[4];
    } else if (dist_function_version == 1) {
        dist_factor[0] = bf[0];
        dist_factor[1] = bf[1];
        dist_factor[2] = get_size_factor( bf, xsize, ysize, dist_function_version );
        dist_factor[3] = bf[3];
    }
}

static ARdouble get_size_factor( ARdouble dist_factor[], int xsize, int ysize, int dist_function_version )
{
    ARdouble  ox, oy, ix, iy;
    ARdouble  olen, ilen;
    ARdouble  sf, sf1;

    sf = 100.0;

    ox = 0.0;
    oy = dist_factor[1];
    olen = dist_factor[0];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = dist_factor[0] - ix;
ARLOG("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = xsize;
    oy = dist_factor[1];
    olen = xsize - dist_factor[0];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = ix - dist_factor[0];
ARLOG("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = dist_factor[0];
    oy = 0.0;
    olen = dist_factor[1];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = dist_factor[1] - iy;
ARLOG("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = dist_factor[0];
    oy = ysize;
    olen = ysize - dist_factor[1];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy, dist_function_version );
    ilen = iy - dist_factor[1];
ARLOG("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    if( sf == 0.0 ) sf = 1.0;

    return sf;
}

static ARdouble calc_distortion2( CALIB_PATT_T *patt, ARdouble *dist_factor, int dist_function_version )
{
	double min;
	
	// ----------------------------------------
    if (dist_function_version == 3) {

        double    err, f1, f2, fb1, fb2;
        int       i, j;

        dist_factor[4] = 0.0;
        dist_factor[5] = 0.0;
        min = get_fitting_error( patt, dist_factor, 3 );

        f1 = dist_factor[4];
        for( i = -100; i < 200; i+=10 ) {
            dist_factor[4] = i;
            err = get_fitting_error( patt, dist_factor, 3 );
            if( err < min ) { min = err; f1 = dist_factor[4]; }
        }

        fb1 = f1;
        for( j = -10; j <= 10; j++ ) {
            for( i = -10; i <= 10; i++ ) {
                dist_factor[4] = fb1 + i;
                dist_factor[5] = j;
                //if( dist_factor[4] < 0 ) continue;
                err = get_fitting_error( patt, dist_factor, 3 );
                if( err < min ) { min = err; f1 = dist_factor[4]; f2 = dist_factor[5]; }
            }
        }

        fb1  = f1;
        fb2  = f2;
        for( j = -10; j <= 10; j++ ) {
            for( i = -10; i <= 10; i++ ) {
                dist_factor[4] = fb1 + 0.1 * i;
                dist_factor[5] = fb2 + 0.1 * j;
                //if( dist_factor[4] < 0 ) continue;
                err = get_fitting_error( patt, dist_factor, 3 );
                if( err < min ) { min = err; f1 = dist_factor[4]; f2 = dist_factor[5]; }
            }
        }

        dist_factor[4] = f1;
        dist_factor[5] = f2;

	// ----------------------------------------
	} else if (dist_function_version == 2) {

	    double    err, f1, f2 = 0.0 /*//COVHI10458*/, fb1, fb2;
        int       i, j;

        dist_factor[3] = 0.0;
        dist_factor[4] = 0.0;
        min = get_fitting_error( patt, dist_factor, 2 );

        f1 = dist_factor[3];
        for( i = -100; i < 200; i+=10 ) {
            dist_factor[3] = i;
            err = get_fitting_error( patt, dist_factor, 2 );
            if( err < min ) {
                min = err;
                f1 = dist_factor[3];
            }
        }

        fb1 = f1;
        for( j = -10; j <= 10; j++ ) {
            for( i = -10; i <= 10; i++ ) {
                dist_factor[3] = fb1 + i;
                dist_factor[4] = j;
                //if( dist_factor[3] < 0 ) continue;
                err = get_fitting_error( patt, dist_factor, 2 );
                if( err < min ) {
                    min = err;
                    f1 = dist_factor[3];
                    f2 = dist_factor[4];
                }
            }
        }

        fb1  = f1;
        fb2  = f2;
        for( j = -10; j <= 10; j++ ) {
            for( i = -10; i <= 10; i++ ) {
                dist_factor[3] = fb1 + 0.1 * i;
                dist_factor[4] = fb2 + 0.1 * j;
                //if( dist_factor[3] < 0 ) continue;
                err = get_fitting_error( patt, dist_factor, 2 );
                if( err < min ) {
                    min = err;
                    f1 = dist_factor[3];
                    f2 = dist_factor[4];
                }
            }
        }

        dist_factor[3] = f1;
        dist_factor[4] = f2;
	
	// ----------------------------------------
	} else if (dist_function_version == 1) {

	    double    err, f, fb;
        int       i;

        dist_factor[3] = 0.0;
        min = get_fitting_error( patt, dist_factor, 1 );

        f = dist_factor[3];
        for( i = -100; i < 200; i+=10 ) {
            dist_factor[3] = i;
            err = get_fitting_error( patt, dist_factor, 1 );
            if( err < min ) { min = err; f = dist_factor[3]; }
        }

        fb = f;
        for( i = -10; i <= 10; i++ ) {
            dist_factor[3] = fb + i;
            //if( dist_factor[3] < 0 ) continue;
            err = get_fitting_error( patt, dist_factor, 1 );
            if( err < min ) { min = err; f = dist_factor[3]; }
        }

        fb = f;
        for( i = -10; i <= 10; i++ ) {
            dist_factor[3] = fb + 0.1 * i;
            //if( dist_factor[3] < 0 ) continue;
            err = get_fitting_error( patt, dist_factor, 1 );
            if( err < min ) { min = err; f = dist_factor[3]; }
        }

        dist_factor[3] = f;

	// ----------------------------------------
	} else {
		min = 0;
	}

    return min;
}

static ARdouble get_fitting_error( CALIB_PATT_T *patt, ARdouble *dist_factor, int dist_function_version )
{
    ARdouble   *x, *y;
    ARdouble   error;
    int        max;
    int        i, j, k, l;
    int        p, c;

    max = (patt->v_num > patt->h_num)? patt->v_num: patt->h_num;
    x = (ARdouble *)malloc( sizeof(ARdouble)*max );
    y = (ARdouble *)malloc( sizeof(ARdouble)*max );
    if( x == NULL || y == NULL ) exit(0);

    error = 0.0;
    c = 0;
    for( i = 0; i < patt->loop_num; i++ ) {
        for( j = 0; j < patt->v_num; j++ ) {
            for( k = 0; k < patt->h_num; k++ ) {
                x[k] = patt->point[i][j*patt->h_num+k].x_coord;
                y[k] = patt->point[i][j*patt->h_num+k].y_coord;
            }
            error += check_error( x, y, patt->h_num, dist_factor, dist_function_version );
            c += patt->h_num;
        }

        for( j = 0; j < patt->h_num; j++ ) {
            for( k = 0; k < patt->v_num; k++ ) {
                x[k] = patt->point[i][k*patt->h_num+j].x_coord;
                y[k] = patt->point[i][k*patt->h_num+j].y_coord;
            }
            error += check_error( x, y, patt->v_num, dist_factor, dist_function_version );
            c += patt->v_num;
        }

        for( j = 3 - patt->v_num; j < patt->h_num - 2; j++ ) {
            p = 0;
            for( k = 0; k < patt->v_num; k++ ) {
                l = j+k;
                if( l < 0 || l >= patt->h_num ) continue;
                x[p] = patt->point[i][k*patt->h_num+l].x_coord;
                y[p] = patt->point[i][k*patt->h_num+l].y_coord;
                p++;
            }
            error += check_error( x, y, p, dist_factor, dist_function_version );
            c += p;
        }

        for( j = 2; j < patt->h_num + patt->v_num - 3; j++ ) {
            p = 0;
            for( k = 0; k < patt->v_num; k++ ) {
                l = j-k;
                if( l < 0 || l >= patt->h_num ) continue;
                x[p] = patt->point[i][k*patt->h_num+l].x_coord;
                y[p] = patt->point[i][k*patt->h_num+l].y_coord;
                p++;
            }
            error += check_error( x, y, p, dist_factor, dist_function_version );
            c += p;
        }
    }

    free( x );
    free( y );

    return SQRT(error/c);
}

static ARdouble check_error( ARdouble *x, ARdouble *y, int num, ARdouble *dist_factor, int dist_function_version )
{
    ARMat    *input, *evec;
    ARVec    *ev, *mean;
    ARdouble a, b, c;
    ARdouble error;
    int      i;

    ev     = arVecAlloc( 2 );
    mean   = arVecAlloc( 2 );
    evec   = arMatrixAlloc( 2, 2 );

    input  = arMatrixAlloc( num, 2 );
    for( i = 0; i < num; i++ ) {
        arParamObserv2Ideal( dist_factor, x[i], y[i],
                             &(input->m[i*2+0]), &(input->m[i*2+1]), dist_function_version );
    }
    if( arMatrixPCA(input, evec, ev, mean) < 0 ) exit(0);
    a =  evec->m[1];
    b = -evec->m[0];
    c = -(a*mean->v[0] + b*mean->v[1]);

    error = 0.0;
    for( i = 0; i < num; i++ ) {
        error += (a*input->m[i*2+0] + b*input->m[i*2+1] + c)
               * (a*input->m[i*2+0] + b*input->m[i*2+1] + c);
    }
    error /= (a*a + b*b);

    arMatrixFree( input );
    arMatrixFree( evec );
    arVecFree( mean );
    arVecFree( ev );

    return error;
}
