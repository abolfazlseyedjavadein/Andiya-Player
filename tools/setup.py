#!/usr/bin/env python3
"""Configure, build, install and optionally package Andiya on its native desktop OS."""
import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def run(command, **kwargs):
    print("+", subprocess.list2cmdline([str(x) for x in command]), flush=True)
    subprocess.run([str(x) for x in command], check=True, **kwargs)

def outside_source(path):
    path = Path(path).expanduser().resolve()
    if path == ROOT or ROOT in path.parents:
        raise ValueError("Keep builds, SDKs and packages outside the source tree.")
    return path

def nsis_quote(value):
    return str(value).replace("$", "$$").replace('"', '$\\"')

def windows_installer(stage, build, version, name, compiler):
    compiler = compiler or shutil.which("makensis")
    if not compiler:
        raise ValueError("NSIS is required for setup.exe. Install NSIS 3.03+ or pass --nsis PATH.")
    output = build / "packages" / (name + "-setup.exe")
    # Only remove shipped paths when uninstalling, never recursively delete the user's directory.
    instructions = []
    paths = sorted(stage.rglob("*"))
    for path in paths:
        if path.is_file():
            relative = nsis_quote(str(path.relative_to(stage)).replace("/", "\\\\"))
            instructions.append('Delete "$INSTDIR\\\\' + relative + '"')
    for path in sorted((p for p in paths if p.is_dir()), key=lambda p: len(p.parts), reverse=True):
        relative = nsis_quote(str(path.relative_to(stage)).replace("/", "\\\\"))
        instructions.append('RMDir "$INSTDIR\\\\' + relative + '"')
    include = build / "uninstall-files.nsh"
    include.write_text("\n".join(instructions) + "\n", encoding="utf-8")
    run([compiler, "/V2", "/DSTAGE=" + str(stage), "/DOUTPUT=" + str(output),
         "/DICON=" + str(ROOT / "assets/andiya.ico"), "/DLICENSE=" + str(ROOT / "LICENSE"),
         "/DVERSION=" + version, "/DUNINSTALL_FILES=" + str(include),
         ROOT / "packaging/windows/Andiya.nsi"])

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qt-prefix", default=os.environ.get("QT_ROOT_DIR"),
                        help="Qt 6.8+ desktop kit directory (or set QT_ROOT_DIR)")
    parser.add_argument("--build-dir", default=str(ROOT.parent / "Andiya-local" / ("build-" + platform.system().lower())))
    parser.add_argument("--generator", help="CMake generator; defaults to VS 2022 on Windows, Ninja elsewhere")
    parser.add_argument("--jobs", type=int, default=min(os.cpu_count() or 2, 4))
    parser.add_argument("--package", action="store_true", help="Create native installers/archives after testing the installed app")
    parser.add_argument("--verify", action="store_true", help="Test the installed copy; automatic with --package")
    parser.add_argument("--archive-only", action="store_true", help="Windows: create ZIP without an NSIS installer")
    parser.add_argument("--nsis", help="Path to makensis.exe")
    args = parser.parse_args(argv)
    if platform.system() not in ("Windows", "Darwin", "Linux"):
        parser.error("This project currently targets Windows, macOS and Linux desktops.")
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    build = outside_source(args.build_dir)
    for tool in ("cmake", "cpack"):
        if not shutil.which(tool):
            parser.error(tool + " is missing. See docs/BUILDING.md.")
    if args.package and os.name == "nt" and not args.archive_only and not (args.nsis or shutil.which("makensis")):
        parser.error("Install NSIS or pass --nsis PATH; use --archive-only to create just the ZIP.")
    generator = args.generator or ("Visual Studio 17 2022" if os.name == "nt" else "Ninja")
    command = ["cmake", "-S", ROOT, "-B", build, "-G", generator,
               "-DCMAKE_BUILD_TYPE=Release", "-DANDIYA_DEPLOY_RUNTIME=ON",
               "-DCMAKE_INSTALL_LIBDIR=lib"]
    if os.name == "nt" and generator.startswith("Visual Studio"):
        command += ["-A", "x64"]
    if args.qt_prefix:
        qt = Path(args.qt_prefix).expanduser().resolve()
        if not (qt / "lib/cmake/Qt6/Qt6Config.cmake").is_file():
            parser.error("--qt-prefix must point to a complete Qt desktop kit.")
        command.append("-DCMAKE_PREFIX_PATH=" + qt.as_posix())
    build.mkdir(parents=True, exist_ok=True)
    run(command)
    run(["cmake", "--build", build, "--config", "Release", "--parallel", args.jobs])
    stage = build / "stage"
    # Refuse to silently carry stale files into an installer. Rename the old stage for inspection.
    if stage.exists():
        backup = build / ("stage-previous-" + str(__import__("time").time_ns()))
        stage.rename(backup)
    stage.mkdir()
    prefix = stage / "opt/andiya" if sys.platform.startswith("linux") else stage
    run(["cmake", "--install", build, "--config", "Release", "--prefix", prefix])
    if sys.platform.startswith("linux"):
        desktop = stage / "usr/share/applications/io.andiya.player.desktop"
        desktop.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / "packaging/linux/io.andiya.player.desktop", desktop)
        icon = stage / "usr/share/icons/hicolor/256x256/apps/io.andiya.player.png"
        icon.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / "assets/andiya-256.png", icon)
    if sys.platform == "darwin":
        # Plugin package folders are data, not nested application bundles.
        # Keep them out of Contents/MacOS so codesign does not parse dotted
        # plugin IDs as bundle names. Internal links preserve runtime lookup.
        contents = stage / "Andiya.app/Contents"
        resources = contents / "Resources"
        resources.mkdir(exist_ok=True)
        for name in ("plugins", "python_runtime"):
            source = contents / "MacOS" / name
            shutil.move(str(source), str(resources / name))
            source.symlink_to("../Resources/" + name, target_is_directory=True)
        for library in (resources / "plugins").rglob("*.dylib"):
            run(["codesign", "--force", "--sign", "-", library])
    if args.verify or args.package:
        # qoffscreen is a test-only Qt plugin and is not included in release archives.
        if args.qt_prefix:
            test_platform = Path(args.qt_prefix) / "plugins" / "platforms" / "qoffscreen.dll"
            if sys.platform == "darwin":
                test_platform = Path(args.qt_prefix) / "plugins" / "platforms" / "libqoffscreen.dylib"
            elif sys.platform.startswith("linux"):
                test_platform = Path(args.qt_prefix) / "plugins" / "platforms" / "libqoffscreen.so"
            if test_platform.is_file():
                destination = stage / ("Andiya.app/Contents/PlugIns/platforms" if sys.platform == "darwin" else
                                       "opt/andiya/plugins/platforms" if sys.platform.startswith("linux") else
                                       "plugins/platforms")
                destination.mkdir(parents=True, exist_ok=True)
                shutil.copy2(test_platform, destination / test_platform.name)
        run([sys.executable, "-B", ROOT / "tools/verify_install.py", stage, "--output", build / "verification"])
        if args.package:
            test_copy = stage / ("Andiya.app/Contents/PlugIns/platforms" if sys.platform == "darwin" else
                                 "opt/andiya/plugins/platforms" if sys.platform.startswith("linux") else
                                 "plugins/platforms")
            for test_file in test_copy.glob("*qoffscreen*"):
                test_file.unlink()
    if sys.platform == "darwin" and not os.environ.get("CI"):
        # Local/ad-hoc identity only. Public distribution requires a maintainer's signing identity.
        run(["codesign", "--force", "--deep", "--sign", "-", stage / "Andiya.app"])
        run(["codesign", "--verify", "--deep", "--strict", stage / "Andiya.app"])
    if args.package:
        config = build / "PackageInstalled.cmake"
        # Package exactly the installed, verified tree (including macOS signatures).
        config.write_text('include("' + (build / "CPackConfig.cmake").as_posix() + '")\n'
                          'set(CPACK_INSTALL_CMAKE_PROJECTS "")\n'
                          'set(CPACK_INSTALL_SCRIPTS "")\n'
                          'set(CPACK_PACKAGING_INSTALL_PREFIX "/")\n'
                          'set(CPACK_INSTALLED_DIRECTORIES "' + stage.as_posix() + ';/")\n',
                          encoding="utf-8")
        run(["cpack", "--config", config, "-C", "Release"], cwd=build)
        cache = (build / "CPackConfig.cmake").read_text()
        import re
        version = re.search(r'set\(CPACK_PACKAGE_VERSION "([^"]+)"\)', cache).group(1)
        name = re.search(r'set\(CPACK_PACKAGE_FILE_NAME "([^"]+)"\)', cache).group(1)
        if os.name == "nt" and not args.archive_only:
            windows_installer(stage, build, version, name, args.nsis)
        artifacts = [p for p in (build / "packages").iterdir()
                     if p.is_file() and p.name.startswith(name) and not p.name.endswith(".sha256")]
        for artifact in artifacts:
            digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
            artifact.with_name(artifact.name + ".sha256").write_text(digest + "  " + artifact.name + "\n")
        print("Packages:", build / "packages")
    print("Installed app:", prefix / ("Andiya.app" if sys.platform == "darwin" else "bin"))

if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print("Setup failed:", error, file=sys.stderr)
        sys.exit(1)
