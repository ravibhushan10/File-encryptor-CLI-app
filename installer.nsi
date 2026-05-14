; =====================================================
; NSIS = Nullsoft Scriptable Install System
; This script builds the Windows installer for FileEncryptor
; =====================================================

; --- Installer Metadata ---
Name "File Encryptor"
OutFile "FileEncryptorSetup.exe"
InstallDir "$PROGRAMFILES64\FileEncryptor"
InstallDirRegKey HKLM "Software\FileEncryptor" "Install_Dir"
RequestExecutionLevel admin

; --- Modern UI (MUI = Modern User Interface) ---
!include "MUI2.nsh"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; --- Version Info ---
VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName" "File Encryptor"
VIAddVersionKey "ProductVersion" "1.0.0"
VIAddVersionKey "FileDescription" "AES-256 File Encryption Tool"
VIAddVersionKey "LegalCopyright" "Ravi Bhushan"

; --- Installation Section ---
Section "Install"

    ; SETOUTPATH = sets the destination folder on the user's PC
    SetOutPath "$INSTDIR"

    ; FILE = copies each file into the installer package
    File "build\fileencryptor.exe"
    File "build\libcrypto-3-x64.dll"
    File "build\libgcc_s_seh-1.dll"
    File "build\libstdc++-6.dll"
    File "build\libwinpthread-1.dll"

    ; Write install path to registry so uninstaller can find it
    ; HKLM = HKEY_LOCAL_MACHINE — system-wide registry key
    WriteRegStr HKLM "Software\FileEncryptor" "Install_Dir" "$INSTDIR"

    ; Register uninstaller in Windows Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FileEncryptor" \
        "DisplayName" "File Encryptor"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FileEncryptor" \
        "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FileEncryptor" \
        "DisplayVersion" "1.0.0"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FileEncryptor" \
        "Publisher" "Ravi Bhushan"

    ; Create the uninstaller exe inside install folder
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Create Start Menu shortcut folder
    CreateDirectory "$SMPROGRAMS\FileEncryptor"
    CreateShortcut "$SMPROGRAMS\FileEncryptor\Uninstall.lnk" "$INSTDIR\uninstall.exe"

SectionEnd

; --- Uninstall Section ---
Section "Uninstall"

    ; DELETE = removes files from the user's PC
    Delete "$INSTDIR\fileencryptor.exe"
    Delete "$INSTDIR\libcrypto-3-x64.dll"
    Delete "$INSTDIR\libgcc_s_seh-1.dll"
    Delete "$INSTDIR\libstdc++-6.dll"
    Delete "$INSTDIR\libwinpthread-1.dll"
    Delete "$INSTDIR\uninstall.exe"

    ; RMDIR = Remove Directory
    RMDir "$INSTDIR"

    ; Remove Start Menu folder
    Delete "$SMPROGRAMS\FileEncryptor\Uninstall.lnk"
    RMDir "$SMPROGRAMS\FileEncryptor"

    ; Remove registry keys
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FileEncryptor"
    DeleteRegKey HKLM "Software\FileEncryptor"

SectionEnd
