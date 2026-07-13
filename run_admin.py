import paramiko

def run_admin():
    host = '127.0.0.1'
    port = 8022
    username = 'u0_a334'
    password = '1234'

    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(host, port, username, password)

    cmd = "cd ~/efootball && nohup python admin_bot.py > admin_bot.log 2>&1 &"
    print(f"Executing: {cmd}")
    stdin, stdout, stderr = ssh.exec_command(cmd)
    
    # We don't wait for exit status because nohup might hang the session
    # Just close the connection
    ssh.close()
    print("Started admin_bot.py!")

if __name__ == '__main__':
    run_admin()
