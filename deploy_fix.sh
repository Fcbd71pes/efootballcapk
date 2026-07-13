#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/deploy3.log 2>&1
cd ~/efootball
echo "Installing dependencies..."
pip install aiohttp aiosqlite httpx "python-telegram-bot[job-queue]"
echo "Starting bot..."
nohup python main.py > bot.log 2>&1 &
echo "Deployment successful!"
