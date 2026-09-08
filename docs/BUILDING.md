# Building and verifying Andiya

Andiya targets Windows, macOS and Linux desktops. The current local release has
been verified on Windows; macOS/Linux build and package configuration exists
but still requires execution and validation on those systems. Build on the OS
and architecture you intend to distribute.

The player, SDK and examples are MIT licensed; third-party plugins can use their
own licenses, including commercial licenses.

## Prerequisites

- CMake 3.24+ for direct configuration and the setup helper. The checked-in
  presets use schema 6 and need CMake 3.25+
  ([CMake preset versions](https://cmake.org/cmake/help/v3.25/manual/cmake-presets.7.html)).
- A C++20 compiler: Visual Studio 2022 with Desktop development with C++ and
  a Windows SDK, or a compatible Clang/GCC toolchain on macOS/Linux.
- Qt 6.8+ with Quick, Quick Controls 2, Multimedia and Network, using a kit that
  matches the compiler and architecture. The current Windows release uses 6.8.3.
- Python 3.9+ for setup, verification, packaging plugins and Python filters.
  Playback and native filters do not require Python.
- Ninja on macOS/Linux for the helper's default generator; Xcode command-line
  tools on macOS. Linux Debian packaging also needs dpkg tooling, including
  `dpkg-shlibdeps` from `dpkg-dev`, and the destination runtime libraries.
- NSIS 3.03+ on Windows for the setup executable. The tested local compiler is
  NSIS 3.12; see [NSIS downloads](https://nsis.sourceforge.io/Download).

Install Qt and build dependencies outside the checkout. Point `--qt-prefix`
or `QT_ROOT_DIR` at the desktop kit directory containing
`lib/cmake/Qt6/Qt6Config.cmake`, not its parent SDK directory.

## Build, install and package

Run from the checkout root. Replace the example Qt path with your kit.

Windows PowerShell:

~~~powershell
.\setup.ps1 --qt-prefix C:/Qt/6.8.3/msvc2022_64 --package
~~~

If PowerShell script execution is disabled, use the same helper directly:

~~~powershell
py -3 -B tools/setup.py --qt-prefix C:/Qt/6.8.3/msvc2022_64 --package
~~~

macOS:

~~~sh
QT_ROOT_DIR="$HOME/Qt/6.8.3/macos" sh ./setup.sh --package
~~~

Linux:

~~~sh
QT_ROOT_DIR="$HOME/Qt/6.8.3/gcc_64" sh ./setup.sh --package
~~~

The helper defaults to Visual Studio 17 2022, x64 on Windows and Ninja elsewhere.
It builds Release, deploys Qt and the media runtime, and installs into
`../Andiya-local/build-<os>/stage` (`windows`, `darwin` or `linux`).
An existing stage is renamed to `stage-previous-<timestamp>` before rebuilding.
Keep these backups only as long as needed.

Without flags, setup builds and installs. `--verify` adds installed-runtime
checks. `--package` automatically verifies before creating packages and SHA-256
sidecars in the external build's `packages` directory.
`--package --archive-only` creates only the Windows ZIP when NSIS is unavailable.
For an NSIS installation outside PATH, add `--nsis C:/Tools/NSIS/makensis.exe`.

Use `--build-dir ABSOLUTE_EXTERNAL_PATH`, `--jobs 4`, or `--generator NAME`
to override defaults. Do not reuse a CMake build directory with a different
generator or incompatible Qt kit. Run `py -3 -B tools/setup.py --help`
(or `python3` on other platforms) for all arguments.

## Manual source builds and presets

The setup helper is the recommended installed-release path. For a development
build with Ninja, set both preset variables first. On Windows, run these commands
in a Visual Studio developer PowerShell so Ninja can locate the compiler:

~~~powershell
$env:QT_ROOT_DIR = "C:/Qt/6.8.3/msvc2022_64"
$env:ANDIYA_BUILD_DIR = [IO.Path]::GetFullPath("../Andiya-local/preset-release")
cmake --preset release
cmake --build --preset release
~~~

On macOS/Linux:

~~~sh
export QT_ROOT_DIR="$HOME/Qt/6.8.3/gcc_64" # macOS: use the macos kit
export ANDIYA_BUILD_DIR="$(cd .. && pwd)/Andiya-local/preset-release"
cmake --preset release
cmake --build --preset release
~~~

The `release` preset does not deploy runtime dependencies on install.
The `package` configure preset enables `ANDIYA_DEPLOY_RUNTIME`, but does not
itself execute setup's staging, verification, signing or installer steps.
Use `setup.py --package` to reproduce that complete sequence.

## Run the installed app

- Windows: `STAGE/bin/Andiya.exe`, or the installer-created shortcut.
- macOS: `STAGE/Andiya.app`.
- Linux: `STAGE/opt/andiya/bin/Andiya`, or the launcher after installing the DEB.

Keep the runtime, helpers, plugins and deployed libraries together.
See [packaging](PACKAGING.md) for install paths and platform-specific limits.

## Verification

The easiest complete check is to rerun setup with `--verify` and the Qt prefix.
It supplies Qt's offscreen platform plugin, generates deterministic media,
runs 15 integration groups and plugin regressions, and creates seven UI snapshots.

To verify an existing development stage from a source checkout:

~~~powershell
py -3 -B tools/verify_install.py ../Andiya-local/build-windows/stage --output ../Andiya-local/verification-docs
~~~

Use a dedicated external output directory: verification replaces its generated
fixture, logs, `exports` and `ui` contents. It isolates test settings and clears
Qt plugin-path overrides. Inspect `results.txt`, `check-*.log` and `ui/*.png`
in that directory.

A stage verified directly must already contain the offscreen plugin in its Qt
platform directory: `plugins/platforms` on Windows,
`Andiya.app/Contents/PlugIns/platforms` on macOS, or
`opt/andiya/plugins/platforms` on Linux. The helper copies it from the specified
Qt prefix and removes it before release packaging. Ordinary desktop playback
uses the normal platform plugin and does not need the offscreen plugin.

For a focused integration run, generate the fixture first and then call the app:

~~~powershell
py -3 -B tools/generate_test_video.py ../Andiya-local/self-test/fixture.avi
& ../Andiya-local/build-windows/stage/bin/Andiya.exe --self-test ../Andiya-local/self-test
~~~

The fixture is a six-second 10 fps AVI. Automated fixtures do not certify every
codec, live protocol, hardware decoder or third-party filter. Keep builds,
packages, generated media, test results and dependencies outside the checkout.
