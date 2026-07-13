#!/data/data/com.termux/files/usr/bin/bash
if pgrep -f "python main.py" > /dev/null; then
  echo "RUNNING" > /sdcard/check_result.txt
else
  echo "NOT_RUNNING" > /sdcard/check_result.txt
fi
cp ~/efootball/bot.log /sdcard/bot.log || echo "NO_LOG" > /sdcard/bot.log
