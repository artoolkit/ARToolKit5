//
//  VirtualEnvironment.h
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

// VirtualEnvironment is an abstraction of a 3D environment,
// and is populated by VEObjects, which implement behaviour such
// as position and drawing, and a very simple object hierachy.
//

//#import <UIKit/UIKit.h> // Using precompiled header.
#import "VEObject.h"

#define VIEW_DISTANCE_MIN        5.0f          // Objects closer to the camera than this will not be displayed.
#define VIEW_DISTANCE_MAX        2000.0f        // Objects further away from the camera than this will not be displayed.


@class ARViewController;

@interface VirtualEnvironment : NSObject {
}

- (VirtualEnvironment *) initWithARViewController:(ARViewController *)vc;

- (void) addObject:(VEObject *)object;

- (void) removeObject:(VEObject *)object;

// Add objects to a VirtualEnvironment by reading object definitions
// from an objects list file.
//
// The format of this file is a simple text file. Comments may be
// included by prefixing the line with a "#" character.
// The first line contains the number of object definitions to
// read from the file. Successive object definitions should be
// separated by a blank line.
// Each object definition consists of the following:
// * A line with the path to the object's data file, relative to the
//   objects file. (This path may include spaces.)
//   Subclasses of VEObject register a set of file extensions which
//   they handle with VirtualEnvironment, and the objectDataFilePath's
//   path extension (e.g. ".obj") is matched against registered
//   extensions to determine which VEObject subclass should be used to
//   load and instantiate the object.
// * A line with the position of the object's origin, relative to the
//   parent coordinate system's origin, expressed as 3 floating point
//   numbers separated by spaces, representing the offset in x, y, and z.
//   This is the same format used by the glTranslate() function.
// * A line with the orientation of the object's coordinate system,
//   relative to the parent coordinate system, expressed as 4 floating
//   point numbers separated by spaces, representing an angle and an
//   axis of a rotation from the parent. This is the same format used
//   by the glRotate() function.
// * A line with the scale factor to apply to the object, expressed
//   as 3 floating point numbers separated by spaces, representing the
//   scale factor to apply in x, y, and z.
//   This is the same format used by the glScale() function.
// * Zero or more lines with optional tokens representing additional
//   information about the object. The following tokens are defined:
//   MARKER_NAME name: Associates this object with the ARMarker object with
//             name 'name' in the passed-in NSArray of ARMarkers.
//             Default is no association.
//   MARKER n: Associates this object with the ARMarker object with
//             index n in the passed-in NSArray of ARMarkers,
//             where n is in the range [1, number of ARMarkers].
//             Default is no association.
//   LIGHTING f: Enables or disables lighting calculations for this
//             object. Note that disabling lighting will result in the
//             object being drawn fully lit but without shading.
//             f = 0 to disable, f = 1 to enable. Default is enabled.
//   TRANSPARENT: Provides a hint that this object includes transparent
//             portions, and should be drawn alpha-blended. Default is
//             that no transparency hint is provided.
//   AUTOPARENT: A hint that this object should be added as a child
//             object of an automatically-selected parent object.
//             Typically the parent would be another object that caused
//             this object to be instantiated.
//
// Returns the number of objects actually added. A value of 0 indicates
// an error.
- (int) addObjectsFromObjectListFile:(NSString *)objectDataFilePath connectToARMarkers:(NSArray *)markers;
- (int) addObjectsFromObjectListFile:(NSString *)objectDataFilePath connectToARMarkers:(NSArray *)markers  autoParentTo:(VEObject *)autoParent;

- (void) updateWithSimulationTime:(NSTimeInterval)timeDelta;

// Weak reference to the parent view controller.
// This can be used to obtain references to other AR-related things
// relevant to a VirtualEnvironment, e.g. an ARView into which VEObjects
// may draw themselves.
@property (nonatomic, assign) IBOutlet ARViewController *arViewController;

@end

#ifdef __cplusplus
extern "C" {
#endif

// Should be called prior to main() by VEObject classes to register themselves.
// If the class is not interested in automatically handling any particular
// extension, it may pass 'nil' for parameter extension.
void VEObjectRegistryRegister(const Class type, const NSString *extension);

#ifdef __cplusplus
}
#endif
