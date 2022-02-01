@echo off
setlocal
setlocal EnableDelayedExpansion

set BUILD_ROOT=%~dp0\..\build

goto :init

:usage
    echo USAGE:
    echo     init.cmd [-c^|--compiler ^<clang^|msvc^>] [-b^|--build ^<debug^|release^|relwithdebinfo^|minsizerel^>]
    echo         [-g^|--generator ^<ninja^|msbuild^>] [-l^|--linker ^<lld-link^|link^>]
    echo.
    echo ARGUMENTS
    echo     -c^|--compiler
    echo         Controls the compiler that is used: 'clang' (default) or 'msvc'
    echo     -b^|--build
    echo         Controls the value of 'CMAKE_BUILD_TYPE': 'debug' (default), 'release', 'relwithdebinfo', or 'minsizerel'
    echo     -g^|--generator
    echo         Controls the generator that is used: 'ninja' (default) or 'msbuild'. Note that 'msbuild' is only applicable
    echo         when 'msvc' is used as the compiler
    echo     -l^|--linker
    echo         Controls the linker that is used: 'lld-link' (default) or 'link'
    echo.
    goto :eof

:init
    set COMPILER=
    set GENERATOR=
    set BUILD_TYPE=
    set LINKER=

:parse
    if /I "%~1"=="" goto :execute

    if /I "%~1"=="--help" call :usage & goto :eof

    set ARGUMENT_SET=0
    if /I "%~1"=="-c" set ARGUMENT_SET=1
    if /I "%~1"=="--compiler" set ARGUMENT_SET=1
    if %ARGUMENT_SET%==1 (
        if "%COMPILER%" NEQ "" echo ERROR: Compiler can only be specified once & call :usage & exit /B 1

        if /I "%~2"=="clang" set COMPILER=clang
        if /I "%~2"=="msvc" set COMPILER=msvc
        if "!COMPILER!"=="" echo ERROR: Unrecognized/missing compiler %~2 & call :usage & exit /B 1

        shift & shift
        goto :parse
    )

    if /I "%~1"=="-g" set ARGUMENT_SET=1
    if /I "%~1"=="--generator" set ARGUMENT_SET=1
    if %ARGUMENT_SET%==1 (
        if "%GENERATOR%" NEQ "" echo ERROR: Generator can only be specified once & call :usage & exit /B 1

        if /I "%~2"=="ninja" set GENERATOR=ninja
        if /I "%~2"=="msbuild" set GENERATOR=msbuild
        if "!GENERATOR!"=="" echo ERROR: Unrecognized/missing generator %~2 & call :usage & exit /B 1

        shift & shift
        goto :parse
    )

    if /I "%~1"=="-b" set ARGUMENT_SET=1
    if /I "%~1"=="--build" set ARGUMENT_SET=1
    if %ARGUMENT_SET%==1 (
        if "%BUILD_TYPE%" NEQ "" echo ERROR: Build type can only be specified once & call :usage & exit /B 1

        if /I "%~2"=="debug" set BUILD_TYPE=debug
        if /I "%~2"=="release" set BUILD_TYPE=release
        if /I "%~2"=="relwithdebinfo" set BUILD_TYPE=relwithdebinfo
        if /I "%~2"=="minsizerel" set BUILD_TYPE=minsizerel
        if "!BUILD_TYPE!"=="" echo ERROR: Unrecognized/missing build type %~2 & call :usage & exit /B 1

        shift & shift
        goto :parse
    )

    if /I "%~1"=="-l" set ARGUMENT_SET=1
    if /I "%~1"=="--linker" set ARGUMENT_SET=1
    if %ARGUMENT_SET%==1 (
        if "%LINKER%" NEQ "" echo ERROR: Linker can only be specified once & call :usage & exit /B 1

        if /I "%~2"=="lld-link" set LINKER=lld-link
        if /I "%~2"=="link" set LINKER=link
        if "!LINKER!"=="" echo ERROR: Unrecognized/missing linker %~2 & call :usage & exit /B 1

        shift & shift
        goto :parse
    )

    echo ERROR: Unrecognized argument %~1
    call :usage
    exit /B 1

:execute
    :: Check for conflicting and associated arguments
    if "%GENERATOR%"=="msbuild" (
        if "%COMPILER%"=="clang" echo ERROR: Cannot use Clang with MSBuild & exit /B 1
        set COMPILER=msvc

        if "%BUILD_TYPE%" NEQ "" (
            echo ERROR: Cannot specify build type when using MSBuild as the generator & exit /B 1
        )
    )

    :: Defaults
    if "%COMPILER%"=="" set COMPILER=clang
    if "%GENERATOR%"=="" set GENERATOR=ninja
    if "%BUILD_TYPE%"=="" set BUILD_TYPE=debug
    if "%LINKER%"=="" set LINKER=lld-link

    :: CMake arguments
    set CMAKE_ARGS=
    if "%GENERATOR%"=="ninja" set CMAKE_ARGS=%CMAKE_ARGS% -G Ninja

    if "%COMPILER%"=="clang" set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
    if "%COMPILER%"=="msvc" set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl

    if "%LINKER%"=="lld-link" set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_LINKER=lld-link

    if "%GENERATOR%" NEQ "msbuild" (
        if "%BUILD_TYPE%"=="debug" set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_BUILD_TYPE=Debug
        if "%BUILD_TYPE%"=="release" set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_BUILD_TYPE=Release
        if "%BUILD_TYPE%"=="relwithdebinfo" set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_BUILD_TYPE=RelWithDebInfo
        if "%BUILD_TYPE%"=="minsizerel" set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_BUILD_TYPE=MinSizeRel
    )

    :: Figure out the platform
    if "%Platform%"=="" echo ERROR: The init script must be run from a Visual Studio command window & exit /B 1
    if "%Platform%"=="x86" (
        set BITNESS=32
        if %COMPILER%==clang set CFLAGS=-m32 & set CXXFLAGS=-m32
    )
    if "%Platform%"=="x64" set BITNESS=64
    if "%BITNESS%"=="" echo ERROR: Unrecognized/unsupported platform %Platform% & exit /B 1

    :: Set up the build directory
    set BUILD_DIR=%BUILD_ROOT%\%COMPILER%%BITNESS%%BUILD_TYPE%
    mkdir %BUILD_DIR% > NUL 2>&1

    :: Run CMake
    pushd %BUILD_DIR%
    echo Using compiler....... %COMPILER%
    echo Using architecture... %Platform%
    echo Using build type..... %BUILD_TYPE%
    echo Using linker......... %LINKER%
    echo Using build root..... %CD%
    echo.
    cmake %CMAKE_ARGS% ..\..
    popd

    goto :eof