; =====================================================
; NSIS installer for fenc (File Encryptor)
; =====================================================
; MUI2.nsh = Modern User Interface 2 header file
; It gives us beautiful pages like Welcome, License,
; Directory picker, Progress, Finish
; !include = include an external file (like #include in C++)

Name "fenc - File Encryptor"
; Name = title shown in installer window title bar

OutFile "fenc-v3.0.0-windows.exe"
; OutFile = name of the final installer .exe file generated

InstallDir "$PROGRAMFILES64\fenc"
; $PROGRAMFILES64 = C:\Program Files\ on 64-bit Windows
; This sets default install location to C:\Program Files\fenc

InstallDirRegKey HKLM "Software\fenc" "Install_Dir"
; HKLM = HKEY_LOCAL_MACHINE = Windows Registry root for all users
; This reads previous install location from registry if app was installed before

RequestExecutionLevel admin
; Why? Because writing to C:\Program Files\ and editing
; system PATH requires Administrator privileges

!include "MUI2.nsh"
; MUI2 = Modern User Interface 2
; This gives us all the beautiful installer page macros

; =====================================================
; INSTALLER PAGES — shown in this exact order
; =====================================================

!insertmacro MUI_PAGE_WELCOME
; Shows "Welcome to fenc 3.0.0 Setup" screen

; LICENSE PAGE SETTINGS
; LicenseData = which file to show as license text
!define MUI_LICENSEPAGE_TEXT_TOP "Please read the license agreement before installing fenc."
!define MUI_LICENSEPAGE_BUTTON "I Agree"
; MUI_LICENSEPAGE_BUTTON = text on the button user clicks to accept
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
; Shows MIT license text from LICENSE.txt file

!insertmacro MUI_PAGE_DIRECTORY
; Lets user choose install folder (default: C:\Program Files\fenc\)

; INSTFILES PAGE = the actual installation progress page
!define MUI_INSTFILESPAGE_FINISHHEADER_TEXT "Installation Complete"
!insertmacro MUI_PAGE_INSTFILES
; Shows progress bar while files are being copied

; FINISH PAGE SETTINGS
!define MUI_FINISHPAGE_TITLE "fenc Installed Successfully!"
; Title shown on the finish screen

!define MUI_FINISHPAGE_TEXT "fenc 3.0.0 has been installed on your computer.$\r$\n$\r$\nOpen a NEW terminal (CMD or PowerShell) and type:$\r$\n$\r$\n    fenc --help$\r$\n$\r$\nto get started."
; $\r$\n = Windows newline (Carriage Return + Line Feed)
; This is the message shown on finish screen

!define MUI_FINISHPAGE_SHOWREADME
; Adds a checkbox "View README" on finish screen
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View README (usage guide)"
; Text shown next to the checkbox
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION OpenReadme
; Which function to call when checkbox is ticked and Finish clicked

!insertmacro MUI_PAGE_FINISH
; Shows the finish screen

; =====================================================
; UNINSTALLER PAGES
; =====================================================
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; =====================================================
; LANGUAGE
; =====================================================
!insertmacro MUI_LANGUAGE "English"
; Must come AFTER all page declarations

; =====================================================
; VERSION INFO — shows in file Properties on Windows
; =====================================================
VIProductVersion "3.0.0.0"
VIAddVersionKey "ProductName"     "fenc - File Encryptor"
VIAddVersionKey "ProductVersion"  "3.0.0"
VIAddVersionKey "FileDescription" "AES-256 File Encryption CLI Tool"
VIAddVersionKey "LegalCopyright"  "Ravi Bhushan"
VIAddVersionKey "FileVersion"     "3.0.0"

; =====================================================
; FUNCTION — runs when user ticks "View README" on finish
; =====================================================
Function OpenReadme
    ; ExecShell = open a file with its default program
    ; "open" = open action (like double clicking)
    ; $INSTDIR = where fenc was installed
    ExecShell "open" "$INSTDIR\README.md"
FunctionEnd

; =====================================================
; INSTALL SECTION — actual installation steps
; =====================================================
Section "Install"

    SetOutPath "$INSTDIR"
    ; SetOutPath = set where files will be copied to
    ; $INSTDIR = the install directory user chose

    ; Showing detail text during progress bar
    DetailPrint "Installing fenc core files..."
    ; DetailPrint = prints a message in the progress log area

    File "build\fenc.exe"
    File "build\libcrypto-3-x64.dll"
    ; libcrypto-3-x64.dll = OpenSSL cryptography DLL
    ; DLL = Dynamic Link Library — shared code your .exe needs to run
    File "build\libgcc_s_seh-1.dll"
    ; libgcc_s_seh-1.dll = GCC runtime DLL
    ; SEH = Structured Exception Handling — Windows error handling system
    File "build\libstdc++-6.dll"
    ; libstdc++-6.dll = C++ Standard Library DLL
    File "build\libwinpthread-1.dll"
    ; libwinpthread-1.dll = Windows threading library DLL

    ; Copy README so user can view it from finish screen
    File "README.md"

    DetailPrint "Creating LICENSE file..."
    File "LICENSE.txt"

    DetailPrint "Adding fenc to system PATH..."
    ; PATH = environment variable Windows uses to find programs
    ; We read current PATH, append our folder, write it back
    ReadRegStr $0 HKLM \
        "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    ; ReadRegStr = read a string value from Windows Registry
    ; $0 = variable to store the result in
    ; HKLM = HKEY_LOCAL_MACHINE

    WriteRegExpandStr HKLM \
        "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" \
        "Path" "$0;$INSTDIR"
    ; WriteRegExpandStr = write expandable string to registry
    ; We append ;$INSTDIR to the existing PATH

    SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 \
        "STR:Environment" /TIMEOUT=5000
    ; SendMessage = broadcasts a Windows message to all open windows
    ; HWND_BROADCAST = send to ALL windows
    ; WM_SETTINGCHANGE = "hey, a system setting changed"
    ; This tells all open apps to reload PATH immediately
    ; TIMEOUT=5000 = wait max 5000 milliseconds = 5 seconds

    DetailPrint "Writing registry entries..."
    WriteRegStr HKLM "Software\fenc" "Install_Dir" "$INSTDIR"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc" \
        "DisplayName" "fenc - File Encryptor"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc" \
        "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc" \
        "DisplayVersion" "3.0.0"
    WriteRegStr HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc" \
        "Publisher" "Ravi Bhushan"
    ; These registry entries make fenc appear in
    ; Windows Settings → Apps → Installed Apps

    DetailPrint "Creating uninstaller..."
    WriteUninstaller "$INSTDIR\uninstall.exe"

    CreateDirectory "$SMPROGRAMS\fenc"
    ; $SMPROGRAMS = Start Menu → Programs folder
    CreateShortcut "$SMPROGRAMS\fenc\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    ; Creates uninstall shortcut in Start Menu

    DetailPrint "Installation complete!"

SectionEnd

; =====================================================
; UNINSTALL SECTION
; =====================================================
Section "Uninstall"

    Delete "$INSTDIR\fenc.exe"
    Delete "$INSTDIR\libcrypto-3-x64.dll"
    Delete "$INSTDIR\libgcc_s_seh-1.dll"
    Delete "$INSTDIR\libstdc++-6.dll"
    Delete "$INSTDIR\libwinpthread-1.dll"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\LICENSE.txt"
    Delete "$INSTDIR\uninstall.exe"

    RMDir "$INSTDIR"
    ; RMDir = Remove Directory (only works if empty)

    Delete "$SMPROGRAMS\fenc\Uninstall.lnk"
    RMDir "$SMPROGRAMS\fenc"

    ; Remove PATH entry
    ReadRegStr $0 HKLM \
        "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    ; We need to remove $INSTDIR from PATH during uninstall

    DeleteRegKey HKLM \
        "Software\Microsoft\Windows\CurrentVersion\Uninstall\fenc"
    DeleteRegKey HKLM "Software\fenc"
    ; DeleteRegKey = removes the registry key and all its values

SectionEnd
