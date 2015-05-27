//
//  ARView.m
//  ARApp
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

#import <QuartzCore/QuartzCore.h>
#import "ARView.h"
#import "ARViewController.h"
#include "glStateCache2.h"
#include <AR/gsub_es2.h>
#include <AR/gsub_mtx.h>

NSString *const ARViewUpdatedCameraLensNotification = @"ARViewUpdatedCameraLensNotification";
NSString *const ARViewUpdatedCameraPoseNotification = @"ARViewUpdatedCameraPoseNotification";
NSString *const ARViewUpdatedViewportNotification = @"ARViewUpdatedViewportNotification";

// Indices of GL ES program uniforms.
enum {
    UNIFORM_MODELVIEW_PROJECTION_MATRIX,
    UNIFORM_COUNT
};

// Indices of of GL ES program attributes.
enum {
    ATTRIBUTE_VERTEX,
    ATTRIBUTE_COLOUR,
    ATTRIBUTE_COUNT
};

@interface ARView (ARViewPrivate)
- (void) calculateProjection;
@end

@implementation ARView {
    
    ARViewController *arViewController;
    
    float cameraLens[16];
    float cameraPose[16];
    BOOL cameraPoseValid;
    int contentWidth;
    int contentHeight;
    BOOL contentRotate90;
    BOOL contentFlipH;
    BOOL contentFlipV;
    ARViewContentScaleMode contentScaleMode;
    ARViewContentAlignMode contentAlignMode;

    float projection[16];
    GLint viewPort[4];
    
    // Drawing-related.
    BOOL gDrawRotate;
    float gDrawRotateAngle;
    GLint uniforms[UNIFORM_COUNT];
    GLuint program;
    
    // Interaction.
    id <ARViewTouchDelegate> touchDelegate;
}

@synthesize contentWidth, contentHeight, contentAlignMode, contentScaleMode, touchDelegate, arViewController;
@synthesize gDrawRotate;

- (id) initWithFrame:(CGRect)frame pixelFormat:(NSString*)format depthFormat:(EAGLDepthFormat)depth withStencil:(BOOL)stencil preserveBackbuffer:(BOOL)retained
{
    if ((self = [super initWithFrame:frame renderingAPI:kEAGLRenderingAPIOpenGLES2 pixelFormat:format depthFormat:depth withStencil:stencil preserveBackbuffer:retained])) {
         
        // Init instance variables.
        arViewController = nil;
        
        mtxLoadIdentityf(cameraLens);
        contentRotate90 = contentFlipH = contentFlipV = NO;
        mtxLoadIdentityf(projection);
        
        contentWidth = (int)frame.size.width;
        contentHeight = (int)frame.size.height;
        contentScaleMode = ARViewContentScaleModeFill;
        contentAlignMode = ARViewContentAlignModeCenter;

        cameraPoseValid = NO;
        
        // Init gestures.
        [self setMultipleTouchEnabled:YES];
        [self setTouchDelegate:self];

        // One-time OpenGL setup goes here.
        glStateCacheFlush();
        
        BOOL ok = CHECK_GL_ERROR();
    }
    
    return (self);
    
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    
    // Calculate viewport.
    int left, bottom, w, h;
    
#ifdef DEBUG
    NSLog(@"[ARView layoutSubviews] backingWidth=%d, backingHeight=%d\n", self.backingWidth, self.backingHeight);
#endif        

    if (self.contentScaleMode == ARViewContentScaleModeStretch) {
        w = self.backingWidth;
        h = self.backingHeight;
    } else {
        int contentWidthFinalOrientation = (contentRotate90 ? contentHeight : contentWidth);
        int contentHeightFinalOrientation = (contentRotate90 ? contentWidth : contentHeight);
        if (self.contentScaleMode == ARViewContentScaleModeFit || self.contentScaleMode == ARViewContentScaleModeFill) {
            float scaleRatioWidth, scaleRatioHeight, scaleRatio;
            scaleRatioWidth = (float)self.backingWidth / (float)contentWidthFinalOrientation;
            scaleRatioHeight = (float)self.backingHeight / (float)contentHeightFinalOrientation;
            if (self.contentScaleMode == ARViewContentScaleModeFill) scaleRatio = MAX(scaleRatioHeight, scaleRatioWidth);
            else scaleRatio = MIN(scaleRatioHeight, scaleRatioWidth);
            w = (int)((float)contentWidthFinalOrientation * scaleRatio);
            h = (int)((float)contentHeightFinalOrientation * scaleRatio);
        } else {
            w = contentWidthFinalOrientation;
            h = contentHeightFinalOrientation;
        }
    }
    
    if (self.contentAlignMode == ARViewContentAlignModeTopLeft
        || self.contentAlignMode == ARViewContentAlignModeLeft
        || self.contentAlignMode == ARViewContentAlignModeBottomLeft) left = 0;
    else if (self.contentAlignMode == ARViewContentAlignModeTopRight
             || self.contentAlignMode == ARViewContentAlignModeRight
             || self.contentAlignMode == ARViewContentAlignModeBottomRight) left = self.backingWidth - w;
    else left = (self.backingWidth - w) / 2;
        
    if (self.contentAlignMode == ARViewContentAlignModeBottomLeft
        || self.contentAlignMode == ARViewContentAlignModeBottom
        || self.contentAlignMode == ARViewContentAlignModeBottomRight) bottom = 0;
    else if (self.contentAlignMode == ARViewContentAlignModeTopLeft
             || self.contentAlignMode == ARViewContentAlignModeTop
             || self.contentAlignMode == ARViewContentAlignModeTopRight) bottom = self.backingHeight - h;
    else bottom = (self.backingHeight - h) / 2;

    glViewport(left, bottom, w, h);
    
    viewPort[viewPortIndexLeft] = left;
    viewPort[viewPortIndexBottom] = bottom;
    viewPort[viewPortIndexWidth] = w;
    viewPort[viewPortIndexHeight] = h;
    [[NSNotificationCenter defaultCenter] postNotificationName:ARViewUpdatedViewportNotification object:self];
#ifdef DEBUG
    NSLog(@"[ARView layoutSubviews] viewport left=%d, bottom=%d, width=%d, height=%d\n", left, bottom, w, h);
#endif
}

- (GLint *)viewPort
{
    return (viewPort);
}

- (void)setCameraLens:(float *)lens
{
    if (lens) {
        mtxLoadMatrixf(cameraLens, lens);
        [self calculateProjection];
    }
}

- (float *)cameraLens
{
    return (projection);
}

- (void) setContentRotate90:(BOOL)contentRotate90_in
{
    contentRotate90 = contentRotate90_in;
    [self calculateProjection];
}

- (BOOL) contentRotate90
{
    return (contentRotate90);
}

- (void) setContentFlipH:(BOOL)contentFlipH_in
{
    contentFlipH = contentFlipH_in;
    [self calculateProjection];
}

- (BOOL) contentFlipH
{
    return (contentFlipH);
}

- (void) setContentFlipV:(BOOL)contentFlipV_in
{
    contentFlipV = contentFlipV_in;
    [self calculateProjection];
}

- (BOOL) contentFlipV
{
    return (contentFlipV);
}

- (void) calculateProjection
{
    float const ir90[16] = {0.0f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f};
    
    if (contentRotate90) mtxLoadMatrixf(projection, ir90);
    else mtxLoadIdentityf(projection);
    if (contentFlipH || contentFlipV) mtxScalef(projection, (contentFlipH ? -1.0f : 1.0f), (contentFlipV ? -1.0f : 1.0f), 1.0f);
    mtxMultMatrixf(projection, cameraLens);
    
    [[NSNotificationCenter defaultCenter] postNotificationName:ARViewUpdatedCameraLensNotification object:self];
}

- (void)setCameraPose:(float *)pose
{
    if (pose) {
        int i;
        for (i = 0; i < 16; i++) cameraPose[i] = pose[i];
        cameraPoseValid = TRUE;
        [[NSNotificationCenter defaultCenter] postNotificationName:ARViewUpdatedCameraPoseNotification object:self];
    } else {
        cameraPoseValid = FALSE;
    }
}

- (float *)cameraPose
{
    if (cameraPoseValid) return (cameraPose);
    else return (NULL);
}

// Something to look at, draw a rotating colour cube.
- (void) drawCube:(float *)viewProjectionMatrix
{
    // Colour cube data.
    int i;
    const GLfloat cube_vertices [8][3] = {
        /* +z */ {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        /* -z */ {0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f} };
    const GLubyte cube_vertex_colors [8][4] = {
        {255, 255, 255, 255}, {255, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 255, 255},
        {255, 0, 255, 255}, {255, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 255, 255} };
    const GLubyte cube_vertex_colors_black [8][4] = {
        {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255},
        {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 255} };
    const GLushort cube_faces [6][4] = { /* ccw-winding */
        /* +z */ {3, 2, 1, 0}, /* -y */ {2, 3, 7, 6}, /* +y */ {0, 1, 5, 4},
        /* -x */ {3, 0, 4, 7}, /* +x */ {1, 2, 6, 5}, /* -z */ {4, 5, 6, 7} };
    float modelViewProjection[16];
    
    mtxLoadMatrixf(modelViewProjection, viewProjectionMatrix);
    mtxRotatef(modelViewProjection, gDrawRotateAngle, 0.0f, 0.0f, 1.0f); // Rotate about z axis.
    mtxScalef(modelViewProjection, 20.0f, 20.0f, 20.0f);
    mtxTranslatef(modelViewProjection, 0.0f, 0.0f, 0.5f); // Place base of cube on marker surface.
    glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEW_PROJECTION_MATRIX], 1, GL_FALSE, modelViewProjection);

 	glVertexAttribPointer(ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, cube_vertices);
	glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
	glVertexAttribPointer(ATTRIBUTE_COLOUR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, cube_vertex_colors);
    glEnableVertexAttribArray(ATTRIBUTE_COLOUR);

#ifdef DEBUG
	if (!arglGLValidateProgram(program)) {
		ARLOGe("[ARView drawView:] Error: shader program %d validation failed.\n", program);
		return;
	}
#endif
    
    for (i = 0; i < 6; i++) {
        glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, &(cube_faces[i][0]));
    }
	glVertexAttribPointer(ATTRIBUTE_COLOUR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, cube_vertex_colors_black);
    glEnableVertexAttribArray(ATTRIBUTE_COLOUR);
    for (i = 0; i < 6; i++) {
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, &(cube_faces[i][0]));
    }
}

- (void)updateWithTimeDelta:(NSTimeInterval)timeDelta
{
    if (gDrawRotate) {
        gDrawRotateAngle += (float)timeDelta * 45.0f; // Rotate cube at 45 degrees per second.
        if (gDrawRotateAngle > 360.0f) gDrawRotateAngle -= 360.0f;
    }
}

- (void) drawView:(id)sender
{
    float const ir90[16] = {0.0f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f};
	float width, height;
    float viewProjection[16];
    
    [self clearBuffers];

    arglDispImage(arViewController.arglContextSettings);
    
    if (!program) {
        GLuint vertShader = 0, fragShader = 0;
        // A simple shader pair which accepts just a vertex position and colour, no lighting.
        const char vertShaderString[] =
            "attribute vec4 position;\n"
            "attribute vec4 colour;\n"
            "uniform mat4 modelViewProjectionMatrix;\n"
            "varying vec4 colourVarying;\n"
            "void main()\n"
            "{\n"
                "gl_Position = modelViewProjectionMatrix * position;\n"
                "colourVarying = colour;\n"
            "}\n";
        const char fragShaderString[] =
            "#ifdef GL_ES\n"
            "precision mediump float;\n"
            "#endif\n"
            "varying vec4 colourVarying;\n"
            "void main()\n"
            "{\n"
                "gl_FragColor = colourVarying;\n"
            "}\n";
        
        if (program) arglGLDestroyShaders(0, 0, program);
        program = glCreateProgram();
        if (!program) {
            ARLOGe("drawCube: Error creating shader program.\n");
            return;
        }
        
        if (!arglGLCompileShaderFromString(&vertShader, GL_VERTEX_SHADER, vertShaderString)) {
            ARLOGe("drawCube: Error compiling vertex shader.\n");
            arglGLDestroyShaders(vertShader, fragShader, program);
            program = 0;
            return;
        }
        if (!arglGLCompileShaderFromString(&fragShader, GL_FRAGMENT_SHADER, fragShaderString)) {
            ARLOGe("drawCube: Error compiling fragment shader.\n");
            arglGLDestroyShaders(vertShader, fragShader, program);
            program = 0;
            return;
        }
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        
        glBindAttribLocation(program, ATTRIBUTE_VERTEX, "position");
        glBindAttribLocation(program, ATTRIBUTE_COLOUR, "colour");
        if (!arglGLLinkProgram(program)) {
            ARLOGe("drawCube: Error linking shader program.\n");
            arglGLDestroyShaders(vertShader, fragShader, program);
            program = 0;
            return;
        }
        arglGLDestroyShaders(vertShader, fragShader, 0); // After linking, shader objects can be deleted.
        
        // Retrieve linked uniform locations.
        uniforms[UNIFORM_MODELVIEW_PROJECTION_MATRIX] = glGetUniformLocation(program, "modelViewProjectionMatrix");        
    }
    glUseProgram(program);

    // Set up 3D mode.
    mtxLoadMatrixf(viewProjection, projection);
    glStateCacheEnableDepthTest();
    
    // Set any initial per-frame GL state you require here.
    // --->

    // Lighting and geometry that moves with the camera should be added here.
    // (I.e. should be specified before camera pose transform.)
    // --->
    
    if (cameraPoseValid) {
        
        mtxMultMatrixf(viewProjection, cameraPose);
        
        // All lighting and geometry to be drawn in world coordinates goes here.
        // --->
        [self drawCube:viewProjection];
    }
    
    // If you added external OpenGL code above, and that code doesn't use the glStateCache routines,
    // then uncomment the line below.
    //glStateCacheFlush();
    
    // Set up 2D mode.
    if (contentRotate90) mtxLoadMatrixf(viewProjection, ir90);
    else mtxLoadIdentityf(viewProjection);
	width = (float)viewPort[(contentRotate90 ? viewPortIndexHeight : viewPortIndexWidth)];
	height = (float)viewPort[(contentRotate90 ? viewPortIndexWidth : viewPortIndexHeight)];
	mtxOrthof(viewProjection, 0.0f, width, 0.0f, height, -1.0f, 1.0f);
    glStateCacheDisableDepthTest();
    
    // Add your own 2D overlays here.
    // --->
    
    // If you added external OpenGL code above, and that code doesn't use the glStateCache routines,
    // then uncomment the line below.
    //glStateCacheFlush();

#ifdef DEBUG
    // Example of 2D drawing. It just draws a white border line.
    const GLfloat square_vertices [4][3] = { {0.5f, 0.5f, 0.0f}, {0.5f, height - 0.5f, 0.0f}, {width - 0.5f, height - 0.5f, 0.0f}, {width - 0.5f, 0.5f, 0.0f} };
    const GLubyte square_vertex_colors_white [4][4] = {
        {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}, {255, 255, 255, 255}};
    
    glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEW_PROJECTION_MATRIX], 1, GL_FALSE, viewProjection);

 	glVertexAttribPointer(ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, square_vertices);
	glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
	glVertexAttribPointer(ATTRIBUTE_COLOUR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, square_vertex_colors_white);
    glEnableVertexAttribArray(ATTRIBUTE_COLOUR);

    if (!arglGLValidateProgram(program)) {
        ARLOGe("[ARView drawView:] Error: shader program %d validation failed.\n", program);
        return;
    }
    
    glDrawArrays(GL_LINE_LOOP, 0, 4);
#endif
    
#ifdef DEBUG
    CHECK_GL_ERROR();
#endif
        
    [self swapBuffers];
}

- (void) dealloc
{
    glUseProgram(0);
    arglGLDestroyShaders(0, 0, program);
    program = 0;
    [super dealloc];
}

// Handles the start of a touch
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray*        array = [touches allObjects];
    UITouch*        touch;
    NSUInteger        i;
    CGPoint            location;
    NSUInteger      numTaps;
    
#ifdef DEBUG
    //NSLog(@"[EAGLView touchesBegan].\n");
#endif
    
    for (i = 0; i < [array count]; ++i) {
        touch = [array objectAtIndex:i];
        if (touch.phase == UITouchPhaseBegan) {
            location = [touch locationInView:self];
            numTaps = [touch tapCount];
            if (touchDelegate) {
                if ([touchDelegate respondsToSelector:@selector(handleTouchAtLocation:tapCount:)]) {
                    [touchDelegate handleTouchAtLocation:location tapCount:numTaps];
                }    
            }
        } // phase match.
    } // touches.
}

// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray*        array = [touches allObjects];
    UITouch*        touch;
    NSUInteger        i;
    
#ifdef DEBUG
    //NSLog(@"[EAGLView touchesMoved].\n");
#endif
    
    for (i = 0; i < [array count]; ++i) {
        touch = [array objectAtIndex:i];
        if (touch.phase == UITouchPhaseMoved) {
            // Can do something appropriate for a moving touch here.
         } // phase match.
    } // touches.
}

// Handles the end of a touch event.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray*        array = [touches allObjects];
    UITouch*        touch;
    NSUInteger        i;
    
#ifdef DEBUG
    //NSLog(@"[EAGLView touchesEnded].\n");
#endif
    
    for (i = 0; i < [array count]; ++i) {
        touch = [array objectAtIndex:i];
        if (touch.phase == UITouchPhaseEnded) {
            // Can do something appropriate for end of touch here.
        } // phase match.
    } // touches.
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray*        array = [touches allObjects];
    UITouch*        touch;
    NSUInteger        i;
    
#ifdef DEBUG
    //NSLog(@"[EAGLView touchesCancelled].\n");
#endif
    
    for (i = 0; i < [array count]; ++i) {
        touch = [array objectAtIndex:i];
        if (touch.phase == UITouchPhaseCancelled) {
               // Can do something appropriate for cancellation of a touch (e.g. by a system event) here.
        } // phase match.
    } // touches.
}

- (void) handleTouchAtLocation:(CGPoint)location tapCount:(NSUInteger)tapCount
{
    gDrawRotate = !gDrawRotate;
}

@end
