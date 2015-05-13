/*
 *  video1394V1setting.c
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
 *  Copyright 2004-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */
/*
 *   Video capture subrutine for Linux/libdc1394 devices
 *   author: Hirokazu Kato ( kato@sys.im.hiroshima-cu.ac.jp )
 *
 *   Revision: 3.0   Date: 2004/01/01
 */

#include <AR/ar.h>
#ifndef AR_INPUT_1394CAM_USE_LIBDC1394_V2
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/types.h>
#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>
#include <AR/video.h>
#include "video1394V1Private.h"


int ar2VideoSetValue1394( AR2VideoParam1394T *vid, int paramName, int value )
{
    unsigned int ub, vr;

    switch( paramName ) {
        case AR_VIDEO_1394_BRIGHTNESS:
            if( dc1394_set_brightness(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to set brightness to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_EXPOSURE:
            if( dc1394_set_exposure(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to set exposure to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_WHITE_BALANCE_UB:
            if( dc1394_get_white_balance(ar2VideoGetPortParam(vid->port)->handle, vid->node, &ub, &vr) != DC1394_SUCCESS ) {
                ARLOGe("unable to get white balance.\n");
                return -1;
            }
            if( dc1394_set_white_balance(ar2VideoGetPortParam(vid->port)->handle, vid->node, value, vr) != DC1394_SUCCESS ) {
                ARLOGe("unable to set white balance.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_WHITE_BALANCE_VR:
            if( dc1394_get_white_balance(ar2VideoGetPortParam(vid->port)->handle, vid->node, &ub, &vr) != DC1394_SUCCESS ) {
                ARLOGe("unable to get white balance.\n");
                return -1;
            }
            if( dc1394_set_white_balance(ar2VideoGetPortParam(vid->port)->handle, vid->node, ub, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to set white balance to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            if( dc1394_set_shutter(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to set shutter speed to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_GAIN:
            if( dc1394_set_gain(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to set gain to %d.\n", value);
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_FOCUS:
            if( dc1394_set_focus(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to set focus to %d.\n", value);
                return -1;
            }
            return 0;
    }

    return -1;
}

int ar2VideoGetValue1394( AR2VideoParam1394T *vid, int paramName, int *value )
{
    unsigned int ub, vr;

    switch( paramName ) {
        case AR_VIDEO_1394_BRIGHTNESS:
            if( dc1394_get_brightness(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to get brightness.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_EXPOSURE:
            if( dc1394_get_exposure(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to get exposure.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_WHITE_BALANCE_UB:
            if( dc1394_get_white_balance(ar2VideoGetPortParam(vid->port)->handle, vid->node, (unsigned int *)value, &vr) != DC1394_SUCCESS ) {
                ARLOGe("unable to get white balance ub.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_WHITE_BALANCE_VR:
            if( dc1394_get_white_balance(ar2VideoGetPortParam(vid->port)->handle, vid->node, &ub, (unsigned int *)value) != DC1394_SUCCESS ) {
                ARLOGe("unable to get white balance vr.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            if( dc1394_get_shutter(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to get shutter speed.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_GAIN:
            if( dc1394_get_gain(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to get gain.\n");
                return -1;
            }
            return 0;
        case AR_VIDEO_1394_FOCUS:
            if( dc1394_get_focus(ar2VideoGetPortParam(vid->port)->handle, vid->node, value) != DC1394_SUCCESS ) {
                ARLOGe("unable to get focus.\n");
                return -1;
            }
            return 0;
    }

    return -1;
}



int ar2VideoGetAutoOn1394( AR2VideoParam1394T *vid, int paramName, int *value )
{
    unsigned int   feature;

    switch( paramName ) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = FEATURE_FOCUS;
            break;
        default:
            return -1;
    }

    if( dc1394_has_auto_mode(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, (dc1394bool_t *)value) != DC1394_SUCCESS ) {
        ARLOGe("unable to check auto mode.\n");
        return -1;
    }
    if( *value == 0 ) return 0;

    if( dc1394_is_feature_auto(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, (dc1394bool_t *)value) != DC1394_SUCCESS ) {
        ARLOGe("unable to check auto mode.\n");
        return -1;
    }

    return 0;
}

int ar2VideoSetAutoOn1394( AR2VideoParam1394T *vid, int paramName, int value )
{
    unsigned int   feature;
    int            v;

    switch( paramName ) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = FEATURE_FOCUS;
            break;
        default:
            return -1;
    }

    if( dc1394_has_auto_mode(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, (dc1394bool_t *)&v) != DC1394_SUCCESS ) {
        ARLOGe("unable to set auto mode.\n");
        return -1;
    }
    if( v == 0 ) {
        if( value == 0 ) return 0;
        ARLOGe("unable to set auto mode.\n");
        return -1;
    }

    if( dc1394_auto_on_off(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, value) != DC1394_SUCCESS ) {
        ARLOGe("unable to set auto mode.\n");
        return -1;
    }

    return 0;
}

int ar2VideoGetFeatureOn1394( AR2VideoParam1394T *vid, int paramName, int *value )
{
    unsigned int   feature;

    switch( paramName ) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = FEATURE_FOCUS;
            break;
        default:
            return -1;
    }

    if( dc1394_is_feature_present(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, (dc1394bool_t *)value) != DC1394_SUCCESS ) {
        ARLOGe("unable to check feature.\n");
        return -1;
    }
    if( *value == 0 ) return 0;

    if( dc1394_is_feature_on(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, (dc1394bool_t *)value) != DC1394_SUCCESS ) {
        ARLOGe("unable to check feature.\n");
        return -1;
    }

    return 0;
}

int ar2VideoSetFeatureOn1394( AR2VideoParam1394T *vid, int paramName, int value )
{
    unsigned int   feature;
    int            v;

    switch( paramName ) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = FEATURE_FOCUS;
            break;
        default:
            return -1;
    }

    if( dc1394_is_feature_present(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, (dc1394bool_t *)&v) != DC1394_SUCCESS ) {
        ARLOGe("unable to turn feature on.\n");
        return -1;
    }
    if( v == 0 ) {
        if( value == 0 ) return 0;
        ARLOGe("unable to turn feature on.\n");
        return -1;
    }

    if( dc1394_feature_on_off(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, value) != DC1394_SUCCESS ) {
        ARLOGe("unable to set auto mode.\n");
        return -1;
    }

    return 0;
}

int ar2VideoGetMaxValue1394( AR2VideoParam1394T *vid, int paramName, int *value )
{
    unsigned int   feature;

    switch( paramName ) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = FEATURE_FOCUS;
            break;
        default:
            return -1;
    }

    if( dc1394_get_max_value(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, (unsigned int *)value) != DC1394_SUCCESS ) {
        ARLOGe("unable to get max value.\n");
        return -1;
    }

    return 0;
}

int ar2VideoGetMinValue1394( AR2VideoParam1394T *vid, int paramName, int *value )
{
    unsigned int   feature;

    switch( paramName ) {
        case AR_VIDEO_1394_BRIGHTNESS:
            feature = FEATURE_BRIGHTNESS;
            break;
        case AR_VIDEO_1394_EXPOSURE:
            feature = FEATURE_EXPOSURE;
            break;
        case AR_VIDEO_1394_WHITE_BALANCE:
            feature = FEATURE_WHITE_BALANCE;
            break;
        case AR_VIDEO_1394_SHUTTER_SPEED:
            feature = FEATURE_SHUTTER;
            break;
        case AR_VIDEO_1394_GAIN:
            feature = FEATURE_GAIN;
            break;
        case AR_VIDEO_1394_FOCUS:
            feature = FEATURE_FOCUS;
            break;
        default:
            return -1;
    }

    if( dc1394_get_min_value(ar2VideoGetPortParam(vid->port)->handle, vid->node, feature, (unsigned int *)value) != DC1394_SUCCESS ) {
        ARLOGe("unable to get max value.\n");
        return -1;
    }

    return 0;
}

int ar2VideoSaveParam1394( AR2VideoParam1394T *vid, char *filename )
{
    FILE    *fp;
    int     value;

    if( (fp=fopen(filename, "w")) == NULL ) return -1;

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON, &value);
    if( value == 1 ) {
        fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS_AUTO_ON, &value);
        if( value == 0 ) {
            fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS, &value);
            fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS\t%d\n", value);
        }
        else {
            fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS_AUTO_ON\t1\n");
        }
    }
    else {
        fprintf(fp, "AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_EXPOSURE_FEATURE_ON, &value);
    if( value == 1 ) {
        fprintf(fp, "AR_VIDEO_1394_EXPOSURE_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_EXPOSURE_AUTO_ON, &value);
        if( value == 0 ) {
            fprintf(fp, "AR_VIDEO_1394_EXPOSURE_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_EXPOSURE, &value);
            fprintf(fp, "AR_VIDEO_1394_EXPOSURE\t%d\n", value);
        }
        else {
            fprintf(fp, "AR_VIDEO_1394_EXPOSURE_AUTO_ON\t1\n");
        }
    }
    else {
        fprintf(fp, "AR_VIDEO_1394_EXPOSURE_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON, &value);
    if( value == 1 ) {
        fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON, &value);
        if( value == 0 ) {
            fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_UB, &value);
            fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_UB\t%d\n", value);
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_VR, &value);
            fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_VR\t%d\n", value);
        }
        else {
            fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON\t1\n");
        }
    }
    else {
        fprintf(fp, "AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON, &value);
    if( value == 1 ) {
        fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON, &value);
        if( value == 0 ) {
            fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, &value);
            fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED\t%d\n", value);
        }
        else {
            fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON\t1\n");
        }
    }
    else {
        fprintf(fp, "AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_GAIN_FEATURE_ON, &value);
    if( value == 1 ) {
        fprintf(fp, "AR_VIDEO_1394_GAIN_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_GAIN_AUTO_ON, &value);
        if( value == 0 ) {
            fprintf(fp, "AR_VIDEO_1394_GAIN_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_GAIN, &value);
            fprintf(fp, "AR_VIDEO_1394_GAIN\t%d\n", value);
        }
        else {
            fprintf(fp, "AR_VIDEO_1394_GAIN_AUTO_ON\t1\n");
        }
    }
    else {
        fprintf(fp, "AR_VIDEO_1394_GAIN_FEATURE_ON\t0\n");
    }

    ar2VideoGetParami1394(vid, AR_VIDEO_1394_FOCUS_FEATURE_ON, &value);
    if( value == 1 ) {
        fprintf(fp, "AR_VIDEO_1394_FOCUS_FEATURE_ON\t1\n");
        ar2VideoGetParami1394(vid, AR_VIDEO_1394_FOCUS_AUTO_ON, &value);
        if( value == 0 ) {
            fprintf(fp, "AR_VIDEO_1394_FOCUS_AUTO_ON\t0\n");
            ar2VideoGetParami1394(vid, AR_VIDEO_1394_FOCUS, &value);
            fprintf(fp, "AR_VIDEO_1394_FOCUS\t%d\n", value);
        }
        else {
            fprintf(fp, "AR_VIDEO_1394_FOCUS_AUTO_ON\t1\n");
        }
    }
    else {
        fprintf(fp, "AR_VIDEO_1394_FOCUS_FEATURE_ON\t0\n");
    }

    fclose(fp);

    return 0;
}

int ar2VideoLoadParam1394( AR2VideoParam1394T *vid, char *filename )
{
    FILE    *fp;
    int     value;
    char    buf[512], buf1[512], buf2[512];
    int     ret = 0;

    if( (fp=fopen(filename, "r")) == NULL ) return -1;

    for(;;) {
        if( fgets(buf, 512, fp) == NULL ) break;
        if( buf[0] == '#' || buf[0] == '\n' ) continue;
        buf1[0] = '\0';
        if( sscanf(buf, "%s %d", buf1, &value) != 2 ) {
            if( buf1[0] == '\0' ) continue;
            ARLOGe("Error: %s\n", buf);
            ret = -1;
            continue;
        }

        if( strcmp(buf1, "AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS_FEATURE_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_BRIGHTNESS_AUTO_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS_AUTO_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_BRIGHTNESS") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_BRIGHTNESS, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_EXPOSURE_FEATURE_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_EXPOSURE_FEATURE_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_EXPOSURE_AUTO_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_EXPOSURE_AUTO_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_EXPOSURE") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_EXPOSURE, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_FEATURE_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_AUTO_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_WHITE_BALANCE_UB") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_UB, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_WHITE_BALANCE_VR") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_WHITE_BALANCE_VR, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED_FEATURE_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED_AUTO_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_SHUTTER_SPEED") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_SHUTTER_SPEED, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_GAIN_FEATURE_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_GAIN_FEATURE_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_GAIN_AUTO_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_GAIN_AUTO_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_GAIN") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_GAIN, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_FOCUS_FEATURE_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_FOCUS_FEATURE_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_FOCUS_AUTO_ON") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_FOCUS_AUTO_ON, value);
        }
        else if( strcmp(buf1, "AR_VIDEO_1394_FOCUS") == 0 ) {
            ar2VideoSetParami1394(vid, AR_VIDEO_1394_FOCUS, value);
        }
        else {
            ARLOGe("Unknown command: %s\n", buf1);
            ret = -1;
            continue;
        }
    }

    fclose(fp);

    return ret;
}
#endif
