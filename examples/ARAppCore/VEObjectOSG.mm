//
//  VEObjectOSG.mm
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

#import "VEObjectOSG.h"
#import "VirtualEnvironment.h"
#import <AR/arosg.h>
#import "glStateCache.h"
#import "osgPlugins.h"
#import <Eden/EdenMath.h>
#import "ARView.h"
#import "ARViewController.h"
#import <dispatch/dispatch.h>

static AROSG *VEObjectOSG_AROSG = NULL;
static unsigned int VEObjectOSG_AROSGRefCount = 0;
static unsigned int VEObjectOSG_notificationRefCount = 0;
static GLint viewPort[4];
static GLfloat projection[16];

@implementation VEObjectOSG {
	int modelIndex;
}

+ (void)load
{
    VEObjectRegistryRegister(self, @"osg");
    VEObjectRegistryRegister(self, @"ive");
}

+ (void) draw:(NSNotification *)notification
{
    if (!VEObjectOSG_AROSG) return;
    
    // Set some state to OSG's expected values.
    glStateCacheDisableLighting();
    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(0);
    glStateCacheDisableTex2D();
    glStateCacheDisableBlend();
    glStateCacheClientActiveTexture(GL_TEXTURE0);
    glStateCacheEnableClientStateVertexArray();
    glStateCacheEnableClientStateNormalArray();
    glStateCacheEnableClientStateTexCoordArray();
    
    // Save the projection and modelview state.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    
    // Draw the whole scenegraph.
    arOSGDraw(VEObjectOSG_AROSG);
    
    // OSG modifies the viewport, so restore it.
    // Also restore projection and modelview.
    glViewport(viewPort[viewPortIndexLeft], viewPort[viewPortIndexBottom], viewPort[viewPortIndexWidth], viewPort[viewPortIndexHeight]);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    // Flush the state cache and ensure depth testing is enabled.
    glStateCacheFlush();
    glStateCacheEnableDepthTest();
}

+ (void) handleReshape:(NSNotification *)notification
{
    if (!VEObjectOSG_AROSG) return;
    ARView *arView_in = (ARView *)[notification object];
    GLint *viewPort_in = [arView_in viewPort];
    viewPort[0] = viewPort_in[0]; viewPort[1] = viewPort_in[1]; viewPort[2] = viewPort_in[2]; viewPort[3] = viewPort_in[3]; 
    arOSGHandleReshape2(VEObjectOSG_AROSG, viewPort_in[viewPortIndexLeft], viewPort_in[viewPortIndexBottom], viewPort_in[viewPortIndexWidth], viewPort_in[viewPortIndexHeight]);
    
    // Also, since at this point the OSG viewer is valid, we can set the projection now. Assuming here ARdouble is float.
    float *projection_in = [arView_in cameraLens];
    for (int i = 0; i < 16; i++) projection[i] = projection_in[i];
    
    // Also, if flipping in just one axis, change the winding so that lighting is calculated correctly.
    BOOL flipV = [arView_in contentFlipV];
    BOOL flipH = [arView_in contentFlipH];
    BOOL useClockwiseWinding = ((flipH && !flipV) || (!flipH && flipV));
    arOSGSetFrontFace(VEObjectOSG_AROSG, useClockwiseWinding);

    arOSGSetProjection(VEObjectOSG_AROSG, projection);
}

- (id) initFromFile:(NSString *)file translation:(const ARdouble [3])translation rotation:(const ARdouble [4])rotation scale:(const ARdouble [3])scale config:(char *)config
{
    if ((self = [super initFromFile:file translation:translation rotation:rotation scale:scale config:config])) {
        
        if ((arOSGGetVersion() >> 8) < 0x040501u) {
            NSLog(@"Error: arOSG library v4.5.1 or later is required.\n");
            [self release];
            return (nil);
        }

        // One-time initialisation.
        @synchronized(self) {
            if (!VEObjectOSG_AROSG) {
                VEObjectOSG_AROSG = arOSGInit();
                if (!VEObjectOSG_AROSG) {
                    NSLog(@"Error: unable to init arOSG library.\n");
                    //[self release];
                    //return (nil);
                }
            }
            VEObjectOSG_AROSGRefCount++;
        }
        
        modelIndex = arOSGLoadModel2(VEObjectOSG_AROSG, [file UTF8String], translation, rotation, scale);
        if (modelIndex < 0) {
            NSLog(@"Error: unable to load OSG model from file '%@'.\n", file);
            [self release];
            return (nil);
        }

        _drawable = TRUE;
        
    }
    return (self);
}

- (void)wasAddedToEnvironment:(VirtualEnvironment *)environment
{
    if (!VEObjectOSG_notificationRefCount) {
        [[NSNotificationCenter defaultCenter] addObserver:[self class] selector:@selector(handleReshape:) name:ARViewUpdatedViewportNotification object:environment.arViewController.glView];
        [[NSNotificationCenter defaultCenter] addObserver:[self class] selector:@selector(draw:) name:ARViewDrawPreCameraNotification object:environment.arViewController.glView];
    }
    VEObjectOSG_notificationRefCount++;
    
    [super wasAddedToEnvironment:environment];
}

- (void) willBeRemovedFromEnvironment:(VirtualEnvironment *)environment
{
    VEObjectOSG_notificationRefCount--;
    if (!VEObjectOSG_notificationRefCount) {
        [[NSNotificationCenter defaultCenter] removeObserver:[self class] name:ARViewUpdatedViewportNotification object:environment.arViewController.glView];
        [[NSNotificationCenter defaultCenter] removeObserver:[self class] name:ARViewDrawPreCameraNotification object:environment.arViewController.glView];
    }
    
    [super willBeRemovedFromEnvironment:environment];
}

-(void) dealloc
{
    arOSGUnloadModel(VEObjectOSG_AROSG, modelIndex);
    
    VEObjectOSG_AROSGRefCount--;
    if (!VEObjectOSG_AROSGRefCount) {
        arOSGFinal(VEObjectOSG_AROSG);
        VEObjectOSG_AROSG = NULL;
    }
    
    // Notifications are unregistered by VEObject.
    
    [super dealloc];
}

- (void) setPose:(ARPose)poseIn
{
    [super setPose:poseIn];
    arOSGSetModelPose(VEObjectOSG_AROSG, modelIndex, _poseInEyeSpace.T);
}

- (void) setPoseInEyeSpace:(ARPose)poseIn
{
    [super setPoseInEyeSpace:poseIn];
    arOSGSetModelPose(VEObjectOSG_AROSG, modelIndex, _poseInEyeSpace.T);
}

- (void) setPoseInEyeSpaceAfterParentUpdate
{
    [super setPoseInEyeSpaceAfterParentUpdate];
    arOSGSetModelPose(VEObjectOSG_AROSG, modelIndex, _poseInEyeSpace.T);
}

- (void) setLocalPosePositionX:(float)x Y:(float)y Z:(float)z
{
    [super setLocalPosePositionX:x Y:y Z:z];
    arOSGSetModelLocalPose(VEObjectOSG_AROSG, modelIndex, _localPose.T);
}

- (void) setLocalPoseOrientationDegrees:(float)degrees axisX:(float)x axisY:(float)y axisZ:(float)z
{
    [super setLocalPoseOrientationDegrees:degrees axisX:x axisY:y axisZ:z];
    arOSGSetModelLocalPose(VEObjectOSG_AROSG, modelIndex, _localPose.T);
}

- (void) setLocalPoseScaleX:(float)x Y:(float)y Z:(float)z
{
    [super setLocalPoseScaleX:x Y:y Z:z];
    arOSGSetModelLocalPose(VEObjectOSG_AROSG, modelIndex, _localPose.T);
}

- (void) setLocalPose:(ARPose)poseIn
{
    [super setLocalPose:poseIn];
    arOSGSetModelLocalPose(VEObjectOSG_AROSG, modelIndex, _localPose.T);
}

- (void) setVisible:(BOOL)visibleIn
{
    [super setVisible:visibleIn];
    arOSGSetModelVisibility(VEObjectOSG_AROSG, modelIndex, _visible);
}

- (void) setLit:(BOOL)lightingIn
{
    [super setLit:lightingIn];
    arOSGSetModelLighting(VEObjectOSG_AROSG, modelIndex, _lit);
}

- (BOOL) isIntersectedByRayFromPoint:(ARVec3)p1 toPoint:(ARVec3)p2
{
    int ret = arOSGGetModelIntersection(VEObjectOSG_AROSG, modelIndex, p1.v, p2.v);
    if (ret > 0) return (TRUE);
    if (ret < 0) {
        NSLog(@"Error: arOSGGetModelIntersection.\n");
    }
    return (FALSE);
}

@end
