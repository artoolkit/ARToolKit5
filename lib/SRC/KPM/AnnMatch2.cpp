/*
 *  AnnMatch2.cpp
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
#include "AnnMatch2.h"

CAnnMatch2::CAnnMatch2(void)
{
    flann_index1 = NULL;
    flann_index2 = NULL;
    index1 = NULL;
    index2 = NULL;
    annThresh = (float)Ann2Thresh;
}

CAnnMatch2::~CAnnMatch2(void)
{
    if( flann_index1 ) delete flann_index1;
    if( flann_index2 ) delete flann_index2;
    if( index1 ) free( index1 );
    if( index2 ) free( index2 );
}

void CAnnMatch2::Construct(FeatureVector* fv)
{
    if( flann_index1 ) {
        delete flann_index1;
        flann_index1 = NULL;
    }
    if( flann_index2 ) {
        delete flann_index2;
        flann_index2 = NULL;
    }
    if (index1) {
        free(index1);
        index1 = NULL;
    }
    if (index2) {
        free(index2);
        index2 = NULL;
    }
    
    if (fv->num == 0) {
        ARLOGw("Warning: CAnnMatch2::Construct called with no features.\n");
        return;
    }
    
    int  num1=0, num2=0;
    for( int i = 0; i < fv->num; i++ ) {
#if 0
        if( fv->sf[i].l ) num1++;
        else              num2++;
#else
        switch( fv->sf[i].l ) {
            case 0: num2++; break;
            case 1: num1++; break;
            default: num1++; num2++;
        }
#endif
    }
    index1 = (int*)malloc(sizeof(int)*num1);
    index2 = (int*)malloc(sizeof(int)*num2);
    
    //-------------------------------------------------------------------------
    // ANN Initialization
    //-------------------------------------------------------------------------
    
    features1.create(num1, SURF_SUB_DIMENSION, CV_32F);
    features2.create(num2, SURF_SUB_DIMENSION, CV_32F);
    float* feature_ptr1 = features1.ptr<float>(0);
    float* feature_ptr2 = features2.ptr<float>(0);
    for(int i = 0, j1 = 0, j2 = 0; i < fv->num; i++ ) {
#if 0
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
#else
        if( fv->sf[i].l != 0 ) {
            memcpy(feature_ptr1, fv->sf[i].v, SURF_SUB_DIMENSION*sizeof(float));
            feature_ptr1 += SURF_SUB_DIMENSION;
            index1[j1++] = i;
        }
        if( fv->sf[i].l != 1 ) {
            memcpy(feature_ptr2, fv->sf[i].v, SURF_SUB_DIMENSION*sizeof(float));
            feature_ptr2 += SURF_SUB_DIMENSION;
            index2[j2++] = i;
        }
#endif
    }
    flann_index1 = new cv::flann::Index(features1, cv::flann::KDTreeIndexParams(4));
    flann_index2 = new cv::flann::Index(features2, cv::flann::KDTreeIndexParams(4));
    //flann_index1 = new cv::flann::Index(features1, cv::flann::LinearIndexParams());
    //flann_index2 = new cv::flann::Index(features2, cv::flann::LinearIndexParams());
    //flann_index1 = new cv::flann::Index(features1, cv::flann::AutotunedIndexParams());
    //flann_index2 = new cv::flann::Index(features2, cv::flann::AutotunedIndexParams());
}

void CAnnMatch2::Match(FeatureVector* fv, int knn, int *result)
{
    int  num1=0, num2=0;
    
    if (fv->num == 0) {
        ARLOGw("Warning: CAnnMatch2::Match called with no features.\n");
        return;
    }

    for( int i = 0; i < fv->num; i++ ) {
        if( fv->sf[i].l ) num1++;
        else              num2++;
    }
    int* qindex1 = (int*)malloc(sizeof(int)*num1);
    int* qindex2 = (int*)malloc(sizeof(int)*num2);
    
    cv::Mat query1(num1, SURF_SUB_DIMENSION, CV_32F);
    cv::Mat m_indices1(num1, knn, CV_32S);
    cv::Mat m_dists1(num1, knn, CV_32F);
    cv::Mat query2(num2, SURF_SUB_DIMENSION, CV_32F);
    cv::Mat m_indices2(num2, knn, CV_32S);
    cv::Mat m_dists2(num2, knn, CV_32F);
    float* query_ptr1 = query1.ptr<float>(0);
    float* query_ptr2 = query2.ptr<float>(0);
    for(int i = 0, j1 = 0, j2 = 0; i < fv->num; i++ ) {
        if( fv->sf[i].l ) {
            memcpy(query_ptr1, fv->sf[i].v, SURF_SUB_DIMENSION*sizeof(float));
            query_ptr1 += SURF_SUB_DIMENSION;
            qindex1[j1++] = i;
        }
        else {
            memcpy(query_ptr2, fv->sf[i].v, SURF_SUB_DIMENSION*sizeof(float));
            query_ptr2 += SURF_SUB_DIMENSION;
            qindex2[j2++] = i;
        }
    }
    
    flann_index1->knnSearch(query1, m_indices1, m_dists1, knn, cv::flann::SearchParams(32) );
    flann_index2->knnSearch(query2, m_indices2, m_dists2, knn, cv::flann::SearchParams(32) );
    
    int*    indices_ptr = m_indices1.ptr<int>(0);
    float*  dists_ptr   = m_dists1.ptr<float>(0);
    for(int i = 0; i < num1; i++ ) {
        for(int j = 0; j < knn; j++ ) {
            if( *dists_ptr <  annThresh ) {
                result[qindex1[i]*knn+j] = index1[*indices_ptr];
            }
            else {
                result[qindex1[i]*knn+j] = -1;
            }
            dists_ptr++;
            indices_ptr++;
        }
    }

    indices_ptr = m_indices2.ptr<int>(0);
    dists_ptr = m_dists2.ptr<float>(0);
    for (int i = 0; i < num2; i++ ) {
        for(int j = 0; j < knn; j++ ) {
            if( *dists_ptr <  annThresh ) {
                result[qindex2[i]*knn+j] = index2[*indices_ptr];
            }
            else {
                result[qindex2[i]*knn+j] = -1;
            }
            dists_ptr++;
            indices_ptr++;
        }
    }
    
    free(qindex1);
    free(qindex2);
    
    return;
}
