//
//  VEObjectMovie.m
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


#import "VEObjectMovie.h"
#import "VirtualEnvironment.h"
#import "ARView.h"
#import "ARViewController.h"
#import <Eden/EdenMath.h>
#import <AR/gsub_es.h>

@interface VEObjectMovie (VEObjectMoviePrivate)
-(void) applicationWillResignActive:(NSNotification *)notification;
-(void) applicationDidBecomeActive:(NSNotification *)notification; 
@end

@implementation VEObjectMovie {
    NSURL *url;
	MovieVideo *movieVideo;
    ARGL_CONTEXT_SETTINGS_REF gMovieArglSettings;
    BOOL _needToCheckPadding;
    BOOL _restartNeeded;
    ARdouble _scale[3];
    float _transform[16];
    float _disappearLatency;
    NSTimer *deferredVisibilityChangeTimer;
}

+ (void)load
{
    VEObjectRegistryRegister(self, @"mov");
    VEObjectRegistryRegister(self, @"mp4");
    VEObjectRegistryRegister(self, @"m4v");
}

- (id) initFromFile:(NSString *)pathnameOrURL translation:(const ARdouble [3])translation rotation:(const ARdouble [4])rotation scale:(const ARdouble [3])scale config:(char *)config
{
	char configDefault[] = "-pause -loop";
	
    if ((self = [super initFromFile:pathnameOrURL translation:translation rotation:rotation scale:scale config:config])) {
        
        // Init instance variables.
        _restartNeeded = FALSE;
        
        // Load and start movie.
		//url = [NSURL URLWithString:pathnameOrURL];
        //url = [[NSBundle mainBundle] URLForResource:pathnameOrURL withExtension:nil];
        url = [NSURL fileURLWithPath:pathnameOrURL isDirectory:FALSE];
		if (!url) {
			NSLog(@"Unable to locate requested movie resource %@.\n", pathnameOrURL);
            [self release];
            return (nil);
		}
        movieVideo = [[MovieVideo alloc] initWithURL:url config:(config ? config : configDefault)];
		if (!movieVideo || ![movieVideo start]) {
			NSLog(@"Unable to start movie %@.\n", pathnameOrURL);
            [self release];
            return (nil);
        }
        
        _drawable = TRUE;
        
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
        
        // Assemble positioning. Scale is dependent on movie frame size, which isn't known yet, so save that for later.
        float tempMtx[16];
        EdenMathIdentityMatrix(_transform);
        if (translation) {
            EdenMathTranslateMatrix(tempMtx, _transform, (float)translation[0], (float)translation[1], (float)translation[2]);
            memcpy(_transform, tempMtx, sizeof(float)*16);
        }
        if (rotation) {
            EdenMathRotateMatrix(tempMtx, _transform, DTOR*(float)rotation[0], (float)rotation[1], (float)rotation[2], (float)rotation[3]);
            memcpy(_transform, tempMtx, sizeof(float)*16);
        }
        if (scale) {
            _scale[0] = scale[0];
            _scale[1] = scale[1];
            _scale[2] = scale[2];
        } else {
            _scale[0] = 1.0f;
            _scale[1] = 1.0f;
            _scale[2] = 1.0f;
        }
        
        _disappearLatency = 1.0f;
        deferredVisibilityChangeTimer = nil;
    }
    return (self);
}

- (void) wasAddedToEnvironment:(VirtualEnvironment *)environment
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(draw:) name:ARViewDrawPreCameraNotification object:environment.arViewController.glView];
    
    [super wasAddedToEnvironment:environment];
}

- (void) willBeRemovedFromEnvironment:(VirtualEnvironment *)environment
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:ARViewDrawPreCameraNotification object:environment.arViewController.glView];
    
    [super willBeRemovedFromEnvironment:environment];
}


-(void) draw:(NSNotification *)notification
{
    if (!gMovieArglSettings) {
        if (!movieVideo.loaded) {
#ifdef DEBUG
            NSLog(@"VEObjectMovie: movie not yet loaded.\n");
#endif
            return;
        }
        
        // Size of movie frame is now known. Adjust the scalefactor.
        float tempMtx[16];
        float scalef = 80.0f/movieVideo.contentWidth; // By default, scale of 1.0 creates a movie width of 80.0mm.
        EdenMathScaleMatrix(tempMtx, _transform, scalef*(float)_scale[0], scalef*(float)_scale[1], scalef*(float)_scale[2]);
        memcpy(_transform, tempMtx, sizeof(float)*16);

        // For convenience, we will use gsub_lite to draw the actual pixels. Set it up now.
        ARParam movieCparam;
        arParamClear(&movieCparam, (int)movieVideo.contentWidth, (int)movieVideo.contentHeight, AR_DIST_FUNCTION_VERSION_DEFAULT);
        gMovieArglSettings = arglSetupForCurrentContext(&movieCparam, movieVideo.ARPixelFormat);
        if (!gMovieArglSettings) {
            NSLog(@"Error initing ARGL for movie playback.\n");
            return;
        }
        arglDistortionCompensationSet(gMovieArglSettings, 0);
        _needToCheckPadding = TRUE; // Check whether the movie is returning padded buffers when we get the first frame.
    }
    
    if (self.isVisible) {
        
        unsigned char *data = [movieVideo getFrame];
        if (data) {
            if (_needToCheckPadding) {
                // Check for MovieVideo returning padded buffers, and if so, pad our buffer too.
                int pixelSize, width;
                arglPixelFormatGet(gMovieArglSettings, NULL, &pixelSize);
                arglPixelBufferSizeGet(gMovieArglSettings, &width, NULL);
                if (movieVideo.bufRowBytes != width*pixelSize) {
                    if (!arglPixelBufferSizeSet(gMovieArglSettings, (int)movieVideo.bufRowBytes/pixelSize, (int)movieVideo.bufHeight)) {
                        NSLog(@"Error resizing movie texture.\n");
                        arglCleanup(gMovieArglSettings);
                        return;
                    }
                }
                _needToCheckPadding = FALSE;
            }
            arglPixelBufferDataUpload(gMovieArglSettings, data);
        }
        
        glPushMatrix();
        glMultMatrixf(_poseInEyeSpace.T);
        glMultMatrixf(_localPose.T);
        glMultMatrixf(_transform);
        glStateCacheTexEnvMode(GL_REPLACE);
        glStateCacheDisableLighting();
        if (movieVideo.transparent) {
            glStateCacheBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glStateCacheEnableBlend();
        }
        arglDispImageStateful(gMovieArglSettings); // Show the movie frame.
        glPopMatrix();
    }
}

// Override marker appearing/disappearing default behaviour.
- (void) markerNotification:(NSNotification *)notification
{
    ARMarker *marker = [notification object];
    
    if (marker) {
        if        ([notification.name isEqualToString:ARMarkerDisappearedNotification]) {
            if (_disappearLatency) {
                // Don't change visibility immediately, but instead schedule it to occur in 2 seconds time.
                // If, during that time, the marker reappears, we can cancel the timer.
                // We will have the timer directly call setVisible:FALSE, so create an invocation which the
                // timer will use.
                NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:@selector(setVisible:)]];
                [invocation setTarget:self];
                [invocation setSelector:@selector(setVisible:)];
                BOOL arg = FALSE;
                [invocation setArgument:&arg atIndex:2]; // Index 0 is self, index 1 is _cmd.
                
                deferredVisibilityChangeTimer = [[NSTimer scheduledTimerWithTimeInterval:(double)_disappearLatency invocation:invocation repeats:NO] retain];
            } else {
                [self setVisible:FALSE];
            }
        } else if ([notification.name isEqualToString:ARMarkerAppearedNotification]) {
            if (deferredVisibilityChangeTimer) { // If the movie is scheduled to be hidden, cancel that.
                [deferredVisibilityChangeTimer invalidate];
                [deferredVisibilityChangeTimer release];
                deferredVisibilityChangeTimer = nil;
            }
            [self setVisible:TRUE];
        } else {
            [super markerNotification:notification];
        }
    }
}

- (void) setVisible:(BOOL)visibleIn
{
    if (deferredVisibilityChangeTimer) { // Clean up our reference to timer that has fired.
        [deferredVisibilityChangeTimer invalidate]; // This message will be redundant if the timer has already fired, but setVisible can also be called by user, in which case we should cancel the timer.
        [deferredVisibilityChangeTimer release];
        deferredVisibilityChangeTimer = nil;
    }
    if (visibleIn != self.isVisible) {
        if (visibleIn) [movieVideo setPaused:FALSE];
        else [movieVideo setPaused:TRUE];
    }
    [super setVisible:visibleIn];
}

- (BOOL) isIntersectedByRayFromPoint:(ARVec3)p1 toPoint:(ARVec3)p2
{
    // TODO:
    return (FALSE);
}

- (void) setPaused:(BOOL)paused
{
    if (paused) [movieVideo setPaused:TRUE];
    else [movieVideo setPaused:FALSE];
}

- (BOOL) isPaused
{
    return (movieVideo.paused);
}

-(void) applicationWillResignActive:(NSNotification *)notification
{
#ifdef DEBUG
    NSLog(@"[VEObjectMovie applicationWillResignActive:]\n");
#endif
    [movieVideo stop];
    _restartNeeded = TRUE;
}

-(void) applicationDidBecomeActive:(NSNotification *)notification
{
#ifdef DEBUG
    NSLog(@"[VEObjectMovie applicationDidBecomeActive:]\n");
#endif
    if (_restartNeeded) {
        [movieVideo start];
        _restartNeeded = FALSE;
    }
}

-(void) dealloc
{
    if (deferredVisibilityChangeTimer) { // If visibility is still scheduled to change, clean it up.
        [deferredVisibilityChangeTimer invalidate];
        [deferredVisibilityChangeTimer release];
    }

    [movieVideo stop];
    [movieVideo release];
    
    // Notifications are unregistered by VEObject.
    
    [super dealloc];
}

@end
