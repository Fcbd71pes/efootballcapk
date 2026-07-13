@echo off
set ADB="%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe"

%ADB% -s 192.168.0.172:5555 shell input text "cp%%s-r%%s/sdcard/Download/efootball%%s~/"
%ADB% -s 192.168.0.172:5555 shell input keyevent 66

timeout /t 2 >nul

%ADB% -s 192.168.0.172:5555 shell input text "cd%%s~/efootball"
%ADB% -s 192.168.0.172:5555 shell input keyevent 66

timeout /t 2 >nul

%ADB% -s 192.168.0.172:5555 shell input text "python%%s-m%%spip%%sinstall%%shttpx%%spython-telegram-bot"
%ADB% -s 192.168.0.172:5555 shell input keyevent 66

timeout /t 10 >nul

%ADB% -s 192.168.0.172:5555 shell input text "nohup%%spython%%smain.py%%s>%%sbot.log%%s2>&1%%s&"
%ADB% -s 192.168.0.172:5555 shell input keyevent 66
