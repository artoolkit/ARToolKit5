/*
 *  calib_optical.h
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
 *  Copyright 2015 Daqri LLC. All Rights Reserved.
 *  Copyright 2012-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb
 */

#ifndef CALIB_OPTICAL_H
#define CALIB_OPTICAL_H

#include <AR/ar.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VIEW_LEFTEYE        = 1,
	VIEW_RIGHTEYE       = 2,
} VIEW_EYE_t;


bool captureInit(const VIEW_EYE_t calibrationEye);
bool capture(int *co1_p, int *co2_p);
bool captureUndo(int *co1_p, int *co2_p);
int calib(ARdouble *fovy_p, ARdouble *aspect_p, ARdouble m[16]);
void saveParam(const char *paramPathname, ARdouble fovy, ARdouble aspect, const ARdouble m[16]);

#ifdef __cplusplus
}
#endif
#endif // !CALIB_OPTICAL_H
