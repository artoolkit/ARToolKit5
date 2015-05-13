/*
 *  arViewerCapi.cpp
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
 *  Copyright 2002-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Raphael Grasset, Philip Lamb
 *
 */

#include <AR/ar.h>
#include <AR/arvrml.h>
#include "arViewer.h"
#include <iostream>
#include <vector>
#include <string>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <stdio.h>
#include <string.h>

//extern "C" {

//}


#define  AR_VRML_MAX   100

static arVrmlViewer    *viewer[AR_VRML_MAX];
static int              init = 1;
static int              vrID = -1;

static char *get_buff( char *buf, int n, FILE *fp );


int arVrmlLoadFile(const char *file)
{
  
    FILE             *fp;
    openvrml::browser * myBrowser = NULL;
    char             buf[256], buf1[256];
    char             buf2[256];
    int              id;
    int              i;

    if( init ) {
        for( i = 0; i < AR_VRML_MAX; i++ ) viewer[i] = NULL;
        init = 0;
    }
    for( i = 0; i < AR_VRML_MAX; i++ ) {
        if( viewer[i] == NULL ) break;
    }
    if( i == AR_VRML_MAX ) return -1;
    id = i;

    if ((fp = fopen(file, "r")) == NULL) {
        ARLOGe("Error: unable to open model description file %s.\n", file);
        return (-1);
    }

    get_buff(buf, 256, fp);
    if( sscanf(buf, "%s", buf1) != 1 ) {fclose(fp); return -1;}
    for( i = 0; file[i] != '\0'; i++ ) buf2[i] = file[i];
    for( ; i >= 0; i-- ) {
        if( buf2[i] == '/' ) break;
    }
    buf2[i+1] = '\0';
    sprintf(buf, "%s%s", buf2, buf1);

    myBrowser = new arVrmlBrowser;
    if (!myBrowser) {
		fclose(fp);
		return -1;
	}

    viewer[id] = new arVrmlViewer();
    if (!viewer[id]) {
        delete myBrowser;
        fclose(fp);
        return -1;
    }
    strcpy(viewer[id]->filename, buf); // Save filename in viewer.
	myBrowser->viewer(viewer[id]);
	
	std::vector<std::string> uri(1, buf);
    std::vector<std::string> parameter; 
    myBrowser->load_url(uri, parameter);
	
    
    get_buff(buf, 256, fp);
    if( sscanf(buf, "%lf %lf %lf", &viewer[id]->translation[0], 
        &viewer[id]->translation[1], &viewer[id]->translation[2]) != 3 ) {
        delete viewer[id];
        viewer[id] = NULL;
        fclose(fp);
        return -1;
    }

    get_buff(buf, 256, fp);
    if( sscanf(buf, "%lf %lf %lf %lf", &viewer[id]->rotation[0],
        &viewer[id]->rotation[1], &viewer[id]->rotation[2], &viewer[id]->rotation[3]) != 4 ) {
        delete viewer[id];
        viewer[id] = NULL;
        fclose(fp);
        return -1;
    }

    get_buff(buf, 256, fp);
    if( sscanf(buf, "%lf %lf %lf", &viewer[id]->scale[0], &viewer[id]->scale[1],
               &viewer[id]->scale[2]) != 3 ) {
        delete viewer[id];
        viewer[id] = NULL;
        fclose(fp);
        return -1;
    }
    fclose(fp);

    return id;
}

int arVrmlFree( int id )
{
    if( viewer[id] == NULL ) return -1;

    delete viewer[id];
    viewer[id] = NULL;

    if( vrID == id ) {
        vrID = -1;
    }

    return 0;
}

int arVrmlTimerUpdate()
{
     int     i;

    for( i = 0; i < AR_VRML_MAX; i++ ) {
        if( viewer[i] == NULL ) continue;
        viewer[i]->timerUpdate();
    }
    return 0;
}

int arVrmlDraw( int id )
{
     if( viewer[id] == NULL ) return -1;
     viewer[id]->redraw();
     return 0;
}

int arVrmlSetInternalLight( int flag )
{
   int     i;

    if( flag ) {
        for( i = 0; i < AR_VRML_MAX; i++ ) {
            if( viewer[i] == NULL ) continue;
            viewer[i]->setInternalLight(true);
        }
    }
    else {
        for( i = 0; i < AR_VRML_MAX; i++ ) {
            if( viewer[i] == NULL ) continue;
            viewer[i]->setInternalLight(false);
        }
    }
  
    return 0;
}

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
