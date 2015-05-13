/*
 *  kpm.h
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

/*!
	@header kpm
	@abstract libKPM NFT image recognition and tracking initialisation routines.
	@discussion
        This header declares types and API for an NFT tracker,
        in particular those routines involved in recognising a texture page and
        initialising the tracking for use by the texture tracker.
    @copyright 2006-2015 ARToolworks, Inc.
 */

#ifndef KPM_H
#define KPM_H

#define     ANN2    1

#include <AR/ar.h>
#include <KPM/surfSub.h>
#include <KPM/kpmType.h>

#define   KpmPose6DOF            1
#define   KpmPoseHomography      2

#define   KpmProcFullSize        1
#define   KpmProcHalfSize        2
#define   KpmProcQuatSize        3
#define   KpmProcOneThirdSize    4
#define   KpmProcTwoThirdSize    5
#define   KpmDefaultProcMode     KpmProcFullSize

#define   KpmCompNull            0
#define   KpmCompX               1
#define   KpmCompY               2
#define   KpmDefaultComp         KpmCompNull

#define   KpmChangePageNoAllPages (-1)

typedef struct {
    float             x;
    float             y;
} KpmCoord2D;

typedef struct {
    SurfSubSkipRegion    *region;
    int                   regionNum;
    int                   regionMax;
} KpmSkipRegionSet;

typedef struct {
    int               width;
    int               height;
    int               imageNo;
} KpmImageInfo;

typedef struct {
    KpmImageInfo     *imageInfo;
    int               imageNum;
    int               pageNo;
} KpmPageInfo;

typedef struct {
    KpmCoord2D        coord2D;
    KpmCoord2D        coord3D;      // millimetres.
    SurfFeature       featureVec;
    int               pageNo;
    int               refImageNo;
} KpmRefData;

/*!
    @typedef    KpmRefDataSet
    @abstract   A loaded dataset for KPM tracking.
    @discussion
        Key point matching takes as input a reference data set of points. This strucutre holds a set of points in memory prior to loading into the tracker.
	@field		refPoint Tracking reference points.
	@field		num Number of refPoints in the dataset.
	@field		pageInfo Array of info about each page in the dataset. One entry per page.
	@field		pageNum Number of pages in the dataset (i.e. a count, not an index).
 */
typedef struct {
    KpmRefData       *refPoint;
    int               num;
    KpmPageInfo      *pageInfo;
    int               pageNum;
} KpmRefDataSet;

typedef struct {
    KpmCoord2D       *coord;
    int               num;
} KpmInputDataSet;

typedef struct {
    int               refIndex;
    int               inIndex;
} KpmMatchData;

typedef struct {
    KpmMatchData     *match;
    int               num;
} KpmMatchResult;

typedef struct {
    float                     camPose[3][4];
    int                       pageNo;
    float                     error;
    int                       inlierNum;
    int                       camPoseF;
    int                       skipF;
} KpmResult;

typedef struct {
    void                     *ann;
    int                      *annCoordIndex;
    int                       pageID;
    int                       imageID;
} KpmAnnInfo;

typedef struct {
    SurfSubHandleT           *surfHandle;
#if ANN2
    void                     *ann2;
#else
    KpmAnnInfo               *annInfo;
    int                       annInfoNum;
#endif

    ARParamLT                *cparamLT;
    int                       poseMode;
    int                       xsize, ysize;
    AR_PIXEL_FORMAT           pixFormat;
    int                       procMode;
    int                       detectedMaxFeature;
    int                       surfThreadNum;

    KpmRefDataSet             refDataSet;
    KpmInputDataSet           inDataSet;
    KpmMatchResult            preRANSAC;
    KpmMatchResult            aftRANSAC;

    KpmSkipRegionSet          skipRegion;
    
    KpmResult                *result;
    int                       resultNum;
} KpmHandle;


#ifdef __cplusplus
extern "C" {
#endif

/*!
    @function
    @abstract Allocate and initialise essential structures for KPM tracking, using full six degree-of-freedom tracking.
    @discussion 
    @param cparamLT Pointer to an ARParamLT structure holding camera parameters in lookup-table form.
        The pointer only is copied, and the ARParamLT structure itself is NOT copied, and must remain
        valid for the lifetime of the KpmHandle.
        This structure also specifies the size of video frames which will be later supplied to the
        kpmMatching() function as cparamLT->param.xsize and cparamLT->param.ysize.
    @param pixFormat Pixel format of video frames which will be later supplied to the kpmMatching() function.
    @result Pointer to a newly-allocated KpmHandle structure. This structure must be deallocated
        via a call to kpmDeleteHandle() when no longer needed.
    @seealso kpmCreateHandleHomography kpmCreateHandleHomography
    @seealso kpmDeleteHandle kpmDeleteHandle
 */
KpmHandle  *kpmCreateHandle ( ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat );
#define     kpmCreatHandle kpmCreateHandle

KpmHandle  *kpmCreateHandle2( int xsize, int ysize, AR_PIXEL_FORMAT pixFormat );
#define     kpmCreatHandle2 kpmCreateHandle2

/*!
    @function
    @abstract Allocate and initialise essential structures for KPM tracking, using homography-only tracking.
    @discussion
        Homography tracking assumes that the camera has zero lens-distortion, and this does not depend on
        camera parameters. It is therefore unable to provide correctly calibrated position measurements,
        but the resulting pose is suitable for visual overlay purposes.
    @param xsize Width of video frames which will be later supplied to the kpmMatching() function.
    @param ysize Height of video frames which will be later supplied to the kpmMatching() function.
    @param pixFormat Pixel format of video frames which will be later supplied to the kpmMatching() function.
    @result Pointer to a newly-allocated KpmHandle structure. This structure must be deallocated
        via a call to kpmDeleteHandle() when no longer needed.
    @seealso kpmCreateHandle kpmCreateHandle
    @seealso kpmDeleteHandle kpmDeleteHandle
 */
KpmHandle  *kpmCreateHandleHomography( int xsize, int ysize, AR_PIXEL_FORMAT pixFormat );
#define     kpmCreatHandleHomography kpmCreateHandleHomography

/*!
    @function
    @abstract Finalise and dispose of structures for KPM tracking.
    @discussion
        Once KPM processing has completed, this routine should be called to
        dispose of memory allocated.
    @param kpmHandle Pointer to a location which holds a pointer to a KpmHandle structure.
        On return, the location pointed to will be set to NULL.
    @result 0 if successful, or value &lt;0 in case of error.
    @seealso kpmCreateHandle kpmCreateHandle
    @seealso kpmCreateHandleHomography kpmCreateHandleHomography
 */
int         kpmDeleteHandle( KpmHandle **kpmHandle );

int         kpmSetProcMode( KpmHandle *kpmHandle, int  procMode );
int         kpmGetProcMode( KpmHandle *kpmHandle, int *procMode );
int         kpmSetDetectedFeatureMax( KpmHandle *kpmHandle, int  detectedMaxFeature );
int         kpmGetDetectedFeatureMax( KpmHandle *kpmHandle, int *detectedMaxFeature );
int         kpmSetSurfThreadNum( KpmHandle *kpmHandle, int surfThreadNum );

/*!
    @function
    @abstract Load a reference data set into the key point matcher for tracking.
    @discussion
        This function takes a reference data set already in memory and makes it the current
        dataset for key point matching.
    @param kpmHandle Handle to the current KPM tracker instance, as generated by kpmCreateHandle or kpmCreateHandleHomography.
    @param refDataSet The reference data set to load into the KPM handle. The operation takes
        a copy of the data required from this dataset, thus unless the need for a further load
        at a later time is required, the dataset can be disposed of by calling kpmDeleteRefDataSet
        after this operation succeeds.
    @result 0 if successful, or value &lt;0 in case of error.
    @seealso kpmCreateHandle kpmCreateHandle
    @seealso kpmCreateHandleHomography kpmCreateHandleHomography
    @seealso kpmDeleteRefDataSet kpmDeleteRefDataSet
 */
int         kpmSetRefDataSet( KpmHandle *kpmHandle, KpmRefDataSet *refDataSet );

/*!
    @function
    @abstract
        Loads a reference data set from a file into the KPM tracker.
    @discussion
        This is a convenience method which performs a sequence of kpmLoadRefDataSet, followed
        by kpmSetRefDataSet and finally kpmDeleteRefDataSet. When tracking from a single
        reference dataset file, this is the simplest means to start.
    @param kpmHandle Handle to the current KPM tracker instance, as generated by kpmCreateHandle or kpmCreateHandleHomography.
    @param filename Path to the dataset. Either full path, or a relative path if supported by
        the operating system.
    @param ext If non-NULL, a '.' charater and this string will be appended to 'filename'.
        Often, this parameter is a pointer to the string "fset2".
    @result Returns 0 if successful, or value &lt;0 in case of error.
    @seealso kpmLoadRefDataSet kpmLoadRefDataSet
    @seealso kpmSetRefDataSet kpmSetRefDataSet
    @seealso kpmDeleteRefDataSet kpmDeleteRefDataSet
 */
int         kpmSetRefDataSetFile( KpmHandle *kpmHandle, const char *filename, const char *ext );

int         kpmSetRefDataSetFileOld( KpmHandle *kpmHandle, const char *filename, const char *ext );

/*!
    @function
    @abstract Perform key-point matching on an image.
    @discussion 
    @param kpmHandle
    @param inImage Source image containing the pixels which will be searched for features.
        Typically, this is one frame from a video stream. The dimensions and pixel format
        of this image must match the values specified at the time of creation of the KPM handle.
    @result 0 if successful, or value &lt;0 in case of error.
    @seealso kpmCreateHandle kpmCreateHandle
    @seealso kpmCreateHandleHomography kpmCreateHandleHomography
 */
int         kpmMatching( KpmHandle *kpmHandle, ARUint8 *inImage );

int         kpmSetMatchingSkipPage( KpmHandle *kpmHandle, int *skipPages, int num );
int         kpmSetMatchingSkipRegion( KpmHandle *kpmHandle, SurfSubRect *skipRegion, int regionNum);
    
int         kpmGetRefDataSet( KpmHandle *kpmHandle, KpmRefDataSet **refDataSet );
int         kpmGetInDataSet( KpmHandle *kpmHandle, KpmInputDataSet **inDataSet );
int         kpmGetMatchingResult( KpmHandle *kpmHandle, KpmMatchResult **preRANSAC, KpmMatchResult **aftRANSAC );
int         kpmGetPose( KpmHandle *kpmHandle, float  pose[3][4], int *pageNo, float  *error );
int         kpmGetResult( KpmHandle *kpmHandle, KpmResult **result, int *resultNum );


int         kpmGenRefDataSet ( ARUint8 *refImage, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, float  dpi, int procMode, int compMode, int maxFeatureNum,
                               int pageNo, int imageNo, KpmRefDataSet **refDataSet );
int         kpmAddRefDataSet ( ARUint8 *refImage, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, float  dpi, int procMode, int compMode, int maxFeatureNum,
                               int pageNo, int imageNo, KpmRefDataSet **refDataSet );

/*!
    @function
    @abstract Merge a second KPM dataset into the first, and dispose of second.
    @discussion
        This function merges two KPM datasets by adding the reference points in
        the second into the first (allocating a new set if the location pointed to
        by refDataSetPtr1 is NULL) and then deleting the second set.
    @param refDataSetPtr1 Pointer to a location which points to the first data set, or pointer
        to NULL if a new dataset is to be created. This will hold the results of the merge.
    @param refDataSetPtr2 Pointer to a location which points to the second data set. After the
        merge, the dataset pointed to will be deleted and the location pointed to set to NULL.
    @result 0 if the merge succeeded, or a value &lt; 0 in case of error.
 */
int         kpmMergeRefDataSet  ( KpmRefDataSet **refDataSetPtr1, KpmRefDataSet **refDataSetPtr2 );
#define     kpmMargeRefDataSet kpmMergeRefDataSet
    
/*!
    @function
    @abstract Dispose of a reference data set and its allocated memory.
    @discussion
        Once a data set has been loaded into a KPM handle, or is otherwise no longer required
        to be held in memory, it should be deleted (i.e. disposed) from memory by calling
        this function.
    @param refDataSetPtr Pointer to memory location which points to the dataset. On success,
        this location will be set to NULL.
    @result 0 if the delete succeeded, or a value &lt; 0 in case of error.
    @seealso kpmLoadRefDataSet kpmLoadRefDataSet
 */
int         kpmDeleteRefDataSet ( KpmRefDataSet **refDataSetPtr );

/*!
    @function
    @abstract 
    @discussion 
    @param filename
    @param ext
    @param refDataSet
    @result 
 */
int         kpmSaveRefDataSet   ( const char *filename, const char *ext, KpmRefDataSet  *refDataSet );

/*!
    @function
    @abstract Load a reference data set from the filesystem into memory.
    @discussion
        This does not set the reference data as the current tracking set. To do that, call
        kpmSetRefDataSet after this load completes. Alternately, the loaded set can be
        merged with another loaded set by calling kpmMergeRefDataSet. To dispose of the
        loaded dataset, call kpmDeleteRefDataSet.
    @param filename Path to the dataset. Either full path, or a relative path if supported by
        the operating system.
    @param ext If non-NULL, a '.' charater and this string will be appended to 'filename'.
        Often, this parameter is a pointer to the string "fset2".
    @result Returns 0 if successful, or value &lt;0 in case of error.
    @param refDataSetPtr Pointer to a location which after loading will point to the loaded
        reference data set.
    @result 0 if the load succeeded, or a value &lt; 0 in case of error.
    @seealso kpmSetRefDataSet kpmSetRefDataSet
    @seealso kpmMergeRefDataSet kpmMergeRefDataSet
    @seealso kpmDeleteRefDataSet kpmDeleteRefDataSet
 */
int         kpmLoadRefDataSet   ( const char *filename, const char *ext, KpmRefDataSet **refDataSetPtr );

int         kpmLoadRefDataSetOld( const char *filename, const char *ext, KpmRefDataSet **refDataSetPtr );

/*!
    @function
    @abstract 
    @discussion 
    @param refDataSet
    @param oldPageNo
    @param newPageNo
    @result 
 */
int         kpmChangePageNoOfRefDataSet ( KpmRefDataSet *refDataSet, int oldPageNo, int newPageNo );


ARUint8    *kpmUtilGenBWImage( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int procMode, int *newXsize, int *newYsize );
int         kpmUtilGetPose ( ARParamLT *cparamLT, KpmMatchResult *matchData, KpmRefDataSet *refDataSet, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *err );
int         kpmUtilGetPose2( ARParamLT *cparamLT, KpmMatchResult *matchData, KpmRefDataSet *refDataSet, int *redDataIndex, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *error );
int         kpmUtilGetPoseHomography( KpmMatchResult *matchData, KpmRefDataSet *refDataSet, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *err );
int         kpmUtilGetCorner( ARUint8 *inImagePtr, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int procMode, int maxPointNum, CornerPoints *cornerPoints );


#ifdef __cplusplus
}
#endif
#endif
