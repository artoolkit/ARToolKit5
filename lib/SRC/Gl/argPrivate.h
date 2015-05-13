/*
 *  argPrivate.h
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
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef AR_GL_PRIVATE_H
#define AR_GL_PRIVATE_H

#include <AR/gsub.h>

#ifdef __cplusplus
extern "C" {
#endif


// Make sure that required OpenGL constant definitions are available at compile-time.
// N.B. These should not be used unless the renderer indicates (at run-time) that it supports them.
	
// Define constants for extensions which became core in OpenGL 1.2
#ifndef GL_VERSION_1_2
#  if GL_EXT_bgra
#    define GL_BGR							GL_BGR_EXT
#    define GL_BGRA							GL_BGRA_EXT
#  else
#    define GL_BGR							0x80E0
#    define GL_BGRA							0x80E1
#  endif
#  ifndef GL_APPLE_packed_pixels
#    define GL_UNSIGNED_INT_8_8_8_8			0x8035
#    define GL_UNSIGNED_INT_8_8_8_8_REV		0x8367
#  endif
#  if GL_SGIS_texture_edge_clamp
#    define GL_CLAMP_TO_EDGE				GL_CLAMP_TO_EDGE_SGIS
#  else
#    define GL_CLAMP_TO_EDGE				0x812F
#  endif
#endif
	
// Define constants for extensions which became core in OpenGL 3.1
#ifndef GL_VERSION_3_1
#  if GL_NV_texture_rectangle
#    define GL_TEXTURE_RECTANGLE            GL_TEXTURE_RECTANGLE_NV
#    define GL_PROXY_TEXTURE_RECTANGLE		GL_PROXY_TEXTURE_RECTANGLE_NV
#    define GL_MAX_RECTANGLE_TEXTURE_SIZE   GL_MAX_RECTANGLE_TEXTURE_SIZE_NV
#  elif GL_EXT_texture_rectangle
#    define GL_TEXTURE_RECTANGLE            GL_TEXTURE_RECTANGLE_EXT
#    define GL_PROXY_TEXTURE_RECTANGLE      GL_PROXY_TEXTURE_RECTANGLE_EXT
#    define GL_MAX_RECTANGLE_TEXTURE_SIZE   GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT
#  else
#    define GL_TEXTURE_RECTANGLE            0x84F5
#    define GL_PROXY_TEXTURE_RECTANGLE      0x84F7
#    define GL_MAX_RECTANGLE_TEXTURE_SIZE   0x84F8
#  endif
#endif

// Define constants for extensions (not yet core).
#ifndef GL_APPLE_ycbcr_422
#  define GL_YCBCR_422_APPLE				0x85B9
#  define GL_UNSIGNED_SHORT_8_8_APPLE		0x85BA
#  define GL_UNSIGNED_SHORT_8_8_REV_APPLE	0x85BB
#endif
#ifndef GL_EXT_abgr
#  define GL_ABGR_EXT						0x8000
#endif


ARGWindowHandle   *argGetWinHandle( void );
ARGViewportHandle *argGetCurrentVPHandle( void );
int                argSetCurrentVPHandle( ARGViewportHandle *vp );
void               argClearDisplayList( ARGDisplayList *dispList );
int                argGetCurrentScale(ARGViewportHandle *vp, ARdouble *sx, ARdouble *sy, ARdouble *offx, ARdouble *offy );





#ifdef __cplusplus
}
#endif
#endif
