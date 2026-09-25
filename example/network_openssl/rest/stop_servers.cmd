@echo off
echo Stopping all REST test servers (Ports: 20011 ~ 20019)...

for /L %%P in (20011,1,20019) do (
    for /f "tokens=5" %%a in ('netstat -aon ^| findstr :%%P ^| findstr LISTENING') do (
        taskkill /F /PID %%a >nul 2>&1
    )
)

echo All servers on ports 20011-20019 have been stopped.
pause

