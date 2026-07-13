import paramiko

host = '127.0.0.1'
port = 8022
username = 'u0_a334'
password = '1234'

ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
ssh.connect(host, port, username, password)

print("Started tailing admin_bot.log...")
stdin, stdout, stderr = ssh.exec_command('tail -n 200 -f ~/efootball/admin_bot.log')

with open('pc_admin.log', 'w', encoding='utf-8') as f:
    for line in iter(stdout.readline, ""):
        f.write(line)
        f.flush()
