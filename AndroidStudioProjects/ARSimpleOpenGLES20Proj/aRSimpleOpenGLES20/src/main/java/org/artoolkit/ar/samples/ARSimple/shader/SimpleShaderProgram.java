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
 */
public class SimpleShaderProgram extends ShaderProgram {

    public SimpleShaderProgram(OpenGLShader vertexShader, OpenGLShader fragmentShader){
        super(vertexShader,fragmentShader);
        bindAttributes();
    }

    @Override
    public int getProjectionMatrixHandle() {
        return GLES20.glGetUniformLocation(shaderProgramHandle, OpenGLShader.projectionMatrixString);
    }

    @Override
    public int getModelViewMatrixHandle() {
        return GLES20.glGetUniformLocation(shaderProgramHandle,OpenGLShader.modelViewMatrixString);
    }

    @Override
    protected void bindAttributes(){
        // Bind attributes
        GLES20.glBindAttribLocation(shaderProgramHandle, 0, OpenGLShader.positionVectorString);
        GLES20.glBindAttribLocation(shaderProgramHandle, 1, SimpleVertexShader.colorVectorString);
    }

    public int getPositionHandle() {
        return GLES20.glGetAttribLocation(shaderProgramHandle, OpenGLShader.positionVectorString);
    }

    public int getColorHandle() {
        return GLES20.glGetAttribLocation(shaderProgramHandle, SimpleVertexShader.colorVectorString);
    }

    @Override
    public void render(FloatBuffer vertexBuffer, FloatBuffer colorBuffer, ByteBuffer indexBuffer) {
        setupShaderUsage();

        // Pass in the position information
        vertexBuffer.position(0);
        GLES20.glVertexAttribPointer(this.getPositionHandle(), positionDataSize, GLES20.GL_FLOAT, false,
                positionStrideBytes, vertexBuffer);

        GLES20.glEnableVertexAttribArray(this.getPositionHandle());

        // Pass in the color information
        colorBuffer.position(0);
        GLES20.glVertexAttribPointer(this.getColorHandle(), colorDataSize, GLES20.GL_FLOAT, false,
                colorStrideBytes, colorBuffer);

        GLES20.glEnableVertexAttribArray(this.getColorHandle());

        GLES20.glDrawElements(GLES20.GL_TRIANGLES, 36, GLES20.GL_UNSIGNED_BYTE, indexBuffer);

    }
}
