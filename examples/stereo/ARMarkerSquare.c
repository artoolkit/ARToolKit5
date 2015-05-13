/*
 *	ARMarkerSquare.c
 *  ARToolKit5
 *
 *	Demonstration of ARToolKit NFT with models rendered in OSG,
 *  and marker pose estimates filtered to reduce jitter.
 *
 *  Press '?' while running for help on available key commands.
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not use, install, modify or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive
 *  license, under Daqri's copyrights in this original Daqri software (the
 *  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
 *  Software, with or without modifications, in source and/or binary forms;
 *  provided that if you redistribute the Daqri Software in its entirety and
 *  without modifications, you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri LLC. All Rights Reserved.
 *  Copyright 2011-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb.
 *
 */

#include "ARMarkerSquare.h"

#ifdef _WIN32
#  include <windows.h>
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h> // MAXPATHLEN
#endif
#include <stdlib.h> // calloc()
#include <string.h>

const ARPose ARPoseUnity = {{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};

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

void newMarkers(const char *markersConfigDataFilePathC, ARPattHandle *arPattHandle, ARMarkerSquare **markersSquare_out, int *markersSquareCount_out, int *patternDetectionMode_out)
{
    FILE          *fp;
    char           buf[MAXPATHLEN], buf1[MAXPATHLEN];
    int            tempI;
    ARMarkerSquare *markersSquare;
    int            markersSquareCount;
    ARdouble       tempF;
    int            i;
    char           markersConfigDataDirC[MAXPATHLEN];
    size_t         markersConfigDataDirCLen;
    int            patt_type = 0;

    if (!markersConfigDataFilePathC || markersConfigDataFilePathC[0] == '\0' || !markersSquareCount_out || !markersSquare_out) return;
    
    // Load the marker data file.
    ARLOGe("Opening marker config. data file from path '%s'.\n", markersConfigDataFilePathC);
    arUtilGetDirectoryNameFromPath(markersConfigDataDirC, markersConfigDataFilePathC, MAXPATHLEN, 1); // 1 = add '/' at end.
    markersConfigDataDirCLen = strlen(markersConfigDataDirC);
    if ((fp = fopen(markersConfigDataFilePathC, "r")) == NULL) {
        ARLOGe("Error: unable to locate marker config data file '%s'.\n", markersConfigDataFilePathC);
        return;
    }
    
    // First line is number of markers to read.
    get_buff(buf, MAXPATHLEN, fp, 1);
    if (sscanf(buf, "%d", &tempI) != 1 ) {
        ARLOGe("Error in marker configuration data file; expected marker count.\n");
        fclose(fp);
        return;
    }
    
    arMallocClear(markersSquare, ARMarkerSquare, tempI);
    markersSquareCount = tempI;
    
    ARLOGd("Reading %d marker configuration(s).\n", markersSquareCount);

    for (i = 0; i < markersSquareCount; i++) {
        
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
            markersSquare[i].valid = markersSquare[i].validPrev = FALSE;

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
                    ARLOGe("Error: Marker pattern file '%s' specified but only barcodes allowed.\n", markersSquare[i].patternPathname);
                    break;
                }
                arMalloc(markersSquare[i].patternPathname, char, markersConfigDataDirCLen + strlen(buf) + 1);
                strcpy(markersSquare[i].patternPathname, markersConfigDataDirC);
                strcpy(markersSquare[i].patternPathname + markersConfigDataDirCLen, buf);
                // Now load pattern file.
                if ((markersSquare[i].patt_id = arPattLoad(arPattHandle, markersSquare[i].patternPathname)) < 0 ) {
                    ARLOGe("Error: Unable to load marker pattern from file'%s'.\n", markersSquare[i].patternPathname);
                    break;
                }
                markersSquare[i].marker_width = markersSquare[i].marker_height = tempF;
                markersSquare[i].patt_type = AR_PATTERN_TYPE_TEMPLATE;
                patt_type |= 0x01;
            } else {
                markersSquare[i].patternPathname = NULL;
                markersSquare[i].patt_id = tempI;
                markersSquare[i].marker_width = markersSquare[i].marker_height = tempF;
                markersSquare[i].patt_type = AR_PATTERN_TYPE_MATRIX;
                patt_type |= 0x02;
            }

        } else if (strcmp(buf1, "MULTI") == 0) {
            ARLOGe("Error in marker configuration data file; MULTI markers not supported in this build.\n");
        } else if (strcmp(buf1, "NFT") == 0) {
            ARLOGe("Error in marker configuration data file; NFT markers not supported in this build.\n");
        } else {
            ARLOGe("Error in marker configuration data file; unsupported marker type %s.\n", buf1);
        }
        
        // Look for optional tokens. A blank line marks end of options.
        while (get_buff(buf, MAXPATHLEN, fp, 0) && (buf[0] != '\0')) {
            if (strncmp(buf, "FILTER", 6) == 0) {
                markersSquare[i].filterCutoffFrequency = AR_FILTER_TRANS_MAT_CUTOFF_FREQ_DEFAULT;
                markersSquare[i].filterSampleRate = AR_FILTER_TRANS_MAT_SAMPLE_RATE_DEFAULT;
                if (strlen(buf) != 6) {
                    if (sscanf(&buf[6],
#ifdef ARDOUBLE_IS_FLOAT
                               "%f"
#else
                               "%lf"
#endif
                               , &tempF) == 1) markersSquare[i].filterCutoffFrequency = tempF;
                }
                markersSquare[i].ftmi = arFilterTransMatInit(markersSquare[i].filterSampleRate, markersSquare[i].filterCutoffFrequency);
            }
            // Unknown tokens are ignored.
        }
    }
    fclose(fp);
    
    // If not all markers were read, an error occurred.
    if (i < markersSquareCount) {
    
        // Clean up.
        for (; i >= 0; i--) {
            if (markersSquare[i].patt_type == AR_PATTERN_TYPE_TEMPLATE && markersSquare[i].patt_id && arPattHandle) arPattFree(arPattHandle, markersSquare[i].patt_id);
            if (markersSquare[i].patternPathname) free(markersSquare[i].patternPathname);
            if (markersSquare[i].ftmi) arFilterTransMatFinal(markersSquare[i].ftmi);
        }
        free(markersSquare);

        *markersSquareCount_out = 0;
        *markersSquare_out = NULL;
        if (patternDetectionMode_out) *patternDetectionMode_out = -1;
        return;
    }
    
    *markersSquareCount_out = markersSquareCount;
    *markersSquare_out = markersSquare;
    if (patternDetectionMode_out) {
        // Work out square marker detection mode.
        if ((patt_type & 0x03) == 0x03) *patternDetectionMode_out = AR_TEMPLATE_MATCHING_COLOR_AND_MATRIX;
        else if (patt_type & 0x02)      *patternDetectionMode_out = AR_MATRIX_CODE_DETECTION;
        else                            *patternDetectionMode_out = AR_TEMPLATE_MATCHING_COLOR;
    } 
}

void deleteMarkers(ARMarkerSquare **markersSquare_p, int *markersSquareCount_p, ARPattHandle *arPattHandle)
{
    int i;
    
    if (!markersSquare_p || !*markersSquare_p || !*markersSquareCount_p || *markersSquareCount_p < 1) return;
    
    for (i = 0; i < *markersSquareCount_p; i++) {
        if ((*markersSquare_p)[i].patt_type == AR_PATTERN_TYPE_TEMPLATE && (*markersSquare_p)[i].patt_id && arPattHandle) arPattFree(arPattHandle, (*markersSquare_p)[i].patt_id);
        if ((*markersSquare_p)[i].patternPathname) {
            free((*markersSquare_p)[i].patternPathname);
            (*markersSquare_p)[i].patternPathname = NULL;
        }
        if ((*markersSquare_p)[i].ftmi) {
            arFilterTransMatFinal((*markersSquare_p)[i].ftmi);
            (*markersSquare_p)[i].ftmi = NULL;
        }
    }
    free(*markersSquare_p);
    *markersSquare_p = NULL;
    *markersSquareCount_p = 0;
}
