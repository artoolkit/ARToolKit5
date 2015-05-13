/*
 *  HomographyEst.cpp
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
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <AR/ar.h>
#include "HomographyEst.h"

static int  RansacHomograhyEstimationSub1(CorspMap* corspMap, int inlierIndex[], int *num, float h[3][3]);
static int  RansacHomograhyEstimationSub2(CorspMap* corspMap, int inlierIndex[], int *num, float h[3][3]);
static int  ComputeHomography(Point2f* pt1, Point2f* pt2, float h[3][3]);
static bool IsGoodSample(Point2f* points);


int kpmRansacHomograhyEstimation(CorspMap* corspMap, int inlierIndex[], int *num, float h[3][3])
{
    if( corspMap->num <= 3 ) return -1;
    else if( corspMap->num == 4 ) {
        Point2f pt1[4];
        Point2f pt2[4];
        for( int i = 0; i < 4; i++ ) {
            inlierIndex[i] = i;
            pt1[i].x = corspMap->mp[i].x1;
            pt1[i].y = corspMap->mp[i].y1;
            pt2[i].x = corspMap->mp[i].x2;
            pt2[i].y = corspMap->mp[i].y2;
        }
        *num = 4;
        return ComputeHomography(pt1, pt2, h);
    }
    else if( corspMap->num < 10 ) {
        return RansacHomograhyEstimationSub2( corspMap, inlierIndex, num, h );
    }
    else {
        return RansacHomograhyEstimationSub1( corspMap, inlierIndex, num, h );
    }
}

static int RansacHomograhyEstimationSub1(CorspMap* corspMap, int inlierIndex[], int *num, float h[3][3])
{
    static int  *tempIndex;
    static int   tempIndexMax = 0;
    int          tempNum;
    int          maxLoopCount = 200;
	int          loopCount    = 0;

    if( corspMap->num <= 4 ) return -1;

    if( tempIndexMax < corspMap->num ) {
        tempIndexMax = (corspMap->num/100 + 1)*100;
        tempIndex = (int*)malloc(sizeof(int)*tempIndexMax);
    }
    
    *num = 0;
    while( loopCount < maxLoopCount ) {
        Point2f pt1[4];
        Point2f pt2[4];
        int     sample[4];
		for(int i = 0 ; i < 4 ; i++ ) {
            static int s = 0;
            int        j;
            if( s++ == 0 ) srand((unsigned int)time(NULL));
            if( s == 128 ) s = 0;
            
            int pos = (int)((float )corspMap->num * rand() / (RAND_MAX + 1.0F));
            for(j = 0; j < i; j++ ) {
                if( pos == sample[j] ) break;
            }
            if( j < i ) {
                i--;
                continue;
            }
            else {
                sample[i] = pos;
            }
			pt1[i].x = (float)corspMap->mp[pos].x1;
			pt1[i].y = (float)corspMap->mp[pos].y1;
			pt2[i].x = (float)corspMap->mp[pos].x2;
			pt2[i].y = (float)corspMap->mp[pos].y2;
		}
        
		if( (!IsGoodSample(pt1)) || (!IsGoodSample(pt2)) ) {loopCount++; continue;}
        
        float htemp[3][3];
        if( ComputeHomography(pt1, pt2, htemp) < 0 ) {loopCount++; continue;}
        
        tempNum = 0;
        for(int i = 0; i < corspMap->num; i++ ) {
            float xx2, yy2, ww2, err;
            xx2 = htemp[0][0] * corspMap->mp[i].x2 + htemp[0][1] * corspMap->mp[i].y2 + htemp[0][2];
            yy2 = htemp[1][0] * corspMap->mp[i].x2 + htemp[1][1] * corspMap->mp[i].y2 + htemp[1][2];
            ww2 = htemp[2][0] * corspMap->mp[i].x2 + htemp[2][1] * corspMap->mp[i].y2 + htemp[2][2];
            xx2 /= ww2;
            yy2 /= ww2;
            err = (xx2 - corspMap->mp[i].x1)*(xx2 - corspMap->mp[i].x1) + (yy2 - corspMap->mp[i].y1)*(yy2 - corspMap->mp[i].y1);
            if( err < INLIER_THRESH ) {
                tempIndex[tempNum] = i;
                tempNum++;
            }
        }

        if( tempNum > *num ) {
            *num = tempNum;
            for(int j=0;j<3;j++) for(int i=0;i<3;i++) h[j][i] = htemp[j][i];
            for(int i=0; i<*num; i++) inlierIndex[i] = tempIndex[i];
            
            static const float p = log(1.0F-0.99F);
            float e;
            int   N;
			e = (float)tempNum / (float)corspMap->num;
			N = (int)(p / log(1 - e*e*e*e));
            if( N < maxLoopCount ) maxLoopCount = N;
		}
        loopCount++;
	}
    //printf("sampleCount= %d, outlierProb=%f\n", loopCount, 1.0F-(float)(*num)/(float)corspMap->num);
    
    return 0;
}

static int RansacHomograhyEstimationSub2(CorspMap* corspMap, int inlierIndex[], int *num, float h[3][3])
{
    Point2f     pt1[4];
    Point2f     pt2[4];
    int         tempIndex[10];
    int         tempNum;
    int         loopCount = 0;
    
    if( corspMap->num <= 4 ) return -1;
    if( corspMap->num >= 10 ) return -1;
    
    *num = 0;
    for( int i1 = 0; i1 < corspMap->num; i1++ ) {
        pt1[0].x = (float)corspMap->mp[i1].x1;
        pt1[0].y = (float)corspMap->mp[i1].y1;
        pt2[0].x = (float)corspMap->mp[i1].x2;
        pt2[0].y = (float)corspMap->mp[i1].y2;
        for( int i2 = i1+1; i2 < corspMap->num; i2++ ) {
            pt1[1].x = (float)corspMap->mp[i2].x1;
            pt1[1].y = (float)corspMap->mp[i2].y1;
            pt2[1].x = (float)corspMap->mp[i2].x2;
            pt2[1].y = (float)corspMap->mp[i2].y2;
            for( int i3 = i2+1; i3 < corspMap->num; i3++ ) {
                pt1[2].x = (float)corspMap->mp[i3].x1;
                pt1[2].y = (float)corspMap->mp[i3].y1;
                pt2[2].x = (float)corspMap->mp[i3].x2;
                pt2[2].y = (float)corspMap->mp[i3].y2;
                for( int i4 = i3+1; i4 < corspMap->num; i4++ ) {
                    pt1[3].x = (float)corspMap->mp[i4].x1;
                    pt1[3].y = (float)corspMap->mp[i4].y1;
                    pt2[3].x = (float)corspMap->mp[i4].x2;
                    pt2[3].y = (float)corspMap->mp[i4].y2;

                    if( (!IsGoodSample(pt1)) || (!IsGoodSample(pt2)) ) {loopCount++; continue;}
                    
                    float htemp[3][3];
                    if( ComputeHomography(pt1, pt2, htemp) < 0 ) {loopCount++; continue;}
                    
                    tempNum = 0;
                    for(int i = 0; i < corspMap->num; i++ ) {
                        float xx2, yy2, ww2, err;
                        xx2 = htemp[0][0] * corspMap->mp[i].x2 + htemp[0][1] * corspMap->mp[i].y2 + htemp[0][2];
                        yy2 = htemp[1][0] * corspMap->mp[i].x2 + htemp[1][1] * corspMap->mp[i].y2 + htemp[1][2];
                        ww2 = htemp[2][0] * corspMap->mp[i].x2 + htemp[2][1] * corspMap->mp[i].y2 + htemp[2][2];
                        xx2 /= ww2;
                        yy2 /= ww2;
                        err = (xx2 - corspMap->mp[i].x1)*(xx2 - corspMap->mp[i].x1) + (yy2 - corspMap->mp[i].y1)*(yy2 - corspMap->mp[i].y1);
                        if( err < INLIER_THRESH ) {
                            tempIndex[tempNum] = i;
                            tempNum++;
                        }
                    }
                    
                    if( tempNum > *num ) {
                        *num = tempNum;
                        for(int j=0;j<3;j++) for(int i=0;i<3;i++) h[j][i] = htemp[j][i];
                        for(int i=0; i<*num; i++) inlierIndex[i] = tempIndex[i];
                        
                        static const float p = log(1.0F-0.99F);
                        float e;
                        int   N;
                        e = (float)tempNum / (float)corspMap->num;
                        N = (int)(p / log(1 - e*e*e*e));
                        if( N < loopCount ) {
                            //printf("sampleCount= %d, outlierProb=%f\n", loopCount, 1.0F-(float)(*num)/(float)corspMap->num);
                            return 0;                            
                        }
                    }
                    loopCount++;
                }
            }
        }
    }
    
    //printf("sampleCount= %d, outlierProb=%f\n", loopCount, 1.0F-(float)(*num)/(float)corspMap->num);
    return 0;
}

static int ComputeHomography(Point2f* pt1, Point2f* pt2, float h[3][3])
{
    ARMatf A, B, C;
    float  m1[8][8], m2[8], *p1, *p2;
    
    A.clm = 8;
    A.row = 8;
    A.m = p1 = &m1[0][0];
    B.clm = 1;
    B.row = 8;
    B.m = p2 = &m2[0];
    C.clm = 1;
    C.row = 8;
    C.m = &h[0][0];
    
    for(int i = 0; i < 4; i++ ) {
        *(p1++) = pt2[i].x;
        *(p1++) = pt2[i].y;
        *(p1++) = 1.0F;
        *(p1++) = 0.0F;
        *(p1++) = 0.0F;
        *(p1++) = 0.0F;
        *(p1++) = -pt2[i].x * pt1[i].x;
        *(p1++) = -pt2[i].y * pt1[i].x;
        *(p1++) = 0.0F;
        *(p1++) = 0.0F;
        *(p1++) = 0.0F;
        *(p1++) = pt2[i].x;
        *(p1++) = pt2[i].y;
        *(p1++) = 1.0F;
        *(p1++) = -pt2[i].x * pt1[i].y;
        *(p1++) = -pt2[i].y * pt1[i].y;
        *(p2++) = pt1[i].x;
        *(p2++) = pt1[i].y;
    }
    if( arMatrixSelfInvf(&A) < 0 ) return -1;
    if( arMatrixMulf(&C, &A, &B) < 0 ) return -1;
    h[2][2] = 1.0F;
    
    return 0;
}

static bool IsGoodSample(Point2f* points)
{
    float   x1, y1, x2, y2, x3, y3;
    float   l;
    
    x1 = points[0].x;
    y1 = points[0].y;
    x2 = points[1].x;
    y2 = points[1].y;
    x3 = points[2].x;
    y3 = points[2].y;
    l = ((y2-y1)*x3 + (x1-x2)*y3 + y1*x2 - x1*y2)/((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    if( l*l < 0.03F*0.03F ) return false;
    l = ((y3-y2)*x1 + (x2-x3)*y1 + y2*x3 - x2*y3)/((x3-x2)*(x3-x2) + (y3-y2)*(y3-y2));
    if( l*l < 0.03F*0.03F ) return false;
    l = ((y1-y3)*x2 + (x3-x1)*y2 + y3*x1 - x3*y1)/((x1-x3)*(x1-x3) + (y1-y3)*(y1-y3));
    if( l*l < 0.03F*0.03F ) return false;
    
    x3 = points[3].x;
    y3 = points[3].y;
    l = ((y2-y1)*x3 + (x1-x2)*y3 + y1*x2 - x1*y2)/((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    if( l*l < 0.03F*0.03F ) return false;
    l = ((y3-y2)*x1 + (x2-x3)*y1 + y2*x3 - x2*y3)/((x3-x2)*(x3-x2) + (y3-y2)*(y3-y2));
    if( l*l < 0.03F*0.03F ) return false;
    l = ((y1-y3)*x2 + (x3-x1)*y2 + y3*x1 - x3*y1)/((x1-x3)*(x1-x3) + (y1-y3)*(y1-y3));
    if( l*l < 0.03F*0.03F ) return false;
    
    x2 = points[2].x;
    y2 = points[2].y;
    l = ((y2-y1)*x3 + (x1-x2)*y3 + y1*x2 - x1*y2)/((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    if( l*l < 0.03F*0.03F ) return false;
    l = ((y3-y2)*x1 + (x2-x3)*y1 + y2*x3 - x2*y3)/((x3-x2)*(x3-x2) + (y3-y2)*(y3-y2));
    if( l*l < 0.03F*0.03F ) return false;
    l = ((y1-y3)*x2 + (x3-x1)*y2 + y3*x1 - x3*y1)/((x1-x3)*(x1-x3) + (y1-y3)*(y1-y3));
    if( l*l < 0.03F*0.03F ) return false;
    
    x1 = points[1].x;
    y1 = points[1].y;
    l = ((y2-y1)*x3 + (x1-x2)*y3 + y1*x2 - x1*y2)/((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    if( l*l < 0.03F*0.03F ) return false;
    l = ((y3-y2)*x1 + (x2-x3)*y1 + y2*x3 - x2*y3)/((x3-x2)*(x3-x2) + (y3-y2)*(y3-y2));
    if( l*l < 0.03F*0.03F ) return false;
    l = ((y1-y3)*x2 + (x3-x1)*y2 + y3*x1 - x3*y1)/((x1-x3)*(x1-x3) + (y1-y3)*(y1-y3));
    if( l*l < 0.03F*0.03F ) return false;
    
	return true;
}
