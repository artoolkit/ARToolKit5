/*
 *  ARToolKitWrapperExportedAPI.h
 *  ARToolKit5
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

#ifndef ARTOOLKITWRAPPEREXPORTEDAPI_H
#define ARTOOLKITWRAPPEREXPORTEDAPI_H

#include <ARWrapper/Platform.h>
#include <ARWrapper/Image.h>
#include <ARWrapper/Error.h>

/**
 * \file ARToolKitWrapperExportedAPI.h
 * Defines functions that provide a C-compatible API. These functions are accessible to 
 * other C applications, as well as languages like C#.
 */
extern "C" {

	/**
	 * Registers a callback function to use when a message is logged.
     * If the callback is to become invalid, be sure to call this function with NULL
     * first so that the callback is unregistered.
	 */
	EXPORT_API void arwRegisterLogCallback(PFN_LOGCALLBACK callback);

    EXPORT_API void arwSetLogLevel(const int logLevel);
    
    // ----------------------------------------------------------------------------------------------------
#pragma mark  ARToolKit lifecycle functions
    // ----------------------------------------------------------------------------------------------------
	/**
	 * Initialises the ARToolKit.
     * For any square template (pattern) markers, the number of rows and columns in the template defaults to AR_PATT_SIZE1 and the maximum number of markers that may be loaded for a single matching pass defaults to AR_PATT_NUM_MAX.
	 * @return			true if successful, false if an error occurred
	 * @see				arwShutdownAR()
	 */
	EXPORT_API bool arwInitialiseAR();

    /**
     * Initialises the ARToolKit with non-default options for size and number of square markers.
     * @param pattSize For any square template (pattern) markers, the number of rows and columns in the template. May not be less than 16 or more than AR_PATT_SIZE1_MAX. Pass AR_PATT_SIZE1 for the same behaviour as arwInitialiseAR().
     * @param pattCountMax For any square template (pattern) markers, the maximum number of markers that may be loaded for a single matching pass. Must be > 0. Pass AR_PATT_NUM_MAX for the same behaviour as arwInitialiseAR().
     * @return			true if successful, false if an error occurred
     * @see				arwShutdownAR()
     */
    EXPORT_API bool arwInitialiseARWithOptions(const int pattSize, const int pattCountMax);
    
	/**
	 * Gets the ARToolKit version as a string, such as "4.5.1".
	 * Must not be called prior to arwInitialiseAR().
	 * @param buffer	The character buffer to populate
	 * @param length	The maximum number of characters to set in buffer
	 * @return			true if successful, false if an error occurred
	 */
	EXPORT_API bool arwGetARToolKitVersion(char *buffer, int length);

    /**
     * Return error information
     * Returns the value of the error flag.  Each detectable error
     * is assigned a numeric code and symbolic name.  When  an  error  occurs,
     * the  error  flag  is set to the appropriate error code value.  No other
     * errors are recorded until arwGetError  is  called,  the  error  code  is
     * returned,  and  the  flag  is  reset  to  AR_ERROR_NONE.   If  a  call to
     * arwGetError returns AR_ERROR_NONE, there  has  been  no  detectable  error
     * since the last call to arwGetError, or since the the library was initialized.
     *
     * To  allow  for  distributed implementations, there may be several error
     * flags.  If any single error flag has recorded an error,  the  value  of
     * that  flag  is  returned  and  that  flag  is reset to AR_ERROR_NONE when
     * arwGetError is called.  If more than one flag  has  recorded  an  error,
     * arwGetError  returns  and  clears  an arbitrary error flag value.  Thus,
     * arwGetError should  always  be  called  in  a  loop,  until  it  returns
     * AR_ERROR_NONE, if all error flags are to be reset.
     *
     * Initially, all error flags are set to AR_ERROR_NONE.
     * @return			enum with error code.
     */
    EXPORT_API int arwGetError();
    
	/**
	 * Changes the working directory to the resources directory used by ARToolKit.
     * Normally, this would be called immediately after arwInitialiseAR()
	 * @return			true if successful, false if an error occurred
	 * @see				arwInitialiseAR()
	 */
	EXPORT_API bool arwChangeToResourcesDir(const char *resourcesDirectoryPath);
    
	/**
	 * Initialises and starts video capture.
	 * @param vconf		The video configuration string
	 * @param cparaName	The camera parameter file, which is used to form the projection matrix
	 * @param nearPlane	The distance to the near plane of the viewing frustum formed by the camera parameters.
	 * @param farPlane	The distance to the far plane of the viewing frustum formed by the camera parameters.
	 * @return			true if successful, false if an error occurred
	 * @see				arwStopRunning()
	 */
	EXPORT_API bool arwStartRunning(const char *vconf, const char *cparaName, const float nearPlane, const float farPlane);
	
	/**
	 * Initialises and starts video capture.
	 * @param vconf		The video configuration string
	 * @param cparaBuff	A string containing the contents of a camera parameter file, which is used to form the projection matrix.
	 * @param cparaBuffLen	Number of characters in cparaBuff.
	 * @param nearPlane	The distance to the near plane of the viewing frustum formed by the camera parameters.
	 * @param farPlane	The distance to the far plane of the viewing frustum formed by the camera parameters.
	 * @return			true if successful, false if an error occurred
	 * @see				arwStopRunning()
	 */
	EXPORT_API bool arwStartRunningB(const char *vconf, const char *cparaBuff, const int cparaBuffLen, const float nearPlane, const float farPlane);

    EXPORT_API bool arwStartRunningStereo(const char *vconfL, const char *cparaNameL, const char *vconfR, const char *cparaNameR, const char *transL2RName, const float nearPlane, const float farPlane);
    
    EXPORT_API bool arwStartRunningStereoB(const char *vconfL, const char *cparaBuffL, const int cparaBuffLenL, const char *vconfR, const char *cparaBuffR, int cparaBuffLenR, const char *transL2RBuff, const int transL2RBuffLen, const float nearPlane, const float farPlane);

	/**
	 * Returns true if ARToolKit is running, i.e. detecting markers.
	 * @return			true when running, otherwise false
	 */
	EXPORT_API bool arwIsRunning();

	/**
	 * Stops video capture and frees video capture resources.
	 * @return			true if successful, false if an error occurred
	 * @see				arwStartRunning()
	 */
	EXPORT_API bool arwStopRunning();

	/**
	 * Shuts down the ARToolKit and frees all resources.
     * N.B.: If this is being called from the destructor of the same module which
     * supplied the log callback, be sure to set the logCallback = NULL
     * prior to calling this function.
	 * @return			true if successful, false if an error occurred
	 * @see				arwInitialiseAR()
	 */
	EXPORT_API bool arwShutdownAR();
    
    // ----------------------------------------------------------------------------------------------------
#pragma mark  Video stream management
    // ----------------------------------------------------------------------------------------------------

	/**
	 * Populates the given float array with the projection matrix computed from camera parameters for the video source.
	 * @param p         Float array to populate with OpenGL compatible projection matrix.
	 * @return          true if successful, false if an error occurred
	 */
	EXPORT_API bool arwGetProjectionMatrix(float p[16]);
    
	/**
	 * Populates the given float arrays with the projection matrices computed from camera parameters for each of the stereo video sources.
	 * @param pL        Float array to populate with OpenGL compatible projection matrix for the left camera of the stereo video pair.
	 * @param pR        Float array to populate with OpenGL compatible projection matrix for the right camera of the stereo video pair.
	 * @return          true if successful, false if an error occurred
	 */
	EXPORT_API bool arwGetProjectionMatrixStereo(float pL[16], float pR[16]);
	
	/**
	 * Returns the parameters of the video source frame.
     * @param width Pointer to an int which will be filled with the width (in pixels) of the video frame, or NULL if this information is not required.
     * @param height Pointer to an int which will be filled with the height (in pixels) of the video frame, or NULL if this information is not required.
     * @param pixelSize Pointer to an int which will be filled with the numbers of bytes per pixel of the source frame.
     * @param pixelFormatStringBuffer Pointer to a buffer which will be filled with the symolic name of the pixel format (as a nul-terminated C-string) of the video frame, or NULL if this information is not required. The name will be of the form "AR_PIXEL_FORMAT_xxx".
     * @param pixelFormatStringBufferLen Length (in bytes) of pixelFormatStringBuffer, or 0 if this information is not required.
	 * @return			True if the values were returned OK, false if there is currently no video source or an error int[] .
	 * @see				arwGetVideoParamsStereo
	 */
	EXPORT_API bool arwGetVideoParams(int *width, int *height, int *pixelSize, char *pixelFormatStringBuffer, int pixelFormatStringBufferLen);
    
	/**
	 * Returns the parameters of the video source frames.
     * @param widthL Pointer to an int which will be filled with the width (in pixels) of the video frame, or NULL if this information is not required.
     * @param widthR Pointer to an int which will be filled with the width (in pixels) of the video frame, or NULL if this information is not required.
     * @param heightL Pointer to an int which will be filled with the height (in pixels) of the video frame, or NULL if this information is not required.
     * @param heightR Pointer to an int which will be filled with the height (in pixels) of the video frame, or NULL if this information is not required.
     * @param pixelSizeL Pointer to an int which will be filled with the numbers of bytes per pixel of the source frame, or NULL if this information is not required.
     * @param pixelSizeR Pointer to an int which will be filled with the numbers of bytes per pixel of the source frame, or NULL if this information is not required.
     * @param pixelFormatStringBufferL Pointer to a buffer which will be filled with the symbolic name of the pixel format (as a nul-terminated C-string) of the video frame, or NULL if this information is not required. The name will be of the form "AR_PIXEL_FORMAT_xxx".
     * @param pixelFormatStringBufferR Pointer to a buffer which will be filled with the symbolic name of the pixel format (as a nul-terminated C-string) of the video frame, or NULL if this information is not required. The name will be of the form "AR_PIXEL_FORMAT_xxx".
     * @param pixelFormatStringBufferLenL Length (in bytes) of pixelFormatStringBufferL, or 0 if this information is not required.
     * @param pixelFormatStringBufferLenR Length (in bytes) of pixelFormatStringBufferR, or 0 if this information is not required.
	 * @return			True if the values were returned OK, false if there is currently no stereo video source or an error int[] .
	 * @see				arwGetVideoParams
	 */
	EXPORT_API bool arwGetVideoParamsStereo(int *widthL, int *heightL, int *pixelSizeL, char *pixelFormatStringBufferL, int pixelFormatStringBufferLenL, int *widthR, int *heightR, int *pixelSizeR, char *pixelFormatStringBufferR, int pixelFormatStringBufferLenR);
    
	/**
	 * Captures a newest frame from the video source.
	 * @return			true if successful, false if an error occurred
	 */
	EXPORT_API bool arwCapture();
	
	/**
	 * Performs detection and marker updates. The newest frame from the video source is retrieved and
	 * analysed. All loaded markers are updated.
	 * @return			true if successful, false if an error occurred
	 */
    EXPORT_API bool arwUpdateAR();
    
	/**
	 * Populates the provided floating-point color buffer with the current video frame.
	 * @param buffer	The color buffer to fill with video
	 * @return			true if successful, false if an error occurred
	 */
	EXPORT_API bool arwUpdateTexture(Color *buffer);
    
    EXPORT_API bool arwUpdateTexture32(unsigned int *buffer);
    
	EXPORT_API bool arwUpdateTextureStereo(Color *bufferL, Color *bufferR);
    
    EXPORT_API bool arwUpdateTexture32Stereo(unsigned int *bufferL, unsigned int *bufferR);
    
	/**
	 * Enables or disables debug mode in the tracker. When enabled, a black and white debug
	 * image is generated during marker detection. The debug image is useful for visualising
	 * the binarization process and choosing a threshold value.
	 * @param debug		true to enable debug mode, false to disable debug mode
	 * @see				arwGetVideoDebugMode()
	 */
	EXPORT_API void arwSetVideoDebugMode(bool debug);
    
	/**
	 * Returns whether debug mode is currently enabled.
	 * @return			true when debug mode is enabled, false when debug mode is disabled
	 * @see				arwSetVideoDebugMode()
	 */
	EXPORT_API bool arwGetVideoDebugMode();
    
    /**
     * Populates the provided color buffer with the current contents of the debug image.
     * @param buffer Pointer to a buffer of pixels (of type 'Color') to be filled. It is the caller's responsibility to ensure that the buffer is of sufficient size.
     * @return				true if successful, false if an error occurred
     */
    EXPORT_API bool arwUpdateDebugTexture(Color *buffer);
    
    
    /**
     * Populates the provided buffer with the current contents of the debug image.
     * @param buffer Pointer to a buffer of pixels (of type 'uint32_t') to be filled. It is the
     *      caller's responsibility to ensure that the buffer is of sufficient size. The pixels are
     *      RGBA in little-endian systems, or ABGR in big-endian systems.
     */
    EXPORT_API bool arwUpdateDebugTexture32(unsigned int *buffer);
    
#if !TARGET_PLATFORM_WINRT
	/**
	 * Uses OpenGL to directly updated the specified texture with the current video frame.
	 * @param textureID	The OpenGL texture ID to upload texture data to
	 * @return			true if successful, false if an error occurred
	 */
	EXPORT_API bool arwUpdateTextureGL(const int textureID);
    
 	EXPORT_API bool arwUpdateTextureGLStereo(const int textureID_L, const int textureID_R);
#endif // !TARGET_PLATFORM_WINRT

    // ----------------------------------------------------------------------------------------------------
#pragma mark  Unity-specific API
    // ----------------------------------------------------------------------------------------------------
    
    enum {
        ARW_UNITY_RENDER_EVENTID_NOP = 0, // No operation (does nothing).
#if !TARGET_PLATFORM_WINRT
        ARW_UNITY_RENDER_EVENTID_UPDATE_TEXTURE_GL = 1,
        ARW_UNITY_RENDER_EVENTID_UPDATE_TEXTURE_GL_STEREO = 2,
#endif // !TARGET_PLATFORM_WINRT
    };
    
    /**
       When ARWrapper is loaded as a plugin into the Unity 3D environment, this function will be
       called for Unity GL.IssuePluginEvent script calls.
       The function will be called on a rendering thread; note that when multithreaded rendering is used,
       the rendering thread WILL BE DIFFERENT from the thread that all scripts & other game logic happens.
       It is up to the user to ensure any synchronization with other plugin script calls.
     */
    EXPORT_API void UnityRenderEvent(int eventID);

#if !TARGET_PLATFORM_WINRT
	/**
	 * Uses OpenGL to directly updated the specified texture with the current video frame.
	 * @param textureID	The OpenGL texture ID to upload texture data to
	 * @return			true if successful, false if an error occurred
	 */
    EXPORT_API void arwSetUnityRenderEventUpdateTextureGLTextureID(int textureID);
    
    EXPORT_API void arwSetUnityRenderEventUpdateTextureGLStereoTextureIDs(int textureID_L, int textureID_R);
#endif // !TARGET_PLATFORM_WINRT

	// ----------------------------------------------------------------------------------------------------
#pragma mark  Tracking configuration
    // ----------------------------------------------------------------------------------------------------
	/**
	 * Sets the threshold value used for image binarization.
	 * @param	threshold	The new threshold value to use
	 * @see					arwGetVideoThreshold()
	 */
	EXPORT_API void arwSetVideoThreshold(int threshold);

	/**
	 * Returns the current threshold value used for image binarization.
	 * @return			The current threshold value
	 * @see				arwSetVideoThreshold()
	 */
	EXPORT_API int arwGetVideoThreshold();

	/**
	 * Sets the current threshold mode used for image binarization.
	 * @param mode		The new threshold mode
	 * @see				arwSetVideoThresholdMode()
	 */
	EXPORT_API void arwSetVideoThresholdMode(int mode);

	/**
	 * Gets the current threshold mode used for image binarization.
	 * @return			The current threshold mode
	 * @see				arwGetVideoThresholdMode()
	 */
	EXPORT_API int arwGetVideoThresholdMode();

    EXPORT_API void arwSetLabelingMode(int mode);
    
    EXPORT_API int arwGetLabelingMode();
    
    EXPORT_API void arwSetPatternDetectionMode(int mode);
    
    EXPORT_API int arwGetPatternDetectionMode();
    
    EXPORT_API void arwSetBorderSize(float size);
    
    EXPORT_API float arwGetBorderSize();
    
    EXPORT_API void arwSetMatrixCodeType(int type);
    
    EXPORT_API int arwGetMatrixCodeType();

    EXPORT_API void arwSetImageProcMode(int mode);
    
    EXPORT_API int arwGetImageProcMode();

    EXPORT_API void arwSetNFTMultiMode(bool on);
    
    EXPORT_API bool arwGetNFTMultiMode();

    // ----------------------------------------------------------------------------------------------------
#pragma mark  Marker management
    // ----------------------------------------------------------------------------------------------------
    /**
	 * Adds a marker as specified in the given configuration string. The format of the string can be 
	 * one of:
     * - Single marker:		"single;pattern_file;pattern_width", e.g. "single;data/hiro.patt;80"
     * - Multi marker:		"multi;config_file", e.g. "multi;data/multi/marker.dat"
     * - NFT marker:        "nft;nft_dataset_pathname", e.g. "nft;gibraltar"
	 * @param cfg		The configuration string
	 * @return			The unique identifier (UID) of the marker instantiated based on the configuration string, or -1 if an error occurred
	 */
	EXPORT_API int arwAddMarker(const char *cfg);
    
    /**
	 * Removes the marker with the given unique identifier (UID).
	 * @param markerUID	The unique identifier (UID) of the marker to remove
	 * @return			true if the marker was removed, false if an error occurred
	 */
	EXPORT_API bool arwRemoveMarker(int markerUID);
	
	/**
	 * Clears the collection of markers.
	 * @return			The number of markers removed
	 */
	EXPORT_API int arwRemoveAllMarkers();
    
	/**
	 * Returns the visibility status of the specified marker. After a call to arwUpdate, all marker 
	 * information will be current. Any marker can then be checked for visibility in the current frame.
	 * @param markerUID	The unique identifier (UID) of the marker to query
	 * @return			true if the specified marker is visible, false if not, or an error occurred
	 */
	EXPORT_API bool arwQueryMarkerVisibility(int markerUID);
    
	/**
	 * Populates the provided float array with the current transformation for the specified marker. After 
	 * a call to arwUpdate, all marker information will be current. Marker transformations can then be 
	 * checked. If the specified marker is not found the last good transformation is used, and false is 
	 * returned.
	 * @param markerUID	The unique identifier (UID) of the marker to query
	 * @param matrix	The float array to populate with an OpenGL compatible transformation matrix
	 * @return			true if the specified marker is visible, false if not, or an error occurred
	 */
	EXPORT_API bool arwQueryMarkerTransformation(int markerUID, float matrix[16]);
	
	/**
	 * Populates the provided float arrays with the current transformations for the specified marker. After
	 * a call to arwUpdate, all marker information will be current. Marker transformations can then be
	 * checked. If the specified marker is not found the last good transformation is used, and false is
	 * returned.
	 * @param markerUID	The unique identifier (UID) of the marker to query
	 * @param matrixL	The float array to populate with an OpenGL compatible transformation matrix for the left camera.
	 * @param matrixR	The float array to populate with an OpenGL compatible transformation matrix for the right camera.
	 * @return			true if the specified marker is visible, false if not, or an error occurred
	 */
	EXPORT_API bool arwQueryMarkerTransformationStereo(int markerUID, float matrixL[16], float matrixR[16]);
	
	/**
	 * Returns the number of pattern images associated with the specified marker. A single marker has one pattern
	 * image. A multimarker has one or more pattern images.
     * Images of NFT markers are not currently supported, so at present this function will return 0 for NFT markers.
	 * @param markerUID	The unique identifier (UID) of the marker
	 * @return			The number of pattern images
	 */
	EXPORT_API int arwGetMarkerPatternCount(int markerUID);
	
	/**
	 * Gets configuration of a pattern associated with a marker.
	 * @param markerUID	The unique identifier (UID) of the marker
	 * @param patternID	The id of the pattern within the marker, in range from 0 to arwGetMarkerPatternCount() - 1, inclusive. Ignored for single markers and NFT markers (i.e. 0 assumed).
	 * @param matrix	The float array to populate with the 4x4 transformation matrix of the pattern (column-major order).
	 * @param width		Float value to set to the width of the pattern
	 * @param height	Float value to set to the height of the pattern.
	 * @param imageSizeX Int value to set to the width of the pattern image (in pixels).
	 * @param imageSizeY Int value to set to the height of the pattern image (in pixels).
	 * @return			true if successful, false if an error occurred
	 */
	EXPORT_API bool arwGetMarkerPatternConfig(int markerUID, int patternID, float matrix[16], float *width, float *height, int *imageSizeX, int *imageSizeY);
	
	/**
	 * Gets a pattern image associated with a marker. The provided color buffer is populated with the image of the 
	 * pattern for the specified marker. If the marker is a multimarker, then the pattern image specified by patternID is 
	 * used.
     * Images of NFT markers are not currently supported, so at present this function will return no image for NFT markers.
	 * @param markerUID	The unique identifier (UID) of the marker
	 * @param patternID	The id for the pattern within that marker. Ignored for single markers and NFT markers.
	 * @param buffer	Color array to populate with pattern image. Use arwGetMarkerPatternConfig to get the required size of this array (imageSizeX * imageSizeY elements).
	 * @return			true if successful, false if an error occurred
	 */
	EXPORT_API bool arwGetMarkerPatternImage(int markerUID, int patternID, Color *buffer);
    
    /**
     * Constants for use with marker option setters/getters.
     */
    enum {
        ARW_MARKER_OPTION_FILTERED = 1,                         ///< bool, true for filtering enabled.
        ARW_MARKER_OPTION_FILTER_SAMPLE_RATE = 2,               ///< float, sample rate for filter calculations.
        ARW_MARKER_OPTION_FILTER_CUTOFF_FREQ = 3,               ///< float, cutoff frequency of filter.
        ARW_MARKER_OPTION_SQUARE_USE_CONT_POSE_ESTIMATION = 4,  ///< bool, true to use continuous pose estimate.
        ARW_MARKER_OPTION_SQUARE_CONFIDENCE = 5,                ///< float, confidence value of most recent marker match
        ARW_MARKER_OPTION_SQUARE_CONFIDENCE_CUTOFF = 6,         ///< float, minimum allowable confidence value used in marker matching.
        ARW_MARKER_OPTION_NFT_SCALE = 7,                        ///< float, scale factor applied to NFT marker size.
        ARW_MARKER_OPTION_MULTI_MIN_SUBMARKERS = 8,             ///< int, minimum number of submarkers for tracking to be valid.
        ARW_MARKER_OPTION_MULTI_MIN_CONF_MATRIX = 9,            ///< float, minimum confidence value for submarker matrix tracking to be valid.
        ARW_MARKER_OPTION_MULTI_MIN_CONF_PATTERN = 10,          ///< float, minimum confidence value for submarker pattern tracking to be valid.
    };
    
	/**
	 * Set boolean options associated with a marker.
	 * @param markerUID	The unique identifier (UID) of the marker
     * @param option Symbolic constant identifying marker option to set.
     * @param value The value to set it to.
     */
    EXPORT_API void arwSetMarkerOptionBool(int markerUID, int option, bool value);
    
	/**
	 * Set integer options associated with a marker.
	 * @param markerUID	The unique identifier (UID) of the marker
     * @param option Symbolic constant identifying marker option to set.
     * @param value The value to set it to.
     */
    EXPORT_API void arwSetMarkerOptionInt(int markerUID, int option, int value);
    
	/**
	 * Set floating-point options associated with a marker.
	 * @param markerUID	The unique identifier (UID) of the marker
     * @param option Symbolic constant identifying marker option to set.
     * @param value The value to set it to.
     */
    EXPORT_API void arwSetMarkerOptionFloat(int markerUID, int option, float value);

	/**
	 * Get boolean options associated with a marker.
	 * @param markerUID	The unique identifier (UID) of the marker
     * @param option Symbolic constant identifying marker option to get.
	 * @return true if option is set, false if option is not set or an error occurred.
     */
    EXPORT_API bool arwGetMarkerOptionBool(int markerUID, int option);
    
	/**
	 * Get integer options associated with a marker.
	 * @param markerUID	The unique identifier (UID) of the marker
     * @param option Symbolic constant identifying marker option to get.
	 * @return integer value of option, or INT_MIN if an error occurred.
     */
    EXPORT_API int arwGetMarkerOptionInt(int markerUID, int option);
    
	/**
	 * Get floating-point options associated with a marker.
	 * @param markerUID	The unique identifier (UID) of the marker
     * @param option Symbolic constant identifying marker option to get.
	 * @return floating-point value of option, or NAN if an error occurred.
     */
    EXPORT_API float arwGetMarkerOptionFloat(int markerUID, int option);
    
    // ----------------------------------------------------------------------------------------------------
#pragma mark  Utility
    // ----------------------------------------------------------------------------------------------------
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
     * @param fovy_p Pointer to a float, which will be filled with the
     *      field-of-view (Y axis) component of the optical parameters, in degrees.
     * @param aspect_p Pointer to a float, which will be filled with the
     *      aspect ratio (width / height) component of the optical parameters.
     * @param m Pointer to an array of 16 floats, which will be filled with the
     *      transformation matrix component of the optical parameters.
     * @param p (Optional) May be NULL, or a pointer to an array of 16 floats,
     *      which will be filled with the perspective matrix calculated from fovy and aspect
     *      combined with the near and far projection values supplied in arwStartRunning().
     */

    EXPORT_API bool arwLoadOpticalParams(const char *optical_param_name, const char *optical_param_buff, const int optical_param_buffLen, float *fovy_p, float *aspect_p, float m[16], float p[16]);
}

#endif // !ARTOOLKITWRAPPEREXPORTEDAPI_H
