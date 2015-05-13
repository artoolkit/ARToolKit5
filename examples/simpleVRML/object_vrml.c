/*
 *	object_vrml.c
 *
 *  ARToolKit object parsing function 
 *  - reads in object data from object file in Data/object_data
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
 *  Copyright 2002-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AR/ar.h>
#include <AR/arvrml.h>
#include "object_vrml.h"

static char *get_buff(char *buf, int n, FILE *fp)
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
    } while (buf[0] == '#' || buf[0] == '\0'); // Reject comments and blank lines.
    
    return (ret);
}

ObjectData_T *read_VRMLdata(char *name, int *objectnum)
{
    FILE          *fp;
    ObjectData_T  *object;
    char           buf[256], buf1[256];
    int            i;

	ARLOGi("Opening model file %s\n", name);

    if ((fp=fopen(name, "r")) == NULL) return(NULL);

    get_buff(buf, 256, fp);
    if (sscanf(buf, "%d", objectnum) != 1) {
		fclose(fp); return(NULL);
	}

	ARLOGi("Loading %d models.\n", *objectnum);

    if (!(object = (ObjectData_T *)malloc(sizeof(ObjectData_T) * (*objectnum)))) goto bail;
	if (!(object->pattHandle = arPattCreateHandle())) {
		ARLOGe("Error: Couldn't create pattern handle.\n");
		goto bail1;
	};
	
    for (i = 0; i < *objectnum; i++) {
		
        get_buff(buf, 256, fp);
        if (sscanf(buf, "%s %s", buf1, object[i].name) != 2) {
            ARLOGe("Error reading config file; expected object type and file name.\n");
            goto bail2;
        }
		ARLOGi("Model %d: %20s\n", i + 1, &(object[i].name[0]));
		
        if (strcmp(buf1, "VRML") == 0) {
            object[i].vrml_id = arVrmlLoadFile(object[i].name);
            if (object[i].vrml_id < 0) {
            	ARLOGe("VRML load error!! Unable to load %s.\n", object[i].name);
                goto bail2;
            }
			ARLOGi("- VRML id %d \n", object[i].vrml_id);
        } else {
			object[i].vrml_id = -1;
		}
		object[i].vrml_id_orig = object[i].vrml_id;
		object[i].visible = 0;

        get_buff(buf, 256, fp);
        if (sscanf(buf, "%s", buf1) != 1) {
            ARLOGe("Error reading config file; expected pattern file name.\n");
			goto bail3;
		}
        if ((object[i].id = arPattLoad(object->pattHandle, buf1)) < 0) {
            ARLOGe("Pattern load error!! Unable to load %s.\n", buf1);
			goto bail3;
		}

        get_buff(buf, 256, fp);
        if (sscanf(buf, "%lf", &object[i].marker_width) != 1) {
            ARLOGe("Error reading config file; expected marker width.\n");
			goto bail4;
		}

        get_buff(buf, 256, fp);
        if (sscanf(buf, "%lf %lf", &object[i].marker_center[0], &object[i].marker_center[1]) != 2) {
            ARLOGe("Error reading config file; expected marker center x and center y.\n");
			goto bail4;
        }
        
    }

    fclose(fp);

    return( object );

bail4:
	arPattFree(object->pattHandle, object[i].id);
bail3:
	if (object[i].vrml_id != -1) arVrmlFree(object[i].vrml_id);
bail2:
	// Unload anything loaded before the error.
	for (i--; i >= 0; i--) {
		arPattFree(object->pattHandle, object[i].id);
		if (object[i].vrml_id != -1) arVrmlFree(object[i].vrml_id);
	}
	arPattDeleteHandle(object->pattHandle);
bail1:
	free(object);
bail:
	fclose(fp);
	return (NULL);
}
