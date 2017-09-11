//
//  ARViewController.m
//  ARAppCameraTest
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

#import "ARViewController.h"
#import "ARView.h"
#import "CameraFocusView.h"
#import <AR/gsub_es2.h>
#import <AR/video.h>
#import <ARUtil/time.h>
#import <AudioToolbox/AudioToolbox.h> // SystemSoundID, AudioServicesCreateSystemSoundID()


const NSString *ARResolutionPresets[] = {
    @"-preset=high",
    @"-preset=medium",
    @"-preset=low",
    @"-preset=480p",
    @"-preset=720p",
    @"-preset=cif",
    @"-preset=1080p"
};
#define ARResolutionPresetsLength (sizeof(ARResolutionPresets)/sizeof(ARResolutionPresets[0]))
const NSString *ARCameraPositionPresets[] = {
    @"-position=back",
    @"-position=front"
};
#define ARCameraPositionPresetsLength (sizeof(ARCameraPositionPresets)/sizeof(ARCameraPositionPresets[0]))

//
// ARViewController
//


@implementation ARViewController {
    
    BOOL            running;
    NSInteger       runLoopInterval;
    NSTimeInterval  runLoopTimePrevious;
    BOOL            videoPaused;
    BOOL            videoAsync;
    CADisplayLink  *runLoopDisplayLink; // For non-async video.
    
    // Video acquisition
    AR2VideoParamT *gVid;
    int             ARResolutionPreset;
    int             ARCameraPositionPreset;
    long            gCallCountMarkerDetect;
    
    // Drawing.
    ARView         *glView;
    ARGL_CONTEXT_SETTINGS_REF arglContextSettings;
    CameraFocusView *focusView;
}

@synthesize glView;
@synthesize arglContextSettings;
@synthesize running, runLoopInterval;
@synthesize overlays;

/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
        // Custom initialization
    }
    return self;
}
*/

- (void)loadView
{
    self.wantsFullScreenLayout = YES;
    
    // This will be overlaid with the actual AR view.
    NSString *irisImage = nil;
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
        irisImage = @"Iris-iPad.png";
    }  else { // UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone
        CGSize result = [[UIScreen mainScreen] bounds].size;
        if (result.height == 568) {
            irisImage = @"Iris-568h.png"; // iPhone 5, iPod touch 5th Gen, etc.
        } else { // result.height == 480
            irisImage = @"Iris.png";
        }
    }
    UIView *irisView = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:irisImage]] autorelease];
    irisView.userInteractionEnabled = YES;
    self.view = irisView;
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
    
    [[NSBundle mainBundle] loadNibNamed:([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad ? @"ARViewOverlays-iPad": @"ARViewOverlays") owner:self options:nil]; // Contains connection to the strong property "overlays".
    self.overlays.frame = self.view.frame;
    if (!focusView) focusView = [[CameraFocusView alloc] initWithFrame:self.view.frame];
   
    // Init instance variables.
    glView = nil;
    gVid = NULL;
    ARResolutionPreset = 0;
    ARCameraPositionPreset = 0;
    arglContextSettings = NULL;
    running = FALSE;
    videoPaused = FALSE;
    runLoopTimePrevious = CFAbsoluteTimeGetCurrent();
    videoAsync = FALSE;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self start];
}

// On iOS 6.0 and later, we must explicitly report which orientations this view controller supports.
- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}

- (void)startRunLoop
{
    if (!running) {
        if (videoAsync) {
            // After starting the video, new frames will invoke frameReadyCallback.
            if (ar2VideoCapStartAsync(gVid, frameReadyCallback, self) != 0) {
                NSLog(@"Error: Unable to begin camera data capture.\n");
                [self stop];
                return;
            }
        } else {
            // But if non-async video (e.g. from a movie file) we'll need to generate regular calls to mainLoop using a display link timer.
            if (ar2VideoCapStart(gVid) != 0) {
                NSLog(@"Error: Unable to begin camera data capture.\n");
                [self stop];
                return;
            }
            runLoopDisplayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(mainLoop)];
            [runLoopDisplayLink setFrameInterval:runLoopInterval];
            [runLoopDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        }
        running = TRUE;
    }
}

- (void)stopRunLoop
{
    if (running) {
        ar2VideoCapStop(gVid);
        if (!videoAsync) {
            [runLoopDisplayLink invalidate];
        }
        running = FALSE;
    }
}

- (void) setRunLoopInterval:(NSInteger)interval
{
    if (interval >= 1) {
        runLoopInterval = interval;
        if (running) {
            [self stopRunLoop];
            [self startRunLoop];
        }
    }
}

- (BOOL) isPaused
{
    if (!running) return (NO);

    return (videoPaused);
}

- (void) setPaused:(BOOL)paused
{
    if (!running) return;
    
    if (videoPaused != paused) {
        if (paused) ar2VideoCapStop(gVid);
        else ar2VideoCapStart(gVid);
        videoPaused = paused;
        if (!videoAsync) {
            if (runLoopDisplayLink.paused != paused) runLoopDisplayLink.paused = paused;
        }
#  ifdef DEBUG
        NSLog(@"Run loop was %s.\n", (paused ? "PAUSED" : "UNPAUSED"));
#  endif
    }
}

static void startCallback(void *userData);

- (IBAction)start
{
    // Open the video path.
    //char *vconf = "-format=BGRA"; // See http://www.artoolworks.com/support/library/Configuring_video_capture_in_ARToolKit_Professional#AR_VIDEO_MODULE_IPHONE
    NSString *vconf = [NSString stringWithFormat:@"%@ %@ %@", @"-format=BGRA", ARResolutionPresets[ARResolutionPreset], ARCameraPositionPresets[ARCameraPositionPreset]];
    if (!(gVid = ar2VideoOpenAsync(vconf.UTF8String, startCallback, self))) {
        if (!(gVid = ar2VideoOpen(vconf.UTF8String))) {
            NSLog(@"Error: Unable to open connection to camera.\n");
            [self stop];
            return;
        }
        [self start2];
    }
}

static void startCallback(void *userData)
{
    ARViewController *vc = (ARViewController *)userData;
    
    [vc start2];
}

- (void) start2
{
    // Find the size of the window.
    int xsize, ysize;
    if (ar2VideoGetSize(gVid, &xsize, &ysize) < 0) {
        NSLog(@"Error: ar2VideoGetSize.\n");
        [self stop];
        return;
    }
    [ARViewController displayToastWithMessage:[NSString stringWithFormat:@"Camera: %dx%d", xsize, ysize]];
    
    // Get the format in which the camera is returning pixels.
    AR_PIXEL_FORMAT pixFormat = ar2VideoGetPixelFormat(gVid);
    if (pixFormat == AR_PIXEL_FORMAT_INVALID) {
        NSLog(@"Error: Camera is using unsupported pixel format.\n");
        [self stop];
        return;
    }

    // Work out if the front camera is being used. If it is, flip the viewing frustum for
    // 3D drawing.
    BOOL flipV = FALSE;
    int frontCamera;
    if (ar2VideoGetParami(gVid, AR_VIDEO_PARAM_AVFOUNDATION_CAMERA_POSITION, &frontCamera) >= 0) {
        if (frontCamera == AR_VIDEO_AVFOUNDATION_CAMERA_POSITION_FRONT) flipV = TRUE;
    }

    // Tell arVideo what the typical focal distance will be. Note that this does NOT
    // change the actual focus, but on devices with non-fixed focus, it lets arVideo
    // choose a better set of camera parameters.
    ar2VideoSetParami(gVid, AR_VIDEO_PARAM_AVFOUNDATION_FOCUS_PRESET, AR_VIDEO_AVFOUNDATION_FOCUS_0_3M); // Default is 0.3 metres. See <AR/video.h> for allowable values.
    
    // Set up default camera parameters.
    ARParam cparam;
    arParamClear(&cparam, xsize, ysize, AR_DIST_FUNCTION_VERSION_DEFAULT);
#ifdef DEBUG
    fprintf(stdout, "*** Camera Parameter ***\n");
    arParamDisp(&cparam);
#endif
    // The camera will be started by -startRunLoop.
    
    // Determine whether ARvideo will return frames asynchronously.
    int ret0;
    if (ar2VideoGetParami(gVid, AR_VIDEO_PARAM_GET_IMAGE_ASYNC, &ret0) != 0) {
        NSLog(@"Error: Unable to query video library for status of async support.\n");
        [self stop];
        return;
    }
    videoAsync = (BOOL)ret0;

    // Allocate the OpenGL view.
    glView = [[[ARView alloc] initWithFrame:[[UIScreen mainScreen] bounds] pixelFormat:kEAGLColorFormatRGBA8 depthFormat:kEAGLDepth16 withStencil:NO preserveBackbuffer:NO] autorelease]; // Don't retain it, as it will be retained when added to self.view.
    [glView setCameraPose:NULL];
    glView.arViewController = self;
    [self.view addSubview:glView];
    
    // Extra view setup.
    [glView addSubview:self.overlays];
    glView.touchDelegate = self;
    [glView addSubview:focusView];
    
    // If flipV is set, flip.
    glView.contentFlipV = flipV;
    
    // Set up content positioning.
    //glView.contentScaleMode = ARViewContentScaleModeFill;
    glView.contentScaleMode = ARViewContentScaleModeFit;
    glView.contentAlignMode = ARViewContentAlignModeTop;
    glView.contentWidth = xsize;
    glView.contentHeight = ysize;
    BOOL isBackingTallerThanWide = (glView.surfaceSize.height > glView.surfaceSize.width);
    if (glView.contentWidth > glView.contentHeight) glView.contentRotate90 = isBackingTallerThanWide;
    else glView.contentRotate90 = !isBackingTallerThanWide;
#ifdef DEBUG
    NSLog(@"[ARViewController start] content %dx%d (wxh) will display in GL context %dx%d%s.\n", glView.contentWidth, glView.contentHeight, (int)glView.surfaceSize.width, (int)glView.surfaceSize.height, (glView.contentRotate90 ? " rotated" : ""));
#endif
    
    // Setup ARGL to draw the background video.
    arglContextSettings = arglSetupForCurrentContext(&cparam, pixFormat);
    arglDistortionCompensationSet(arglContextSettings, FALSE);
    
    arglSetRotate90(arglContextSettings, (glView.contentWidth > glView.contentHeight ? isBackingTallerThanWide : !isBackingTallerThanWide));
    if (flipV) arglSetFlipV(arglContextSettings, TRUE);
    int width, height;
    ar2VideoGetBufferSize(gVid, &width, &height);
    arglPixelBufferSizeSet(arglContextSettings, width, height);
    
    // For FPS statistics.
    arUtilTimerReset();
    gCallCountMarkerDetect = 0;
    
     //Create our runloop timer
    [self setRunLoopInterval:2]; // Target 30 fps on a 60 fps device.
    [self startRunLoop];
}

void frameReadyCallback(void *userdata)
{
    if (!userdata) return;
    ARViewController *vc = (ARViewController *)userdata;
    [vc mainLoop];
}

- (void) mainLoop
{
    // Request a video frame.
    AR2VideoBufferT *buffer = ar2VideoGetImage(gVid);
    if (buffer) {
        
        // Upload the frame to OpenGL.
        if (buffer->bufPlaneCount == 2) arglPixelBufferDataUploadBiPlanar(arglContextSettings, buffer->bufPlanes[0], buffer->bufPlanes[1]);
        else arglPixelBufferDataUpload(arglContextSettings, buffer->buff);
        
        gCallCountMarkerDetect++; // Increment ARToolKit FPS counter.
#ifdef DEBUG
        //NSLog(@"video frame %ld.\n", gCallCountMarkerDetect);
#endif
#ifdef DEBUG
        if (gCallCountMarkerDetect % 150 == 0) {
            NSLog(@"*** Camera - %f (frame/sec)\n", (double)gCallCountMarkerDetect/arUtilTimer());
            gCallCountMarkerDetect = 0;
            arUtilTimerReset();            
        }
#endif
        
        // Get current time (units = seconds).
        NSTimeInterval runLoopTimeNow;
        runLoopTimeNow = CFAbsoluteTimeGetCurrent();
        [glView updateWithTimeDelta:(runLoopTimeNow - runLoopTimePrevious)];
        
        // The display has changed.
        [glView drawView:self];
        
        // Save timestamp for next loop.
        runLoopTimePrevious = runLoopTimeNow;
    }
}

- (IBAction)stop
{
    [self stopRunLoop];
    
    if (arglContextSettings) {
        arglCleanup(arglContextSettings);
        arglContextSettings = NULL;
    }
    [glView removeFromSuperview]; // Will result in glView being released.
    glView = nil;
    [focusView removeFromSuperview];
    
    if (gVid) {
        ar2VideoClose(gVid);
        gVid = NULL;
    }
}

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void)viewWillDisappear:(BOOL)animated {
    [self stop];
    [super viewWillDisappear:animated];
}

- (void)dealloc {
    if (focusView) {
        [focusView release];
        focusView = nil;
    }
    [super dealloc];
}

- (IBAction)switchCamerasButtonPressed
{
    [self stop];

    ARCameraPositionPreset++;
    if (ARCameraPositionPreset >= ARCameraPositionPresetsLength) ARCameraPositionPreset = 0;

    // Don't allow 1080p resolution to be selected for the front camera.
    if ([ARResolutionPresets[ARResolutionPreset] hasSuffix:@"1080p"] && [ARCameraPositionPresets[ARCameraPositionPreset] hasSuffix:@"front"]) {
        ARResolutionPreset++;
        if (ARResolutionPreset >= ARResolutionPresetsLength) ARResolutionPreset = 0;
    }
    
    [self start];
}

- (IBAction)changeResolutionButtonPressed
{
    [self stop];
    
    ARResolutionPreset++;
    if (ARResolutionPreset >= ARResolutionPresetsLength) ARResolutionPreset = 0;
    
    // Don't allow 1080p resolution to be selected for the front camera.
    if ([ARResolutionPresets[ARResolutionPreset] hasSuffix:@"1080p"] && [ARCameraPositionPresets[ARCameraPositionPreset] hasSuffix:@"front"]) {
        ARResolutionPreset++;
        if (ARResolutionPreset >= ARResolutionPresetsLength) ARResolutionPreset = 0;
    }
    
    [self start];
}

- (IBAction)cameraButtonPressed
{
    ar2VideoSetParami(gVid, AR_VIDEO_PARAM_AVFOUNDATION_WILL_CAPTURE_NEXT_FRAME, 1);
}

- (void) handleTouchAtLocation:(CGPoint)location tapCount:(NSUInteger)tapCount
{
    // Convert touch coordinates to a location in the OpenGL viewport.
    CGPoint locationInViewportCoords = CGPointMake(location.x * (float)glView.backingWidth/glView.surfaceSize.width - glView.viewPort[viewPortIndexLeft], (glView.surfaceSize.height - location.y) * (float)glView.backingHeight/glView.surfaceSize.height - glView.viewPort[viewPortIndexBottom]);

    // Now work out where that is in the OpenGL ortho2D frustum.
    int contentWidthFinalOrientation = (glView.contentRotate90 ? glView.contentHeight : glView.contentWidth);
    int contentHeightFinalOrientation = (glView.contentRotate90 ? glView.contentWidth : glView.contentHeight);
    float viewPortScaleFactorWidth = (float)glView.viewPort[viewPortIndexWidth] / (float)contentWidthFinalOrientation;
    float viewPortScaleFactorHeight = (float)glView.viewPort[viewPortIndexHeight] / (float)contentHeightFinalOrientation;
    CGPoint locationInOrtho2DCoords = CGPointMake(locationInViewportCoords.x / viewPortScaleFactorWidth, locationInViewportCoords.y / viewPortScaleFactorHeight);
    
    // Now reverse the transformations we used to fit the content into the ortho2D frustum.
    if (glView.contentRotate90) locationInOrtho2DCoords = CGPointMake(glView.contentWidth - locationInOrtho2DCoords.y, locationInOrtho2DCoords.x);
    if (glView.contentFlipH) locationInOrtho2DCoords = CGPointMake(glView.contentWidth - locationInOrtho2DCoords.x, locationInOrtho2DCoords.y);
    if (glView.contentFlipV) locationInOrtho2DCoords = CGPointMake(locationInOrtho2DCoords.x, glView.contentHeight - locationInOrtho2DCoords.y);
    
    // (0, 0) is top-left of frame.
    CGPoint locationInContentCoords = CGPointMake(locationInOrtho2DCoords.x, glView.contentHeight - locationInOrtho2DCoords.y);
    
    // Now request a point-of-interest focus cycle.
    ar2VideoSetParamd(gVid, AR_VIDEO_FOCUS_POINT_OF_INTEREST_X, locationInContentCoords.x);
    ar2VideoSetParamd(gVid, AR_VIDEO_FOCUS_POINT_OF_INTEREST_Y, locationInContentCoords.y);
    if (ar2VideoSetParami(gVid, AR_VIDEO_FOCUS_MODE, AR_VIDEO_FOCUS_MODE_POINT_OF_INTEREST) == 0) {
        // Show the focus indicator.
        [focusView updatePoint:location];
        [focusView animateFocusingAction];
    }
}


// Call this method to take a snapshot of the ARView.
// Once the image is ready, tookSnapshot:forview: will be called.
- (void)takeSnapshot
{
    // We will need to wait for OpenGL rendering to complete.
    [glView setTookSnapshotDelegate:self];
    [glView takeSnapshot];
}

// Here you can choose what to do with the image.
// We will save it to the iOS camera roll.
- (void)tookSnapshot:(UIImage *)snapshot forView:(EAGLView *)view
{
    // First though, unset ourselves as delegate.
    [glView setTookSnapshotDelegate:nil];
        
    // Write image to camera roll.
    UIImageWriteToSavedPhotosAlbum(snapshot, self, @selector(image:didFinishSavingWithError:contextInfo:), NULL);
}

// Let the user know that the image was saved by playing a shutter sound,
// or if there was an error, put up an alert.
- (void) image:(UIImage *)image didFinishSavingWithError:(NSError *)error contextInfo:(void *)contextInfo
{
    if (!error) {
        SystemSoundID shutterSound;
        AudioServicesCreateSystemSoundID((CFURLRef)[[NSBundle mainBundle] URLForResource: @"slr_camera_shutter" withExtension: @"wav"], &shutterSound);
        AudioServicesPlaySystemSound(shutterSound);
    } else {
        NSString *titleString = @"Error saving screenshot";
        NSString *messageString = [error localizedDescription];
        NSString *moreString = [error localizedFailureReason] ? [error localizedFailureReason] : NSLocalizedString(@"Please try again.", nil);
        messageString = [NSString stringWithFormat:@"%@. %@", messageString, moreString];
        UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:titleString message:messageString delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
        [alertView show];
        [alertView release];
    }
}

+ (void)displayToastWithMessage:(NSString *)toastMessage
{
    [[NSOperationQueue mainQueue] addOperationWithBlock:^ {
        UIWindow * keyWindow = [[UIApplication sharedApplication] keyWindow];
        UILabel *toastView = [[UILabel alloc] init];
        toastView.text = toastMessage;
        toastView.font = [UIFont fontWithName:@"Helvetica" size:14.0f];
        toastView.textColor = [UIColor whiteColor];
        toastView.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.9];
        toastView.textAlignment = NSTextAlignmentCenter;
        toastView.frame = CGRectMake(0.0f, 0.0f, keyWindow.frame.size.width/2.0f, 28.0f);
        toastView.layer.cornerRadius = 7.0f;
        toastView.layer.masksToBounds = YES;
        toastView.center = keyWindow.center;
        
        [keyWindow addSubview:toastView];
        
        [UIView animateWithDuration: 3.0f
                              delay: 0.0
                            options: UIViewAnimationOptionCurveEaseOut
                         animations: ^{
                             toastView.alpha = 0.0;
                         }
                         completion: ^(BOOL finished) {
                             [toastView removeFromSuperview];
                         }
         ];
    }];
}

@end
