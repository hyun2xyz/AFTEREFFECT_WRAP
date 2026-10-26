@echo off
rem Double-click to install Edge Light Wrap. Runs install.cmd next to this file.
call "%~dp0install.cmd" --pause %*
exit /b %ERRORLEVEL%
