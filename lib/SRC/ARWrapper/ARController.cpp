/*
 *  ARController.cpp
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

#include <ARWrapper/ARController.h>
#include <ARWrapper/Error.h>
#if TARGET_PLATFORM_ANDROID
#  include <ARWrapper/AndroidFeatures.h>
#endif
#if TARGET_PLATFORM_ANDROID || TARGET_PLATFORM_IOS
#  include <AR/gsub_es.h>
#elif TARGET_PLATFORM_WINRT
#  include <AR/gsub_es2.h>
#else
#  include <AR/gsub_lite.h>
#endif
#include <AR/gsub_mtx.h>
#ifdef __APPLE__
#  include <syslog.h>
#endif
#if HAVE_NFT
#  include "trackingSub.h"
#endif
#include <AR/paramGL.h>

#include <stdarg.h>

#include <algorithm>
#include <string>

// ----------------------------------------------------------------------------------------------------
#pragma mark  Singleton, constructor, destructor
// ----------------------------------------------------------------------------------------------------

static const char LOG_TAG[] = "ARController (native)";
PFN_LOGCALLBACK ARController::logCallback = NULL;

ARController::ARController() :
    state(NOTHING_INITIALISED),
    versionString(NULL),
    m_videoSource0(NULL),
    m_videoSource1(NULL),
    m_videoSourceIsStereo(false),
    m_updateFrameStamp0({0,0}),
    m_updateFrameStamp1({0,0}),
    m_projectionNearPlane(10.0),
    m_projectionFarPlane(10000.0),
	m_projectionMatrixSet(false),
    threshold(AR_DEFAULT_LABELING_THRESH),
    thresholdMode(AR_LABELING_THRESH_MODE_DEFAULT),
    imageProcMode(AR_DEFAULT_IMAGE_PROC_MODE),
    labelingMode(AR_DEFAULT_LABELING_MODE),
    pattRatio(AR_PATT_RATIO),
    patternDetectionMode(AR_DEFAULT_PATTERN_DETECTION_MODE),
    matrixCodeType(AR_MATRIX_CODE_TYPE_DEFAULT),
    debugMode(FALSE),
    markers(),
    doMarkerDetection(false),
    m_arHandle0(NULL),
    m_arHandle1(NULL),
    m_arPattHandle(NULL),
    m_ar3DHandle(NULL),
    m_ar3DStereoHandle(NULL),
#if HAVE_NFT
    doNFTMarkerDetection(false),
    m_nftMultiMode(false),
    m_kpmRequired(true),
    m_kpmBusy(false),
    trackingThreadHandle(NULL),
    m_ar2Handle(NULL),
    m_kpmHandle(NULL),
#endif
    m_error(ARW_ERROR_NONE)
{
#ifdef __APPLE__
    //openlog("ARController C++", LOG_CONS, LOG_USER);
    //syslog(LOG_ERR, "Hello world!\n");
#endif
#if HAVE_NFT
    for (int i = 0; i < PAGES_MAX; i++) surfaceSet[i] = NULL;
#endif
    pthread_mutex_init(&m_videoSourceLock, NULL);
}

ARController::~ARController()
{
	shutdown();
    pthread_mutex_destroy(&m_videoSourceLock);
#ifdef __APPLE__
    //closelog();
#endif
    if (versionString) free(versionString);
}

const char* ARController::getARToolKitVersion()
{
    if (!versionString) arGetVersion(&versionString);
	return versionString;
}

void ARController::setError(int error)
{
    if (m_error == ARW_ERROR_NONE) {
        m_error = error;
    }
}

int ARController::getError()
{
    int temp = m_error;
    if (temp != ARW_ERROR_NONE) {
        m_error = ARW_ERROR_NONE;
    }
    return temp;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  ARToolKit lifecycle functions
// ----------------------------------------------------------------------------------------------------

bool ARController::initialiseBase(const int patternSize, const int patternCountMax)
{
    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::initialiseBase(): called");
	if (state != NOTHING_INITIALISED) {
        logv(AR_LOG_LEVEL_ERROR, "Initialise called while already initialised. Will shutdown first, exiting, returning false");
		if (!shutdown())
            return false;
	}
	logv(AR_LOG_LEVEL_INFO, "ARWrapper::ARController::initialiseBase(): Initialising...");

	// Check libAR version matches libARvideo version.
	ARUint32 version = arGetVersion(NULL);
#if !TARGET_PLATFORM_ANDROID
    ARUint32 videoVersion = arVideoGetVersion();
    if (version != videoVersion)
        logv(AR_LOG_LEVEL_WARN, "ARWrapper::ARController::initialiseBase(): ARToolKit libAR version (%06x) does not match libARvideo version (%06x)", version >> 8, videoVersion >> 8);
    if (videoVersion < 0x04050b00) { // Minimum version 4.5.11 required.
        logv(AR_LOG_LEVEL_ERROR, "ARWrapper::ARController::initialiseBase(): ARToolKit libARvideo version (%06x) is too old. Version 04050b required, exiting, returning false", videoVersion >> 8);
        return (false);
    }
#endif

	// Create pattern handle so markers can begin to be added.
	if ((m_arPattHandle = arPattCreateHandle2(patternSize, patternCountMax)) == NULL) {
        logv(AR_LOG_LEVEL_ERROR, "Error: arPattCreateHandle2, exiting, returning false");
		return false;
	}

	state = BASE_INITIALISED;

    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::initialiseBase(): Initialised, exiting, returning true");
	return true;
}

void ARController::setProjectionNearPlane(const ARdouble projectionNearPlane)
{
    m_projectionNearPlane = projectionNearPlane;
}

void ARController::setProjectionFarPlane(const ARdouble projectionFarPlane)
{
    m_projectionFarPlane = projectionFarPlane;
}

ARdouble ARController::projectionNearPlane(void)
{
    return m_projectionNearPlane;
}

ARdouble ARController::projectionFarPlane(void)
{
    return m_projectionFarPlane;
}

bool ARController::startRunning(const char* vconf, const char* cparaName, const char* cparaBuff, const long cparaBuffLen)
{
	logv(AR_LOG_LEVEL_INFO, "ARController::startRunning(): called, start running");

	// Check for initialization before starting video
	if (state != BASE_INITIALISED) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: not initialized, exiting, returning false");
		return false;
	}

	m_videoSource0 = new ARVideoSource;
	if (!m_videoSource0) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: no video source, exiting, returning false");
		return false;
	}

	m_videoSource0->configure(vconf, false, cparaName, cparaBuff, cparaBuffLen);

    if (!m_videoSource0->open()) {
        if (m_videoSource0->getError() == ARW_ERROR_DEVICE_UNAVAILABLE) {
            logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: video source unavailable, exiting, returning false");
            setError(ARW_ERROR_DEVICE_UNAVAILABLE);
        } else {
            logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: unable to open video source, exiting, returning false");
        }
        delete m_videoSource0;
        m_videoSource0 = NULL;
        return false;
    }

    m_videoSourceIsStereo = false;
	state = WAITING_FOR_VIDEO;
    stateWaitingMessageLogged = false;

    logv(AR_LOG_LEVEL_DEBUG, "ARController::startRunning(): exiting, returning true");
	return true;
}

bool ARController::startRunningStereo(const char* vconfL, const char* cparaNameL, const char* cparaBuffL, const long cparaBuffLenL,
                                      const char* vconfR, const char* cparaNameR, const char* cparaBuffR, const long cparaBuffLenR,
                                      const char* transL2RName, const char* transL2RBuff, const long transL2RBuffLen)
{
    logv(AR_LOG_LEVEL_INFO, "ARController::startRunningStereo(): called, start running");

	// Check for initialisation before starting video
	if (state != BASE_INITIALISED) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: not initialized, exiting, returning false");
		return false;
	}

    // Load stereo parameters.
    if (transL2RName) {
        if (arParamLoadExt(transL2RName, m_transL2R) < 0) {
            logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: arParamLoadExt, exiting, returning false");
            return false;
        }
    } else if (transL2RBuff && transL2RBuffLen > 0) {
        if (arParamLoadExtFromBuffer(transL2RBuff, transL2RBuffLen, m_transL2R) < 0) {
            logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: arParamLoadExtFromBuffer, exiting, returning false");
            return false;
        }
    } else {
        logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: transL2R not specified, exiting, returning false");
		return false;
    }
    //arUtilMatInv(m_transL2R, transR2L);
    arParamDispExt(m_transL2R);

	m_videoSource0 = new ARVideoSource;
	m_videoSource1 = new ARVideoSource;
	if (!m_videoSource0 || !m_videoSource1) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: no video source, exiting, returning false");
		return false;
	}

	m_videoSource0->configure(vconfL, false, cparaNameL, cparaBuffL, cparaBuffLenL);
	m_videoSource1->configure(vconfR, false, cparaNameR, cparaBuffR, cparaBuffLenR);

    if (!m_videoSource0->open()) {
        if (m_videoSource0->getError() == ARW_ERROR_DEVICE_UNAVAILABLE) {
            logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: video source 0 unavailable, exiting, returning false");
            setError(ARW_ERROR_DEVICE_UNAVAILABLE);
        } else {
            logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: unable to open video source 0, exiting, returning false");
        }
        delete m_videoSource0;
        m_videoSource0 = NULL;
        delete m_videoSource1;
        m_videoSource1 = NULL;
        return false;
    }
    if (!m_videoSource1->open()) {
        if (m_videoSource1->getError() == ARW_ERROR_DEVICE_UNAVAILABLE) {
            logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: video source 1 unavailable, exiting, returning false");
            setError(ARW_ERROR_DEVICE_UNAVAILABLE);
        } else {
            logv(AR_LOG_LEVEL_ERROR, "ARController::startRunning(): Error: unable to open video source 1, exiting, returning false");
        }
        delete m_videoSource0;
        m_videoSource0 = NULL;
        delete m_videoSource1;
        m_videoSource1 = NULL;
        return false;
    }

    m_videoSourceIsStereo = true;
	state = WAITING_FOR_VIDEO;
    stateWaitingMessageLogged = false;

    logv(AR_LOG_LEVEL_DEBUG, "ARController::startRunningStereo(): exiting, returning true");
	return true;
}

bool ARController::capture()
{
    // First check there is a video source and it's open.
    if (!m_videoSource0 || !m_videoSource0->isOpen() || (m_videoSourceIsStereo && (!m_videoSource1 || !m_videoSource1->isOpen()))) {
        logv(AR_LOG_LEVEL_ERROR, "ARWrapper::ARController::capture(): Error-no video source or video source is closed, returning false");
        return false;
    }

    if (!m_videoSource0->captureFrame()) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::capture(): m_videoSource0->captureFrame() returned false, exiting returning false");
        return false;
    }

    if (m_videoSourceIsStereo) {
        if (!m_videoSource1->captureFrame()) {
            logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::capture(): m_videoSource1->captureFrame() returned false, exiting returning false");
            return false;
        }
    }

    return true;
}

bool ARController::updateTexture32(const int videoSourceIndex, uint32_t *buffer)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;

    if (!debugMode) {
        ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
        if (!vs) return false;

        return vs->getFrameTextureRGBA32(buffer);
    } else {
        return updateDebugTexture32(videoSourceIndex, buffer);
    }
}

bool ARController::update()
{

	if (state != DETECTION_RUNNING) {
        if (state != WAITING_FOR_VIDEO) {
            // State is NOTHING_INITIALISED or BASE_INITIALISED.
            logv(AR_LOG_LEVEL_ERROR, "ARWrapper::ARController::update(): Error-if (state != WAITING_FOR_VIDEO) true, exiting returning false");
            return false;

        } else {

            // First check there is a video source and it's open.
            if (!m_videoSource0 || !m_videoSource0->isOpen() || (m_videoSourceIsStereo && (!m_videoSource1 || !m_videoSource1->isOpen()))) {
                logv(AR_LOG_LEVEL_ERROR, "ARWrapper::ARController::update(): Error-no video source or video source is closed, exiting returning false");
                return false;
            }

            // Video source is open, check whether we're waiting for it to start running.
            // If it's not running, return to caller now.
            if (!m_videoSource0->isRunning() || (m_videoSourceIsStereo && !m_videoSource1->isRunning())) {

                if (!stateWaitingMessageLogged) {
                    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::update(): \"Waiting for video\" message logged, exiting returning true");
                    stateWaitingMessageLogged = true;
                }
                return true;
            }

            // Initialise the parts of the AR tracking that rely on information from the video source.
            // Compute the projection matrix.
#if TARGET_PLATFORM_ANDROID || TARGET_PLATFORM_IOS || TARGET_PLATFORM_WINRT
            arglCameraFrustumRHf(&m_videoSource0->getCameraParameters()->param, m_projectionNearPlane, m_projectionFarPlane, m_projectionMatrix0);
            if (m_videoSourceIsStereo) arglCameraFrustumRHf(&m_videoSource1->getCameraParameters()->param, m_projectionNearPlane, m_projectionFarPlane, m_projectionMatrix1);
#else
            arglCameraFrustumRH(&m_videoSource0->getCameraParameters()->param, m_projectionNearPlane, m_projectionFarPlane, m_projectionMatrix0);
            if (m_videoSourceIsStereo) arglCameraFrustumRH(&m_videoSource1->getCameraParameters()->param, m_projectionNearPlane, m_projectionFarPlane, m_projectionMatrix1);
#endif
            m_projectionMatrixSet = true;
            logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::update(): Video ready, computed projection matrix using near=%f far=%f",
                     m_projectionNearPlane, m_projectionFarPlane);
            logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::update(): setting state to DETECTION_RUNNING");
            state = DETECTION_RUNNING;
        }
	}

    // Checkout frame(s).
    AR2VideoBufferT *image0, *image1 = NULL;
    image0 = m_videoSource0->checkoutFrameIfNewerThan(m_updateFrameStamp0);
    if (!image0) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::update(): m_videoSource0->checkoutFrameIfNewerThan() called but no frame returned, exiting returning true");
        return true;
    }
    m_updateFrameStamp0 = image0->time;
    if (m_videoSourceIsStereo) {
        image1 = m_videoSource1->checkoutFrameIfNewerThan(m_updateFrameStamp1);
        if (!image1) {
            logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::update(): m_videoSource1->checkoutFrameIfNewerThan() called but no frame returned, exiting returning true");
            m_videoSource0->checkinFrame(); // If we didn't checkout this frame, but we already checked out a frame from one or more other video sources, check those back in.
            return true;
        }
        m_updateFrameStamp1 = image1->time;
    }

    //
    // Detect markers.
    //

    bool ret;
    
    if (doMarkerDetection) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::update(): if (doMarkerDetection) true");

        ARMarkerInfo *markerInfo0 = NULL;
        ARMarkerInfo *markerInfo1 = NULL;
        int markerNum0 = 0;
        int markerNum1 = 0;

        if (!m_arHandle0 || (m_videoSourceIsStereo && !m_arHandle1)) {
            if (!initAR()) {
                logv(AR_LOG_LEVEL_ERROR, "ARController::update(): Error initialising AR, exiting returning false");
                ret = false;
                goto done;
            }
        }

        if (m_arHandle0) {
            if (arDetectMarker(m_arHandle0, image0) < 0) {
                logv(AR_LOG_LEVEL_ERROR, "ARController::update(): Error: arDetectMarker(), exiting returning false");
                ret = false;
                goto done;
            }
            markerInfo0 = arGetMarker(m_arHandle0);
            markerNum0 = arGetMarkerNum(m_arHandle0);
        }
        if (m_videoSourceIsStereo && m_arHandle1) {
            if (arDetectMarker(m_arHandle1, image1) < 0) {
                logv(AR_LOG_LEVEL_ERROR, "ARController::update(): Error: arDetectMarker(), exiting returning false");
                ret = false;
                goto done;
            }
            markerInfo1 = arGetMarker(m_arHandle1);
            markerNum1 = arGetMarkerNum(m_arHandle1);
        }

        // Update square markers.
        bool success = true;
        if (!m_videoSourceIsStereo) {
            for (std::vector<ARMarker *>::iterator it = markers.begin(); it != markers.end(); ++it) {
                if ((*it)->type == ARMarker::SINGLE) {
                    success &= ((ARMarkerSquare *)(*it))->updateWithDetectedMarkers(markerInfo0, markerNum0, m_ar3DHandle);
                } else if ((*it)->type == ARMarker::MULTI) {
                    success &= ((ARMarkerMulti *)(*it))->updateWithDetectedMarkers(markerInfo0, markerNum0, m_ar3DHandle);
                }
            }
        } else {
            for (std::vector<ARMarker *>::iterator it = markers.begin(); it != markers.end(); ++it) {
                if ((*it)->type == ARMarker::SINGLE) {
                    success &= ((ARMarkerSquare *)(*it))->updateWithDetectedMarkersStereo(markerInfo0, markerNum0, markerInfo1, markerNum1, m_ar3DStereoHandle, m_transL2R);
                } else if ((*it)->type == ARMarker::MULTI) {
                    success &= ((ARMarkerMulti *)(*it))->updateWithDetectedMarkersStereo(markerInfo0, markerNum0, markerInfo1, markerNum1, m_ar3DStereoHandle, m_transL2R);
                }
            }
        }
    } // doMarkerDetection

#if HAVE_NFT
    if (doNFTMarkerDetection) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::update(): if (doNFTMarkerDetection) true");

        if (!m_kpmHandle || !m_ar2Handle) {
            if (!initNFT()) {
                logv(AR_LOG_LEVEL_ERROR, "ARController::update(): Error initialising NFT, exiting returning false");
                ret = false;
                goto done;
            }
        }
        if (!trackingThreadHandle) {
            loadNFTData();
        }

        if (trackingThreadHandle) {

            // Do KPM tracking.
            float err;
            float trackingTrans[3][4];

            if (m_kpmRequired) {
                if (!m_kpmBusy) {
                    trackingInitStart(trackingThreadHandle, image0->buffLuma);
                    m_kpmBusy = true;
                } else {
                    int ret;
                    int pageNo;
                    ret = trackingInitGetResult(trackingThreadHandle, trackingTrans, &pageNo);
                    if (ret != 0) {
                        m_kpmBusy = false;
                        if (ret == 1) {
                            if (pageNo >= 0 && pageNo < PAGES_MAX) {
								if (surfaceSet[pageNo]->contNum < 1) {
									//logv("Detected page %d.\n", pageNo);
									ar2SetInitTrans(surfaceSet[pageNo], trackingTrans); // Sets surfaceSet[page]->contNum = 1.
								}
                            } else {
                                logv(AR_LOG_LEVEL_ERROR, "ARController::update(): Detected bad page %d", pageNo);
                            }
                        } else /*if (ret < 0)*/ {
                            //logv("No page detected.");
                        }
                    }
                }
            }

            // Do AR2 tracking and update NFT markers.
            int page = 0;
            int pagesTracked = 0;
            bool success = true;
            ARdouble *transL2R = (m_videoSourceIsStereo ? (ARdouble *)m_transL2R : NULL);

            for (std::vector<ARMarker *>::iterator it = markers.begin(); it != markers.end(); ++it) {
                if ((*it)->type == ARMarker::NFT) {

                    if (surfaceSet[page]->contNum > 0) {
                        if (ar2Tracking(m_ar2Handle, surfaceSet[page], image0->buff, trackingTrans, &err) < 0) {
                            //logv("Tracking lost on page %d.", page);
                            success &= ((ARMarkerNFT *)(*it))->updateWithNFTResults(-1, NULL, NULL);
                        } else {
                            //logv("Tracked page %d (pos = {% 4f, % 4f, % 4f}).\n", page, trackingTrans[0][3], trackingTrans[1][3], trackingTrans[2][3]);
                            success &= ((ARMarkerNFT *)(*it))->updateWithNFTResults(page, trackingTrans, (ARdouble (*)[4])transL2R);
                            pagesTracked++;
                        }
                    }

                    page++;
                }
            }

            m_kpmRequired = (pagesTracked < (m_nftMultiMode ? page : 1));

        } // trackingThreadHandle
    } // doNFTMarkerDetection
#endif // HAVE_NFT
    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::update(): exiting, returning true");

    ret = true;

done:
    // Checkin frames.
    m_videoSource0->checkinFrame();
    if (m_videoSourceIsStereo) m_videoSource1->checkinFrame();

    return ret;
}

bool ARController::initAR(void)
{
    logv(AR_LOG_LEVEL_INFO, "ARController::initAR() called");

    // Create AR handle
    if ((m_arHandle0 = arCreateHandle(m_videoSource0->getCameraParameters())) == NULL) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::initAR(): Error: arCreateHandle()");
        goto bail;
    }

    // Set the pixel format
    if (arSetPixelFormat(m_arHandle0, m_videoSource0->getPixelFormat()) < 0) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::initAR(): Error: arSetPixelFormat");
        goto bail1;
    }

    arPattAttach(m_arHandle0, m_arPattHandle);

    // Set initial configuration. One call for each configuration option.
    arSetLabelingThresh(m_arHandle0, threshold);
    arSetLabelingThreshMode(m_arHandle0, thresholdMode);
    arSetImageProcMode(m_arHandle0, imageProcMode);
    arSetDebugMode(m_arHandle0, debugMode);
    arSetLabelingMode(m_arHandle0, labelingMode);
    arSetPattRatio(m_arHandle0, pattRatio);
    arSetPatternDetectionMode(m_arHandle0, patternDetectionMode);
    arSetMatrixCodeType(m_arHandle0, matrixCodeType);

    if (m_videoSourceIsStereo) {
        // Create AR handle
        if ((m_arHandle1 = arCreateHandle(m_videoSource1->getCameraParameters())) == NULL) {
            logv(AR_LOG_LEVEL_ERROR, "ARController::initAR(): Error: arCreateHandle()");
            goto bail1;
        }

        // Set the pixel format
        if (arSetPixelFormat(m_arHandle1, m_videoSource1->getPixelFormat()) < 0) {
            logv(AR_LOG_LEVEL_ERROR, "ARController::initAR(): Error: arSetPixelFormat");
            goto bail2;
        }

        arPattAttach(m_arHandle1, m_arPattHandle);

        // Set initial configuration. One call for each configuration option.
        arSetLabelingThresh(m_arHandle1, threshold);
        arSetLabelingThreshMode(m_arHandle1, thresholdMode);
        arSetImageProcMode(m_arHandle1, imageProcMode);
        arSetDebugMode(m_arHandle1, debugMode);
        arSetLabelingMode(m_arHandle1, labelingMode);
        arSetPattRatio(m_arHandle1, pattRatio);
        arSetPatternDetectionMode(m_arHandle1, patternDetectionMode);
        arSetMatrixCodeType(m_arHandle1, matrixCodeType);
    }

    if (!m_videoSourceIsStereo) {
        // Create 3D handle
        if ((m_ar3DHandle = ar3DCreateHandle(&m_videoSource0->getCameraParameters()->param)) == NULL) {
            logv(AR_LOG_LEVEL_ERROR, "ARController::initAR(): Error: ar3DCreateHandle");
            goto bail2;
        }
    } else{
        m_ar3DStereoHandle = ar3DStereoCreateHandle(&m_videoSource0->getCameraParameters()->param, &m_videoSource1->getCameraParameters()->param, AR_TRANS_MAT_IDENTITY, m_transL2R);
        if (!m_ar3DStereoHandle) {
            logv(AR_LOG_LEVEL_ERROR, "ARController::initAR(): Error: ar3DStereoCreateHandle, exiting, returning false");
            goto bail2;
        }
    }

    logv(AR_LOG_LEVEL_DEBUG, "ARController::initAR() exiting, returning true");
    return true;

bail2:
    arDeleteHandle(m_arHandle1);
    m_arHandle1 = NULL;
bail1:
    arDeleteHandle(m_arHandle0);
    m_arHandle0 = NULL;
bail:
    logv(AR_LOG_LEVEL_ERROR, "ARController::initAR() exiting, returning false");
    return false;
}

#if HAVE_NFT
bool ARController::initNFT(void)
{
 	logv(AR_LOG_LEVEL_INFO, "ARController::initNFT(): called, initialising NFT");
    //
    // NFT init.
    //

    // KPM init.
    m_kpmHandle = kpmCreateHandle(m_videoSource0->getCameraParameters());
    if (!m_kpmHandle) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::initNFT(): Error: kpmCreatHandle, exiting, returning false");
        return (false);
    }
    //kpmSetProcMode( m_kpmHandle, KpmProcHalfSize );

    // AR2 init.
    if( (m_ar2Handle = ar2CreateHandle(m_videoSource0->getCameraParameters(), m_videoSource0->getPixelFormat(), AR2_TRACKING_DEFAULT_THREAD_NUM)) == NULL ) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::initNFT(): Error: ar2CreateHandle, exiting, returning false");
        kpmDeleteHandle(&m_kpmHandle);
        return (false);
    }
    if (threadGetCPU() <= 1) {
        logv(AR_LOG_LEVEL_INFO, "Using NFT tracking settings for a single CPU");
        // Settings for devices with single-core CPUs.
        ar2SetTrackingThresh( m_ar2Handle, 5.0 );
        ar2SetSimThresh( m_ar2Handle, 0.50 );
        ar2SetSearchFeatureNum(m_ar2Handle, 16);
        ar2SetSearchSize(m_ar2Handle, 6);
        ar2SetTemplateSize1(m_ar2Handle, 6);
        ar2SetTemplateSize2(m_ar2Handle, 6);
    } else {
        logv(AR_LOG_LEVEL_INFO, "Using NFT tracking settings for more than one CPU");
        // Settings for devices with dual/multi-core CPUs.
        ar2SetTrackingThresh( m_ar2Handle, 5.0 );
        ar2SetSimThresh( m_ar2Handle, 0.50 );
        ar2SetSearchFeatureNum(m_ar2Handle, 16);
        ar2SetSearchSize(m_ar2Handle, 12);
        ar2SetTemplateSize1(m_ar2Handle, 6);
        ar2SetTemplateSize2(m_ar2Handle, 6);
    }
    logv(AR_LOG_LEVEL_DEBUG, "ARController::initNFT(): NFT initialisation complete, exiting, returning true");
    return (true);
}

bool ARController::unloadNFTData(void)
{
    int i;

    if (trackingThreadHandle) {
        logv(AR_LOG_LEVEL_INFO, "Stopping NFT tracking thread.");
        trackingInitQuit(&trackingThreadHandle);
        m_kpmBusy = false;
    }
    for (i = 0; i < PAGES_MAX; i++) surfaceSet[i] = NULL; // Discard weak-references.
    m_kpmRequired = true;

    return true;
}

bool ARController::loadNFTData(void)
{
    // If data was already loaded, stop KPM tracking thread and unload previously loaded data.
    if (trackingThreadHandle) {
        logv(AR_LOG_LEVEL_INFO, "Reloading NFT data");
        unloadNFTData();
    } else {
        logv(AR_LOG_LEVEL_INFO, "Loading NFT data");
    }

    KpmRefDataSet *refDataSet = NULL;
    int pageCount = 0;

    for (std::vector<ARMarker *>::iterator it = markers.begin(); it != markers.end(); ++it) {
		if ((*it)->type == ARMarker::NFT) {
            // Load KPM data.
            KpmRefDataSet *refDataSet2;
            logv(AR_LOG_LEVEL_INFO, "Reading %s.fset3", ((ARMarkerNFT *)(*it))->datasetPathname);
            if (kpmLoadRefDataSet(((ARMarkerNFT *)(*it))->datasetPathname, "fset3", &refDataSet2) < 0) {
                logv(AR_LOG_LEVEL_ERROR, "Error reading KPM data from %s.fset3", ((ARMarkerNFT *)(*it))->datasetPathname);
                ((ARMarkerNFT *)(*it))->pageNo = -1;
                continue;
            }
            ((ARMarkerNFT *)(*it))->pageNo = pageCount;
            logv(AR_LOG_LEVEL_INFO, "  Assigned page no. %d.", pageCount);
            if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, pageCount) < 0) {
                logv(AR_LOG_LEVEL_ERROR, "ARController::loadNFTData(): Error: kpmChangePageNoOfRefDataSet, exit(-1)");
                exit(-1);
            }
            if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
                logv(AR_LOG_LEVEL_ERROR, "ARController::loadNFTData(): Error: kpmMergeRefDataSet, exit(-1)");
                exit(-1);
            }
            logv(AR_LOG_LEVEL_INFO, "Done");

            // For convenience, create a weak reference to the AR2 data.
            surfaceSet[pageCount] = ((ARMarkerNFT *)(*it))->surfaceSet;

            pageCount++;
            if (pageCount == PAGES_MAX) {
                logv(AR_LOG_LEVEL_ERROR, "Maximum number of NFT pages (%d) loaded", PAGES_MAX);
                break;
            }
        }
    }
    if (kpmSetRefDataSet(m_kpmHandle, refDataSet) < 0) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::loadNFTData(): Error: kpmSetRefDataSet, exit(-1)");
        exit(-1);
    }
    kpmDeleteRefDataSet(&refDataSet);

    // Start the KPM tracking thread.
    logv(AR_LOG_LEVEL_INFO, "Starting NFT tracking thread.");
    trackingThreadHandle = trackingInitInit(m_kpmHandle);
    if (!trackingThreadHandle) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::loadNFTData(): trackingInitInit(), exit(-1)");
        exit(-1);
    }

    logv(AR_LOG_LEVEL_DEBUG, "Loading of NFT data complete, exiting, return true");
    return true;
}
#endif // HAVE_NFT

bool ARController::stopRunning()
{
	logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): called");
	if (state != DETECTION_RUNNING && state != WAITING_FOR_VIDEO) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::stopRunning(): Error: Not running.");
		return false;
	}

#if HAVE_NFT
    // Tracking thread is holding a reference to the camera parameters. Closing the
    // video source will dispose of the camera parameters, thus invalidating this reference.
    // So must stop tracking before closing the video source.
    if (trackingThreadHandle) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): calling unloadNFTData()");
        unloadNFTData();
    }
#endif

    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): called lockVideoSource()");
    if (m_videoSource0) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): if (m_videoSource0) true");
        if (m_videoSource0->isOpen()) {
            logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): calling m_videoSource0->close()");
            m_videoSource0->close();
        }
        delete m_videoSource0;
        m_videoSource0 = NULL;
    }

    if (m_videoSource1) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): if (m_videoSource1) true");
        if (m_videoSource1->isOpen()) {
            logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): calling m_videoSource1->close()");
            m_videoSource1->close();
        }
        delete m_videoSource1;
        m_videoSource1 = NULL;
    }
    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): called unlockVideoSource()");
	m_projectionMatrixSet = false;

#if HAVE_NFT
    // NFT cleanup.
    //logv("Cleaning up ARToolKit NFT handles.");
    if (m_ar2Handle) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): calling ar2DeleteHandle(&m_ar2Handle)");
        ar2DeleteHandle(&m_ar2Handle); // Sets m_ar2Handle to NULL.
    }
    if (m_kpmHandle) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): calling kpmDeleteHandle(&m_kpmHandle)");
        kpmDeleteHandle(&m_kpmHandle); // Sets m_kpmHandle to NULL.
    }
#endif

	//logv("Cleaning up ARToolKit handles.");
	if (m_ar3DHandle) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): calling ar3DDeleteHandle(&m_ar3DHandle)");
        ar3DDeleteHandle(&m_ar3DHandle); // Sets ar3DHandle0 to NULL.
    }
	if (m_ar3DStereoHandle) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): calling ar3DStereoDeleteHandle(&m_ar3DStereoHandle)");
        ar3DStereoDeleteHandle(&m_ar3DStereoHandle); // Sets ar3DStereoHandle to NULL.
    }

    if (m_arHandle0) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): if (m_arHandle0) true");
		arPattDetach(m_arHandle0);
		arDeleteHandle(m_arHandle0);
		m_arHandle0 = NULL;
	}

    if (m_arHandle1) {
        logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): if (m_arHandle1) true");
		arPattDetach(m_arHandle1);
		arDeleteHandle(m_arHandle1);
		m_arHandle1 = NULL;
	}

	state = BASE_INITIALISED;

    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::stopRunning(): exiting, returning true");
	return true;
}

bool ARController::shutdown()
{
    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::shutdown(): called");
    do {
        switch (state) {
            case DETECTION_RUNNING:
            case WAITING_FOR_VIDEO:
                logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::shutdown(): DETECTION_RUNNING or WAITING_FOR_VIDEO, forcing stop.");
                stopRunning();
            break;

            case BASE_INITIALISED:
                if (countMarkers() > 0) {
                    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::shutdown(): BASE_INITIALISED, cleaning up markers.");
                    removeAllMarkers();
                }

                if (m_arPattHandle) {
                    arPattDeleteHandle(m_arPattHandle);
                    m_arPattHandle = NULL;
                }

                state = NOTHING_INITIALISED;
                // Fall though.
            case NOTHING_INITIALISED:
                logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::shutdown(): NOTHING_INITIALISED, complete");
            break;
        }
    } while (state != NOTHING_INITIALISED);

    logv(AR_LOG_LEVEL_DEBUG, "ARWrapper::ARController::shutdown(): exiting, returning true");
	return true;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  State queries
// ----------------------------------------------------------------------------------------------------

bool ARController::getProjectionMatrix(const int videoSourceIndex, ARdouble proj[16])
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;

	if (m_projectionMatrixSet) {
		memcpy(proj, (videoSourceIndex == 0 ? m_projectionMatrix0 : m_projectionMatrix1), sizeof(ARdouble) * 16);
		return true;
	}

	return false;
}

bool ARController::canAddMarker()
{
	// Check we are in a valid state to add a marker (i.e. base initialisation has occurred)
	return (state != NOTHING_INITIALISED);
}

bool ARController::isRunning()
{
	return state == DETECTION_RUNNING;
}

bool ARController::videoParameters(const int videoSourceIndex, int *width, int *height, AR_PIXEL_FORMAT *pixelFormat)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return false;

    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!vs) return false;

    if (width) *width = vs->getVideoWidth();
    if (height) *height = vs->getVideoHeight();
    if (pixelFormat) *pixelFormat = vs->getPixelFormat();

    return true;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Configuration functions
// If an arHandle is current, it will be updated. If no arHandle is current, the value will be
// set on initialisation (in initAR()). The value can be queried at any time.
// ----------------------------------------------------------------------------------------------------
void ARController::setDebugMode(bool debug)
{
    debugMode = debug;
	if (m_arHandle0) {
		if (arSetDebugMode(m_arHandle0, debugMode ? AR_DEBUG_ENABLE : AR_DEBUG_DISABLE) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Debug mode set to %s", debug ? "on." : "off.");
		}
	}
	if (m_arHandle1) {
		if (arSetDebugMode(m_arHandle1, debugMode ? AR_DEBUG_ENABLE : AR_DEBUG_DISABLE) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Debug mode set to %s", debug ? "on." : "off.");
		}
	}
}

bool ARController::getDebugMode() const
{
    return debugMode;
}

void ARController::setImageProcMode(int mode)
{
    imageProcMode = mode;

    if (m_arHandle0) {
        if (arSetImageProcMode(m_arHandle0, mode) == 0) {
            logv(AR_LOG_LEVEL_INFO, "Image proc. mode set to %d.", imageProcMode);
        }
    }
    if (m_arHandle1) {
        if (arSetImageProcMode(m_arHandle1, mode) == 0) {
            logv(AR_LOG_LEVEL_INFO, "Image proc. mode set to %d.", imageProcMode);
        }
    }
}

int ARController::getImageProcMode() const
{
    return imageProcMode;
}

void ARController::setThreshold(int thresh)
{
    if (thresh < 0 || thresh > 255) return;
    threshold = thresh;
	if (m_arHandle0) {
		if (arSetLabelingThresh(m_arHandle0, threshold) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Threshold set to %d", threshold);
		}
	}
	if (m_arHandle1) {
		if (arSetLabelingThresh(m_arHandle1, threshold) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Threshold set to %d", threshold);
		}
	}
}

int ARController::getThreshold() const
{
    return threshold;
}

void ARController::setThresholdMode(int mode)
{
    thresholdMode = (AR_LABELING_THRESH_MODE)mode;
	if (m_arHandle0) {
		if (arSetLabelingThreshMode(m_arHandle0, thresholdMode) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Threshold mode set to %d", (int)thresholdMode);
		}
	}
	if (m_arHandle1) {
		if (arSetLabelingThreshMode(m_arHandle1, thresholdMode) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Threshold mode set to %d", (int)thresholdMode);
		}
	}
}

int ARController::getThresholdMode() const
{
    return (int)thresholdMode;
}

void ARController::setLabelingMode(int mode)
{
    labelingMode = mode;
	if (m_arHandle0) {
		if (arSetLabelingMode(m_arHandle0, labelingMode) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Labeling mode set to %d", labelingMode);
		}
	}
	if (m_arHandle1) {
		if (arSetLabelingMode(m_arHandle1, labelingMode) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Labeling mode set to %d", labelingMode);
		}
	}
}

int ARController::getLabelingMode() const
{
    return labelingMode;
}

void ARController::setPatternDetectionMode(int mode)
{
    patternDetectionMode = mode;
	if (m_arHandle0) {
		if (arSetPatternDetectionMode(m_arHandle0, patternDetectionMode) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Pattern detection mode set to %d.", patternDetectionMode);
		}
	}
	if (m_arHandle1) {
		if (arSetPatternDetectionMode(m_arHandle1, patternDetectionMode) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Pattern detection mode set to %d.", patternDetectionMode);
		}
	}
}

int ARController::getPatternDetectionMode() const
{
    return patternDetectionMode;
}

void ARController::setPattRatio(float ratio)
{
    if (ratio <= 0.0f || ratio >= 1.0f) return;
    pattRatio = (ARdouble)ratio;
	if (m_arHandle0) {
		if (arSetPattRatio(m_arHandle0, pattRatio) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Pattern ratio size set to %d.", pattRatio);
		}
	}
	if (m_arHandle1) {
		if (arSetPattRatio(m_arHandle1, pattRatio) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Pattern ratio size set to %d.", pattRatio);
		}
	}
}

float ARController::getPattRatio() const
{
    return (float)pattRatio;
}

void ARController::setMatrixCodeType(int type)
{
    matrixCodeType = (AR_MATRIX_CODE_TYPE)type;
	if (m_arHandle0) {
		if (arSetMatrixCodeType(m_arHandle0, matrixCodeType) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Matrix code type set to %d.", matrixCodeType);
		}
	}
	if (m_arHandle1) {
		if (arSetMatrixCodeType(m_arHandle1, matrixCodeType) == 0) {
			logv(AR_LOG_LEVEL_INFO, "Matrix code type set to %d.", matrixCodeType);
		}
	}
}

int ARController::getMatrixCodeType() const
{
    return (int)matrixCodeType;
}

void ARController::setNFTMultiMode(bool on)
{
#if HAVE_NFT
    m_nftMultiMode = on;
#endif
}

bool ARController::getNFTMultiMode() const
{
#if HAVE_NFT
    return m_nftMultiMode;
#else
    return false;
#endif
}

// ----------------------------------------------------------------------------------------------------
#pragma mark Debug texture
// ----------------------------------------------------------------------------------------------------

bool ARController::updateDebugTexture32(const int videoSourceIndex, uint32_t* buffer)
{
#ifdef AR_DISABLE_LABELING_DEBUG_MODE
    logv(AR_LOG_LEVEL_ERROR, "Debug texture not supported.");
    return false;
#else
	if (state != DETECTION_RUNNING) {
		logv(AR_LOG_LEVEL_ERROR, "Cannot update debug texture. Wrong state.");
		return false;
	}

	// Check everything is valid.
	if (!buffer) return false;
    ARHandle *arHandle = (videoSourceIndex == 1 ? m_arHandle1 : m_arHandle0);
    if (!arHandle) return false;
    if (!arHandle->labelInfo.bwImage) return false;

    uint8_t *src;
    uint32_t* dest = buffer;
    int h = arHandle->ysize;
    int arImageProcMode;
    arGetImageProcMode(arHandle, &arImageProcMode);
    if (arImageProcMode == AR_IMAGE_PROC_FIELD_IMAGE) {
        int wdiv2 = arHandle->xsize >> 1;
        for (int y = 0; y < h; y++) {
            src = &(arHandle->labelInfo.bwImage[(h >> 1) * wdiv2]);
            for (int x = 0; x < wdiv2; x++) {
                *dest = ((*src) << 24) + ((*src) << 16) + ((*src) << 8) + 255;
                dest++;
                *dest = ((*src) << 24) + ((*src) << 16) + ((*src) << 8) + 255;
                dest++;
                src++;
            }
        }
    } else {
        src = arHandle->labelInfo.bwImage;
        int w = arHandle->xsize;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                *dest = ((*src) << 24) + ((*src) << 16) + ((*src) << 8) + 255;
                src++;
                dest++;
            }
        }

    }
	return true;
#endif
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Marker list management functions.
// ----------------------------------------------------------------------------------------------------

int ARController::addMarker(const char* cfg)
{
	if (!canAddMarker()) {
		logv(AR_LOG_LEVEL_ERROR, "Error: Cannot add marker. ARToolKit not initialised");
		return -1;
	}

    ARMarker *marker = ARMarker::newWithConfig(cfg, m_arPattHandle);
    if (!marker) {
        logv(AR_LOG_LEVEL_ERROR, "Error: Failed to load marker.\n");
        return -1;
    }
    if (!addMarker(marker)) {
        return -1;
    }
    return marker->UID;
}

// private
bool ARController::addMarker(ARMarker* marker)
{
    logv(AR_LOG_LEVEL_DEBUG, "ARController::addMarker(): called");
	if (!canAddMarker()) {
        logv(AR_LOG_LEVEL_ERROR, "Error: Cannot add marker. ARToolKit not initialised, exiting, returning false");
		return false;
	}

	if (!marker) {
        logv(AR_LOG_LEVEL_ERROR, "Error: Cannot add a NULL marker, exiting, returning false");
		return false;
	}

    markers.push_back(marker);

#if HAVE_NFT
    if (marker->type == ARMarker::NFT) {
        if (!doNFTMarkerDetection)
            logv(AR_LOG_LEVEL_INFO, "First NFT marker added; enabling NFT marker detection.");
        doNFTMarkerDetection = true;
        if (trackingThreadHandle) {
            unloadNFTData(); // loadNFTData() will be called on next update().
        }
    } else {
#endif
        doMarkerDetection = true;
#if HAVE_NFT
    }
#endif

	logv(AR_LOG_LEVEL_INFO, "Added marker (UID=%d), total markers loaded: %d.", marker->UID, countMarkers());
	return true;
}

bool ARController::removeMarker(int UID)
{
    ARMarker *marker = findMarker(UID);
    if (!marker) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::removeMarker(): could not find marker (UID=%d).");
        return (false);
    }
	return removeMarker(marker);
}

// private
bool ARController::removeMarker(ARMarker* marker)
{
    logv(AR_LOG_LEVEL_DEBUG, "ARController::removeMarker(): called");
	if (!marker) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::removeMarker(): no marker specified, exiting, returning false");
		return false;
	}

	int UID = marker->UID;
    std::vector<ARMarker *>::iterator position = std::find(markers.begin(), markers.end(), marker);
    bool found = (position != markers.end());
    if (!found) {
        logv(AR_LOG_LEVEL_ERROR, "ARController::removeMarker(): Could not find marker (UID=%d), exiting, returning false", UID);
        return false;
    }
#if HAVE_NFT
    if (marker->type == ARMarker::NFT && trackingThreadHandle) {
        unloadNFTData(); // If at least 1 NFT marker remains, loadNFTData() will be called on next update().
    }
#endif

    delete marker; // std::vector does not call destructor if it's a raw pointer being stored, so explicitly delete it.
    markers.erase(position);

#if HAVE_NFT
    // Count each type of marker.
    int nftMarkerCount = 0;
    int squareMarkerCount = 0;
    std::vector<ARMarker *>::const_iterator it = markers.begin();
    while (it != markers.end()) {
        if ((*it)->type == ARMarker::NFT ) nftMarkerCount++;
        else squareMarkerCount++;
        ++it;
    }
    if (nftMarkerCount == 0) {
        if (doNFTMarkerDetection)
            logv(AR_LOG_LEVEL_INFO, "Last NFT marker removed; disabling NFT marker detection.");
        doNFTMarkerDetection = false;
    }
    if (squareMarkerCount == 0) {
        if (doMarkerDetection)
            logv(AR_LOG_LEVEL_INFO, "Last square marker removed; disabling square marker detection.");
        doMarkerDetection = false;
    }
    int markerCount = nftMarkerCount + squareMarkerCount;
#else
    int markerCount = countMarkers();
    if (markerCount == 0) {
        logv(AR_LOG_LEVEL_INFO, "Last square marker removed; disabling square marker detection.");
        doMarkerDetection = false;
    }
#endif

    logv(AR_LOG_LEVEL_INFO, "Removed marker (UID=%d), now %d markers loaded", UID, markerCount);
    logv(AR_LOG_LEVEL_DEBUG, "ARController::removeMarker(): exiting, returning %s", ((found) ? "true" : "false"));
    return (found);
}

int ARController::removeAllMarkers()
{
	unsigned int count = countMarkers();
#if HAVE_NFT
    if (trackingThreadHandle) {
        unloadNFTData();
    }
#endif
    markers.clear();
    doMarkerDetection = false;
#if HAVE_NFT
    doNFTMarkerDetection = false;
#endif
	logv(AR_LOG_LEVEL_INFO, "Removed all %d markers.", count);

	return count;
}

unsigned int ARController::countMarkers()
{
	return ((unsigned int)markers.size());
}

ARMarker* ARController::findMarker(int UID)
{

    std::vector<ARMarker *>::const_iterator it = markers.begin();
	while (it != markers.end()) {
		if ((*it)->UID == UID) return (*it);
        ++it;
	}
	return NULL;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Utility
// ----------------------------------------------------------------------------------------------------

/*private: static*/void ARController::logvBuf(va_list args, const char* format, char** bufPtr, int* lenPtr)
{
#   ifdef _WIN32
        // _vscprintf() - the value returned does not include the terminating null character.
        *lenPtr = _vscprintf(format, args);
        if (0 <= *lenPtr) {
            *bufPtr = (char *)malloc((*lenPtr + 1)*sizeof(char)); // +1 for nul-term.
            // vsnprintf() - the generated string has a length of at most n-1, leaving space
            //               for the additional terminating null character.
            vsnprintf(*bufPtr, *lenPtr + 1, format, args);
            (*bufPtr)[*lenPtr] = '\0'; // nul-terminate.
        }
#   else
        *lenPtr = vasprintf(bufPtr, format, args);
#   endif
}

/*private: static*/void ARController::logvWriteBuf(char* buf, int len, const int logLevel)
{
    // Pre-pend a tag onto the message to identify the source as the C++ wrapper
    std::string ErrOrWarnBuf;
    if (AR_LOG_LEVEL_ERROR == logLevel)
        ErrOrWarnBuf = "[error]";
    else if (AR_LOG_LEVEL_WARN == logLevel)
        ErrOrWarnBuf = "[warning]";
    else if (AR_LOG_LEVEL_INFO == logLevel)
        ErrOrWarnBuf = "[info]";
    else if (AR_LOG_LEVEL_DEBUG == logLevel)
        ErrOrWarnBuf = "[debug]";
    else
        ErrOrWarnBuf = "";

    len += sizeof(LOG_TAG) + 2 ; // +2 for ": ".
    char* Out = (char *)malloc(sizeof(char) * ((len + 1) + strlen(ErrOrWarnBuf.c_str()))); // +1 for nul.
    if (NULL != Out) {
        sprintf(Out, "%s: %s%s", LOG_TAG, ErrOrWarnBuf.c_str(), buf);
        // Pass message to log callback if it's valid
        logCallback(Out);
        free(Out);
    }
}

/*public: static*/void ARController::logv(const int logLevel, const char* format, ...)
{
    // Check input for NULL
    if (logLevel < arLogLevel) return;
    if (!format) return;
    if (!logCallback) return;

    // Unpack msg formatting.
    char *Buf = NULL;
    int Len;
    va_list Ap;

    va_start(Ap, format);
    logvBuf(Ap, format, &Buf, &Len);
    va_end(Ap);

    if (Len >= 0)
        logvWriteBuf(Buf, Len, logLevel);

    if (NULL != Buf)
        free(Buf);
}

/*public: static*/void ARController::logv(const char* format, ...)
{
	// Check input for NULL
	if (!format) return;
    if (!logCallback) return;

    // Unpack msg formatting.
    char* Buf = NULL;
    int Len;
    va_list Ap;

    va_start(Ap, format);
    logvBuf(Ap, format, &Buf, &Len);
    va_end(Ap);

    if (Len >= 0)
        logvWriteBuf(Buf, Len, AR_LOG_LEVEL_ERROR);

    if (NULL != Buf)
        free(Buf);
}

bool ARController::loadOpticalParams(const char *optical_param_name, const char *optical_param_buff, const long optical_param_buffLen, ARdouble *fovy_p,
                                     ARdouble *aspect_p, ARdouble m[16], ARdouble p[16])
{
    if (!fovy_p || !aspect_p || !m) return false;

    // Load the optical parameters.
    if (optical_param_name) {
        if (arParamLoadOptical(optical_param_name, fovy_p, aspect_p, m) < 0) {
            logv(AR_LOG_LEVEL_ERROR, "Error: loading optical parameters from file '%s'.\n", optical_param_name);
            return false;
        }
    } else if (optical_param_buff && optical_param_buffLen) {
        if (arParamLoadOpticalFromBuffer(optical_param_buff, optical_param_buffLen, fovy_p, aspect_p, m) < 0) {
            logv(AR_LOG_LEVEL_ERROR, "Error: loading optical parameters from buffer.\n");
            return false;
        }
    } else return false;

#ifdef DEBUG
    ARLOG("*** Optical parameters ***\n");
    arParamDispOptical(*fovy_p, *aspect_p, m);
#endif

    if (p) {
        // Create the OpenGL projection from the optical parameters.
        // We are using an optical see-through display, so
        // perspective is determined by its field of view and aspect ratio only.
        // This is the same calculation as performed by:
        // gluPerspective(fovy, aspect, nearPlane, farPlane);
#ifdef ARDOUBLE_IS_FLOAT
        mtxLoadIdentityf(p);
        mtxPerspectivef(p, *fovy_p, *aspect_p, m_projectionNearPlane, m_projectionFarPlane);
#else
        mtxLoadIdentityd(p);
        mtxPerspectived(p, *fovy_p, *aspect_p, m_projectionNearPlane, m_projectionFarPlane);
#endif
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------
#pragma mark  Android-only API
// ----------------------------------------------------------------------------------------------------

#if TARGET_PLATFORM_ANDROID
jint ARController::androidVideoPushInit(JNIEnv *env, jobject obj, jint videoSourceIndex, jint width, jint height, const char *pixelFormat, jint camera_index, jint camera_face)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return -1;
 
    int ret = -1;
    
    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);;
    if (!vs) {
        ARLOGe("ARController::androidVideoPushInit: no ARVideoSource.");
    } else {
        if (!vs->isOpen() || !vs->isRunning()) {
            ret = vs->androidVideoPushInit(env, obj, width, height, pixelFormat, camera_index, camera_face);
        } else {
            ARLOGe("ARController::androidVideoPushInit: ARVideoSource is either closed or already running.");
        }
    }
done:

    return ret;
}

jint ARController::androidVideoPush1(JNIEnv *env, jobject obj, jint videoSourceIndex, jbyteArray buf, jint bufSize)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return -1;

    int ret = -1;

    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!vs) {
        ARLOGe("ARController::androidVideoPush1: no ARVideoSource.");
    } else {
        if (vs->isRunning()) {
            ret = vs->androidVideoPush1(env, obj, buf, bufSize);
        } else {
            ARLOGe("ARController::androidVideoPush1: ARVideoSource is not running.");
        }
    }
done:

    return ret;
}

jint ARController::androidVideoPush2(JNIEnv *env, jobject obj, jint videoSourceIndex,
                                     jobject buf0, jint buf0PixelStride, jint buf0RowStride,
                                     jobject buf1, jint buf1PixelStride, jint buf1RowStride,
                                     jobject buf2, jint buf2PixelStride, jint buf2RowStride,
                                     jobject buf3, jint buf3PixelStride, jint buf3RowStride)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return -1;

    int ret = -1;

    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!vs) {
        ARLOGe("ARController::androidVideoPush2: no ARVideoSource.");
    } else {
        if (vs->isRunning()) {
            ret = vs->androidVideoPush2(env, obj, buf0, buf0PixelStride, buf0RowStride, buf1, buf1PixelStride, buf1RowStride, buf2, buf2PixelStride, buf2RowStride, buf3, buf3PixelStride, buf3RowStride);
        } else {
            ARLOGe("ARController::androidVideoPush2: ARVideoSource is not running.");
        }
    }
done:

    return ret;
}

jint ARController::androidVideoPushFinal(JNIEnv *env, jobject obj, jint videoSourceIndex)
{
    if (videoSourceIndex < 0 || videoSourceIndex > (m_videoSourceIsStereo ? 1 : 0)) return -1;

    int ret = -1;

    ARVideoSource *vs = (videoSourceIndex == 0 ? m_videoSource0 : m_videoSource1);
    if (!vs) {
        ARLOGe("ARController::androidVideoPushFinal: no ARVideoSource.");
    } else {
        if (vs->isOpen()) {
            ret = vs->androidVideoPushFinal(env, obj);
        } else {
            ARLOGe("ARController::androidVideoPushFinal: ARVideoSource is not open.");
        }
    }
done:

    return ret;
}
#endif // TARGET_PLATFORM_ANDROID

