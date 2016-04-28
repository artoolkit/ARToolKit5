/*
 *  cpuInfo.c
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
 *  Copyright 2015-216 Daqri, LLC.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "cpuInfo.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/machine.h>

static const char *cpuTypeNameUnknown = "unknown";
static const char *cpuTypeNameARMv6 = "armv6";
static const char *cpuTypeNameARMv7 = "armv7";
static const char *cpuTypeNameARMv7s = "armv7s";
static const char *cpuTypeNameARMv7k = "armv7k"; // Apple Watch.
static const char *cpuTypeNameARM64 = "armv64";
static const char *cpuTypeNameX86 = "x86";
static const char *cpuTypeNameX86_64 = "x86_64";

const char *cpuTypeName(void)
{
    size_t size;
    cpu_type_t type;
    cpu_subtype_t subtype;
    
    size = sizeof(type);
    sysctlbyname("hw.cputype", &type, &size, NULL, 0);
    size = sizeof(subtype);
    sysctlbyname("hw.cpusubtype", &subtype, &size, NULL, 0);
    
    // Values for cputype and cpusubtype defined in <mach/machine.h>.
    if (type == CPU_TYPE_ARM) {
        switch (subtype) {
            case CPU_SUBTYPE_ARM_V7S:
                return cpuTypeNameARMv7s;
                break;
            case CPU_SUBTYPE_ARM_V7:
                return cpuTypeNameARMv7;
                break;
            case CPU_SUBTYPE_ARM_V6:
                return cpuTypeNameARMv6;
                break;
            case CPU_SUBTYPE_ARM_V7K:
                return cpuTypeNameARMv7k;
                break;
        }
    } else if (type == CPU_TYPE_ARM64) {
        return cpuTypeNameARM64;
    } else if (type == CPU_TYPE_X86) {
        return cpuTypeNameX86;
    } else if (type == CPU_TYPE_X86_64) {
        return cpuTypeNameX86_64;
    }
    return cpuTypeNameUnknown;    
}

