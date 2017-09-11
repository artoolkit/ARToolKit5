'
'  Configure-winrt.vbs
'  ARToolKit5
'
'  Windows configuration script
'
'  This file is part of ARToolKit.
'
'  ARToolKit is free software: you can redistribute it and/or modify
'  it under the terms of the GNU Lesser General Public License as published by
'  the Free Software Foundation, either version 3 of the License, or
'  (at your option) any later version.
'
'  ARToolKit is distributed in the hope that it will be useful,
'  but WITHOUT ANY WARRANTY; without even the implied warranty of
'  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
'  GNU Lesser General Public License for more details.
'
'  You should have received a copy of the GNU Lesser General Public License
'  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
'
'  As a special exception, the copyright holders of this library give you
'  permission to link this library with independent modules to produce an
'  executable, regardless of the license terms of these independent modules, and to
'  copy and distribute the resulting executable under terms of your choice,
'  provided that you also meet, for each linked independent module, the terms and
'  conditions of the license of that module. An independent module is a module
'  which is neither derived from nor based on this library. If you modify this
'  library, you may extend this exception to your version of the library, but you
'  are not obligated to do so. If you do not wish to do so, delete this exception
'  statement from your version.
'
'  Copyright 2015 Daqri, LLC.
'  Copyright 2007-2015 ARToolworks, Inc.
'
'  Author: Philip Lamb
'

'
' If config.h doesn't exist, or is older than config.h.in,
' copies config.h.in to config.h, setting defaults.
'

Const strConfigHFileName = "include\AR\config.h"
Const strConfigHInFileName = "include\AR\config.h.in"

' Change working directory to the same folder as this script.
Set objFSO = CreateObject("Scripting.FileSystemObject")
Set objShell = CreateObject("WScript.Shell")
objShell.CurrentDirectory = objFSO.GetParentFolderName(WScript.ScriptFullName)

Dim doUpdate
doUpdate = False
If Not objFSO.FileExists(strConfigHFileName) Then
    doUpdate = True
ElseIf objFSO.GetFile(strConfigHFileName).DateLastModified < objFSO.GetFile(strConfigHInFileName).DateLastModified Then
    doUpdate = True
    ' Delete the old config.h in prep for making a new one.
    objFSO.DeleteFile(strConfigHFileName)
End If

If doUpdate Then
    ' Read file in, do replacements. Then create new file and write out.
    Const ForReading = 1
    'Const ForWriting = 2
    Set objFile = objFSO.OpenTextFile(strConfigHInFileName, ForReading)
    strText = objFile.ReadAll
    objFile.Close
    strText = Replace(strText, "#undef  ARVIDEO_INPUT_IMAGE", "#define ARVIDEO_INPUT_IMAGE")
    strText = Replace(strText, "#undef  ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION", "#define ARVIDEO_INPUT_WINDOWS_MEDIA_FOUNDATION")
    strText = Replace(strText, "#undef  ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE", "#define ARVIDEO_INPUT_WINDOWS_MEDIA_CAPTURE")
    strText = Replace(strText, "#undef  ARVIDEO_INPUT_DEFAULT_WINDOWS_MEDIA_CAPTURE", "#define ARVIDEO_INPUT_DEFAULT_WINDOWS_MEDIA_CAPTURE")
    Set objFile = objFSO.CreateTextFile(strConfigHFileName)
    objFile.WriteLine strText
    objFile.Close
End If

Const TIMEOUT = 2
Dim strResults
If doUpdate Then
    strResults = "config.h was created or updated."
Else
    strResults = "config.h was not modified."
End If
objShell.Popup strResults, TIMEOUT, "ARToolKit Configure results", vbInformation + vbOKOnly
'WScript.echo strResults
