@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SHADER_DIR=QuasarEngine-Editor\Assets\Shaders\vk"
set "OUT_DIR=QuasarEngine-Editor\Assets\Shaders\vk\spv"

set "GLSLC="

if defined VULKAN_SDK (
    set "GLSLC=%VULKAN_SDK%\Bin\glslc.exe"
)

if not exist "%GLSLC%" (
    for /f "delims=" %%I in ('where glslc 2^>nul') do (
        set "GLSLC=%%~fI"
        goto :glslc_found
    )
) else (
    goto :glslc_found
)

:glslc_not_found
echo [ERROR] Could not find "glslc.exe".
echo - Make sure the VULKAN_SDK variable is set (e.g. C:\VulkanSDK\1.3.xxx.x)
echo   OR that glslc is available in PATH.
exit /b 1

:glslc_found
if not exist "%GLSLC%" goto :glslc_not_found

if not exist "%OUT_DIR%" (
    mkdir "%OUT_DIR%" 2>nul
)

echo ----------------------------
echo  Compiling shaders...
echo ----------------------------

for %%F in ("%SHADER_DIR%\*.glsl") do (
    call :compile_one "%%~fF"
)

echo ----------------------------
echo  Compilation finished
echo ----------------------------

pause

:compile_one
set "FULLPATH=%~1"
set "FILE=%~nx1"
set "STAGE="

echo(!FILE!| findstr /I "\.vert\." >nul && set "STAGE=vertex"
echo(!FILE!| findstr /I "\.frag\." >nul && set "STAGE=fragment"
echo(!FILE!| findstr /I "\.comp\." >nul && set "STAGE=compute"
echo(!FILE!| findstr /I "\.geom\." >nul && set "STAGE=geometry"
echo(!FILE!| findstr /I "\.tesc\." >nul && set "STAGE=tesscontrol"
echo(!FILE!| findstr /I "\.tese\." >nul && set "STAGE=tesseval"

if not defined STAGE (
    echo [WARNING] Could not determine shader type for "%FILE%". Skipped.
	echo Expected: *.vert.glsl / *.frag.glsl / *.comp.glsl / *.geom.glsl / *.tesc.glsl / *.tese.glsl
    goto :eof
)

set "OUTFILE=%FILE:.glsl=.spv%"

echo Compiling "%FILE%" (type: %STAGE%)
"%GLSLC%" -fshader-stage=%STAGE% "%FULLPATH%" -o "%OUT_DIR%\%OUTFILE%"
if errorlevel 1 (
    echo [ERROR] Failed to compile: "%FILE%"
) else (
    echo [OK] Compiled: "%OUT_DIR%\%OUTFILE%"
)
goto :eof
