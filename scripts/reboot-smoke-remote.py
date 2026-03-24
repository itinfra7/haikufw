#!/usr/bin/env python3
import argparse
import subprocess
import sys
import time


def ssh_base(host: str, password_file: str):
    return [
        "sshpass", "-f", password_file,
        "ssh",
        "-o", "StrictHostKeyChecking=no",
        "-o", "UserKnownHostsFile=/dev/null",
        "-o", "ConnectTimeout=5",
        f"user@{host}",
    ]


def run_remote(base, command: str, timeout: int = 20):
    return subprocess.run(base + [command], capture_output=True, text=True, timeout=timeout)


def ssh_up(base) -> bool:
    try:
        p = subprocess.run(base + ["echo up"], capture_output=True, text=True, timeout=8)
        return p.returncode == 0 and "up" in p.stdout
    except Exception:
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Remote Haiku reboot smoke for haikufw")
    parser.add_argument("--host", required=True)
    parser.add_argument("--password-file", required=True)
    parser.add_argument("--iterations", type=int, default=6)
    parser.add_argument("--settle-seconds", type=int, default=30)
    args = parser.parse_args()

    base = ssh_base(args.host, args.password_file)
    logs = []

    for i in range(1, args.iterations + 1):
        logs.append(f"ITER {i} PRECHECK")
        pre = run_remote(
            base,
            "zerotier-cli info; ls /dev/misc/haikufw; "
            "/boot/system/non-packaged/bin/haikufwctl status",
            timeout=30,
        )
        logs.append(pre.stdout.strip())
        if pre.returncode != 0 or "ONLINE" not in pre.stdout:
            print("\n".join(logs))
            return 10

        logs.append(f"ITER {i} REBOOT")
        reboot = run_remote(base, "shutdown -r -d 0", timeout=15)
        logs.append(f"reboot rc={reboot.returncode}")

        down = False
        for sec in range(1, 61):
            if not ssh_up(base):
                logs.append(f"ssh-down-at={sec}")
                down = True
                break
            time.sleep(1)
        if not down:
            logs.append("ssh never went down")

        up_at = None
        for sec in range(1, 121):
            if ssh_up(base):
                up_at = sec
                logs.append(f"ssh-up-at={sec}")
                break
            time.sleep(1)
        if up_at is None:
            logs.append("ssh did not return")
            print("\n".join(logs))
            return 20

        time.sleep(args.settle_seconds)

        post = run_remote(
            base,
            "uptime; zerotier-cli info; ls /dev/misc/haikufw; "
            "/boot/system/non-packaged/bin/haikufwctl status",
            timeout=40,
        )
        logs.append(f"POST rc={post.returncode}")
        logs.append(post.stdout.strip())
        if (
            post.returncode != 0
            or "ONLINE" not in post.stdout
            or "/dev/misc/haikufw" not in post.stdout
        ):
            print("\n".join(logs))
            return 30

    print("\n".join(logs))
    return 0


if __name__ == "__main__":
    sys.exit(main())
