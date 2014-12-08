Name "Sauerbraten WC NG"

!tempfile "version.txt"
!appendfile "version.txt" "!define VERSION "
!system "get_version.exe >> version.txt"
!include "version.txt"
!delfile "version.txt"

OutFile "wc-ng-sdl2-v${VERSION}_windows_setup.exe"

SetCompressor /SOLID lzma
CRCCheck on

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Function .onInit

    ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sauerbraten_wc" "UninstallString"
    
    IfErrors wcnotinstalled
        MessageBox MB_YESNO "Please uninstall Sauerbraten_wc before installing a newer/older version of Sauerbraten_wc." IDYES true IDNO false
        true:
            Exec $0
            ClearErrors
            goto wcuninstalled
        false:
            MessageBox MB_OK "You really should be uninstalling previous versions, before installing a newer/older version. You have been warned."  
            
    wcnotinstalled:
    ClearErrors
    
    wcuninstalled:

    ReadRegStr $INSTDIR HKLM "Software\Sauerbraten" "Install_Dir"
    
    IfErrors 0 sauerinstalled
        MessageBox MB_OK "You do not seem to have Sauerbraten installed. If you do, make sure,$\r$\nyou are installing this client into the correct Sauerbraten installation path."
        ClearErrors
    sauerinstalled:
    
    SetOutPath $INSTDIR

FunctionEnd

Section "Sauerbraten WC NG (required)"

    SetOutPath $INSTDIR
    SetDateSave off

    File /r /x "src" "..\..\..\bin"
    File /r /x "src" "..\..\..\bin64"
    File /r /x "src" "..\..\..\plugins"
    File /r /x "src" "..\..\data"
    File /r /x "src" "..\..\doc"
    File /r /x "src" "..\..\packages"

    WriteRegDWORD HKLM SOFTWARE\Sauerbraten "WC_INSTALLED"     0x00000001
    WriteRegStr   HKLM SOFTWARE\Sauerbraten "WC_INSTALL_DIR"   $INSTDIR
    WriteRegDWORD HKLM SOFTWARE\Sauerbraten "WC_INSTALLED_VER" ${VERSION}
    
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sauerbraten_wc" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sauerbraten_wc" "NoRepair" 1
    WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sauerbraten_wc" "UninstallString" "$INSTDIR\uninstall_wc.exe"
    WriteUninstaller "uninstall_wc.exe"

    FileOpen $9 sauerbraten_wc.bat w

    FileWrite $9 "@ECHO OFF$\r$\n"
    FileWrite $9 "set SAUER_BIN=bin$\r$\n"
    FileWrite $9 "IF /I $\"%PROCESSOR_ARCHITECTURE%$\" == $\"amd64$\" ($\r$\n"
    FileWrite $9 "    set SAUER_BIN=bin64$\r$\n"
    FileWrite $9 ")$\r$\n"
    FileWrite $9 "IF /I $\"%PROCESSOR_ARCHITEW6432%$\" == $\"amd64$\" ($\r$\n"
    FileWrite $9 "    set SAUER_BIN=bin64$\r$\n"
    FileWrite $9 ")$\r$\n"
    FileWrite $9 "start %SAUER_BIN%\sauerbraten_wc.exe $\"-q$$HOME\My Games\Sauerbraten$\" -glog.txt %*$\r$\n"

    FileClose $9
    
    IfFileExists "$INSTDIR\wc_backup.cfg" WCConfigFound WCConfigNotFound
    WCConfigFound:
        Rename "$INSTDIR\wc_backup.cfg" "$INSTDIR\wc-sdl2.cfg"
    WCConfigNotFound:

SectionEnd

Section "Start Menu Shortcuts"

    CreateDirectory "$SMPROGRAMS\Sauerbraten_wc"

    CreateShortCut "$SMPROGRAMS\Sauerbraten_wc\WC_README.lnk"      "$INSTDIR\doc\WC_README.html" "" "$INSTDIR\doc\WC_README.html" 0
    CreateShortCut "$INSTDIR\Sauerbraten_wc.lnk"                   "$INSTDIR\sauerbraten_wc.bat" "" "$INSTDIR\bin\sauerbraten_wc.exe" 0 SW_SHOWMINIMIZED
    CreateShortCut "$SMPROGRAMS\Sauerbraten_wc\Sauerbraten_wc.lnk" "$INSTDIR\sauerbraten_wc.bat" "" "$INSTDIR\bin\sauerbraten_wc.exe" 0 SW_SHOWMINIMIZED
    CreateShortCut "$SMPROGRAMS\Sauerbraten_wc\Uninstall.lnk"      "$INSTDIR\uninstall_wc.exe"   "" "$INSTDIR\uninstall_wc.exe" 0

SectionEnd

Section "Desktop Shortcuts"

    CreateShortCut "$DESKTOP\WC_README.lnk"                        "$INSTDIR\doc\WC_README.html" "" "$INSTDIR\doc\WC_README.html" 0
    CreateShortCut "$DESKTOP\Sauerbraten_wc.lnk"                   "$INSTDIR\sauerbraten_wc.bat" "" "$INSTDIR\bin\sauerbraten_wc.exe" 0 SW_SHOWMINIMIZED

SectionEnd


Section "Uninstall"

    ReadRegStr $INSTDIR HKLM "Software\Sauerbraten" "WC_INSTALL_DIR"
    
    IfFileExists "$INSTDIR\wc-sdl2.cfg" WCConfigFound WCConfigNotFound
    WCConfigFound:
        Delete "$INSTDIR\wc_backup.cfg"
        Rename "$INSTDIR\wc-sdl2.cfg" "$INSTDIR\wc_backup.cfg"
    WCConfigNotFound:

    Rename "$INSTDIR\wc-sdl2.cfg" "$INSTDIR\wc_backup.cfg"

    DeleteRegValue HKLM "Software\Sauerbraten" "WC_INSTALLED"
    DeleteRegValue HKLM "Software\Sauerbraten" "WC_INSTALL_DIR"
    DeleteRegValue HKLM "Software\Sauerbraten" "WC_INSTALLED_VER"
    
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sauerbraten_wc"
    
    Delete "$INSTDIR\bin\Sauerbraten_wc.exe"
    Delete "$INSTDIR\bin64\Sauerbraten_wc.exe"
    Delete "$INSTDIR\wc-sdl2.cfg"
    Delete "$INSTDIR\data\GeoIP.dat"
    Delete "$INSTDIR\Sauerbraten_wc.bat"
    Delete "$INSTDIR\Sauerbraten_wc.lnk"
    Delete "$INSTDIR\plugins\*.dll"
    RMDir  "$INSTDIR\plugins"
    RMDir /r "$INSTDIR\doc\examples"
    Delete "$INSTDIR\doc\style.css"
    Delete "$INSTDIR\doc\WC*.html"
    RMDir  "$INSTDIR\doc"
    Delete "$INSTDIR\uninstall_wc.exe"
        
    RMDir /r "$SMPROGRAMS\Sauerbraten_wc\"
    Delete "$DESKTOP\Sauerbraten_wc.lnk"
    Delete "$DESKTOP\WC_README.lnk"
    
    SetShellVarContext all
    RMDir /r "$SMPROGRAMS\Sauerbraten_wc\"
    Delete "$DESKTOP\Sauerbraten_wc.lnk"
    Delete "$DESKTOP\WC_README.lnk"

SectionEnd
