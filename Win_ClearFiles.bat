@echo off
setlocal

set filesDeleted=0
set foldersDeleted=0
set cleanupDone=0

for /r %%d in (*.sln *.vcxproj *.vcxproj.filters *.vcxproj.user) do (
    if exist "%%d" (
        del /q "%%d"
        set /a filesDeleted+=1
    )
)

for /d /r %%d in (bin, bin-int) do (
    echo "%%d" | findstr /I "\\.git\\" >nul
    if errorlevel 1 (
        if exist "%%d" (
            rmdir /s /q "%%d"
            set /a foldersDeleted+=1
        )
    )
)

if %filesDeleted% GTR 0 echo Deleted %filesDeleted% project files.
if %foldersDeleted% GTR 0 echo Deleted %foldersDeleted% folders.

if %filesDeleted% GTR 0 set cleanupDone=1
if %foldersDeleted% GTR 0 set cleanupDone=1

if defined cleanupDone (
    echo Cleanup completed.
    pause
)
