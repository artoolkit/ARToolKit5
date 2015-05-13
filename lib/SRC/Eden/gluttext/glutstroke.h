/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
 and is provided without guarantee or warrantee expressed or
 implied. This program is -not- in the public domain. */

#ifndef __glutstroke_h__
#define __glutstroke_h__

#if defined(EDEN_OPENGL3)
#  ifdef EDEN_MACOSX
#    include <OpenGL/gl3.h>
#  else
#    include <GL3/gl3.h>
#  endif
#elif defined(EDEN_OPENGLES)
#  if defined ANDROID
#    include <GLES/gl.h>
#    include <GLES/glext.h>
#  else
#    include <OpenGLES/ES1/gl.h>
#    include <OpenGLES/ES1/glext.h>
#  endif
#else
#  ifdef EDEN_MACOSX
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#endif

typedef struct {
    float x;
    float y;
} CoordRec, *CoordPtr;

typedef struct {
    int num_coords;
    const CoordRec *coord;
} StrokeRec, *StrokePtr;

typedef struct {
    int num_strokes;
    const StrokeRec *stroke;
    float center;
    float right;
} StrokeCharRec, *StrokeCharPtr;

typedef struct _StrokeFontRec {
    const char *name;
    int num_chars;
    const StrokeCharRec *ch;
    float top;
    float bottom;
} StrokeFontRec, *StrokeFontPtr;

typedef void *GLUTstrokeFont;

#endif /* __glutstroke_h__ */
