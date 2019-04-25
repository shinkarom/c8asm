@echo off
setlocal
call %~dp0build.cmd
echo.
call %~dp0run.cmd %*