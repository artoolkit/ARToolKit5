package org.artoolkit.ar.samples.ardistanceopengles20.shader;

import org.artoolkit.ar.base.rendering.gles20.BaseFragmentShader;

/**
 * Created by Thorsten Bux on 26.01.2016.
 */
public class MarkerDistanceFragmentShader extends BaseFragmentShader{
    public static String colorVectorString = "a_Color";

    private String fragmentShader =
            "precision lowp float;" +
            "uniform vec4 " + colorVectorString +"; \n"     // This is the color from the vertex shader interpolated across the
            // triangle per fragment.
            + "void main()                    \n"     // The entry point for our fragment shader.
            + "{                              \n"
            + "   gl_FragColor = " + colorVectorString +"; \n"     // Pass the color directly through the pipeline.
            + "}                              \n";

    public MarkerDistanceFragmentShader() {
        super();
        setShaderSource(fragmentShader);
    }

    @Override
    public int configureShader() {
        return super.configureShader();
    }
}
