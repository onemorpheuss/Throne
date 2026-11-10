#!/usr/bin/env python3
"""Offline micro-benchmark for large profile imports and subscription refreshes.

Starts an isolated headless instance with resume and kill-switch disabled. The
HTTP fixture and API listen on loopback only; no profile is ever connected or
latency-tested.
"""

import argparse
import json
import os
from pathlib import Path
import socket
import statistics
import subprocess
import tempfile
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


def free_port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--thronectl", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, required=True)
    parser.add_argument("--profiles", type=int, default=5000)
    parser.add_argument("--runs", type=int, default=3)
    parser.add_argument("--label", default="current")
    parser.add_argument("--msys2-root", type=Path)
    args = parser.parse_args()
    if args.profiles < 1 or args.runs < 1:
        parser.error("--profiles and --runs must be positive")

    exe = args.exe.resolve()
    ctl = args.thronectl.resolve()
    work_dir = args.work_dir.resolve()
    work_dir.mkdir(parents=True, exist_ok=True)
    body = "\n".join(
        f"socks5://127.0.0.1:1080#bench-{i:05d}" for i in range(args.profiles)
    ).encode()

    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path != "/subscription":
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, *_):
            pass

    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    results = []
    try:
        for run_index in range(args.runs):
            with tempfile.TemporaryDirectory(prefix=f"perf-{run_index}-", dir=work_dir) as temp:
                data_dir = Path(temp) / "data"
                api_port = free_port()
                address = f"127.0.0.1:{api_port}"
                key_file = data_dir / "config" / "api.key"
                env = os.environ.copy()
                env["THRONE_ADDR"] = address
                env["THRONE_KEY_FILE"] = str(key_file)
                if args.msys2_root:
                    env["PATH"] = str(args.msys2_root.resolve() / "ucrt64" / "bin") + os.pathsep + env.get("PATH", "")
                process = None
                try:
                    started = time.perf_counter()
                    process = subprocess.Popen(
                        [str(exe), "--headless", "--data-dir", str(data_dir),
                         "--api-listen", address, "--no-resume", "--no-kill-switch"],
                        cwd=exe.parent, env=env,
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                    )
                    deadline = time.monotonic() + 30
                    while True:
                        if process.poll() is not None:
                            raise RuntimeError(f"Throne exited with {process.returncode}")
                        if key_file.exists():
                            try:
                                probe = subprocess.run(
                                    [str(ctl), "--json", "status"], env=env,
                                    capture_output=True, text=True, timeout=3,
                                )
                                if probe.returncode == 0:
                                    break
                            except subprocess.TimeoutExpired:
                                pass
                        if time.monotonic() >= deadline:
                            raise RuntimeError("Timed out waiting for the local API")
                        time.sleep(0.1)
                    startup = time.perf_counter() - started

                    def command(*arguments, stdin=None):
                        return subprocess.run(
                            [str(ctl), "--json", *arguments], input=stdin,
                            env=env, capture_output=True, text=True, timeout=180,
                            check=True,
                        )

                    command("settings", "set", "sub_auto_update=0")
                    timings = {"startup_ms": startup * 1000}
                    for name in ("import_first", "import_repeat"):
                        started = time.perf_counter()
                        imported = json.loads(command("import", "-", stdin=body.decode()).stdout)
                        timings[name + "_ms"] = (time.perf_counter() - started) * 1000
                        if imported["imported"] != args.profiles:
                            raise RuntimeError(f"{name}: unexpected import count {imported['imported']}")
                        profiles = json.loads(command("profiles").stdout)["profiles"]
                        direct = sorted(p["id"] for p in profiles if p["name"].startswith("bench-"))
                        expected_total = args.profiles if name == "import_first" else 2 * args.profiles
                        if len(profiles) != expected_total or len(direct) != expected_total:
                            raise RuntimeError(f"{name}: unexpected profile count {len(profiles)}")

                    url = f"http://127.0.0.1:{server.server_port}/subscription"
                    started = time.perf_counter()
                    group = json.loads(command("sub", "add", url, "--name", "bench-sub").stdout)["group"]
                    timings["subscription_add_ms"] = (time.perf_counter() - started) * 1000
                    if group["profile_count"] != args.profiles:
                        raise RuntimeError("Unexpected subscription membership")
                    gid = group["id"]
                    profiles = json.loads(command("profiles").stdout)["profiles"]
                    sub_ids = sorted(p["id"] for p in profiles if p["group"] == gid)
                    if len(profiles) != 3 * args.profiles or len(sub_ids) != args.profiles:
                        raise RuntimeError("Unexpected combined profile count")

                    started = time.perf_counter()
                    updated = json.loads(command("sub", "update", str(gid)).stdout)
                    timings["subscription_update_ms"] = (time.perf_counter() - started) * 1000
                    if not updated["results"][0]["ok"]:
                        raise RuntimeError("Subscription refresh failed")
                    profiles = json.loads(command("profiles").stdout)["profiles"]
                    if sorted(p["id"] for p in profiles if p["group"] == gid) != sub_ids:
                        raise RuntimeError("Repeated subscription refresh changed profile IDs")
                    if len(profiles) != 3 * args.profiles:
                        raise RuntimeError("Repeated subscription refresh duplicated profiles")
                    results.append(timings)
                finally:
                    if process is not None:
                        if process.poll() is None and key_file.exists():
                            try:
                                subprocess.run([str(ctl), "shutdown", "--yes"], env=env,
                                               capture_output=True, timeout=20)
                            except (OSError, subprocess.TimeoutExpired):
                                pass
                        try:
                            process.wait(timeout=10)
                        except subprocess.TimeoutExpired:
                            process.terminate()
                            try:
                                process.wait(timeout=10)
                            except subprocess.TimeoutExpired:
                                process.kill()
                                process.wait()
    finally:
        server.shutdown()
        server.server_close()

    summary = {"label": args.label, "profiles": args.profiles, "runs": results}
    for key in results[0]:
        summary[key + "_median"] = statistics.median(run[key] for run in results)
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
