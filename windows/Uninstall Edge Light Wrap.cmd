@echo off
rem Double-click to remove Edge Light Wrap. The plug-in is moved to a backup folder.
call "%~dp0install.cmd" --uninstall --pause %*
exit /b %ERRORLEVEL%
