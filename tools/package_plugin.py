"""Create a portable Andiya plugin bundle using Python's standard library.
Run: python tools/package_plugin.py PLUGIN_FOLDER OUTPUT.andiyaplugin
Use --hash-only before signing plugin.json; then --preserve-manifest to bundle.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path

def package(folder, output, hash_only=False, preserve=False):
    root = Path(folder).resolve()
    manifest_path = root / "plugin.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    files = {}
    total = 0
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            raise ValueError("Symbolic links are not allowed")
        if not path.is_file():
            continue
        if root not in path.resolve().parents:
            raise ValueError("File escapes plugin folder")
        relative = path.relative_to(root).as_posix()
        data = path.read_bytes()
        total += len(data)
        if len(files) >= 2000 or total > 40 * 1024 * 1024:
            raise ValueError("Package exceeds the portable bundle limit")
        files[relative] = data
    hashes = {name: hashlib.sha256(data).hexdigest() for name, data in files.items()
              if name not in ("plugin.json", "plugin.json.p7s")}
    if preserve:
        if manifest.get("files") != hashes:
            raise ValueError("Signed manifest hashes do not match package files")
    else:
        if "plugin.json.p7s" in files:
            raise ValueError("Use --preserve-manifest for an already signed package")
        manifest["files"] = hashes
        files["plugin.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    if hash_only:
        manifest_path.write_bytes(files["plugin.json"])
        return
    bundle = {"format": "andiya-plugin-bundle",
              "files": {name: base64.b64encode(data).decode("ascii") for name, data in files.items()}}
    destination = Path(output).resolve()
    if root == destination or root in destination.parents:
        raise ValueError("Write the bundle outside the plugin folder")
    destination.write_text(json.dumps(bundle, separators=(",", ":")), encoding="utf-8")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder")
    parser.add_argument("output", nargs="?")
    parser.add_argument("--hash-only", action="store_true")
    parser.add_argument("--preserve-manifest", action="store_true")
    args = parser.parse_args()
    if not args.hash_only and not args.output:
        parser.error("output is required unless --hash-only is used")
    package(args.folder, args.output, args.hash_only, args.preserve_manifest)
