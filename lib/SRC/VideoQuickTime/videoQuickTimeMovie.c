//
//  videoQuickTimeMovie.h
//  QuickTime movie playing routines.
//
//  Copyright (c) 2007-2012 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//	Authorised licensee ARToolworks, Inc.
//	
//	Rev		Date		Who		Changes
//	1.0.0	2007-01-12	PRL		Initial version.
//
//

//
// Portions © 1998 by Apple Computer, Inc., all rights reserved.
//

#include <AR/video.h>

#ifdef AR_INPUT_QUICKTIME

#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(lib,"QTMLClient.lib")
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(__APPLE__) || defined(_WIN32)

#if defined(_WIN32)
#  include <windows.h>
#  ifndef __MOVIES__
#    include <Movies.h>
#  endif
#  ifndef __QTML__
#    include <QTML.h>
#  endif
#  ifndef __GXMATH__
#    include <GXMath.h>
#  endif
#  ifndef __MEDIAHANDLERS__
#    include <MediaHandlers.h>
#  endif
#  ifndef __MOVIESFORMAT__
#    include <MoviesFormat.h>
#  endif
#else
#  include <QuickTime/QuickTime.h>
#  include "QuickDrawCompatibility.h" // As of Mac OS X 10.7 SDK, a bunch of QuickDraw functions are private.
#endif

#ifdef _WIN32
#  define valloc malloc
#endif

//
// Private types and definitions.
//

#define AR_VIDEO_QUICKTIME_MOVIE_STATUS_BIT_READY	0x01
#define AR_VIDEO_QUICKTIME_MOVIE_STATUS_BIT_BUFFER	0x02

#if defined(__APPLE__)
#  define AR_VIDEO_QUICKTIME_MOVIE_DEFAULT_PIXEL_FORMAT AR_PIXEL_FORMAT_ARGB
#elif defined(_WIN32)
#  define AR_VIDEO_QUICKTIME_MOVIE_DEFAULT_PIXEL_FORMAT AR_PIXEL_FORMAT_BGRA
#else
#  define AR_VIDEO_QUICKTIME_MOVIE_DEFAULT_PIXEL_FORMAT AR_PIXEL_FORMAT_RGBA
#endif

typedef int AR_BOOL;
#ifndef TRUE
#  define TRUE 1
#else
#  if (TRUE != 1)
#    error 'TRUE incorrectly defined somewhere other than __FILE__.'
#  endif
#endif
#ifndef FALSE
#  define FALSE 0
#else
#  if (FALSE != 0)
#    error 'FALSE incorrectly defined somewhere other than __FILE__.'
#  endif
#endif

#ifndef MIN
#define MIN(x,y) (x < y ? x : y)
#endif
#ifndef MAX
#define MAX(x,y) (x > y ? x : y)
#endif

struct _AR_VIDEO_QUICKTIME_MOVIE_t {
	unsigned int	index;			// Index into global array of movies.
	Movie			movie;
	Rect			sourceRect;		// Movie's unscaled bounding rect.
	unsigned int	sourceWidth;	// Movie's unscaled width.
	unsigned int	sourceHeight;	// Movie's unscaled height.
	Rect			destRect;		// Destination buffer's bounding rect.
	unsigned int	destWidth;		// Destination buffer's width.
	unsigned int	destHeight;		// Destination buffer's height.
	Rect			rect;			// Movie's actual post-scaling bounding rect.
	unsigned int	width;			// Movie's actual post-scaling width.
	unsigned int	height;			// Movie's actual post-scaling height.
	GWorldPtr		movieGWorld;
	MovieController	MC;
	TimeValue		movieTimeValueDuration;
	TimeScale		movieTimeScale;
	// For offscreen movies:
	AR_BOOL			offscreen;
	unsigned int	pixFormatEnum;
	long			rowBytes;
	long			bufSize;
	AR_BOOL			bufCopyFlag;
	unsigned char	*pixels;		// QuickTime will unpack movie frames into this buffer.
	TimeValue		movieTime;      // Time of the most recent frame unpacked.
	unsigned char	*pixelsCopy1;	// Buffer 1 of 2 which will be checked out to caller.
	unsigned char	*pixelsCopy2;	// Buffer 2 of 2 which will be checked out to caller.
	MovieDrawingCompleteUPP drawCompleteProc; // UPP for the routine which does the copying.
	unsigned int	status;			// Tracks checked-out state of pixel buffers.
    AR_BOOL         playEveryFrame;
    AR_BOOL         loop;
	// For full-screen windowed movies.
	WindowRef		fullScreenWindowRef;
	Ptr				fullScreenRestoreState;
};

#define AR_VIDEO_QUICKTIME_MOVIE_COUNT_MAX 128 // "640k ought to be enough for anybody."

//
// Globals.
//

static int						gInited = 0;

static AR_VIDEO_QUICKTIME_MOVIE_t	gMovies[AR_VIDEO_QUICKTIME_MOVIE_COUNT_MAX] = {NULL};
static unsigned int gMovieIndex = AR_VIDEO_QUICKTIME_MOVIE_COUNT_MAX - 1; // Last allocated slot in gMovies.

//
// Private functions.
//

//
// newMovieFromURL
// Open the movie file referenced by the specified uniform resource locator (URL).
//
//
//  QuickTime Streaming has a URL data handler, which you can use to open movies that are
//  specified using uniform resource locators (URLs). A URL is the address of some resource
//  on the Internet or on a local disk. The QuickTime URL data handler can open http URLs,
//  ftp URLs, file URLs, and rtsp URLs.
//
Movie newMovieFromURL(char *theURL)
{
	Movie    myMovie = NULL;
	Handle    myHandle = NULL;
	Size    mySize = 0;
	
	// copy the specified URL into a handle
	// get the size of the URL, plus the terminating null byte
	mySize = (Size)strlen(theURL) + 1;
	if (mySize == 0) goto bail;
	
	// allocate a new handle
	myHandle = NewHandleClear(mySize);
    if (myHandle == NULL) goto bail;
	
	// copy the URL into the handle
	BlockMove(theURL, *myHandle, mySize);
	
	// instantiate a movie from the specified URL
	// the data reference that is passed to NewMovieFromDataRef is a handle
	// containing the text of the URL, *with* a terminating null byte; this
	// is an exception to the usual practice with data references (where you
	// need to pass a handle to a handle containing the relevant data)
	NewMovieFromDataRef(&myMovie, newMovieActive, NULL, myHandle, URLDataHandlerSubType);
	
bail:
	if (myHandle != NULL) DisposeHandle(myHandle);
    
	return(myMovie);
}

//
// getURLBasename
// Return the basename of the specified URL.
//
// The basename of a URL is the portion of the URL following the rightmost URL separator. This function
// is useful for setting window titles of movies opened using the URL data handler to the basename of a
// URL (just like MoviePlayer does).
//
// The caller is responsible for disposing of the pointer returned by this function (by calling free).
//
char *getURLBasename (char *theURL)
{
	char  *myBasename = NULL;
	short  myLength = 0;
	short  myIndex;
	
	// make sure we got a URL passed in
	if (theURL == NULL) goto bail;
    
	// get the length of the URL
	myLength = (short)strlen(theURL);
	
	// find the position of the rightmost URL separator in theURL
	if (strchr(theURL, '/') != NULL) {
		myIndex = myLength - 1;
		while (theURL[myIndex] != '/') myIndex--;
		
		// calculate the length of the basename
		myLength = myLength - myIndex - 1;
	} else {
		// there is no rightmost URL separator in theURL;
		// set myIndex so that myIndex + 1 == 0, for the call to BlockMove below
		myIndex = -1;
	}
	
	// allocate space to hold the string that we return to the caller
	myBasename = malloc(myLength + 1);
	if (myBasename == NULL) goto bail;
    
	// copy into myBasename the substring of theURL from myIndex + 1 to the end
	BlockMove(&theURL[myIndex + 1], myBasename, myLength);
	myBasename[myLength] = '\0';
	
bail:  
	return(myBasename);
}

#ifdef _WIN32
//
// handleMessages
// Handle Windows messages for the full-screen window.
// 
LRESULT CALLBACK handleMessages (HWND theWnd, UINT theMessage, UINT wParam, LONG lParam)
{
	MSG				myMsg = {0};
	EventRecord		myMacEvent;
	DWORD			myPoints;
	AR_VIDEO_QUICKTIME_MOVIE_t movie;

	if (theMessage != WM_COMMAND) {

		// Get the attached user data, if any.
		movie = gMovies[(unsigned int)GetWindowLong(theWnd, GWL_USERDATA)];
		if (movie) {
			if (movie->MC) {
				// pass the Mac event to the movie controller,
				myMsg.hwnd = theWnd;
				myMsg.message = theMessage;
				myMsg.wParam = wParam;
				myMsg.lParam = lParam;
				myMsg.time = GetMessageTime();
				myPoints = GetMessagePos();
				myMsg.pt.x = LOWORD(myPoints);
				myMsg.pt.y = HIWORD(myPoints);
		
				// translate the Windows message to a Mac event
				WinEventToMacEvent(&myMsg, &myMacEvent);
		
				// only if not minimised.
				if (!IsIconic(theWnd)) MCIsPlayerEvent(movie->MC, &myMacEvent);
			}
		}
	}

	return (DefWindowProc(theWnd, theMessage, wParam, lParam));
}
#endif // _WIN32

// Called when our movie has completed drawing a frame into
// our offscreen.
static pascal OSErr drawCompleteProc(Movie theActualMovie, long refCon)
{
#pragma unused(theActualMovie)
	AR_VIDEO_QUICKTIME_MOVIE_t movie;
    void *pixelsCopy;
	
	movie = gMovies[(unsigned int)refCon];
	if (movie) {
		if (movie->bufCopyFlag) {
            pixelsCopy = (movie->status & AR_VIDEO_QUICKTIME_MOVIE_STATUS_BIT_BUFFER ? movie->pixelsCopy2 : movie->pixelsCopy1);
			memcpy(pixelsCopy, (void *)(movie->pixels), movie->bufSize);
		}
		movie->status |= AR_VIDEO_QUICKTIME_MOVIE_STATUS_BIT_READY; // Set ready bit.
        movie->movieTime = GetMovieTime(theActualMovie, NULL);
    }
	
	return noErr;    
}

//
// Public functions.
//

AR_E_t arVideoQuickTimeMovieInit(void)
{
	OSStatus                err_oss;
	OSErr					err_ose;
    long version;

	if (gInited) return (AR_E_INVALID_COMMAND);
	
#ifdef _WIN32
	if ((err_ose = InitializeQTML(0)) != noErr) {
        ARLOGe("OS error: QuickTime not installed.\n");
		return (AR_E_LIBRARY_NOT_AVAILABLE);
	}
#endif // _WIN32
	
	// Check the version of QuickTime installed.
	if ((err_oss = Gestalt(gestaltQuickTime, &version)) != noErr) {
        ARLOGe("OS error: QuickTime not installed.\n");
		return (AR_E_LIBRARY_NOT_AVAILABLE);
	}
	
	// Check for a minimum version.
	// 0x06008000: First public release of QuickTime 6.
	// 0x06408000: First release of QuickTime supporting threading.
	// 0x04000000: First release of QuickTime with Sequence Grabber.
    if (version < 0x06008000) {
        ARLOGe("OS error: QuickTime version 6 or later is required.\n");
		return (AR_E_LIBRARY_TOO_OLD);
    }
	
	if ((err_ose = EnterMovies()) != noErr) {
        ARLOGe("OS error (%hd): Can't initialize QuickTime.\n", err_ose);
		return (AR_E_GENERIC_TOOLBOX);
	}
	
	gInited = 1;
	return (AR_E_NONE);
}

AR_E_t arVideoQuickTimeMoviePlay(char *URL, char *conf, AR_VIDEO_QUICKTIME_MOVIE_t *movie_p)
{
	AR_VIDEO_QUICKTIME_MOVIE_t movie = NULL;
	unsigned int	movieIndex;
	AR_E_t			err_e = AR_E_NONE;
	OSErr			err_ose;
	QDErr			err_qd;
    ComponentResult err_comp;
	long			bytesPerPixel;
	CGrafPtr		theSavedPort;
	GDHandle		theSavedDevice;
	GWorldPtr		tmpGW;
	Rect			tmpRect;
	long			flags = 0L;
	int				len;
	AR_BOOL			URLOK = FALSE;
    MediaHandler    mh = NULL;

    // Configuration options.
    char			*a, line[256];
	int				err_i = 0;
	OSType			pixFormat = (OSType)0;
	unsigned int	pixFormatEnum = 0;
	int				width = 0;
	int				height = 0;
	AR_BOOL			offscreen = FALSE;
	AR_BOOL			alpha = TRUE;
	unsigned int	loop = 0;
	unsigned int	showController = 0, singleBuffer = 0, pause = 0;
	int				noSound = 0;
	int				flipH = 0, flipV = 0;
	int				scaleFill = 0, scale1to1 = 0, scaleStretch = 0;
    float           rate = 0.0f;
    Fixed           rateFixed;
    AR_BOOL         playEveryFrame = FALSE;

	short left, top, right, bottom;
	
	if (!URL) return (AR_E_INVALID_COMMAND);
	if (!(URL[0])) return (AR_E_INVALID_COMMAND);

	if (!gInited) {
		if ((err_e = arVideoQuickTimeMovieInit()) != AR_E_NONE) return (err_e);
	}

	// Process configuration options.
    if (conf) {
		a = conf;
		err_i = 0;
        for (;;) {
            while (*a == ' ' || *a == '\t') a++; // Skip whitespace.
            if (*a == '\0') break;
			
            if (strncmp(a, "-width=", 7) == 0) {
                sscanf(a, "%s", line);
                if (strlen(line) <= 7 || sscanf(&line[7], "%d", &width) == 0) err_i = 1;
            } else if (strncmp(a, "-height=", 8) == 0) {
                sscanf(a, "%s", line);
                if (strlen(line) <= 8 || sscanf(&line[8], "%d", &height) == 0) err_i = 1;
            } else if (strncmp(a, "-pixelformat=", 13) == 0) {
                sscanf(a, "%s", line);
				if (strlen(line) <= 13) err_i = 1;
				else {
#ifdef AR_BIG_ENDIAN
					if (strlen(line) == 17) err_i = (sscanf(&line[13], "%4c", (char *)&pixFormat) < 1);
#else
					if (strlen(line) == 17) err_i = (sscanf(&line[13], "%c%c%c%c", &(((char *)&pixFormat)[3]), &(((char *)&pixFormat)[2]), &(((char *)&pixFormat)[1]), &(((char *)&pixFormat)[0])) < 1);
#endif
					else err_i = (sscanf(&line[13], "%li", (long *)&pixFormat) < 1); // Integer.
				}
            } else if (strncmp(a, "-fliph", 6) == 0) {
                flipH = 1;
            } else if (strncmp(a, "-nofliph", 8) == 0) {
                flipH = 0;
            } else if (strncmp(a, "-flipv", 6) == 0) {
                flipV = 1;
            } else if (strncmp(a, "-noflipv", 8) == 0) {
                flipV = 0;
            } else if (strncmp(a, "-loop", 5) == 0) {
                loop = 1;
            } else if (strncmp(a, "-noloop", 7) == 0) {
                loop = 0;
            } else if (strncmp(a, "-fill", 5) == 0) {
                scaleFill = 1;
            } else if (strncmp(a, "-nofill", 7) == 0) {
                scaleFill = 0;
            } else if (strncmp(a, "-1to1", 4) == 0) {
                scale1to1 = 1;
            } else if (strncmp(a, "-no1to1", 6) == 0) {
                scale1to1 = 0;
            } else if (strncmp(a, "-stretch", 8) == 0) {
                scaleStretch = 1;
            } else if (strncmp(a, "-nostretch", 10) == 0) {
                scaleStretch = 0;
            } else if (strncmp(a, "-offscreen", 10) == 0) {
                offscreen = TRUE;
            } else if (strncmp(a, "-nooffscreen", 12) == 0) {
                offscreen = FALSE;
            } else if (strncmp(a, "-showcontroller", 15) == 0) {
                showController = 1;
            } else if (strncmp(a, "-noshowcontroller", 17) == 0) {
                showController = 0;
            } else if (strncmp(a, "-mute", 5) == 0) {
                noSound = 1;
            } else if (strncmp(a, "-nomute", 7) == 0) {
                noSound = 0;
			} else if (strncmp(a, "-singlebuffer", 13) == 0) {
                singleBuffer = 1;
			} else if (strncmp(a, "-nosinglebuffer", 15) == 0) {
				singleBuffer = 0;
			} else if (strncmp(a, "-pause", 6) == 0) {
                pause = 1;
			} else if (strncmp(a, "-nopause", 8) == 0) {
                pause = 0;
			} else if (strncmp(a, "-alpha", 6) == 0) {
                alpha = TRUE;
			} else if (strncmp(a, "-noalpha", 8) == 0) {
                alpha = FALSE;
			} else if (strncmp(a, "-playeveryframe", 15) == 0) {
                playEveryFrame = TRUE;
            } else if (strncmp(a, "-rate=", 6) == 0) {
                sscanf(a, "%s", line);
                if (strlen(line) <= 6 || sscanf(&line[6], "%f", &rate) == 0) err_i = 1;
            } else {
                err_i = 1;
            }
			
			if (err_i) {
				ARLOGe("Error with configuration option. Ignoring.\n");
			}
            
			while (*a != ' ' && *a != '\t' && *a != '\0') a++; // Skip to next whitespace.
        }
    }

	len = (int)strlen(URL);
	if (len >= 7) {
		if ((strncmp(URL, "file://", 7) == 0) ||
			(strncmp(URL, "rtsp://", 7) == 0) ||
			(strncmp(URL, "http://", 7) == 0) ||
			(strncmp(URL, "https://", 8 ) == 0) ||
			(strncmp(URL, "ftp://", 6) == 0)) {
			URLOK = TRUE;
		}
	}

	// Find a free slot in the gMovies array.
	movieIndex = gMovieIndex;
	do {
		movieIndex++;
		if (movieIndex == AR_VIDEO_QUICKTIME_MOVIE_COUNT_MAX) movieIndex = 0; // Loop around.
		if (movieIndex == gMovieIndex) return (AR_E_OVERFLOW); // No free slots = too many movies loaded.
	} while (gMovies[movieIndex]); // Free slots have NULL pointers.
	gMovieIndex = movieIndex;

	// Allocate the parameters structure and fill it in. It will be free'd when the movie is closed.
	if (!(movie = (AR_VIDEO_QUICKTIME_MOVIE_t)calloc(1, sizeof(struct _AR_VIDEO_QUICKTIME_MOVIE_t)))) return (AR_E_OUT_OF_MEMORY);
	gMovies[movieIndex] = movie;
	movie->index = movieIndex; // The index allows us to find the movie info from an unsigned int. Create the reverse here.
	movie->offscreen = offscreen;
	movie->bufCopyFlag = !singleBuffer;
    movie->playEveryFrame = playEveryFrame;
    movie->loop = loop;

    // Open the movie.
	// Find movie's dimensions first, using a temp 1x1 offscreen GWorld.
	// We will overwrite current GWorld, so save and restore it.
	GetGWorld(&theSavedPort, &theSavedDevice);
	MacSetRect(&tmpRect, 0, 0, 1, 1);
	err_qd = NewGWorld(&tmpGW, 32, &tmpRect, NULL, NULL, 0);
	SetGWorld(tmpGW, NULL);
	movie->movie = newMovieFromURL(URL);
	if (!(movie->movie)) {
		ARLOGe("Movie not found at URL %s.\n", URL);
		err_e = AR_E_FILE_NOT_FOUND;
		goto bail1;
	}
    GetMovieNaturalBoundsRect(movie->movie, &(movie->sourceRect));
	//GetMovieBox(movie->movie, &(movie->sourceRect));
	MacOffsetRect(&(movie->sourceRect), -movie->sourceRect.left, -movie->sourceRect.top); // The movie's bounding box may not actually have an origin of (0, 0).
	movie->sourceWidth = movie->sourceRect.right;
	movie->sourceHeight = movie->sourceRect.bottom;
	
	// Create the place the rendering will go to, either
	// a full-screen window or an off-screen buffer.
	
	if (!movie->offscreen) {
		
		// At present, width and height do nothing in fullscreen mode.
		// Default size of destination is current screen size.
		//if (!width) width = ;
		//if (!height) height = ;
		
		// Set up a new window for full-screen display.
		err_ose = BeginFullScreen(&(movie->fullScreenRestoreState), NULL, 0, 0, &(movie->fullScreenWindowRef), NULL, /*fullScreenDontChangeMenuBar |*/ fullScreenAllowEvents);
		if (err_ose != noErr) {
			ARLOGe("Error (%d) beginning full-screen mode.\n", err_ose);
			err_e = AR_E_GENERIC_TOOLBOX;
			goto bail;
		}
# ifdef _WIN32
		// on Windows, set a window procedure for the new window and associate a port with that window.
		QTMLSetWindowWndProc(movie->fullScreenWindowRef, handleMessages);
		SetWindowLong(GetPortNativeWindow(movie->fullScreenWindowRef), GWL_USERDATA, (LONG)(movie->index));
		CreatePortAssociation(GetPortNativeWindow(movie->fullScreenWindowRef), NULL, 0L);
#else
		SetWRefCon(movie->fullScreenWindowRef, (long)(movie->index));
#endif // _WIN32
		movie->movieGWorld = GetWindowPort(movie->fullScreenWindowRef);
		
		// Work out how big the destination is.
#if TARGET_API_MAC_CARBON
		GetPortBounds(movie->movieGWorld, &(movie->destRect));
#else
		movie->destRect = movie->movieGWorld->portRect;
#endif
		MacOffsetRect(&(movie->destRect), -movie->destRect.left, -movie->destRect.top); // The destination's bounding box may not actually have an origin of (0, 0).
		movie->destWidth = movie->destRect.right;
		movie->destHeight = movie->destRect.bottom;
	
	} else {
		
		// Default size of destination is same size as source.
		// This assumes its OK to draw into an arbitrarily-sized buffer.
		// This assumption might be violated if the end-user of the buffer
		// can't tolerate arbitrary sizes, and the movie's size isn't
		// one of the tolerated sizes. E.g. OpenGL might require power-of-two
		// dimensions since the buffer will be used as texture data.
		if (!width) width = movie->sourceWidth;
		if (!height) height = movie->sourceHeight;
		movie->destWidth = width;
		movie->destHeight = height;
		MacSetRect(&(movie->destRect), 0, 0, width, height);
				
        // If no pixel format was specified in command-line options,
        // assign the one specified at compile-time as the default.
        if (!pixFormat) {
            switch (AR_VIDEO_QUICKTIME_MOVIE_DEFAULT_PIXEL_FORMAT) {
                case AR_PIXEL_FORMAT_2vuy:
                    pixFormat = k2vuyPixelFormat;		// k422YpCbCr8CodecType, k422YpCbCr8PixelFormat
                    break;
                case AR_PIXEL_FORMAT_yuvs:
                    pixFormat = kYUVSPixelFormat;		// kComponentVideoUnsigned
                    break;
                case AR_PIXEL_FORMAT_RGB:
                    pixFormat = k24RGBPixelFormat;
                    break;
                case AR_PIXEL_FORMAT_BGR:
                    pixFormat = k24BGRPixelFormat;
                    break;
                case AR_PIXEL_FORMAT_ARGB:
                    pixFormat = k32ARGBPixelFormat;
                    break;
                case AR_PIXEL_FORMAT_RGBA:
                    pixFormat = k32RGBAPixelFormat;
                    break;
                case AR_PIXEL_FORMAT_ABGR:
                    pixFormat = k32ABGRPixelFormat;
                    break;
                case AR_PIXEL_FORMAT_BGRA:
                    pixFormat = k32BGRAPixelFormat;
                    break;
                case AR_PIXEL_FORMAT_MONO:
                    pixFormat = k8IndexedGrayPixelFormat;
                    break;
                default:
                    ARLOGe("Error: Unsupported default pixel format specified.\n");
                    goto bail1;
            }
        }

		switch (pixFormat) {
			case k2vuyPixelFormat:
				bytesPerPixel = 2l;
				pixFormatEnum = AR_PIXEL_FORMAT_2vuy;
				break;
			case kYUVSPixelFormat:
				bytesPerPixel = 2l;
				pixFormatEnum = AR_PIXEL_FORMAT_yuvs;
				break; 
			case k24RGBPixelFormat:
				bytesPerPixel = 3l;
				pixFormatEnum = AR_PIXEL_FORMAT_RGB;
				break;
			case k24BGRPixelFormat:
				bytesPerPixel = 3l;
				pixFormatEnum = AR_PIXEL_FORMAT_BGR;
				break;
			case k32ARGBPixelFormat:
				bytesPerPixel = 4l;
				pixFormatEnum = AR_PIXEL_FORMAT_ARGB;
				break;
			case k32BGRAPixelFormat:
				bytesPerPixel = 4l;
				pixFormatEnum = AR_PIXEL_FORMAT_BGRA;
				break;
			case k32ABGRPixelFormat:
				bytesPerPixel = 4l;
				pixFormatEnum = AR_PIXEL_FORMAT_ABGR;
				break;
			case k32RGBAPixelFormat:
				bytesPerPixel = 4l;
				pixFormatEnum = AR_PIXEL_FORMAT_RGBA;
				break;
			case k8IndexedGrayPixelFormat:
				bytesPerPixel = 1l;
				pixFormatEnum = AR_PIXEL_FORMAT_MONO;
				break;
			default:
#ifdef AR_BIG_ENDIAN
				ARLOGe("Unsupported pixel format requested:  0x%08x = %u = '%c%c%c%c'.\n", (unsigned int)pixFormat, (unsigned int)pixFormat, ((char *)&pixFormat)[0], ((char *)&pixFormat)[1], ((char *)&pixFormat)[2], ((char *)&pixFormat)[3]);
#else
				ARLOGe("Unsupported pixel format requested:  0x%08x = %u = '%c%c%c%c'.\n", (unsigned int)pixFormat, (unsigned int)pixFormat, ((char *)&pixFormat)[3], ((char *)&pixFormat)[2], ((char *)&pixFormat)[1], ((char *)&pixFormat)[0]);
#endif
				goto bail1;
				break;			
		}
		
		// Allocate buffer for QuickTime to write pixel data into, and use
		// QTNewGWorldFromPtr() to wrap an offscreen GWorld structure around
		// it. We do it in these two steps rather than using QTNewGWorld()
		// to guarantee that we don't get padding bytes at the end of rows.
		// We use valloc to get page-aligned buffers.
		movie->pixFormatEnum = pixFormatEnum;
		movie->rowBytes = movie->destWidth * bytesPerPixel;
		movie->bufSize = movie->destHeight * movie->rowBytes;
		movie->pixels = (unsigned char *)valloc(movie->bufSize * sizeof(unsigned char));
		if (movie->bufCopyFlag) {
			movie->pixelsCopy1 = (unsigned char *)valloc(movie->bufSize * sizeof(unsigned char));
			movie->pixelsCopy2 = (unsigned char *)valloc(movie->bufSize * sizeof(unsigned char));
		}
		if (!(movie->pixels) || (movie->bufCopyFlag && (!(movie->pixelsCopy1) || !(movie->pixelsCopy2)))) {
			err_e = AR_E_OUT_OF_MEMORY;
			goto bail1;
		}
		// Wrap a GWorld around the pixel buffer.
		err_ose = QTNewGWorldFromPtr(&(movie->movieGWorld), // returned GWorld
								   pixFormat,				// format of pixels
								   &(movie->destRect),		// bounds
								   0,						// color table
								   NULL,					// GDHandle
								   0,						// flags
								   (void *)(movie->pixels), // pixel base addr
								   movie->rowBytes);		// bytes per row
		if (err_ose != noErr) {
			ARLOGe("Unable to create offscreen buffer (Mac OS error code %d).\n", err_ose);
			err_e = AR_E_GENERIC_TOOLBOX;
			goto bail1;
		}
		
		// Lock the pixmap and make sure it's locked because
		// we can't decompress into an unlocked PixMap.
		if (!LockPixels(GetGWorldPixMap(movie->movieGWorld))) {
			ARLOGe("Unable to lock buffer.\n");
			err_e = AR_E_GENERIC_TOOLBOX;
			goto bail1;
		}
        
	}
	
	// Calculate movie scaling.
	if (scaleStretch) {
		movie->width = movie->destWidth;
		movie->height = movie->destHeight;
		left = top = 0;
	} else {
		if (scale1to1) {
			movie->width = movie->sourceWidth;
			movie->height = movie->sourceHeight;
		} else {
			double scaleRatioWidth, scaleRatioHeight, scaleRatio;
			scaleRatioWidth = (double)movie->destWidth / (double)movie->sourceWidth;
			scaleRatioHeight = (double)movie->destHeight / (double)movie->sourceHeight;
			if (scaleFill) scaleRatio = MAX(scaleRatioHeight, scaleRatioWidth);
			else scaleRatio = MIN(scaleRatioHeight, scaleRatioWidth);
			movie->width = (unsigned int)((double)movie->sourceWidth * scaleRatio);
			movie->height = (unsigned int)((double)movie->sourceHeight * scaleRatio);
		}
		// Position in centre of destination (but on unit pixel boundary of course).
		left = (short)((movie->destWidth - movie->width) / 2);
		top = (short)((movie->destHeight - movie->height) / 2);
	}
	right = left + movie->width;
	bottom = top + movie->height;
	MacSetRect(&(movie->rect), left, top, right, bottom);
	//ARLOGe("(W,H) source: %d,%d, destination: %d,%d, movie: %d,%d.\n", movie->sourceWidth, movie->sourceHeight, movie->destWidth, movie->destHeight, movie->width, movie->height);
	
	// Set the rendering destination for the movie
	SetMovieGWorld(movie->movie, movie->movieGWorld, NULL);
	if ((err_ose = GetMoviesError()) != noErr) {
		ARLOGe("Unable to set movie offscreen buffer (Mac OS error code %d).\n", err_ose);
		err_e = AR_E_GENERIC_TOOLBOX;
		goto bail1;
	}
	SetMovieBox(movie->movie, &(movie->rect));
	
	// If flipping, adjust movie matrix.
	if (flipV || flipH) {
		MatrixRecord matrix;
		GetMovieMatrix(movie->movie, &matrix);
		ScaleMatrix(&matrix, IntToFixed(flipH ? -1 : 1), IntToFixed(flipV ? -1 : 1), IntToFixed((left + right) / 2), IntToFixed((top + bottom) / 2));
		SetMovieMatrix(movie->movie, &matrix);
	}
	
	if (movie->offscreen) {
		// Set a movie drawing complete proc.
		movie->drawCompleteProc = NewMovieDrawingCompleteUPP(drawCompleteProc);
		if (movie->drawCompleteProc) {
			SetMovieDrawingCompleteProc(movie->movie, movieDrawingCallWhenChanged, movie->drawCompleteProc, (long)(movie->index));
		}
	}
    
    // Set transfer mode. Default is graphicsModeStraightAlpha. See QuickDrawCompatibility.h for others.
    // See also http://developer.apple.com/library/mac/documentation/quicktime/qtff/QTFFChap4/qtff4.html#//apple_ref/doc/uid/TP40000939-CH206-BBCDHAAE
    if (alpha) {
        mh = GetMediaHandler(GetTrackMedia(GetMovieIndTrackType(movie->movie, 1L, VideoMediaType, movieTrackMediaType)));
        if (!mh) {
            ARLOGe("Unable to retrieve movie media handler. Ignoring request for alpha transfer mode.\n");
        } else {
            err_comp = MediaSetGraphicsMode(mh, srcCopy, NULL);
            if (err_comp != noErr) {
                ARLOGe("Error %ld setting alpha transfer mode.\n", err_comp);
            }
        }
    }
    
	// Get the movie duration and time scale.
	movie->movieTimeValueDuration = GetMovieDuration(movie->movie);
	movie->movieTimeScale = GetMovieTimeScale(movie->movie);
	
	// Create the movie controller.
	SetGWorld(movie->movieGWorld, NULL); // NewMovieController caches the current GWorld, so need to set it first.
	if (!showController) flags = mcNotVisible | mcTopLeftMovie;
	movie->MC = NewMovieController(movie->movie, &(movie->rect), flags);
	if (!movie->MC) {
		ARLOGe("Unable to create movie controller.\n");
		err_e = AR_E_GENERIC_TOOLBOX;
		goto bail1;
	}
	
	// Loop the movie or not.
	err_comp = MCDoAction(movie->MC, mcActionSetLooping, (void *)loop);
    if (err_comp != noErr) {
		ARLOGe("Unable to set movie looping state (Mac OS error code %ld).\n", err_comp);
		err_e = AR_E_GENERIC_TOOLBOX;
		goto bail1;
    }

	// Adjust movie volume.
	if (noSound) {
		err_comp = MCDoAction(movie->MC, mcActionSetVolume, (void *)IntToFixed(0));
        if (err_comp != noErr) {
            ARLOGe("Unable to set movie volume (Mac OS error code %ld).\n", err_comp);
            err_e = AR_E_GENERIC_TOOLBOX;
            goto bail1;
        }
	}
	
	// Use the movie controller instead of StartMovie();.
    if (pause || playEveryFrame) rateFixed = IntToFixed(0);
    else if (rate != 0.0f) rateFixed = FloatToFixed(rate);
    else rateFixed = GetMoviePreferredRate(movie->movie);
	err_comp = MCDoAction(movie->MC, mcActionPrerollAndPlay, (void*)rateFixed);
    if (err_comp != noErr) {
		ARLOGe("Unable to preroll%s movie (Mac OS error code %ld).\n", (pause ? "" : " and play"), err_comp);
		err_e = AR_E_GENERIC_TOOLBOX;
		goto bail1;
    }
    
	goto bail;

bail1:
	arVideoQuickTimeMovieStop(&movie); // Cleans up and deallocates.
	
bail:	
	DisposeGWorld(tmpGW);
	SetGWorld(theSavedPort, theSavedDevice);      // Restore GWorld.
	*movie_p = movie;
	return (err_e);
}	

AR_E_t arVideoQuickTimeMovieIdle(AR_VIDEO_QUICKTIME_MOVIE_t movie)
{
	if (!movie) return (AR_E_INVALID_COMMAND);
	MCIdle(movie->MC);
	if (IsMovieDone(movie->movie)) return (AR_E_EOF);
	return (AR_E_NONE);
}

AR_E_t arVideoQuickTimeMovieSetPlayRate(AR_VIDEO_QUICKTIME_MOVIE_t movie, float rate)
{
	if (!movie) return (AR_E_INVALID_COMMAND);
    if (!movie->playEveryFrame) {
        MCDoAction(movie->MC, mcActionPlay, (void *)FloatToFixed(rate));
    }
	return (AR_E_NONE);
}

AR_E_t arVideoQuickTimeMovieGetPlayRate(AR_VIDEO_QUICKTIME_MOVIE_t movie, float *rate)
{
	Fixed rateFixed;
	if (!movie || !rate) return (AR_E_INVALID_COMMAND);
	MCDoAction(movie->MC, mcActionGetPlayRate, (void *)(&rateFixed));
	*rate = FixedToFloat(rateFixed);
	return (AR_E_NONE);
}

#ifndef shortfixed1
#  define shortfixed1              ((ShortFixed) 0x0100)
#endif
#ifndef ShortFixedToFloat
#  define ShortFixedToFloat(a)     ((float)(a) / shortfixed1)
#endif
#ifndef FloatToShortFixed
#  define FloatToShortFixed(a)     ((ShortFixed)((float)(a) * shortfixed1))
#endif

AR_E_t arVideoQuickTimeMovieSetPlayVolume(AR_VIDEO_QUICKTIME_MOVIE_t movie, float volume)
{
	if (!movie) return (AR_E_INVALID_COMMAND);
	MCDoAction(movie->MC, mcActionSetVolume, (void *)((long)((ShortFixed)FloatToShortFixed(volume))));
	return (AR_E_NONE);
}

AR_E_t arVideoQuickTimeMovieGetPlayVolume(AR_VIDEO_QUICKTIME_MOVIE_t movie, float *volume)
{
	ShortFixed volumeFixed;
	if (!movie || !volume) return (AR_E_INVALID_COMMAND);
	MCDoAction(movie->MC, mcActionGetVolume, (void *)(&volumeFixed));
	*volume = ShortFixedToFloat(volumeFixed);
	return (AR_E_NONE);
}

AR_E_t arVideoQuickTimeMovieGetPlayLength(AR_VIDEO_QUICKTIME_MOVIE_t movie, double *lengthInSeconds)
{
	if (!movie) return (AR_E_INVALID_COMMAND);
	if (!movie->movieTimeValueDuration || !movie->movieTimeScale) return (AR_E_LENGTH_UNAVAILABLE);
	*lengthInSeconds = (double)movie->movieTimeValueDuration / (double)movie->movieTimeScale;
	return (AR_E_NONE);
}

AR_E_t arVideoQuickTimeMovieGetPlayPosition(AR_VIDEO_QUICKTIME_MOVIE_t movie, double *positionInSeconds)
{
	TimeValue tvGot;
	
	if (!movie) return (AR_E_INVALID_COMMAND);
	if (!movie->movieTimeScale) return (AR_E_LENGTH_UNAVAILABLE);
	tvGot = GetMovieTime(movie->movie, NULL);
	*positionInSeconds = (double)tvGot / (double)movie->movieTimeScale;
	return (AR_E_NONE);
}

AR_E_t arVideoQuickTimeMovieSetPlayPosition(AR_VIDEO_QUICKTIME_MOVIE_t movie, double positionInSeconds)
{
	TimeValue tvToSet;
	
	if (!movie) return (AR_E_INVALID_COMMAND);
	if (!movie->movieTimeValueDuration || !movie->movieTimeScale) return (AR_E_LENGTH_UNAVAILABLE);
	tvToSet = (long)(positionInSeconds / (1.0 / (double)movie->movieTimeScale));
	if (tvToSet < 0 || tvToSet > movie->movieTimeValueDuration) return (AR_E_OVERFLOW);
	SetMovieTimeValue(movie->movie, tvToSet);
	MCMovieChanged(movie->MC, movie->movie);
	return (AR_E_NONE);
	
#if 0
	MCGetCurrentTime(movie->MC,movie->movieTimeScale);
	TimeRecord trToSet;
	MCDoAction(movie->MC, mcActionGoToTime, (void *)trToSet);
#endif
}

AR_E_t arVideoQuickTimeMovieSetPlayPositionNextFrame(AR_VIDEO_QUICKTIME_MOVIE_t movie)
{
    TimeValue tv = GetMovieTime(movie->movie, NULL);
    //TimeValue playEveryFrameFrameDuration; // Can be used when writing another movie.
    const OSType mediaTypes[] = {VIDEO_TYPE};
    short flags = nextTimeMediaSample /*| (tv == 0 ? nextTimeEdgeOK : 0)*/; // If on first frame, include that as an "interesting time".
    
    GetMovieNextInterestingTime(movie->movie, flags, sizeof(mediaTypes)/sizeof(mediaTypes[0]), mediaTypes, tv, 0, &tv, /* &playEveryFrameFrameDuration */ NULL);
    if (tv == -1) {
        if (!movie->loop) return (AR_E_EOF);
        else tv = 0;
    }
    SetMovieTimeValue(movie->movie, tv);
    MCMovieChanged(movie->MC, movie->movie);
    
	return (AR_E_NONE);
}

int arVideoQuickTimeMovieIsPlayingEveryFrame(AR_VIDEO_QUICKTIME_MOVIE_t movie)
{
    if (!movie) return 0;
    return (movie->playEveryFrame);
}

unsigned char *arVideoQuickTimeMovieGetFrame(AR_VIDEO_QUICKTIME_MOVIE_t movie, double *time)
{
	unsigned char *pix = NULL;
	
	if (!movie) return (NULL);
    if (movie->status & AR_VIDEO_QUICKTIME_MOVIE_STATUS_BIT_READY) {
        if (movie->bufCopyFlag) {
            if (movie->status & AR_VIDEO_QUICKTIME_MOVIE_STATUS_BIT_BUFFER) {
                pix = movie->pixelsCopy2;
                movie->status &= ~AR_VIDEO_QUICKTIME_MOVIE_STATUS_BIT_BUFFER; // Clear buffer bit.
            } else {
                pix = movie->pixelsCopy1;
                movie->status |= AR_VIDEO_QUICKTIME_MOVIE_STATUS_BIT_BUFFER; // Set buffer bit.
            }
        } else {
            pix = movie->pixels;
        }
        movie->status &= ~AR_VIDEO_QUICKTIME_MOVIE_STATUS_BIT_READY; // Clear ready bit.
        if (time) *time = (double)movie->movieTime / (double)movie->movieTimeScale;
    }
	return (pix);
}

AR_E_t arVideoQuickTimeMovieGetFrameSize(AR_VIDEO_QUICKTIME_MOVIE_t movie, unsigned int *width, unsigned int *height, unsigned int *pixelSize, unsigned int *pixFormat)
{
	if (!movie) return (AR_E_INVALID_COMMAND);
	if (width) *width = movie->destWidth;
	if (height) *height = movie->destHeight;
	if (pixelSize) *pixelSize = movie->rowBytes / movie->destWidth;
	if (pixFormat) *pixFormat = movie->pixFormatEnum;
	return (AR_E_NONE);	
}

AR_E_t arVideoQuickTimeMovieStop(AR_VIDEO_QUICKTIME_MOVIE_t *movie_p)
{
	AR_E_t err_e = AR_E_NONE;
	OSErr err_ose;

	if (!movie_p) return (AR_E_INVALID_COMMAND);
	if (!(*movie_p)) return (AR_E_INVALID_COMMAND);
	
	// Use the movie controller instead of StopMovie();.
	if ((*movie_p)->movie && (*movie_p)->MC) {
		MCDoAction((*movie_p)->MC, mcActionPlay, (void*)IntToFixed(0));
	}

	if ((*movie_p)->offscreen) {
		// Clear the movie drawing complete proc.
		if ((*movie_p)->drawCompleteProc) {
			SetMovieDrawingCompleteProc((*movie_p)->movie, movieDrawingCallWhenChanged, NULL, 0L);
			DisposeMovieDrawingCompleteUPP((*movie_p)->drawCompleteProc);
			(*movie_p)->drawCompleteProc = NULL;
		}
	}	
	
	if ((*movie_p)->MC) {
		DisposeMovieController((*movie_p)->MC);
		(*movie_p)->MC = NULL;
	}

	if ((*movie_p)->offscreen) {
		if ((*movie_p)->movieGWorld) {
			UnlockPixels(GetGWorldPixMap((*movie_p)->movieGWorld));
			DisposeGWorld((*movie_p)->movieGWorld);
			(*movie_p)->movieGWorld = NULL;
		}
		if ((*movie_p)->bufCopyFlag) {
			free((*movie_p)->pixelsCopy2);
            (*movie_p)->pixelsCopy2 = NULL;
            free((*movie_p)->pixelsCopy1);
            (*movie_p)->pixelsCopy1 = NULL;
		}
		free((*movie_p)->pixels);
        (*movie_p)->pixels = NULL;
	} else {
		if ((*movie_p)->fullScreenWindowRef) {
#ifdef _WIN32
			SetWindowLong(GetPortNativeWindow((*movie_p)->fullScreenWindowRef), GWL_USERDATA, 0L);
			DestroyPortAssociation((CGrafPtr)(*movie_p)->fullScreenWindowRef);
#else
			SetWRefCon((*movie_p)->fullScreenWindowRef, 0L);
#endif // _WIN32
			if ((err_ose = EndFullScreen((*movie_p)->fullScreenRestoreState, 0L)) != noErr) {
				ARLOGe("Error (%d) exiting full-screen mode.\n", err_ose);
			}
			(*movie_p)->fullScreenWindowRef = NULL;
		}
	}
	
	if ((*movie_p)->movie) {
		// Opened a movie, so get rid of it.
		DisposeMovie((*movie_p)->movie);
		(*movie_p)->movie = NULL;
	}

	// Clear the slot in the global array.
	gMovies[(*movie_p)->index] = NULL;

	free(*movie_p);
	*movie_p = NULL;

	
	return (err_e);
}

void arVideoQuickTimeMovieFinal(void)
{
	if (!gInited) return;
	
	ExitMovies();
#ifdef _WIN32	
	TerminateQTML();
#endif // _WIN32	
	gInited = 0;
}

#endif // Mac OS, Mac OS X, Windows

#endif // AR_INPUT_QUICKTIME