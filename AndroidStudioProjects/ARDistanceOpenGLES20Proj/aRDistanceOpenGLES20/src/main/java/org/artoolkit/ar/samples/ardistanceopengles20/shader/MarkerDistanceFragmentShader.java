/*
 *  MarkerDistanceFragmentShader.java
 *  ARToolKit5
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not use, install, modify or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive
 *  license, under Daqri's copyrights in this original Daqri software (the
 *  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
 *  Software, with or without modifications, in source and/or binary forms;
 *  provided that if you redistribute the Daqri Software in its entirety and
 *  without modifications, you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

package org.artoolkit.ar.samples.ardistanceopengles20.shader;

import org.artoolkit.ar.base.rendering.gles20.BaseFragmentShader;

/**
 * Created by Thorsten Bux on 26.01.2016.
 * <p/>
 * FragmentShader class that extends the {@link BaseFragmentShader} class.
 * Here you define your fragment shader and what it does with the given color.
 * In that case it just applies it to the geometry and prints it on the screen.
 */
public class MarkerDistanceFragmentShader extends BaseFragmentShader {
    public static String colorVectorString = "a_Color";

    /**
     * We get the color to apply to the rendered geometry from the vertex shader.
     * We don't do anything with it, just simply pass it to the rendering pipe.
     * Therefor OpenGL 2.0 uses the gl_FragColor variable
     */
    private String fragmentShader =
            "precision lowp float;" +
                    "uniform vec4 " + colorVectorString + "; \n"     // This is the color from the vertex shader interpolated across the
                    // triangle per fragment.
                    + "void main()                    \n"     // The entry point for our fragment shader.
                    + "{                              \n"
                    + "   gl_FragColor = " + colorVectorString + "; \n"     // Pass the color directly through the pipeline.
                    + "}                              \n";

    public MarkerDistanceFragmentShader() {
        super();
        setShaderSource(fragmentShader);
    }

    /**
     * This method gets called by the {@link org.artoolkit.ar.base.rendering.gles20.BaseShaderProgram}
     * during initializing the shaders.
     * You can use it to pass in your own shader program as shown here. If you do not pass your own
     * shader program the one from {@link BaseFragmentShader} is used.
     *
     * @return The handle of the fragment shader
     */
    @Override
    public int configureShader() {
        return super.configureShader();
    }
}
