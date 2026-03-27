#!/usr/bin/env python3
"""
deploy_example.py - Simplified remote build & deploy via SSH/SFTP

Educational/portfolio example showing the Paramiko-based deployment
pattern used to cross-compile on a remote Linux build server from
a Windows development machine.

All credentials, IPs, and paths are placeholder values.
"""

import paramiko
import os
import sys

# --- Configuration (all values are REDACTED placeholders) ---
REMOTE_HOST = "REDACTED_HOST"
REMOTE_USER = "REDACTED_USER"
REMOTE_PASS = "REDACTED_PASS"
REMOTE_SRC  = "/home/user/project/src"
REMOTE_BUILD = "/home/user/project/build"
REMOTE_DEPLOY = "/home/user/server/bin"
BINARY_NAME = "GameServer"


def ssh_exec(client: paramiko.SSHClient, cmd: str) -> str:
    """Execute a command over SSH and return stdout. Raises on failure."""
    stdin, stdout, stderr = client.exec_command(cmd)
    exit_code = stdout.channel.recv_exit_status()
    output = stdout.read().decode()
    if exit_code != 0:
        err = stderr.read().decode()
        raise RuntimeError(f"Command failed ({exit_code}): {cmd}\n{err}")
    return output


def deploy(local_files: list[str]):
    """Upload changed source files, build remotely, and deploy."""
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(REMOTE_HOST, username=REMOTE_USER, password=REMOTE_PASS)

    try:
        # Step 1: Upload modified source files via SFTP
        sftp = client.open_sftp()
        for local_path in local_files:
            filename = os.path.basename(local_path)
            remote_path = f"{REMOTE_SRC}/{filename}"
            sftp.put(local_path, remote_path)
            print(f"  Uploaded: {filename}")
        sftp.close()

        # Step 2: Build on remote machine
        print("  Building...")
        ssh_exec(client, f"cd {REMOTE_BUILD} && cmake .. -DCMAKE_BUILD_TYPE=Release")
        output = ssh_exec(client, f"cd {REMOTE_BUILD} && make -j$(nproc) {BINARY_NAME}")
        print(f"  Build OK ({output.count('Linking')} targets linked)")

        # Step 3: Deploy binary to runtime directory
        ssh_exec(client, f"cp {REMOTE_BUILD}/bin/{BINARY_NAME} {REMOTE_DEPLOY}/{BINARY_NAME}")
        print(f"  Deployed to {REMOTE_DEPLOY}")

        # Step 4: Verify the binary exists and is recent
        stat = ssh_exec(client, f"stat --format='%s bytes, %y' {REMOTE_DEPLOY}/{BINARY_NAME}")
        print(f"  Verified: {stat.strip()}")

    finally:
        client.close()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} file1.cpp [file2.cpp ...]")
        sys.exit(1)
    deploy(sys.argv[1:])
