/*    
      glm.h
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

#ifndef __glm_h__
#define __glm_h__

#define GLM_MATERIAL_TEXTURES	// PRL 2003-08-06: Uncomment to build glm with provision for texture mapping in materials.

#ifdef GLM_MATERIAL_TEXTURES
#  ifndef __Eden_h__
#    include <Eden/Eden.h>
#  endif
#  include <Eden/EdenSurfaces.h>		// PRL 20030806: TEXTURE_INFO_t, TEXTURE_INDEX_t, EdenSurfacesTextureLoad(), EdenSurfacesTextureSet(), EdenSurfacesTextureUnload()
#endif // GLM_MATERIAL_TEXTURES

#ifdef EDEN_MACOSX
#  include <OpenGL/gl.h>
#elif defined(EDEN_OPENGLES)
#  if defined ANDROID
#    include <GLES/gl.h>
#    include <GLES/glext.h>
#  else
#    include <OpenGLES/ES1/gl.h>
#    include <OpenGLES/ES1/glext.h>
#  endif
#else
#  include <GL/gl.h>
#endif
    
#ifdef __cplusplus
extern "C" {
#endif
	
#define GLM_NONE     (0)            /* render with only vertices */
#define GLM_FLAT     (1 << 0)       /* render with facet normals */
#define GLM_SMOOTH   (1 << 1)       /* render with vertex normals */
#define GLM_TEXTURE  (1 << 2)       /* render with texture coords */
#define GLM_COLOR    (1 << 3)       /* render with colors */
#define GLM_MATERIAL (1 << 4)       /* render with materials */

/* GLMmaterial: Structure that defines a material in a model. 
 */
typedef struct _GLMmaterial
{
    char* name;                     /* name of material */
    GLfloat diffuse[4];             /* diffuse component */
    GLfloat ambient[4];             /* ambient component */
    GLfloat specular[4];            /* specular component */
    GLfloat emmissive[4];           /* emmissive component */
    GLfloat shininess;              /* specular exponent */
    /*
     0	 Color on and Ambient off 
     1	 Color on and Ambient on 
     2	 Highlight on 
     3	 Reflection on and Ray trace on 
     4	 Transparency: Glass on 
         Reflection: Ray trace on 
     5	 Reflection: Fresnel on and Ray trace on 
     6	 Transparency: Refraction on 
         Reflection: Fresnel off and Ray trace on 
     7	 Transparency: Refraction on 
         Reflection: Fresnel on and Ray trace on 
     8	 Reflection on and Ray trace off 
     9	 Transparency: Glass on 
         Reflection: Ray trace off 
     10	 Casts shadows onto invisible surfaces
     */
    GLint illum;                    // Illumination model.
#ifdef GLM_MATERIAL_TEXTURES
    char* texturemap;               // PRL 2003-08-06: path to texture file, relative to material file.
    char* texturemappath;           // PRL 2012-09-25: path to texture file, relative to working directory.
    TEXTURE_INDEX_t texturemap_index; // PRL 2003-08-06: index to loaded texture. 0 indicates not loaded.
    char texturemap_hasAlpha;       // Whether texture has an alpha channel.
#endif // GLM_MATERIAL_TEXTURES
} GLMmaterial;
    
/* GLMtriangle: Structure that defines a triangle in a model.
 */
typedef struct _GLMtriangle {
    GLuint vindices[3];           /* array of triangle vertex indices */
    GLuint nindices[3];           /* array of triangle normal indices */
    GLuint tindices[3];           /* array of triangle texcoord indices*/
    GLuint findex;                /* index of triangle facet normal */
} GLMtriangle;

/* GLMgroup: Structure that defines a group in a model.
 */
typedef struct _GLMgroup {
    char*             name;           /* name of this group */
    GLuint            numtriangles;   /* number of triangles in this group */
    GLuint*           triangles;      /* array of triangle _indices_ */
    GLuint            material;       /* index to material for group */
    struct _GLMgroup* next;           /* pointer to next group in model */
} GLMgroup;

typedef struct _GLMarray {
    GLfloat *vertices;
    GLfloat *normals;
    GLfloat *texcoords;
    GLushort *indices;
    GLushort indexCount;
    GLMmaterial *material;
    struct _GLMarray *next;
} GLMarray;

    
/* GLMmodel: Structure that defines a model.
 */
typedef struct _GLMmodel {
    char*    pathname;            /* path to this model */
    char*    mtllibname;          /* name of the material library */

    GLuint   numvertices;         /* number of vertices in model */
    GLfloat* vertices;            /* array of vertices  */

    GLuint   numnormals;          /* number of normals in model */
    GLfloat* normals;             /* array of normals */

    GLuint   numtexcoords;        /* number of texcoords in model */
    GLfloat* texcoords;           /* array of texture coordinates */

    GLuint   numfacetnorms;       /* number of facetnorms in model */
    GLfloat* facetnorms;          /* array of facetnorms */

    GLuint       numtriangles;    /* number of triangles in model */
    GLMtriangle* triangles;       /* array of triangles */

    GLuint       nummaterials;    /* number of materials in model */
    GLMmaterial* materials;       /* array of materials */

    GLuint       numgroups;       /* number of groups in model */
    GLMgroup*    groups;          /* linked list of groups */

    GLMarray *arrays;             // Optimised arrays of vertices, normals, texcoords for fast drawing.
    GLuint arrayMode;             // The GLM_* mode with which the arrays were created.
    
    GLboolean readTextureRequired; // Set if textures need to be read in and submitted to OpenGL.
    GLboolean flipTextureV;       // Set if textures need to be flipped in vertical dimension.
    
} GLMmodel;


/* glmUnitize: "unitize" a model by translating it to the origin and
 * scaling it to fit in a unit cube around the origin.  Returns the
 * scalefactor used.
 *
 * model - properly initialized GLMmodel structure 
 */
GLfloat
glmUnitize(GLMmodel* model);

/* glmDimensions: Calculates the dimensions (width, height, depth) of
 * a model.
 *
 * model      - initialized GLMmodel structure
 * dimensions - array of 3 GLfloats (GLfloat dimensions[3])
 */
GLvoid
glmDimensions(GLMmodel* model, GLfloat* dimensions);

/* glmScale: Scales a model by a given amount.
 * 
 * model - properly initialized GLMmodel structure
 * scale - scalefactor (0.5 = half as large, 2.0 = twice as large)
 */
GLvoid
glmScale(GLMmodel* model, GLfloat scale);

/* glmScaleTextures: Scales a model's textures by a given amount.
 * 
 * model - properly initialized GLMmodel structure
 * scale - scalefactor (0.5 = half as large, 2.0 = twice as large)
 */
GLvoid
glmScaleTextures(GLMmodel* model, GLfloat scale);

/* glmTranslate: Translates (moves) a model by a given amount in x, y, z.
 *
 * model - properly initialized GLMmodel structure
 * offset- vector specifying amount to translate model by.
 */
GLvoid
glmTranslate(GLMmodel* model, const GLfloat offset[3]);

/* glmRotate: Rotates a model by a given amount angle (in radians) about axis in x, y, z.
 *
 * model - properly initialized GLMmodel structure
 * angle - scalar specifying amount to rotate model by, in radians.
 * x, y, z - scalar components of a normalized vector specifiying axis of rotation.
 */
GLvoid
glmRotate(GLMmodel* model, const GLfloat angle, const GLfloat x, const GLfloat y, const GLfloat z);

/* glmReverseWinding: Reverse the polygon winding for all polygons in
 * this model.  Default winding is counter-clockwise.  Also changes
 * the direction of the normals.
 * 
 * model - properly initialized GLMmodel structure 
 */
GLvoid
glmReverseWinding(GLMmodel* model);

/* glmFacetNormals: Generates facet normals for a model (by taking the
 * cross product of the two vectors derived from the sides of each
 * triangle).  Assumes a counter-clockwise winding.
 *
 * model - initialized GLMmodel structure
 */
GLvoid
glmFacetNormals(GLMmodel* model);

/* glmVertexNormals: Generates smooth vertex normals for a model.
 * First builds a list of all the triangles each vertex is in.  Then
 * loops through each vertex in the the list averaging all the facet
 * normals of the triangles each vertex is in.  Finally, sets the
 * normal index in the triangle for the vertex to the generated smooth
 * normal.  If the dot product of a facet normal and the facet normal
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
glmVertexNormals(GLMmodel* model, GLfloat angle);

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
glmLinearTexture(GLMmodel* model);

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
glmSpheremapTexture(GLMmodel* model);

/* glmDelete: Deletes a GLMmodel structure.
 *
 * model - initialized GLMmodel structure
 * contextIndex - PRL: index to the current OpenGL context (for texturing.) If you have only
 *             one OpenGL context (the most common case) set this parameter to 0.
 */
GLvoid
glmDelete(GLMmodel* model, const int contextIndex);

/* glmReadOBJ: Reads a model description from a Wavefront .OBJ file.
 * Returns a pointer to the created object which should be free'd with
 * glmDelete().
 *
 * filename - name of the file containing the Wavefront .OBJ format data.
 * contextIndex - PRL: index to the current OpenGL context (for texturing.)
 */
GLMmodel* 
glmReadOBJ(const char *filename, const int contextIndex);

/* glmReadOBJ2: Reads a model description from a Wavefront .OBJ file.
 * Returns a pointer to the created object which should be free'd with
 * glmDelete().
 * Whereas glReadOBJ will attempt to load and submit to OpenGL any
 * textures specified in an accompanying materials file, this variant allows
 * texture loading to be delayed until the model is actually to be drawn,
 * i.e. lazily loaded.
 *
 * filename - name of the file containing the Wavefront .OBJ format data.
 * contextIndex - PRL: index to the current OpenGL context (for texturing.)
 * readTexturesNow - TRUE to load and submit to OpenGL any textures specified
 * in an accompanying materials file, or FALSE to lazily load textures
 * at the time of a later glmDraw() or glmDrawArrays call().
 */
GLMmodel*
glmReadOBJ2(const char *filename, const int contextIndex, const GLboolean readTexturesNow);
    
/* glmReadOBJ2: Reads a model description from a Wavefront .OBJ file.
 * Returns a pointer to the created object which should be free'd with
 * glmDelete().
 * Whereas glReadOBJ will attempt to load and submit to OpenGL any
 * textures specified in an accompanying materials file, this variant allows
 * texture loading to be delayed until the model is actually to be drawn,
 * i.e. lazily loaded.
 *
 * filename - name of the file containing the Wavefront .OBJ format data.
 * contextIndex - PRL: index to the current OpenGL context (for texturing.)
 * readTexturesNow - TRUE to load and submit to OpenGL any textures specified
 * in an accompanying materials file, or FALSE to lazily load textures
 * at the time of a later glmDraw() or glmDrawArrays call().
 * flipTextureV - If TRUE, textures will be flipped in the V (vertical)
 * dimension as they are loaded. Some .obj files are written with textures flipped
 * in this way. If FALSE, behaviour is the same as for glmReadOBJ2().
 */
GLMmodel*
glmReadOBJ3(const char *filename, const int contextIndex, const GLboolean readTexturesNow, const GLboolean flipTextureV);
    
/* glmWriteOBJ: Writes a model description in Wavefront .OBJ format to
 * a file.
 *
 * model    - initialized GLMmodel structure
 * filename - name of the file to write the Wavefront .OBJ format data to
 * mode     - a bitwise or of values describing what is written to the file
 *            GLM_NONE    -  write only vertices
 *            GLM_FLAT    -  write facet normals
 *            GLM_SMOOTH  -  write vertex normals
 *            GLM_TEXTURE -  write texture coords
 *            GLM_FLAT and GLM_SMOOTH should not both be specified.
 */
GLvoid
glmWriteOBJ(GLMmodel* model, char* filename, GLuint mode);

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
glmDraw(GLMmodel* model, GLuint mode, const int contextIndex);

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
glmList(GLMmodel* model, GLuint mode, const int contextIndex);
#endif // !EDEN_OPENGLES

/* glmWeld: eliminate (weld) vectors that are within an epsilon of
 * each other.
 *
 * model      - initialized GLMmodel structure
 * epsilon    - maximum difference between vertices
 *              ( 0.00001 is a good start for a unitized model)
 *
 */
GLvoid
glmWeld(GLMmodel* model, GLfloat epsilon);


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
glmReadPPM(const char* filename, int* width, int* height);

/* glmCreateArrays: Creates a linked list of vertex, normal, and texcoord data arrays and
 * a set of triangle indices for the model specified using the mode specified.
 *
 * Each node in the list represents one group in the model.
 * The created arrays are added to the model and do not track any other state changes in
 * the model apart from material properties, so if you change the model, or if you wish
 * to draw using a different mode, you will need  to regenerate the arrays by calling
 * glmCreateArrays again.
 *
 * This function must be called before any call to glmDrawArrays().
 *
 * By Philip Lamb, phil@eden.net.nz, 2008-07-30
 *
 * model - initialized GLMmodel structure.
 * mode  - a bitwise OR of values describing what is to be rendered.
 *             GLM_NONE     -  render with only vertices
 *             GLM_FLAT     -  render with facet normals
 *             GLM_SMOOTH   -  render with vertex normals
 *             GLM_TEXTURE  -  render with texture coords
 *             GLM_COLOR    -  render with colors (color material)
 *             GLM_MATERIAL -  render with materials
 *             GLM_COLOR and GLM_MATERIAL should not both be specified.
 *             GLM_FLAT and GLM_SMOOTH should not both be specified.
 */
GLvoid glmCreateArrays(GLMmodel* model, GLuint mode);

/* glmDeleteArrays: Remove any arrays from a model structure.
 *
 * This function may be called after all calsl to glmDrawArrays() are complete
 * to free up memory associated with the arrays. This will be done
 * automatically if the model itself is deleted (by a call to glmDelete()).
 *
 * By Philip Lamb, phil@eden.net.nz, 2008-07-30
 *
 * model - initialized GLMmodel structure.
 */
GLvoid glmDeleteArrays(GLMmodel *model);
    

/* glmDrawArrays: Draws a model using array data created by glmCreateArrays().
 *
 * By Philip Lamb, phil@eden.net.nz, 2008-07-30
 *
 * model - initialized GLMmodel structure, which has had arrays created.
 * contextIndex - index to the current OpenGL context (for texturing.) If you have only
 *             one OpenGL context (the most common case) set this parameter to 0.
 */
GLvoid glmDrawArrays(GLMmodel* model, const int contextIndex);

#ifdef __cplusplus
}
#endif

#endif                  /* !__glm_h__ */
