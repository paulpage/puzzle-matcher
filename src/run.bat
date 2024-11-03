call build.bat
if %errorlevel% neq 0 exit /b %errorlevel%
..\projects\VS2022\build\game\bin\x64\Debug\game.exe
