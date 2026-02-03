@echo off
setlocal enabledelayedexpansion

set "folderCount=0"
set "cppCount=0"
set "hppCount=0"
set "hCount=0"
set "lineCount=0"

cd QuasarEngine-Core\src

for /d /r %%d in (*) do (
    set /a folderCount+=1
)

for /r %%f in (*.cpp) do (
    set /a cppCount+=1
)

for /r %%f in (*.hpp) do (
    set /a hppCount+=1
)

for /r %%f in (*.h) do (
    set /a hCount+=1
)

for /r %%f in (*.cpp) do (
    for /f %%l in ('find /c /v "" ^< "%%f"') do (
        set /a lineCount+=%%l
    )
)

for /r %%f in (*.hpp) do (
    for /f %%l in ('find /c /v "" ^< "%%f"') do (
        set /a lineCount+=%%l
    )
)

for /r %%f in (*.h) do (
    for /f %%l in ('find /c /v "" ^< "%%f"') do (
        set /a lineCount+=%%l
    )
)

cd ..\..\QuasarEngine-Editor\src

for /d /r %%d in (*) do (
    set /a folderCount+=1
)

for /r %%f in (*.cpp) do (
    set /a cppCount+=1
)

for /r %%f in (*.hpp) do (
    set /a hppCount+=1
)

for /r %%f in (*.h) do (
    set /a hCount+=1
)

for /r %%f in (*.cpp) do (
    for /f %%l in ('find /c /v "" ^< "%%f"') do (
        set /a lineCount+=%%l
    )
)

for /r %%f in (*.hpp) do (
    for /f %%l in ('find /c /v "" ^< "%%f"') do (
        set /a lineCount+=%%l
    )
)

for /r %%f in (*.h) do (
    for /f %%l in ('find /c /v "" ^< "%%f"') do (
        set /a lineCount+=%%l
    )
)

echo -----------------
echo Total number of folders: %folderCount%
echo Total number of .cpp files: %cppCount%
echo Total number of .hpp files: %hppCount%
echo Total number of .h files: %hCount%
echo Total number of lines in .cpp, .hpp, and .h files: %lineCount%

pause
endlocal