#include <Eden/gluttext.h>
#if GLUTTEXT_STROKE_ENABLE
#include "glStateCache.h"

/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
 and is provided without guarantee or warrantee expressed or
 implied. This program is -not- in the public domain. */

//#include "glutint.h"
#include "glutstroke.h"

void
glutStrokeCharacter(GLUTstrokeFont font, int c)
{
    const StrokeCharRec *ch;
    const StrokeRec *stroke;
    StrokeFontPtr fontinfo;
    int i;
#if 0
    const CoordRec *coord;
    int j;
#endif
    
    
    fontinfo = (StrokeFontPtr) font;
    
    if (c < 0 || c >= fontinfo->num_chars)
        return;
    ch = &(fontinfo->ch[c]);
    if (ch) {
        for (i = ch->num_strokes, stroke = ch->stroke;
             i > 0; i--, stroke++) {
#if 1
            // Essential setup for glDrawArrays();
            glVertexPointer(2, GL_FLOAT, 0, stroke->coord);
            glStateCacheEnableClientStateVertexArray();
            glStateCacheClientActiveTexture(GL_TEXTURE0);
            glStateCacheDisableClientStateTexCoordArray();
            glStateCacheDisableClientStateNormalArray();
            
            glDrawArrays(GL_LINE_STRIP, 0, stroke->num_coords);
#else
            glBegin(GL_LINE_STRIP);
            for (j = stroke->num_coords, coord = stroke->coord;
                 j > 0; j--, coord++) {
                glVertex2f(coord->x, coord->y);
            }
            glEnd();
#endif
        }
        glTranslatef(ch->right, 0.0, 0.0);
    }
}

#endif
