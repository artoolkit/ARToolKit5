[Setup]
AppName="ARToolKit"
AppVerName="ARToolKit v5.3.2r1"
AppVersion=5.3.2
AppPublisher="Daqri LLC"
AppPublisherURL=http://www.artoolkit.org/
AppSupportURL=http://www.artoolkit.org/documentation
AppUpdatesURL=http://www.artoolkit.org/download
DefaultDirName={pf}\ARToolKit5
DefaultGroupName=ARToolKit
InfoBeforeFile=README.md
Compression=lzma
SolidCompression=yes
SourceDir=..\..
OutputBaseFilename="ARToolKit v5.3.2r1 Setup (bin-win32-vs120)"
OutputDir=..
ChangesEnvironment=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: full; Description: "Install full ARToolKit, including libraries, utilities, and examples, and sourcecode, plus documentation."
Name: minimal; Description: "Install minimal ARToolKit for application development."
Name: custom; Description: "Choose which parts of ARToolKit to install."; Flags: iscustom

[Components]
Name: dev; Description: "Libraries and header files needed to develop applications that use ARToolKit"; Types: full minimal custom; Flags: fixed
Name: utils; Description: "Essential ARToolKit utilities"; Types: full minimal custom; Flags: fixed
Name: examples; Description: "Basic examples of ARToolKit"; Types: full custom
Name: src; Description: "Add sourcecode for each of the items selected above"; Types: full custom
Name: docs; Description: "Documentation for ARToolKit"; Types: full custom

[InstallDelete]
; Here we remove obsolete files from earlier versions.
; Removed from 5.4, or moved.
Type: filesandordirs; Name: "{app}\DSVL\"
Type: files; Name: "{app}\bin\DSVL.dll"
Type: files; Name: "{app}\bin\DSVLd.dll"
Type: files; Name: "{app}\bin64\DSVL.dll"
Type: files; Name: "{app}\bin64\DSVLd.dll"
Type: files; Name: "{app}\include\win32-i386\qedit.h"
Type: files; Name: "{app}\include\win64-x64\qedit.h"
Type: filesandordirs; Name: "{app}\include\AR\sys\"
Type: filesandordirs; Name: "{app}\lib\SRC\VideoWinDummy\"
Type: filesandordirs; Name: "{app}\lib\SRC\VideoWinImage\"
Type: filesandordirs; Name: "{app}\lib\SRC\VideoWinDF\"
Type: filesandordirs; Name: "{app}\lib\SRC\VideoWinDS\"
Type: filesandordirs; Name: "{app}\lib\SRC\VideoWinDSVL\"
Type: filesandordirs; Name: "{app}\lib\SRC\VideoWinMF\"
Type: filesandordirs; Name: "{app}\lib\SRC\VideoQuickTime\"
; Removed from 5.3.3
Type: files; Name: "{app}\VisualStudio\vs120\ARvrml.vcxproj"
Type: files; Name: "{app}\VisualStudio\vs120\simpleVRML.vcxproj"
Type: filesandordirs; Name: "{app}\examples\simpleVRML\"
Type: files; Name: "{app}\include\AR\arvrml.h"
Type: filesandordirs; Name: "{app}\lib\SRC\ARvrml\"
Type: files; Name: "{app}\share\artoolkit-examples\Data\object_data_vrml"
Type: filesandordirs; Name: "{app}\share\artoolkit-examples\Wrl\"
; Removed from 5.3.2
Type: files; Name: "{app}\bin\Data\patt.hiro"
Type: files; Name: "{app}\bin\Data\patt.kanji"
Type: files; Name: "{app}\bin\Data\patt.sample1"
Type: files; Name: "{app}\bin\Data\patt.sample2"
Type: files; Name: "{app}\bin\Data\patt.calib"
Type: files; Name: "{app}\bin\Data\multi\patt.a"
Type: files; Name: "{app}\bin\Data\multi\patt.b"
Type: files; Name: "{app}\bin\Data\multi\patt.c"
Type: files; Name: "{app}\bin\Data\multi\patt.d"
Type: files; Name: "{app}\bin\Data\multi\patt.f"
Type: files; Name: "{app}\bin\Data\multi\patt.g"
; Removed from 5.3
Type: files; Name: "{app}\include\KPM\surfSub.h"
Type: files; Name: "{app}\lib\SRC\KPM\AnnMatch.cpp"
Type: files; Name: "{app}\lib\SRC\KPM\AnnMatch.h"
Type: files; Name: "{app}\lib\SRC\KPM\AnnMatch2.cpp"
Type: files; Name: "{app}\lib\SRC\KPM\AnnMatch2.h"
Type: files; Name: "{app}\lib\SRC\KPM\HomographyEst.cpp"
Type: files; Name: "{app}\lib\SRC\KPM\surfSub1.cpp"
Type: files; Name: "{app}\lib\SRC\KPM\surfSub2.cpp"
Type: files; Name: "{app}\lib\SRC\KPM\surfSubPrivate.h"
Type: filesandordirs; Name: "{app}\lib\SRC\KPM\surfOld"
Type: files; Name: "{app}\bin\DataNFT\pinball.fset2"
; Removed from 5.1.2
Type: files; Name: "{app}\include\ARWrapper\ARToolKit.h"
; Removed from 5.1.0
Type: files; Name: "{app}\doc\Calibration chessboard.pdf"
; Removed from 5.0.5
Type: files; Name: "{app}\bin\msvcr71.dll"
Type: files; Name: "{app}\bin\msvcr71d.dll"
Type: files; Name: "{app}\bin\msvcp71.dll"
Type: files; Name: "{app}\bin\msvcp71d.dll"
; Removed from 5.0.2
Type: files; Name: "{app}\bin\opencv_core220.dll"
Type: files; Name: "{app}\bin64\opencv_core220.dll"
Type: files; Name: "{app}\bin\opencv_flann220.dll"
Type: files; Name: "{app}\bin64\opencv_flann220.dll"
Type: files; Name: "{app}\lib\win32-i386\opencv_core220.lib"
Type: files; Name: "{app}\lib\win32-i386\opencv_flann220.lib"
Type: files; Name: "{app}\lib\win64-x64\opencv_core220.lib"
Type: files; Name: "{app}\lib\win64-x64\opencv_flann220.lib"
Type: files; Name: "{app}\bin\opencv_calib3d220.dll"
Type: files; Name: "{app}\bin\opencv_imgproc220.dll"
Type: files; Name: "{app}\bin\opencv_imgproc220.dll"
Type: files; Name: "{app}\bin64\opencv_calib3d220.dll"
Type: files; Name: "{app}\bin64\opencv_imgproc220.dll"
Type: files; Name: "{app}\lib\win32-i386\opencv_calib3d220.lib"
Type: files; Name: "{app}\lib\win32-i386\opencv_imgproc220.lib"
Type: filesandordirs; Name: "{app}\include\opencv"
Type: filesandordirs; Name: "{app}\include\opencv2"

[Files]
; dev = required to build apps against SDK.
Source: "README.md"; Components: dev; DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "LICENSE.txt"; Components: dev; DestDir: "{app}"; Flags: ignoreversion
Source: "ChangeLog.txt"; Components: dev; DestDir: "{app}"; Flags: ignoreversion
Source: "include\AR\*"; Components: dev; DestDir: "{app}\include\AR"; Flags: recursesubdirs ignoreversion
Source: "include\AR2\*"; Components: dev; DestDir: "{app}\include\AR2"; Flags: recursesubdirs ignoreversion
Source: "include\KPM\*"; Components: dev; DestDir: "{app}\include\KPM"; Flags: recursesubdirs ignoreversion
Source: "include\ARWrapper\*"; Components: dev; DestDir: "{app}\include\ARWrapper"; Flags: recursesubdirs ignoreversion
Source: "include\Eden\*"; Components: dev; DestDir: "{app}\include\Eden"; Flags: recursesubdirs ignoreversion
Source: "include\ARUtil\*"; Components: dev; DestDir: "{app}\include\ARUtil"; Flags: recursesubdirs ignoreversion
Source: "include\glStateCache.h"; Components: dev; DestDir: "{app}\include"; Flags: ignoreversion
Source: "include\glStateCache2.h"; Components: dev; DestDir: "{app}\include"; Flags: ignoreversion
Source: "include\win32-i386\stdint.h"; Components: dev; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win64-x64\stdint.h"; Components: dev; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "include\win32-i386\GL\glext.h"; Components: dev; DestDir: "{app}\include\win32-i386\GL"; Flags: ignoreversion
Source: "include\win64-x64\GL\glext.h"; Components: dev; DestDir: "{app}\include\win64-x64\GL"; Flags: ignoreversion
Source: "include\win32-i386\GL\wglext.h"; Components: dev; DestDir: "{app}\include\win32-i386\GL"; Flags: ignoreversion
Source: "include\win64-x64\GL\wglext.h"; Components: dev; DestDir: "{app}\include\win64-x64\GL"; Flags: ignoreversion
Source: "lib\win32-i386\AR*"; Components: dev; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\AR*"; Components: dev; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
Source: "lib\win32-i386\KPM*"; Components: dev; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\KPM*"; Components: dev; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
Source: "lib\win32-i386\Eden.lib"; Components: dev; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\Eden.lib"; Components: dev; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
Source: "bin\ARvideo*.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin64\ARvideo*.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin\ARWrapper*.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin64\ARWrapper*.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin\pthreadVC2.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin64\pthreadVC2.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "include\win32-i386\GL\glut.h"; Components: dev; DestDir: "{app}\include\win32-i386\GL"; Flags: ignoreversion
Source: "include\win64-x64\GL\glut.h"; Components: dev; DestDir: "{app}\include\win64-x64\GL"; Flags: ignoreversion
Source: "lib\win32-i386\glut32.lib"; Components: dev; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\glut64.lib"; Components: dev; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
Source: "bin\glut32.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin64\glut64.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin\ARosg*.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin64\ARosg*.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion

; dev, runtimes
Source: "bin\vcredist_x86.exe"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin64\vcredist_x64.exe"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion

; dev, external (OpenSceneGraph-3.2.1)
;Source: "bin\gdal18.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
;Source: "bin\libexpat.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\libpng16.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\osg100-*.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\ot20-OpenThreads.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\zlib1.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\libxml2.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\libcurl.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\libeay32.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\ssleay32.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\libssh2.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\libxml2.dll"; Components: dev; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\osgPlugins-3.2.1\*"; Components: dev; DestDir: "{app}\bin\osgPlugins-3.2.1"; Flags: recursesubdirs ignoreversion
;Source: "bin64\gdal18.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
;Source: "bin64\libexpat.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\libpng16.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\osg100-*.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\ot20-OpenThreads.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\zlib1.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\libxml2.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\libcurl.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\libeay32.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\ssleay32.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\libssh2.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\libxml2.dll"; Components: dev; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\osgPlugins-3.2.1\*"; Components: dev; DestDir: "{app}\bin64\osgPlugins-3.2.1"; Flags: recursesubdirs ignoreversion

; dev and src = required to rebuild dev. 
Source: "lib\SRC\AR\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\AR"; Flags: recursesubdirs ignoreversion
Source: "lib\SRC\ARICP\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\ARICP"; Flags: recursesubdirs ignoreversion
Source: "lib\SRC\ARMulti\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\ARMulti"; Flags: recursesubdirs ignoreversion
Source: "lib\SRC\AR2\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\AR2"; Flags: recursesubdirs ignoreversion
Source: "lib\SRC\KPM\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\KPM"; Flags: recursesubdirs ignoreversion
Source: "lib\SRC\ARUtil\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\ARUtil"; Flags: recursesubdirs ignoreversion
Source: "lib\SRC\ARosg\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\ARosg"; Flags: recursesubdirs ignoreversion
Source: "lib\SRC\ARWrapper\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\ARWrapper"; Flags: recursesubdirs ignoreversion
Source: "lib\SRC\Eden\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\Eden"; Flags: recursesubdirs ignoreversion
Source: "lib\SRC\Gl\argBase.c"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Gl\argDraw.c"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Gl\argDrawImage.c"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Gl\argDrawMode.c"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Gl\argFunction.c"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Gl\argPrivate.h"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Gl\argWindow.c"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Gl\gsub_lite.c"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Gl\gsub_mtx.c"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Gl\gsubTest.c"; Components: dev and src; DestDir: "{app}\lib\SRC\Gl"; Flags: ignoreversion
Source: "lib\SRC\Video\*"; Excludes: "Makefile.in"; Components: dev and src; DestDir: "{app}\lib\SRC\Video"; Flags: recursesubdirs ignoreversion
Source: "Configure-win32.*"; Components: dev and src; DestDir: "{app}"; Flags: ignoreversion
Source: "VisualStudio\vs120\*"; Excludes: "Debug*\*,Release*\*,x64\*,ipch\*,*.ncb,*.suo,*.user,*.sdf"; Components: dev and src; DestDir: "{app}\VisualStudio\vs120"; Flags: recursesubdirs ignoreversion
Source: "share\packaging\ARToolKit5-bin-win32-vs120*"; Components: dev and src; DestDir: "{app}\share\packaging"; Flags: ignoreversion
;Source: "share\*"; Excludes: "artoolkit5-config.in,artoolkit5-setenv,artoolkit5-unsetenv"; Components: dev and src; DestDir: "{app}\share"; Flags: ignoreversion

; Libraries and headers, source: external dependencies.
; pthreads
Source: "include\win32-i386\pthread.h"; Components: dev and src; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win32-i386\sched.h"; Components: dev and src; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win32-i386\semaphore.h"; Components: dev and src; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win64-x64\pthread.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "include\win64-x64\sched.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "include\win64-x64\semaphore.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "lib\win32-i386\pthreadVC2.lib"; Components: dev and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\pthreadVC2.lib"; Components: dev and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
; libjpeg
Source: "include\win32-i386\jpeglib.h"; Components: dev and src; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win32-i386\jconfig.h"; Components: dev and src; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win32-i386\jmorecfg.h"; Components: dev and src; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win32-i386\jversion.h"; Components: dev and src; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win64-x64\jpeglib.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "include\win64-x64\jconfig.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "include\win64-x64\jmorecfg.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "include\win64-x64\jversion.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "lib\win32-i386\libjpeg.lib"; Components: dev and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\libjpeg.lib"; Components: dev and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
; zlib
Source: "include\win32-i386\zconf.h"; Components: dev and src; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win32-i386\zlib.h"; Components: dev and src; DestDir: "{app}\include\win32-i386"; Flags: ignoreversion
Source: "include\win64-x64\zconf.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "include\win64-x64\zlib.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "include\win64-x64\jversion.h"; Components: dev and src; DestDir: "{app}\include\win64-x64"; Flags: ignoreversion
Source: "lib\win32-i386\zlib.lib"; Components: dev and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\zlib.lib"; Components: dev and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
; OpenSceneGraph-3.2.1
Source: "include\win32-i386\OpenThreads\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\OpenThreads"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\OpenThreads\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\OpenThreads"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osg\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osg"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osg\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osg"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgAnimation\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgAnimation"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgAnimation\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgAnimation"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgDB\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgDB"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgDB\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgDB"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgFX\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgFX"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgFX\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgFX"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgGA\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgGA"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgGA\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgGA"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgManipulator\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgManipulator"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgManipulator\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgManipulator"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgParticle\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgParticle"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgParticle\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgParticle"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgPresentation\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgPresentation"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgPresentation\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgPresentation"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgShadow\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgShadow"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgShadow\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgShadow"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgSim\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgSim"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgSim\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgSim"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgTerrain\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgTerrain"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgTerrain\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgTerrain"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgText\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgText"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgText\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgText"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgUtil\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgUtil"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgUtil\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgUtil"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgViewer\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgViewer"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgViewer\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgViewer"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgVolume\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgVolume"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgVolume\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgVolume"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\osgWidget\*"; Components: dev and src; DestDir: "{app}\include\win32-i386\osgWidget"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\osgWidget\*"; Components: dev and src; DestDir: "{app}\include\win64-x64\osgWidget"; Flags: recursesubdirs ignoreversion
Source: "lib\win32-i386\OpenThreads.lib"; Components: dev and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\OpenThreads.lib"; Components: dev and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
Source: "lib\win32-i386\osg*.lib"; Components: dev and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\osg*.lib"; Components: dev and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion

; Utilities
Source: "bin\calib_*.exe"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\mk_patt.exe"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\check_id.exe"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\genTexData.exe"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\dispImageSet.exe"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\dispFeatureSet.exe"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\checkResolution.exe"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\genMarkerSet.exe"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\opencv_core2410.dll"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin64\opencv_core2410.dll"; Components: utils; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin\opencv_flann2410.dll"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin64\opencv_flann2410.dll"; Components: utils; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin\opencv_calib3d2410.dll"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\opencv_imgproc2410.dll"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\opencv_features2d2410.dll"; Components: utils; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin64\opencv_calib3d2410.dll"; Components: utils; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\opencv_imgproc2410.dll"; Components: utils; DestDir: "{app}\bin64"; Flags: ignoreversion
Source: "bin64\opencv_features2d2410.dll"; Components: utils; DestDir: "{app}\bin64"; Flags: ignoreversion

Source: "share\artoolkit-utils\Data\camera_para.dat"; Components: utils; DestDir: "{app}\share\artoolkit-utils\Data"; Flags: ignoreversion comparetimestamp
Source: "share\artoolkit-utils\Data\calibStereoMarkerConfig.dat"; Components: utils; DestDir: "{app}\share\artoolkit-utils\Data"; Flags: ignoreversion
Source: "share\artoolkit-utils\Data\hiro.patt"; Components: utils; DestDir: "{app}\share\artoolkit-utils\Data"; Flags: ignoreversion
Source: "share\artoolkit-utils\Data\calib.patt"; Components: utils; DestDir: "{app}\share\artoolkit-utils\Data"; Flags: ignoreversion

; Utilities, source
Source: "util\calib_camera\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\calib_camera"; Flags: recursesubdirs ignoreversion
Source: "util\calib_camera_old-v3\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\calib_camera_old-v3"; Flags: recursesubdirs ignoreversion
Source: "util\calib_optical\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\calib_optical"; Flags: recursesubdirs ignoreversion
Source: "util\calib_stereo\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\calib_stereo"; Flags: recursesubdirs ignoreversion
Source: "util\calib_stereo_old-v3\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\calib_stereo_old-v3"; Flags: recursesubdirs ignoreversion
Source: "util\checkResolution\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\checkResolution"; Flags: recursesubdirs ignoreversion
Source: "util\check_id\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\check_id"; Flags: recursesubdirs ignoreversion
Source: "util\dispFeatureSet\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\dispFeatureSet"; Flags: recursesubdirs ignoreversion
Source: "util\dispImageSet\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\dispImageSet"; Flags: recursesubdirs ignoreversion
Source: "util\genMarkerSet\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\genMarkerSet"; Flags: recursesubdirs ignoreversion
Source: "util\genTexData\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\genTexData"; Flags: recursesubdirs ignoreversion
Source: "util\mk_patt\*"; Excludes: "Makefile.in"; Components: utils and src; DestDir: "{app}\util\mk_patt"; Flags: recursesubdirs ignoreversion
; OpenCV
Source: "include\win32-i386\opencv\*"; Components: utils and src; DestDir: "{app}\include\win32-i386\opencv"; Flags: recursesubdirs ignoreversion
Source: "include\win32-i386\opencv2\*"; Components: utils and src; DestDir: "{app}\include\win32-i386\opencv2"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\opencv\*"; Components: utils and src; DestDir: "{app}\include\win64-x64\opencv"; Flags: recursesubdirs ignoreversion
Source: "include\win64-x64\opencv2\*"; Components: utils and src; DestDir: "{app}\include\win64-x64\opencv2"; Flags: recursesubdirs ignoreversion
Source: "lib\win32-i386\opencv_core2410.lib"; Components: utils and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win32-i386\opencv_flann2410.lib"; Components: utils and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win32-i386\opencv_calib3d2410.lib"; Components: utils and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win32-i386\opencv_imgproc2410.lib"; Components: utils and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win32-i386\opencv_features2d2410.lib"; Components: utils and src; DestDir: "{app}\lib\win32-i386"; Flags: ignoreversion
Source: "lib\win64-x64\opencv_core2410.lib"; Components: utils and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
Source: "lib\win64-x64\opencv_flann2410.lib"; Components: utils and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
Source: "lib\win64-x64\opencv_calib3d2410.lib"; Components: utils and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
Source: "lib\win64-x64\opencv_imgproc2410.lib"; Components: utils and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion
Source: "lib\win64-x64\opencv_features2d2410.lib"; Components: utils and src; DestDir: "{app}\lib\win64-x64"; Flags: ignoreversion

; Examples
Source: "bin\multi.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\multiCube.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\simpleLite.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\optical.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\opticalStereo.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\simpleMovie.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\simpleOSG.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\simple.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\stereo.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\nftSimple.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "bin\nftBook.exe"; Components: examples; DestDir: "{app}\bin"; Flags: ignoreversion

;Source: "share\artoolkit-examples\Data\cparaL.dat"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion comparetimestamp
;Source: "share\artoolkit-examples\Data\cparaR.dat"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion comparetimestamp
;Source: "share\artoolkit-examples\Data\transL2R.dat"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion comparetimestamp
;Source: "share\artoolkit-examples\Data\optical_param.dat"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion comparetimestamp
Source: "share\artoolkit-examples\Data\sample1.patt"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion
Source: "share\artoolkit-examples\Data\sample2.patt"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion
Source: "share\artoolkit-examples\Data\multi\*"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data\multi"; Flags: recursesubdirs ignoreversion
Source: "share\artoolkit-examples\Data\cubeMarkerConfig.dat"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion
Source: "share\artoolkit-examples\Data\objects.dat"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion
Source: "share\artoolkit-examples\Data\markers.dat"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion
Source: "share\artoolkit-examples\OSG\*"; Components: examples; DestDir: "{app}\share\artoolkit-examples\OSG"; Flags: recursesubdirs ignoreversion
Source: "share\artoolkit-examples\Data2\*"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data2"; Flags: recursesubdirs ignoreversion
Source: "share\artoolkit-examples\DataNFT\*"; Components: examples; DestDir: "{app}\share\artoolkit-examples\DataNFT"; Flags: recursesubdirs ignoreversion
Source: "share\artoolkit-examples\Data\sample.mov"; Components: examples; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion
Source: "share\artoolkit-examples\Data\hiro.patt"; Components: utils; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion
Source: "share\artoolkit-examples\Data\kanji.patt"; Components: utils; DestDir: "{app}\share\artoolkit-examples\Data"; Flags: ignoreversion

; Examples, source
Source: "examples\multi\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\multi"; Flags: recursesubdirs ignoreversion
Source: "examples\multiCube\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\multiCube"; Flags: recursesubdirs ignoreversion
Source: "examples\multiWin\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\multiWin"; Flags: recursesubdirs ignoreversion
Source: "examples\simple\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\simple"; Flags: recursesubdirs ignoreversion
Source: "examples\simpleLite\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\simpleLite"; Flags: recursesubdirs ignoreversion
Source: "examples\optical\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\optical"; Flags: recursesubdirs ignoreversion
Source: "examples\opticalStereo\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\opticalStereo"; Flags: recursesubdirs ignoreversion
Source: "examples\simpleMovie\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\simpleMovie"; Flags: recursesubdirs ignoreversion
Source: "examples\simpleOSG\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\simpleOSG"; Flags: recursesubdirs ignoreversion
Source: "examples\stereo\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\stereo"; Flags: recursesubdirs ignoreversion
Source: "examples\nftSimple\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\nftSimple"; Flags: recursesubdirs ignoreversion
Source: "examples\nftBook\*"; Excludes: "Makefile.in,*-Info.plist"; Components: examples and src; DestDir: "{app}\examples\nftBook"; Flags: recursesubdirs ignoreversion

; Documentation
Source: "doc\*"; Components: docs; DestDir: "{app}\doc"; Flags: recursesubdirs ignoreversion
Source: "share\doc\*"; Components: docs and src; DestDir: "{app}\share\doc"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\Open ARToolKit5 directory"; Filename: "{app}";
Name: "{group}\Open command console in ARToolKit5 binaries directory"; Filename: "{cmd}"; WorkingDir: "{app}\bin"
Name: "{group}\ARToolKit - Home (www)"; Filename: "http://www.artoolkit.org/"
Name: "{group}\ARToolKit - Documentation (www)"; Filename: "http://www.artoolkit.org/documentation"
Name: "{group}\ARToolKit - API Reference"; Filename: "{app}\doc\apiref\masterTOC.html"
Name: "{group}\{cm:UninstallProgram,ARToolKit}"; Filename: "{uninstallexe}"

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: string; ValueName: "ARTOOLKIT5_ROOT"; ValueData: {app}; Flags: uninsdeletevalue
