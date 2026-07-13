#!/data/data/com.termux/files/usr/bin/bash
cd ~/efootball
echo "Installing all dependencies..."
pip install aiohttp aiosqlite fastapi httpx paramiko pydantic "python-telegram-bot[job-queue]"
echo "Starting bot..."
nohup python main.py > bot.log 2>&1 &
echo "Deployment complete!"
