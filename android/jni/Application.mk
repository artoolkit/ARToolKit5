#--------------------------------------------------------------------------
#
#  ARToolKit5
#  ARToolKit for Android
#
#  This file is part of ARToolKit.
#
#  ARToolKit is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ARToolKit is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
#
#  As a special exception, the copyright holders of this library give you
#  permission to link this library with independent modules to produce an
#  executable, regardless of the license terms of these independent modules, and to
#  copy and distribute the resulting executable under terms of your choice,
#  provided that you also meet, for each linked independent module, the terms and
#  conditions of the license of that module. An independent module is a module
#  which is neither derived from nor based on this library. If you modify this
#  library, you may extend this exception to your version of the library, but you
#  are not obligated to do so. If you do not wish to do so, delete this exception
#  statement from your version.
#
#  Copyright 2015 Daqri, LLC.
#  Copyright 2011-2015 ARToolworks, Inc.
#
#  Authors: Julian Looser, Philip Lamb
#
#--------------------------------------------------------------------------
# Build with $NDK/ndk-build NDK_APPLICATION_MK=Application.mk
#--------------------------------------------------------------------------
APP_MODULES		:= ar aricp argsub_es argsub_es2 armulti eden kpm ar2 arutil arosg arvideo
#APP_OPTIM		:= release
#APP_OPTIM		:= debug
#APP_ABI			:= all
#APP_ABI		:= armeabi armeabi-v7a x86 mips
APP_ABI		:= armeabi armeabi-v7a x86 mips arm64-v8a x86_64 mips64
APP_PLATFORM    := android-15
APP_STL 		:= c++_shared
APP_CPPFLAGS 	:= -frtti -fexceptions
NDK_TOOLCHAIN_VERSION := clang
