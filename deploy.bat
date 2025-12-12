@echo off
echo Deploying Qt dependencies...

REM Set Qt path - adjust this to match your Qt installation
set QT_PATH=M:\Qt\6.9.0\mingw_64

REM Run windeployqt
"%QT_PATH%\bin\windeployqt.exe" "build\Desktop_Qt_6_9_0_MinGW_64_bit-Release\release\Photo-Editor.exe" --release

echo Deployment complete!
pause 