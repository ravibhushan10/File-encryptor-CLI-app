; =====================================================
; NSIS installer for fenc (File Encryptor)
; =====================================================

Name "fenc - File Encryptor"
OutFile "fencSetup.exe"
InstallDir "$PROGRAMFILES64\fenc"
InstallDirRegKey HKLM "Software\fenc" "Install_Dir"
RequestExecutionLevel admin

!include "MUI2.nsh"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

VIProductVersion "3.0.0.0"
VIAddVersionKey "ProductName"     "fenc - File Encryptor"
VIAddVersionKey "ProductVersion"  "3.0.0"
VIAddVersionKey "FileDescription" "AES-256 File Encryption CLI Tool"
VIAddVersionKey "LegalCopyright"  "Ravi Bhushan"
VIAddVersionKey "FileVersion"     "3.0.0"

; --- Install ---
Section "Install"

    SetOutPath "$INSTDIR"

    File "build\fenc.exe"
    File "build\libcrypto-3-x64.dll"
    File "build\libgcc_s_seh-1.dll"
    File "build\libstdc++-6.dll"
    File "build\libwinpthread-1.dll"

    ; Add to PATH using registry directly — simplest reliable method
    ; Reads current PATH, appends our folder, writes back
    ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" \
        "Path" "$0;$INSTDIR"
    ; Broadcast so all open windows see the new PATH immediately
    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000

    WriteRegStr HKLM "Software\fenc" "Install_Dir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc" \
        "DisplayName" "fenc - File Encryptor"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc" \
        "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc" \
        "DisplayVersion" "3.0.0"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc" \
        "Publisher" "Ravi Bhushan"

    WriteUninstaller "$INSTDIR\uninstall.exe"

    CreateDirectory "$SMPROGRAMS\fenc"
    CreateShortcut "$SMPROGRAMS\fenc\Uninstall.lnk" "$INSTDIR\uninstall.exe"

SectionEnd

; --- Uninstall ---
Section "Uninstall"

    Delete "$INSTDIR\fenc.exe"
    Delete "$INSTDIR\libcrypto-3-x64.dll"
    Delete "$INSTDIR\libgcc_s_seh-1.dll"
    Delete "$INSTDIR\libstdc++-6.dll"
    Delete "$INSTDIR\libwinpthread-1.dll"
    Delete "$INSTDIR\uninstall.exe"

    RMDir "$INSTDIR"

    Delete "$SMPROGRAMS\fenc\Uninstall.lnk"
    RMDir "$SMPROGRAMS\fenc"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc"
    DeleteRegKey HKLM "Software\fenc"

SectionEnd
