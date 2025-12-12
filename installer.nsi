!include "MUI2.nsh"

Name "Photo Editor"
OutFile "Photo-Editor-Setup.exe"
InstallDir "$PROGRAMFILES\Photo Editor"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"
    
    # Copy executable and DLLs
    File "build\Desktop_Qt_6_9_0_MinGW_64_bit-Release\release\Photo-Editor.exe"
    File "build\Desktop_Qt_6_9_0_MinGW_64_bit-Release\release\*.dll"
    File /r "build\Desktop_Qt_6_9_0_MinGW_64_bit-Release\release\platforms"
    File /r "build\Desktop_Qt_6_9_0_MinGW_64_bit-Release\release\styles"
    File /r "build\Desktop_Qt_6_9_0_MinGW_64_bit-Release\release\imageformats"
    File /r "build\Desktop_Qt_6_9_0_MinGW_64_bit-Release\release\iconengines"
    File "libopencv_core455.dll"
    File "libopencv_imgproc455.dll"

    # Copy resources
    SetOutPath "$INSTDIR\icons"
    File /r "icons\*.*"
    
    SetOutPath "$INSTDIR\translations"
    File /r "translations\*.*"
    
    # Create start menu shortcut
    CreateDirectory "$SMPROGRAMS\Photo Editor"
    CreateShortcut "$SMPROGRAMS\Photo Editor\Photo Editor.lnk" "$INSTDIR\Photo-Editor.exe"
    
    # Create uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
    
    # Add uninstall information to Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PhotoEditor" \
                     "DisplayName" "Photo Editor"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PhotoEditor" \
                     "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
SectionEnd

Section "Uninstall"
    # Remove files
    Delete "$INSTDIR\Photo-Editor.exe"
    Delete "$INSTDIR\*.dll"
    RMDir /r "$INSTDIR\platforms"
    RMDir /r "$INSTDIR\styles"
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\iconengines"
    RMDir /r "$INSTDIR\icons"
    RMDir /r "$INSTDIR\translations"
    Delete "$INSTDIR\uninstall.exe"
    
    # Remove shortcuts
    Delete "$SMPROGRAMS\Photo Editor\Photo Editor.lnk"
    RMDir "$SMPROGRAMS\Photo Editor"
    
    # Remove registry entries
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PhotoEditor"
    
    # Remove installation directory
    RMDir "$INSTDIR"
SectionEnd 