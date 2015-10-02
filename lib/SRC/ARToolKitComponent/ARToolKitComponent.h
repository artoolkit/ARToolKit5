//
//  ARToolKitComponent.h
//  ARToolKit5
//
//  This file is part of ARToolKit.
//
//  ARToolKit is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ARToolKit is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
//  Copyright 2015 Daqri, LLC.
//  Copyright 2014-2015 ARToolworks, Inc.
//
//  Author(s): Philip Lamb
//

#pragma once

#include <string>

using namespace std;
using namespace Platform;
using namespace Windows::Foundation;


namespace ARToolKitComponent
{
	typedef float Color;		  // Actually passed in groups of 4 floats, with R lowest in memory, then G, then B, then A.
	typedef unsigned int Color32; // On a little-endian system, R occupies the lowest 8 bits, then G, then B, then A the highest 8 bits.
	typedef unsigned char Color8; // Actually passed in groups of 4 bytes, with R lowest in memory, then G, then B, then A.

	public enum class ArThresholdMode
	{
		Manual = 0,
		AutoMedian,
		AutoOtsu,
		AutoAdaptive
	};

	public enum class ArMarkerOption
	{
		Filtered = 1,                        ///< bool, true for filtering enabled.
		FilterSampleRate = 2,                ///< float, sample rate for filter calculations.
		FilterCutoffFreq = 3,                ///< float, cutoff frequency of filter.
		SquareUseContPoseEstimation = 4,     ///< bool, true to use continuous pose estimate.
		SquareConfidence = 5,                ///< float, confidence value of most recent marker match
		SquareConfidenceCutoff = 6,          ///< float, minimum allowable confidence value used in marker matching.
		NftScale = 7                         ///< float, scale factor applied to nft marker size.
	};

	//When the image processing mode is AR_IMAGE_PROC_FRAME_IMAGE,
	//ARToolKit processes all pixels in each incoming image
	//to locate markers.When the mode is AR_IMAGE_PROC_FIELD_IMAGE,
	//ARToolKit processes pixels in only every second pixel row and
	//column.This is useful both for handling images from interlaced
	//video sources(where alternate lines are assembled from alternate
	//fields and thus have one field time - difference, resulting in a
	//"comb" effect) such as Digital Video cameras.
	//The effective reduction by 75 % in the pixels processed also
	//has utility in accelerating tracking by effectively reducing
	//the image size to one quarter size, at the cost of pose accuraccy.
	public enum class ArImageProcMode
	{
		FrameImage = 0,
		FieldImage
	};

	public enum class ArPatternDetectionMode
	{
		TemplateMatchingColor = 0,
		TemplateMatchingMono = 1,
		MatrixCodeDetection = 2,
		TemplateMatchingColorAndMatrix = 3,
		TemplateMatchingMonoAndMatrix = 4
	};


	public delegate void ATOOLKITCOMPONENT_DELEGATE_LOG(Platform::String^ message);

    public ref class ARWrapper sealed
    {
    public:
		ARWrapper();

		void arwRegisterLogCallback(ATOOLKITCOMPONENT_DELEGATE_LOG^ callback);
		bool arwGetARToolKitVersion(Platform::String^ *buffer); // returns an allocated string.

		// ----------------------------------------------------------------------------------------------------
		//  ARToolKit lifecycle functions
		// ----------------------------------------------------------------------------------------------------
		bool arwInitialiseAR();
		bool arwInitialiseARWithOptions(int pattSize, int pattCountMax);
		int  arwGetError();
		//bool arwChangeToResourcesDir(Platform::String^ resourcesDirectoryPath);
		bool arwStartRunning(Platform::String^ vconf, Platform::String^ cparaName, float nearPlane, float farPlane);
		bool arwStartRunningB(Platform::String^ vconf, const Platform::Array<uint8>^ cparaBuff, float nearPlane, float farPlane);
		bool arwStartRunningStereo(Platform::String^ vconfL, Platform::String^ cparaNameL, Platform::String^ vconfR, Platform::String^ cparaNameR, Platform::String^ transL2RName, float nearPlane, float farPlane);
		bool arwStartRunningStereoB(Platform::String^ vconfL, const Platform::Array<uint8>^ cparaBuffL, Platform::String^ vconfR, const Platform::Array<uint8>^ cparaBuffR, const Platform::Array<uint8>^ transL2RBuff, float nearPlane, float farPlane);
		bool arwIsRunning();
		bool arwStopRunning();
		bool arwShutdownAR();

		// ----------------------------------------------------------------------------------------------------
		//  Video stream management
		// ----------------------------------------------------------------------------------------------------

		bool arwGetProjectionMatrix(Platform::WriteOnlyArray<float32>^ p16);
		bool arwGetProjectionMatrixStereo(Platform::WriteOnlyArray<float32>^ p16L, Platform::WriteOnlyArray<float32>^ p16R);
		bool arwGetVideoParams(int *width, int *height, int *pixelSize, Platform::String^ *pixelFormatStringBuffer); // returns an allocated string.
		bool arwGetVideoParamsStereo(int *widthL, int *heightL, int *pixelSizeL, Platform::String^ *pixelFormatStringBufferL, int *widthR, int *heightR, int *pixelSizeR, Platform::String^ *pixelFormatStringBufferR);
		bool arwCapture();
		bool arwUpdateAR();
		bool arwUpdateTexture(Platform::WriteOnlyArray<Color>^ buffer);
		bool arwUpdateTextureStereo(Platform::WriteOnlyArray<Color>^ bufferL, Platform::WriteOnlyArray<Color>^ bufferR);
		bool arwUpdateTexture32(Platform::WriteOnlyArray<Color32>^ buffer);
		bool arwUpdateTexture32Stereo(Platform::WriteOnlyArray<Color32>^ bufferL, Platform::WriteOnlyArray<Color32>^ bufferR);
		void arwSetVideoDebugMode(bool debug);
		bool arwGetVideoDebugMode();

		// ----------------------------------------------------------------------------------------------------
		//  Tracking configuration
		// ----------------------------------------------------------------------------------------------------
		
		void arwSetVideoThreshold(int threshold);
		int  arwGetVideoThreshold();
		void arwSetVideoThresholdMode(ArThresholdMode mode);
		ArThresholdMode  arwGetVideoThresholdMode();
		void arwSetLabelingMode(int mode);
		int  arwGetLabelingMode();
		void arwSetPatternDetectionMode(ArPatternDetectionMode mode);
		ArPatternDetectionMode arwGetPatternDetectionMode();
		void arwSetBorderSize(float size);
		float arwGetBorderSize();
		void arwSetMatrixCodeType(int type);
		int  arwGetMatrixCodeType();
		void arwSetImageProcMode(ArImageProcMode mode);
		ArImageProcMode  arwGetImageProcMode();
		void arwSetNFTMultiMode(bool on);
		bool arwGetNFTMultiMode();

		// ----------------------------------------------------------------------------------------------------
		//  Marker management
		// ----------------------------------------------------------------------------------------------------
		
		int arwAddMarker(Platform::String^ cfg);
		bool arwRemoveMarker(int markerUID);
		int arwRemoveAllMarkers();
		bool arwQueryMarkerVisibility(int markerUID);
		bool arwQueryMarkerTransformation(int markerUID, Platform::WriteOnlyArray<float32>^ matrix16);
		bool arwQueryMarkerTransformationStereo(int markerUID, Platform::WriteOnlyArray<float32>^ matrix16L, Platform::WriteOnlyArray<float32>^ matrix16R);
		int arwGetMarkerPatternCount(int markerUID);
		bool arwGetMarkerPatternConfig(int markerUID, int patternID, Platform::WriteOnlyArray<float32>^ matrix16, float *width, float *height, int *imageSizeX, int *imageSizeY);
		bool arwGetMarkerPatternImage(int markerUID, int patternID, Platform::WriteOnlyArray<Color>^ buffer);
		void arwSetMarkerOptionBool(int markerUID, ArMarkerOption option, bool value);
		void arwSetMarkerOptionInt(int markerUID, ArMarkerOption option, int value);
		void arwSetMarkerOptionFloat(int markerUID, ArMarkerOption option, float value);
		bool arwGetMarkerOptionBool(int markerUID, ArMarkerOption option);
		int arwGetMarkerOptionInt(int markerUID, ArMarkerOption option);
		float arwGetMarkerOptionFloat(int markerUID, ArMarkerOption option);

		// ----------------------------------------------------------------------------------------------------
		//  Utility
		// ----------------------------------------------------------------------------------------------------
		bool arwLoadOpticalParams(Platform::String^ optical_param_name, const Platform::Array<uint8>^ optical_param_buff, float *fovy_p, float *aspect_p, Platform::WriteOnlyArray<float32>^ m16, Platform::WriteOnlyArray<float32>^ p16);

	private:
		//~ARWrapper(); // Compiler creates a default destructor.
	};
}