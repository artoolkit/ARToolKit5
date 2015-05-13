'
' ARToolKit Professional
' Windows QuickTime 7.3 SDK patch script for Windows 8 SDK (included with Visual Studio 2013).
'
' Copyright 2014 ARToolworks, Inc.
'
'
'  ARToolKit5
'  PatchQuickTime73SDK.vbs
'
'  Use this script to patch the Windows QuickTime 7.3 SDK so that it can be used correctly
'  with the Windows 8 SDK (included with Visual Studio 2013).
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

'WScript.echo "OK"
'WScript.Quit

'Ensure we're running as Administrator
If Not WScript.Arguments.Named.Exists("elevate") Then
  CreateObject("Shell.Application").ShellExecute WScript.FullName, chr(34) & WScript.ScriptFullName & Chr(34) & " /elevate", "", "runas", 1
  WScript.Quit
End If

Const strFileName1 = "C:\Program Files (x86)\QuickTime SDK\CIncludes\fp.h"
Const strFileName2 = "C:\Program Files (x86)\QuickTime SDK\CIncludes\Processes.h"

' Change working directory to the same folder as this script.
Set objFSO = CreateObject("Scripting.FileSystemObject")
Set objShell = CreateObject("WScript.Shell")
objShell.CurrentDirectory = objFSO.GetParentFolderName(WScript.ScriptFullName)

Const ForReading = 1
Const ForWriting = 2

' For fp.h, a simple single-line find and replace.
Set objFile = objFSO.OpenTextFile(strFileName1, ForReading)
strText = objFile.ReadAll
objFile.Close

strText1 = Replace(strText, "#if (defined(__MWERKS__) && defined(__cmath__)) || (TARGET_RT_MAC_MACHO && defined(__MATH__))", "#if (_WIN32_WINNT >= 0x0602) || (_MSC_VER >= 1800) || (defined(__MWERKS__) && defined(__cmath__)) || (TARGET_RT_MAC_MACHO && defined(__MATH__))")

patched1 = StrComp(strText, strText1)
If Not patched1 = 0 Then
    Set objFile = objFSO.OpenTextFile(strFileName1, ForWriting)
    objFile.Write strText1
    objFile.Close
End If

' For Processes.h, multi-line find and replace necessitates regular expression.
Set objFile = objFSO.OpenTextFile(strFileName2, ForReading)
strText = objFile.ReadAll
objFile.Close

Set re = New RegExp
re.Pattern = " \*/\r\nEXTERN_API\( OSErr \)\r\nGetProcessInformation\(\r\n  const ProcessSerialNumber \*  PSN,\r\n  ProcessInfoRec \*             info\)                          THREEWORDINLINE\(0x3F3C, 0x003A, 0xA88F\);\r\n"
strText1 = re.Replace(strText, " */" & vbCrLf & "#if _WIN32_WINNT < 0x0602" & vbCrLf & "EXTERN_API( OSErr )" & vbCrLf & "GetProcessInformation(" & vbCrLf & "  const ProcessSerialNumber *  PSN," & vbCrLf & "  ProcessInfoRec *             info)                          THREEWORDINLINE(0x3F3C, 0x003A, 0xA88F);" & vbCrLf & "#endif" & vbCrLf)

patched2 = StrComp(strText, strText1)
If Not patched2 = 0 Then
    Set objFile = objFSO.OpenTextFile(strFileName2, ForWriting)
    objFile.Write strText1
    objFile.Close
End If

Const TIMEOUT = 10
Dim strResults
If patched1 Or patched2 Then
    strResults = "QuickTime headers were patched."
Else
    strResults = "QuickTime headers were not modified."
End If
objShell.Popup strResults, TIMEOUT, "QuickTime 7.3 SDK patch results", vbInformation + vbOKOnly
'WScript.echo strResults
