#!/bin/bash
cp -r /sdcard/Download/efootball ~/
cd ~/efootball
python -m pip install "python-telegram-bot[job-queue]" httpx
nohup python main.py > bot.log 2>&1 &
echo "BOT DEPLOYMENT SUCCESSFUL!"
