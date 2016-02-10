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
package org.artoolkit.ar.samples.ardistanceopengles20.shader;

import android.opengl.GLES20;
import android.util.Log;

import org.artoolkit.ar.base.rendering.gles20.BaseShaderProgram;
import org.artoolkit.ar.base.rendering.gles20.OpenGLShader;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;

/**
 * Created by Thorsten Bux on 25.01.2016.
 */
public class MarkerDistanceShaderProgram extends BaseShaderProgram{

    private static final String TAG = "MarkerDistanceShader";

    public void setLineWidth(int lineWidth) {
        this.lineWidth = lineWidth;
    }

    private int lineWidth;

    public MarkerDistanceShaderProgram(OpenGLShader vertexShader, OpenGLShader fragmentShader, int lineWidth) {
        super(vertexShader, fragmentShader);
        this.lineWidth = lineWidth;
    }

    @Override
    public int getProjectionMatrixHandle() {
        return super.getProjectionMatrixHandle();
    }

    @Override
    public int getModelViewMatrixHandle() {
        return super.getModelViewMatrixHandle();
    }

    @Override
    protected void bindAttributes() {
        super.bindAttributes();
    }

    public int getColorHandle() {
        return GLES20.glGetUniformLocation(shaderProgramHandle, MarkerDistanceFragmentShader.colorVectorString);
    }

    @Override
    public void render(FloatBuffer vertexBuffer, FloatBuffer colorBuffer, ByteBuffer indexBuffer) {
        setupShaderUsage();

        //camPosition.length * 4 bytes per float
        GLES20.glVertexAttribPointer(this.getPositionHandle(), positionDataSize, GLES20.GL_FLOAT, false,
                positionStrideBytes, vertexBuffer);
        GLES20.glEnableVertexAttribArray(this.getPositionHandle());


        // Pass in the color information
        //pass in (colorHandle, 1 as count of color vertexes (my line has only one color), colorBuffer)
        GLES20.glUniform4fv(this.getColorHandle(), 1, colorBuffer);
        GLES20.glLineWidth(this.lineWidth);

        GLES20.glDrawArrays(GLES20.GL_LINES, 0, 2);

        Log.e(TAG, "Shader Info: " + GLES20.glGetShaderInfoLog(getShaderProgramHandle()));
    }
}
