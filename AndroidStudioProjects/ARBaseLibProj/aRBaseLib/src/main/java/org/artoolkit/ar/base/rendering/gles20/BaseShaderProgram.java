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
package org.artoolkit.ar.base.rendering.gles20;

import android.opengl.GLES20;

/**
 * The shader program links together the vertex shader and the fragment shader and compiles them.
 * It also is responsible for binding the attributes. Attributes can be used to pass in values to the
 * shader during runtime.
 * <p/>
 * Finally it renders the given geometry.
 */
public class BaseShaderProgram extends ShaderProgram {

    public BaseShaderProgram(OpenGLShader vertexShader, OpenGLShader fragmentShader) {
        super(vertexShader, fragmentShader);
        bindAttributes();
    }

    protected void bindAttributes() {
        // Bind attributes
        GLES20.glBindAttribLocation(shaderProgramHandle, 0, OpenGLShader.positionVectorString);
    }

    @Override
    public int getProjectionMatrixHandle() {
        return GLES20.glGetUniformLocation(shaderProgramHandle, OpenGLShader.projectionMatrixString);
    }

    @Override
    public int getModelViewMatrixHandle() {
        return GLES20.glGetUniformLocation(shaderProgramHandle, OpenGLShader.modelViewMatrixString);
    }

    public int getPositionHandle() {
        return GLES20.glGetAttribLocation(shaderProgramHandle, OpenGLShader.positionVectorString);
    }

    public void render(float[] position) {
        setupShaderUsage();

        //camPosition.length * 4 bytes per float
        GLES20.glVertexAttribPointer(this.getPositionHandle(), position.length, GLES20.GL_FLOAT, false,
                position.length * 4, 0);
        GLES20.glEnableVertexAttribArray(this.getPositionHandle());
    }

}
