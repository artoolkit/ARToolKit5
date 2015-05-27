//
//  VEObject.h
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

//#import <Foundation/Foundation.h> // Using precompiled header.
#import <AR/ar.h>
#import "ARMarker.h"

@class VirtualEnvironment;

@interface VEObject : NSObject {
@protected
    VirtualEnvironment *_ve; // Reference to containing environment.
    BOOL _drawable;
    BOOL _lit;
    BOOL _transparent;
    BOOL _visible;
    ARPose _pose;
    ARPose _localPose;
    ARPose _poseInEyeSpace;
}

// Designated initialiser.
// 'file' is ignored by default, but may be used by VEObject suclasses.
// 'config' is ignored by default, but may be used by VEObject suclasses.
- (id) initFromFile:(NSString *)file translation:(const ARdouble [3])translation rotation:(const ARdouble [4])rotation scale:(const ARdouble [3])scale config:(char *)config NS_DESIGNATED_INITIALIZER;

// Convenience initialisers.
- (id) initFromFile:(NSString *)file translation:(const ARdouble [3])translation rotation:(const ARdouble [4])rotation scale:(const ARdouble [3])scale;
- (id) init;

// If the object is added to or removed from a VirtualEnvironment,
// these methods will be invoked on the object by the VirtualEnvironment.
// 'environment' will be nil if not current in a VE.
- (void) wasAddedToEnvironment:(VirtualEnvironment *)environment;
- (void) willBeRemovedFromEnvironment:(VirtualEnvironment *)environment;
@property(readonly, assign) VirtualEnvironment *environment;

//
// VEObject hierachy.
//
// * Objects are not required to have a parent, but if they do, may have only one parent.
// * One object may have many children or none.
// * Parents own their children (i.e. they retain them in "add" and release them in "remove"). Children do not own their parents.
//
// Hierachy functionality:
// * Setting 'visible' on a parent will also set it on all children.
// * Adjusting a parent's pose or pose in eye coordinates will cause the pose in eye coordinates
//   of all children to change.
//

@property(readonly, nonatomic, assign) VEObject *parent;
@property(readonly, nonatomic, retain) NSArray *children;
- (void) addChild:(VEObject *)child;
- (void) removeChild:(VEObject *)child;

// These messages are sent when an object is added to or removed from the VEObject hierachy.
// If overriding these, be sure to call through to super at the end of your implementation.
- (void) willBeAddedAsChildOf:(VEObject *)parent;
- (void) wasAddedAsChildOf:(VEObject *)parent;
- (void) willBeRemovedAsChildOf:(VEObject *)parent;
- (void) wasRemovedAsChildOf:(VEObject *)parent;

//
// Properties
//

@property(readonly) NSString *type;
@property(copy) NSString *name;
@property(readonly, getter=isDrawable) BOOL drawable;   // Whether this object is drawable.

@property(getter=isLit) BOOL lit;                       // For drawable objects only, whether the object is currently drawn with lighting or without. Initial state is lit = TRUE.
@property(getter=isVisible) BOOL visible;               // For drawable objects only, whether the object is currently hidden or visible. Initial state is visible = TRUE.
@property(getter=isTransparent) BOOL transparent;       // For drawable objects only, whether the object has transparent portions. Initial state is transparent = FALSE.

// 'pose' combines position and orientation in a column-major 4x4 matrix.
// Position(x,y,z) = {pose.T[12], pose.T[13], pose.T[14]}.
@property ARPose pose;                                  // Position and orientation of object expressed in parent coordinate system. If no parent, value is same as poseInEyeSpace.
@property(readonly) ARPose poseInverse;                 // Position and orientation of parent, expressed in object coordinate system. If no parent, value is same as poseInEyeSpaceInverse.
@property ARPose poseInEyeSpace;                        // Position and orientation of object expressed in eye (i.e. viewer or camera) coordinate system.
@property(readonly) ARPose poseInEyeSpaceInverse;       // Position and orientation of eye (i.e. viewer or camera) expressed in object coordinate system.
- (void) setPoseInEyeSpaceAfterParentUpdate;            // This method is invoked by the parent when its pose changes. It should not normally be called by the user.

- (BOOL) isIntersectedByRayFromPoint:(ARVec3)p1 toPoint:(ARVec3)p2; // p1 and p2 are in eye coordinates.

// VEObjects can be made to update their internal state depending on the state
// of an ARMarker object by observing its notifications.
// The default behaviour is to respond to marker visibility and pose.
// This can be extended by implementing markerNotification: in the subclass.
// Be sure to call super as well unless you wish to drop the default behaviour.
- (void) startObservingARMarker:(ARMarker *)marker;
- (void) stopObservingARMarker:(ARMarker *)marker;
- (void) markerNotification:(NSNotification *)notification;

// 'localpose' is a relative adjustment expressed in the object's coordinate system.
// localpose may affect drawing or other operations in this coordinate system,
// but does not affect children.
// The following methods assume that position, orientation and scale factors
// are applied in the order position, orientation, scale. I.e. position and
// orientation are expressed in the coordinate system establish by 'pose' and
// finally scale is applied in the rotated coordinate system 'localPose'.
@property ARPose localPose; // Additional transform specifiying position and orientation of object in local coordinate system, column-major order.
- (void) localPosePositionX:(float *)x Y:(float *)y Z:(float *)z;
- (void) localPoseOrientationAngle:(float *)degrees axisX:(float *)x axisY:(float *)y axisZ:(float *)z;
- (void) localPoseScaleX:(float *)x Y:(float *)y Z:(float *)z;
- (void) setLocalPosePositionX:(float)x Y:(float)y Z:(float)z;
- (void) setLocalPoseOrientationDegrees:(float)degrees axisX:(float)x axisY:(float)y axisZ:(float)z;
- (void) setLocalPoseScaleX:(float)x Y:(float)y Z:(float)z;

@end

// Implementers of this protocol receive these messages when a VirtualEnvironment is being
// created or destroyed, provided they have registered with the VEObjectRegistry.
// These are a useful place for subclasses of VEObject to perform one-time initialisation/finalisation.
// Note that at the time at which these are being called, the VirtualEnvironment's arViewController
// property will be valid.
@protocol VEObjectRegistryEntryIsInterestedInVirtualEnvironmentLifespan <NSObject>
@required
+ (void) virtualEnvironmentIsBeingCreated:(VirtualEnvironment *)ve;
+ (void) virtualEnvironmentIsBeingDestroyed:(VirtualEnvironment *)ve;
@end


