/*
 *  ARActivity.java
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
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb
 *
 */
package org.artoolkit.ar.samples.ARSimple.shader;

import android.opengl.GLES20;

import org.artoolkit.ar.base.rendering.gles20.OpenGLShader;
import org.artoolkit.ar.base.rendering.gles20.ShaderProgram;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;

/**
 * Created by Thorsten Bux on 21.01.2016.
 *
 * The shader program links together the vertex shader and the fragment shader and compiles them.
 * It also is responsible for binding the attributes. Attributes can be used to pass in values to the
 * shader during runtime.
 *
 * Finally it renders the given geometry.
 */
public class SimpleShaderProgram extends ShaderProgram {

    /**
     * Constructor for the shader program. Most of the work is done in the {@link ShaderProgram} directly.
     * @param vertexShader Vertex shader used to transform the rendered geometry
     * @param fragmentShader Fragment shader used to color the rendered geometry
     */
    public SimpleShaderProgram(OpenGLShader vertexShader, OpenGLShader fragmentShader) {
        super(vertexShader, fragmentShader);
        bindAttributes();
    }

    /**
     * Get the projection matrix handle from the shader. This handel is used later to pass in the
     * projection matrix to the vertex shader.
     * @return The handle for the projection matrix
     */
    @Override
    public int getProjectionMatrixHandle() {
        return GLES20.glGetUniformLocation(shaderProgramHandle, OpenGLShader.projectionMatrixString);
    }

    /**
     * Get the model view matrix handle from the shader. This handel is used later to pass in the
     * model view matrix to the vertex shader.
     * @return The handle for the model view matrix
     */
    @Override
    public int getModelViewMatrixHandle() {
        return GLES20.glGetUniformLocation(shaderProgramHandle, OpenGLShader.modelViewMatrixString);
    }

    /**
     * Binds the configurable attributes from the fragment and vertex shader to a specified int value.
     */
    @Override
    protected void bindAttributes() {
        // Bind attributes
        GLES20.glBindAttribLocation(shaderProgramHandle, 0, OpenGLShader.positionVectorString);
        GLES20.glBindAttribLocation(shaderProgramHandle, 1, SimpleVertexShader.colorVectorString);
    }

    /**
     *
     * @return The handle for the position of the geometry. Used later to pass in the position of
     * the marker that comes from the ARToolKit.
     */
    public int getPositionHandle() {
        return GLES20.glGetAttribLocation(shaderProgramHandle, OpenGLShader.positionVectorString);
    }

    /**
     * @return The handle for the color of the geometry. Used later to pass in the color of
     * the geometry.
     */
    public int getColorHandle() {
        return GLES20.glGetAttribLocation(shaderProgramHandle, SimpleVertexShader.colorVectorString);
    }

    /**
     * There are several render methods available from the base class. In this case we override the {@link #render(FloatBuffer, FloatBuffer, ByteBuffer)} one.
     * Although we never use the index ByteBuffer.
     * We pass in the vertex and color information from the {@link org.artoolkit.ar.base.rendering.gles20.LineGLES20} object.
     *
     * @param vertexBuffer Contains the position information as two vertexes. Start and end of the line to draw
     * @param colorBuffer  Contains the color of the line
     * @param indexBuffer TODO
     */
    @Override
    public void render(FloatBuffer vertexBuffer, FloatBuffer colorBuffer, ByteBuffer indexBuffer) {
        setupShaderUsage();

        vertexBuffer.position(0);
        /**
         * We use the OpenGL methods to set the vertex information in the following order.
         * 1. The handle generated by the {@link MarkerDistanceShaderProgram}
         * 2. Size of the position information. As we operate in 3D space this is 3 (x,y,z) (but for OpenGL and matrix operations it could also be 4, as you need a 4 size vector for 4x4 matrix operations).
         * 3. Kind of the vector position data (Float, Double)
         * 4. Is the vector normalized?
         * 5. The distance in Bytes between each vertex information including the vertex itself. First
         *      thing to remember here is that we are very close to C programming. That is why everything
         *      is handled in bytes and why we try and optimize things. The other thing is that you might want
         *      to use your vertexBuffer as container for position and color information (eg: [pos(x,y,z),color(r,g,b),pos(x,y,z),...])
         *      That is why you need to specify the distance between each vertex. In this case the vertexBuffer only holds the position information
         *      So the distance in byte is position size (3 as described in point 2.) multiplied with bytes per float (4)
         * 6. The vertex information itself.
         */
        //camPosition.length * 4 bytes per float
        GLES20.glVertexAttribPointer(this.getPositionHandle(), positionDataSize, GLES20.GL_FLOAT, false,
                positionStrideBytes, vertexBuffer);
        GLES20.glEnableVertexAttribArray(this.getPositionHandle());

        // Pass in the color information
        colorBuffer.position(0);
        /** Pass the color information to OpenGL
         * 1. The handle generated by the {@link MarkerDistanceShaderProgram}
         * 2. Pass in 1 as count of color vertexes (my line has only one color)
         * 3. The color information itself.
         */
        GLES20.glVertexAttribPointer(this.getColorHandle(), colorDataSize, GLES20.GL_FLOAT, false,
                colorStrideBytes, colorBuffer);

        GLES20.glEnableVertexAttribArray(this.getColorHandle());

        //Finally draw the geometry as triangles
        //The geometry consists of 36 points each represented by a x,y,z vector
        //The index buffer tells the renderer how the vector points are combined together.
        //eg. combine vertex 1,2,3 for the first triangle and 2,3,4 for the next triangle, ...
        GLES20.glDrawElements(GLES20.GL_TRIANGLES, 36, GLES20.GL_UNSIGNED_BYTE, indexBuffer);

    }
}
