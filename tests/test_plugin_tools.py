"""Regression checks for plugin packaging, validation and protocol rejection."""
import argparse
import importlib.util
import json
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

def run(validator, worker, toolkit):
    spec = importlib.util.spec_from_file_location("packager", toolkit)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    with tempfile.TemporaryDirectory(prefix="andiya-validation-") as temporary:
        root = Path(temporary)
        plugin = root / "plugin"
        plugin.mkdir()
        manifest = {"id": "test.validation", "name": "Validation test", "version": "1.0.0",
                    "andiyaApi": "1.0", "execution": "python", "runtime": "process",
                    "entrypoint": "filter.py", "capabilities": ["image-filter"],
                    "permissions": ["frame-access"]}
        (plugin / "plugin.json").write_text(json.dumps(manifest))
        (plugin / "filter.py").write_text("def process_frame(*args): return args[-1]\n")
        def validate(expected):
            result = subprocess.run([validator, str(plugin)], capture_output=True, timeout=10)
            assert (result.returncode == 0) == expected, result.stdout.decode()
            return json.loads(result.stdout)
        validate(True)
        module.package(plugin, root / "sample.andiyaplugin")
        bundle = json.loads((root / "sample.andiyaplugin").read_text())
        assert bundle["format"] == "andiya-plugin-bundle"
        module.package(plugin, None, hash_only=True)
        before = validate(True)["fingerprint"]
        (plugin / "filter.py").write_text("tampered")
        validate(False)
        (plugin / "filter.py").write_text("def process_frame(*args): return args[-1]\n")
        assert validate(True)["fingerprint"] == before
        (plugin / "plugin.json.p7s").write_bytes(b"invalid signature")
        validate(False)
        (plugin / "plugin.json.p7s").unlink()
        manifest["entrypoint"] = "../outside.py"
        (root / "outside.py").write_text("raise RuntimeError('must not run')")
        (plugin / "plugin.json").write_text(json.dumps(manifest))
        validate(False)
        manifest["entrypoint"] = "filter.py"
        manifest["parameters"] = [{"key": "bad", "min": 2, "max": 1, "default": 1}]
        (plugin / "plugin.json").write_text(json.dumps(manifest))
        validate(False)
        manifest.pop("parameters")
        manifest["andiyaApi"] = "9.0"
        (plugin / "plugin.json").write_text(json.dumps(manifest))
        validate(False)
        # The worker must reject an invalid frame header without allocating its payload.
        folder = Path(worker).parent / "plugins/example.soft-contrast"
        native = folder / json.loads((folder / "plugin.json").read_text())["entrypoint"]
        assert native.is_file(), native
        result = subprocess.run([worker, "example.soft-contrast", str(native), "{}"],
                                input=struct.pack("<IIIIIqQ", 0x414e4652, 16385, 1, 4, 1, 0, 4),
                                capture_output=True, timeout=10)
        assert result.returncode != 0
        print("PASS package round trip, content fingerprint, tamper rejection, bad signature, path traversal, parameter/API validation, worker frame bounds")
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("validator")
    parser.add_argument("worker")
    parser.add_argument("packager")
    args = parser.parse_args()
    run(args.validator, args.worker, args.packager)
