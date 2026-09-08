"""Send known pixels through an actual Andiya worker; no GUI or third-party modules."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("runtime", choices=("python", "native"))
parser.add_argument("host", help="Python bridge script or native worker executable")
parser.add_argument("plugin", help="Python filter.py or native package folder")
args = parser.parse_args()
host = str(Path(args.host).resolve())
plugin = Path(args.plugin).resolve()
if args.runtime == "python":
    command = [sys.executable, "-B", host, str(plugin), '{"amount":1}']
else:
    manifest = json.loads((plugin / "plugin.json").read_text(encoding="utf-8"))
    command = [host, manifest["id"], str(plugin / manifest["entrypoint"]), "{}"]
pixels = bytes([0, 64, 255, 123, 255, 128, 32, 0])
request = struct.pack("<IIIIIqQ", 0x414E4652, 2, 1, 8, 1, 123456, len(pixels)) + pixels
result = subprocess.run(command, input=request, capture_output=True, timeout=15)
if result.returncode:
    raise SystemExit("Worker failed: " + result.stderr.decode(errors="replace"))
header = struct.Struct("<IiIIIIQ")
if len(result.stdout) < header.size:
    raise SystemExit("Missing worker response: " + result.stderr.decode(errors="replace"))
expected_header = (0x414E5253, 0, 2, 1, 8, 1, len(pixels))
if header.unpack(result.stdout[:header.size]) != expected_header:
    raise SystemExit("Unexpected worker response header")
if result.stdout[header.size:] != bytes([255, 191, 0, 123, 0, 127, 223, 0]):
    raise SystemExit("Unexpected pixels: inversion or alpha preservation failed")
print("PASS", args.runtime, "worker: inversion, alpha, dimensions and protocol")
