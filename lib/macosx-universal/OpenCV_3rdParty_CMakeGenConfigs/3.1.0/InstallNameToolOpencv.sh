#!/bin/bash
#To change the install name: install_name_tool -id @loader_path/PRIMARY PRIMARY
#To change the names of dependencies: install_name_tool -change lib/SECONDARY @loader_path/SECONDARY
#Execute this script in directory which contain the target shared libraries.

#libopencv_calib3d.3.1.dylib (compatibility version 3.1.0, current version 3.1.0):
#libopencv_features2d.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_flann.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_ml.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_highgui.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_videoio.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_imgcodecs.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
install_name_tool -id     @loader_path/libopencv_calib3d.3.1.0.dylib libopencv_calib3d.3.1.0.dylib
install_name_tool -change libopencv_features2d.3.1.dylib @loader_path/libopencv_features2d.3.1.0.dylib libopencv_calib3d.3.1.dylib
install_name_tool -change libopencv_flann.3.1.dylib      @loader_path/libopencv_flann.3.1.0.dylib      libopencv_calib3d.3.1.dylib
install_name_tool -change libopencv_ml.3.1.dylib         @loader_path/libopencv_ml.3.1.0.dylib         libopencv_calib3d.3.1.dylib
install_name_tool -change libopencv_highgui.3.1.dylib    @loader_path/libopencv_highgui.3.1.0.dylib    libopencv_calib3d.3.1.dylib
install_name_tool -change libopencv_videoio.3.1.dylib    @loader_path/libopencv_videoio.3.1.0.dylib    libopencv_calib3d.3.1.dylib
install_name_tool -change libopencv_imgcodecs.3.1.dylib  @loader_path/libopencv_imgcodecs.3.1.0.dylib  libopencv_calib3d.3.1.dylib
install_name_tool -change libopencv_imgproc.3.1.dylib    @loader_path/libopencv_imgproc.3.1.0.dylib    libopencv_calib3d.3.1.dylib
install_name_tool -change libopencv_core.3.1.dylib       @loader_path/libopencv_core.3.1.0.dylib       libopencv_calib3d.3.1.dylib

#libopencv_features2d.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0):
#libopencv_flann.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_ml.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_highgui.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_videoio.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_imgcodecs.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
install_name_tool -id     @loader_path/libopencv_features2d.3.1.0.dylib libopencv_features2d.3.1.0.dylib
install_name_tool -change libopencv_flann.3.1.dylib     @loader_path/libopencv_flann.3.1.0.dylib    libopencv_features2d.3.1.0.dylib
install_name_tool -change libopencv_ml.3.1.dylib        @loader_path/libopencv_ml.3.1.0.dylib       libopencv_features2d.3.1.0.dylib
install_name_tool -change libopencv_highgui.3.1.dylib   @loader_path/libopencv_highgui.3.1.0.dylib  libopencv_features2d.3.1.dylib
install_name_tool -change libopencv_videoio.3.1.dylib   @loader_path/libopencv_videoio.3.1.0.dylib  libopencv_features2d.3.1.dylib
install_name_tool -change libopencv_imgcodecs.3.1.dylib @loader_path/libopencv_imgcodecs.3.1.0.dylib libopencv_features2d.3.1.dylib
install_name_tool -change libopencv_imgproc.3.1.dylib   @loader_path/libopencv_imgproc.3.1.0.dylib  libopencv_features2d.3.1.dylib
install_name_tool -change libopencv_core.3.1.dylib      @loader_path/libopencv_core.3.1.0.dylib     libopencv_features2d.3.1.dylib

#libopencv_flann.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0):
#libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
install_name_tool -id     @loader_path/libopencv_flann.3.1.0.dylib libopencv_flann.3.1.0.dylib
install_name_tool -change libopencv_core.3.1.dylib      @loader_path/libopencv_core.3.1.0.dylib libopencv_flann.3.1.dylib

#libopencv_ml.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0):
#libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
install_name_tool -id     @loader_path/libopencv_ml.3.1.0.dylib libopencv_ml.3.1.0.dylib
install_name_tool -change libopencv_core.3.1.dylib      @loader_path/libopencv_core.3.1.0.dylib libopencv_ml.3.1.dylib

#libopencv_highgui.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0):
#libopencv_videoio.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_imgcodecs.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
install_name_tool -id     @loader_path/libopencv_highgui.3.1.0.dylib libopencv_highgui.3.1.0.dylib
install_name_tool -change libopencv_videoio.3.1.dylib   @loader_path/libopencv_videoio.3.1.0.dylib   libopencv_highgui.3.1.dylib
install_name_tool -change libopencv_imgcodecs.3.1.dylib @loader_path/libopencv_imgcodecs.3.1.0.dylib libopencv_highgui.3.1.dylib
install_name_tool -change libopencv_imgproc.3.1.dylib   @loader_path/libopencv_imgproc.3.1.0.dylib   libopencv_highgui.3.1.dylib
install_name_tool -change libopencv_core.3.1.dylib      @loader_path/libopencv_core.3.1.0.dylib      libopencv_highgui.3.1.dylib

#libopencv_videoio.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0):
#libopencv_imgcodecs.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
install_name_tool -id     @loader_path/libopencv_videoio.3.1.0.dylib libopencv_videoio.3.1.0.dylib
install_name_tool -change libopencv_imgcodecs.3.1.dylib @loader_path/libopencv_imgcodecs.3.1.0.dylib libopencv_videoio.3.1.dylib
install_name_tool -change libopencv_imgproc.3.1.dylib   @loader_path/libopencv_imgproc.3.1.0.dylib   libopencv_videoio.3.1.dylib
install_name_tool -change libopencv_core.3.1.dylib      @loader_path/libopencv_core.3.1.0.dylib      libopencv_videoio.3.1.dylib

#libopencv_imgcodecs.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0):
#libopencv_imgproc.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
#libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
install_name_tool -id     @loader_path/libopencv_imgcodecs.3.1.0.dylib libopencv_imgcodecs.3.1.0.dylib
install_name_tool -change libopencv_imgproc.3.1.dylib @loader_path/libopencv_imgproc.3.1.0.dylib libopencv_imgcodecs.3.1.dylib
install_name_tool -change libopencv_core.3.1.dylib    @loader_path/libopencv_core.3.1.0.dylib    libopencv_imgcodecs.3.1.dylib

#libopencv_imgproc.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0):
#libopencv_core.3.1.dylib (compatibility version 3.1.0, current version 3.1.0)
install_name_tool -id     @loader_path/libopencv_imgproc.3.1.0.dylib libopencv_imgproc.3.1.0.dylib
install_name_tool -change libopencv_core.3.1.dylib @loader_path/libopencv_core.3.1.0.dylib libopencv_imgproc.3.1.dylib

#libopencv_core.3.1.0.dylib (compatibility version 3.1.0, current version 3.1.0):
install_name_tool -id     @loader_path/libopencv_core.3.1.0.dylib libopencv_core.3.1.0.dylib
