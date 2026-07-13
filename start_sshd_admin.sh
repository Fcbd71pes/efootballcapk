#!/data/data/com.termux/files/usr/bin/bash
echo "Starting sshd..."
sshd
cd ~/efootball
echo "Starting admin_bot..."
nohup python admin_bot.py > admin_bot.log 2>&1 &
echo "Done!"
