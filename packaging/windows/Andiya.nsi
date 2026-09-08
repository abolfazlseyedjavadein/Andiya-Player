Unicode true
!include "MUI2.nsh"
Name "Andiya"
OutFile "${OUTPUT}"
InstallDir "$LOCALAPPDATA\Programs\Andiya"
InstallDirRegKey HKCU "Software\Andiya\Installer" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma
!define MUI_ICON "${ICON}"
!define MUI_UNICON "${ICON}"
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${LICENSE}"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Andiya" Main
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    File /r "${STAGE}\*"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    CreateDirectory "$SMPROGRAMS\Andiya"
    CreateShortcut "$SMPROGRAMS\Andiya\Andiya.lnk" "$INSTDIR\bin\Andiya.exe"
    CreateShortcut "$SMPROGRAMS\Andiya\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKCU "Software\Andiya\Installer" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Andiya" "DisplayName" "Andiya"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Andiya" "DisplayVersion" "${VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Andiya" "Publisher" "Andiya contributors"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Andiya" "DisplayIcon" "$INSTDIR\bin\Andiya.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Andiya" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Andiya" "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Andiya" "NoRepair" 1
SectionEnd

Section "Uninstall"
    SetShellVarContext current
    ; Generated list removes only shipped files. User-created files are preserved.
    !include "${UNINSTALL_FILES}"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"
    Delete "$SMPROGRAMS\Andiya\Andiya.lnk"
    Delete "$SMPROGRAMS\Andiya\Uninstall.lnk"
    RMDir "$SMPROGRAMS\Andiya"
    DeleteRegKey HKCU "Software\Andiya\Installer"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Andiya"
SectionEnd
