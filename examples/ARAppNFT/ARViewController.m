//
//  ARViewController.m
//  ARAppNFT
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
#import <AR/gsub_es.h>
#import <ARUtil/time.h>
#import "../ARAppCore/ARMarkerNFT.h"
#import "../ARAppCore/trackingSub.h"

#define VIEW_DISTANCE_MIN        5.0f          // Objects closer to the camera than this will not be displayed.
#define VIEW_DISTANCE_MAX        2000.0f        // Objects further away from the camera than this will not be displayed.


//
// ARViewController
//

@interface ARViewController (ARViewControllerPrivate)
- (void) loadNFTData;
- (void) startRunLoop;
- (void) stopRunLoop;
- (void) setRunLoopInterval:(NSInteger)interval;
- (void) mainLoop;
@end

@implementation ARViewController {
    
    BOOL            running;
    NSInteger       runLoopInterval;
    NSTimeInterval  runLoopTimePrevious;
    BOOL            videoPaused;
    BOOL            videoAsync;
    CADisplayLink  *runLoopDisplayLink; // For non-async video.
    
    // Video acquisition
    AR2VideoParamT *gVid;
    
    // Marker detection.
    long            gCallCountMarkerDetect;
    
    // Markers.
    NSMutableArray *markers;
    
    // Drawing.
    ARParamLT      *gCparamLT;
    ARView         *glView;
    VirtualEnvironment *virtualEnvironment;
    ARGL_CONTEXT_SETTINGS_REF arglContextSettings;
    
    // NFT.
    THREAD_HANDLE_T     *threadHandle;
    AR2HandleT          *ar2Handle;
    KpmHandle           *kpmHandle;
    AR2SurfaceSetT      *surfaceSet[PAGES_MAX]; // Weak-reference. Strong reference is now in ARMarkerNFT class.
    
    // NFT results.
    int detectedPage; // -2 Tracking not inited, -1 tracking inited OK, >= 0 tracking online on page.
    float trackingTrans[3][4];
}

@synthesize glView, virtualEnvironment, markers;
@synthesize arglContextSettings;
@synthesize running, runLoopInterval;

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
    
    // Init instance variables.
    glView = nil;
    virtualEnvironment = nil;
    markers = nil;
    gVid = NULL;
    gCparamLT = NULL;
    gCallCountMarkerDetect = 0;
    arglContextSettings = NULL;
    running = FALSE;
    videoPaused = FALSE;
    runLoopTimePrevious = CFAbsoluteTimeGetCurrent();
    videoAsync = FALSE;
    
    detectedPage   = -2;
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
    char *vconf = ""; // See http://www.artoolworks.com/support/library/Configuring_video_capture_in_ARToolKit_Professional#AR_VIDEO_MODULE_IPHONE
    if (!(gVid = ar2VideoOpenAsync(vconf, startCallback, self))) {
        NSLog(@"Error: Unable to open connection to camera.\n");
        [self stop];
        return;
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
    
    // Load the camera parameters, resize for the window and init.
    ARParam cparam;
    if (ar2VideoGetCParam(gVid, &cparam) < 0) {
        arParamClearWithFOVy(&cparam, xsize, ysize, M_PI_4); // M_PI_4 radians = 45 degrees.
        NSLog(@"Using default camera parameters for %dx%d image size, 45 degrees vertical field-of-view.", xsize, ysize);
    }
    if (cparam.xsize != xsize || cparam.ysize != ysize) {
#ifdef DEBUG
        fprintf(stdout, "*** Camera Parameter resized from %d, %d. ***\n", cparam.xsize, cparam.ysize);
#endif
        arParamChangeSize(&cparam, xsize, ysize, &cparam);
    }
#ifdef DEBUG
    fprintf(stdout, "*** Camera Parameter ***\n");
    arParamDisp(&cparam);
#endif
    if ((gCparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
        NSLog(@"Error: arParamLTCreate.\n");
        [self stop];
        return;
    }
    // The camera will be started by -startRunLoop.

    //
    // NFT init.
    //
    
    // KPM init.
    kpmHandle = kpmCreateHandle(gCparamLT);
    if (!kpmHandle) {
        NSLog(@"Error: kpmCreateHandle.\n");
        [self stop];
        return;
    }
    //kpmSetProcMode( kpmHandle, KpmProcHalfSize );
    
    // AR2 init.
    if (!(ar2Handle = ar2CreateHandle(gCparamLT, pixFormat, AR2_TRACKING_DEFAULT_THREAD_NUM))) {
        NSLog(@"Error: ar2CreateHandle.\n");
        [self stop];
        return;
    }
    if (threadGetCPU() <= 1) {
#ifdef DEBUG
        NSLog(@"Using NFT tracking settings for a single CPU.");
#endif
        ar2SetTrackingThresh(ar2Handle, 5.0);
        ar2SetSimThresh(ar2Handle, 0.50);
        ar2SetSearchFeatureNum(ar2Handle, 16);
        ar2SetSearchSize(ar2Handle, 6);
        ar2SetTemplateSize1(ar2Handle, 6);
        ar2SetTemplateSize2(ar2Handle, 6);
    } else {
#ifdef DEBUG
        NSLog(@"Using NFT tracking settings for more than one CPU.");
#endif
        ar2SetTrackingThresh(ar2Handle, 5.0);
        ar2SetSimThresh(ar2Handle, 0.50);
        ar2SetSearchFeatureNum(ar2Handle, 16);
        ar2SetSearchSize(ar2Handle, 12);
        ar2SetTemplateSize1(ar2Handle, 6);
        ar2SetTemplateSize2(ar2Handle, 6);
    }
    // NFT dataset loading will happen later.
    
    // Runloop setup.
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
    glView.arViewController = self;
    [self.view addSubview:glView];
    
    // Create the OpenGL projection from the calibrated camera parameters.
    // If flipV is set, flip.
    GLfloat frustum[16];
    arglCameraFrustumRHf(&gCparamLT->param, VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, frustum);
    [glView setCameraLens:frustum];
    glView.contentFlipV = flipV;
    
    // Set up content positioning.
    glView.contentScaleMode = ARViewContentScaleModeFill;
    glView.contentAlignMode = ARViewContentAlignModeCenter;
    glView.contentWidth = gCparamLT->param.xsize;
    glView.contentHeight = gCparamLT->param.ysize;
    BOOL isBackingTallerThanWide = (glView.surfaceSize.height > glView.surfaceSize.width);
    if (glView.contentWidth > glView.contentHeight) glView.contentRotate90 = isBackingTallerThanWide;
    else glView.contentRotate90 = !isBackingTallerThanWide;
#ifdef DEBUG
    NSLog(@"[ARViewController start] content %dx%d (wxh) will display in GL context %dx%d%s.\n", glView.contentWidth, glView.contentHeight, (int)glView.surfaceSize.width, (int)glView.surfaceSize.height, (glView.contentRotate90 ? " rotated" : ""));
#endif
    
    // Setup ARGL to draw the background video.
    arglContextSettings = arglSetupForCurrentContext(&gCparamLT->param, pixFormat);
    
    arglSetRotate90(arglContextSettings, (glView.contentWidth > glView.contentHeight ? isBackingTallerThanWide : !isBackingTallerThanWide));
    if (flipV) arglSetFlipV(arglContextSettings, TRUE);
    int width, height;
    ar2VideoGetBufferSize(gVid, &width, &height);
    arglPixelBufferSizeSet(arglContextSettings, width, height);
        
    // Load marker(s).
    NSString *markerConfigDataFilename = @"Data2/markers.dat";
    if ((markers = [ARMarker newMarkersFromConfigDataFile:markerConfigDataFilename arPattHandle:NULL arPatternDetectionMode:NULL]) == nil) {
        NSLog(@"Error loading markers.\n");
        [self stop];
        return;
    }
#ifdef DEBUG
    NSLog(@"Marker count = %d\n", [markers count]);
#endif

    // Marker data has been loaded, so now load NFT data.
    [self loadNFTData];
    
    // Set up the virtual environment.
    self.virtualEnvironment = [[[VirtualEnvironment alloc] initWithARViewController:self] autorelease];
    [self.virtualEnvironment addObjectsFromObjectListFile:@"Data2/models.dat" connectToARMarkers:markers];
    
    // Because in this example we're not currently assigning a world coordinate system
    // (we're just using local marker coordinate systems), set the camera pose now, to
    // the default (i.e. the identity matrix).
    float pose[16] = {1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f};
    [glView setCameraPose:pose];
    
    // For FPS statistics.
    arUtilTimerReset();
    gCallCountMarkerDetect = 0;
    
     //Create our runloop timer
    [self setRunLoopInterval:2]; // Target 30 fps on a 60 fps device.
    [self startRunLoop];
}

- (void)loadNFTData
{
    int i;
    
    // If data was already loaded, stop KPM tracking thread and unload previously loaded data.
    trackingInitQuit(&threadHandle);
    for (i = 0; i < PAGES_MAX; i++) surfaceSet[i] = NULL; // Discard weak-references.
    
    KpmRefDataSet *refDataSet = NULL;
    int pageCount = 0;
    for (ARMarker *marker in markers) {
        if ([marker isKindOfClass:[ARMarkerNFT class]]) {
            ARMarkerNFT *markerNFT = (ARMarkerNFT *)marker;
            
            // Load KPM data.
            KpmRefDataSet  *refDataSet2;
            printf("Read %s.fset3\n", markerNFT.datasetPathname);
            if (kpmLoadRefDataSet(markerNFT.datasetPathname, "fset3", &refDataSet2) < 0 ) {
                NSLog(@"Error reading KPM data from %s.fset3", markerNFT.datasetPathname);
                markerNFT.pageNo = -1;
                continue;
            }
            markerNFT.pageNo = pageCount;
            if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, pageCount) < 0) {
                NSLog(@"Error: kpmChangePageNoOfRefDataSet");
                exit(-1);
            }
            if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
                NSLog(@"Error: kpmMergeRefDataSet");
                exit(-1);
            }
            printf("  Done.\n");
            
            // For convenience, create a weak reference to the AR2 data.
            surfaceSet[pageCount] = markerNFT.surfaceSet;
            
            pageCount++;
            if (pageCount == PAGES_MAX) break;
        }
    }
    if (kpmSetRefDataSet(kpmHandle, refDataSet) < 0) {
        NSLog(@"Error: kpmSetRefDataSet");
        exit(-1);
    }
    kpmDeleteRefDataSet(&refDataSet);
    
    // Start the KPM tracking thread.
    threadHandle = trackingInitInit(kpmHandle);
    if (!threadHandle) exit(0);
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
        //NSLog(@"video frame %ld (%p).\n", gCallCountMarkerDetect, bufDataPtr);
#endif
#ifdef DEBUG
        if (gCallCountMarkerDetect % 150 == 0) {
            NSLog(@"*** Camera - %f (frame/sec)\n", (double)gCallCountMarkerDetect/arUtilTimer());
            gCallCountMarkerDetect = 0;
            arUtilTimerReset();            
        }
#endif
        
        if (threadHandle) {
            // Perform NFT tracking.
            float            err;
            int              ret;
            int              pageNo;
            
            if( detectedPage == -2 ) {
                trackingInitStart( threadHandle, buffer->buffLuma );
                detectedPage = -1;
            }
            if( detectedPage == -1 ) {
                ret = trackingInitGetResult( threadHandle, trackingTrans, &pageNo);
                if( ret == 1 ) {
                    if (pageNo >= 0 && pageNo < PAGES_MAX) {
                        detectedPage = pageNo;
#ifdef DEBUG
                        NSLog(@"Detected page %d.\n", detectedPage);
#endif
                        ar2SetInitTrans(surfaceSet[detectedPage], trackingTrans);
                    } else {
                        NSLog(@"Detected bad page %d.\n", pageNo);
                        detectedPage = -2;
                    }
                } else if( ret < 0 ) {
                    detectedPage = -2;
                }
            }
            if( detectedPage >= 0 && detectedPage < PAGES_MAX) {
                if( ar2Tracking(ar2Handle, surfaceSet[detectedPage], buffer->buff, trackingTrans, &err) < 0 ) {
                    detectedPage = -2;
                } else {
#ifdef DEBUG
                    NSLog(@"Tracked page %d.\n", detectedPage);
#endif
                }
            }
        } else detectedPage = -2;
        
        // Update all marker objects with detected markers.
        for (ARMarker *marker in markers) {
            if ([marker isKindOfClass:[ARMarkerNFT class]]) {
                [(ARMarkerNFT *)marker updateWithNFTResultsDetectedPage:detectedPage trackingTrans:trackingTrans];
            } else {
                [marker update];
            }
        }
        
        // Get current time (units = seconds).
        NSTimeInterval runLoopTimeNow;
        runLoopTimeNow = CFAbsoluteTimeGetCurrent();
        [virtualEnvironment updateWithSimulationTime:(runLoopTimeNow - runLoopTimePrevious)];
        
        // The display has changed.
        [glView drawView:self];
        
        // Save timestamp for next loop.
        runLoopTimePrevious = runLoopTimeNow;
    }
}

- (IBAction)stop
{
    int i;
    
    [self stopRunLoop];
    
    self.virtualEnvironment = nil;
    
    [markers release];
    
    if (arglContextSettings) {
        arglCleanup(arglContextSettings);
        arglContextSettings = NULL;
    }
    [glView removeFromSuperview]; // Will result in glView being released.
    glView = nil;
    
    // NFT cleanup.
    trackingInitQuit(&threadHandle);
    detectedPage = -2;
    for (i = 0; i < PAGES_MAX; i++) surfaceSet[i] = NULL; // Discard weak-references.
    ar2DeleteHandle(&ar2Handle);
    kpmDeleteHandle(&kpmHandle);
    arParamLTFree(&gCparamLT);
    
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
    [super dealloc];
}

@end
