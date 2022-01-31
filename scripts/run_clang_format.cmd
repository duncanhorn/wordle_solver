
@echo off
setlocal
setlocal EnableDelayedExpansion

set THIS_DIR=%~dp0
set ROOT_DIR=%THIS_DIR%\..

set DIRS=tools
set EXTS=.cpp .h

for %%d in (%DIRS%) do call :recursive_format %ROOT_DIR%\%%d
goto :eof

:recursive_format
    :: First do files
    for %%e in (%EXTS%) do (
        for %%f in (%1\*%%e) do call :run_clang_format %%f
    )
    :: Now do subdirectories
    for /d %%d in (%1\*) do call :recursive_format %%d
    goto :eof

:run_clang_format
    clang-format -style=file -i %1
    goto :eof
