/*
 *  surfSub.h
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

#ifndef SURF_SUB_H
#define SURF_SUB_H

#include <AR/ar.h>

#define    SURF_SUB_DEFAULT_OCTAVES         4
#define    SURF_SUB_DEFAULT_INIT_STEP       2
#define    SURF_SUB_DEFAULT_THRESH    (int)(0.0004*255*255)

#define    SURF_SUB_OCTAVES                 5
#define    SURF_SUB_INTERVALS               4
#define    SURF_SUB_DIMENSION              64

#define    SURF_SUB_INTEREST_POINT_MAX   4000
#define    SURF_SUB_RESPONSE_LAYER_MAX     20

typedef struct {
    float             x;
    float             y;
} SurfSubCoord2D;

typedef struct {
    SurfSubCoord2D    vertex[4];
} SurfSubRect;

typedef struct {
    SurfSubRect       rect;
    float             param[4][3];
    int               min;
    int               max;
} SurfSubSkipRegion;

typedef struct {
    float             x, y;               // Coordinates of the detected interest point
    float             scale;              // Detected scale
    float             orientation;        // Orientation measured anti-clockwise from +ve x-axis
    int               laplacian;          // Sign of laplacian for fast matching purposes
    float             descriptor[64];     // Vector of descriptor components
    int               value;
} SurfSubIPointT;

typedef struct {
    SurfSubIPointT    iPoint[SURF_SUB_INTEREST_POINT_MAX];
    int               num;
} SurfSubIPointArrayT;

typedef struct {
    int               width;
    int               height;
    int               step;
    int               filter;     // Filter size for this layer.
    int              *responses;
    unsigned char    *laplacian;
} SurfSubResponseLayerT;

typedef struct {
    SurfSubResponseLayerT   *responseLayer[SURF_SUB_RESPONSE_LAYER_MAX];
    int                      num;
} SurfSubResponseMapT;

typedef struct {
    int                     *image;
    SurfSubResponseMapT     *responseMap;
    SurfSubIPointArrayT      iPointArray;
    int                      width;
    int                      height;
    int                      border;
    AR_PIXEL_FORMAT          pixFormat;
    int                      octaves;
    int                      initStep;
    int                      thresh;
    int                      maxPointNum;
    int                      threadNum;
} SurfSubHandleT;

#ifdef __cplusplus
extern "C" {
#endif

SurfSubHandleT *surfSubCreateHandle         ( int width, int height, AR_PIXEL_FORMAT pixFormat );
int             surfSubDeleteHandle         ( SurfSubHandleT **surfHandle );
int             surfSubGetThresh            ( SurfSubHandleT  *surfHandle, int *thresh );
int             surfSubSetThresh            ( SurfSubHandleT  *surfHandle, int  thresh );
int             surfSubGetMaxPointNum       ( SurfSubHandleT  *surfHandle, int *maxPointNum );
int             surfSubSetMaxPointNum       ( SurfSubHandleT  *surfHandle, int  maxPointNum );
int             surfSubSetThreadNum         ( SurfSubHandleT  *surfHandle, int  threadNum );

int             surfSubExtractFeaturePoint  ( SurfSubHandleT  *surfHandle, unsigned char *image, SurfSubSkipRegion *region, int regionNum );

int             surfSubGetFeaturePointNum   ( SurfSubHandleT  *surfHandle );
int             surfSubGetFeaturePosition   ( SurfSubHandleT  *surfHandle, int index, float *x, float *y );
int             surfSubGetFeatureSign       ( SurfSubHandleT  *surfHandle, int index );
float          *surfSubGetFeatureDescPtr    ( SurfSubHandleT  *surfHandle, int index );

#ifdef __cplusplus
}
#endif
#endif
