/*
 *  argFunction.c
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
 *  Author(s): Hirokazu Kato
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "argPrivate.h"

static void (*wmainFunc)(void) = NULL;
static void (*widleFunc)(void) = NULL;
static void (*wkeyFunc)(unsigned char key, int x, int y) = argDefaultKeyFunc;
static void (*wmouseFunc)(int button, int state, int x, int y) = NULL;
static void (*wmotionFunc)(int x, int y) = NULL;

static void vmainFunc(void);
static void vidleFunc(void);
static void vkeyFunc(unsigned char key, int x, int y);
static void vmouseFunc(int button, int state, int x, int y);
static void vmotionFunc(int x, int y);

int argSetDispFunc( void (*mainFunc)(void), int idleFlag )
{
    glutDisplayFunc( vmainFunc );
    wmainFunc = mainFunc;

    if( idleFlag ) {
        glutIdleFunc( vidleFunc );
        widleFunc = mainFunc;
    }

    return 0;
}

int argSetKeyFunc( void (*keyFunc)(unsigned char key, int x, int y) )
{
    glutKeyboardFunc( vkeyFunc );
    wkeyFunc = keyFunc;

    return 0;
}

int argSetMouseFunc( void (*mouseFunc)(int button, int state, int x, int y) )
{
    glutMouseFunc( vmouseFunc );
    wmouseFunc = mouseFunc;

    return 0;
}

int argSetMotionFunc( void (*motionFunc)(int x, int y) )
{
    glutMotionFunc( vmotionFunc );
    wmotionFunc = motionFunc;

    return 0;
}

void argMainLoop( void )
{
    glutMainLoop();
}

void argDefaultKeyFunc( unsigned char key, int x, int y )
{
    if( key == 0x1b ) {
        argCleanup();
        exit(0);
    }

    return;
}

static void vmainFunc(void)
{
    if( wmainFunc != NULL ) (*wmainFunc)();
}

static void vidleFunc(void)
{
    if( widleFunc != NULL ) (*widleFunc)();
}

static void vkeyFunc(unsigned char key, int x, int y)
{
    if( wkeyFunc != NULL ) (*wkeyFunc)(key, x, y);
}

static void vmouseFunc(int button, int state, int x, int y)
{
    if( wmouseFunc != NULL ) (*wmouseFunc)(button, state, x, y);
}

static void vmotionFunc(int x, int y)
{
    if( wmotionFunc != NULL ) (*wmotionFunc)(x, y);
}
