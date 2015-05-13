/*
 *  surfSub2.cpp
 *  libKPM
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not compile, install, use, or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive,
 *  non-transferable license, under Daqri's copyrights in this original Daqri
 *  software (the "Daqri Software"), to compile, install and execute Daqri Software
 *  exclusively in conjunction with the ARToolKit software development kit version 5.2
 *  ("ARToolKit"). The allowed usage is restricted exclusively to the purposes of
 *  two-dimensional surface identification and camera pose extraction and initialisation,
 *  provided that applications involving automotive manufacture or operation, military,
 *  and mobile mapping are excluded.
 *
 *  You may reproduce and redistribute the Daqri Software in source and binary
 *  forms, provided that you redistribute the Daqri Software in its entirety and
 *  without modifications, and that you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri, LLC. All rights reserved.
 *  Copyright 2006-2015 ARToolworks, Inc. All rights reserved.
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <stdio.h>
#include <math.h>
#ifdef _WIN32
#  define lroundf(x) ((x)>=0.0f?(long)((x)+0.5f):(long)((x)-0.5f))
#endif
#include <assert.h>
#include <thread_sub.h>
#include <AR/ar.h>
#include <KPM/surfSub.h>
#include "surfSubPrivate.h"

static float gaussTable[600][100];
static int   gaussTableF = 0;

void surfSubGenGaussTable(void)
{
    if( gaussTableF ) return;
    
    for(int i=0; i<600; i++) {
        for(int j=1; j<100; j++) {
            float l = (float)i;
            float s = j*0.1f;
            gaussTable[i][j] = (1.0f/(2.0f*(float)M_PI*s*s)) * expf( -l/(2.0f*s*s));
        }
    }
    gaussTableF = 1;
}

const double gauss25 [7][7] = {
  {0.02350693969273,0.01849121369071,0.01239503121241,0.00708015417522,0.00344628101733,0.00142945847484,0.00050524879060},
  {0.02169964028389,0.01706954162243,0.01144205592615,0.00653580605408,0.00318131834134,0.00131955648461,0.00046640341759},
  {0.01706954162243,0.01342737701584,0.00900063997939,0.00514124713667,0.00250251364222,0.00103799989504,0.00036688592278},
  {0.01144205592615,0.00900063997939,0.00603330940534,0.00344628101733,0.00167748505986,0.00069579213743,0.00024593098864},
  {0.00653580605408,0.00514124713667,0.00344628101733,0.00196854695367,0.00095819467066,0.00039744277546,0.00014047800980},
  {0.00318131834134,0.00250251364222,0.00167748505986,0.00095819467066,0.00046640341759,0.00019345616757,0.00006837798818},
  {0.00131955648461,0.00103799989504,0.00069579213743,0.00039744277546,0.00019345616757,0.00008024231247,0.00002836202103}
};

/*const double gauss33 [11][11] = {
  {0.014614763,0.013958917,0.012162744,0.00966788,0.00701053,0.004637568,0.002798657,0.001540738,0.000773799,0.000354525,0.000148179},
  {0.013958917,0.013332502,0.011616933,0.009234028,0.006695928,0.004429455,0.002673066,0.001471597,0.000739074,0.000338616,0.000141529},
  {0.012162744,0.011616933,0.010122116,0.008045833,0.005834325,0.003859491,0.002329107,0.001282238,0.000643973,0.000295044,0.000123318},
  {0.00966788,0.009234028,0.008045833,0.006395444,0.004637568,0.003067819,0.001851353,0.001019221,0.000511879,0.000234524,9.80224E-05},
  {0.00701053,0.006695928,0.005834325,0.004637568,0.003362869,0.002224587,0.001342483,0.000739074,0.000371182,0.000170062,7.10796E-05},
  {0.004637568,0.004429455,0.003859491,0.003067819,0.002224587,0.001471597,0.000888072,0.000488908,0.000245542,0.000112498,4.70202E-05},
  {0.002798657,0.002673066,0.002329107,0.001851353,0.001342483,0.000888072,0.000535929,0.000295044,0.000148179,6.78899E-05,2.83755E-05},
  {0.001540738,0.001471597,0.001282238,0.001019221,0.000739074,0.000488908,0.000295044,0.00016243,8.15765E-05,3.73753E-05,1.56215E-05},
  {0.000773799,0.000739074,0.000643973,0.000511879,0.000371182,0.000245542,0.000148179,8.15765E-05,4.09698E-05,1.87708E-05,7.84553E-06},
  {0.000354525,0.000338616,0.000295044,0.000234524,0.000170062,0.000112498,6.78899E-05,3.73753E-05,1.87708E-05,8.60008E-06,3.59452E-06},
  {0.000148179,0.000141529,0.000123318,9.80224E-05,7.10796E-05,4.70202E-05,2.83755E-05,1.56215E-05,7.84553E-06,3.59452E-06,1.50238E-06}
};*/

static int   getOrientation( SurfSubIPointT *iPoint, int *image, int width, int height, int border );
static int   getDescriptor( SurfSubIPointT *iPoint, int *image, int width, int height, int border );
static float getAngle(float X, float Y);
inline float gaussian(int x, int y, float sig);
inline float gaussian(float x, float y, float sig);
inline int   haarX(int *img, int width, int height, int border, int row, int column, int s);
inline int   haarY(int *img, int width, int height, int border, int row, int column, int s);

static int compSurfP(const void *a, const void *b)
{
    SurfSubIPointT  *a1, *b1;
    a1 = (SurfSubIPointT*)a;
    b1 = (SurfSubIPointT*)b;
    return b1->value - a1->value;
}

typedef struct {
    SurfSubIPointT   *iPoint;
    char             *flag;
    int              *image;
    int               width;
    int               height;
    int               border;
    int               startNum;
    int               endNum;
} SurfSubGetDescriptorsParam;

static void *surfSubGetDescriptorsSub( THREAD_HANDLE_T *threadHandle );


int surfSubGetDescriptors( SurfSubIPointArrayT *iPointArray, int *image, int width, int height, int border, int maxNum, int threadNum )
{
#if 1
    int  k = 0;
    for(int i = 0; i < iPointArray->num; i++) {
        int     x, y, s;
        x = (int)(iPointArray->iPoint[i].x+0.5);
        y = (int)(iPointArray->iPoint[i].y+0.5);
        s = (int)(iPointArray->iPoint[i].scale+0.5);
        if( x-s*20 < 0 || y-s*20 < 0 || x+s*20 >= width || y+s*20 >= height ) {
            continue;
        }
        if( i != k ) {
            iPointArray->iPoint[k] = iPointArray->iPoint[i];
        }
        k++;
    }
    iPointArray->num = k;
    if( iPointArray->num == 0 ) return 0;
#endif

    if( maxNum > 0 ) {
        if( iPointArray->num > maxNum ) {
            qsort( iPointArray->iPoint, iPointArray->num, sizeof(SurfSubIPointT), compSurfP );
            iPointArray->num = maxNum;
        }
    }

    static int                          initF = 0;
    static int                          threadMax;
    static THREAD_HANDLE_T            **threadHandle;
    static SurfSubGetDescriptorsParam  *arg;
    if( initF == 0 ) {
        threadMax = threadGetCPU();
        threadHandle = (THREAD_HANDLE_T **)malloc(sizeof(THREAD_HANDLE_T*)*threadMax);
        if( threadHandle == NULL ) {ARLOGe("Malloc error: surfSubGetDescriptors.\n"); exit(0);}
        arg = (SurfSubGetDescriptorsParam *)malloc(sizeof(SurfSubGetDescriptorsParam)*threadMax);
        if( arg == NULL ) {ARLOGe("Malloc error: surfSubGetDescriptors.\n"); exit(0);}
        for(int i = 0; i < threadMax; i++ ) {
            threadHandle[i] = threadInit(i, &(arg[i]), surfSubGetDescriptorsSub);
        }
        initF = 1;
    }

    if( threadNum < 0 ) threadNum = threadMax;
    if( threadNum > threadMax ) threadNum = threadMax;
    char *flag = (char *)malloc(iPointArray->num);
    int i = iPointArray->num/threadNum;
    int j = iPointArray->num%threadNum;
    k = 0;
    for(int l = 0; l < j; l++ ) {
        arg[l].iPoint       = iPointArray->iPoint;
        arg[l].flag         = flag;
        arg[l].image        = image;
        arg[l].width        = width;
        arg[l].height       = height;
        arg[l].border       = border;
        arg[l].startNum     = k;
        arg[l].endNum       = k + i;
        threadStartSignal( threadHandle[l] );
        k += (i+1);
    }
    for(int l = j; l < threadNum; l++ ) {
        arg[l].iPoint       = iPointArray->iPoint;
        arg[l].flag         = flag;
        arg[l].image        = image;
        arg[l].width        = width;
        arg[l].height       = height;
        arg[l].border       = border;
        arg[l].startNum     = k;
        arg[l].endNum       = k + i - 1;
        threadStartSignal( threadHandle[l] );
        k += i;
    }
    
    for(int l = 0; l < threadNum; l++ ) {
        threadEndWait( threadHandle[l] );
    }
    
    k = 0;
    for(int i = 0; i < iPointArray->num; i++) {
        if( flag[i] == 0 ) continue;
        if( i != k ) {
            iPointArray->iPoint[k] = iPointArray->iPoint[i];
        }
        k++;
    }
    iPointArray->num = k;
    free(flag);
    
    /*
     for(int i = 0; i < iPointArray->num; i++) {
     if( getOrientation(&(iPointArray->iPoint[i]), image, width, height, border) < 0 ) {
     for( int j = i+1; j < iPointArray->num; j++ ) iPointArray->iPoint[j-1] = iPointArray->iPoint[j];
     iPointArray->num--;
     i--;
     continue;
     }
     
     if( getDescriptor(&(iPointArray->iPoint[i]), image, width, height, border) < 0 ) {
     for( int j = i+1; j < iPointArray->num; j++ ) iPointArray->iPoint[j-1] = iPointArray->iPoint[j];
     iPointArray->num--;
     i--;
     continue;
     }
     }
     */

    return 0;
}

static void *surfSubGetDescriptorsSub( THREAD_HANDLE_T *threadHandle )
{
    SurfSubGetDescriptorsParam  *arg;
    int                          startNum;
    int                          endNum;
    int                          i;
    
    arg = (SurfSubGetDescriptorsParam *)threadGetArg(threadHandle);
    
    for(;;) {
        if( threadStartWait(threadHandle) < 0 ) break;
        startNum = arg->startNum;
        endNum = arg->endNum;
        for( i = startNum; i <= endNum; i++ ) {
            if( getOrientation(&(arg->iPoint[i]), arg->image, arg->width, arg->height, arg->border) < 0 ) {
                arg->flag[i] = 0;
                continue;
            }
            if( getDescriptor(&(arg->iPoint[i]), arg->image, arg->width, arg->height, arg->border) < 0 ) {
                arg->flag[i] = 0;
                continue;
            }
            arg->flag[i] = 1;
        }
        threadEndSignal(threadHandle);
    }
    
    return NULL;
}

static int getOrientation( SurfSubIPointT *iPoint, int *image, int width, int height, int border )
{
    float    resX[109], resY[109], Ang[109];
    float    gauss = 0.0f;
    float    scale = iPoint->scale;
    int      id[] = {6,5,4,3,2,1,0,1,2,3,4,5,6};
    int      c = (int)(iPoint->x + 0.5);
    int      r = (int)(iPoint->y + 0.5);
    int      s = (int)(scale + 0.5);

    // calculate haar responses for points within radius of 6*scale
    int idx = 0;
    for(int i = -6; i <= 6; i++) {
        for(int j = -6; j <= 6; j++) {
            if( i*i + j*j >= 36 ) continue;

            gauss = (float)gauss25[id[i+6]][id[j+6]]; // Or: gauss25[abs(i)][abs(j)]
            resX[idx] = gauss * haarX(image, width, height, border, r+j*s, c+i*s, 4*s);
            resY[idx] = gauss * haarY(image, width, height, border, r+j*s, c+i*s, 4*s);
            Ang[idx]  = getAngle(resX[idx], resY[idx]);
            idx++;
        }
    }

    // calculate the dominant direction 
    float sumX=0.f, sumY=0.f;
    float maxSumX=0.f, maxSumY=0.f;
    float max=0.f;
    float ang1=0.f, ang2=0.f;

    // loop slides pi/3 window around feature point
    for(ang1 = 0; ang1 < 2.0f*M_PI;  ang1+=0.15f) {
        ang2 = ( ang1 + (float)M_PI/3.0f > 2.0f*(float)M_PI ? ang1 - 5.0f*(float)M_PI/3.0f : ang1 + (float)M_PI/3.0f);
        sumX = sumY = 0.f; 
        for (int k = 0; k < idx; ++k) {
            // get angle from the x-axis of the sample point
            const float & ang = Ang[k];

            // determine whether the point is within the window
            if (ang1 < ang2 && ang1 < ang && ang < ang2) {
                sumX+=resX[k];  
                sumY+=resY[k];
            } 
            else if (ang2 < ang1 && ((ang > 0 && ang < ang2) || (ang > ang1 && ang < 2*M_PI) )) {
                sumX+=resX[k];  
                sumY+=resY[k];
            }
        }

        // if the vector produced from this window is longer than all 
        // previous vectors then this forms the new dominant direction
        if (sumX*sumX + sumY*sumY > max) {
            // store largest orientation
            max = sumX*sumX + sumY*sumY;
            maxSumX = sumX;
            maxSumY = sumY;
        }
    }

    // assign orientation of the dominant response vector
    iPoint->orientation = getAngle(maxSumX, maxSumY);

    return 0;
}

//-------------------------------------------------------

//! Get the modified descriptor. See Agrawal ECCV 08
//! Modified descriptor contributed by Pablo Fernandez
static int getDescriptor( SurfSubIPointT *iPoint, int *image, int width, int height, int border )
{
    int        y, x, sample_x, sample_y, count=0;
    int        i = 0, ix = 0, j = 0, jx = 0, xs = 0, ys = 0;
    float      scale, *desc, dx, dy, mdx, mdy, co, si;
    float      gauss_s1 = 0.f, gauss_s2 = 0.f;
    float      rx = 0.f, ry = 0.f, rrx = 0.f, rry = 0.f, len = 0.f;
    float      cx = -0.5f, cy = 0.f; //Subregion centers for the 4x4 gaussian weighting
    
    scale = iPoint->scale;
    x = (int)(iPoint->x + 0.5);
    y = (int)(iPoint->y + 0.5);
    desc = iPoint->descriptor;
    
    co = cosf(iPoint->orientation);
    si = sinf(iPoint->orientation);

    i = -8;

    //Calculate descriptor for this interest point
    while(i < 12) {
        j = -8;
        i = i-4;
        cx += 1.f;
        cy = -0.5f;

        while(j < 12) {
            dx=dy=mdx=mdy=0.f;
            cy += 1.f;
            j  = j - 4;
            ix = i + 5;
            jx = j + 5;

            xs = (int)lroundf(x + (-jx*scale*si + ix*scale*co));
            ys = (int)lroundf(y + ( jx*scale*co + ix*scale*si));

            for (int k = i; k < i + 9; ++k) {
                for (int l = j; l < j + 9; ++l) {
                    //Get coords of sample point on the rotated axis
                    sample_x = (int)lroundf(x + (-l*scale*si + k*scale*co));
                    sample_y = (int)lroundf(y + ( l*scale*co + k*scale*si));

                    //Get the gaussian weighted x and y responses
                    gauss_s1 = gaussian(xs-sample_x,ys-sample_y,2.5f*scale);
                    rx = (float)haarX(image, width, height, border, sample_y, sample_x, 2*((int)(scale+0.5)));
                    ry = (float)haarY(image, width, height, border, sample_y, sample_x, 2*((int)(scale+0.5)));

                    //Get the gaussian weighted x and y responses on rotated axis
                    rrx = gauss_s1*(-rx*si + ry*co);
                    rry = gauss_s1*( rx*co + ry*si);

                    dx += rrx;
                    dy += rry;
                    mdx += fabsf(rrx);
                    mdy += fabsf(rry);
                }
            }

            //Add the values to the descriptor vector
            gauss_s2 = gaussian(cx-2.0f,cy-2.0f,1.5f);

            desc[count++] = dx*gauss_s2;
            desc[count++] = dy*gauss_s2;
            desc[count++] = mdx*gauss_s2;
            desc[count++] = mdy*gauss_s2;

            len += (dx*dx + dy*dy + mdx*mdx + mdy*mdy) * gauss_s2*gauss_s2;

            j += 9;
        }
        i += 9;
    }

    //Convert to Unit Vector
    len = sqrtf(len);
    for(int i = 0; i < 64; ++i) {
        desc[i] /= len;
    }

    return 0;
}


//-------------------------------------------------------

//! Calculate the value of the 2d gaussian at x,y
inline float gaussian(int x, int y, float sig)
{
    int i = x*x + y*y;
    int j = (int)((sig + 0.05f)*10.0f);
    if( i < 600 && j > 0 && j < 100 ) return gaussTable[i][j];
    return (1.0f/(2.0f*(float)M_PI*sig*sig)) * expf( -(x*x + y*y)/(2.0f*sig*sig));
}

//-------------------------------------------------------

//! Calculate the value of the 2d gaussian at x,y
inline float gaussian(float x, float y, float sig)
{
    int i = (int)((x*x + y*y + 0.5f));
    int j = (int)((sig + 0.05f)*10.0f);
    if( i < 600 && j > 0 && j < 100 ) return gaussTable[i][j];
    return 1.0f/(2.0f*(float)M_PI*sig*sig) * expf( -(x*x+y*y)/(2.0f*sig*sig));
}

//-------------------------------------------------------

//! Calculate Haar wavelet responses in x direction
inline int haarX(int *img, int width, int height, int border, int row, int column, int s)
{
    return getBoxIntegral(img, width, height, border, column,     row-s/2, s/2, s) 
         - getBoxIntegral(img, width, height, border, column-s/2, row-s/2, s/2, s);
}

//-------------------------------------------------------

//! Calculate Haar wavelet responses in y direction
inline int haarY(int *img, int width, int height, int border, int row, int column, int s)
{
    return getBoxIntegral(img, width, height, border, column-s/2, row,     s, s/2) 
         - getBoxIntegral(img, width, height, border, column-s/2, row-s/2, s, s/2);
}

//-------------------------------------------------------

//! Get the angle from the +ve x-axis of the vector given by (X Y)
static float getAngle(float X, float Y)
{
    float  angle = atan2f(Y, X);
    if( angle < 0 ) angle += (float)M_PI * 2.0f;

    return angle;
}
