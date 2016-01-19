package org.artoolkit.ar.base.rendering;

import android.opengl.GLES10;

import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;

/**
 * Created by Thorsten Bux on 15.01.2016.
 */
public class Line {

    private final float[] start;
    private final float[] end;
    private float width;

    private FloatBuffer mVertexBuffer;

    /**
     *
     * @param start Vector were the line starts
     * @param end Vector were the line ends
     * @param width Width of the vector
     */
    public Line(float[] start, float[] end, float width){
        this.start = start;
        this.end = end;
        this.width = width;
        setArrays();
    }

    private void setArrays(){
        float[] vertices = new float[start.length+end.length];

        for(int i = 0; i < start.length; i++){
            vertices[i] = start[0];
            vertices[i+start.length] = end[i];
        }

        mVertexBuffer = RenderUtils.buildFloatBuffer(vertices);
    }

    public void draw(GL10 gl) {
        gl.glVertexPointer(start.length, GLES10.GL_FLOAT, 0, mVertexBuffer);

        gl.glEnableClientState(GLES10.GL_VERTEX_ARRAY);
        gl.glColor4f(1, 0, 0, 1); // Red
        gl.glLineWidth(this.width);
        gl.glDrawArrays(GLES10.GL_LINES, 0, 2);
        gl.glDisableClientState(GLES10.GL_VERTEX_ARRAY);
    }

    public float getWidth(){
        return width;
    }

    public void setWidth(float width){
        this.width = width;
    }
}
