#!/usr/bin/env python3
"""Boot NitrOS-9 on exec09 multicomp09, answer Time?, reach OS9: shell.

Usage:
  ./boot-to-shell.py              # auto date 2026/08/09 12:00:00
  ./boot-to-shell.py interactive  # you type at Time? (still via this PTY)
"""
from __future__ import annotations

import os
import pty
import re
import select
import sys
import time

RT = os.path.dirname(os.path.abspath(__file__))
RUN = os.path.expanduser(os.environ.get("M6809_RUN", "~/6809/exec09/m6809-run"))
DATE = os.environ.get("NITROS9_DATE", "2026/08/09 12:00:00")


def main() -> int:
    interactive = len(sys.argv) > 1 and sys.argv[1].startswith("int")
    os.chdir(RT)
    with open("multicomp09.bat", "wb") as f:
        f.write(b"NITROS9\r")

    pid, fd = pty.fork()
    if pid == 0:
        os.execv(RUN, [RUN, "-s", "multicomp09", "-b", "6809M.bin", "-I", "20000"])
        os._exit(127)

    buf = b""
    sent = False
    deadline = time.time() + 120
    try:
        while time.time() < deadline:
            r, _, _ = select.select([fd], [], [], 0.5)
            if fd not in r:
                continue
            try:
                chunk = os.read(fd, 4096)
            except OSError:
                break
            if not chunk:
                break
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
            buf += chunk
            text = buf.decode("latin1", "replace")
            if (not sent) and "Time ?" in text:
                time.sleep(0.5)
                if interactive:
                    print("\n[host] type date as yyyy/mm/dd hh:mm:ss then ENTER\n", flush=True)
                    line = sys.stdin.readline()
                    if not line.endswith("\r") and not line.endswith("\n"):
                        line += "\r"
                    payload = line.replace("\n", "\r").encode("ascii", "replace")
                else:
                    payload = (DATE + "\r").encode("ascii")
                for ch in payload:
                    os.write(fd, bytes([ch]))
                    time.sleep(0.03)
                sent = True
                print(f"\n[host] submitted {payload!r}\n", flush=True)
            if sent and "OS9:" in text:
                # drain a moment for prompt stability
                end = time.time() + 2
                while time.time() < end:
                    r2, _, _ = select.select([fd], [], [], 0.2)
                    if fd in r2:
                        try:
                            chunk = os.read(fd, 4096)
                        except OSError:
                            break
                        if not chunk:
                            break
                        sys.stdout.buffer.write(chunk)
                        sys.stdout.buffer.flush()
                        buf += chunk
                break
    finally:
        try:
            os.kill(pid, 9)
            os.waitpid(pid, 0)
        except Exception:
            pass
        try:
            os.close(fd)
        except Exception:
            pass

    text = re.sub(r"\x1b\[[0-9;]*m", "", buf.decode("latin1", "replace"))
    ok = ("Time ?" in text) and sent and ("OS9:" in text or "Shell" in text)
    print("\n===== RESULT =====")
    print("Time?:", "Time ?" in text)
    print("input sent:", sent)
    print("Shell:", "Shell" in text)
    print("OS9:", "OS9:" in text)
    print("OK:" if ok else "FAIL:", ok)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
