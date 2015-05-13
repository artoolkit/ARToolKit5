/*    
      glm.c
      Nate Robins, 1997, 2000
      nate@pobox.com, http://www.pobox.com/~nate

      Wavefront OBJ model file format reader/writer/manipulator.

      Includes routines for generating smooth normals with
      preservation of edges, welding redundant vertices & texture
      coordinate generation (spheremap and planar projections) + more.
*/

//
//	Updated by Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
//
//	Rev		Date		Who		Changes
//  1.0.0   ????-??-??	NR      Original version released as part of Nate Robin's OpenGL tutors package.
//	1.0.1	2001-11-14	PRL		Fix linear texture coordinate generation.
//	1.0.2	2001-11-15	PRL		Correct glmUnitize & glmDimensions for model with no -ve vertices,
//								and correct glmUnitize to use unit cube, not cube with sides=2.0.
//	1.1.0	2002-01-28	PRL		Add glmDrawArrays to use compiled vertex arrays.
//	1.2.0	2002-01-30	PRL		glmDraw, glmDrawArrays and glmList are now OpenGL context-specific.
//	1.2.1	2002-02-03	PRL		New glmVertexNormals() routine to handle models with polys with > 3 sides more
//                              consistently.
//	1.3.0	2003-08-06	PRL		Now handles materials with dissolve factors for transparency and texture maps
//								and dissolve factors. glmReadOBJ is now OpenGL context-specific.
//	1.4.0	2008-07-30	PRL		Complete rewrite of glmDrawArrays to now correctly handle normals and texcoords.
//								Implementation is now OpenGL|ES 1.1-compliant.
//                              Fixed a few small bugs in material handling in drawing routines.
//                              Made glmMax into a macro and added glmMin, & replaced glmAbs with fabsf.
//  1.4.1   2009-04-21  PRL     Allow use of multiple materials within a group in the file, by forking off a
//                              new group. Should now handle models exported from Google Sketchup Pro OK.
//  1.4.2   2010-03-23  PRL     Add glmRotate function. Minor changes for correct compilation.
//  1.5.0   2010-09-17  PRL     Now supports transparency in materials, including textures. Because depth sorting
//                              is not yet implemented, overlay of multiple transparent surfaces may produce
//                              odd visual results. It is recommended that models be sorted back to front
//                              prior to drawing.
//  1.5.1   2011-03-01  PRL     Better handle the case when some but not all faces have normals or texcoords.
//  1.6.0   2011-08-17  PRL     Incorporate glStateCache to improve performance on TBDR OpenGL ES renderers.
//  1.7.0   2012-09-28  PRL     Added lazy loading of model textures at render time, glmReadOBJ2 to request this.
//  1.7.1   2013-10-02  PRL     Added flipping of textures in vertical dimension (some .obj models need it),
//                              and glmReadOBJ3 to request this. Also, better handles models with odd ordering
//                              of group and material commands.
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

#include <Eden/glm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>             // USHRT_MAX
#include <Eden/EdenMath.h>      // M_PI, sqrtf(), acosf(), asinf(), fabsf()
#ifndef EDEN_OPENGLES
#  define DISABLE_GL_STATE_CACHE
#endif
#include "glStateCache.h"
#ifndef GL_GENERATE_MIPMAP
#  define GL_GENERATE_MIPMAP 0x8191
#endif

#define glmDefaultGroupName "default"
#define T(x) (model->triangles[(x)])

/* _GLMnode: general purpose node, used by glmVertexNormals() to build a
	linked list of triangles containing a particular vertex. */
typedef struct _GLMnode {
    GLuint			index;		// Which triangle this node is tracking.
	GLuint			indexindex;	// Which of the points in this triangle this node is tracking.
    GLboolean		averaged;
    struct _GLMnode* next;		// The next node in the list, or NULL if this node is the tail.
} GLMnode;

/* _GLMnode: general purpose node, used by glmDrawArrays() to build a
 linked list of triangles containing a particular vertex. */
typedef struct _GLMnode2 {
    GLushort          index; // Index into list of per-vertex data values. Each 3 defines a triangle.
    struct _GLMnode2* next;		// The next node in the list, or NULL if this node is the tail.
} GLMnode2;

#define glmMax(a, b) ((a) > (b) ? (a) : (b))
#define glmMin(a, b) ((a) < (b) ? (a) : (b))

/* glmDot: compute the dot product of two vectors
 *
 * u - array of 3 GLfloats (GLfloat u[3])
 * v - array of 3 GLfloats (GLfloat v[3])
 */
static GLfloat
glmDot(GLfloat* u, GLfloat* v)
{
    assert(u); assert(v);
    
    return u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
}

/* glmCross: compute the cross product of two vectors
 *
 * u - array of 3 GLfloats (GLfloat u[3])
 * v - array of 3 GLfloats (GLfloat v[3])
 * n - array of 3 GLfloats (GLfloat n[3]) to return the cross product in
 */
static GLvoid
glmCross(GLfloat* u, GLfloat* v, GLfloat* n)
{
    assert(u); assert(v); assert(n);
    
    n[0] = u[1]*v[2] - u[2]*v[1];
    n[1] = u[2]*v[0] - u[0]*v[2];
    n[2] = u[0]*v[1] - u[1]*v[0];
}

/* glmNormalize: normalize a vector
 *
 * v - array of 3 GLfloats (GLfloat v[3]) to be normalized
 */
static GLvoid
glmNormalize(GLfloat* v)
{
    GLfloat l;
    
    assert(v);
    
    l = (GLfloat)sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
}

/* glmEqual: compares two vectors and returns GL_TRUE if they are
 * equal (within a certain threshold) or GL_FALSE if not. An epsilon
 * that works fairly well is 0.000001.
 *
 * u - array of 3 GLfloats (GLfloat u[3])
 * v - array of 3 GLfloats (GLfloat v[3]) 
 */
static GLboolean glmEqual(GLfloat* u, GLfloat* v, GLfloat epsilon)
{
    if (fabsf(u[0] - v[0]) < epsilon &&
        fabsf(u[1] - v[1]) < epsilon &&
        fabsf(u[2] - v[2]) < epsilon) 
    {
        return GL_TRUE;
    }
    return GL_FALSE;
}

/* glmWeldVectors: eliminate (weld) vectors that are within an
 * epsilon of each other.
 *
 * vectors     - array of GLfloat[3]'s to be welded
 * numvectors - number of GLfloat[3]'s in vectors
 * epsilon     - maximum difference between vectors 
 *
 */
static GLfloat *glmWeldVectors(GLfloat* vectors, GLuint* numvectors, GLfloat epsilon)
{
    GLfloat* copies;
    GLuint   copied;
    GLuint   i, j;
    
    copies = (GLfloat*)malloc(sizeof(GLfloat) * 3 * (*numvectors + 1));
    memcpy(copies, vectors, (sizeof(GLfloat) * 3 * (*numvectors + 1)));
    
    copied = 1;
    for (i = 1; i <= *numvectors; i++) {
        for (j = 1; j <= copied; j++) {
            if (glmEqual(&vectors[3 * i], &copies[3 * j], epsilon)) {
                goto duplicate;
            }
        }
        
        /* must not be any duplicates -- add to the copies array */
        copies[3 * copied + 0] = vectors[3 * i + 0];
        copies[3 * copied + 1] = vectors[3 * i + 1];
        copies[3 * copied + 2] = vectors[3 * i + 2];
        j = copied;             /* pass this along for below */
        copied++;
        
duplicate:
/* set the first component of this vector to point at the correct
        index into the new copies array */
        vectors[3 * i + 0] = (GLfloat)j;
    }
    
    *numvectors = copied-1;
    return copies;
}

/* glmFindGroup: Find a group in the model */
static GLMgroup *glmFindGroup(GLMmodel* model, char* name, const GLuint material)
{
    GLMgroup* group;
    
    assert(model);
    
    group = model->groups;
    while (group) {
        if (!strcmp(name, group->name) && group->material == material) break; // Both name and material must match.
        group = group->next;
    }
    
    return group;
}

/* glmFindOrAddGroup: Add a group to the model */
static GLMgroup *glmFindOrAddGroup(GLMmodel* model, char* name, const GLuint material)
{
    GLMgroup* group;
    GLMgroup* tail = NULL;
    
    assert(model);
    
    group = model->groups;
    while (group) {
        if (!strcmp(name, group->name) && group->material == material) break; // Both name and material must match.
        tail = group;
        group = group->next;
    }
    
    if (!group) {
        group = (GLMgroup*)malloc(sizeof(GLMgroup));
        group->name = strdup(name);
        group->material = material;
        group->numtriangles = 0;
        group->triangles = NULL;
        group->next = NULL;
        if (tail) tail->next = group;
        else model->groups = group;
        model->numgroups++;
    }
    
    return group;
}

/* glmFindMaterial: Find a material in the model */
static GLuint glmFindMaterial(GLMmodel* model, char* name)
{
    GLuint i;
    
    /* XXX doing a linear search on a string key'd list is pretty lame,
    but it works and is fast enough for now. */
    for (i = 0; i < model->nummaterials; i++) {
        if (!strcmp(model->materials[i].name, name))
            goto found;
    }
    
    /* didn't find the name, so print a warning and return the default
    material (0). */
    EDEN_LOGe("glmFindMaterial():  can't find material \"%s\".\n", name);
    i = 0;
    
found:
    return i;
}

/* glmDirName: return the directory given a path
 *
 * path - filesystem path
 *
 * NOTE: the return value should be free'd.
 */
static char*
glmDirName(char* path)
{
    char* dir;
    char* s;
    
    dir = strdup(path);
    
    s = strrchr(dir, '/');
    if (s)
        s[1] = '\0';	// place end of string after last separator
    else
        dir[0] = '\0';
    
    return dir;
}

#ifdef GLM_MATERIAL_TEXTURES
static GLboolean readTextureAndSendToGL(const int contextIndex, char *texturefilename, TEXTURE_INDEX_t *texturemap_index, char *texturemap_hasAlpha, const GLboolean flipH, const GLboolean flipV)
{
	TEXTURE_INFO_t textureInfo = {		// PRL 2003-08-06: Defaults for texturemaps.
		NULL,							//   pointer to name will go here.
		GL_TRUE,						//   generate mipmaps.
		0,                              //   internal format (0 = don't attempt conversion).
		GL_LINEAR_MIPMAP_LINEAR,		//   minification mode.
		GL_LINEAR,						//   magnification mode.
		GL_REPEAT,                      //   wrap_s.
		GL_REPEAT,                      //   wrap_t.
		0.5,							//   priority.
		GL_REPLACE,                     //   env_mode.
		//{0.0,0.0,0.0,0.0}				//   env_color.
	};
    static char initedSurfaces = FALSE;

    textureInfo.pathname = texturefilename;
    if (!initedSurfaces) {
        EdenSurfacesInit(1, 256); // Up to 256 textures, into 1 OpenGL context.
        initedSurfaces = TRUE;
    }
    if (!EdenSurfacesTextureLoad2(contextIndex, 1, &textureInfo, texturemap_index, texturemap_hasAlpha, flipH, flipV)) {
        EDEN_LOGe("EdenSurfacesTextureLoad() couldn't read texture file \"%s\".\n", texturefilename);
        return (FALSE);
    }
    return (TRUE);
}
#endif // GLM_MATERIAL_TEXTURES

/* glmReadMTL: read a wavefront material library file
 *
 * model - properly initialized GLMmodel structure
 * name  - name of the material library
 * contextIndex - PRL: index to the current OpenGL context (for texturing.) If you have only
 *             one OpenGL context (the most common case) set this parameter to 0.
 */
static GLboolean
glmReadMTL(GLMmodel* model, char* name, const int contextIndex, const GLboolean readTexturesNow)
{
    FILE* file;
    char* dir;
    char* filename;
    char    buf[128];
    GLuint nummaterials, i;
    
    dir = glmDirName(model->pathname);
    filename = (char*)malloc(sizeof(char) * (strlen(dir) + strlen(name) + 1));
    strcpy(filename, dir);
    strcat(filename, name);
    free(dir);
    
    file = fopen(filename, "r");
    if (!file) {
        EDEN_LOGe("glmReadMTL() failed: can't open material file \"%s\".\n", filename);
        return (FALSE);
    }
    
    /* count the number of materials in the file */
    nummaterials = 1; // default material 0 is always defined.
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'n':               /* newmtl */
            fgets(buf, sizeof(buf), file);
            nummaterials++;
            //sscanf(buf, "%s %s", buf, buf);
            break;
		default:
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        }
    }
    
    rewind(file);
    
    model->materials = (GLMmaterial*)malloc(sizeof(GLMmaterial) * nummaterials);
    model->nummaterials = nummaterials;
    
    /* set the defaults for each material */
    for (i = 0; i < nummaterials; i++) {
        model->materials[i].name = NULL;
        model->materials[i].shininess = 65.0;
        model->materials[i].diffuse[0] = 0.8;
        model->materials[i].diffuse[1] = 0.8;
        model->materials[i].diffuse[2] = 0.8;
        model->materials[i].diffuse[3] = 1.0; // Opaque.
        model->materials[i].ambient[0] = 0.2;
        model->materials[i].ambient[1] = 0.2;
        model->materials[i].ambient[2] = 0.2;
        model->materials[i].ambient[3] = 1.0;
        model->materials[i].specular[0] = 0.0;
        model->materials[i].specular[1] = 0.0;
        model->materials[i].specular[2] = 0.0;
        model->materials[i].specular[3] = 1.0;
#ifdef GLM_MATERIAL_TEXTURES
        model->materials[i].texturemap = NULL;		// PRL 20030806: No texture by default.
        model->materials[i].texturemappath = NULL;
		model->materials[i].texturemap_index = (TEXTURE_INDEX_t)0;
        model->materials[i].texturemap_hasAlpha = 0;
#endif // GLM_MATERIAL_TEXTURES
        model->materials[i].illum = 2; // Is 2 the default?
    }
    model->materials[0].name = strdup("default");
    
    /* now, read in the data */
    nummaterials = 0;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
            case '#':               /* comment */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
            case 'n':               /* newmtl */
                fgets(buf, sizeof(buf), file);
                sscanf(buf, "%s %s", buf, buf);
                nummaterials++;
                model->materials[nummaterials].name = strdup(buf);
                break;
            case 'N':
                switch (buf[1]) {
                    case 's':
                        fscanf(file, "%f", &model->materials[nummaterials].shininess);
                        /* wavefront shininess is from [0, 1000], so scale to [0, 127] for OpenGL */
                        model->materials[nummaterials].shininess /= 1000.0;
                        model->materials[nummaterials].shininess *= 128.0;
                        break;
                    default:
                        // Unsupported options:
                        // Ni = Refraction index. Values range from 1 upwards. A value of 1 will cause no refraction. A higher value implies refraction.
                        fgets(buf, sizeof(buf), file);
                        break;
                }
                break;
            case 'K':
                switch(buf[1]) {
                    case 'd':
                        fscanf(file, "%f %f %f",
                            &model->materials[nummaterials].diffuse[0],
                            &model->materials[nummaterials].diffuse[1],
                            &model->materials[nummaterials].diffuse[2]);
                        break;
                    case 's':
                        fscanf(file, "%f %f %f",
                            &model->materials[nummaterials].specular[0],
                            &model->materials[nummaterials].specular[1],
                            &model->materials[nummaterials].specular[2]);
                        break;
                    case 'a':
                        fscanf(file, "%f %f %f",
                            &model->materials[nummaterials].ambient[0],
                            &model->materials[nummaterials].ambient[1],
                            &model->materials[nummaterials].ambient[2]);
                        break;
                    default:
                        /* eat up rest of line */
                        fgets(buf, sizeof(buf), file);
                        break;
                }
                break;
            case 'd':		// PRL 20030806: dissolve factor, pseudo-transparency.
                fscanf(file, "%f", &model->materials[nummaterials].diffuse[3]);
                break;
#ifdef GLM_MATERIAL_TEXTURES
            case 'm':		// PRL 20030806: texturemap.
                if (strstr(buf, "map_Kd")) { // Process diffuse colour map.
                    fgets(buf, sizeof(buf), file);		// Read up to (and including) EOL from file into string.
                    buf[strlen(buf)-1] = '\0';  		// nuke '\n'.
                    model->materials[nummaterials].texturemap = strdup(buf+1);	// Save relative path from mtl file. +1 skips leading space.
                    // Handle relative paths from model and material.
                    dir = glmDirName(filename);
                    model->materials[nummaterials].texturemappath = (char*)malloc(sizeof(char) * (strlen(dir) + strlen(model->materials[nummaterials].texturemap) + 1));
                    strcpy(model->materials[nummaterials].texturemappath, dir);
                    strcat(model->materials[nummaterials].texturemappath, model->materials[nummaterials].texturemap);
                    free(dir);
                    if (readTexturesNow) {
                        if (!readTextureAndSendToGL(contextIndex, model->materials[nummaterials].texturemappath, &(model->materials[nummaterials].texturemap_index), &(model->materials[nummaterials].texturemap_hasAlpha), FALSE, model->flipTextureV)) {
                            EDEN_LOGe("glmReadMTL(): Error loading texture.\n");
                        }
                    }
                } else {
                    // Unsupported options:
                    // map_Ka, ambient colour map.
                    // map_Ks, specular colour map.
                    fgets(buf, sizeof(buf), file);		// eat up rest of line.
                }
                break;
#endif // GLM_MATERIAL_TEXTURES
            case 'i':       // Illumination model.
                fscanf(file, "%d", &model->materials[nummaterials].illum);
                break;
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
        }
    }
    free(filename);
    return (TRUE);
}

/* glmWriteMTL: write a wavefront material library file
 *
 * model   - properly initialized GLMmodel structure
 * modelpath  - pathname of the model being written
 * mtllibname - name of the material library to be written
 */
static GLvoid
glmWriteMTL(GLMmodel* model, char* modelpath, char* mtllibname)
{
    FILE* file;
    char* dir;
    char* filename;
    GLMmaterial* material;
    GLuint i;
    
    dir = glmDirName(modelpath);
    filename = (char*)malloc(sizeof(char) * (strlen(dir)+strlen(mtllibname)));
    strcpy(filename, dir);
    strcat(filename, mtllibname);
    free(dir);
    
    /* open the file */
    file = fopen(filename, "w");
    if (!file) {
        EDEN_LOGe("glmWriteMTL() failed: can't open file \"%s\".\n",
            filename);
        exit(1);
    }
    free(filename);
    
    /* spit out a header */
    fprintf(file, "#  \n");
    fprintf(file, "#  Wavefront MTL generated by GLM library\n");
    fprintf(file, "#  \n");
    fprintf(file, "#  GLM library\n");
    fprintf(file, "#  Nate Robins\n");
    fprintf(file, "#  ndr@pobox.com\n");
    fprintf(file, "#  http://www.pobox.com/~ndr\n");
    fprintf(file, "#  \n\n");
    
    for (i = 0; i < model->nummaterials; i++) {
        material = &model->materials[i];
        fprintf(file, "newmtl %s\n", material->name);
        fprintf(file, "Ka %f %f %f\n", 
            material->ambient[0], material->ambient[1], material->ambient[2]);
        fprintf(file, "Kd %f %f %f\n", 
            material->diffuse[0], material->diffuse[1], material->diffuse[2]);
        fprintf(file, "Ks %f %f %f\n", 
            material->specular[0],material->specular[1],material->specular[2]);
        fprintf(file, "Ns %f\n", material->shininess / 128.0 * 1000.0);
        if (material->diffuse[3] != 1.0) fprintf(file, "d %f\n", material->diffuse[3]); // PRL 20030806: dissolve factor, pseudo-transparency.
#ifdef GLM_MATERIAL_TEXTURES
        if (material->texturemap) fprintf(file, "map_Kd %s\n", material->texturemap); // PRL 20030806: texturemap.
#endif // GLM_MATERIAL_TEXTURES
        fprintf(file, "illum %d\n", material->illum);
        fprintf(file, "\n");
    }
}

static void trim(char *buf)
{
    size_t index;
    
    if (!buf) return;
    
    index = strlen(buf);
    if (!index) return;
    index--;
    
    // Strip trailing CR and NL chars.
    while (index && (buf[index] == '\r' || buf[index] == '\n')) {
        buf[index] = '\0';
        index--;
    }
}

/* glmFirstPass: first pass at a Wavefront OBJ file that gets all the
 * statistics of the model (such as #vertices, #normals, etc)
 * Also allocates memory in each groups for triangle _indices_ (not the triangles themselves though.)
 *
 * model - properly initialized GLMmodel structure
 * file  - (fopen'd) file descriptor
 * contextIndex - PRL: index to the current OpenGL context (for texturing.) If you have only
 *             one OpenGL context (the most common case) set this parameter to 0.
 */
static GLvoid
glmFirstPass(GLMmodel* model, FILE* file, const int contextIndex, const GLboolean readTexturesNow)
{
    GLuint       numvertices = 0;    /* number of vertices in model */
    GLuint       numnormals = 0;     /* number of normals in model */
    GLuint       numtexcoords = 0;   /* number of texcoords in model */
    GLuint       numtriangles = 0;   /* number of triangles in model */
    GLMgroup*    group;              /* current group */
    char*        groupName;
    char*        groupNamePrev;
    GLuint       material;
    GLuint       materialPrev;
    unsigned int v, n, t;
    char         buf[128];
    
    
    /* set the default group */
    material = 0;
    materialPrev = 0;
    groupName = strdup(glmDefaultGroupName);
    groupNamePrev = strdup("");
    
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
            case '#':               /* comment */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
            case 'v':               /* v, vn, vt */
                switch(buf[1]) {
                    case '\0':          /* vertex */
                        /* eat up rest of line */
                        fgets(buf, sizeof(buf), file);
                        numvertices++;
                        break;
                    case 'n':           /* normal */
                        /* eat up rest of line */
                        fgets(buf, sizeof(buf), file);
                        numnormals++;
                        break;
                    case 't':           /* texcoord */
                        /* eat up rest of line */
                        fgets(buf, sizeof(buf), file);
                        numtexcoords++;
                        break;
                    default:
                        EDEN_LOGe("glmFirstPass(): Error: Unknown token \"%s\".\n", buf);
                        fgets(buf, sizeof(buf), file);
                        break;
                }
                break;
            case 'm':				/* material library */
                fgets(buf, sizeof(buf), file);
                sscanf(buf, "%s %s", buf, buf);
                if (model->mtllibname) {
                    if (strcmp(buf, model->mtllibname) != 0) EDEN_LOGe("glmFirstPass(): Warning: Multiple material library names found. Only the first will be used.\n");
                } else {
                    if (glmReadMTL(model, buf, contextIndex, readTexturesNow)) model->mtllibname = strdup(buf);
                }
                break;
            case 'u':				/* usemtl */
                fgets(buf, sizeof(buf), file);
                sscanf(buf, "%s %s", buf, buf);
                material = glmFindMaterial(model, buf);
                break;
            case 'g':               /* group */
                fgets(buf, sizeof(buf), file); // Read up to (and including) EOL from file into string.
                trim(buf);
                if (groupName) free(groupName);
                groupName = strdup(buf+1);
                break;
            case 'f':               /* face */
                
                // If group name or material has changed since last face, create a new group.
                if (strcmp(groupName, groupNamePrev) != 0 || material != materialPrev) {
                    group = glmFindOrAddGroup(model, groupName, material);
                    if (groupNamePrev) free(groupNamePrev);
                    groupNamePrev = strdup(groupName);
                    materialPrev = material;
                }
                
                v = n = t = 0;
                fscanf(file, "%s", buf);
                /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
                /* Faces with > 3 vertices will be broken into triangles. */
                if (strstr(buf, "//")) {
                    /* v//n */
                    sscanf(buf, "%d//%d", &v, &n);
                    fscanf(file, "%d//%d", &v, &n);
                    fscanf(file, "%d//%d", &v, &n);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d//%d", &v, &n) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
                    /* v/t/n */
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
                    /* v/t */
                    fscanf(file, "%d/%d", &v, &t);
                    fscanf(file, "%d/%d", &v, &t);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d/%d", &v, &t) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                } else {
                    /* v */
                    fscanf(file, "%d", &v);
                    fscanf(file, "%d", &v);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d", &v) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                }
                break;
                    
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
        }
	}
  
    // Dispose of allocations.
    if (groupName) free(groupName);
    if (groupNamePrev) free(groupNamePrev);
    
	/* set the stats in the model structure */
	model->numvertices  = numvertices;
	model->numnormals   = numnormals;
	model->numtexcoords = numtexcoords;
	model->numtriangles = numtriangles;
  
	/* allocate memory for the triangle _indices_ in each group */
	group = model->groups;
	while (group) {
		if (group->numtriangles) group->triangles = (GLuint*)malloc(sizeof(GLuint) * group->numtriangles);
		group->numtriangles = 0; // Reset count in prep. for second pass.
		group = group->next;
	}
}

/* glmSecondPass: second pass at a Wavefront OBJ file that gets all
 * the data.
 *
 * model - properly initialized GLMmodel structure
 * file  - (fopen'd) file descriptor 
 */
static GLvoid
glmSecondPass(GLMmodel* model, FILE* file) 
{
	GLuint		numvertices;		/* number of vertices in model */
    GLuint		numnormals;			/* number of normals in model */
    GLuint		numtexcoords;		/* number of texcoords in model */
    GLuint		numtriangles;		/* number of triangles in model */
    GLfloat*    vertices;			/* array of vertices  */
    GLfloat*    normals;			/* array of normals */
    GLfloat*    texcoords;			/* array of texture coordinates */
    GLMgroup*	group;				/* current group pointer */
    char*       groupName;
    char*       groupNamePrev;
    GLuint      material;
    GLuint      materialPrev;
    GLuint		v, n, t;
    char        buf[128];			// Holds lines as they are read from the file.
    
    
    /* set the default group */
    material = 0;
    materialPrev = 0;
    groupName = strdup(glmDefaultGroupName);
    groupNamePrev = strdup("");

    /* set the pointer shortcuts */
    vertices	= model->vertices;
    normals		= model->normals;
    texcoords	= model->texcoords;
    
    /* on the second pass through the file, read all the data into the
    allocated arrays */
    numvertices = numnormals = numtexcoords = 1;	// Vertices, normals, texcoords are numbered from 1, not 0.
    numtriangles = 0;
    
    while (fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
            case '#':               /* comment */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
            case 'v':               /* v, vn, vt */
                switch(buf[1]) {
                case '\0':          /* vertex */
                    fscanf(file, "%f %f %f", 
                        &vertices[3 * numvertices + 0], 
                        &vertices[3 * numvertices + 1], 
                        &vertices[3 * numvertices + 2]);
                    numvertices++;
                    break;
                case 'n':           /* normal */
                    fscanf(file, "%f %f %f", 
                        &normals[3 * numnormals + 0],
                        &normals[3 * numnormals + 1], 
                        &normals[3 * numnormals + 2]);
                    numnormals++;
                    break;
                case 't':           /* texcoord */
                    fscanf(file, "%f %f", 
                        &texcoords[2 * numtexcoords + 0],
                        &texcoords[2 * numtexcoords + 1]);
                    numtexcoords++;
                    break;
                }
                break;
            case 'u':				/* use material */
                fgets(buf, sizeof(buf), file);		// Read up to (and including) EOL from file into string.
                sscanf(buf, "%s %s", buf, buf);
                material = glmFindMaterial(model, buf);
                break;
            case 'g':               /* group */
                fgets(buf, sizeof(buf), file); // Read up to (and including) EOL from file into string.
                trim(buf);
                if (groupName) free(groupName);
                groupName = strdup(buf+1);
                break;
            case 'f':               /* face */
                
                // If group name or material has changed since last face, find the group.
                if (strcmp(groupName, groupNamePrev) != 0 || material != materialPrev) {
                    group = glmFindGroup(model, groupName, material);
                    if (groupNamePrev) free(groupNamePrev);
                    groupNamePrev = strdup(groupName);
                    materialPrev = material;
                }

                v = n = t = 0;
                fscanf(file, "%s", buf);
                /* can be one of %d, %d//%d, %d/%d, %d/%d/%d */
                /* Faces with > 3 vertices will be broken into triangle fans. */
                if (strstr(buf, "//")) {
                    /* v//n */
                    sscanf(buf, "%d//%d", &v, &n);
                    T(numtriangles).vindices[0] = v;
                    T(numtriangles).tindices[0] = 0;
                    T(numtriangles).nindices[0] = n;
                    fscanf(file, "%d//%d", &v, &n);
                    T(numtriangles).vindices[1] = v;
                    T(numtriangles).tindices[1] = 0;
                    T(numtriangles).nindices[1] = n;
                    fscanf(file, "%d//%d", &v, &n);
                    T(numtriangles).vindices[2] = v;
                    T(numtriangles).tindices[2] = 0;
                    T(numtriangles).nindices[2] = n;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d//%d", &v, &n) > 0) {
                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = 0;
                        T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = 0;
                        T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
                        T(numtriangles).vindices[2] = v;
                        T(numtriangles).tindices[2] = 0;
                        T(numtriangles).nindices[2] = n;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
                    /* v/t/n */
                    T(numtriangles).vindices[0] = v;
                    T(numtriangles).tindices[0] = t;
                    T(numtriangles).nindices[0] = n;
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    T(numtriangles).vindices[1] = v;
                    T(numtriangles).tindices[1] = t;
                    T(numtriangles).nindices[1] = n;
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    T(numtriangles).vindices[2] = v;
                    T(numtriangles).tindices[2] = t;
                    T(numtriangles).nindices[2] = n;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
                        T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
                        T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
                        T(numtriangles).vindices[2] = v;
                        T(numtriangles).tindices[2] = t;
                        T(numtriangles).nindices[2] = n;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
                    /* v/t */
                    T(numtriangles).vindices[0] = v;
                    T(numtriangles).tindices[0] = t;
                    T(numtriangles).nindices[0] = 0;
                    fscanf(file, "%d/%d", &v, &t);
                    T(numtriangles).vindices[1] = v;
                    T(numtriangles).tindices[1] = t;
                    T(numtriangles).nindices[1] = 0;
                    fscanf(file, "%d/%d", &v, &t);
                    T(numtriangles).vindices[2] = v;
                    T(numtriangles).tindices[2] = t;
                    T(numtriangles).nindices[2] = 0;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d/%d", &v, &t) > 0) {
                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
                        T(numtriangles).nindices[0] = 0;
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
                        T(numtriangles).nindices[1] = 0;
                        T(numtriangles).vindices[2] = v;
                        T(numtriangles).tindices[2] = t;
                        T(numtriangles).nindices[2] = 0;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else {
                    /* v */
                    sscanf(buf, "%d", &v);
                    T(numtriangles).vindices[0] = v;
                    T(numtriangles).tindices[0] = 0;
                    T(numtriangles).nindices[0] = 0;
                    fscanf(file, "%d", &v);
                    T(numtriangles).vindices[1] = v;
                    T(numtriangles).tindices[1] = 0;
                    T(numtriangles).nindices[1] = 0;
                    fscanf(file, "%d", &v);
                    T(numtriangles).vindices[2] = v;
                    T(numtriangles).tindices[2] = 0;
                    T(numtriangles).nindices[2] = 0;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d", &v) > 0) {
                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = 0;
                        T(numtriangles).nindices[0] = 0;
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = 0;
                        T(numtriangles).nindices[1] = 0;
                        T(numtriangles).vindices[2] = v;
                        T(numtriangles).tindices[2] = 0;
                        T(numtriangles).nindices[2] = 0;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                }
                break;
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
		}	/* switch(buf[0]); */
	}	/* !EOF */
  
    // Dispose of allocations.
    if (groupName) free(groupName);
    if (groupNamePrev) free(groupNamePrev);
    
#if 0
    /* announce the memory requirements */
    EDEN_LOGe(" Memory: %d bytes\n",
              numvertices  * 3*sizeof(GLfloat) +
              numnormals   * 3*sizeof(GLfloat) * (numnormals ? 1 : 0) +
              numtexcoords * 3*sizeof(GLfloat) * (numtexcoords ? 1 : 0) +
              numtriangles * sizeof(GLMtriangle));
#endif
}

#pragma mark -

/* Public functions */

/* glmUnitize: "unitize" a model by translating it to the origin and
 * scaling it to fit in a unit cube around the origin.   Returns the
 * scalefactor used.
 *
 * model - properly initialized GLMmodel structure 
 */
GLfloat
glmUnitize(GLMmodel* model)
{
	GLuint  i;
    GLfloat maxx, minx, maxy, miny, maxz, minz;
    GLfloat cx, cy, cz, w, h, d;
    GLfloat scale;
    
    assert(model);
    assert(model->vertices);
    
    /* get the max/mins */
    maxx = minx = model->vertices[3 + 0];
    maxy = miny = model->vertices[3 + 1];
    maxz = minz = model->vertices[3 + 2];
    for (i = 1; i <= model->numvertices; i++) {
        if (maxx < model->vertices[3 * i + 0])
            maxx = model->vertices[3 * i + 0];
        if (minx > model->vertices[3 * i + 0])
            minx = model->vertices[3 * i + 0];
        
        if (maxy < model->vertices[3 * i + 1])
            maxy = model->vertices[3 * i + 1];
        if (miny > model->vertices[3 * i + 1])
            miny = model->vertices[3 * i + 1];
        
        if (maxz < model->vertices[3 * i + 2])
            maxz = model->vertices[3 * i + 2];
        if (minz > model->vertices[3 * i + 2])
            minz = model->vertices[3 * i + 2];
    }
    
    /* calculate model width, height, and depth */
    w = maxx - minx;
    h = maxy - miny;
    d = maxz - minz;
    
    /* calculate center of the model */
    cx = (maxx + minx) / 2.0;
    cy = (maxy + miny) / 2.0;
    cz = (maxz + minz) / 2.0;
    
    /* calculate unitizing scale factor */
    scale = 1.0 / glmMax(glmMax(w, h), d);
    
    /* translate around center then scale */
    for (i = 1; i <= model->numvertices; i++) {
        model->vertices[3 * i    ] -= cx;
        model->vertices[3 * i    ] *= scale;
        model->vertices[3 * i + 1] -= cy;
        model->vertices[3 * i + 1] *= scale;
        model->vertices[3 * i + 2] -= cz;
        model->vertices[3 * i + 2] *= scale;
    }
    
    return scale;
}

/* glmDimensions: Calculates the dimensions (width, height, depth) of
 * a model.
 *
 * model   - initialized GLMmodel structure
 * dimensions - array of 3 GLfloats (GLfloat dimensions[3])
 */
GLvoid
glmDimensions(GLMmodel* model, GLfloat* dimensions)
{
    GLuint i;
    GLfloat maxx, minx, maxy, miny, maxz, minz;
    
    assert(model);
    assert(model->vertices);
    assert(dimensions);
    
    /* get the max/mins */
    maxx = minx = model->vertices[3 + 0];
    maxy = miny = model->vertices[3 + 1];
    maxz = minz = model->vertices[3 + 2];
    for (i = 1; i <= model->numvertices; i++) {
        if (maxx < model->vertices[3 * i + 0])
            maxx = model->vertices[3 * i + 0];
        if (minx > model->vertices[3 * i + 0])
            minx = model->vertices[3 * i + 0];
        
        if (maxy < model->vertices[3 * i + 1])
            maxy = model->vertices[3 * i + 1];
        if (miny > model->vertices[3 * i + 1])
            miny = model->vertices[3 * i + 1];
        
        if (maxz < model->vertices[3 * i + 2])
            maxz = model->vertices[3 * i + 2];
        if (minz > model->vertices[3 * i + 2])
            minz = model->vertices[3 * i + 2];
    }
    
    /* calculate model width, height, and depth */
    dimensions[0] = maxx - minx;
    dimensions[1] = maxy - miny;
    dimensions[2] = maxz - minz;
}

/* glmScale: Scales a model by a given amount.
 * 
 * model - properly initialized GLMmodel structure
 * scale - scalefactor (0.5 = half as large, 2.0 = twice as large)
 */
GLvoid
glmScale(GLMmodel* model, GLfloat scale)
{
    GLuint i;
    
    for (i = 1; i <= model->numvertices; i++) {
        model->vertices[3 * i + 0] *= scale;
        model->vertices[3 * i + 1] *= scale;
        model->vertices[3 * i + 2] *= scale;
    }
}

/* glmScaleTextures: Scales a model's textures by a given amount.
 * 
 * model - properly initialized GLMmodel structure
 * scale - scalefactor (0.5 = half as large, 2.0 = twice as large)
 */
GLvoid
glmScaleTextures(GLMmodel* model, GLfloat scale)
{
    GLuint i;
    
    for (i = 1; i <= model->numtexcoords; i++) {
        model->texcoords[2 * i + 0] /= scale;
        model->texcoords[2 * i + 1] /= scale;
    }
}

/* glmTranslate: Translates (moves) a model by a given amount in x, y, z.
*
* model - properly initialized GLMmodel structure
* offset- vector specifying amount to translate model by.
*/
GLvoid
glmTranslate(GLMmodel* model, const GLfloat offset[3])
{
	GLuint i;

	/* translate around center */
	for (i = 1; i <= model->numvertices; i++) {
		model->vertices[3 * i    ] += offset[0];
		model->vertices[3 * i + 1] += offset[1];
		model->vertices[3 * i + 2] += offset[2];
	}
}

/* glmRotate: Rotates a model by a given amount angle (in radians) about axis in x, y, z.
 *
 * model - properly initialized GLMmodel structure
 * angle - scalar specifying amount to rotate model by, in radians.
 * x, y, z - scalar components of a normalized vector specifiying axis of rotation.
 */
GLvoid
glmRotate(GLMmodel* model, const GLfloat angle, const GLfloat x, const GLfloat y, const GLfloat z)
{
    GLfloat C, S, V;
	GLfloat xy, yz, xz;
	GLfloat Sx, Sy, Sz;
	GLfloat Vxy, Vyz, Vxz;
    GLfloat R1[3], R2[3], R3[3];
	GLuint i;
    GLfloat vertex[3];
	
	C = cosf(angle);
	S = sinf(angle);
	V = 1.0f - C;
	xy = x*y;
	yz = y*z;
	xz = x*z;
	Sx = S*x;
	Sy = S*y;
	Sz = S*z;
	Vxy = V*xy;
	Vyz = V*yz;
	Vxz = V*xz;
	R1[0] = V*x*x + C;  R1[1] = Vxy - Sz;	R1[2] = Vxz + Sy;
	R2[0] = Vxy + Sz;	R2[1] = V*y*y + C;  R2[2] = Vyz - Sx;
	R3[0] = Vxz - Sy;	R3[1] = Vyz + Sx;	R3[2] = V*z*z + C;
    
	for (i = 1; i <= model->numvertices; i++) {
		vertex[0] = model->vertices[3 * i    ];
		vertex[1] = model->vertices[3 * i + 1];
		vertex[2] = model->vertices[3 * i + 2];
		model->vertices[3 * i    ] = glmDot(R1, vertex);
		model->vertices[3 * i + 1] = glmDot(R2, vertex);
		model->vertices[3 * i + 2] = glmDot(R3, vertex);
	}
}

/* glmReverseWinding: Reverse the polygon winding for all polygons in
 * this model.   Default winding is counter-clockwise.  Also changes
 * the direction of the normals.
 * 
 * model - properly initialized GLMmodel structure 
 */
GLvoid
glmReverseWinding(GLMmodel* model)
{
    GLuint i, swap;
    
    assert(model);
    
    for (i = 0; i < model->numtriangles; i++) {
        swap = T(i).vindices[0];
        T(i).vindices[0] = T(i).vindices[2];
        T(i).vindices[2] = swap;
        
        if (model->numnormals) {
            swap = T(i).nindices[0];
            T(i).nindices[0] = T(i).nindices[2];
            T(i).nindices[2] = swap;
        }
        
        if (model->numtexcoords) {
            swap = T(i).tindices[0];
            T(i).tindices[0] = T(i).tindices[2];
            T(i).tindices[2] = swap;
        }
    }
    
    /* reverse facet normals */
    for (i = 1; i <= model->numfacetnorms; i++) {
        model->facetnorms[3 * i + 0] = -model->facetnorms[3 * i + 0];
        model->facetnorms[3 * i + 1] = -model->facetnorms[3 * i + 1];
        model->facetnorms[3 * i + 2] = -model->facetnorms[3 * i + 2];
    }
    
    /* reverse vertex normals */
    for (i = 1; i <= model->numnormals; i++) {
        model->normals[3 * i + 0] = -model->normals[3 * i + 0];
        model->normals[3 * i + 1] = -model->normals[3 * i + 1];
        model->normals[3 * i + 2] = -model->normals[3 * i + 2];
    }
}

/* glmFacetNormals: Generates facet normals for a model (by taking the
 * cross product of the two vectors derived from the sides of each
 * triangle).  Assumes a counter-clockwise winding.
 *
 * model - initialized GLMmodel structure
 */
GLvoid
glmFacetNormals(GLMmodel* model)
{
    GLuint  i;
    GLfloat u[3];
    GLfloat v[3];
    
    assert(model);
    assert(model->vertices);
    
    /* clobber any old facetnormals */
    if (model->facetnorms)
        free(model->facetnorms);
    
    /* allocate memory for the new facet normals */
    model->numfacetnorms = model->numtriangles;
    model->facetnorms = (GLfloat*)malloc(sizeof(GLfloat) *
                       3 * (model->numfacetnorms + 1));
    
    for (i = 0; i < model->numtriangles; i++) {
        model->triangles[i].findex = i+1;
        
        u[0] = model->vertices[3 * T(i).vindices[1] + 0] - model->vertices[3 * T(i).vindices[0] + 0];
        u[1] = model->vertices[3 * T(i).vindices[1] + 1] - model->vertices[3 * T(i).vindices[0] + 1];
        u[2] = model->vertices[3 * T(i).vindices[1] + 2] - model->vertices[3 * T(i).vindices[0] + 2];
        
        v[0] = model->vertices[3 * T(i).vindices[2] + 0] - model->vertices[3 * T(i).vindices[0] + 0];
        v[1] = model->vertices[3 * T(i).vindices[2] + 1] - model->vertices[3 * T(i).vindices[0] + 1];
        v[2] = model->vertices[3 * T(i).vindices[2] + 2] - model->vertices[3 * T(i).vindices[0] + 2];
        
        glmCross(u, v, &model->facetnorms[3 * (i+1)]);
        glmNormalize(&model->facetnorms[3 * (i+1)]);
    }
}

/* glmVertexNormals: Generates smooth vertex normals for a model.
 * First builds a list of all the triangles each vertex is in.   Then
 * loops through each vertex in the the list averaging all the facet
 * normals of the triangles each vertex is in.   Finally, sets the
 * normal index in the triangle for the vertex to the generated smooth
 * normal.   If the dot product of a facet normal and the facet normal
 * associated with the first triangle in the list of triangles the
 * current vertex is in is greater than the cosine of the angle
 * parameter to the function, that facet normal is not added into the
 * average normal calculation and the corresponding vertex is given
 * the facet normal.  This tends to preserve hard edges.  The angle to
 * use depends on the model, but 90 degrees is usually a good start.
 *
 * model - initialized GLMmodel structure
 * angle - maximum angle (in degrees) to smooth across
 */
GLvoid
glmVertexNormals(GLMmodel* model, GLfloat angle)
{
	GLMnode*	node;
	GLMnode*	testnode;
	GLMnode*	tail;
	GLMnode**	members;
    GLfloat*    normals;
    GLuint		numnormals;
	GLfloat		sum[3];
    GLfloat		dot, cos_angle;
    GLuint		i, avg;
    
    assert(model);
    assert(model->facetnorms);
    
    /* calculate the cosine of the angle (in degrees) */
    cos_angle = cosf(angle * M_PI / 180.0f);
    
    /* nuke any previous normals */
    if (model->normals)
        free(model->normals);
    
    /* allocate space for new normals */
    model->numnormals = model->numtriangles * 3; /* 3 normals per triangle */
    model->normals = (GLfloat*)malloc(sizeof(GLfloat) * 3 * (model->numnormals + 1));
    
    /* allocate a structure that will hold a linked list of triangle
    indices for each vertex */
    members = (GLMnode**)malloc(sizeof(GLMnode*) * (model->numvertices + 1));
    for (i = 1; i <= model->numvertices; i++)
        members[i] = NULL;
    
    /* for every triangle, create a node for each vertex in it */
    for (i = 0; i < model->numtriangles; i++) {
        node = (GLMnode*)malloc(sizeof(GLMnode));
        node->index = i;
		node->indexindex = 0;
        node->next  = members[T(i).vindices[0]];	// Link to the current head of the list
        members[T(i).vindices[0]] = node;			// and make this node the new head.
        
        node = (GLMnode*)malloc(sizeof(GLMnode));
        node->index = i;
		node->indexindex = 1;
        node->next  = members[T(i).vindices[1]];	// Link to the current head of the list
        members[T(i).vindices[1]] = node;			// and make this node the new head.
        
        node = (GLMnode*)malloc(sizeof(GLMnode));
        node->index = i;
		node->indexindex = 2;
        node->next  = members[T(i).vindices[2]];	// Link to the current head of the list
        members[T(i).vindices[2]] = node;			// and make this node the new head.
    }
    
    /* calculate the average normal for each vertex */
    numnormals = 1;
    for (i = 1; i <= model->numvertices; i++) {
#if 0
		/* calculate an average normal for this vertex by averaging the
        facet normal of every triangle this vertex is in */
        node = members[i];
        if (!node)
            EDEN_LOGe("glmVertexNormals(): vertex w/o a triangle\n");
        sum[0] = 0.0; sum[1] = 0.0; sum[2] = 0.0;
        avg = 0;
        while (node) {
			/* only average if the dot product of the angle between the two
			facet normals is greater than the cosine of the threshold
			angle -- or, said another way, the angle between the two
			facet normals is less than (or equal to) the threshold angle */
            dot = glmDot(&model->facetnorms[3 * T(node->index).findex], &model->facetnorms[3 * T(members[i]->index).findex]);
            if (dot > cos_angle) {
                node->averaged = GL_TRUE;
                sum[0] += model->facetnorms[3 * T(node->index).findex + 0];
                sum[1] += model->facetnorms[3 * T(node->index).findex + 1];
                sum[2] += model->facetnorms[3 * T(node->index).findex + 2];
                avg = 1;            /* we averaged at least one normal! */
            } else {
                node->averaged = GL_FALSE;
            }
            node = node->next;
        }
        
        if (avg) {
            /* normalize the averaged normal */
            glmNormalize(sum);
            
            /* add the normal to the vertex normals list */
            model->normals[3 * numnormals + 0] = sum[0];
            model->normals[3 * numnormals + 1] = sum[1];
            model->normals[3 * numnormals + 2] = sum[2];
            avg = numnormals;
            numnormals++;
        }
        
        /* set the normal of this vertex in each triangle it is in */
        node = members[i];
        while (node) {
            if (node->averaged) {
                /* if this node was averaged, use the average normal */
                if (T(node->index).vindices[0] == i)
                    T(node->index).nindices[0] = avg;
                else if (T(node->index).vindices[1] == i)
                    T(node->index).nindices[1] = avg;
                else if (T(node->index).vindices[2] == i)
                    T(node->index).nindices[2] = avg;
            } else {
                /* if this node wasn't averaged, use the facet normal */
                model->normals[3 * numnormals + 0] = model->facetnorms[3 * T(node->index).findex + 0];
                model->normals[3 * numnormals + 1] = model->facetnorms[3 * T(node->index).findex + 1];
                model->normals[3 * numnormals + 2] = model->facetnorms[3 * T(node->index).findex + 2];
                if (T(node->index).vindices[0] == i)
                    T(node->index).nindices[0] = numnormals;
                else if (T(node->index).vindices[1] == i)
                    T(node->index).nindices[1] = numnormals;
                else if (T(node->index).vindices[2] == i)
                    T(node->index).nindices[2] = numnormals;
                numnormals++;
            }
            node = node->next;
        } // while (node)
#else
		// New routine: For the linked list of nodes containing this vertex, look at
		// each node in the list and work out which other other nodes in the list should
		// have their facetnorms averaged together with it to produce a vertex normal.
		node = members[i];
		if (!node)
            EDEN_LOGe("glmVertexNormals(): Model has vertex not contained in any triangle.\n");
		while (node) {
			/* Average the facet normal of this node with the facet normals
			of other nodes in the list. Test each other node against the current
			node before adding it to the average and only add the tested node if
			the dot product of the angle between the two facet normals is greater than the
			cosine of the threshold angle -- or, said another way, the angle
			between the two facet normals is less than (or equal to) the
			threshold angle. */
			sum[0] = model->facetnorms[3 * T(node->index).findex + 0];	// Use the facet normal by default.
			sum[1] = model->facetnorms[3 * T(node->index).findex + 1];
			sum[2] = model->facetnorms[3 * T(node->index).findex + 2];
			avg = GL_TRUE;	// All nodes containing this vertex share the same vertex normal until proven otherwise.
			testnode = members[i];	// Point to the head of the node list.
			while (testnode) {
				if (testnode != node) {
					dot = glmDot(&model->facetnorms[3 * T(node->index).findex], &model->facetnorms[3 * T(testnode->index).findex]);
					if (dot > cos_angle) {
						sum[0] += model->facetnorms[3 * T(testnode->index).findex + 0];
						sum[1] += model->facetnorms[3 * T(testnode->index).findex + 1];
						sum[2] += model->facetnorms[3 * T(testnode->index).findex + 2];
					} else {
						avg = GL_FALSE;	// "One of these nodes is not like the other nodes."
					}
				}
				testnode = testnode->next;
			} // while (testnode)
			glmNormalize(sum);		// Normalize the summed facetnorms to get the average.

			// Store the normal in the vertex normals list.
			model->normals[3 * numnormals + 0] = sum[0];
			model->normals[3 * numnormals + 1] = sum[1];
			model->normals[3 * numnormals + 2] = sum[2];
			// If all nodes containing the current vertex share the same average, assign just
			// one normal to all of them.
			if (avg) {
				// Assign one normal to all nodes containing the current vertex.
				node = members[i];
				while (node) {
					T(node->index).nindices[node->indexindex] = numnormals;
					node = node->next;
				}
				numnormals++;
				node = NULL;
			} else {
				// Assign one normal to the point in the triangle pointed to by this node.
				T(node->index).nindices[node->indexindex] = numnormals;
				numnormals++;
				node = node->next;
			}
		} // while (node)
#endif
    } // for (i = 1; i <= model->numvertices; i++)
    
    model->numnormals = numnormals - 1;
    
    /* free the member information */
    for (i = 1; i <= model->numvertices; i++) {
        node = members[i];
        while (node) {
            tail = node;
            node = node->next;
            free(tail);
        }
    }
    free(members);
    
    /* pack the normals array (we previously allocated the maximum
    number of normals that could possibly be created (numtriangles *
    3), so get rid of some of them (usually a lot unless none of the
    facet normals were averaged)) */
    normals = model->normals;
    model->normals = (GLfloat*)malloc(sizeof(GLfloat)* 3* (model->numnormals+1));
    for (i = 1; i <= model->numnormals; i++) {
        model->normals[3 * i + 0] = normals[3 * i + 0];
        model->normals[3 * i + 1] = normals[3 * i + 1];
        model->normals[3 * i + 2] = normals[3 * i + 2];
    }
    free(normals);
}


/* glmLinearTexture: Generates texture coordinates according to a
 * linear projection of the texture map.  It generates these by
 * linearly mapping the vertices onto a square.
 *
 * PRL, 20011114: The square lies in the x-z plane and is of the same
 * dimension as the model in this plane. Note that this mapping will
 * produce highly distored textures on polygons whose projections
 * onto the x-z plane have areas approaching zero, i.e. whose facet
 * normals have little or no y component.
 *
 * model - pointer to initialized GLMmodel structure
 */
GLvoid
glmLinearTexture(GLMmodel* model)
{
    GLMgroup *group;
    GLfloat dimensions[3];
    GLfloat x, y, scalefactor;
    GLuint i;
    
    assert(model);
    
    if (model->texcoords)
        free(model->texcoords);
    model->numtexcoords = model->numvertices;
    model->texcoords=(GLfloat*)malloc(sizeof(GLfloat)*2*(model->numtexcoords+1));
    
    glmDimensions(model, dimensions);
    // PRL, 20011114: Corrected to scale ignoring the y dimension.
	//scalefactor = 2.0 / fabsf(glmMax(glmMax(dimensions[0], dimensions[1]), dimensions[2]));
    scalefactor = 2.0 / fabsf(glmMax(dimensions[0], dimensions[2]));
	
    /* do the calculations */
    for(i = 1; i <= model->numvertices; i++) {
        x = model->vertices[3 * i + 0] * scalefactor;	// 
        y = model->vertices[3 * i + 2] * scalefactor;
        model->texcoords[2 * i + 0] = (x + 1.0) / 2.0;
        model->texcoords[2 * i + 1] = (y + 1.0) / 2.0;
    }
    
    /* go through and put texture coordinate indices in all the triangles */
    group = model->groups;
    while(group) {
        for(i = 0; i < group->numtriangles; i++) {
            T(group->triangles[i]).tindices[0] = T(group->triangles[i]).vindices[0];
            T(group->triangles[i]).tindices[1] = T(group->triangles[i]).vindices[1];
            T(group->triangles[i]).tindices[2] = T(group->triangles[i]).vindices[2];
        }    
        group = group->next;
    }
    
#if 0
    EDEN_LOGe("glmLinearTexture(): generated %d linear texture coordinates\n",
        model->numtexcoords);
#endif
}

/* glmSpheremapTexture: Generates texture coordinates according to a
 * spherical projection of the texture map.  Sometimes referred to as
 * spheremap, or reflection map texture coordinates.  It generates
 * these by using the normal to calculate where that vertex would map
 * onto a sphere.  Since it is impossible to map something flat
 * perfectly onto something spherical, there is distortion at the
 * poles.  This particular implementation causes the poles along the X
 * axis to be distorted.
 *
 * model - pointer to initialized GLMmodel structure
 */
GLvoid
glmSpheremapTexture(GLMmodel* model)
{
    GLMgroup* group;
    GLfloat theta, phi, rho, x, y, z, r;
    GLuint i;
    
    assert(model);
    assert(model->normals);
    
    if (model->texcoords)
        free(model->texcoords);
    model->numtexcoords = model->numnormals;
    model->texcoords=(GLfloat*)malloc(sizeof(GLfloat)*2*(model->numtexcoords+1));
    
    for (i = 1; i <= model->numnormals; i++) {
        z = model->normals[3 * i + 0];  /* re-arrange for pole distortion */
        y = model->normals[3 * i + 1];
        x = model->normals[3 * i + 2];
        r = sqrtf((x * x) + (y * y));
        rho = sqrt((r * r) + (z * z));
        
        if (r == 0.0) {
            theta = 0.0;
            phi = 0.0;
        } else {
            if(z == 0.0)
                phi = 3.14159265 / 2.0;
            else
                phi = acosf(z / rho);
            
            if(y == 0.0)
                theta = 3.141592365 / 2.0;
            else
                theta = asinf(y / r) + (3.14159265 / 2.0);
        }
        
        model->texcoords[2 * i + 0] = theta / 3.14159265;
        model->texcoords[2 * i + 1] = phi / 3.14159265;
    }
    
    /* go through and put texcoord indices in all the triangles */
    group = model->groups;
    while(group) {
        for (i = 0; i < group->numtriangles; i++) {
            T(group->triangles[i]).tindices[0] = T(group->triangles[i]).nindices[0];
            T(group->triangles[i]).tindices[1] = T(group->triangles[i]).nindices[1];
            T(group->triangles[i]).tindices[2] = T(group->triangles[i]).nindices[2];
        }
        group = group->next;
    }
}

/* glmDelete: Deletes a GLMmodel structure.
 *
 * model - initialized GLMmodel structure
 * contextIndex - PRL: index to the current OpenGL context (for texturing.) If you have only
 *             one OpenGL context (the most common case) set this parameter to 0.
 */
GLvoid
glmDelete(GLMmodel* model, const int contextIndex)
{
    GLMgroup* group;
    GLuint i;
    
    assert(model);
    
    if (model->arrays)      glmDeleteArrays(model);
    if (model->pathname)	free(model->pathname);
    if (model->mtllibname)	free(model->mtllibname);
    if (model->vertices)	free(model->vertices);
    if (model->normals)		free(model->normals);
    if (model->texcoords)	free(model->texcoords);
    if (model->facetnorms)	free(model->facetnorms);
    if (model->triangles)	free(model->triangles);
    if (model->materials) {
        for (i = 0; i < model->nummaterials; i++) {
#ifdef GLM_MATERIAL_TEXTURES
			if (model->materials[i].texturemap_index)		// PRL 20030806: texturemap.
				EdenSurfacesTextureUnload(contextIndex, 1, &model->materials[i].texturemap_index);
			if (model->materials[i].texturemap)
				free(model->materials[i].texturemap);
			if (model->materials[i].texturemappath)
				free(model->materials[i].texturemappath);
#endif // GLM_MATERIAL_TEXTURES
            free(model->materials[i].name);
		}
    }
    free (model->materials);
    while (model->groups) {
        group = model->groups;
        model->groups = model->groups->next;
        free(group->name);
        if (group->triangles) free(group->triangles);
        free(group);
    }
    
    free(model);
}

/* glmReadOBJ: Reads a model description from a Wavefront .OBJ file.
 * Returns a pointer to the created object which should be free'd with
 * glmDelete().
 *
 * filename - name of the file containing the Wavefront .OBJ format data.
 * contextIndex - PRL: index to the current OpenGL context (for texturing.) If you have only
 *             one OpenGL context (the most common case) set this parameter to 0.
 */
GLMmodel*
glmReadOBJ(const char *filename, const int contextIndex)
{
    return (glmReadOBJ3(filename, contextIndex, TRUE, FALSE));
}

GLMmodel*
glmReadOBJ2(const char *filename, const int contextIndex, const GLboolean readTexturesNow)
{
    return (glmReadOBJ3(filename, contextIndex, readTexturesNow, FALSE));
}

GLMmodel*
glmReadOBJ3(const char *filename, const int contextIndex, const GLboolean readTexturesNow, const GLboolean flipTextureV)
{
    GLMmodel* model;
    FILE*   file;
    
    /* open the file */
    file = fopen(filename, "r");
    if (!file) {
        EDEN_LOGe("glmReadOBJ() failed: can't open data file \"%s\".\n", filename);
        return (NULL);
    }
    
    /* allocate a new model */
    model = (GLMmodel*)malloc(sizeof(GLMmodel));
    model->pathname		= strdup(filename);
    model->mtllibname	= NULL;
    model->numvertices	= 0;
    model->vertices		= NULL;
    model->numnormals	= 0;
    model->normals		= NULL;
    model->numtexcoords	= 0;
    model->texcoords	= NULL;
    model->numfacetnorms	= 0;
    model->facetnorms	= NULL;
    model->numtriangles	= 0;
    model->triangles	= NULL;
    model->nummaterials	= 0;
    model->materials	= NULL;
    model->numgroups	= 0;
    model->groups		= NULL;
    model->arrays       = NULL;
    model->arrayMode    = 0;
    model->readTextureRequired = !readTexturesNow;
    model->flipTextureV        = flipTextureV;
    
    /* make a first pass through the file to get a count of the number
    of vertices, normals, texcoords & triangles */
    glmFirstPass(model, file, contextIndex, readTexturesNow);
    
    /* allocate memory */
    model->vertices = (GLfloat*)malloc(sizeof(GLfloat) *
        3 * (model->numvertices + 1));			// Uses + 1 because vertices, normals and texcoords are numbered from 1, not 0.
    model->triangles = (GLMtriangle*)malloc(sizeof(GLMtriangle) *
        model->numtriangles);
    if (model->numnormals) {
        model->normals = (GLfloat*)malloc(sizeof(GLfloat) *
            3 * (model->numnormals + 1));		// Uses + 1 because vertices, normals and texcoords are numbered from 1, not 0.
    }
    if (model->numtexcoords) {
        model->texcoords = (GLfloat*)malloc(sizeof(GLfloat) *
            2 * (model->numtexcoords + 1));		// Uses + 1 because vertices, normals and texcoords are numbered from 1, not 0.
    }
    
    /* rewind to beginning of file and read in the data this pass */
    rewind(file);
    
    glmSecondPass(model, file);
    
    /* close the file */
    fclose(file);
    
    return model;
}

/* glmWriteOBJ: Writes a model description in Wavefront .OBJ format to
 * a file.
 *
 * model - initialized GLMmodel structure
 * filename - name of the file to write the Wavefront .OBJ format data to
 * mode  - a bitwise or of values describing what is written to the file
 *             GLM_NONE     -  render with only vertices
 *             GLM_FLAT     -  render with facet normals
 *             GLM_SMOOTH   -  render with vertex normals
 *             GLM_TEXTURE  -  render with texture coords
 *             GLM_COLOR    -  render with colors (color material)
 *             GLM_MATERIAL -  render with materials
 *             GLM_COLOR and GLM_MATERIAL should not both be specified.  
 *             GLM_FLAT and GLM_SMOOTH should not both be specified.  
 */
GLvoid
glmWriteOBJ(GLMmodel* model, char* filename, GLuint mode)
{
    GLuint  i;
    FILE*   file;
    GLMgroup* group;
    
    assert(model);
    
    /* do a bit of warning */
    if (mode & GLM_FLAT && !model->facetnorms) {
        EDEN_LOGe("glmWriteOBJ() warning: flat normal output requested "
            "with no facet normals defined.\n");
        mode &= ~GLM_FLAT;
    }
    if (mode & GLM_SMOOTH && !model->normals) {
        EDEN_LOGe("glmWriteOBJ() warning: smooth normal output requested "
            "with no normals defined.\n");
        mode &= ~GLM_SMOOTH;
    }
    if (mode & GLM_TEXTURE && !model->texcoords) {
        EDEN_LOGe("glmWriteOBJ() warning: texture coordinate output requested "
            "with no texture coordinates defined.\n");
        mode &= ~GLM_TEXTURE;
    }
    if (mode & GLM_FLAT && mode & GLM_SMOOTH) {
        EDEN_LOGe("glmWriteOBJ() warning: flat normal output requested "
            "and smooth normal output requested (using smooth).\n");
        mode &= ~GLM_FLAT;
    }
    if (mode & GLM_COLOR && !model->materials) {
        EDEN_LOGe("glmWriteOBJ() warning: color output requested "
            "with no colors (materials) defined.\n");
        mode &= ~GLM_COLOR;
    }
    if (mode & GLM_MATERIAL && !model->materials) {
        EDEN_LOGe("glmWriteOBJ() warning: material output requested "
            "with no materials defined.\n");
        mode &= ~GLM_MATERIAL;
    }
    if (mode & GLM_COLOR && mode & GLM_MATERIAL) {
        EDEN_LOGe("glmWriteOBJ() warning: color and material output requested "
            "outputting only materials.\n");
        mode &= ~GLM_COLOR;
    }
    
    
    /* open the file */
    file = fopen(filename, "w");
    if (!file) {
        EDEN_LOGe("glmWriteOBJ() failed: can't open file \"%s\" to write.\n",
            filename);
        exit(1);
    }
    
    /* spit out a header */
    fprintf(file, "#  \n");
    fprintf(file, "#  Wavefront OBJ generated by GLM library\n");
    fprintf(file, "#  \n");
    fprintf(file, "#  GLM library\n");
    fprintf(file, "#  Nate Robins\n");
    fprintf(file, "#  ndr@pobox.com\n");
    fprintf(file, "#  http://www.pobox.com/~ndr\n");
    fprintf(file, "#  \n");
    
    if (mode & GLM_MATERIAL && model->mtllibname) {
        fprintf(file, "\nmtllib %s\n\n", model->mtllibname);
        glmWriteMTL(model, filename, model->mtllibname);
    }
    
    /* spit out the vertices */
    fprintf(file, "\n");
    fprintf(file, "# %d vertices\n", model->numvertices);
    for (i = 1; i <= model->numvertices; i++) {
        fprintf(file, "v %f %f %f\n", 
            model->vertices[3 * i + 0],
            model->vertices[3 * i + 1],
            model->vertices[3 * i + 2]);
    }
    
    /* spit out the smooth/flat normals */
    if (mode & GLM_SMOOTH) {
        fprintf(file, "\n");
        fprintf(file, "# %d normals\n", model->numnormals);
        for (i = 1; i <= model->numnormals; i++) {
            fprintf(file, "vn %f %f %f\n", 
                model->normals[3 * i + 0],
                model->normals[3 * i + 1],
                model->normals[3 * i + 2]);
        }
    } else if (mode & GLM_FLAT) {
        fprintf(file, "\n");
        fprintf(file, "# %d normals\n", model->numfacetnorms);
        for (i = 1; i <= model->numnormals; i++) {
            fprintf(file, "vn %f %f %f\n", 
                model->facetnorms[3 * i + 0],
                model->facetnorms[3 * i + 1],
                model->facetnorms[3 * i + 2]);
        }
    }
    
    /* spit out the texture coordinates */
    if (mode & GLM_TEXTURE) {
        fprintf(file, "\n");
        fprintf(file, "# %d texcoords\n", model->numtexcoords);
        for (i = 1; i <= model->numtexcoords; i++) {
            fprintf(file, "vt %f %f\n", 
                model->texcoords[2 * i + 0],
                model->texcoords[2 * i + 1]);
        }
    }
    
    fprintf(file, "\n");
    fprintf(file, "# %d groups\n", model->numgroups);
    fprintf(file, "# %d faces (triangles)\n", model->numtriangles);
    fprintf(file, "\n");
    
    group = model->groups;
    while(group) {
        fprintf(file, "g %s\n", group->name);
        if (mode & GLM_MATERIAL)
            fprintf(file, "usemtl %s\n", model->materials[group->material].name);
        for (i = 0; i < group->numtriangles; i++) {
            if (mode & GLM_SMOOTH && mode & GLM_TEXTURE) {
                fprintf(file, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
                    T(group->triangles[i]).vindices[0], 
                    T(group->triangles[i]).nindices[0], 
                    T(group->triangles[i]).tindices[0],
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).nindices[1],
                    T(group->triangles[i]).tindices[1],
                    T(group->triangles[i]).vindices[2],
                    T(group->triangles[i]).nindices[2],
                    T(group->triangles[i]).tindices[2]);
            } else if (mode & GLM_FLAT && mode & GLM_TEXTURE) {
                fprintf(file, "f %d/%d %d/%d %d/%d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).findex,
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).findex,
                    T(group->triangles[i]).vindices[2],
                    T(group->triangles[i]).findex);
            } else if (mode & GLM_TEXTURE) {
                fprintf(file, "f %d/%d %d/%d %d/%d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).tindices[0],
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).tindices[1],
                    T(group->triangles[i]).vindices[2],
                    T(group->triangles[i]).tindices[2]);
            } else if (mode & GLM_SMOOTH) {
                fprintf(file, "f %d//%d %d//%d %d//%d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).nindices[0],
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).nindices[1],
                    T(group->triangles[i]).vindices[2], 
                    T(group->triangles[i]).nindices[2]);
            } else if (mode & GLM_FLAT) {
                fprintf(file, "f %d//%d %d//%d %d//%d\n",
                    T(group->triangles[i]).vindices[0], 
                    T(group->triangles[i]).findex,
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).findex,
                    T(group->triangles[i]).vindices[2],
                    T(group->triangles[i]).findex);
            } else {
                fprintf(file, "f %d %d %d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).vindices[2]);
            }
        }
        fprintf(file, "\n");
        group = group->next;
    }
    
    fclose(file);
}

#ifndef EDEN_OPENGLES
/* glmDraw: Renders the model to the current OpenGL context using the
 * mode specified.
 *
 * model - initialized GLMmodel structure
 * mode  - a bitwise OR of values describing what is to be rendered.
 *             GLM_NONE     -  render with only vertices
 *             GLM_FLAT     -  render with facet normals
 *             GLM_SMOOTH   -  render with vertex normals
 *             GLM_TEXTURE  -  render with texture coords
 *             GLM_COLOR    -  render with colors (color material)
 *             GLM_MATERIAL -  render with materials
 *             GLM_COLOR and GLM_MATERIAL should not both be specified.  
 *             GLM_FLAT and GLM_SMOOTH should not both be specified.
 * contextIndex - PRL: index to the current OpenGL context (for texturing.) If you have only
 *             one OpenGL context (the most common case) set this parameter to 0.
 */
GLvoid
glmDraw(GLMmodel* model, GLuint mode, const int contextIndex)
{
	GLuint i;
	GLMgroup* group;
	GLMtriangle* triangle;
	GLMmaterial* material;
    char transparencyPass, transparencyGLStateIsSet, hasTransparency;

	assert(model);
	assert(model->vertices);

	/* do a bit of warning */
	if (mode & GLM_FLAT && !model->facetnorms) {
		EDEN_LOGe("glmDraw() warning: flat render mode requested "
			"with no facet normals defined.\n");
		mode &= ~GLM_FLAT;
	}
	if (mode & GLM_SMOOTH && !model->normals) {
		EDEN_LOGe("glmDraw() warning: smooth render mode requested "
			"with no normals defined.\n");
		mode &= ~GLM_SMOOTH;
	}
	if (mode & GLM_TEXTURE && !model->texcoords) {
		EDEN_LOGe("glmDraw() warning: texture render mode requested "
			"with no texture coordinates defined.\n");
		mode &= ~GLM_TEXTURE;
	}
	if (mode & GLM_FLAT && mode & GLM_SMOOTH) {
		EDEN_LOGe("glmDraw() warning: flat render mode requested "
			"and smooth render mode requested (using smooth).\n");
		mode &= ~GLM_FLAT;
	}
	if (mode & GLM_COLOR && !model->materials) {
		EDEN_LOGe("glmDraw() warning: color render mode requested "
			"with no materials defined.\n");
		mode &= ~GLM_COLOR;
	}
	if (mode & GLM_MATERIAL && !model->materials) {
		EDEN_LOGe("glmDraw() warning: material render mode requested "
			"with no materials defined.\n");
		mode &= ~GLM_MATERIAL;
	}
	if (mode & GLM_COLOR && mode & GLM_MATERIAL) {
		EDEN_LOGe("glmDraw() warning: color and material render mode requested "
			"using only material mode.\n");
		mode &= ~GLM_COLOR;
	}
	
	if (mode & GLM_COLOR) glEnable(GL_COLOR_MATERIAL);

    if (mode & GLM_TEXTURE) {
        // Reset texture matrix if texturing.
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        // Make sure all required textures are loaded. (Late loading)
        if (model->readTextureRequired) {
            for (i = 0; i < model->nummaterials; i++) {
                if (model->materials[i].texturemappath && !model->materials[i].texturemap_index) {
                    if (!readTextureAndSendToGL(contextIndex, model->materials[i].texturemappath, &(model->materials[i].texturemap_index), &(model->materials[i].texturemap_hasAlpha), FALSE, model->flipTextureV)) {
                        EDEN_LOGe("glmDrawArrays(): Error loading texture.\n");
                    }
                }
            }
            model->readTextureRequired = FALSE;
        }
    }

    glStateCacheBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Pass through entire model twice. In first pass, draw all opaque groups.
    // In second pass, draw non-opaque (transparent) groups.
    for (transparencyPass = 0; transparencyPass <= 1; transparencyPass++) {
        transparencyGLStateIsSet = FALSE;

        for (group = model->groups; group; group = group->next) {
        
            material = &model->materials[group->material];
            hasTransparency = material && (
#ifdef GLM_MATERIAL_TEXTURES
                                           (mode & GLM_TEXTURE && material->texturemap_hasAlpha) ||
#endif
                                           (mode & (GLM_MATERIAL | GLM_COLOR) && material->diffuse[3] < 1.0f)
                                           );
            if ((!transparencyPass && hasTransparency) || (transparencyPass && !hasTransparency)) continue; // Skip if group is wrong sort. (XOR).
            
            // Enable or disable blend here rather than at the start of each pass so that
            // it isn't called in the second pass unless the model actually has transparency.
            // This avoids unecessary (and sometimes costly) flip-flopping of the OpenGL blend
            // state when drawing opaque models consecutively.
            if (!transparencyGLStateIsSet) {
                if (!transparencyPass) glStateCacheDisableBlend();
                else glStateCacheEnableBlend();
                transparencyGLStateIsSet = TRUE;
            }

            if (material) {
                if (mode & GLM_MATERIAL) {
                    glStateCacheMaterialv(GL_AMBIENT, material->ambient);
                    glStateCacheMaterialv(GL_DIFFUSE, material->diffuse);
                    glStateCacheMaterialv(GL_SPECULAR, material->specular);
                    glStateCacheMaterial(GL_SHININESS, material->shininess);
                } else if (mode & GLM_COLOR) {
                    glColor4fv(material->diffuse);
                }
#ifdef GLM_MATERIAL_TEXTURES
                glStateCacheActiveTexture(GL_TEXTURE0);
                if (mode & GLM_TEXTURE) {
                    if (material->texturemap_index) {
                        EdenSurfacesTextureSet(contextIndex, material->texturemap_index);
                        glStateCacheEnableTex2D();                   
                    }
                    else glStateCacheDisableTex2D();
                } else {
                    glStateCacheDisableTex2D();
                }
#endif // GLM_MATERIAL_TEXTURES
            }

            // Draw the triangles. (Branches moved outside loop for speed.)
            glBegin(GL_TRIANGLES);
            if (mode & GLM_TEXTURE) {
                if (mode & GLM_FLAT) {
                    for (i = 0; i < group->numtriangles; i++) {
                        triangle = &T(group->triangles[i]);
                        glNormal3fv(&model->facetnorms[3 * triangle->findex]);
                        glTexCoord2fv(&model->texcoords[2 * triangle->tindices[0]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[0]]);
                        glTexCoord2fv(&model->texcoords[2 * triangle->tindices[1]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[1]]);
                        glTexCoord2fv(&model->texcoords[2 * triangle->tindices[2]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[2]]);
                    }
                } else if (mode & GLM_SMOOTH) {
                    for (i = 0; i < group->numtriangles; i++) {
                        triangle = &T(group->triangles[i]);
                        glNormal3fv(&model->normals[3 * triangle->nindices[0]]);
                        glTexCoord2fv(&model->texcoords[2 * triangle->tindices[0]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[0]]);
                        glNormal3fv(&model->normals[3 * triangle->nindices[1]]);
                        glTexCoord2fv(&model->texcoords[2 * triangle->tindices[1]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[1]]);
                        glNormal3fv(&model->normals[3 * triangle->nindices[2]]);
                        glTexCoord2fv(&model->texcoords[2 * triangle->tindices[2]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[2]]);
                    }
                } else {
                    for (i = 0; i < group->numtriangles; i++) {
                        triangle = &T(group->triangles[i]);
                        glTexCoord2fv(&model->texcoords[2 * triangle->tindices[0]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[0]]);
                        glTexCoord2fv(&model->texcoords[2 * triangle->tindices[1]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[1]]);
                        glTexCoord2fv(&model->texcoords[2 * triangle->tindices[2]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[2]]);
                    }
                }
            } else {
                if (mode & GLM_FLAT) {
                    for (i = 0; i < group->numtriangles; i++) {
                        triangle = &T(group->triangles[i]);
                        glNormal3fv(&model->facetnorms[3 * triangle->findex]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[0]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[1]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[2]]);
                    }
                } else if (mode & GLM_SMOOTH) {
                    for (i = 0; i < group->numtriangles; i++) {
                        triangle = &T(group->triangles[i]);
                        glNormal3fv(&model->normals[3 * triangle->nindices[0]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[0]]);
                        glNormal3fv(&model->normals[3 * triangle->nindices[1]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[1]]);
                        glNormal3fv(&model->normals[3 * triangle->nindices[2]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[2]]);
                    }
                } else {
                    for (i = 0; i < group->numtriangles; i++) {
                        triangle = &T(group->triangles[i]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[0]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[1]]);
                        glVertex3fv(&model->vertices[3 * triangle->vindices[2]]);
                    }
                }
            }
            glEnd();
        }
    }
    
    if (mode & GLM_COLOR) glDisable(GL_COLOR_MATERIAL);
}

/* glmList: Generates and returns a display list for the model using
 * the mode specified.
 *
 * model - initialized GLMmodel structure
 * mode  - a bitwise OR of values describing what is to be rendered.
 *             GLM_NONE     -  render with only vertices
 *             GLM_FLAT     -  render with facet normals
 *             GLM_SMOOTH   -  render with vertex normals
 *             GLM_TEXTURE  -  render with texture coords
 *             GLM_COLOR    -  render with colors (color material)
 *             GLM_MATERIAL -  render with materials
 *             GLM_COLOR and GLM_MATERIAL should not both be specified.  
 *             GLM_FLAT and GLM_SMOOTH should not both be specified.
 * contextIndex - PRL: index to the current OpenGL context (for texturing.) If you have only
 *             one OpenGL context (the most common case) set this parameter to 0.
 */
GLuint
glmList(GLMmodel* model, GLuint mode, const int contextIndex)
{
    GLuint list;
    
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glmDraw(model, mode, contextIndex);
    glEndList();
    
    return list;
}
#endif // !EDEN_OPENGLES


/* glmWeld: eliminate (weld) vectors that are within an epsilon of
 * each other.
 *
 * model   - initialized GLMmodel structure
 * epsilon     - maximum difference between vertices
 *               ( 0.00001 is a good start for a unitized model)
 *
 */
GLvoid
glmWeld(GLMmodel* model, GLfloat epsilon)
{
    GLfloat* vectors;
    GLfloat* copies;
    GLuint   numvectors;
    GLuint   i;
    
    /* vertices */
    numvectors = model->numvertices;
    vectors  = model->vertices;
    copies = glmWeldVectors(vectors, &numvectors, epsilon);
    
#if 0
    EDEN_LOGe("glmWeld(): %d redundant vertices.\n", 
        model->numvertices - numvectors - 1);
#endif
    
    for (i = 0; i < model->numtriangles; i++) {
        T(i).vindices[0] = (GLuint)vectors[3 * T(i).vindices[0] + 0];
        T(i).vindices[1] = (GLuint)vectors[3 * T(i).vindices[1] + 0];
        T(i).vindices[2] = (GLuint)vectors[3 * T(i).vindices[2] + 0];
    }
    
    /* free space for old vertices */
    free(vectors);
    
    /* allocate space for the new vertices */
    model->numvertices = numvectors;
    model->vertices = (GLfloat*)malloc(sizeof(GLfloat) * 
        3 * (model->numvertices + 1));
    
    /* copy the optimized vertices into the actual vertex list */
    for (i = 1; i <= model->numvertices; i++) {
        model->vertices[3 * i + 0] = copies[3 * i + 0];
        model->vertices[3 * i + 1] = copies[3 * i + 1];
        model->vertices[3 * i + 2] = copies[3 * i + 2];
    }
    
    free(copies);
}

/* glmReadPPM: read a PPM raw (type P6) file.  The PPM file has a header
 * that should look something like:
 *
 *    P6
 *    # comment
 *    width height max_value
 *    rgbrgbrgb...
 *
 * where "P6" is the magic cookie which identifies the file type and
 * should be the only characters on the first line followed by a
 * carriage return.  Any line starting with a # mark will be treated
 * as a comment and discarded.   After the magic cookie, three integer
 * values are expected: width, height of the image and the maximum
 * value for a pixel (max_value must be < 256 for PPM raw files).  The
 * data section consists of width*height rgb triplets (one byte each)
 * in binary format (i.e., such as that written with fwrite() or
 * equivalent).
 *
 * The rgb data is returned as an array of unsigned chars (packed
 * rgb).  The malloc()'d memory should be free()'d by the caller.  If
 * an error occurs, an error message is sent to stderr and NULL is
 * returned.
 *
 * filename   - name of the .ppm file.
 * width      - will contain the width of the image on return.
 * height     - will contain the height of the image on return.
 *
 */
GLubyte* 
glmReadPPM(const char* filename, int* width, int* height)
{
    FILE* fp;
    int i, w, h, d;
    unsigned char* image;
    char head[70];          /* max line <= 70 in PPM (per spec). */

    fp = fopen(filename, "rb");
    if (!fp) {
        EDEN_LOGe("glmReadPPM(): Unable to open file '%s'.\n", filename);
        EDEN_LOGperror(NULL);
        return NULL;
    }

    /* grab first two chars of the file and make sure that it has the
       correct magic cookie for a raw PPM file. */
    if (!fgets(head, 70, fp)) {
        EDEN_LOGe("%s: Error reading file header\n", filename);
        goto bail;
    };
    if (strncmp(head, "P6", 2)) {
        EDEN_LOGe("%s: Not a raw PPM file\n", filename);
        goto bail;
    }

    /* grab the three elements in the header (width, height, maxval). */
    i = 0;
    while(i < 3) {
        fgets(head, 70, fp);
        if (head[0] == '#')     /* skip comments. */
            continue;
        if (i == 0)
            i += sscanf(head, "%d %d %d", &w, &h, &d);
        else if (i == 1)
            i += sscanf(head, "%d %d", &h, &d);
        else if (i == 2)
            i += sscanf(head, "%d", &d);
    }

    /* grab all the image data in one fell swoop. */
    image = (unsigned char*)malloc(sizeof(unsigned char)*w*h*3);
    if (fread(image, sizeof(unsigned char), w*h*3, fp) < w*h*3) {
        EDEN_LOGe("%s: PPM image data truncated\n", filename);
        goto bail;
    };
    fclose(fp);

    *width = w;
    *height = h;
    return image;
bail:
    fclose(fp);
    return (NULL);
}

GLvoid glmCreateArrays(GLMmodel* model, GLuint mode)
{
	GLMgroup* group;
	GLMtriangle* triangle;
    GLMarray *array = NULL, *arrayTail;
    GLMnode2 *node, **members, *tail;
    GLfloat *vertices, *normals, *texcoords;
    GLushort *indices;
    GLushort indexCount, vntCount;
    int i, j;
    
    assert(model);
    assert(model->vertices);
    
    if (model->arrays) glmDeleteArrays(model);
    
    /* do a bit of warning */
    if (mode & GLM_FLAT && !model->facetnorms) {
        EDEN_LOGe("glmCreateArrays() warning: flat render mode requested "
                "with no facet normals defined.\n");
        mode &= ~GLM_FLAT;
    }
    if (mode & GLM_SMOOTH && !model->normals) {
        EDEN_LOGe("glmCreateArrays() warning: smooth render mode requested "
                "with no normals defined.\n");
        mode &= ~GLM_SMOOTH;
    }
    if (mode & GLM_TEXTURE && !model->texcoords) {
        EDEN_LOGe("glmCreateArrays() warning: texture render mode requested "
                "with no texture coordinates defined.\n");
        mode &= ~GLM_TEXTURE;
    }
    if (mode & GLM_COLOR && !model->materials) {
        EDEN_LOGe("glmCreateArrays() warning: color render mode requested "
                "with no materials defined.\n");
        mode &= ~GLM_COLOR;
    }
    if (mode & GLM_MATERIAL && !model->materials) {
        EDEN_LOGe("glmCreateArrays() warning: material render mode requested "
                "with no materials defined.\n");
        mode &= ~GLM_MATERIAL;
    }
    if (mode & GLM_COLOR && mode & GLM_MATERIAL) {
        EDEN_LOGe("glmCreateArrays() warning: color and material render mode requested "
                "using only material mode.\n");
        mode &= ~GLM_COLOR;
    }

    group = model->groups;
    while (group) {
    
        if (group->numtriangles) {
            
            int needNormals;
            int needFacetnorms;
            int needTexcoords;

			arrayTail = array;
            array = (GLMarray *)calloc(1, sizeof(GLMarray)); // implicit array->next = NULL;
            if (!arrayTail) {
                model->arrays = array;
                model->arrayMode = mode;
            } else arrayTail->next = array;
            
            // For every point in every triangle in the group, look to see if another point in another
            // triangle in the group already has the exact same vertex, normal and texcooord.
            // If it does, stash the index for that data in our list of indices.
            // If not, then copy the vertex, normal and texcoord from the model, stash the index
            // AND record that this data exists by adding to the existing per-vertex
            // linked-list (the members array) of GLMnodes.
            
            // Allocate a structure that will hold a linked list of triangle
            // indices for each vertex.
            members = (GLMnode2**)calloc(model->numvertices + 1, sizeof(GLMnode2 *)); // + 1 because indices in model are 1-based.
            
            needNormals = mode & GLM_SMOOTH;
            needFacetnorms = mode & GLM_FLAT;
            needTexcoords = mode & GLM_TEXTURE;
            
            // Allocate arrays to hold the new vertex, normal and texcoord data.
            // The arrays will be the maximum theoretical size, and we will shrink them at the end.
            vertices = (GLfloat *)malloc(sizeof(GLfloat) * 3 * group->numtriangles * 3); // 3 floats per vertex, 3 vertices per triangle.
            if (needNormals || needFacetnorms) normals = (GLfloat *)malloc(sizeof(GLfloat) * 3 * group->numtriangles * 3); // 3 floats per normal, 3 normals per triangle.
            else normals = NULL;
            if (needTexcoords) texcoords = (GLfloat *)malloc(sizeof(GLfloat) * 2 * group->numtriangles * 3); // 2 floats per texcoord, 3 texcoords per triangle.
            else texcoords = NULL;
            vntCount = 0;
            
            indices = (GLushort *)malloc(sizeof(GLushort) * group->numtriangles * 3); // 1 index per v/n/t, 3 v/n/ts per triangle.
            indexCount = 0;
            
            for (i = 0; i < glmMin(group->numtriangles, (USHRT_MAX + 1) / 3); i++) { // We are using GLushorts for indices, so limit to USHRT_MAX / 3 triangles per group.
                triangle = &(T(group->triangles[i]));
                for (j = 0; j < 3; j++) {
                    // We are now examining the linked list for vertex with index (1-based) T(i).vindices[j].
                    node = members[triangle->vindices[j]]; // Get current head of linked-list.
                    while (node) {
                        if (!needNormals || ((model->normals[triangle->nindices[j] * 3    ] == normals[node->index * 3    ]) &&
                                             (model->normals[triangle->nindices[j] * 3 + 1] == normals[node->index * 3 + 1]) &&
                                             (model->normals[triangle->nindices[j] * 3 + 2] == normals[node->index * 3 + 2]))) {
                            if (!needFacetnorms || ((model->facetnorms[triangle->findex * 3    ] == normals[node->index * 3    ]) &&
                                                    (model->facetnorms[triangle->findex * 3 + 1] == normals[node->index * 3 + 1]) &&
                                                    (model->facetnorms[triangle->findex * 3 + 2] == normals[node->index * 3 + 2]))) {
                                if (!needTexcoords || ((model->texcoords[triangle->tindices[j] * 2    ] == texcoords[node->index * 2    ]) &&
                                                       (model->texcoords[triangle->tindices[j] * 2 + 1] == texcoords[node->index * 2 + 1]))) {
                                    break; // The data for "node" is a match, so reuse its index.
                                }
                            }
                        }
                        node = node->next;
                    }
                    if (!node) {
                        // No re-usable set of data (vertex, texcoord, normal) was found, make a new one.
                        // First make a node to point to it.
                        node = (GLMnode2 *)malloc(sizeof(GLMnode2));
                        node->index = vntCount;
                        node->next = members[triangle->vindices[j]];     // Link to the current head of the list
                        members[triangle->vindices[j]] = node;			// and make this node the new head.
                        // Now copy the data; (vx, vy, vz), (nx, ny, nz), (tu, tv).
                        vertices[vntCount * 3    ] = model->vertices[triangle->vindices[j] * 3    ];
                        vertices[vntCount * 3 + 1] = model->vertices[triangle->vindices[j] * 3 + 1];
                        vertices[vntCount * 3 + 2] = model->vertices[triangle->vindices[j] * 3 + 2];
                        if (needNormals) {
                            normals[vntCount * 3    ] = model->normals[triangle->nindices[j] * 3    ];
                            normals[vntCount * 3 + 1] = model->normals[triangle->nindices[j] * 3 + 1];
                            normals[vntCount * 3 + 2] = model->normals[triangle->nindices[j] * 3 + 2];
                        } else if (needFacetnorms) {
                            normals[vntCount * 3    ] = model->facetnorms[triangle->findex * 3    ];
                            normals[vntCount * 3 + 1] = model->facetnorms[triangle->findex * 3 + 1];
                            normals[vntCount * 3 + 2] = model->facetnorms[triangle->findex * 3 + 2];
                        }
                        if (needTexcoords) {
                            texcoords[vntCount * 2    ] = model->texcoords[triangle->tindices[j] * 2    ];
                            texcoords[vntCount * 2 + 1] = model->texcoords[triangle->tindices[j] * 2 + 1];
                        }
                        indices[indexCount++] = vntCount++;
                    } else {
                        indices[indexCount++] = node->index;
                    }
                } // for j
            } // for i
            
            // Compress the v/n/t arrays by creating new allocation and copying the data.
            array->vertices = (GLfloat *)malloc(sizeof(GLfloat) * 3 * vntCount);
            for (i = 0; i < 3 * vntCount; i++) array->vertices[i] = vertices[i];
            free (vertices);
            if (needNormals) {
                array->normals = (GLfloat *)malloc(sizeof(GLfloat) * 3 * vntCount);
                for (i = 0; i < 3 * vntCount; i++) array->normals[i] = normals[i];
                free (normals);
            }
            if (needTexcoords) {
                array->texcoords = (GLfloat *)malloc(sizeof(GLfloat) * 2 * vntCount);
                for (i = 0; i < 2 * vntCount; i++) array->texcoords[i] = texcoords[i];
                free (texcoords);
            }
            
            // Clean up the members array.
            for (i = 1; i <= model->numvertices; i++) {
                node = members[i];
                while (node) {
                    tail = node;
                    node = node->next;
                    free(tail);
                }
            }
            free(members);
            
            array->indices = indices;
            array->indexCount = indexCount;
            
            // If this group has a material set, stash a pointer to it.
            if (group->material) array->material = &(model->materials[group->material]);
            
        } // if group->numtriangles
        
        group = group->next;
    } // while group
    
}

GLvoid glmDeleteArrays(GLMmodel *model)
{
    GLMarray *arrayHead, *arrayPrevHead;
    
    assert(model);

    arrayHead = model->arrays;
    while (arrayHead) {
        free(arrayHead->vertices);
        if (arrayHead->normals) free(arrayHead->normals);
        if (arrayHead->texcoords) free(arrayHead->texcoords);
        free(arrayHead->indices);
        
        arrayPrevHead = arrayHead;
        arrayHead = arrayHead->next;
        free(arrayPrevHead);
    }
    model->arrays = NULL;
    model->arrayMode = 0;
}

GLvoid glmDrawArrays(GLMmodel* model, const int contextIndex)
{
	GLMarray* array;
    char transparencyPass, transparencyGLStateIsSet, hasTransparency;
    int i;
 
    assert(model);

    if (model->arrayMode & GLM_COLOR) glEnable(GL_COLOR_MATERIAL);

    if (model->arrayMode & GLM_TEXTURE) {
        // Reset texture matrix if texturing.
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        // Make sure all required textures are loaded. (Late loading)
        if (model->readTextureRequired) {
            for (i = 0; i < model->nummaterials; i++) {
                if (model->materials[i].texturemappath && !model->materials[i].texturemap_index) {
                    if (!readTextureAndSendToGL(contextIndex, model->materials[i].texturemappath, &(model->materials[i].texturemap_index), &(model->materials[i].texturemap_hasAlpha), FALSE, model->flipTextureV)) {
                        EDEN_LOGe("glmDrawArrays(): Error loading texture.\n");
                    }
                }
            }
            model->readTextureRequired = FALSE;
        }
    }
    
    // Pass through entire model twice. In first pass, draw all opaque groups.
    // In second pass, draw non-opaque (transparent) groups.
    for (transparencyPass = 0; transparencyPass <= 1; transparencyPass++) {
        transparencyGLStateIsSet = FALSE;
        
        for (array = model->arrays; array != NULL; array = array->next) { // Loop through all arrays.
  
            hasTransparency = array->material && (
#ifdef GLM_MATERIAL_TEXTURES
                                                  (model->arrayMode & GLM_TEXTURE && array->material->texturemap_hasAlpha) ||
#endif
                                                  (model->arrayMode & (GLM_MATERIAL | GLM_COLOR) && array->material->diffuse[3] < 1.0f)
                                                  );
            if ((!transparencyPass && hasTransparency) || (transparencyPass && !hasTransparency)) continue; // Skip if array is wrong sort. (XOR).

            // Enable or disable blend here rather than at the start of each pass so that
            // it isn't called in the second pass unless the model actually has transparency.
            // This avoids unecessary (and sometimes costly) flip-flopping of the OpenGL blend
            // state when drawing opaque models consecutively.
            if (!transparencyGLStateIsSet) {
                if (!transparencyPass) glStateCacheDisableBlend();
                else {
                    glStateCacheBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glStateCacheEnableBlend();
                }
                transparencyGLStateIsSet = TRUE;
            }

            // Set array state.
            glVertexPointer(3, GL_FLOAT, 0, array->vertices);
            glStateCacheEnableClientStateVertexArray();
            if (array->normals) {
                glNormalPointer(GL_FLOAT, 0, array->normals);
                glStateCacheEnableClientStateNormalArray();
            } else {
                glStateCacheDisableClientStateNormalArray();
            }
            glStateCacheClientActiveTexture(GL_TEXTURE0);
            if (array->texcoords) {
                glTexCoordPointer(2, GL_FLOAT, 0, array->texcoords);
                glStateCacheEnableClientStateTexCoordArray();
            } else {
                glStateCacheDisableClientStateTexCoordArray();
            }
            
            // Set material state.
            if (array->material) {
                if (model->arrayMode & GLM_MATERIAL) {
                    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, array->material->ambient);
                    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, array->material->diffuse);
                    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, array->material->specular);
                    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, array->material->shininess);
                } else if (model->arrayMode & GLM_COLOR) {
                    glColor4f(array->material->diffuse[0], array->material->diffuse[1], array->material->diffuse[2], array->material->diffuse[3]);
                }
#ifdef GLM_MATERIAL_TEXTURES
                glStateCacheActiveTexture(GL_TEXTURE0);
                if (model->arrayMode & GLM_TEXTURE) {
                    if (array->material->texturemap_index) {
                        EdenSurfacesTextureSet(contextIndex, array->material->texturemap_index);
                        glStateCacheEnableTex2D();
                    }
                    else glStateCacheDisableTex2D();
                } else {
                    glStateCacheDisableTex2D();
                }
#endif // GLM_MATERIAL_TEXTURES
            }
            
            // Draw the group.
            glDrawElements(GL_TRIANGLES, array->indexCount, GL_UNSIGNED_SHORT, array->indices);
        }
    } // transparency pass.

    if (model->arrayMode & GLM_COLOR) glDisable(GL_COLOR_MATERIAL);
}

#pragma mark -

#if 0
/* normals */
if (model->numnormals) {
    numvectors = model->numnormals;
    vectors  = model->normals;
    copies = glmOptimizeVectors(vectors, &numvectors);
    
    EDEN_LOGe("glmOptimize(): %d redundant normals.\n", 
        model->numnormals - numvectors);
    
    for (i = 0; i < model->numtriangles; i++) {
        T(i).nindices[0] = (GLuint)vectors[3 * T(i).nindices[0] + 0];
        T(i).nindices[1] = (GLuint)vectors[3 * T(i).nindices[1] + 0];
        T(i).nindices[2] = (GLuint)vectors[3 * T(i).nindices[2] + 0];
    }
    
    /* free space for old normals */
    free(vectors);
    
    /* allocate space for the new normals */
    model->numnormals = numvectors;
    model->normals = (GLfloat*)malloc(sizeof(GLfloat) * 
        3 * (model->numnormals + 1));
    
    /* copy the optimized vertices into the actual vertex list */
    for (i = 1; i <= model->numnormals; i++) {
        model->normals[3 * i + 0] = copies[3 * i + 0];
        model->normals[3 * i + 1] = copies[3 * i + 1];
        model->normals[3 * i + 2] = copies[3 * i + 2];
    }
    
    free(copies);
}

/* texcoords */
if (model->numtexcoords) {
    numvectors = model->numtexcoords;
    vectors  = model->texcoords;
    copies = glmOptimizeVectors(vectors, &numvectors);
    
    EDEN_LOGe("glmOptimize(): %d redundant texcoords.\n", 
        model->numtexcoords - numvectors);
    
    for (i = 0; i < model->numtriangles; i++) {
        for (j = 0; j < 3; j++) {
            T(i).tindices[j] = (GLuint)vectors[3 * T(i).tindices[j] + 0];
        }
    }
    
    /* free space for old texcoords */
    free(vectors);
    
    /* allocate space for the new texcoords */
    model->numtexcoords = numvectors;
    model->texcoords = (GLfloat*)malloc(sizeof(GLfloat) * 
        2 * (model->numtexcoords + 1));
    
    /* copy the optimized vertices into the actual vertex list */
    for (i = 1; i <= model->numtexcoords; i++) {
        model->texcoords[2 * i + 0] = copies[2 * i + 0];
        model->texcoords[2 * i + 1] = copies[2 * i + 1];
    }
    
    free(copies);
}
#endif

#if 0
/* look for unused vertices */
/* look for unused normals */
/* look for unused texcoords */
for (i = 1; i <= model->numvertices; i++) {
    for (j = 0; j < model->numtriangles; i++) {
        if (T(j).vindices[0] == i || 
            T(j).vindices[1] == i || 
            T(j).vindices[1] == i)
            break;
    }
}
#endif
#undef T
