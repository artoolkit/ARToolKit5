/*
 *  ARController.h
 *  ARToolKit5
 *
 *  A C++ class encapsulating core controller functionality of ARToolKit.
 *
 *  This file is part of ARToolKit.
 *
 *  ARToolKit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ARToolKit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb, Julian Looser.
 *
 */


#ifndef ARCONTROLLER_H
#define ARCONTROLLER_H

#include <ARWrapper/Platform.h>

#include <AR/ar.h>
#include <AR/arMulti.h>
#include <AR/video.h>

#include <ARWrapper/ARVideoSource.h>
#include <ARWrapper/ARMarker.h>
#include <ARWrapper/ARMarkerSquare.h>
#include <ARWrapper/ARMarkerMulti.h>
#if HAVE_NFT
#  include <AR2/tracking.h>
#  include <KPM/kpm.h>
#  include <ARWrapper/ARMarkerNFT.h>
#endif


#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#if !defined(_WINRT)
#  include <pthread.h>
#else
#  define pthread_mutex_t               CRITICAL_SECTION
#  define pthread_mutex_init(pm, a)     InitializeCriticalSectionEx(pm, 4000, CRITICAL_SECTION_NO_DEBUG_INFO)
#  define pthread_mutex_lock(pm)        EnterCriticalSection(pm)
#  define pthread_mutex_unlock(pm)      LeaveCriticalSection(pm)
#  define pthread_mutex_destroy(pm)     DeleteCriticalSection(pm)
#endif
#define PAGES_MAX 64

/**
 * Wrapper for ARToolKit functionality. This class handles ARToolKit initialisation, updates,
 * and cleanup. It maintains a collection of markers, providing methods to add and remove them.
 */
class ARController {

private:
#pragma mark Private types and instance variables
    // ------------------------------------------------------------------------------
    // Private types and instance variables.
    // ------------------------------------------------------------------------------

	typedef enum {
		NOTHING_INITIALISED,			///< No initialisation yet and no resources allocated.
		BASE_INITIALISED,				///< Marker management initialised, markers can be added.
		WAITING_FOR_VIDEO,				///< Waiting for video source to become ready.
		DETECTION_RUNNING				///< Video running, additional initialisation occurred, marker detection running.
	} ARToolKitState;
    
	ARToolKitState state;				///< Current state of operation, progress through initialisation
    bool stateWaitingMessageLogged;

	char* versionString;				///< ARToolKit version string

	ARVideoSource *m_videoSource0;      ///< VideoSource providing video frames for tracking
	ARVideoSource *m_videoSource1;      ///< VideoSource providing video frames for tracking
    pthread_mutex_t m_videoSourceLock;
    bool m_videoSourceIsStereo;
    AR2VideoTimestampT m_updateFrameStamp0;
    AR2VideoTimestampT m_updateFrameStamp1;
    
    // Virtual environment parameters.
	ARdouble m_projectionNearPlane;		///< Near plane distance for projection matrix calculation
	ARdouble m_projectionFarPlane;		///< Far plane distance for projection matrix calculation
	ARdouble m_projectionMatrix0[16];	///< OpenGL style projection matrix computed from camera parameters
	ARdouble m_projectionMatrix1[16];	///< OpenGL style projection matrix computed from camera parameters
	bool m_projectionMatrixSet;			///< True once the projection matrix has been computed, which requires an open video source
    
    // ARToolKit configuration. These allow for configuration prior to starting ARToolKit.
    int threshold;
    AR_LABELING_THRESH_MODE thresholdMode;
    int imageProcMode;
    int labelingMode;
    ARdouble pattRatio;
    int patternDetectionMode;
    AR_MATRIX_CODE_TYPE matrixCodeType;
    bool debugMode;

    std::vector<ARMarker *> markers;    ///< List of markers.

    bool doMarkerDetection;
    // ARToolKit data.
	ARHandle *m_arHandle0;				///< Structure containing general ARToolKit tracking information
	ARHandle *m_arHandle1;				///< Structure containing general ARToolKit tracking information
	ARPattHandle *m_arPattHandle;			///< Structure containing information about trained patterns
	AR3DHandle *m_ar3DHandle;		    ///< Structure used to compute 3D poses from tracking data
    ARdouble m_transL2R[3][4];
    AR3DStereoHandle *m_ar3DStereoHandle;
    
#if HAVE_NFT
    bool doNFTMarkerDetection;
    bool m_nftMultiMode;
    bool m_kpmRequired;
    bool m_kpmBusy;
    // NFT data.
    THREAD_HANDLE_T     *trackingThreadHandle;
    AR2HandleT          *m_ar2Handle;
    KpmHandle           *m_kpmHandle;
    AR2SurfaceSetT      *surfaceSet[PAGES_MAX]; // Weak-reference. Strong reference is now in ARMarkerNFT class.
#endif
    
    int m_error;
    void setError(int error);
    
#pragma mark Private methods.
    // ------------------------------------------------------------------------------
    // Private methods.
    // ------------------------------------------------------------------------------

    //
    // Internal marker management.
    //
    
	/**
	 * Adds a marker to the collection.
	 * @param marker	The marker to add
	 * @return			true if the marker was added successfully, otherwise false
	 */
	bool addMarker(ARMarker* marker);

	/**
	 * Removes the specified marker.
	 * @param marker		The marker to remove
	 * @return				true if the marker was removed, false if an error occurred.
	 */
	bool removeMarker(ARMarker* marker);
	
    //
    // Convenience initialisers.
    //
    
    bool initAR(void);
#if HAVE_NFT
    bool unloadNFTData(void);
    bool loadNFTData(void);
    bool initNFT(void);
#endif

public:
#pragma mark Public API
    // ------------------------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------------------------
    
    /**
	 * Constructor.
	 */
	ARController();
    
	/**
	 * Destructor.
	 */
	~ARController();
	
	static PFN_LOGCALLBACK logCallback;		///< Callback where log messages are passed to

private:
    static void logvBuf(va_list args, const char* format, char **bufPtr, int* lenPtr);
    static void logvWriteBuf(char* buf, int len, const int logLevel);

public:
    static void logv(const int logLevel, const char* format, ...);

	/**
	 * If a log callback has been set, then this passes the formatted message to it.
     * If no log callback has been set, then the message is discarded.
	 * @param msg		The message to output. Follows the same formatting rules as printf().
	 */
	static void logv(const char* msg, ...);

	/**
	 * Returns a string containing the ARToolKit version, such as "4.5.1".
	 * @return		The ARToolKit version
	 */
	const char* getARToolKitVersion();
    
    int getError();
    
	/** 
	 * Start marker management so markers can be added and removed.
     * @param patternSize For any square template (pattern) markers, the number of rows and columns in the template.
     * @param patternCountMax For any square template (pattern) markers, the maximum number of markers that may be loaded for a single matching pass. Must be > 0.
     * @return       true if initialisation was OK, false if an error occured.
	 */
	bool initialiseBase(const int patternSize = AR_PATT_SIZE1, const int patternCountMax = AR_PATT_NUM_MAX);

	/**
	 * Report whether a marker can be added. Markers can be added once basic
	 * initialisation has occurred.
	 * @return       true if adding a marker is currently possible
	 */
	bool canAddMarker();
    
    void setProjectionNearPlane(const ARdouble projectionNearPlane);
    void setProjectionFarPlane(const ARdouble projectionFarPlane);
    ARdouble projectionNearPlane(void);
    ARdouble projectionFarPlane(void);

	/**
	 * Start video capture and marker detection. (AR/NFT initialisation will begin on a subsequent call to update().)
	 * @param vconf			Video configuration string.
	 * @param cparaName		Camera parameters filename, or NULL if camera parameters file not being used.
	 * @param cparaBuff		A byte-buffer containing contents of a camera parameters file, or NULL if a camera parameters file is being used.
	 * @param cparaBuffLen	Length (in bytes) of cparaBuffLen, or 0 if a camera parameters file is being used.
	 * @return				true if video capture and marker detection was started, otherwise false.
	 */
	bool startRunning(const char* vconf, const char* cparaName, const char* cparaBuff, const long cparaBuffLen);
	
	/**
	 * Start stereo video capture and marker detection. (AR/NFT initialisation will begin on a subsequent call to update().)
	 * @param vconfL		Video configuration string for the "left" video source.
	 * @param cparaNameL	Camera parameters filename for the "left" video source, or NULL if camera parameters file not being used.
	 * @param cparaBuffL	A byte-buffer containing contents of a camera parameters file for the "left" video source, or NULL if a camera parameters file is being used.
	 * @param cparaBuffLenL	Length (in bytes) of cparaBuffLenL, or 0 if a camera parameters file is being used.
	 * @param vconfR		Video configuration string for the "right" video source.
	 * @param cparaNameR	Camera parameters filename for the "right" video source, or NULL if camera parameters file not being used.
	 * @param cparaBuffR	A byte-buffer containing contents of a camera parameters file for the "right" video source, or NULL if a camera parameters file is being used.
	 * @param cparaBuffLenR	Length (in bytes) of cparaBuffLenR, or 0 if a camera parameters file is being used.
     * @param transL2RName	Stereo calibration filename, or NULL if stereo calibration file not being used.
     * @param transL2RBuff	A byte-buffer containing contents of a stereo calibration file, or NULL if a stereo calibration file is being used.
     * @param transL2RBuffLen Length (in bytes) of transL2RBuff, or 0 if a stereo calibration file is being used.
	 * @return				true if video capture and marker detection was started, otherwise false.
	 */
	bool startRunningStereo(const char* vconfL, const char* cparaNameL, const char* cparaBuffL, const long cparaBuffLenL,
                            const char* vconfR, const char* cparaNameR, const char* cparaBuffR, const long cparaBuffLenR,
                            const char* transL2RName, const char* transL2RBuff, const long transL2RBuffLen);

#if TARGET_PLATFORM_ANDROID
    jint androidVideoPushInit(JNIEnv *env, jobject obj, jint videoSourceIndex, jint width, jint height, const char *pixelFormat, jint camera_index, jint camera_face);
    jint androidVideoPush1(JNIEnv *env, jobject obj, jint videoSourceIndex, jbyteArray buf, jint bufSize);
    jint androidVideoPush2(JNIEnv *env, jobject obj, jint videoSourceIndex,
                           jobject buf0, jint buf0PixelStride, jint buf0RowStride,
                           jobject buf1, jint buf1PixelStride, jint buf1RowStride,
                           jobject buf2, jint buf2PixelStride, jint buf2RowStride,
                           jobject buf3, jint buf3PixelStride, jint buf3RowStride);
    jint androidVideoPushFinal(JNIEnv *env, jobject obj, jint videoSourceIndex);
#endif
    
	/**
	 * Reports width, height and pixel format of a video source.
     * To retrieve the size (in bytes) of each pixel, use arUtilGetPixelSize(*pixelFormat);
     * To get a C-string with the name of the pixel format, use arUtilGetPixelFormatName(*pixelFormat);
	 * @param videoSourceIndex Index into an array of video sources, specifying which source should be queried.
     * @param width Pointer to an int which will be filled with the width (in pixels) of the video frame, or NULL if this information is not required.
     * @param height Pointer to an int which will be filled with the height (in pixels) of the video frame, or NULL if this information is not required.
     * @param pixelFormat Pointer to an AR_PIXEL_FORMAT which will be filled with the pixel format of the video frame, or NULL if this information is not required.
	 * @return		true if the video source(s) is/are open and returning frames, otherwise false.
	 */
    bool videoParameters(const int videoSourceIndex, int *width, int *height, AR_PIXEL_FORMAT *pixelFormat);
    
	/**
	 * Returns true if video capture and marker detection is running.
	 * @return		true if the video source(s) is/are open and returning frames, otherwise false.
	 */
	bool isRunning();
    
    /**
	 * Video capture and marker detection stops, but markers are still valid and can be configured.
	 * @return				true if video capture and marker detection was stopped, otherwise false.
	 */
	bool stopRunning();

	/**
	 * Stop, if running. Remove all markers, clean up all memory.
     * Starting again from this state requires initialiseBase() to be called again.
	 * @return				true if shutdown was successful, otherwise false
	 */
	bool shutdown();

	/**
	 * Populates the provided array with the ARToolKit projection matrix. The projection matrix is computed
	 * once the video source has been opened, and camera parameters become available. If this method is called
	 * before this happens, then the passed array is untouched and the method will return false.
	 * @param videoSourceIndex Index into an array of video sources, specifying which source should be queried.
	 * @param proj		Array to populate with OpenGL compatible projection matrix
	 * @return			true if the projection matrix has been computed, otherwise false
	 */
	bool getProjectionMatrix(const int videoSourceIndex, ARdouble proj[16]);
    
	/**
	 * Adds a marker as specified in the given configuration string. The format of the string can be 
	 * one of:
	 *
     * - Single marker:		"single;pattern_file;pattern_width", e.g. "single;data/hiro.patt;80"
     * - Multi marker:		"multi;config_file", e.g. "multi;data/multi/marker.dat"
     * - NFT marker:        "nft;nft_dataset_pathname", e.g. "nft;gibraltar"
     *
	 * @param cfg		The configuration string
	 * @return			The UID of the marker instantiated based on the configuration string, or -1 if an error occurred.
	 */
	int addMarker(const char* cfg);

	/**
	 * Removes the marker with the given ID.
	 * @param UID			The UID of the marker to remove
	 * @return				true if the marker was removed, false if an error occurred.
	 */
	bool removeMarker(int UID);
	
	/**
	 * Clears the collection of markers.
	 * @return				The number of markers removed
	 */
	int removeAllMarkers();

	/**
	 * Returns the number of currently loaded markers.
	 * @return				The number of currently loaded markers.
	 */
	unsigned int countMarkers();

	/**
	 * Searches the collection of markers for the given ID.
	 * @param UID			The UID of the marker to find
	 * @return				The found marker, or NULL if no matching ID was found.
	 */
	ARMarker* findMarker(int UID);
	
    bool capture();
    
    /**
     * Asks the video source to push the most recent frame into the passed-in buffer.
     * @param videoSourceIndex Index into an array of video sources, specifying which source should
     *      be queried.
     * @param buffer Pointer to a buffer of pixels (of type 'uint32_t') to be filled. It is the
     *      caller's responsibility to ensure that the buffer is of sufficient size. The pixels are
     *      RGBA in little-endian systems, or ABGR in big-endian systems.
     */
    bool updateTexture32(const int videoSourceIndex, uint32_t *buffer);
    
	/**
	 * Performs marker detection and updates all markers. The latest frame from the current 
	 * video source is retrieved and analysed. Each marker in the collection is updated with
	 * new tracking information. The marker info array is 
	 * iterated over, and detected markers are matched up with those in the marker collection. Each matched 
	 * marker is updated with visibility and transformation information. Any markers not detected are considered 
	 * not currently visible.
     *
	 * @return				true if update completed successfully, false if an error occurred
	 */
	bool update();

	/**
	 * Enables or disables debug mode in the tracker. When enabled, a black and white debug
	 * image is generated during marker detection. The debug image is useful for visualising
	 * the binarization process and choosing a threshold value.
	 * @param	debug		true to enable debug mode, false to disable debug mode
	 * @see					getDebugMode()
	 */
	void setDebugMode(bool debug);
    
	/**
	 * Returns whether debug mode is currently enabled.
	 * @return				true when debug mode is enabled, false when debug mode is disabled
	 * @see					setDebugMode()
	 */
	bool getDebugMode() const;
    
    void setImageProcMode(int mode);
    
    int getImageProcMode() const;
    
	/**
	 * Sets the threshold value used for image binarization.
	 * @param	thresh	The new threshold value to use
	 * @see					getThreshold()
	 */
	void setThreshold(int thresh);

	/**
	 * Returns the current threshold value used for image binarization.
	 * @return				The current threshold value
	 * @see					setThreshold()
	 */
	int getThreshold() const;
    
	/**
	 * Sets the thresholding mode to use.
	 * @param mode			The new thresholding mode to use.
	 * @see					getThresholdMode()
	 */
	void setThresholdMode(int mode);

	/**
	 * Returns the current thresholding mode.
	 * @return				The current thresholding mode.
	 * @see					setThresholdMode()
	 */
	int getThresholdMode() const;
	
	/**
	 * Sets the labeling mode to use.
	 * @param mode			The new labeling mode to use.
	 * @see					getLabelingMode()
	 */
	void setLabelingMode(int mode);
    
	/**
	 * Returns the current labeling mode.
	 * @return				The current labeling mode.
	 * @see					setLabelingMode()
	 */
	int getLabelingMode() const;
	
    void setPatternDetectionMode(int mode);
    
    int getPatternDetectionMode() const;

    void setPattRatio(float ratio);
    
    float getPattRatio() const;
    
    void setMatrixCodeType(int type);
    
    int getMatrixCodeType() const;
    
    void setNFTMultiMode(bool on);
    
    bool getNFTMultiMode() const;

    /**
     * Populates the provided buffer with the current contents of the debug image.
     * @param videoSourceIndex Index into an array of video sources, specifying which source should
     *      be queried.
     * @param buffer Pointer to a buffer of pixels (of type 'uint32_t') to be filled. It is the
     *      caller's responsibility to ensure that the buffer is of sufficient size. The pixels are
     *      RGBA in little-endian systems, or ABGR in big-endian systems.
     */
    bool updateDebugTexture32(const int videoSourceIndex, uint32_t* buffer);

	/**
	 * Populates the provided color buffer with the image for the specified pattern.
	 * @param	patternID	The ARToolKit pattern ID to use
	 * @param	buffer		The color buffer to populate
	 * @return				true if successful, false if an error occurred
	 */
	bool getPatternImage(int patternID, uint32_t* buffer);
    
	/**
	 * Loads an optical parameters structure from file or from buffer.
     *
     * @param optical_param_name If supplied, points to a buffer specifying the path
     *      to the optical parameters file (as generated by the calib_optical utility.)
     * @param optical_param_buff If optical_param_name is NULL, the contents of this
     *      buffer will be interpreted as containing the contents of an optical
     *      parameters file.
     * @param optical_param_buffLen Length of the buffer specified in optical_param_buff.
     *      Ignored if optical_param_buff is NULL.
     * @param fovy_p Pointer to an ARdouble, which will be filled with the
     *      field-of-view (Y axis) component of the optical parameters.
     * @param aspect_p Pointer to an ARdouble, which will be filled with the
     *      aspect ratio (width / height) component of the optical parameters.
     * @param m Pointer to an array of 16 ARdoubles, which will be filled with the
     *      transformation matrix component of the optical parameters. Note that the
     *      position vector (m[12], m[13], m[14]) will be scaled by the viewScale
     *      value supplued in startRunning().
     * @param p (Optional) May be NULL, or a pointer to an array of 16 ARdoubles,
     *      which will be filled with the perspective matrix calculated from fovy and aspect
     *      combined with the near and far projection values supplied in startRunning().
     */
    bool loadOpticalParams(const char *optical_param_name, const char *optical_param_buff, const long optical_param_buffLen, ARdouble *fovy_p, ARdouble *aspect_p, ARdouble m[16], ARdouble p[16]);
    
};


#endif // !ARCONTROLLER_H
