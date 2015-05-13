/*
 *  surfSub1.cpp
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
#include <assert.h>
#include <AR/ar.h>
#include "surfSubPrivate.h"


static int                    buildResponseLayer ( SurfSubResponseLayerT *rl, int *image, int width, int height, int border, SurfSubSkipRegion *skipRegion, int regionNum );
static SurfSubResponseLayerT *createResponseLayer( int width, int height, int step, int filter );
static int                    deleteResponseLayer( SurfSubResponseLayerT **responseLayer );
static SurfSubResponseMapT   *createResponseMap  ( int width, int height, int octaves, int initStep );
static int                    deleteResponseMap  ( SurfSubResponseMapT **responseMap );
//static int                    isExtremum         (int x, int y, SurfSubResponseLayerT *t, SurfSubResponseLayerT *m, SurfSubResponseLayerT *b);

static int                    genIntegralImage   ( unsigned char *inImage, int width, int height, int border, int pixFormat, int *outImage );
static int                    interpolateStep    ( int x, int y, SurfSubResponseLayerT *t, SurfSubResponseLayerT *m, SurfSubResponseLayerT *b,
                                                   double* xi, double* xr, double* xc );


int surfSubGetFeaturePointNum( SurfSubHandleT  *surfHandle )
{
    return surfHandle->iPointArray.num;
}

int surfSubGetFeaturePosition( SurfSubHandleT  *surfHandle, int index, float *x, float *y )
{
    *x = surfHandle->iPointArray.iPoint[index].x;
    *y = surfHandle->iPointArray.iPoint[index].y;
    return 0;
}

int surfSubGetFeatureSign( SurfSubHandleT  *surfHandle, int index )
{
    return surfHandle->iPointArray.iPoint[index].laplacian;
}

float *surfSubGetFeatureDescPtr( SurfSubHandleT  *surfHandle, int index )
{
    return surfHandle->iPointArray.iPoint[index].descriptor;
}

int surfSubExtractFeaturePoint( SurfSubHandleT *surfHandle, unsigned char *image, SurfSubSkipRegion *skipRegion, int regionNum )
{
    static const int           filter_map[SURF_SUB_OCTAVES][SURF_SUB_INTERVALS] = {{0,1,2,3}, {1,3,4,5}, {3,5,6,7}, {5,7,8,9}, {7,9,10,11}};
    SurfSubResponseLayerT     *b, *m, *t;
    int                        layerBorder;
    int                        scale,scale1;
    int                        filterStep;
    int                        candidate, *p, *p1;

    surfHandle->iPointArray.num = 0;
    genIntegralImage( image, surfHandle->width, surfHandle->height, surfHandle->border, surfHandle->pixFormat, surfHandle->image );
    
    for(int i = 0; i < surfHandle->responseMap->num; i++) {
        buildResponseLayer( surfHandle->responseMap->responseLayer[i], surfHandle->image, surfHandle->width, surfHandle->height, surfHandle->border, skipRegion, regionNum );
    }

    for(int k = 0; k < surfHandle->octaves; k++) {
        for(int l = 0; l <= 1; l++) {
            b = surfHandle->responseMap->responseLayer[filter_map[k][l+0]];
            m = surfHandle->responseMap->responseLayer[filter_map[k][l+1]];
            t = surfHandle->responseMap->responseLayer[filter_map[k][l+2]];
            layerBorder = (t->filter + 1) / (2 * t->step);
            scale = t->step / m->step;

            for(int j = layerBorder+1; j < t->height - layerBorder; j++) {
                p = &(m->responses[(j*(m->width) + (layerBorder+1))*scale]);
                for(int i = layerBorder+1; i < t->width - layerBorder; i++, p+=scale) {
                    candidate = *p;
                    if( candidate < surfHandle->thresh ) continue;

                    if( candidate <= *(p-scale)
                     || candidate <= *(p+scale)
                     || candidate <= *(p-scale*(m->width))
                     || candidate <= *(p-scale*(m->width + 1))
                     || candidate <= *(p-scale*(m->width - 1))
                     || candidate <= *(p+scale*(m->width))
                     || candidate <= *(p+scale*(m->width - 1))
                     || candidate <= *(p+scale*(m->width + 1)) ) continue;

                    p1 = &(t->responses[j*(t->width) + i]);
                    if( candidate <= *p1
                     || candidate <= *(p1-1)
                     || candidate <= *(p1+1)
                     || candidate <= *(p1-(t->width))
                     || candidate <= *(p1-(t->width + 1))
                     || candidate <= *(p1-(t->width - 1))
                     || candidate <= *(p1+(t->width))
                     || candidate <= *(p1+(t->width - 1))
                     || candidate <= *(p1+(t->width + 1)) ) continue;

                    scale1 = t->step / b->step;
                    p1 = &(b->responses[(j*(b->width) + i)*scale1]);
                    if( candidate <= *p1
                     || candidate <= *(p1-scale1)
                     || candidate <= *(p1+scale1)
                     || candidate <= *(p1-scale1*(b->width))
                     || candidate <= *(p1-scale1*(b->width + 1))
                     || candidate <= *(p1-scale1*(b->width - 1))
                     || candidate <= *(p1+scale1*(b->width))
                     || candidate <= *(p1+scale1*(b->width - 1))
                     || candidate <= *(p1+scale1*(b->width + 1)) ) continue;

                    // Get the offsets to the actual location of the extremum
                    filterStep = (m->filter - b->filter);
                    double xi = 0, xr = 0, xc = 0;
                    interpolateStep(i, j, t, m, b, &xi, &xr, &xc );

                    // If point is sufficiently close to the actual extremum
                    if( fabs( xi ) >= 0.5f  ||  fabs( xr ) >= 0.5f  ||  fabs( xc ) >= 0.5f ) continue;
                    int num = surfHandle->iPointArray.num;
                    surfHandle->iPointArray.iPoint[num].x = (float)((i + xc) * t->step);
                    surfHandle->iPointArray.iPoint[num].y = (float)((j + xr) * t->step);
                    surfHandle->iPointArray.iPoint[num].scale = (float)((0.1333f) * (m->filter + xi * filterStep));
                    surfHandle->iPointArray.iPoint[num].laplacian = m->laplacian[(j*(m->width)+i)*scale];
                    surfHandle->iPointArray.iPoint[num].value = candidate;
                    surfHandle->iPointArray.num++;
                    if( surfHandle->iPointArray.num == SURF_SUB_INTEREST_POINT_MAX ) {
                        ARLOGw("Num of feature point1 = %d\n", surfHandle->iPointArray.num);
                        surfSubGetDescriptors( &(surfHandle->iPointArray), surfHandle->image, surfHandle->width, surfHandle->height, surfHandle->border, surfHandle->maxPointNum, surfHandle->threadNum );
                        return 0;
                    }
                }
            }
        }
    }
    surfSubGetDescriptors( &(surfHandle->iPointArray), surfHandle->image, surfHandle->width, surfHandle->height, surfHandle->border, surfHandle->maxPointNum, surfHandle->threadNum );

    return 0;
}

SurfSubHandleT *surfSubCreateHandle(int width, int height, AR_PIXEL_FORMAT pixFormat)
{
    SurfSubHandleT  *surfHandle;

    assert(width > 0 && height > 0);
    if( pixFormat != AR_PIXEL_FORMAT_MONO ) {
        return NULL;
    }
    
    surfHandle = (SurfSubHandleT *)malloc(sizeof(SurfSubHandleT));
    if( surfHandle == NULL ) return NULL;

    surfHandle->width       = width;
    surfHandle->height      = height;
    surfHandle->pixFormat   = AR_PIXEL_FORMAT_MONO;
    surfHandle->octaves     = SURF_SUB_DEFAULT_OCTAVES;
    surfHandle->initStep    = SURF_SUB_DEFAULT_INIT_STEP;
    surfHandle->thresh      = SURF_SUB_DEFAULT_THRESH;
    surfHandle->maxPointNum = -1;
    surfHandle->threadNum   = -1;

    surfHandle->iPointArray.num = 0;

    surfHandle->responseMap = createResponseMap( width, height, surfHandle->octaves, surfHandle->initStep);
    if( surfHandle->responseMap == NULL ) {
        free( surfHandle );
        return NULL;
    }

    int filterSize = 0;
    for(int i = 0; i < surfHandle->responseMap->num; i++) {
        if( surfHandle->responseMap->responseLayer[i]->filter > filterSize ) {
            filterSize = surfHandle->responseMap->responseLayer[i]->filter;
        }
    }
    //surfHandle->border = (filterSize-1)/2 + 1;
    surfHandle->border = filterSize;

    surfHandle->image = (int *)malloc(sizeof(int)*(width + surfHandle->border*2)*(height + surfHandle->border*2));
    if( surfHandle->image == NULL ) {
        deleteResponseMap( &(surfHandle->responseMap) );
        free( surfHandle );
        return NULL;
    }

    surfSubGenGaussTable();

    return surfHandle;
}

int surfSubDeleteHandle( SurfSubHandleT **surfHandle )
{
    if( surfHandle == NULL || *surfHandle == NULL ) return -1;

    deleteResponseMap( &((*surfHandle)->responseMap) );
    free( (*surfHandle)->image );
    free( (*surfHandle) );

    *surfHandle = NULL;

    return 0;
}

int surfSubGetThresh( SurfSubHandleT  *surfHandle, int *thresh )
{
    if( surfHandle == NULL ) return -1;
    *thresh = surfHandle->thresh;
    return 0;
}

int surfSubSetThresh( SurfSubHandleT  *surfHandle, int  thresh )
{
    if( surfHandle == NULL ) return -1;
    surfHandle->thresh = thresh;
    return 0;
}


int surfSubGetMaxPointNum( SurfSubHandleT  *surfHandle, int *maxPointNum )
{
    if( surfHandle == NULL ) return -1;
    *maxPointNum = surfHandle->maxPointNum;
    return 0;
}

int surfSubSetMaxPointNum( SurfSubHandleT  *surfHandle, int  maxPointNum )
{
    if( surfHandle == NULL ) return -1;
    surfHandle->maxPointNum = maxPointNum;
    return 0;
}

int surfSubSetThreadNum( SurfSubHandleT  *surfHandle, int  threadNum )
{
    if( surfHandle == NULL ) return -1;
    surfHandle->threadNum = threadNum;
    return 0;
}


static SurfSubResponseMapT *createResponseMap(int width, int height, int octaves, int initStep)
{
    SurfSubResponseMapT  *responseMap;
    int                   w, h, s;
    int                   filterSizes[12] = {9, 15, 21, 27, 39, 51, 75, 99, 147, 195, 291, 387};
    //int                   filterSizeMax;
    

    responseMap = (SurfSubResponseMapT *)malloc(sizeof(SurfSubResponseMapT));
    if( responseMap == NULL ) {
        ARLOGe("Out of memory!!\n");
        return NULL;
    }

    w = width  / initStep;
    h = height / initStep;
    s = initStep;

    switch( octaves ) {
        case 1: responseMap->num =  4; break;
        case 2: responseMap->num =  6; break;
        case 3: responseMap->num =  8; break;
        case 4: responseMap->num = 10; break;
        case 5: responseMap->num = 12; break;
    }

    // Check for the case where the filter size would be larger than the image.
    /*filterSizeMax = filterSizes[responseMap->num - 1]; // Filter size at maximum depth.
    if (filterSizeMax >= width || filterSizeMax >= height) {
        ARLOGe("Error: Input image size %dx%d is smaller than required minimum size %dx%d.\n", width, height, filterSizeMax + 1, filterSizeMax + 1);
        free(responseMap);
        return (NULL);
    }*/
    
    if(octaves >= 1) {
        responseMap->responseLayer[0] = createResponseLayer(w,   h,   s,   filterSizes[0]);
        responseMap->responseLayer[1] = createResponseLayer(w,   h,   s,   filterSizes[1]);
        responseMap->responseLayer[2] = createResponseLayer(w,   h,   s,   filterSizes[2]);
        responseMap->responseLayer[3] = createResponseLayer(w,   h,   s,   filterSizes[3]);
    }
    if(octaves >= 2) {
        responseMap->responseLayer[4] = createResponseLayer(w/2, h/2, s*2, filterSizes[4]);
        responseMap->responseLayer[5] = createResponseLayer(w/2, h/2, s*2, filterSizes[5]);
    }
    if(octaves >= 3) {
        responseMap->responseLayer[6] = createResponseLayer(w/4, h/4, s*4, filterSizes[6]);
        responseMap->responseLayer[7] = createResponseLayer(w/4, h/4, s*4, filterSizes[7]);
    }
    if(octaves >= 4) {
        responseMap->responseLayer[8] = createResponseLayer(w/8, h/8, s*8, filterSizes[8]);
        responseMap->responseLayer[9] = createResponseLayer(w/8, h/8, s*8, filterSizes[9]);
    }
    if(octaves >= 5) {
        responseMap->responseLayer[10] = createResponseLayer(w/16, h/16, s*16, filterSizes[10]);
        responseMap->responseLayer[11] = createResponseLayer(w/16, h/16, s*16, filterSizes[11]);
    }
    
    return responseMap;
}

static int deleteResponseMap( SurfSubResponseMapT **responseMap )
{
    if( responseMap == NULL || *responseMap == NULL ) return -1;

    for(int i = 0; i < (*responseMap)->num; i++ ) {
        deleteResponseLayer( &((*responseMap)->responseLayer[i]) );
    }
    free( *responseMap );

    *responseMap = NULL;

    return 0;
}

static SurfSubResponseLayerT *createResponseLayer(int width, int height, int step, int filter)
{
    SurfSubResponseLayerT    *responseLayer;

    assert(width > 0 && height > 0);

    responseLayer = (SurfSubResponseLayerT *)malloc(sizeof(SurfSubResponseLayerT));
    if( responseLayer == NULL ) {
        ARLOGe("Out of memory!!\n");
        return NULL;
    }

    responseLayer->width  = width;
    responseLayer->height = height;
    responseLayer->step   = step;
    responseLayer->filter = filter;
    
    responseLayer->responses = (int *)malloc(sizeof(int)*width*height);
    if( responseLayer->responses == NULL ) {
        ARLOGe("Out of memory!!\n");
        free(responseLayer);
        return NULL;
    }
    responseLayer->laplacian = (unsigned char *)malloc(sizeof(unsigned char)*width*height);
    if( responseLayer->laplacian == NULL ) {
        ARLOGe("Out of memory!!\n");
        free(responseLayer->responses);
        free(responseLayer);
        return NULL;
    }

    return responseLayer;
}

static int deleteResponseLayer( SurfSubResponseLayerT **responseLayer )
{
    if( responseLayer == NULL || *responseLayer == NULL ) return -1;

    free( (*responseLayer)->laplacian );
    free( (*responseLayer)->responses );
    free( (*responseLayer) );

    *responseLayer = NULL;

    return 0;
}



static int buildResponseLayer(SurfSubResponseLayerT *rl, int *image, int width, int height, int border, SurfSubSkipRegion *skipRegion, int regionNum)
{
    int           *pr           = rl->responses;             // response storage
    unsigned char *pl           = rl->laplacian;             // laplacian sign storage
    int            step         = rl->step;                  // step size for this filter
    int            b            = (rl->filter - 1) / 2;      // border for this filter
    int            l            = rl->filter / 3;            // lobe for this filter (filter size / 3)
    int            w            = rl->filter;                // filter size
    int            area         = w*w;                       // normalisation factor
    int            Dxx, Dyy, Dxy;
    
    //if (w >= width || w >= height) return (-1); // image is smaller than filter.
    
#if 1
    int   *imgPtr0;
    int   *imgPtr1, *imgPtr2, *imgPtr3, *imgPtr4, *imgPtr5;
    int   *imgPtr6, *imgPtr7, *imgPtr8, *imgPtr9, *imgPtr10;
    int   *imgPtr11, *imgPtr12, *imgPtr13, *imgPtr14, *imgPtr15;
    int    Dxx1, Dxx2, Dyy1, Dyy2, Dxy1, Dxy2, Dxy3, Dxy4;
    int    width2 = width + border*2;
    
    int    r, j;
    for(r = 0, j = 0; j < rl->height; j++, r+=step) {
        for(int ii = 0; ii < regionNum; ii++ ) {
            skipRegion[ii].min = 0;
            skipRegion[ii].max = width;
            float x;
            for(int jj = 0; jj < 4; jj++ ) {
                if( skipRegion[ii].param[jj][0] > 0 ) {
                    x= -(skipRegion[ii].param[jj][1] * r + skipRegion[ii].param[jj][2]) / skipRegion[ii].param[jj][0];
                    if( x > skipRegion[ii].min) skipRegion[ii].min = x;
                }
                else if( skipRegion[ii].param[jj][0] < 0 ) {
                    x = -(skipRegion[ii].param[jj][1] * r + skipRegion[ii].param[jj][2]) / skipRegion[ii].param[jj][0];
                    if( x < skipRegion[ii].max ) skipRegion[ii].max = x;
                }
                else if( r <= -skipRegion[ii].param[jj][2]/skipRegion[ii].param[jj][1] ) {
                    skipRegion[ii].min = width;
                    skipRegion[ii].max = 0;
                }
            }
        }
        
        int c = 0;
        int i = 0;
        for(;;) {
            for(;;) {
                int  ii;
                for( ii = 0; ii < regionNum; ii++ ) {
                    if( c >= skipRegion[ii].min && c < skipRegion[ii].max && c < width ) break;
                }
                if( ii < regionNum ) {
                    int max = (skipRegion[ii].max < rl->width*step)? skipRegion[ii].max: rl->width*step;
                    while( c < max ) {
                        *(pr++) = 0;
                        *(pl++) = 0;
                        c += step;
                        i++;
                    }
                }
                else break;
            }
            if( i >= rl->width ) break;

            int c1 = rl->width*step;
            for(int ii = 0; ii < regionNum; ii++ ) {
                if( skipRegion[ii].min > c && skipRegion[ii].min < c1 ) c1 = skipRegion[ii].min;
            }
            
            imgPtr0 = imgPtr1 = imgPtr6 = imgPtr11 = &image[(r+border)*width2+border];
            imgPtr1 += c-b-1;
            imgPtr2 = imgPtr1 -  l   *width2;
            imgPtr3 = imgPtr1 + (l-1)*width2;
            imgPtr4 = imgPtr2 + b+1 - (l+1)/2;
            imgPtr5 = imgPtr3 + b+1 - (l+1)/2;
            imgPtr6 += c-l;
            imgPtr7 = imgPtr6 - (b+1)*width2;
            imgPtr8 = imgPtr6 + (w-b-1)*width2;
            imgPtr9 = imgPtr6 - (l+1)/2*width2;
            imgPtr10 = imgPtr6 + (l-1)/2*width2;
            imgPtr11 += c;
            imgPtr12 = imgPtr11 - (l+1)*width2;
            imgPtr13 = imgPtr11 - 1*width2;
            imgPtr14 = imgPtr11;
            imgPtr15 = imgPtr11 + l*width2;
            for(; c < c1; i++, c+=step) {
                Dxx1 = *imgPtr2 - *(imgPtr2+w) - *imgPtr3 + *(imgPtr3+w);
                Dxx2 = *imgPtr4 - *(imgPtr4+l) - *imgPtr5 + *(imgPtr5+l);
                Dxx = Dxx1 - Dxx2*3;
                imgPtr2+=step;  imgPtr3+=step;  imgPtr4+=step;  imgPtr5+=step;

                Dyy1 = *imgPtr7 - *(imgPtr7+2*l-1) - *imgPtr8  + *(imgPtr8 +2*l-1);
                Dyy2 = *imgPtr9 - *(imgPtr9+2*l-1) - *imgPtr10 + *(imgPtr10+2*l-1);
                Dyy = Dyy1 - Dyy2*3;
                imgPtr7+=step;  imgPtr8+=step;  imgPtr9+=step;  imgPtr10+=step;

                Dxy1 = *imgPtr12 - *(imgPtr12+l) - *imgPtr13 + *(imgPtr13+l);
                Dxy2 = *(imgPtr14-l-1) - *(imgPtr14-1) - *(imgPtr15-l-1) + *(imgPtr15-1);
                Dxy3 = *(imgPtr12-l-1) - *(imgPtr12-1) - *(imgPtr13-l-1) + *(imgPtr13-1);
                Dxy4 = *imgPtr14 - *(imgPtr14+l) - *imgPtr15 + *(imgPtr15+l);
                Dxy = Dxy1 + Dxy2 - Dxy3 - Dxy4;
                imgPtr12+=step;  imgPtr13+=step;  imgPtr14+=step;  imgPtr15+=step;
            
                Dxx /= area;
                Dyy /= area;
                Dxy /= area;
            
                *(pr++) = (Dxx * Dyy * 100 - Dxy * Dxy * 81)/100;
                *(pl++) = (Dxx + Dyy >= 0 ? 1: 0);
            }
            if( i >= rl->width ) break;
        }
    }
#else
    for(int r = 0, j = 0; j < rl->height; j++, r+=step) {
        for(int c = 0, i = 0; i < rl->width; i++, c+=step) {
            Dxx = getBoxIntegral(image, width, height, border, c-b,       r-l+1, w, 2*l-1)
                - getBoxIntegral(image, width, height, border, c-(l-1)/2, r-l+1, l, 2*l-1) * 3;
            
            Dyy = getBoxIntegral(image, width, height, border, c-l+1, r-b,       2*l-1, w)
                - getBoxIntegral(image, width, height, border, c-l+1, r-(l-1)/2, 2*l-1, l)*3;
            
            Dxy = getBoxIntegral(image, width, height, border, c+1, r-l, l, l)
                + getBoxIntegral(image, width, height, border, c-l, r+1, l, l)
                - getBoxIntegral(image, width, height, border, c-l, r-l, l, l)
                - getBoxIntegral(image, width, height, border, c+1, r+1, l, l);
            
            Dxx /= area;
            Dyy /= area;
            Dxy /= area;
            
            *(pr++) = (Dxx * Dyy * 100 - Dxy * Dxy * 81)/100;
            *(pl++) = (Dxx + Dyy >= 0 ? 1: 0);
        }
    }
#endif
    
    return 0;
}

static int genIntegralImage( unsigned char *inImage, int width, int height, int border, int pixFormat, int *outImage )
{
    unsigned char  *p1;
    int            *p2, *p3;
    int             rs = 0;

    if( pixFormat == AR_PIXEL_FORMAT_MONO ) {
        p1 = inImage;
        p2 = outImage;
        for(int j = 0; j < border; j++) {
            for(int i = 0; i < width+border*2; i++) {
                *(p2++) = 0;
            }
        }

        for(int i = 0; i < border; i++) {
            *(p2++) = 0;
        }
        for( int i = 0; i < width; i++ ) {
            rs += *(p1++);
            *(p2++) = rs;
        }
        for(int i = 0; i < border; i++) {
            *(p2++) = rs;
        }

        for( int j = 1; j < height; j++ ) {
            for(int i = 0; i < border; i++) {
                *(p2++) = 0;
            }
            p3 = p2 - (width+border*2);
            rs = 0;
            for( int i = 0; i < width; i++ ) {
                rs += *(p1++);
                *(p2++) = rs + *(p3++);
            }
            int rs2 = *(p2-1);
            for(int i = 0; i < border; i++) {
                *(p2++) = rs2;
            }
        }

        p3 = p2 - (width+border*2);
        for(int j = 0; j < border; j++ ) {
            for(int i = 0; i < width+border*2; i++) {
                *(p2++) = *(p3++);
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR ) {
        p1 = inImage;
        p2 = outImage;
        for(int j = 0; j < border; j++) {
            for(int i = 0; i < width+border*2; i++) {
                *(p2++) = 0;
            }
        }

        for(int i = 0; i < border; i++) {
            *(p2++) = 0;
        }
        for( int i = 0; i < width; i++ ) {
            rs += (*(p1+0) + *(p1+1) + *(p1+2)) / 3;
            p1 += 3;
            *(p2++) = rs;
        }
        for(int i = 0; i < border; i++) {
            *(p2++) = rs;
        }

        for( int j = 1; j < height; j++ ) {
            for(int i = 0; i < border; i++) {
                *(p2++) = 0;
            }
            p3 = p2 - (width+border*2);
            rs = 0;
            for( int i = 0; i < width; i++ ) {
                rs += (*(p1+0) + *(p1+1) + *(p1+2)) / 3;
                p1 += 3;
                *(p2++) = rs + *(p3++);
            }
            int rs2 = *(p2-1);
            for(int i = 0; i < border; i++) {
                *(p2++) = rs2;
            }
        }

        p3 = p2 - (width+border*2);
        for(int j = 0; j < border; j++ ) {
            for(int i = 0; i < width+border*2; i++) {
                *(p2++) = *(p3++);
            }
        }
    }
    else exit(0);

    return 0;
}

static int interpolateStep( int x, int y, SurfSubResponseLayerT *t, SurfSubResponseLayerT *m, SurfSubResponseLayerT *b,
                            double* xi, double* xr, double* xc )
{
    float    vv[3], mm[3][3], mi[3][3];
    float    d, v, dxx, dyy, dss, dxy, dxs, dys;
    int      scale1, scale2;

    scale1 = t->step / m->step;
    scale2 = t->step / b->step;

    vv[0] = (float)((m->responses[(y*(m->width)+(x+1))*scale1] - m->responses[(y*(m->width)+(x-1))*scale1]) / 2);
    vv[1] = (float)((m->responses[((y+1)*(m->width)+x)*scale1] - m->responses[((y-1)*(m->width)+x)*scale1]) / 2);
    vv[2] = (float)((t->responses[y*(t->width)+x] - b->responses[(y*(b->width)+x)*scale2]) / 2);

    v = (float)(m->responses[(y*(m->width)+x)*scale1]);
    dxx = m->responses[(y*(m->width)+(x+1))*scale1] + m->responses[(y*(m->width)+(x-1))*scale1] - 2*v;
    dyy = m->responses[((y+1)*(m->width)+x)*scale1] + m->responses[((y-1)*(m->width)+x)*scale1] - 2*v;
    dss = t->responses[y*(t->width)+x] + b->responses[(y*(b->width)+x)*scale2] - 2*v;
    dxy = (float)(( m->responses[((y+1)*(m->width)+(x+1))*scale1] - m->responses[((y+1)*(m->width)+(x-1))*scale1]
                  - m->responses[((y-1)*(m->width)+(x+1))*scale1] + m->responses[((y-1)*(m->width)+(x-1))*scale1]) / 4);
    dxs = (float)(( t->responses[y*(t->width)+(x+1)] - t->responses[y*(t->width)+(x-1)]
                  - b->responses[(y*(b->width)+(x+1))*scale2] + b->responses[(y*(b->width)+(x-1))*scale2]) / 4);
    dys = (float)(( t->responses[(y+1)*(t->width)+x] - t->responses[(y-1)*(t->width)+x]
                  - b->responses[((y+1)*(b->width)+x)*scale2] + b->responses[((y-1)*(b->width)+x)*scale2]) / 4);
    mm[0][0] = dxx;
    mm[0][1] = dxy;
    mm[0][2] = dxs;
    mm[1][0] = dxy;
    mm[1][1] = dyy;
    mm[1][2] = dys;
    mm[2][0] = dxs;
    mm[2][1] = dys;
    mm[2][2] = dss;

    d = mm[0][0]*mm[1][1]*mm[2][2]
      + mm[0][1]*mm[1][2]*mm[2][0]
      + mm[0][2]*mm[1][0]*mm[2][1]
      - mm[0][2]*mm[1][1]*mm[2][0]
      - mm[0][1]*mm[1][0]*mm[2][2]
      - mm[0][0]*mm[1][2]*mm[2][1];
    if( d == 0.0 ) return -1;
    mi[0][0] =  (mm[1][1]*mm[2][2] - mm[1][2]*mm[2][1])/d;
    mi[1][0] = -(mm[1][0]*mm[2][2] - mm[1][2]*mm[2][0])/d;
    mi[2][0] =  (mm[1][0]*mm[2][1] - mm[1][1]*mm[2][0])/d;
    mi[0][1] = -(mm[0][1]*mm[2][2] - mm[0][2]*mm[2][1])/d;
    mi[1][1] =  (mm[0][0]*mm[2][2] - mm[0][2]*mm[2][0])/d;
    mi[2][1] = -(mm[0][0]*mm[2][1] - mm[0][1]*mm[2][0])/d;
    mi[0][2] =  (mm[0][1]*mm[1][2] - mm[0][2]*mm[1][1])/d;
    mi[1][2] = -(mm[0][0]*mm[1][2] - mm[0][2]*mm[1][0])/d;
    mi[2][2] =  (mm[0][0]*mm[1][1] - mm[0][1]*mm[1][0])/d;

    *xc = -(mi[0][0]*vv[0] + mi[0][1]*vv[1] + mi[0][2]*vv[2]);
    *xr = -(mi[1][0]*vv[0] + mi[1][1]*vv[1] + mi[1][2]*vv[2]);
    *xi = -(mi[2][0]*vv[0] + mi[2][1]*vv[1] + mi[2][2]*vv[2]);

    return 0;
}
