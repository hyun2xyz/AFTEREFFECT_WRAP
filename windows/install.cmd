@echo off
rem Edge Light Wrap 2.0.0 Windows installer. Plain cmd.exe; no PowerShell script involved.
rem Installs EdgeLightWrap.aex into the Adobe common MediaCore folder. Writing there needs
rem one UAC approval by the student. Does not quit After Effects or change security settings.
rem   --dry-run            check the package and print the target only
rem   --uninstall          move the installed plug-in to a recoverable backup
rem   --sandbox-root DIR   rehearse in DIR instead of the real Adobe folder (tests)
rem   --pause              wait for a key before closing (double-click launchers)
setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2.0.0"
set "MATCH_NAME=Hyun2 Edge Light Wrap"
set "PACKAGE_DIR=%~dp0"
set "PACKAGE_DIR=%PACKAGE_DIR:~0,-1%"
set "SOURCE=%PACKAGE_DIR%\EdgeLightWrap.aex"
set "SELF=%~f0"
set "DRY_RUN=0"
set "UNINSTALL=0"
set "SANDBOX_ROOT="
set "PAUSE_AT_END=0"
set "ELEVATED=0"
set "ACTION=install"
set "CODE=1"
set "MSG="

:parse
if "%~1"=="" goto parsed
if "%~1"=="--dry-run" set "DRY_RUN=1" & shift & goto parse
if "%~1"=="--uninstall" set "UNINSTALL=1" & set "ACTION=removal" & shift & goto parse
if "%~1"=="--pause" set "PAUSE_AT_END=1" & shift & goto parse
if "%~1"=="--elevated" set "ELEVATED=1" & shift & goto parse
if "%~1"=="--sandbox-root" goto opt_sandbox
set "MSG=Unknown option: %~1"
goto fail
:opt_sandbox
if "%~2"=="" set "MSG=--sandbox-root needs a folder." & goto fail
set "SANDBOX_ROOT=%~f2"
shift
shift
goto parse

:parsed
if defined SANDBOX_ROOT goto sandbox_paths
set "PF=%ProgramW6432%"
if not defined PF set "PF=%ProgramFiles%"
call :find_mediacore
set "SUPPORT_DIR=%ProgramData%\Edge Light Wrap"
goto paths_done
:sandbox_paths
if not exist "%SANDBOX_ROOT%\*" set "MSG=The rehearsal folder must already exist." & goto fail
set "PLUGIN_DIR=%SANDBOX_ROOT%\MediaCore"
set "SUPPORT_DIR=%SANDBOX_ROOT%\Support"
:paths_done
set "TARGET=%PLUGIN_DIR%\EdgeLightWrap.aex"
set "BACKUPS=%SUPPORT_DIR%\Backups"
set "LOCK=%SUPPORT_DIR%\.install-lock"

if "%UNINSTALL%"=="1" goto header
if not exist "%PACKAGE_DIR%\payload.sha256" goto missing_payload
if not exist "%SOURCE%" goto missing_payload
set "EXPECTED="
for /f "usebackq tokens=1,*" %%A in ("%PACKAGE_DIR%\payload.sha256") do if "%%B"=="EdgeLightWrap.aex" set "EXPECTED=%%A"
if not defined EXPECTED goto missing_payload
call :sha256 "%SOURCE%"
if /I not "%HASH%"=="%EXPECTED%" set "MSG=Package file is damaged or changed: EdgeLightWrap.aex. Download the whole repository again." & goto fail
if not "%ELEVATED%"=="1" echo EdgeLightWrap.aex: OK

:header
if "%ELEVATED%"=="1" goto preflight
echo.
echo Edge Light Wrap %VERSION% (Windows x64)
echo Plug-in path: %TARGET%
if not defined SANDBOX_ROOT call :report_after_effects
if "%DRY_RUN%"=="1" echo Check complete. No files were changed and After Effects was not closed.& set "CODE=0" & goto finish
:preflight
rem Nothing to change when this exact file is installed: no need to quit AE or elevate.
if "%UNINSTALL%"=="1" goto preflight_checks
if not exist "%TARGET%" goto preflight_checks
if exist "%TARGET%\*" goto preflight_checks
call :sha256 "%TARGET%"
if /I "%HASH%"=="%EXPECTED%" echo.& echo The same version is already installed. Nothing was changed.& set "CODE=0" & goto finish
:preflight_checks
if defined SANDBOX_ROOT goto run_action
call :after_effects_running
if "%AE_RUNNING%"=="1" set "MSG=Save your work and quit After Effects yourself, then run this again." & goto fail
fltmc >nul 2>&1
if errorlevel 1 goto elevate
:run_action
call :check_links
if defined MSG goto fail
if "%UNINSTALL%"=="1" goto do_uninstall

rem ---------- install ----------
if exist "%TARGET%\*" set "MSG=A folder with the plug-in's name is in the way: %TARGET%" & goto fail
if not exist "%TARGET%" goto install_new
findstr /M /C:"%MATCH_NAME%" "%TARGET%" >nul 2>&1
if errorlevel 1 set "MSG=Another plug-in uses this file name. It was not overwritten." & goto fail
:install_new
if not exist "%PLUGIN_DIR%\" mkdir "%PLUGIN_DIR%" || (set "MSG=Could not create the plug-in folder." & goto fail)
if not exist "%BACKUPS%\" mkdir "%BACKUPS%" || (set "MSG=Could not create the backup folder." & goto fail)
mkdir "%LOCK%" 2>nul || (set "MSG=Another install or removal is running." & goto fail)
set "STAGE=%PLUGIN_DIR%\.EdgeLightWrap-stage-%RANDOM%%RANDOM%.tmp"
set "BACKUP="
set "OLD_MOVED=0"
set "NEW_MOVED=0"
rem The stage file is not named .aex, so a starting AE never loads a half-copied file.
copy /b /y "%SOURCE%" "%STAGE%" >nul || (set "MSG=Copying the plug-in failed." & goto rollback)
call :sha256 "%STAGE%"
if /I not "%HASH%"=="%EXPECTED%" set "MSG=The copied file differs from the package." & goto rollback
if not exist "%TARGET%" goto place_new
call :timestamp
set "BACKUP=%BACKUPS%\EdgeLightWrap-%STAMP%.aex"
move /y "%TARGET%" "%BACKUP%" >nul || (set "MSG=Could not move the old version to the backup folder." & goto rollback)
set "OLD_MOVED=1"
:place_new
move /y "%STAGE%" "%TARGET%" >nul || (set "MSG=Could not place the new plug-in." & goto rollback)
set "NEW_MOVED=1"
call :sha256 "%TARGET%"
if /I not "%HASH%"=="%EXPECTED%" set "MSG=The installed file could not be verified." & goto rollback
set "RECEIPT=%SUPPORT_DIR%\install-receipt.txt"
> "%RECEIPT%" echo version=%VERSION%
>> "%RECEIPT%" echo plugin=%TARGET%
>> "%RECEIPT%" echo backup=%BACKUP%
>> "%RECEIPT%" echo sha256=%EXPECTED%
rmdir "%LOCK%" 2>nul
echo.
echo Install complete. Open After Effects and choose Effect ^> Hyun2 ^> Edge Light Wrap.
if defined BACKUP echo Previous version backed up to: %BACKUP%
set "CODE=0"
goto finish

:rollback
if "%NEW_MOVED%"=="1" if exist "%TARGET%" del /f /q "%TARGET%" >nul 2>&1
if "%OLD_MOVED%"=="1" if exist "%BACKUP%" move /y "%BACKUP%" "%TARGET%" >nul
if exist "%STAGE%" del /f /q "%STAGE%" >nul 2>&1
rmdir "%LOCK%" 2>nul
goto fail

rem ---------- uninstall ----------
:do_uninstall
if not exist "%TARGET%" echo Edge Light Wrap is not installed on this computer.& set "CODE=0" & goto finish
if exist "%TARGET%\*" set "MSG=Other plug-ins are not removed." & goto fail
findstr /M /C:"%MATCH_NAME%" "%TARGET%" >nul 2>&1
if errorlevel 1 set "MSG=Other plug-ins are not removed." & goto fail
if not exist "%BACKUPS%\" mkdir "%BACKUPS%" || (set "MSG=Could not create the backup folder." & goto fail)
mkdir "%LOCK%" 2>nul || (set "MSG=Another install or removal is running." & goto fail)
call :timestamp
set "BACKUP=%BACKUPS%\Removed-EdgeLightWrap-%STAMP%.aex"
move /y "%TARGET%" "%BACKUP%" >nul || (rmdir "%LOCK%" 2>nul & set "MSG=Could not move the plug-in to the backup folder." & goto fail)
> "%SUPPORT_DIR%\uninstall-receipt.txt" echo backup=%BACKUP%
rmdir "%LOCK%" 2>nul
echo Removed. The plug-in was kept in a recoverable backup:
echo %BACKUP%
set "CODE=0"
goto finish

rem ---------- elevation ----------
rem The elevated copy writes its output to a log that this window prints afterwards.
:elevate
if "%ELEVATED%"=="1" set "MSG=Administrator rights were not granted." & goto fail
set "ELW_SELF=%SELF%"
set "ELW_LOG=%TEMP%\EdgeLightWrap-%RANDOM%%RANDOM%.log"
set "ELW_ARGS=--elevated"
if "%UNINSTALL%"=="1" set "ELW_ARGS=--elevated --uninstall"
echo.
echo A Windows administrator prompt (UAC) will open. Choose [Yes]: the shared Adobe plug-in folder needs it.
powershell.exe -NoProfile -NonInteractive -Command "$q=[char]34; $a='/d /c '+$q+$q+$env:ELW_SELF+$q+' '+$env:ELW_ARGS+' > '+$q+$env:ELW_LOG+$q+' 2>&1'+$q; try { $p=Start-Process -FilePath $env:ComSpec -ArgumentList $a -Verb RunAs -WindowStyle Hidden -Wait -PassThru; exit 0 } catch { exit 1223 }"
if errorlevel 1223 set "MSG=The administrator prompt was cancelled, so nothing was changed. Run again and choose [Yes]." & goto fail
if not exist "%ELW_LOG%" set "MSG=The elevated %ACTION% did not report a result." & goto fail
findstr /V /B /C:"__EXIT__=" "%ELW_LOG%"
set "CODE=1"
for /f "usebackq tokens=2 delims==" %%C in (`findstr /B /C:"__EXIT__=" "%ELW_LOG%"`) do set "CODE=%%C"
del /f /q "%ELW_LOG%" >nul 2>&1
goto finish

rem ---------- helpers ----------
:missing_payload
set "MSG=EdgeLightWrap.aex and payload.sha256 must stay together in the windows folder. Download the whole repository."
goto fail

:find_mediacore
rem AE records the shared plug-in folder in the registry; fall back to Adobe's default.
set "PLUGIN_DIR="
for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\Adobe\After Effects" /s /v CommonPluginInstallPath /reg:64 2^>nul ^| findstr /I /C:"CommonPluginInstallPath"') do if not defined PLUGIN_DIR set "PLUGIN_DIR=%%B"
if not defined PLUGIN_DIR set "PLUGIN_DIR=%PF%\Adobe\Common\Plug-ins\7.0\MediaCore"
if "%PLUGIN_DIR:~-1%"=="\" set "PLUGIN_DIR=%PLUGIN_DIR:~0,-1%"
exit /b 0

:report_after_effects
set "AE_FOUND=0"
for /d %%D in ("%PF%\Adobe\Adobe After Effects*") do if exist "%%~D\Support Files\AfterFX.exe" call :report_one "%%~D"
if "%AE_FOUND%"=="0" echo Note: After Effects was not found in the default folder. Install AE, then check again.
exit /b 0
:report_one
set "AE_FOUND=1"
echo After Effects: %~1
for /f "delims=" %%F in ('dir /s /b "%~1\Support Files\Plug-ins\EdgeLightWrap.aex" 2^>nul') do echo Warning: AE's own Plug-ins folder also has this plug-in. Keep only one: %%F
exit /b 0

:after_effects_running
set "AE_RUNNING=0"
tasklist /FI "IMAGENAME eq AfterFX.exe" /NH 2>nul | find /I "AfterFX.exe" >nul && set "AE_RUNNING=1"
tasklist /FI "IMAGENAME eq aerender.exe" /NH 2>nul | find /I "aerender.exe" >nul && set "AE_RUNNING=1"
exit /b 0

:check_links
for %%P in ("%PLUGIN_DIR%" "%SUPPORT_DIR%" "%BACKUPS%" "%TARGET%") do call :check_link "%%~P"
exit /b 0
:check_link
if not exist "%~1" exit /b 0
set "ATTR=%~a1"
if not "%ATTR:l=%"=="%ATTR%" set "MSG=Links (reparse points) are not modified: %~1"
exit /b 0

:sha256
set "HASH="
for /f "skip=1 delims=" %%H in ('certutil -hashfile "%~1" SHA256') do if not defined HASH set "HASH=%%H"
if defined HASH set "HASH=%HASH: =%"
exit /b 0

:timestamp
set "STAMP="
for /f %%T in ('powershell.exe -NoProfile -NonInteractive -Command "Get-Date -Format yyyyMMdd-HHmmss"') do set "STAMP=%%T"
if not defined STAMP set "STAMP=%RANDOM%%RANDOM%"
set "STAMP=%STAMP%-%RANDOM%"
exit /b 0

:fail
>&2 echo %MSG%
set "CODE=1"
:finish
if not "%CODE%"=="0" if not "%ELEVATED%"=="1" >&2 echo.& >&2 echo The %ACTION% did not finish. Fix the error above and run again.
if "%ELEVATED%"=="1" echo __EXIT__=%CODE%
if "%PAUSE_AT_END%"=="1" pause
exit /b %CODE%
