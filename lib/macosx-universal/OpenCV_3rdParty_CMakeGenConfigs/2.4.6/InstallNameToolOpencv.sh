#!/bin/bash
#install_name_tool -id @loader_path/PRIMARY PRIMARY
#install_name_tool -change lib/SECONDARY @loader_path/SECONDARY

install_name_tool -id @loader_path/libopencv_core.2.4.6.dylib libopencv_core.2.4.6.dylib

install_name_tool -id @loader_path/libopencv_features2d.2.4.6.dylib libopencv_features2d.2.4.6.dylib
install_name_tool -change lib/libopencv_core.2.4.dylib @loader_path/libopencv_core.2.4.6.dylib libopencv_features2d.2.4.dylib
install_name_tool -change lib/libopencv_imgproc.2.4.dylib @loader_path/libopencv_imgproc.2.4.6.dylib libopencv_features2d.2.4.dylib
install_name_tool -change lib/libopencv_calib3d.2.4.dylib @loader_path/libopencv_calib3d.2.4.6.dylib libopencv_features2d.2.4.dylib
install_name_tool -change lib/libopencv_highgui.2.4.dylib @loader_path/libopencv_highgui.2.4.6.dylib libopencv_features2d.2.4.dylib
install_name_tool -change lib/libopencv_flann.2.4.dylib @loader_path/libopencv_flann.2.4.6.dylib libopencv_features2d.2.4.6.dylib

install_name_tool -id @loader_path/libopencv_flann.2.4.6.dylib libopencv_flann.2.4.6.dylib
install_name_tool -change lib/libopencv_core.2.4.dylib @loader_path/libopencv_core.2.4.6.dylib libopencv_flann.2.4.dylib

install_name_tool -id @loader_path/libopencv_imgproc.2.4.6.dylib libopencv_imgproc.2.4.6.dylib
install_name_tool -change lib/libopencv_core.2.4.dylib @loader_path/libopencv_core.2.4.6.dylib libopencv_imgproc.2.4.dylib

install_name_tool -id @loader_path/libopencv_calib3d.2.4.6.dylib libopencv_calib3d.2.4.6.dylib
install_name_tool -change lib/libopencv_core.2.4.dylib @loader_path/libopencv_core.2.4.6.dylib libopencv_calib3d.2.4.dylib
install_name_tool -change lib/libopencv_imgproc.2.4.dylib @loader_path/libopencv_imgproc.2.4.6.dylib libopencv_calib3d.2.4.dylib
install_name_tool -change lib/libopencv_features2d.2.4.dylib @loader_path/libopencv_features2d.2.4.6.dylib libopencv_calib3d.2.4.dylib
install_name_tool -change lib/libopencv_highgui.2.4.dylib @loader_path/libopencv_highgui.2.4.6.dylib libopencv_calib3d.2.4.dylib
install_name_tool -change lib/libopencv_flann.2.4.dylib @loader_path/libopencv_flann.2.4.6.dylib libopencv_calib3d.2.4.dylib

install_name_tool -id @loader_path/libopencv_highgui.2.4.6.dylib libopencv_highgui.2.4.6.dylib
install_name_tool -change lib/libopencv_core.2.4.dylib @loader_path/libopencv_core.2.4.6.dylib libopencv_highgui.2.4.dylib
install_name_tool -change lib/libopencv_imgproc.2.4.dylib @loader_path/libopencv_imgproc.2.4.6.dylib libopencv_highgui.2.4.dylib
