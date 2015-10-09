/*
 *  Cube.java
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

package org.artoolkit.ar.base.rendering;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;

import android.opengl.GLES10;

/**
 * Simple class to render a coloured cube.
 */
public class Cube {
    
	private FloatBuffer	mVertexBuffer;
    private FloatBuffer	mColorBuffer;
    private ByteBuffer	mIndexBuffer;
    
    public Cube() {
    	this(1.0f);
    }
    
	public Cube(float size) {
		this(size, 0.0f, 0.0f, 0.0f);
	}
	
	public Cube(float size, float x, float y, float z) {
		setArrays(size, x, y, z);
	}
	
	private void setArrays(float size, float x, float y, float z) {

		float hs = size / 2.0f;

		float vertices[] = { 
			x - hs, y - hs, z - hs, // 0
			x + hs, y - hs, z - hs, // 1
			x + hs, y + hs, z - hs, // 2
			x - hs, y + hs, z - hs, // 3
			x - hs, y - hs, z + hs, // 4
			x + hs, y - hs, z + hs, // 5
			x + hs, y + hs, z + hs, // 6
			x - hs, y + hs, z + hs, // 7
		};

		float c = 1.0f;
		float colors[] = { 
			0, 0, 0, c, // 0 black
			c, 0, 0, c, // 1 red
			c, c, 0, c, // 2 yellow
			0, c, 0, c, // 3 green
			0, 0, c, c, // 4 blue
			c, 0, c, c, // 5 magenta
			c, c, c, c, // 6 white
			0, c, c, c, // 7 cyan
		};

		byte indices[] = { 
			0, 4, 5, 	0, 5, 1, 
			1, 5, 6, 	1, 6, 2, 
			2, 6, 7, 	2, 7, 3, 
			3, 7, 4, 	3, 4, 0, 
			4, 7, 6, 	4, 6, 5, 
			3, 0, 1, 	3, 1, 2 
		};


	mVertexBuffer = RenderUtils.buildFloatBuffer(vertices);
	mColorBuffer = RenderUtils.buildFloatBuffer(colors);	
	mIndexBuffer = RenderUtils.buildByteBuffer(indices);
	
    }
    
    public void draw(GL10 unused) {
    	
        
    	GLES10.glColorPointer(4, GLES10.GL_FLOAT, 0, mColorBuffer);
    	GLES10.glVertexPointer(3, GLES10.GL_FLOAT, 0, mVertexBuffer);
    	
    	GLES10.glEnableClientState(GLES10.GL_COLOR_ARRAY); 
    	GLES10.glEnableClientState(GLES10.GL_VERTEX_ARRAY);
    	
    	GLES10.glDrawElements(GLES10.GL_TRIANGLES, 36, GLES10.GL_UNSIGNED_BYTE, mIndexBuffer);
        
    	GLES10.glDisableClientState(GLES10.GL_COLOR_ARRAY); 
    	GLES10.glDisableClientState(GLES10.GL_VERTEX_ARRAY);
        
    }
    

}