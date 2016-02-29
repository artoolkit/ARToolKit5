/*
 *  ARMarker.cpp
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
 *  Author(s): Julian Looser, Philip Lamb.
 *
 */

#include <ARWrapper/ARMarker.h>
#include <ARWrapper/ARMarkerSquare.h>
#include <ARWrapper/ARMarkerMulti.h>
#if HAVE_NFT
#  include <ARWrapper/ARMarkerNFT.h>
#endif
#include <ARWrapper/ARController.h>
#if TARGET_PLATFORM_ANDROID || TARGET_PLATFORM_IOS
#  include <AR/gsub_es.h>
#elif TARGET_PLATFORM_WINRT
#  include <AR/gsub_es2.h>
#else
#  include <AR/gsub_lite.h>
#endif

#ifdef _WIN32
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h>
#endif

static char *get_buff(char *buf, int n, FILE *fp, int skipblanks)
{
    char *ret;
	size_t l;
    
    do {
        ret = fgets(buf, n, fp);
        if (ret == NULL) return (NULL); // EOF or error.
        
        // Remove NLs and CRs from end of string.
        l = strlen(buf);
        while (l > 0) {
            if (buf[l - 1] != '\n' && buf[l - 1] != '\r') break;
            l--;
            buf[l] = '\0';
        }
    } while (buf[0] == '#' || (skipblanks && buf[0] == '\0')); // Reject comments and blank lines.
    
    return (ret);
}


std::vector<ARMarker *> ARMarker::newFromConfigDataFile(const char *markersConfigDataFilePath, ARPattHandle *arPattHandle, int *patternDetectionMode_out)
{
    char           markersConfigDataDir[MAXPATHLEN];
    size_t         markersConfigDataDirLen;
    int            numMarkers;
    std::vector<ARMarker *> markers;
    FILE          *fp;
    char           buf[MAXPATHLEN], buf1[MAXPATHLEN];
    int            tempI;
    ARdouble       tempF;
    int            i;
    int            patt_type = 0;
    
    if (!markersConfigDataFilePath || markersConfigDataFilePath[0] == '\0') return markers; // Return empty vector.
    
    // Load the marker data file.
    ARLOGd("Opening marker config. data file from path '%s'.\n", markersConfigDataFilePath);
    arUtilGetDirectoryNameFromPath(markersConfigDataDir, markersConfigDataFilePath, MAXPATHLEN, 1); // 1 = add '/' at end.
    markersConfigDataDirLen = strlen(markersConfigDataDir);
    if ((fp = fopen(markersConfigDataFilePath, "r")) == NULL) {
        ARLOGe("Error: unable to locate marker config data file '%s'.\n", markersConfigDataFilePath);
        return markers; // Return empty vector.
    }
    
    // First line is number of markers to read.
    get_buff(buf, MAXPATHLEN, fp, 1);
    if (sscanf(buf, "%d", &numMarkers) != 1 ) {
        ARLOGe("Error in marker configuration data file; expected marker count.\n");
        fclose(fp);
        return markers; // Return empty vector.;
    }
    
    markers.reserve(numMarkers);
    
    ARLOGd("Reading %d marker configuration(s).\n", numMarkers);
    
    for (i = 0; i < numMarkers; i++) {
        
        ARMarker *tempObject = NULL;
        
        // Read marker name.
        if (!get_buff(buf, MAXPATHLEN, fp, 1)) {
            ARLOGe("Error in marker configuration data file; expected marker name.\n");
            break;
        }
        
        // Read marker type.
        if (!get_buff(buf1, MAXPATHLEN, fp, 1)) {
            ARLOGe("Error in marker configuration data file; expected marker type.\n");
            break;
        }
        
        // Interpret marker type, and read more data.
        if (strcmp(buf1, "SINGLE") == 0) {
            
            // Read marker width.
            if (!get_buff(buf1, MAXPATHLEN, fp, 1) || sscanf(buf1,
#ifdef ARDOUBLE_IS_FLOAT
                                                             "%f"
#else
                                                             "%lf"
#endif
                                                             , &tempF) != 1) {
                ARLOGe("Error in marker configuration data file; expected marker width.\n");
                break;
            }
            
            // Interpret marker name (still in buf), test if it's a pattern number, load as pattern file if not.
            if (sscanf(buf, "%d", &tempI) != 1) {
                if (!arPattHandle) {
                    ARLOGe("Error: Marker pattern file '%s' specified but only barcodes allowed.\n", buf);
                    break;
                }
                
                strncpy(markersConfigDataDir + markersConfigDataDirLen, buf, MAXPATHLEN - markersConfigDataDirLen - 1); markersConfigDataDir[MAXPATHLEN - 1] = '\0';
                
                tempObject = new ARMarkerSquare();
                if (!((ARMarkerSquare *)tempObject)->initWithPatternFile(markersConfigDataDir, tempF, arPattHandle)) {
                    // Marker failed to load, or was not added
                    delete tempObject;
                    tempObject = NULL;
                }
                patt_type |= 0x01;
                
            } else {
                
                tempObject = new ARMarkerSquare();
                if (!((ARMarkerSquare *)tempObject)->initWithBarcode(tempI, tempF)) {
                    // Marker failed to load, or was not added
                    delete tempObject;
                    tempObject = NULL;
                }
                patt_type |= 0x02;
            }
            
        } else if (strcmp(buf1, "MULTI") == 0) {
            
            strncpy(markersConfigDataDir + markersConfigDataDirLen, buf, MAXPATHLEN - markersConfigDataDirLen - 1); markersConfigDataDir[MAXPATHLEN - 1] = '\0';
            
            tempObject = new ARMarkerMulti();
            if (!((ARMarkerMulti *)tempObject)->load(markersConfigDataDir, arPattHandle)) {
                // Marker failed to load, or was not added
                delete tempObject;
                tempObject = NULL;
            }
#if HAVE_NFT
        } else if (strcmp(buf1, "NFT") == 0) {
            
            strncpy(markersConfigDataDir + markersConfigDataDirLen, buf, MAXPATHLEN - markersConfigDataDirLen - 1); markersConfigDataDir[MAXPATHLEN - 1] = '\0';
            
            tempObject = new ARMarkerNFT();
            if (!((ARMarkerNFT *)tempObject)->load(markersConfigDataDir)) {
                // Marker failed to load, or was not added
                delete tempObject;
                tempObject = NULL;
            }
#endif
        } else {
            ARLOGe("Error in marker configuration data file; unsupported marker type %s.\n", buf1);
        }
        
        // Look for optional tokens. A blank line marks end of options.
        while (get_buff(buf, MAXPATHLEN, fp, 0) && (buf[0] != '\0')) {
            if (strncmp(buf, "FILTER", 6) == 0) {
                if (tempObject) {
                    if (strlen(buf) != 6) {
                        if (sscanf(&buf[6],
#ifdef ARDOUBLE_IS_FLOAT
                                   "%f"
#else
                                   "%lf"
#endif
                                   , &tempF) == 1) tempObject->setFilterCutoffFrequency(tempF);
                    }
                    tempObject->setFiltered(true);
                }
            }
            // Unknown tokens are ignored.
        }
        
        if (tempObject) {
            markers.push_back(tempObject);
        }
    }
    
    // Work out square marker detection mode.
    if (patternDetectionMode_out) {
        if ((patt_type & 0x03) == 0x03) *patternDetectionMode_out = AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX;
        else if (patt_type & 0x02)      *patternDetectionMode_out = AR_MATRIX_CODE_DETECTION;
        else                            *patternDetectionMode_out = AR_TEMPLATE_MATCHING_COLOR;
    }

    fclose(fp);
    
    return markers;
}


// single;data/hiro.patt;80
// single_buffer;80;buffer=234 221 237...
// single_barcode;0;80
// multi;data/multi/marker.dat
// nft;data/nft/pinball

ARMarker* ARMarker::newWithConfig(const char* cfg, ARPattHandle *arPattHandle)
{
    ARMarker *markerRet = NULL;
    
	// Ensure the configuration string exists
	if (!cfg) return NULL;
    
    // strtok modifies the string, so need to copy it.
	const char *bufferStart = strstr(cfg, ";buffer=");
	char *cfgCopy;
	if (bufferStart) {
        arMalloc(cfgCopy, char, bufferStart - cfg + 1); // +1 leaves space for nul terminator.
		strncpy(cfgCopy, cfg, (size_t)(bufferStart - cfg)); cfgCopy[bufferStart - cfg] = '\0';
	} else {
        arMalloc(cfgCopy, char, strlen(cfg) + 1);
		strcpy(cfgCopy, cfg);
	}
    
	// First token is the marker type
	if (char* markerTypePtr = strtok (cfgCopy, ";")) {
		
		if (strcmp(markerTypePtr, "single") == 0) {
            
			// Single ARMarker, second token is the pattern file.
			if (char *path = strtok(NULL, ";")) {
                // Third token is the marker width.
                if (char *widthPtr = strtok(NULL, ";")) {
                    ARdouble width = 0.0f;
#ifdef ARDOUBLE_IS_FLOAT
                    if (sscanf(widthPtr, "%f", &width) == 1)
#else
                        if (sscanf(widthPtr, "%lf", &width) == 1)
#endif
                        {
                            markerRet = new ARMarkerSquare();
                            if (!((ARMarkerSquare *)markerRet)->initWithPatternFile(path, width, arPattHandle)) {
                                // Marker failed to load, or was not added
                                delete markerRet;
                                markerRet = NULL;
                            }
                        }
				}
			}
            
		} else if (strcmp(markerTypePtr, "single_buffer") == 0) {
			
            // ARMarkerSquare, second token is the marker width.
			if (bufferStart) {
                // Single ARMarker, second token is the marker width.
                if (char *widthPtr = strtok(NULL, ";")) {
                    ARdouble width = 0.0f;
#ifdef ARDOUBLE_IS_FLOAT
                    if (sscanf(widthPtr, "%f", &width) == 1)
#else
                        if (sscanf(widthPtr, "%lf", &width) == 1)
#endif
                        {
                            markerRet = new ARMarkerSquare();
                            if (!((ARMarkerSquare *)markerRet)->initWithPatternFromBuffer(bufferStart + 8, width, arPattHandle)) {
                                // Marker failed to load, or was not added
                                delete markerRet;
                                markerRet = NULL;
                            }
                        }
				}
            }
            
		} else if (strcmp(markerTypePtr, "single_barcode") == 0) {
			
			// ARMarkerSquare, second token is the barcode ID.
			if (char *barcodeIDPtr = strtok(NULL, ";")) {
                int barcodeID = -1;
                if (sscanf(barcodeIDPtr, "%d", &barcodeID) == 1) {
                    // Third token is the marker width.
                    if (char *widthPtr = strtok(NULL, ";")) {
                        ARdouble width = 0.0f;
#ifdef ARDOUBLE_IS_FLOAT
                        if (sscanf(widthPtr, "%f", &width) == 1)
#else
                            if (sscanf(widthPtr, "%lf", &width) == 1)
#endif
                            {
                                markerRet = new ARMarkerSquare();
                                if (!((ARMarkerSquare *)markerRet)->initWithBarcode(barcodeID, width)) {
                                    // Marker failed to load, or was not added
                                    delete markerRet;
                                    markerRet = NULL;
                                }
                            }
                    }
                }
			}
            
            
		} else if (strcmp(markerTypePtr, "multi") == 0) {
            
			// ARMarkerMultiSquare, second token is the multimarker config file.
			if (char *config = strtok(NULL, ";")) {
                markerRet = new ARMarkerMulti();
                if (!((ARMarkerMulti *)markerRet)->load(config, arPattHandle)) {
                    // Marker failed to load, or was not added
                    delete markerRet;
                    markerRet = NULL;
                }
			}
            
		} else if (strcmp(markerTypePtr, "nft") == 0) {
#if HAVE_NFT
			// NFT AR Marker, second token is the NFT data path base.
			if (char *config = strtok(NULL, ";")) {
                markerRet = new ARMarkerNFT();
                if (!((ARMarkerNFT *)markerRet)->load(config)) {
                    // Marker failed to load, or was not added
                    delete markerRet;
                    markerRet = NULL;
                }
			}
#else
            ARController::logv(AR_LOG_LEVEL_ERROR, "Error: NFT markers not supported in this build/platform.\n");
#endif // HAVE_NFT
		} else {
            
			// Unknown marker type
			ARController::logv(AR_LOG_LEVEL_ERROR, "Error: Unknown marker type '%s' in config '%s'.", markerTypePtr, cfg);
		}
        
	}
    
    free(cfgCopy);
    return (markerRet);
}

ARMarker::ARMarker(MarkerType type) :
    m_ftmi(NULL),
    m_filterCutoffFrequency(AR_FILTER_TRANS_MAT_CUTOFF_FREQ_DEFAULT),
    m_filterSampleRate(AR_FILTER_TRANS_MAT_SAMPLE_RATE_DEFAULT),
#ifdef ARDOUBLE_IS_FLOAT
    m_positionScaleFactor(1.0f),
#else
    m_positionScaleFactor(1.0),
#endif
    type(type),
    visiblePrev(false),
	visible(false),
    patternCount(0),
    patterns(NULL)
{
	static int nextUID = 0;
	UID = nextUID++;
}

ARMarker::~ARMarker()
{
    freePatterns();

    if (m_ftmi) arFilterTransMatFinal(m_ftmi);
}

void ARMarker::allocatePatterns(int count)
{
	freePatterns();

    if (count) {
        //ARController::logv("Allocating %d patterns on marker %d", patternCount, UID);
        patternCount = count;
        patterns = new ARPattern*[patternCount];
        for (int i = 0; i < patternCount; i++) {
            patterns[i] = new ARPattern();
        }        
    }
}

void ARMarker::freePatterns()
{
	//if (patternCount) ARController::logv("Freeing %d patterns on marker %d", patternCount, UID);

	for (int i = 0; i < patternCount; i++) {
        if (patterns[i]) {
            delete patterns[i];
            patterns[i] = NULL;
        }
	}
	if (patterns) {
        delete[] patterns;
        patterns = NULL;
    }

	patternCount = 0;
}

ARPattern* ARMarker::getPattern(int n)
{
	// Check n is in acceptable range
	if (!patterns || n < 0 || n >= patternCount) return NULL;

	return patterns[n];
}

void ARMarker::setPositionScalefactor(ARdouble scale)
{
    m_positionScaleFactor = scale;
}

ARdouble ARMarker::positionScalefactor()
{
    return m_positionScaleFactor;
}

bool ARMarker::update(const ARdouble transL2R[3][4])
{
    // Subclasses will have already determined visibility and set/cleared 'visible' and 'visiblePrev',
    // as well as setting 'trans'.
    if (visible) {
        
        // Filter the pose estimate.
        if (m_ftmi) {
            if (arFilterTransMat(m_ftmi, trans, !visiblePrev) < 0) {
                ARController::logv(AR_LOG_LEVEL_ERROR, "arFilterTransMat error with marker %d.\n", UID);
            }
        }
        
        if (!visiblePrev) {
            ARController::logv(AR_LOG_LEVEL_INFO, "Marker %d now visible", UID);
        }
        
        // Convert to GL matrix.
#ifdef ARDOUBLE_IS_FLOAT
        arglCameraViewRHf(trans, transformationMatrix, m_positionScaleFactor);
#else
        arglCameraViewRH(trans, transformationMatrix, m_positionScaleFactor);
#endif

        // Do stereo if required.
        if (transL2R) {
            ARdouble transR[3][4];
            
            arUtilMatMul(transL2R, trans, transR);
#ifdef ARDOUBLE_IS_FLOAT
            arglCameraViewRHf(transR, transformationMatrixR, m_positionScaleFactor);
#else
            arglCameraViewRH(transR, transformationMatrixR, m_positionScaleFactor);
#endif
            
        }
    } else {
        
        if (visiblePrev) {
            ARController::logv(AR_LOG_LEVEL_INFO, "Marker %d no longer visible", UID);
        }
        
    }
    
    return true;
}

void ARMarker::setFiltered(bool flag)
{
    if (flag && !m_ftmi) {
        m_ftmi = arFilterTransMatInit(m_filterSampleRate, m_filterCutoffFrequency);
    } else if (!flag && m_ftmi) {
        arFilterTransMatFinal(m_ftmi);
        m_ftmi = NULL;
    }
}

bool ARMarker::isFiltered()
{
    return (m_ftmi != NULL);
}

ARdouble ARMarker::filterSampleRate()
{
    return m_filterSampleRate;
}

void ARMarker::setFilterSampleRate(ARdouble rate)
{
    m_filterSampleRate = rate;
    if (m_ftmi) arFilterTransMatSetParams(m_ftmi, m_filterSampleRate, m_filterCutoffFrequency);
}

ARdouble ARMarker::filterCutoffFrequency()
{
    return m_filterCutoffFrequency;
}

void ARMarker::setFilterCutoffFrequency(ARdouble freq)
{
    m_filterCutoffFrequency = freq;
    if (m_ftmi) arFilterTransMatSetParams(m_ftmi, m_filterSampleRate, m_filterCutoffFrequency);
}

