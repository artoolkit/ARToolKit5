/*
 *  main.c
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
#include <string.h>
#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif
#include <AR/ar.h>
#include <AR/video.h>
#include <AR/gsub.h>
#include <ARUtil/time.h>
#include "calib_camera.h"

// Make sure that required OpenGL constant definitions are available at compile-time.
// N.B. These should not be used unless the renderer indicates (at run-time) that it supports them.
	
// Define constants for extensions which became core in OpenGL 1.2
#ifndef GL_VERSION_1_2
#  if GL_EXT_bgra
#    define GL_BGR							GL_BGR_EXT
#    define GL_BGRA							GL_BGRA_EXT
#  else
#    define GL_BGR							0x80E0
#    define GL_BGRA							0x80E1
#  endif
#  ifndef GL_APPLE_packed_pixels
#    define GL_UNSIGNED_INT_8_8_8_8			0x8035
#    define GL_UNSIGNED_INT_8_8_8_8_REV		0x8367
#  endif
#  if GL_SGIS_texture_edge_clamp
#    define GL_CLAMP_TO_EDGE				GL_CLAMP_TO_EDGE_SGIS
#  else
#    define GL_CLAMP_TO_EDGE				0x812F
#  endif
#endif
	
// Define constants for extensions (not yet core).
#ifndef GL_APPLE_ycbcr_422
#  define GL_YCBCR_422_APPLE				0x85B9
#  define GL_UNSIGNED_SHORT_8_8_APPLE		0x85BA
#  define GL_UNSIGNED_SHORT_8_8_REV_APPLE	0x85BB
#endif
#ifndef GL_EXT_abgr
#  define GL_ABGR_EXT						0x8000
#endif
#if GL_NV_texture_rectangle
#  define GL_TEXTURE_RECTANGLE				GL_TEXTURE_RECTANGLE_NV
#  define GL_PROXY_TEXTURE_RECTANGLE		GL_PROXY_TEXTURE_RECTANGLE_NV
#  define GL_MAX_RECTANGLE_TEXTURE_SIZE		GL_MAX_RECTANGLE_TEXTURE_SIZE_NV
#elif GL_EXT_texture_rectangle
#  define GL_TEXTURE_RECTANGLE				GL_TEXTURE_RECTANGLE_EXT
#  define GL_PROXY_TEXTURE_RECTANGLE		GL_PROXY_TEXTURE_RECTANGLE_EXT
#  define GL_MAX_RECTANGLE_TEXTURE_SIZE		GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT
#else
#  define GL_TEXTURE_RECTANGLE				0x84F5
#  define GL_PROXY_TEXTURE_RECTANGLE		0x84F7
#  define GL_MAX_RECTANGLE_TEXTURE_SIZE		0x84F8
#endif

#define             SCALE    1.0

int                 xsize;
int                 ysize;
int                 pixelFormat;
int                 pixelSize;
int                 thresh = THRESH;
unsigned char      *clipImage;
CALIB_PATT_T        patt;          
ARdouble            dist_factor[AR_DIST_FACTOR_NUM_MAX];
int					dist_function_version = 3;
ARdouble            aspect_ratio = 1.0;
ARdouble            mat[3][4];
char                save_filename[256];
ARGViewportHandle  *vp;


int             point_num;
int             sx, sy, ex, ey;
int             status;
int             check_num;

static void     init( int argc, char *argv[] );
static void     mouseEvent(int button, int state, int x, int y);
static void     motionEvent( int x, int y );
static void     keyEvent(unsigned char key, int x, int y);
static void     dispImage(void);
static void     dispClipImage( int sx, int sy, int xsize, int ysize, ARUint8 *clipImage );
static void     draw_warp_line( double a, double b , double c );
static void     draw_line(void);
static void     draw_line2( double *x, double *y, int num );
static void     draw_warp_line( double a, double b , double c );
static void     print_comment( int status );
static void     save_param(void);

static void     save_param(void)
{
    ARParam  param;
    int      i, j;

    param.xsize = xsize;
    param.ysize = ysize;
    for( i = 0; i < AR_DIST_FACTOR_NUM_MAX; i++ ) param.dist_factor[i] = dist_factor[i];
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            param.mat[j][i] = mat[j][i];
        }
    }
	param.dist_function_version = dist_function_version;
    arParamDisp( &param );

    if( save_filename[0] == '\0' ) {
        printf("Filename: ");
        scanf( "%s", save_filename );
    }
    arParamSave( save_filename, 1, &param );

    return;
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
    init( argc, argv );

    argSetKeyFunc(keyEvent);
    argSetMouseFunc(mouseEvent);
    argSetMotionFunc(motionEvent);
    argSetDispFunc(dispImage, 1);

    print_comment(0);
    status = 0;
    point_num = 0;
    arVideoCapStart();
    argMainLoop();
	
	return (0);
}

static void init( int argc, char *argv[] )
{
    char         vconf[256];
    ARGViewport  viewport;
    ARUint32     euid0, euid1;
    double       length;
    int          i, j;

	// Allow user to choose file version for backwards compatibility.
    ARLOG("A calibrated camera parameter file for ARToolKit will be produced.\n");
    ARLOG("The file can be generated to be backwards compatible with previous ARToolKit versions.\n");
    ARLOG("    1: ARToolKit v1.0 and later\n");
    ARLOG("    2: ARToolKit v4.0 and later\n");
    ARLOG("    3: ARToolKit v4.1 and later\n");
	printf("Enter the number (1 to %d) of the camera parameter file version: ", AR_DIST_FUNCTION_VERSION_MAX); 
	scanf("%d", &dist_function_version); while (getchar() != '\n');
	
	patt.h_num    = H_NUM;
    patt.v_num    = V_NUM;
    patt.loop_num = 0;
    if( patt.h_num < 3 || patt.v_num < 3 ) exit(0);

    printf("Input the distance between each marker dot, in millimeters: ");
    scanf("%lf", &length); while( getchar()!='\n' );
    patt.world_coord = (CALIB_COORD_T *)malloc( sizeof(CALIB_COORD_T)*patt.h_num*patt.v_num );
    for( j = 0; j < patt.v_num; j++ ) {
        for( i = 0; i < patt.h_num; i++ ) {
            patt.world_coord[j*patt.h_num+i].x_coord =  length * i;
            patt.world_coord[j*patt.h_num+i].y_coord =  length * j;
        }
    }

    if( argc == 1 ) {
        strcpy(vconf, VCONF);
    }
    else {
        strcpy(vconf, argv[1]);
        for( i = 2; i < argc; i++ ) {
            strcat(vconf, " ");
            strcat(vconf, argv[i]);
        }
    }
    if( arVideoOpen(vconf) < 0  ) exit(0);
    if( arVideoGetSize(&xsize, &ysize) < 0 ) exit(0);
    if( (pixelFormat=arVideoGetPixelFormat()) < 0 ) exit(0);
    if( (pixelSize=arVideoGetPixelSize()) < 0 ) exit(0);
    ARLOG("Camera image size (x,y) = (%d,%d)\n", xsize, ysize);

    viewport.sx = 0;
    viewport.sy = 0;
    viewport.xsize = xsize/SCALE;
    viewport.ysize = ysize/SCALE;
    if( (vp=argCreateViewport(&viewport)) == NULL ) exit(0);
    argViewportSetImageSize( vp, xsize, ysize );
    argViewportSetPixFormat( vp, pixelFormat );
    argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_GL_DRAW_PIXELS );
    argViewportSetDispMode( vp, AR_GL_DISP_MODE_FIT_TO_VIEWPORT );
    argViewportSetDistortionMode( vp, AR_GL_DISTORTION_COMPENSATE_DISABLE );

    clipImage = (unsigned char *)malloc( xsize*ysize*pixelSize );
    if( clipImage == NULL ) exit(0);

    save_filename[0] = '\0';
    if( arVideoGetId(&euid0, &euid1) == 0 ) {
        sprintf(save_filename, "cpara.%08x%08x", euid0, euid1);
    }
}

static void motionEvent( int x, int y )
{
    unsigned char   *p, *p1;
    int             ssx, ssy, eex, eey;
    int             i, j, k;

	if( x < 0 ) x = 0;
	if( x >= xsize ) x = xsize-1;
	if( y < 0 ) y = 0;
	if( y >= ysize ) y = ysize-1;

	x *= SCALE;
	y *= SCALE;

    if( status == 1 && sx != -1 && sy != -1 ) {
        ex = x;
        ey = y;

        if( sx < ex ) { ssx = sx; eex = ex; }
         else         { ssx = ex; eex = sx; }
        if( sy < ey ) { ssy = sy; eey = ey; }
         else         { ssy = ey; eey = sy; }
        p1 = clipImage;
        for( j = ssy; j <= eey; j++ ) {
            p = &(patt.savedImage[patt.loop_num-1][(j*xsize+ssx)*pixelSize]);
            for( i = ssx; i <= eex; i++ ) {
				if (pixelFormat == AR_PIXEL_FORMAT_BGRA || pixelFormat == AR_PIXEL_FORMAT_RGBA) {
				   k = (255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3;
				  if( k < thresh ) k = 0;
				  else k = 255;
				  *(p1+0) = *(p1+1) = *(p1+2) = k;
				}
				else if (pixelFormat == AR_PIXEL_FORMAT_ABGR || pixelFormat == AR_PIXEL_FORMAT_ARGB) {
					k = (255*3 - (*(p+1) + *(p+2) + *(p+3))) / 3;
					if( k < thresh ) k = 0;
					else k = 255;
					*(p1+1) = *(p1+2) = *(p1+3) = k;
				}
				else if (pixelFormat == AR_PIXEL_FORMAT_BGR || pixelFormat == AR_PIXEL_FORMAT_RGB) {
					k = (255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3;
					if( k < thresh ) k = 0;
					else k = 255;
					*(p1+0) = *(p1+1) = *(p1+2) = k;
				}
				else if (pixelFormat == AR_PIXEL_FORMAT_MONO || pixelFormat == AR_PIXEL_FORMAT_420v || pixelFormat == AR_PIXEL_FORMAT_420f) {
					k = 255 - *p;
					if( k < thresh ) k = 0;
					else k = 255;
					*p1 = k;
				}
				else if (pixelFormat == AR_PIXEL_FORMAT_2vuy) {
					k = 255 - *(p+1);
					if( k < thresh ) k = 0;
					else k = 255;
					*(p1+1) = k;
				}
				else if (pixelFormat == AR_PIXEL_FORMAT_yuvs) {
					k = 255 - *p;
					if( k < thresh ) k = 0;
					else k = 255;
					*p1 = k;
				}
                p  += pixelSize;
                p1 += pixelSize;
            }
        }
    }
}

static void mouseEvent(int button, int state, int x, int y)
{
    AR2VideoBufferT *buff;
    unsigned char   *p, *p1;
    int             ssx, ssy, eex, eey;
    int             i, j, k;
    char            line[256];

	if( x < 0 ) x = 0;
	if( x >= xsize ) x = xsize-1;
	if( y < 0 ) y = 0;
	if( y >= ysize ) y = ysize-1;

	x *= SCALE;
	y *= SCALE;

    if( button == GLUT_RIGHT_BUTTON  && state == GLUT_UP ) {
        if( status == 0 ) {
            arVideoCapStop();
            arVideoClose();

            if( patt.loop_num > 0 ) {
                calc_distortion( &patt, xsize, ysize, aspect_ratio, dist_factor, dist_function_version );
                ARLOG("--------------\n");
				if (dist_function_version == 3) {
					ARLOG("Center X: %f\n", dist_factor[0]);
					ARLOG("       Y: %f\n", dist_factor[1]);
					ARLOG("Size Adjust: %f\n", dist_factor[2]);
					ARLOG("Aspect Ratio: %f\n", dist_factor[3]);
					ARLOG("Dist Factor1: %f\n", dist_factor[4]);
					ARLOG("Dist Factor2: %f\n", dist_factor[5]);
				} else if (dist_function_version == 2) {
					ARLOG("Center X: %f\n", dist_factor[0]);
					ARLOG("       Y: %f\n", dist_factor[1]);
					ARLOG("Size Adjust: %f\n", dist_factor[2]);
					ARLOG("Dist Factor1: %f\n", dist_factor[3]);
					ARLOG("Dist Factor2: %f\n", dist_factor[4]);
				} else if (dist_function_version == 1) {
					ARLOG("Center X: %f\n", dist_factor[0]);
					ARLOG("       Y: %f\n", dist_factor[1]);
					ARLOG("Size Adjust: %f\n", dist_factor[2]);
					ARLOG("Dist Factor: %f\n", dist_factor[3]);
				}
                ARLOG("--------------\n");
                status = 2;
                check_num = 0;
                print_comment(5);
            }
            else {
                exit(0);
            }
        }
        else if( status == 1 ) {
            if( patt.loop_num == 0 ) {ARLOGe("error!!\n"); exit(0);}
            patt.loop_num--;
            free( patt.point[patt.loop_num] );
            free( patt.savedImage[patt.loop_num] );
            status = 0;
            point_num = 0;
            arVideoCapStart();

            if( patt.loop_num == 0 ) print_comment(0);
             else                    print_comment(4);
        }
    }

    if( button == GLUT_LEFT_BUTTON  && state == GLUT_DOWN ) {
        if( status == 1 && point_num < patt.h_num*patt.v_num ) {
            sx = ex = x;
            sy = ey = y;

            p  = &(patt.savedImage[patt.loop_num-1][(y*xsize+x)*pixelSize]);
            p1 = &(clipImage[0]);
			if (pixelFormat == AR_PIXEL_FORMAT_BGRA || pixelFormat == AR_PIXEL_FORMAT_RGBA) {
				k = (255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3;
				if( k < thresh ) k = 0;
                else k = 255;
				*(p1+0) = *(p1+1) = *(p1+2) = k;
			}
			else if (pixelFormat == AR_PIXEL_FORMAT_ARGB || pixelFormat == AR_PIXEL_FORMAT_ABGR) {
				k = (255*3 - (*(p+1) + *(p+2) + *(p+3))) / 3;
				if( k < thresh ) k = 0;
                else k = 255;
				*(p1+1) = *(p1+2) = *(p1+3) = k;
			}
			else if (pixelFormat == AR_PIXEL_FORMAT_BGR || pixelFormat == AR_PIXEL_FORMAT_RGB) {
				k = (255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3;
				if( k < thresh ) k = 0;
                else k = 255;
				*(p1+0) = *(p1+1) = *(p1+2) = k;
			}
			else if (pixelFormat == AR_PIXEL_FORMAT_MONO || pixelFormat == AR_PIXEL_FORMAT_420v || pixelFormat == AR_PIXEL_FORMAT_420f) {
				k = 255 - *p;
				if( k < thresh ) k = 0;
                else k = 255;
				*p1 = k;
			}
			else if (pixelFormat == AR_PIXEL_FORMAT_2vuy) {
				k = 255 - *(p+1);
				if( k < thresh ) k = 0;
                else k = 255;
				*(p1+1) = k;
			}
			else if (pixelFormat == AR_PIXEL_FORMAT_yuvs) {
				k = 255 - *p;
				if( k < thresh ) k = 0;
                else k = 255;
				*p1 = k;
			}
        }
    }

    if( button == GLUT_LEFT_BUTTON  && state == GLUT_UP ) {
        if( status == 0 && patt.loop_num < LOOP_MAX ) {
            while (!(buff = arVideoGetImage()) || !buff->fillFlag) arUtilSleep(2);
            p = buff->buff;
            patt.savedImage[patt.loop_num] = (unsigned char *)malloc( xsize*ysize*pixelSize );
            if( patt.savedImage[patt.loop_num] == NULL ) exit(0);

            p1 = patt.savedImage[patt.loop_num];
            for(i=0;i<xsize*ysize*pixelSize;i++) *(p1++) = *(p++);
            arVideoCapStop();

            patt.point[patt.loop_num] = (CALIB_COORD_T *)malloc( sizeof(CALIB_COORD_T)*patt.h_num*patt.v_num );
            if( patt.point[patt.loop_num] == NULL ) exit(0);

            patt.loop_num++;
            status = 1;
            sx = sy = ex= ey = -1;

            print_comment(1);
        }
        else if( status == 1 && point_num == patt.h_num*patt.v_num ) {
            status = 0;
            point_num = 0;
            arVideoCapStart();

            ARLOG("### No.%d ###\n", patt.loop_num);
            for( j = 0; j < patt.v_num; j++ ) {
                for( i = 0; i < patt.h_num; i++ ) {
                    ARLOG("%2d, %2d: %6.2f, %6.2f\n", i+1, j+1,
                           patt.point[patt.loop_num-1][j*patt.h_num+i].x_coord,
                           patt.point[patt.loop_num-1][j*patt.h_num+i].y_coord);
                }
            }
            ARLOG("\n\n");
            if( patt.loop_num < LOOP_MAX ) print_comment(4);
             else                          print_comment(6);
        }
        else if( status == 1 ) {
            if( sx < ex ) { ssx = sx; eex = ex; }
             else         { ssx = ex; eex = sx; }
            if( sy < ey ) { ssy = sy; eey = ey; }
             else         { ssy = ey; eey = sy; }

            patt.point[patt.loop_num-1][point_num].x_coord = 0.0;
            patt.point[patt.loop_num-1][point_num].y_coord = 0.0;
            p = clipImage;
            k = 0;
            for( j = 0; j < (eey-ssy+1); j++ ) {
                for( i = 0; i < (eex-ssx+1); i++ ) {
                    if( pixelSize == 1 ) {
                        patt.point[patt.loop_num-1][point_num].x_coord += i * *p;
                        patt.point[patt.loop_num-1][point_num].y_coord += j * *p;
                        k += *p;
                    }
                    else {
                        patt.point[patt.loop_num-1][point_num].x_coord += i * *(p+1);
                        patt.point[patt.loop_num-1][point_num].y_coord += j * *(p+1);
                        k += *(p+1);
                    }
                    p += pixelSize;
                }
            }
            if( k != 0 ) {
                patt.point[patt.loop_num-1][point_num].x_coord /= k;
                patt.point[patt.loop_num-1][point_num].y_coord /= k;
                patt.point[patt.loop_num-1][point_num].x_coord += ssx;
                patt.point[patt.loop_num-1][point_num].y_coord += ssy;
                point_num++;
            }
            sx = sy = ex= ey = -1;

            ARLOG(" # %d/%d\n", point_num, patt.h_num*patt.v_num);
            if( point_num == patt.h_num*patt.v_num ) print_comment(2);
        }
        else if( status == 2 ) {
            check_num++;
            if( check_num == patt.loop_num ) {
                if(patt.loop_num >= 2) {
                    if( calc_inp(&patt, dist_factor, xsize, ysize, mat, dist_function_version) < 0 ) {
                        ARLOGe("Calibration failed.\n");
                        exit(0);
                    }
                    save_param();
					if (dist_function_version == 3) {
						printf("Do you want to repeat again?");
						scanf("%s", line);
						if( line[0] == 'y' ) {
							aspect_ratio *= mat[0][0] / mat[1][1];
							ARLOG("New aspect ratio = %f\n", aspect_ratio);
							calc_distortion( &patt, xsize, ysize, aspect_ratio, dist_factor, dist_function_version );
							ARLOG("--------------\n");
							ARLOG("Center X: %f\n", dist_factor[0]);
							ARLOG("       Y: %f\n", dist_factor[1]);
							ARLOG("Size Adjust: %f\n", dist_factor[2]);
							ARLOG("Aspect Ratio: %f\n", dist_factor[3]);
							ARLOG("Dist Factor1: %f\n", dist_factor[4]);
							ARLOG("Dist Factor2: %f\n", dist_factor[5]);
							ARLOG("--------------\n");
							status = 2;
							check_num = 0;
							print_comment(5);
							return;
						}
					}
                }
                exit(0);
            }

            if( check_num+1 == patt.loop_num ) {
                ARLOG("\nLeft Mouse Button: Next Step.\n");
            }
            else {
                ARLOG("   %d/%d.\n", check_num+1, patt.loop_num);
            }
        }
    }

    return;
}

static void keyEvent(unsigned char key, int x, int y)
{
    int threshChange = 0;
	switch (key) {
		case 'T':
		case 't':
			printf("Enter new threshold value (now = %d): ", thresh);
			scanf("%d",&thresh); while( getchar()!='\n' );
				printf("\n");
			break;
		case '1':
		case '-':
			threshChange = -5;
			break;
		case '2':
		case '=':
		case '+':
			threshChange = 5;
			break;
		default:
			break;
	}
	if (threshChange) {
		thresh += threshChange;
		if (thresh < 0) thresh = 0;
		if (thresh > 255) thresh = 255;
		ARLOG("Threshhold changed to %d.\n", thresh);
	}
}

static void dispImage(void)
{
    AR2VideoBufferT *buff;
    double        x, y;
    int           ssx, eex, ssy, eey;
    int           i;

    if( status == 0 ) {
        while (!(buff = arVideoGetImage()) || !buff->fillFlag) arUtilSleep(2);
        argDrawMode2D( vp );
        argDrawImage(buff->buff);
    }

    else if( status == 1 ) {
        argDrawMode2D( vp );
        argDrawImage( patt.savedImage[patt.loop_num-1] );

        for( i = 0; i < point_num; i++ ) {
            x = patt.point[patt.loop_num-1][i].x_coord;
            y = patt.point[patt.loop_num-1][i].y_coord;
            glColor3f( 1.0f, 0.0f, 0.0f );
            argDrawLineByObservedPos( x-10, y, x+10, y );
            argDrawLineByObservedPos( x, y-10, x, y+10 );
        }

        if( sx != -1 && sy != -1 ) {
            if( sx < ex ) { ssx = sx; eex = ex; }
             else         { ssx = ex; eex = sx; }
            if( sy < ey ) { ssy = sy; eey = ey; }
             else         { ssy = ey; eey = sy; }
            dispClipImage( ssx, ssy, eex-ssx+1, eey-ssy+1, clipImage );
        }
    }

    else if( status == 2 ) {
        argDrawMode2D( vp );
        argDrawImage( patt.savedImage[check_num] );
        for( i = 0; i < patt.h_num*patt.v_num; i++ ) {
            x = patt.point[check_num][i].x_coord;
            y = patt.point[check_num][i].y_coord;
            glColor3f( 1.0f, 0.0f, 0.0f );
            argDrawLineByObservedPos( x-10, y, x+10, y );
            argDrawLineByObservedPos( x, y-10, x, y+10 );
        }
        draw_line();
    }

    argSwapBuffers();
}

static void draw_line(void)
{
    double   *x, *y;
    int      max;
    int      i, j, k, l;
    int      p;

    max = (patt.v_num > patt.h_num)? patt.v_num: patt.h_num;
    x = (double *)malloc( sizeof(double)*max );
    y = (double *)malloc( sizeof(double)*max );
    if( x == NULL || y == NULL ) exit(0);

    i = check_num;

    for( j = 0; j < patt.v_num; j++ ) {
        for( k = 0; k < patt.h_num; k++ ) {
            x[k] = patt.point[i][j*patt.h_num+k].x_coord;
            y[k] = patt.point[i][j*patt.h_num+k].y_coord;
        }
        draw_line2( x, y, patt.h_num );
    }

    for( j = 0; j < patt.h_num; j++ ) {
        for( k = 0; k < patt.v_num; k++ ) {
            x[k] = patt.point[i][k*patt.h_num+j].x_coord;
            y[k] = patt.point[i][k*patt.h_num+j].y_coord;
        }
        draw_line2( x, y, patt.v_num );
    }

    for( j = 3 - patt.v_num; j < patt.h_num - 2; j++ ) {
        p = 0;
        for( k = 0; k < patt.v_num; k++ ) {
            l = j+k;
            if( l < 0 || l >= patt.h_num ) continue;
            x[p] = patt.point[i][k*patt.h_num+l].x_coord;
            y[p] = patt.point[i][k*patt.h_num+l].y_coord;
            p++;
        }
        draw_line2( x, y, p );
    }

    for( j = 2; j < patt.h_num + patt.v_num - 3; j++ ) {
        p = 0;
        for( k = 0; k < patt.v_num; k++ ) {
            l = j-k;
            if( l < 0 || l >= patt.h_num ) continue;
            x[p] = patt.point[i][k*patt.h_num+l].x_coord;
            y[p] = patt.point[i][k*patt.h_num+l].y_coord;
            p++;
        }
        draw_line2( x, y, p );
    }

    free( x );
    free( y );
}

static void draw_line2( double *x, double *y, int num )
{
    ARMat    *input, *evec;
    ARVec    *ev, *mean;
    double   a, b, c;
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

    arMatrixFree( input );
    arMatrixFree( evec );
    arVecFree( mean );
    arVecFree( ev );

    draw_warp_line(a, b, c);
}

static void draw_warp_line( double a, double b , double c )
{
    ARdouble   x, y;
    ARdouble   x0 = 0, y0 = 0;
    ARdouble   x1, y1;
    int        i;

    glLineWidth( 1.0f );
    glBegin(GL_LINE_STRIP);
    if( a*a >= b*b ) {
        for( i = -20; i <= ysize+20; i+=10 ) {
            x = -(b*i + c)/a;
            y = i;
            arParamIdeal2Observ( dist_factor, x, y, &x1, &y1, dist_function_version );
            if( i != -20 ) {
                argDrawLineByObservedPos( x0, y0, x1, y1 );
            }
            x0 = x1;     
            y0 = y1;  
        }
    }
    else {
        for( i = -20; i <= xsize+20; i+=10 ) {
            x = i;
            y = -(a*i + c)/b;
            arParamIdeal2Observ( dist_factor, x, y, &x1, &y1, dist_function_version );
            if( i != -20 ) {
                argDrawLineByObservedPos( x0, y0, x1, y1 );
            }
            x0 = x1;     
            y0 = y1;  
        }
    }
    glEnd();
}

static void     print_comment( int status )
{
    ARLOG("\n-----------\n");
    switch( status ) {
        case 0:
           ARLOG("Mouse Button\n");
           ARLOG(" Left   : Grab image.\n");
           ARLOG(" Right  : Quit.\n");
           break;
        case 1:
           ARLOG("Mouse Button\n");
           ARLOG(" Left   : Rubber-bounding of feature. (%d x %d)\n", patt.h_num, patt.v_num);
           ARLOG(" Right  : Cancel rubber-bounding & Retry grabbing.\n");
           break;
        case 2:
           ARLOG("Mouse Button\n");
           ARLOG(" Left   : Save feature position.\n");
           ARLOG(" Right  : Discard & Retry grabbing.\n");
           break;
        case 4:
           ARLOG("Mouse Button\n");
           ARLOG(" Left   : Grab next image.\n");
           ARLOG(" Right  : Calc parameter.\n");
           break;
        case 5:
           ARLOG("Mouse Button\n");
           ARLOG(" Left   : Check fittness.\n");
           ARLOG(" Right  :\n");
           ARLOG("   %d/%d.\n", check_num+1, patt.loop_num);
           break;
        case 6:
           ARLOG("Mouse Button\n");
           ARLOG(" Left   :\n");
           ARLOG(" Right  : Calc parameter.\n");
           ARLOG("   %d/%d.\n", check_num+1, patt.loop_num);
           break;
    }
    ARLOG("-----------\n");
}

static void dispClipImage( int sx, int sy, int xsize, int ysize, ARUint8 *clipImage )
{
    float   ssx, ssy;

    ssx = sx/SCALE - 0.5f;
    ssy = sy/SCALE - 0.5f;
    glPixelZoom( (GLfloat)1.0f/SCALE, (GLfloat)-1.0f/SCALE);
    glRasterPos2f( ssx, ssy );

	if( pixelFormat == AR_PIXEL_FORMAT_ABGR ) {
		glDrawPixels( xsize, ysize, GL_ABGR_EXT, GL_UNSIGNED_BYTE, clipImage );
	} else if( pixelFormat == AR_PIXEL_FORMAT_ARGB ) {
#ifdef AR_BIG_ENDIAN
		glDrawPixels( xsize, ysize, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, clipImage );
#else
		glDrawPixels( xsize, ysize, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, clipImage );
#endif
	} else if( pixelFormat == AR_PIXEL_FORMAT_BGRA ) {
		glDrawPixels( xsize, ysize, GL_BGRA, GL_UNSIGNED_BYTE, clipImage );
	} else if( pixelFormat == AR_PIXEL_FORMAT_BGR ) {
		glDrawPixels( xsize, ysize, GL_BGR, GL_UNSIGNED_BYTE, clipImage );
	} else if( pixelFormat == AR_PIXEL_FORMAT_RGB ) {
		glDrawPixels( xsize, ysize, GL_RGB, GL_UNSIGNED_BYTE, clipImage );
	} else if( pixelFormat == AR_PIXEL_FORMAT_RGBA ) {
		glDrawPixels( xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, clipImage );
	} else if( pixelFormat == AR_PIXEL_FORMAT_MONO  || pixelFormat == AR_PIXEL_FORMAT_420v || pixelFormat == AR_PIXEL_FORMAT_420f) {
		glDrawPixels( xsize, ysize, GL_LUMINANCE, GL_UNSIGNED_BYTE, clipImage );
	} else if( pixelFormat == AR_PIXEL_FORMAT_2vuy ) {
#ifdef AR_BIG_ENDIAN
		glDrawPixels( xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, clipImage );
#else
		glDrawPixels( xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, clipImage );
#endif
	} else if( pixelFormat == AR_PIXEL_FORMAT_yuvs ) {
#ifdef AR_BIG_ENDIAN
		glDrawPixels( xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, clipImage );
#else
		glDrawPixels( xsize, ysize, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_REV_APPLE, clipImage );
#endif
	}
}
