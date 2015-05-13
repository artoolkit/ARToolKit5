//
//  VEObject.m
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

#import "VEObject.h"
#import "VirtualEnvironment.h"
#import <Eden/EdenMath.h>

//@interface VEObject (VEObjectPrivate)
//@end

@implementation VEObject {
    VEObject *_parent;
    NSMutableArray *_children;
    BOOL _needToCalculatePoseInverse;
    ARPose _poseInverse;
    BOOL _needToCalculatePoseInEyeSpaceInverse;
    ARPose _poseInEyeSpaceInverse;
    NSString *_name;
}

@synthesize drawable=_drawable, lit=_lit, transparent=_transparent, parent=_parent, children=_children;
@synthesize localPose=_localPose, name = _name;

+(NSString *) type
{
    return (NSStringFromClass(self));
}

- (id) initFromFile:(NSString *)file translation:(const ARdouble [3])translation rotation:(const ARdouble [4])rotation scale:(const ARdouble [3])scale config:(char *)config
{
    if ((self = [super init])) {
        _ve = nil;
        _drawable = FALSE;
        _lit = TRUE;
        _transparent = FALSE;
        _visible = TRUE;
        _parent = nil;
        _children = [[NSMutableArray alloc] init];
        _pose = _poseInverse = _poseInEyeSpaceInverse = _poseInEyeSpace = ARPoseUnity;
        _needToCalculatePoseInverse = FALSE;
        _localPose = ARPoseUnity;
        if (file) _name = [[file lastPathComponent] copy];
        else _name = nil;
    }
    return (self);
}

- (id) initFromFile:(NSString *)file translation:(const ARdouble [3])translation rotation:(const ARdouble [4])rotation scale:(const ARdouble [3])scale
{
    return ([self initFromFile:file translation:translation rotation:rotation scale:scale config:NULL]);
}

- (id) init
{
    const ARdouble translationDefault[3] = {0.0f, 0.0f, 0.0f};
    const ARdouble rotationDefault[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const ARdouble scaleDefault[3] = {1.0f, 1.0f, 1.0f};
    
    return ([self initFromFile:nil translation:translationDefault rotation:rotationDefault scale:scaleDefault]);
}

- (void) dealloc
{
    if (_name) {
        [_name release];
        _name = nil;
    }
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    // Empty array first, then release the empty array.
    for (VEObject *child in _children) [self removeChild:child];
    [_children release];

    [super dealloc];
}

- (BOOL) isIntersectedByRayFromPoint:(ARVec3)p1 toPoint:(ARVec3)p2
{
    return (FALSE);
}

- (void) addChild:(VEObject *)child
{
    if (child->_parent == self) return; // Sanity check for case where already added.
    
    [_children addObject:child]; // It might seem strange to add it first, but this prevents it being de'alloced if it is being re-parented.
    if (child->_parent) [child->_parent removeChild:child]; // The child may have only one parent.
    [child willBeAddedAsChildOf:self];
    child->_parent = self;
    child.visible = _visible;    // Make the child's visibility match ours.
    [child setPoseInEyeSpaceAfterParentUpdate];
#ifdef DEBUG
    NSLog(@"VEObject '%@' added child '%@' (%s).\n", _name, child.name, (_visible ? "VISIBLE" : "NOT VISIBLE"));
#endif
    [child wasAddedAsChildOf:self];
}

- (void) removeChild:(VEObject *)child
{
    if (child->_parent != self) return; // Sanity check.
    
    [child willBeRemovedAsChildOf:self];
    child->_parent = nil;
    [child setPoseInEyeSpaceAfterParentUpdate];
    [child wasRemovedAsChildOf:self];
    [_children removeObject:child]; // Do this last, in case it causes the object to be de-alloced.
}

- (void) wasAddedToEnvironment:(VirtualEnvironment *)environment
{
    _ve = environment;
}

- (void) willBeRemovedFromEnvironment:(VirtualEnvironment *)environment
{
    _ve = nil;
}

- (void) willBeAddedAsChildOf:(VEObject *)parent
{
    
}

- (void) wasAddedAsChildOf:(VEObject *)parent
{
    
}

- (void) willBeRemovedAsChildOf:(VEObject *)parent
{
    
}

- (void) wasRemovedAsChildOf:(VEObject *)parent
{
    
}

- (BOOL) isVisible
{
    return (_visible);
}

- (void) setVisible:(BOOL)v
{
    _visible = v;
    for (VEObject *child in _children) {
        [child setVisible:v];
    }
}

- (ARPose) pose
{
    return (_pose);
}

- (void) setPose:(ARPose)p
{
    // Copy new pose.
    _pose = p;
    _needToCalculatePoseInverse = TRUE;
    
    // Now update pose in eye space.
    if (!_parent) _poseInEyeSpace = p;
    else EdenMathMultMatrix(_poseInEyeSpace.T, p.T, _parent->_poseInEyeSpace.T);
    _needToCalculatePoseInEyeSpaceInverse = TRUE;
    
    // Update children.
    for (VEObject *child in _children) [child setPoseInEyeSpaceAfterParentUpdate];
}

- (ARPose) poseInverse
{
    if (_needToCalculatePoseInverse) {
        EdenMathInvertMatrix(_poseInverse.T, _pose.T);
        _needToCalculatePoseInverse = FALSE;
    }
    return (_poseInverse);
}

- (ARPose)poseInEyeSpace
{
    return (_poseInEyeSpace);
}

- (void) setPoseInEyeSpace:(ARPose)p
{
    // Copy new pose in eye space.
    _poseInEyeSpace = p;
    _needToCalculatePoseInEyeSpaceInverse = TRUE;
    
    // Now update pose.
    if (!_parent) _pose = p;
    else EdenMathMultMatrix(_pose.T, p.T, _parent.poseInEyeSpaceInverse.T);
    _needToCalculatePoseInverse = TRUE;

    // Update children.
    for (VEObject *child in _children) [child setPoseInEyeSpaceAfterParentUpdate];
}

- (void) setPoseInEyeSpaceAfterParentUpdate
{
    if (!_parent) _poseInEyeSpace = _pose;
    else EdenMathMultMatrix(_poseInEyeSpace.T, _pose.T, _parent->_poseInEyeSpace.T);
    _needToCalculatePoseInEyeSpaceInverse = TRUE;
    
    // Update children.
    for (VEObject *child in _children) [child setPoseInEyeSpaceAfterParentUpdate];
}

- (ARPose) poseInEyeSpaceInverse
{
    if (_needToCalculatePoseInEyeSpaceInverse) {
        EdenMathInvertMatrix(_poseInEyeSpaceInverse.T, _poseInEyeSpace.T);
        _needToCalculatePoseInEyeSpaceInverse = FALSE;
    }
    return (_poseInEyeSpaceInverse);
}

- (void) startObservingARMarker:(ARMarker *)marker
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(markerNotification:) name:nil object:marker];
}

- (void) stopObservingARMarker:(ARMarker *)marker
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:nil object:marker];
}

- (void) markerNotification:(NSNotification *)notification
{
    ARMarker *marker = [notification object];
    
    if (marker) {
        if        ([notification.name isEqualToString:ARMarkerUpdatedPoseNotification]) {
            [self setPoseInEyeSpace:[marker pose]];
        } else if ([notification.name isEqualToString:ARMarkerDisappearedNotification]) {
            [self setVisible:FALSE];
        } else if ([notification.name isEqualToString:ARMarkerAppearedNotification]) {
            [self setVisible:TRUE];
        }
    }
}

- (void) localPosePositionX:(float *)x Y:(float *)y Z:(float *)z
{
    if (x) *x = _localPose.T[13];
    if (y) *y = _localPose.T[14];
    if (z) *z = _localPose.T[15];
}

- (void) localPoseOrientationAngle:(float *)degrees axisX:(float *)x axisY:(float *)y axisZ:(float *)z
{
    float tempX = _localPose.T[6] - _localPose.T[9];
    float tempY = _localPose.T[8] - _localPose.T[2];
    float tempZ = _localPose.T[1] - _localPose.T[4];
    float r = sqrtf(tempX*tempX + tempY*tempY + tempZ*tempZ);
    if (!r) {
        if (degrees) *degrees = 0.0f;
        if (x) *x = 1.0f;
        if (y) *y = 0.0f;
        if (z) *z = 0.0f;
    } else {
        float t = _localPose.T[0] + _localPose.T[5] + _localPose.T[10];
        if (degrees) *degrees = RTOD*atan2(r, t - 1.0f);
        if (x) *x = tempX / r;
        if (y) *y = tempY / r;
        if (z) *z = tempZ / r;
    }
}

// Will give incorrect results if negative scale factors have been used, OR if skew factors
// have been manually set in localPose.
- (void) localPoseScaleX:(float *)x Y:(float *)y Z:(float *)z
{
    if (x) *x = sqrtf(_localPose.T[0]*_localPose.T[0] + _localPose.T[1]*_localPose.T[1] + _localPose.T[2]*_localPose.T[2]);
    if (y) *y = sqrtf(_localPose.T[4]*_localPose.T[4] + _localPose.T[5]*_localPose.T[5] + _localPose.T[6]*_localPose.T[9]);
    if (z) *z = sqrtf(_localPose.T[8]*_localPose.T[8] + _localPose.T[9]*_localPose.T[9] + _localPose.T[10]*_localPose.T[10]);
}

- (void) setLocalPosePositionX:(float)x Y:(float)y Z:(float)z
{
    _localPose.T[13] = x;
    _localPose.T[14] = y;
    _localPose.T[15] = z;
}

- (void) setLocalPoseOrientationDegrees:(float)degrees axisX:(float)x axisY:(float)y axisZ:(float)z
{
    float px, py, pz, sx, sy, sz;
    [self localPosePositionX:&px Y:&py Z:&pz]; // Save position.
    [self localPoseScaleX:&sx Y:&sy Z:&sz]; // Save scale.
    EdenMathRotationMatrix(_localPose.T, DTOR*degrees, x, y, z);
    [self setLocalPosePositionX:px Y:py Z:pz];
    [self setLocalPoseScaleX:sx Y:sy Z:sz];
}

- (void) setLocalPoseScaleX:(float)x Y:(float)y Z:(float)z
{
    float sx, sy, sz;
    [self localPoseScaleX:&sx Y:&sy Z:&sz]; // Get current scale.
    sx = x/sx; sy = y/sy; sz = z/sz; // Adjust for new scale.
    _localPose.T[0] *= sx;
    _localPose.T[1] *= sx;
    _localPose.T[2] *= sx;
    _localPose.T[4] *= sy;
    _localPose.T[5] *= sy;
    _localPose.T[6] *= sy;
    _localPose.T[8] *= sz;
    _localPose.T[9] *= sz;
    _localPose.T[10] *= sz;
}

@end
