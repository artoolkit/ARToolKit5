/*
 *  AnnMatch.cpp
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
#include "AnnMatch.h"

CAnnMatch::CAnnMatch(void)
{
    features1 = NULL;
    features2 = NULL;
    index1 = NULL;
    index2 = NULL;
    num1 = 0;
    num2 = 0;
    annThresh  = (float)AnnThresh;
    annThresh2 = (float)AnnThresh2;
}

CAnnMatch::~CAnnMatch(void)
{
    if( features1 ) free(features1);
    if( features2 ) free(features2);
    if( index1 ) free( index1 );
    if( index2 ) free( index2 );
    features1 = NULL;
    features2 = NULL;
    index1 = NULL;
    index2 = NULL;
    num1 = 0;
    num2 = 0;
}

void CAnnMatch::Construct(FeatureVector* fv)
{
    if( features1 ) free(features1);
    if( features2 ) free(features2);
    if( index1 ) free( index1 );
    if( index2 ) free( index2 );

    num1=0;
    num2=0;
    for( int i = 0; i < fv->num; i++ ) {
        if( fv->sf[i].l ) num1++;
        else              num2++;
    }
    if(num1) {
        index1 = (int*)malloc(sizeof(int)*num1);
        features1 = (float*)malloc(sizeof(float)*SURF_SUB_DIMENSION*num1);
    }
    else {
        index1 = NULL;
        features1 = NULL;
    }
    if(num2) {
        index2 = (int*)malloc(sizeof(int)*num2);
        features2 = (float*)malloc(sizeof(float)*SURF_SUB_DIMENSION*num2);
    }
    else {
        index2 = NULL;
        features2 = NULL;
    }
    
    float *feature_ptr1 = features1;
    float *feature_ptr2 = features2;
    for(int i = 0, j1 = 0, j2 = 0; i < fv->num; i++ ) {
        if( fv->sf[i].l ) {
            memcpy(feature_ptr1, fv->sf[i].v, SURF_SUB_DIMENSION*sizeof(float));
            feature_ptr1 += SURF_SUB_DIMENSION;
            index1[j1++] = i;
        }
        else {
            memcpy(feature_ptr2, fv->sf[i].v, SURF_SUB_DIMENSION*sizeof(float));
            feature_ptr2 += SURF_SUB_DIMENSION;
            index2[j2++] = i;
        }
    }
}

#define  CALC_T     1

void CAnnMatch::Match(FeatureVector* fv, int *result)
{
    //float   disMin = 1.0;
    //int     co=0;

    for (int i = 0; i < fv->num; i++ ) {
#if CALC_T
        float    tmax;
#else
        float    smin;
#endif
        int      kmin = -1;
        if( fv->sf[i].l ) {
            float   *fvPtr = features1;
            for(int k = 0; k < num1; k++ ) {
                float *p = fv->sf[i].v;
#if CALC_T
                float t = 0.0;
#else
                float s = 0.0;
#endif
                for(int j = 0; j < SURF_SUB_DIMENSION; j++) {
#if CALC_T
                    t += *(p++) * *(fvPtr++);
#else
                    s += (*p - *fvPtr) * (*p - *fvPtr);
                    p++;
                    fvPtr++;
#endif
                }
                //printf("%f\t%f\n", s, t);
#if CALC_T
                if( t > annThresh2 && (kmin < 0 || t > tmax) ) {
                    kmin = index1[k];
                    tmax = t;
#else
                if( s < annThresh && (kmin < 0 || s < smin) ) {
                    kmin = index1[k];
                    smin = s;
#endif
                }
            }
        }
        else {
            float   *fvPtr = features2;
            for(int k = 0; k < num2; k++ ) {
                float *p = fv->sf[i].v;
#if CALC_T
                float t = 0.0;
#else
                float s = 0.0;
                
#endif
                for(int j = 0; j < SURF_SUB_DIMENSION; j++) {
#if CALC_T
                    t += *(p++) * *(fvPtr++);
#else
                    s += (*p - *fvPtr) * (*p - *fvPtr);
                    p++;
                    fvPtr++;
#endif
                }
                //printf("%f\t%f\n", s, t);
#if CALC_T
                if( t > annThresh2 && (kmin < 0 || t > tmax) ) {
                    kmin = index2[k];
                    tmax = t;
#else
                if( s < annThresh && (kmin < 0 || s < smin) ) {
                    kmin = index2[k];
                    smin = s;
#endif
                }
            }
        }

        result[i] = kmin;
        //if( kmin >= 0 ) {
        //    if( disMin > smin ) disMin = smin;
        //    co++;
        //}
    }
    //printf("Min=%f, co=%d/%d\n", disMin, co, fv->num);

    return;
}
