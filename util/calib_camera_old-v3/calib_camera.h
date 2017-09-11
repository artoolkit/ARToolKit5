/*
 *  calib_camera.h
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
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef CALIB_DIST_H
#define CALIB_DIST_H

#if defined(ARVIDEO_INPUT_DEFAULT_V4L2)
#  define VCONF  "-width=640 -height=480"
#elif defined(ARVIDEO_INPUT_DEFAULT_1394)
#  if ARVIDEO_INPUT_1394CAM_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_MONO
#    define VCONF  "-mode=640x480_MONO"
#  elif defined(ARVIDEO_INPUT_1394CAM_USE_DRAGONFLY)
#    define VCONF  "-mode=640x480_MONO_COLOR"
#  else
#    define VCONF  "-mode=640x480_YUV411"
#  endif
#else
#  define VCONF  ""
#endif





#define  H_NUM        6
#define  V_NUM        4
#define  LOOP_MAX    20
#define  THRESH     100

typedef struct {
    ARdouble   x_coord;
    ARdouble   y_coord;
} CALIB_COORD_T;

typedef struct patt {
    unsigned char  *savedImage[LOOP_MAX];
    CALIB_COORD_T  *world_coord;
    CALIB_COORD_T  *point[LOOP_MAX];
    int            h_num;
    int            v_num;
    int            loop_num;
} CALIB_PATT_T;

void calc_distortion( CALIB_PATT_T *patt, int xsize, int ysize, ARdouble aspect_ratio, ARdouble dist_factor[], int dist_function_version );
int  calc_inp( CALIB_PATT_T *patt, ARdouble dist_factor[], int xsize, int ysize, ARdouble mat[3][4], int dist_function_version );

#endif
