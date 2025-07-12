@echo off

set url="https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-windows.zip"

mkdir temp\

call curl -A "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64)" -L %url% -o temp\premake-5.0.0-beta2-windows.zip

cd temp

tar -xf premake-5.0.0-beta2-windows.zip

cd ../

xcopy /s temp\premake5.exe vendor\bin\premake\
xcopy /s temp\premake5.exe QuasarEngine-Editor\Assets\premake\

rmdir /s /q temp\

PAUSE