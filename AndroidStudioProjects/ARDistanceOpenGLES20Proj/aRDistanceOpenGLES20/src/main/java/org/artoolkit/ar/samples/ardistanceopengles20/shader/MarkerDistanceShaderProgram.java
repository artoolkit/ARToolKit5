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
