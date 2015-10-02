//
//  ARToolKitComponent.cpp
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

#include "pch.h"
#include "ARToolKitComponent.h"
#include <string>
#include "windows.h"

#include <ARWrapper/ARToolKitWrapperExportedAPI.h>
#include <AR/config.h>
#include <AR/ar.h>

using namespace ARToolKitComponent;
using namespace Platform;
using namespace std;


// ============================================================================
//	Private functions.
// ============================================================================

// Convert a UTF16 std::wstring to an ANSI std::string.
static std::string utf16ws_to_ansis(const std::wstring &wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte                  (CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an ANSI std::string to a UTF16 std::wstring.
static std::wstring ansis_to_utf16ws(const std::string &str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar                  (CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Convert a Platform::String to an ANSI std::string.
static std::string ps_to_ansis(Platform::String^ ps)
{
	return utf16ws_to_ansis(std::wstring(ps->Data()));
}

// Convert an ANSI std::string to a Platform::String.
static Platform::String ^ansis_to_ps(std::string s)
{
	return ref new Platform::String(ansis_to_utf16ws(s).c_str());
}

// Convert an ANSI C string to a Platform::String.
static Platform::String ^ansicstr_to_ps(const char *text)
{
	return ansis_to_ps(std::string(text));
}

// Convert a UTF16 std::wstring to a UTF8 std::string.
static std::string utf16ws_to_utf8s(const std::wstring &wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte                  (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an UTF8 std::string to a UTF16 std::wstring.
static std::wstring utf8s_to_utf16ws(const std::string &str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar                  (CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Convert a Platform::String to a UTF8 std::string.
static std::string ps_to_utf8s(Platform::String^ ps)
{
	return utf16ws_to_utf8s(std::wstring(ps->Data()));
}

// Convert a UTF8 std::string to a Platform::String.
static Platform::String ^utf8s_to_ps(std::string s)
{
	return ref new Platform::String(utf8s_to_utf16ws(s).c_str());
}

// Convert a UTF8 C string to a Platform::String.
static Platform::String ^utf8cstr_to_ps(const char *text)
{
	return utf8s_to_ps(std::string(text));
}

static ARToolKitComponent::ATOOLKITCOMPONENT_DELEGATE_LOG^ logCallback;
void CALL_CONV ARToolKitComponent_log(const char *msg)
{
	if (logCallback != nullptr) {
		logCallback(ansicstr_to_ps(msg));
	}
}

// ============================================================================
//	Public functions.
// ============================================================================

ARWrapper::ARWrapper()
{
}

void ARWrapper::arwRegisterLogCallback(ATOOLKITCOMPONENT_DELEGATE_LOG^ callback)
{
#ifdef _DEBUG
	arLogLevel = AR_LOG_LEVEL_DEBUG;
#endif

	logCallback = callback;
	if (callback == nullptr) {
		::arwRegisterLogCallback(NULL);
	} else {
		::arwRegisterLogCallback(ARToolKitComponent_log);
	}
}

bool ARWrapper::arwGetARToolKitVersion(Platform::String^ *buffer)
{
	char buf[256];
	bool ok = ::arwGetARToolKitVersion(buf, sizeof(buf));
	if (!ok) {
		*buffer = nullptr;
	} else {
		*buffer = ansicstr_to_ps(buf);
	}
	return ok;
}

bool ARWrapper::arwInitialiseAR()
{
	return ::arwInitialiseAR();
}

bool ARWrapper::arwInitialiseARWithOptions(int pattSize, int pattCountMax)
{
	return ::arwInitialiseARWithOptions(pattSize, pattCountMax);
}

int ARWrapper::arwGetError()
{
	return ::arwGetError();
}

bool ARWrapper::arwStartRunning(Platform::String^ vconf, Platform::String^ cparaName, float nearPlane, float farPlane)
{
	const char *vconf_c = ps_to_ansis(vconf).c_str();
	const char *cparaName_c = ps_to_ansis(cparaName).c_str();

	return ::arwStartRunning(vconf_c, cparaName_c, nearPlane, farPlane);
}

bool ARWrapper::arwStartRunningB(Platform::String^ vconf, const Platform::Array<uint8>^ cparaBuff, float nearPlane, float farPlane)
{
	if (cparaBuff == nullptr) return false;

	auto str = ps_to_ansis(vconf);
	const char *vconf_c = str.c_str();
	unsigned int cparaBuffLen = cparaBuff->Length;
	char *cparaBuff_c = (char *)malloc(cparaBuffLen);
	for (unsigned int i = 0; i < cparaBuffLen; i++) cparaBuff_c[i] = cparaBuff[i];

	bool ok = ::arwStartRunningB(vconf_c, cparaBuff_c, cparaBuffLen, nearPlane, farPlane);

	free(cparaBuff_c);

	return ok;
}

bool ARWrapper::arwStartRunningStereo(Platform::String^ vconfL, Platform::String^ cparaNameL, Platform::String^ vconfR, Platform::String^ cparaNameR, Platform::String^ transL2RName, float nearPlane, float farPlane)
{
	const char *vconfL_c = ps_to_ansis(vconfL).c_str();
	const char *cparaNameL_c = ps_to_ansis(cparaNameL).c_str();
	const char *vconfR_c = ps_to_ansis(vconfR).c_str();
	const char *cparaNameR_c = ps_to_ansis(cparaNameR).c_str();
	const char *transL2RName_c = ps_to_ansis(transL2RName).c_str();
	return ::arwStartRunningStereo(vconfL_c, cparaNameL_c, vconfR_c, cparaNameR_c, transL2RName_c, nearPlane, farPlane);
}

bool ARWrapper::arwStartRunningStereoB(Platform::String^ vconfL, const Platform::Array<uint8>^ cparaBuffL, Platform::String^ vconfR, const Platform::Array<uint8>^ cparaBuffR, const Platform::Array<uint8>^ transL2RBuff, float nearPlane, float farPlane)
{
	if (cparaBuffL == nullptr || cparaBuffR == nullptr) return false;

	const char *vconfL_c = ps_to_ansis(vconfL).c_str();
	const char *vconfR_c = ps_to_ansis(vconfR).c_str();
	unsigned int cparaBuffLenL = cparaBuffL->Length;
	unsigned int cparaBuffLenR = cparaBuffR->Length;
	unsigned int transL2RBuffLen = transL2RBuff->Length;
	char *cparaBuffL_c = (char *)malloc(cparaBuffLenL);
	char *cparaBuffR_c = (char *)malloc(cparaBuffLenR);
	char *transL2RBuff_c = (char *)malloc(transL2RBuffLen);
	for (unsigned int i = 0; i < cparaBuffLenL; i++) cparaBuffL_c[i] = cparaBuffL[i];
	for (unsigned int i = 0; i < cparaBuffLenR; i++) cparaBuffR_c[i] = cparaBuffR[i];
	for (unsigned int i = 0; i < transL2RBuffLen; i++) transL2RBuff_c[i] = transL2RBuff[i];

	bool ok = ::arwStartRunningStereoB(vconfL_c, cparaBuffL_c, cparaBuffLenL, vconfR_c, cparaBuffR_c, cparaBuffLenR, transL2RBuff_c, transL2RBuffLen, nearPlane, farPlane);
	
	free(cparaBuffL_c);
	free(cparaBuffR_c);
	free(transL2RBuff_c);

	return ok;
}

bool ARWrapper::arwIsRunning()
{
	return ::arwIsRunning();
}

bool ARWrapper::arwStopRunning()
{
	return ::arwStopRunning();
}

bool ARWrapper::arwShutdownAR()
{
	return ::arwShutdownAR();
}

bool ARWrapper::arwGetProjectionMatrix(Platform::WriteOnlyArray<float>^ p)
{
	if (p->Length != 16) return false;

	//float p_c[16];
	//bool ok = ::arwGetProjectionMatrix(p_c);
	//if (ok) {
	//	for (int i = 0; i < 16; i++) p[i] = p_c[i];
	//}
	//return ok;

	return ::arwGetProjectionMatrix(p->Data);
}

bool ARWrapper::arwGetProjectionMatrixStereo(Platform::WriteOnlyArray<float>^ p16L, Platform::WriteOnlyArray<float>^ p16R)
{
	if (p16L->Length != 16 || p16R->Length != 16) return false;

	//float p16L_c[16];
	//float p16R_c[16];
	//bool ok = ::arwGetProjectionMatrixStereo(p16L_c, p16R_c);
	//if (ok) {
	//	for (int i = 0; i < 16; i++) p16L[i] = p16L_c[i];
	//	for (int i = 0; i < 16; i++) p16R[i] = p16R_c[i];
	//}
	//return ok;

	return ::arwGetProjectionMatrixStereo(p16L->Data, p16R->Data);
}

bool ARWrapper::arwGetVideoParams(int *width, int *height, int *pixelSize, Platform::String^ *pixelFormatStringBuffer)
{
	if (width == nullptr || height == nullptr || pixelSize == nullptr || pixelFormatStringBuffer == nullptr) return false;

	char buf[256];
	bool ok = ::arwGetVideoParams(width, height, pixelSize, buf, sizeof(buf));
	if (!ok) {
		*pixelFormatStringBuffer = nullptr;
	} else {
		*pixelFormatStringBuffer = ansicstr_to_ps(buf);
	}
	return ok;
}

bool ARWrapper::arwGetVideoParamsStereo(int *widthL, int *heightL, int *pixelSizeL, Platform::String^ *pixelFormatStringBufferL, int *widthR, int *heightR, int *pixelSizeR, Platform::String^ *pixelFormatStringBufferR)
{
	if (widthL == nullptr || heightL == nullptr || pixelSizeL == nullptr || pixelFormatStringBufferL == nullptr || widthR == nullptr || heightR == nullptr || pixelSizeR == nullptr || pixelFormatStringBufferR == nullptr) return false;

	char bufL[256];
	char bufR[256];
	bool ok = ::arwGetVideoParamsStereo(widthL, heightL, pixelSizeL, bufL, sizeof(bufL), widthR, heightR, pixelSizeR, bufR, sizeof(bufR));
	if (!ok) {
		*pixelFormatStringBufferL = nullptr;
		*pixelFormatStringBufferR = nullptr;
	} else {
		*pixelFormatStringBufferL = ansicstr_to_ps(bufL);
		*pixelFormatStringBufferR = ansicstr_to_ps(bufR);
	}
	return ok;
}

bool ARWrapper::arwCapture()
{
	return ::arwCapture();
}

bool ARWrapper::arwUpdateAR()
{
	return ::arwUpdateAR();
}

bool ARWrapper::arwUpdateTexture(Platform::WriteOnlyArray<::ARToolKitComponent::Color>^ buffer)
{
	return ::arwUpdateTexture((::Color *)(buffer->Data));
}

bool ARWrapper::arwUpdateTextureStereo(Platform::WriteOnlyArray<::ARToolKitComponent::Color>^ bufferL, Platform::WriteOnlyArray<::ARToolKitComponent::Color>^ bufferR)
{
	return ::arwUpdateTextureStereo((::Color *)(bufferL->Data), (::Color *)(bufferR->Data));
}

bool ARWrapper::arwUpdateTexture32(Platform::WriteOnlyArray<::ARToolKitComponent::Color32>^ buffer)
{
	return ::arwUpdateTexture32((::Color32 *)(buffer->Data));
}

bool ARWrapper::arwUpdateTexture32Stereo(Platform::WriteOnlyArray<::ARToolKitComponent::Color32>^ bufferL, Platform::WriteOnlyArray<::ARToolKitComponent::Color32>^ bufferR)
{
	return ::arwUpdateTexture32Stereo((::Color32 *)(bufferL->Data), (::Color32 *)(bufferR->Data));
}

void ARWrapper::arwSetVideoDebugMode(bool debug)
{
	::arwSetVideoDebugMode(debug);
}

bool ARWrapper::arwGetVideoDebugMode()
{
	return ::arwGetVideoDebugMode();
}

void ARWrapper::arwSetVideoThreshold(int threshold)
{
	::arwSetVideoThreshold(threshold);
}

int  ARWrapper::arwGetVideoThreshold()
{
	return ::arwGetVideoThreshold();
}

void ARWrapper::arwSetVideoThresholdMode(ArThresholdMode mode)
{
	::arwSetVideoThresholdMode((int)mode);
}

ArThresholdMode  ARWrapper::arwGetVideoThresholdMode()
{
	return (ArThresholdMode)::arwGetVideoThresholdMode();
}

void ARWrapper::arwSetLabelingMode(int mode)
{
	::arwSetLabelingMode(mode);
}

int  ARWrapper::arwGetLabelingMode()
{
	return ::arwGetLabelingMode();
}

void ARWrapper::arwSetPatternDetectionMode(ArPatternDetectionMode mode)
{
	::arwSetPatternDetectionMode((int)mode);
}

ArPatternDetectionMode ARWrapper::arwGetPatternDetectionMode()
{
	return (ArPatternDetectionMode)::arwGetPatternDetectionMode();
}

void ARWrapper::arwSetBorderSize(float size)
{
	::arwSetBorderSize(size);
}

float ARWrapper::arwGetBorderSize()
{
	return ::arwGetBorderSize();
}

void ARWrapper::arwSetMatrixCodeType(int type)
{
	::arwSetMatrixCodeType(type);
}

int  ARWrapper::arwGetMatrixCodeType()
{
	return ::arwGetMatrixCodeType();
}

void ARWrapper::arwSetImageProcMode(ArImageProcMode mode)
{
	::arwSetImageProcMode((int)mode);
}

ArImageProcMode  ARWrapper::arwGetImageProcMode()
{
	return (ArImageProcMode)::arwGetImageProcMode();
}

void ARWrapper::arwSetNFTMultiMode(bool on)
{
	::arwSetNFTMultiMode(on);
}

bool ARWrapper::arwGetNFTMultiMode()
{
	return ::arwGetNFTMultiMode();
}

int ARWrapper::arwAddMarker(Platform::String^ cfg)
{
	auto str = ps_to_ansis(cfg);
	return ::arwAddMarker(str.c_str());
}

bool ARWrapper::arwRemoveMarker(int markerUID)
{
	return ::arwRemoveMarker(markerUID);
}

int ARWrapper::arwRemoveAllMarkers()
{
	return ::arwRemoveAllMarkers();
}

bool ARWrapper::arwQueryMarkerVisibility(int markerUID)
{
	return ::arwQueryMarkerVisibility(markerUID);
}

bool ARWrapper::arwQueryMarkerTransformation(int markerUID, Platform::WriteOnlyArray<float>^ matrix16)
{
	if (matrix16->Length != 16) return false;

	return ::arwQueryMarkerTransformation(markerUID, matrix16->Data);
}

bool ARWrapper::arwQueryMarkerTransformationStereo(int markerUID, Platform::WriteOnlyArray<float>^ matrix16L, Platform::WriteOnlyArray<float>^ matrix16R)
{
	if (matrix16L->Length != 16 || matrix16R->Length != 16) return false;

	return ::arwQueryMarkerTransformationStereo(markerUID, matrix16L->Data, matrix16R->Data);
}

int ARWrapper::arwGetMarkerPatternCount(int markerUID)
{
	return ::arwGetMarkerPatternCount(markerUID);
}

bool ARWrapper::arwGetMarkerPatternConfig(int markerUID, int patternID, Platform::WriteOnlyArray<float32>^ matrix16, float *width, float *height, int *imageSizeX, int *imageSizeY)
{
	if (matrix16->Length != 16 || width == nullptr || height == nullptr || imageSizeX == nullptr || imageSizeY == nullptr) return false;

	return ::arwGetMarkerPatternConfig(markerUID, patternID, matrix16->Data, width, height, imageSizeX, imageSizeY);
}

bool ARWrapper::arwGetMarkerPatternImage(int markerUID, int patternID, Platform::WriteOnlyArray<::ARToolKitComponent::Color>^ buffer)
{
	if (buffer->Length != 16) return false;

	return ::arwGetMarkerPatternImage(markerUID, patternID, (::Color *)(buffer->Data));
}

void ARWrapper::arwSetMarkerOptionBool(int markerUID, ArMarkerOption option, bool value)
{
	::arwSetMarkerOptionBool(markerUID, (int)option, value);
}

void ARWrapper::arwSetMarkerOptionInt(int markerUID, ArMarkerOption option, int value)
{
	::arwSetMarkerOptionInt(markerUID, (int)option, value);
}

void ARWrapper::arwSetMarkerOptionFloat(int markerUID, ArMarkerOption option, float value)
{
	::arwSetMarkerOptionFloat(markerUID, (int)option, value);
}

bool ARWrapper::arwGetMarkerOptionBool(int markerUID, ArMarkerOption option)
{
	return ::arwGetMarkerOptionBool(markerUID, (int)option);
}

int ARWrapper::arwGetMarkerOptionInt(int markerUID, ArMarkerOption option)
{
	return ::arwGetMarkerOptionInt(markerUID, (int)option);
}

float ARWrapper::arwGetMarkerOptionFloat(int markerUID, ArMarkerOption option)
{
	return ::arwGetMarkerOptionFloat(markerUID, (int)option);
}

bool ARWrapper::arwLoadOpticalParams(Platform::String^ optical_param_name, const Platform::Array<uint8>^ optical_param_buff, float *fovy_p, float *aspect_p, Platform::WriteOnlyArray<float>^ m16, Platform::WriteOnlyArray<float>^ p16)
{
	if ((optical_param_name == nullptr && optical_param_buff == nullptr) || fovy_p == nullptr || aspect_p == nullptr || m16->Length != 16 || p16->Length != 16) return false;

	if (optical_param_name != nullptr) {
		const char *optical_param_name_c = ps_to_ansis(optical_param_name).c_str();
		return ::arwLoadOpticalParams(optical_param_name_c, NULL, 0, fovy_p, aspect_p, m16->Data, p16->Data);
	} else {
		unsigned int optical_param_buffLen = optical_param_buff->Length;
		char *optical_param_buff_c = (char *)malloc(optical_param_buffLen);
		for (unsigned int i = 0; i < optical_param_buffLen; i++) optical_param_buff_c[i] = optical_param_buff[i];
		bool ok = ::arwLoadOpticalParams(NULL, optical_param_buff_c, optical_param_buffLen, fovy_p, aspect_p, m16->Data, p16->Data);
		free(optical_param_buff_c);
		return ok;
	}
}

