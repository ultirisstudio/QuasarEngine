@echo off
setlocal enabledelayedexpansion

set GLSLC="C:\VulkanSDK\1.3.296.0\Bin\glslc.exe"
set SHADER_DIR=QuasarEngine-Editor\Assets\Shaders
set OUT_DIR=QuasarEngine-Editor\Assets\Shaders

if not exist %OUT_DIR% (
    mkdir %OUT_DIR%
)

echo ----------------------------
echo  Compilation des shaders...
echo ----------------------------

for %%F in (%SHADER_DIR%\*.glsl) do (
    call :compile_one "%%F"
)

echo ----------------------------
echo    Compilation terminee
echo ----------------------------

pause
exit /b

:compile_one
set "FULLPATH=%~1"
set "FILE=%~nx1"
set "STAGE="

echo %FILE% | findstr /I ".vert." >nul && set STAGE=vertex
echo %FILE% | findstr /I ".frag." >nul && set STAGE=fragment
echo %FILE% | findstr /I ".comp." >nul && set STAGE=compute
echo %FILE% | findstr /I ".geom." >nul && set STAGE=geometry
echo %FILE% | findstr /I ".tesc." >nul && set STAGE=tesscontrol
echo %FILE% | findstr /I ".tese." >nul && set STAGE=tesseval

if not defined STAGE (
    echo Impossible de determiner le type de shader pour %FILE%
    goto :eof
)

set "OUTFILE=%FILE:.glsl=.spv%"

echo Compilation de %FILE% (type: %STAGE%)
%GLSLC% -fshader-stage=%STAGE% "%FULLPATH%" -o "%OUT_DIR%\%OUTFILE%"
if errorlevel 1 (
    echo Erreur de compilation: %FILE%
) else (
    echo Fichier compile: %OUT_DIR%\%OUTFILE%
)
goto :eof
