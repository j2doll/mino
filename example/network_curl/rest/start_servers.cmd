@echo off
chcp 65001 > nul
echo Starting all REST test servers (Ports: 20011 ~ 20019)...

start "GET Server (20011)" python get_server.py
start "POST Server (20012)" python post_server.py
start "PUT Server (20013)" python put_server.py
start "PATCH Server (20014)" python patch_server.py
start "DELETE Server (20015)" python delete_server.py
start "HEAD Server (20016)" python head_server.py
start "OPTIONS Server (20017)" python options_server.py
start "TRACE Server (20018)" python trace_server.py
start "CONNECT Server (20019)" python connect_server.py

echo.
echo All 9 servers have been started.
echo To stop all servers, run 'stop_servers.cmd' or close the opened console windows.
pause

