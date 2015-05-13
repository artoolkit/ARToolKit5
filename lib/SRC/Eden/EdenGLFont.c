//
//  EdenGLFont.c
//  The Eden Library
//
//  Copyright (c) 2001-2013 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//	Some portions of font loading code based on code by Jeff Molofee, 1999, http://nehe.gamedev.net/
//
//	Rev		Date		Who		Changes
//

// @@BEGIN_EDEN_LICENSE_HEADER@@
//
//  This file is part of The Eden Library.
//
//  The Eden Library is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  The Eden Library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with The Eden Library.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
// @@END_EDEN_LICENSE_HEADER@@

// ============================================================================
//	Private includes.
// ============================================================================

#include <Eden/EdenGLFont.h>

#include <stdlib.h> // calloc()
#include <string.h>

// EdenSurfaces also does OpenGL header inclusion.
#include <Eden/EdenSurfaces.h>	// TEXTURE_INFO_t, TEXTURE_INDEX_t, SurfacesTextureLoad(), SurfacesTextureSet(), SurfacesTextureUnload()
#ifdef EDEN_OPENGLES
#  include <Eden/gluttext.h>
#else
#  ifdef __APPLE__
#    include <GLUT/GLUT.h>
#  else
#    include <GL/glut.h>
#  endif
#  define DISABLE_GL_STATE_CACHE
#endif
#include "glStateCache.h"


// ============================================================================
//  Private types and definitions.
// ============================================================================

typedef enum {
    EDEN_GL_FONT_TYPE_TEXTURE,
    EDEN_GL_FONT_TYPE_GLUT_STROKE,
} EDEN_GL_FONT_TYPE;

struct _EDEN_GL_FONT_INFO_t {
    EDEN_GL_FONT_TYPE type;
    char *fontName;
    char *fontDataPathname; // Pointer to font resources. For texture fonts, pathname to a texure. For GLUT fonts, NULL.
    float naturalHeight;
    EDEN_BOOL monospaced;
    float naturalWidthIfMonospaced;
    void *tsi; // Type-specific info. For texture fonts, pointer to TEXTURE_INFO_t. For GLUT fonts, the GLUT font name.
};

// When EDEN_GL_FONT_INFO_t.type==EDEN_GL_FONT_TYPE_TEXTURE, tsi will be a pointer to this structure.
typedef struct _EDEN_GL_FONT_TEXTURE_INFO {
    TEXTURE_INFO_t textureInfo;
    TEXTURE_INDEX_t *textureIndexPerContext; // Dynamically allocated to size gContextsActiveCount.
    int refCount; // Incremented each time loadTexture() is called, decremented each time unloadTexture() is called. When decremented to zero, this whole structure will be deallocated and parent ref set to NULL.
} EDEN_GL_FONT_TEXTURE_INFO;

typedef struct _EDEN_GL_FONT_VIEW_SETTINGS {
    float width;
    float height;
    float pixelsPerInch;
} EDEN_GL_FONT_VIEW_SETTINGS;

typedef struct _EDEN_GL_FONT_FORMATTING_SETTINGS {
    float characterSpacing;
    float lineSpacing;
    float wordExtraSpacing;
} EDEN_GL_FONT_FORMATTING_SETTINGS;

typedef struct _EDEN_GL_FONT_FONT_SETTINGS {
    EDEN_GL_FONT_INFO_t *font;
    float size;
} EDEN_GL_FONT_FONT_SETTINGS;

// ============================================================================
//  Globals
// ============================================================================

// Predefined fonts.
static EDEN_GL_FONT_INFO_t geneva = {
    EDEN_GL_FONT_TYPE_TEXTURE,
    "Geneva",
    "Geneva_bitmap.sgi",
    32.0f,
    TRUE,
    16.0f,
    NULL
};
EDEN_GL_FONT_INFO_t *const EDEN_GL_FONT_ID_Bitmap16_Geneva = &geneva;
static EDEN_GL_FONT_INFO_t ocrb10 = {
    EDEN_GL_FONT_TYPE_TEXTURE,
    "OCR-B-10",
    "OCR-B-10_bitmap.sgi",
    26.25f, // Looks best at 26.25f.
    TRUE,
    16.0f, // Looks best at 16.0f.
    NULL
};
EDEN_GL_FONT_INFO_t *const EDEN_GL_FONT_ID_Bitmap16_OCR_B_10 = &ocrb10;
static EDEN_GL_FONT_INFO_t roman = {
    EDEN_GL_FONT_TYPE_GLUT_STROKE,
    "Roman",
    NULL,
    119.05f,
    FALSE,
    0.0f,
    GLUT_STROKE_ROMAN
};
EDEN_GL_FONT_INFO_t *const EDEN_GL_FONT_ID_Stroke_Roman = &roman;
static EDEN_GL_FONT_INFO_t monoroman = {
    EDEN_GL_FONT_TYPE_GLUT_STROKE,
    "Mono Roman",
    NULL,
    119.05f,
    TRUE,
    104.76f,
    GLUT_STROKE_MONO_ROMAN
};
EDEN_GL_FONT_INFO_t *const EDEN_GL_FONT_ID_Stroke_MonoRoman = &monoroman;

static int gContextsActiveCount = 0;
static EDEN_BOOL gInited = FALSE; // Set to TRUE once EdenGLFontInit() has succesfully completed.

static EDEN_GL_FONT_VIEW_SETTINGS gViewSettings = {
    0.0f,
    0.0f,
    72.0f
};
static EDEN_GL_FONT_FORMATTING_SETTINGS gFormattingSettings = {
    0.0625f,
    1.125f,
    1.0f
};
static EDEN_GL_FONT_FONT_SETTINGS gFontSettings = {
    &monoroman,
    16.0f
};

// ============================================================================
//  Private functions
// ============================================================================

// ============================================================================
//  Public functions
// ============================================================================

EDEN_BOOL EdenGLFontInit(const int contextsActiveCount)
{
	// Sanity check
	if (gInited) return (FALSE);

	gContextsActiveCount = contextsActiveCount;
	gInited = TRUE;
    
	return (TRUE);
}

EDEN_BOOL EdenGLFontFinal(void)
{
    EDEN_BOOL ok = TRUE;
    
	// Sanity check
	if (!gInited) return (FALSE);
    
    gContextsActiveCount = 0;
	gInited = FALSE;
    
	return (ok);
}

EDEN_GL_FONT_INFO_t *EdenGLFontNewTextureFont(const char *fontName, const char *pathname, const float naturalHeight, const float naturalWidth)
{
    EDEN_GL_FONT_INFO_t *fontInfo;
    
    if (!fontName || !pathname) return (NULL);
    
    fontInfo = (EDEN_GL_FONT_INFO_t *)calloc(1, sizeof(EDEN_GL_FONT_INFO_t));
    if (!fontInfo) {
        EDEN_LOGe("Out of memory!");
        return (NULL);
    }
    fontInfo->type = EDEN_GL_FONT_TYPE_TEXTURE;
    fontInfo->fontName = strdup(fontName);
    fontInfo->fontDataPathname = strdup(pathname);
    fontInfo->naturalHeight = naturalHeight;
    fontInfo->monospaced = TRUE;
    fontInfo->naturalWidthIfMonospaced = naturalWidth;
    
    return (fontInfo);
}

void EdenGLFontDeleteTextureFont(EDEN_GL_FONT_INFO_t **fontInfo_p)
{
    if (!fontInfo_p || !*fontInfo_p) return;
    
    // Check that the user isn't trying to delete one of the static fonts.
    if (*fontInfo_p == &geneva ||
        *fontInfo_p == &ocrb10 ||
        *fontInfo_p == &roman ||
        *fontInfo_p == &monoroman) return;
    
    free((*fontInfo_p)->fontName);
    free((*fontInfo_p)->fontDataPathname);
    free(*fontInfo_p);
    *fontInfo_p = NULL;
}

void EdenGLFontSetFont(EDEN_GL_FONT_INFO_t *font)
{
    gFontSettings.font = font;
}

EDEN_GL_FONT_INFO_t * EdenGLFontGetFont(void)
{
    return (gFontSettings.font);
}

void EdenGLFontSetSize(const float points)
{
    gFontSettings.size = points;
}

float EdenGLFontGetSize(void)
{
    return (gFontSettings.size);
}

void EdenGLFontSetCharacterSpacing(const float spacing)
{
    gFormattingSettings.characterSpacing = spacing;
}

float EdenGLFontGetCharacterSpacing(void)
{
    return (gFormattingSettings.characterSpacing);
}

void EdenGLFontSetLineSpacing(const float spacing)
{
    gFormattingSettings.lineSpacing = spacing;
}

float EdenGLFontGetLineSpacing(void)
{
    return (gFormattingSettings.lineSpacing);
}

void EdenGLFontSetWordSpacing(const float spacing)
{
    gFormattingSettings.wordExtraSpacing = spacing - 1.0f;
}

float EdenGLFontGetWordSpacing(void)
{
    return (gFormattingSettings.wordExtraSpacing + 1.0f);
}

void EdenGLFontSetDisplayResolution(const float pixelsPerInch)
{
    gViewSettings.pixelsPerInch = pixelsPerInch;
}

float EdenGLFontGetDisplayResolution(void)
{
    return (gViewSettings.pixelsPerInch);
}

void EdenGLFontSetViewSize(const float widthInPixels, const float heightInPixels)
{
	gViewSettings.width = widthInPixels;
	gViewSettings.height = heightInPixels;
}

float EdenGLFontGetHeight(void)
{
    return (gFontSettings.size/72.0f * gViewSettings.pixelsPerInch);
}

float EdenGLFontGetCharacterWidth(const unsigned char c)
{
    float widthAtSizeOfOnePoint;
    
    if (c < ' ') return (0.0f);
    
    switch (gFontSettings.font->type) {
        case EDEN_GL_FONT_TYPE_GLUT_STROKE:
            if (gFontSettings.font->monospaced) {
                widthAtSizeOfOnePoint = gFontSettings.font->naturalWidthIfMonospaced / gFontSettings.font->naturalHeight;
            } else {
                widthAtSizeOfOnePoint = glutStrokeWidth(gFontSettings.font->tsi, c);
                if (c == ' ' && gFormattingSettings.wordExtraSpacing) widthAtSizeOfOnePoint *= (gFormattingSettings.wordExtraSpacing + 1.0f);
                widthAtSizeOfOnePoint /= gFontSettings.font->naturalHeight;
            }
            break;
        case EDEN_GL_FONT_TYPE_TEXTURE:
            widthAtSizeOfOnePoint = gFontSettings.font->naturalWidthIfMonospaced / gFontSettings.font->naturalHeight;
            break;
    }
    return (widthAtSizeOfOnePoint * gFontSettings.size/72.0f * gViewSettings.pixelsPerInch);
}

float EdenGLFontGetLineWidth(const unsigned char *line)
{
    int i;
    int charCount;
    int spaceCount;
    float widthAtSizeOfOnePoint;
    
    if (!line) return (0.0f);
    
    // Count non-control characters.
    charCount = spaceCount = i = 0;
    while (line[i]) {
        if (line[i] >= ' ') charCount++;
        if (line[i] == ' ') spaceCount++;
        i++;
    }
    
    switch (gFontSettings.font->type) {
        case EDEN_GL_FONT_TYPE_GLUT_STROKE:
            if (gFontSettings.font->monospaced) {
                widthAtSizeOfOnePoint = gFontSettings.font->naturalWidthIfMonospaced * charCount / gFontSettings.font->naturalHeight;
            } else {
                widthAtSizeOfOnePoint = (glutStrokeLength(gFontSettings.font->tsi, line) + glutStrokeWidth(gFontSettings.font->tsi, ' ')*gFormattingSettings.wordExtraSpacing*spaceCount) / gFontSettings.font->naturalHeight;
            }
            break;
        case EDEN_GL_FONT_TYPE_TEXTURE:
            widthAtSizeOfOnePoint = gFontSettings.font->naturalWidthIfMonospaced  * charCount / gFontSettings.font->naturalHeight;
            break;
    }
    widthAtSizeOfOnePoint += (charCount - 1)*gFormattingSettings.characterSpacing; // Add spaces between characters.
    return (widthAtSizeOfOnePoint * gFontSettings.size/72.0f * gViewSettings.pixelsPerInch);
}

float EdenGLFontGetBlockWidth(const unsigned char **lines, const unsigned int lineCount)
{
    int i;
    float width;
    float maxWidth = 0.0f;
    
    if (!lineCount || !lines) return (0.0f);
    
    for (i = 0; i < lineCount; i++) {
        width = EdenGLFontGetLineWidth(lines[i]);
        if (width > maxWidth) maxWidth = width;
    }
    return (maxWidth);
}

float EdenGLFontGetBlockHeight(const unsigned char **lines, const unsigned int lineCount)
{
    if (!lineCount || !lines) return (0.0f);

    return ((lineCount + (lineCount - 1)*(gFormattingSettings.lineSpacing - 1.0f)) * gFontSettings.size/72.0f * gViewSettings.pixelsPerInch);
}

EDEN_BOOL EdenGLFontLoadTextureFontForContext(const int contextIndex, EDEN_GL_FONT_INFO_t *fontInfo)
{
    EDEN_GL_FONT_TEXTURE_INFO *fontTextureInfo;
    char hasAlpha;
	
    // Sanity checks.
	if (contextIndex < 0 || contextIndex >= gContextsActiveCount) return (FALSE);
	if (!fontInfo) return (FALSE);
    if (fontInfo->type != EDEN_GL_FONT_TYPE_TEXTURE) return (FALSE);
	
	// If first time called, set up the texture info.
    if (!fontInfo->tsi) {
        
        fontInfo->tsi = calloc(1, sizeof(struct _EDEN_GL_FONT_TEXTURE_INFO));
        if (!fontInfo->tsi) return (FALSE);
        fontTextureInfo = (EDEN_GL_FONT_TEXTURE_INFO *)fontInfo->tsi; // Type convenience.
        
        fontTextureInfo->textureInfo.pathname = fontInfo->fontDataPathname;
        fontTextureInfo->textureInfo.mipmaps = GL_FALSE;
        fontTextureInfo->textureInfo.internalformat = GL_LUMINANCE;
        fontTextureInfo->textureInfo.min_filter = GL_LINEAR;
        fontTextureInfo->textureInfo.mag_filter = GL_LINEAR;
        fontTextureInfo->textureInfo.wrap_s = GL_REPEAT;
        fontTextureInfo->textureInfo.wrap_t = GL_REPEAT;
        fontTextureInfo->textureInfo.priority = 0.9;
        fontTextureInfo->textureInfo.env_mode = GL_REPLACE;
        
        fontTextureInfo->textureIndexPerContext = calloc(gContextsActiveCount, sizeof(TEXTURE_INDEX_t));
        if (!fontTextureInfo->textureIndexPerContext) {
            free(fontTextureInfo);
            fontTextureInfo = NULL;
            return (FALSE);
        }
    }
    
    fontTextureInfo = (EDEN_GL_FONT_TEXTURE_INFO *)fontInfo->tsi; // Type convenience.
    
    // Unload texture if previously loaded.
    if (fontTextureInfo->textureIndexPerContext[contextIndex]) {
        EdenSurfacesTextureUnload(contextIndex, 1, &(fontTextureInfo->textureIndexPerContext[contextIndex]));
    }
    
    // Load texture.
	if (!EdenSurfacesTextureLoad(contextIndex, 1, &(fontTextureInfo->textureInfo), &(fontTextureInfo->textureIndexPerContext[contextIndex]), &hasAlpha)) {
		fprintf(stderr,"EdenGLFontLoad(): Unable to load font texture.\n");
        
        // If this was first texture load and it didn't work out, don't keep the fontTextureInfo around.
        if (fontTextureInfo->refCount == 0) {
            free(fontTextureInfo->textureIndexPerContext);
            free(fontTextureInfo);
            fontTextureInfo = NULL;
        }
		return (FALSE);
    }
    fontTextureInfo->refCount++;
    
    return (TRUE);
}

EDEN_BOOL EdenGLFontUnloadTextureFontForContext(const int contextIndex, EDEN_GL_FONT_INFO_t *fontInfo)
{
    EDEN_GL_FONT_TEXTURE_INFO *fontTextureInfo;
    
	if (contextIndex < 0 || contextIndex >= gContextsActiveCount) return (FALSE); // Sanity check.
	if (!fontInfo) return (FALSE);
    if (!fontInfo->type == EDEN_GL_FONT_TYPE_TEXTURE) return (FALSE);
    
    if (!fontInfo->tsi) return (FALSE);
    
    // Just for convenience of type.
    fontTextureInfo = (EDEN_GL_FONT_TEXTURE_INFO *)fontInfo->tsi;
    
    EdenSurfacesTextureUnload(contextIndex, 1, &(fontTextureInfo->textureIndexPerContext[contextIndex]));
    
    fontTextureInfo->refCount--;
    if (fontTextureInfo->refCount == 0) {
        free(fontTextureInfo->textureIndexPerContext);
        free(fontTextureInfo);
        fontTextureInfo = NULL;
    }
    
    return (TRUE);
}

struct _VTs {
    GLfloat vertices[4][2];
    GLfloat texcoords[4][2];
};

static void drawSetup(const int contextIndex, struct _VTs *VTs)
{
	EDEN_GL_FONT_TEXTURE_INFO *fontTextureInfo;

    VTs->vertices[0][0] = 0.0f; VTs->vertices[0][1] = 0.0f;
    VTs->vertices[1][0] = gFontSettings.font->naturalWidthIfMonospaced; VTs->vertices[1][1] = 0.0f;
    VTs->vertices[2][0] = gFontSettings.font->naturalWidthIfMonospaced; VTs->vertices[2][1] = gFontSettings.font->naturalHeight;
    VTs->vertices[3][0] = 0.0f; VTs->vertices[3][1] = gFontSettings.font->naturalHeight;
    VTs->texcoords[0][0] = 0.0f; VTs->texcoords[0][1] = 0.0f;
    VTs->texcoords[1][0] = 0.0625f; VTs->texcoords[1][1] = 0.0f;
    VTs->texcoords[2][0] = 0.0625f; VTs->texcoords[2][1] = 0.0625f;
    VTs->texcoords[3][0] = 0.0f; VTs->texcoords[3][1] = 0.0625f;
    
    fontTextureInfo = (EDEN_GL_FONT_TEXTURE_INFO *)gFontSettings.font->tsi;
    
    // Set up for texture drawing.
    EdenSurfacesTextureSet(contextIndex, fontTextureInfo->textureIndexPerContext[contextIndex]); // Select font texture.
    glStateCacheBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR); // Blend by luminance.
    glStateCacheEnableBlend();
    glVertexPointer(2, GL_FLOAT, 0, VTs->vertices);
    glStateCacheEnableClientStateVertexArray();
    glStateCacheDisableClientStateNormalArray();
    glStateCacheClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, 0, VTs->texcoords);
    glStateCacheEnableClientStateTexCoordArray();
    glStateCacheEnableTex2D();
}

static void drawOneLine(const unsigned char *line)
{
    int i = 0;
    unsigned char c;

    if (gFontSettings.font->type == EDEN_GL_FONT_TYPE_GLUT_STROKE) {
        while ((c = line[i++])) {
            if (c < ' ') continue;
            glutStrokeCharacter(gFontSettings.font->tsi, c);
            if (!gFontSettings.font->monospaced && c == ' ' && gFormattingSettings.wordExtraSpacing) glTranslatef(glutStrokeWidth(gFontSettings.font->tsi, ' ') * gFormattingSettings.wordExtraSpacing, 0.0f, 0.0f);
            glTranslatef(gFontSettings.font->naturalHeight * gFormattingSettings.characterSpacing, 0.0f, 0.0f);
        }
    } else if (gFontSettings.font->type == EDEN_GL_FONT_TYPE_TEXTURE) {
        while ((c = line[i++])) {
            if (c < ' ') continue;
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glTranslatef((float)(c%16)*0.0625f, 1.0f - (float)(c/16 + 1)*0.0625f, 0.0f); // Select the appropriate bit of font texture.
            glMatrixMode(GL_MODELVIEW);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glTranslatef(gFontSettings.font->naturalWidthIfMonospaced + (gFontSettings.font->naturalHeight * gFormattingSettings.characterSpacing), 0.0f, 0.0f); // Move to the right.
        }
    }
}

void EdenGLFontDrawLine(const int contextIndex, const unsigned char *line, const float hOffset, const float vOffset, H_OFFSET_TYPE hOffsetType, V_OFFSET_TYPE vOffsetType)
{
    GLfloat x, y;
    GLfloat fontScalef;
    struct _VTs VTs;
    
    if (!line) return;
    
    if (hOffsetType == H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE) x = hOffset;
    else {
    	float textWidth = EdenGLFontGetLineWidth(line);
    	if (hOffsetType == H_OFFSET_TEXT_RIGHT_EDGE_TO_VIEW_RIGHT_EDGE) x = gViewSettings.width - hOffset - textWidth;
    	else /* H_OFFSET_VIEW_CENTER_TO_TEXT_CENTER */ x = (gViewSettings.width - textWidth)/2.0f + hOffset;
    }
    if (vOffsetType == V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE) y = vOffset;
    else {
    	float textHeight = EdenGLFontGetHeight();
    	if (vOffsetType == V_OFFSET_VIEW_TEXT_TOP_TO_VIEW_TOP) y = gViewSettings.height - hOffset - textHeight;
    	else /* V_OFFSET_VIEW_CENTER_TO_TEXT_CENTER */ y = (gViewSettings.height - textHeight)/2.0f + hOffset;
    }
    
    if (gFontSettings.font->type == EDEN_GL_FONT_TYPE_TEXTURE) drawSetup(contextIndex, &VTs);
    fontScalef = gFontSettings.size/72.0f * gViewSettings.pixelsPerInch / gFontSettings.font->naturalHeight;
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(fontScalef, fontScalef, fontScalef);
    drawOneLine(line);
    glPopMatrix();
}

void EdenGLFontDrawBlock(const int contextIndex, const unsigned char **lines, const unsigned int lineCount, const float hOffset, const float vOffset, H_OFFSET_TYPE hOffsetType, V_OFFSET_TYPE vOffsetType)
{
    int i;
    GLfloat x, y;
    GLfloat fontScalef;
    struct _VTs VTs;
    
    if (!lines) return;
    
    
    if (hOffsetType == H_OFFSET_VIEW_LEFT_EDGE_TO_TEXT_LEFT_EDGE) x = hOffset;
    else {
    	float textWidth = EdenGLFontGetBlockWidth(lines, lineCount);
    	if (hOffsetType == H_OFFSET_TEXT_RIGHT_EDGE_TO_VIEW_RIGHT_EDGE) x = gViewSettings.width - hOffset - textWidth;
    	else /* H_OFFSET_VIEW_CENTER_TO_TEXT_CENTER */ x = (gViewSettings.width - textWidth)/2.0f + hOffset;
    }
    if (vOffsetType == V_OFFSET_VIEW_BOTTOM_TO_TEXT_BASELINE) y = vOffset;
    else {
    	float textHeight = EdenGLFontGetBlockHeight(lines, lineCount);
    	if (vOffsetType == V_OFFSET_VIEW_TEXT_TOP_TO_VIEW_TOP) y = gViewSettings.height - hOffset - textHeight;
    	else /* V_OFFSET_VIEW_CENTER_TO_TEXT_CENTER */ y = (gViewSettings.height - textHeight)/2.0f + hOffset;
    }
    
    if (gFontSettings.font->type == EDEN_GL_FONT_TYPE_TEXTURE) drawSetup(contextIndex, &VTs);
    fontScalef = gFontSettings.size/72.0f * gViewSettings.pixelsPerInch / gFontSettings.font->naturalHeight;
    glPushMatrix();
    for (i = 0; i < lineCount; i++) {
        if (lines[i]) {
            glPopMatrix();
            glPushMatrix();
            glTranslatef(x, y, 0.0f);
            glScalef(fontScalef, fontScalef, fontScalef);
            glTranslatef(0.0f, ((lineCount - 1) - i)*gFontSettings.font->naturalHeight*gFormattingSettings.lineSpacing, 0.0f); // Translate to baseline for this line.
            drawOneLine(lines[i]);
        }
    }
    glPopMatrix();
}

