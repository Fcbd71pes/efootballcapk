#!/data/data/com.termux/files/usr/bin/bash
exec > /sdcard/deploy.log 2>&1
cd ~
echo "Cleaning all files in home directory..."
rm -rf ~/*
mkdir -p ~/efootball
cp -r /sdcard/efootball/* ~/efootball/
cd ~/efootball
echo "Installing dependencies..."
pip install aiohttp aiosqlite fastapi httpx paramiko pydantic "python-telegram-bot[job-queue]"
echo "Starting bot..."
nohup python main.py > bot.log 2>&1 &
echo "Deployment successful!"
