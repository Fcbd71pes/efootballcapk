import paramiko
import os
import time

def deploy():
    host = '192.168.0.172'
    port = 8022
    username = 'u0_a334'
    password = '1234'

    print("Connecting to Termux...")
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, port, username, password)

    print("Creating project directory in Termux...")
    ssh.exec_command('mkdir -p ~/efootball')
    
    print("Uploading project files...")
    sftp = ssh.open_sftp()
    
    local_dir = r"d:\Desktop\efootball"
    for filename in os.listdir(local_dir):
        if filename.endswith('.py') or filename.endswith('.db') or filename == 'requirements.txt':
            local_path = os.path.join(local_dir, filename)
            remote_path = f"/data/data/com.termux/files/home/efootball/{filename}"
            print(f"Uploading {filename}...")
            sftp.put(local_path, remote_path)
    sftp.close()

    commands = [
        "pkg update -y && pkg upgrade -y",
        "pkg install python termux-api -y",
        "python -m ensurepip --upgrade",
        "termux-wake-lock",
        "cd ~/efootball && python -m pip install \"python-telegram-bot[job-queue]\" httpx aiohttp aiosqlite fastapi pydantic paramiko",
        "cd ~/efootball && nohup python main.py > bot.log 2>&1 &"
    ]

    for cmd in commands:
        print(f"Executing: {cmd}")
        stdin, stdout, stderr = ssh.exec_command(cmd)
        exit_status = stdout.channel.recv_exit_status()
        print(stdout.read().decode('utf-8', errors='ignore'))
        if exit_status != 0:
            print(f"Error: {stderr.read().decode('utf-8', errors='ignore')}")

    ssh.close()
    print("Deployment complete!")

if __name__ == '__main__':
    deploy()
