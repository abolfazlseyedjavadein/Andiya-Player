#!/usr/bin/env python3
"""Exercise the installed runtime, both plugin helpers and the QML workspace."""
import argparse
import os
import subprocess
import sys
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stage", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    stage = args.stage.resolve()
    output = args.output.resolve()
    if output == ROOT or ROOT in output.parents:
        parser.error("Test output must be outside the source tree.")
    output.mkdir(parents=True, exist_ok=True)
    # Each verification run owns these generated paths; remove stale results so
    # capture counts and screenshots are deterministic.
    for generated in ("fixture.avi", "results.txt", "bypass.py", "crash.py", "hang.py"):
        (output / generated).unlink(missing_ok=True)
    for generated_dir in ("exports", "ui"):
        shutil.rmtree(output / generated_dir, ignore_errors=True)
    for old_log in output.glob("check-*.log"):
        old_log.unlink()
    runtime = stage / ("Andiya.app/Contents/MacOS" if sys.platform == "darwin" else
                       "opt/andiya/bin" if sys.platform.startswith("linux") else "bin")
    suffix = ".exe" if os.name == "nt" else ""
    app = runtime / ("Andiya" + suffix)
    worker = runtime / ("AndiyaFilterWorker" + suffix)
    validator = runtime / ("AndiyaPluginValidator" + suffix)
    for required in (app, worker, validator, runtime / "python_runtime/andiya_python_host.py",
                     runtime / "plugins/example.soft-contrast/plugin.json"):
        if not required.is_file():
            raise RuntimeError("Missing installed file: " + str(required))
    env = dict(os.environ)
    for key in ("QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QML_IMPORT_PATH", "QML2_IMPORT_PATH"):
        env.pop(key, None)
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["QT_QUICK_BACKEND"] = "software"
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    commands = [
        [sys.executable, "-B", ROOT / "tools/generate_test_video.py", output / "fixture.avi"],
        [app, "--self-test", output],
        [sys.executable, "-B", ROOT / "tests/test_plugin_tools.py", validator, worker, ROOT / "tools/package_plugin.py"],
        [app, "--ui-smoke", output / "ui"],
    ]
    for index, command in enumerate(commands):
        log = output / ("check-" + str(index) + ".log")
        with log.open("w", encoding="utf-8") as stream:
            result = subprocess.run([str(x) for x in command], env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=180)
        if result.returncode:
            print("--- Failed verification output: " + str(log) + " ---", file=sys.stderr)
            print("Exit code:", result.returncode, file=sys.stderr)
            print(log.read_text(encoding="utf-8", errors="replace")[-6000:], file=sys.stderr)
            results_file = output / "results.txt"
            if results_file.is_file():
                print(results_file.read_text(encoding="utf-8", errors="replace")[-6000:], file=sys.stderr)
            raise RuntimeError("Installed verification failed; inspect " + str(log))
    results = (output / "results.txt").read_text()
    if "ALL TESTS PASSED" not in results:
        raise RuntimeError(results)
    if len(list((output / "ui").glob("*.png"))) < 7:
        raise RuntimeError("The workspace smoke test did not produce all screenshots.")
    print(results)
    print("Installed plugin checks and seven UI snapshots passed:", output)

if __name__ == "__main__":
    main()
