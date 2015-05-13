//
//  ARMarker.h
//  ARToolKit for iOS
//
//  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
//  LLC ("Daqri") in consideration of your agreement to the following
//  terms, and your use, installation, modification or redistribution of
//  this Daqri software constitutes acceptance of these terms.  If you do
//  not agree with these terms, please do not use, install, modify or
//  redistribute this Daqri software.
//
//  In consideration of your agreement to abide by the following terms, and
//  subject to these terms, Daqri grants you a personal, non-exclusive
//  license, under Daqri's copyrights in this original Daqri software (the
//  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
//  Software, with or without modifications, in source and/or binary forms;
//  provided that if you redistribute the Daqri Software in its entirety and
//  without modifications, you must retain this notice and the following
//  text and disclaimers in all such redistributions of the Daqri Software.
//  Neither the name, trademarks, service marks or logos of Daqri LLC may
//  be used to endorse or promote products derived from the Daqri Software
//  without specific prior written permission from Daqri.  Except as
//  expressly stated in this notice, no other rights or licenses, express or
//  implied, are granted by Daqri herein, including but not limited to any
//  patent rights that may be infringed by your derivative works or by other
//  works in which the Daqri Software may be incorporated.
//
//  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
//  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
//  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
//  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
//
//  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
//  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
//  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
//  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
//  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//
//  Copyright 2015 Daqri LLC. All Rights Reserved.
//  Copyright 2010-2015 ARToolworks, Inc. All rights reserved.
//
//  Author(s): Philip Lamb
//

//
// ARMarker is the base class for all types of ARToolKit markers.
// It encapsulates the operation of a single ARToolKit marker,
// providing methods for initialising and finalising the marker,
// as well as updating the marker's visibility and pose.
// It provides a set of "hooks" (via NSNotification) via which other
// objects can receive updates to the marker state.
// Additionally, it provides pose estimate filtering.
//

//#import <UIKit/UIKit.h> // Using precompiled header.
#import <AR/ar.h>
#import <AR/arFilterTransMat.h>

// The macro "WITH_NFT" is normally defined in build settings for projects
// that support NFT.
//#define WITH_NFT

@class ARViewController;

// Types.

typedef struct {
    ARdouble v[3];
} ARVec3;

typedef struct {
    ARdouble T[16]; // Position and orientation, column-major order. (position(x,y,z) = {T[12], T[13], T[14]}
} ARPose;

extern const ARPose ARPoseUnity;

// Notifications.
extern NSString *const ARMarkerCreatedNotification;     // Sent when marker is first instantiated. Note that at the time of notification, the marker object may not be completely initialized, and no methods should be called or properties read from the marker object itself.
extern NSString *const ARMarkerAppearedNotification;    // Sent when marker.pose becomes valid.
extern NSString *const ARMarkerDisappearedNotification; // Sent when marker.pose becomes invalid.
extern NSString *const ARMarkerUpdatedPoseNotification; // Sent when marker.pose changes to a new valid value.
extern NSString *const ARMarkerDestroyedNotification;   // Sent before marker is deallocated. Note that at the time of notification, the marker object may no longer be functional, and no methods should be called or properties read from the marker object itself.

@interface ARMarker : NSObject {
@protected
    NSString  *name;
    BOOL       valid;
    BOOL       validPrev;
    ARdouble   trans[3][4];
    ARPose     pose;
    ARPose     poseInverse;
    ARdouble   marker_width;
    ARdouble   marker_height;
}

//  Load a list of markers from a text file, along with associated parameters.
//  'arPattHandle': Pointer to an ARPattHandle which will store the patterns
//  for template-based markers specified in the set. If not accepting
//  template-based markers (i.e. only barcode or NFT markers), pass NULL.

//
// The format of this file is a simple text file. Comments may be
// included by prefixing the line with a "#" character.
// The first line contains the number of marker definitions to
// read from the file. Successive marker definitions should be
// separated by a blank line.
// Each entry consists of:
// * A line with the marker name, ID, or data file. This is interpreted
//   depending on the marker type.
//   * For SINGLE markers, this is EITHER the pathname (relative to
//     this file) of the pattern file defining the marker template,
//     OR an integer number specifying the barcode ID of the marker.
//   * For MULTI markers, this is the pathname (relative to
//     this file) of the multi-marker definition file.
//   * For NFT markers, this is the pathname (relative to
//     this file) of the imageset file associated with the NFT data,
//     WITHOUT its .iset suffix.
// * A line with the marker type. The following types are defined:
//     SINGLE
//     MULTI
//     NFT
//   Note that not all marker types will necessarily be supported by all
//   applications.
// * For single markers, an extra line with the width of the marker in
//   ARToolKit coordinates (usually millimetres).
// * Additional keywords may be specified on lines following the main marker
//   data, as follows:
//     FILTER [x]   Enable pose estimate filtering for the preceding marker
//                  x (optional) specifies the cutoff frequency. Default
//                  value is AR_FILTER_TRANS_MAT_CUTOFF_FREQ_DEFAULT, which
//                  at time of writing, equals 5.0.

// Variant if using both square and NFT detection.
+ (NSMutableArray *)newMarkersFromConfigDataFile:(NSString *)markersConfigDataFilePath arPattHandle:(ARPattHandle *)arPattHandle_in arPatternDetectionMode:(int *)patternDetectionMode_out;

// Find a marker by name from a set of markers.
+ (ARMarker *)findMarkerWithName:(NSString *)name inMarkers:(NSArray *)markers;

- (id) init;
- (void) update;                             // Subclasses must caller super.

@property(atomic, copy) NSString *name;      // A name by which this marker can be searched for or referred to in configuration files. A value of nil is valid, but precludes the marker from being able to be searched for by name.
@property(readonly, getter=isValid) BOOL valid; // Whether pose is valid. Generally, this is the same as whether the marker is visible, but could also apply when the marker is not visible but still valid (e.g. position is estimated).
@property(readonly) ARPose pose;             // Marker transform, scaled, as 4x4 column-major HCT matrix.
@property(readonly) ARPose poseInverse;      // Inverse of marker transform, scaled, as 4x4 column-major HCT matrix.
@property(readonly) ARdouble marker_width;   // Marker width, in calibrated ARToolKit units (usually millimetres).
@property(readonly) ARdouble marker_height;  // Marker height, in calibrated ARToolKit units (usually millimetres).
@property ARdouble positionScalefactor;      // Scalefactor between ARToolKit units (usually millimetres) and returned units in position portion of pose. Defaults to 1.0f.
@property(getter=isFiltered) BOOL filtered;
@property ARdouble filterCutoffFrequency;
@property ARdouble filterSampleRate;

@end
