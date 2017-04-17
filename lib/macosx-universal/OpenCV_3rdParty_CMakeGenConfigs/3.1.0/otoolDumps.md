#### otool -L libopencv_calib3d.3.1.0.dylib    
libopencv_calib3d.3.1.0.dylib:  
before  
> libopencv_calib3d.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)    
libopencv_features2d.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)    
libopencv_flann.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_ml.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_highgui.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_videoio.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_imgcodecs.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)  

after  
> @loader_path/libopencv_calib3d.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_features2d.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_flann.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_ml.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_highgui.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_videoio.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_imgcodecs.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_imgproc.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)  

#### otool -L libopencv_core.3.1.0.dylib
libopencv_core.3.1.0.dylib:  
before
> libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

after  
> @loader_path/libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

#### otool -L libopencv_features2d.3.1.0.dylib  
libopencv_features2d.3.1.0.dylib:  
before
> libopencv_features2d.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_flann.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_ml.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_highgui.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_videoio.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_imgcodecs.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

after  
> @loader_path/libopencv_features2d.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	@loader_path/libopencv_flann.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	@loader_path/libopencv_ml.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	@loader_path/libopencv_highgui.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	@loader_path/libopencv_videoio.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	@loader_path/libopencv_imgcodecs.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	@loader_path/libopencv_imgproc.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	@loader_path/libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)
	
#### otool -L libopencv_flann.3.1.0.dylib  
libopencv_flann.3.1.0.dylib:  
before  
> libopencv_flann.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

after  
> @loader_path/libopencv_flann.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	@loader_path/libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
	/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

#### otool -L libopencv_ml.3.1.0.dylib
libopencv_ml.3.1.0.dylib:  
before  
> libopencv_ml.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

after  
> @loader_path/libopencv_ml.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

#### otool -L libopencv_highgui.3.1.0.dylib  
libopencv_highgui.3.1.0.dylib: 
before   
>libopencv_highgui.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_videoio.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa (compatibility version 1.0.0, current version 22.0.0)  
libopencv_imgcodecs.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation (compatibility version 300.0.0, current version 1258.0.0)  
/usr/lib/libobjc.A.dylib (compatibility version 1.0.0, current version 228.0.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)  
/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit (compatibility version 45.0.0, current version 1404.46.0)  
/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1258.1.0) 

after  
> @loader_path/libopencv_highgui.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_videoio.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
/System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa (compatibility version 1.0.0, current version 22.0.0)  
@loader_path/libopencv_imgcodecs.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_imgproc.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation (compatibility version 300.0.0, current version 1258.0.0)  
/usr/lib/libobjc.A.dylib (compatibility version 1.0.0, current version 228.0.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)  
/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit (compatibility version 45.0.0, current version 1404.46.0)  
/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1258.1.0)

#### otool -L libopencv_videoio.3.1.0.dylib 
libopencv_videoio.3.1.0.dylib:  
before  
>libopencv_videoio.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_imgcodecs.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/System/Library/Frameworks/QTKit.framework/Versions/A/QTKit (compatibility version 1.0.0, current version 1.0.0)  
/System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore (compatibility version 1.2.0, current version 1.11.0)  
/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit (compatibility version 45.0.0, current version 1404.46.0)  
libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation (compatibility version 300.0.0, current version 1258.0.0)  
/usr/lib/libobjc.A.dylib (compatibility version 1.0.0, current version 228.0.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)  
/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1258.1.0)  
/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo (compatibility version 1.2.0, current version 1.5.0)

after  
> @loader_path/libopencv_videoio.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_imgcodecs.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
/System/Library/Frameworks/QTKit.framework/Versions/A/QTKit (compatibility version 1.0.0, current version 1.0.0)  
/System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore (compatibility version 1.2.0, current version 1.11.0)  
/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit (compatibility version 45.0.0, current version 1404.46.0)  
@loader_path/libopencv_imgproc.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation (compatibility version 300.0.0, current version 1258.0.0)  
/usr/lib/libobjc.A.dylib (compatibility version 1.0.0, current version 228.0.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)  
/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1258.1.0)  
/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo (compatibility version 1.2.0, current version 1.5.0)

#### otool -L libopencv_imgcodecs.3.1.dylib
libopencv_imgcodecs.3.1.dylib:  
before  
>libopencv_imgcodecs.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

after  
> @loader_path/libopencv_imgcodecs.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_imgproc.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

#### otool -L libopencv_imgproc.3.1.0.dylib  
libopencv_imgproc.3.1.0.dylib:  
before  
> libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)

after  
> @loader_path/libopencv_imgproc.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
@loader_path/libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0)  
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 120.1.0)  
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1226.10.1)
