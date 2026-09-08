# Andiya

Andiya is an MIT-licensed desktop player for video, audio, images, network streams,
frame review and lossless image export. Third parties can create and sell plugins
under their own licenses. No separate desktop media player is required.

## Creator

Andiya is created and maintained by **Seyed Abolfazl Seyed Javadein**, an educator
and developer focused on deep learning, computer vision, machine learning,
image processing and Python. Visit [the personal website](https://codetipsacademy.com)
or [LinkedIn](https://www.linkedin.com/in/abolfazl-javadein/) to learn more.

## Features

- Qt Multimedia playback, playlists, history, resume, subtitles and audio controls.
- Forward/backward navigation using decoded frame timestamps, bookmarks with notes
  and A–B looping for seekable video.
- Original/filtered comparison, full-resolution PNG captures, interval exports,
  bookmark exports, image batches, cancellation and timestamped contact sheets.
- Asynchronous video filter previews, with a bounded 960-pixel preview resolution.
  Export processing retains full decoded resolution.
- Native C-ABI and Python filter workers running outside the player process.
  Failed workers are bypassed during preview and reported in diagnostics.
- Persistent filter order, strength, numeric parameters and named presets.
- Local plugin installation, portable bundles, custom HTTPS/local catalogs,
  version rollback, SHA-256 checks and Windows detached publisher-signature checks.
- Named stream profiles encrypted for the Windows user account. History and
  portable workspaces omit URL credentials, query strings and fragments.
- Portable workspace import/export: bookmarks, notes, playlist, filter presets,
  theme, text size and keyboard preferences. Media files are referenced by path.
- Playback diagnostics, adjustable text size, keyboard-focusable buttons and
  configurable common shortcuts.
- The supplied Andiya icon is used in the window and executable.

Open **Workspace** from the navigation rail or press **Ctrl+Shift+W**.
Settings remain available with **Ctrl+,**. Open a file with **Ctrl+O**, or a stream
with **Ctrl+U**. The default frame-step keys are comma and period; **S** captures
the original frame and **K** opens comparison.

## Documentation

- [User guide](docs/USER_GUIDE.md): playback, sidebars, exports and workspaces.
- [Build and verification](docs/BUILDING.md): Windows, macOS and Linux setup.
- [Release packaging](docs/PACKAGING.md): artifacts, install layouts and release status.
- [Create a plugin](PLUGIN_DEVELOPMENT.md): runnable Python and C++ starters.
- [Plugin reference](docs/PLUGIN_REFERENCE.md): manifest, API, trust and troubleshooting.
- [Implementation notes](docs/IMPLEMENTATION.md): behavior, limits and verification.
- [Third-party notices](THIRD_PARTY.md): dependencies and distribution requirements.

## Build and run

Requires C++20, CMake 3.24+ (3.25+ for presets), Python 3.9+ for setup/tests, and Qt 6.8+
with Quick, Quick Controls 2, Multimedia and Network. Build on the target
desktop OS and keep SDKs, builds and generated media outside this checkout.

The native-platform setup commands are:

~~~powershell
.\setup.ps1 --qt-prefix C:/Qt/6.8.3/msvc2022_64 --package
~~~

~~~sh
QT_ROOT_DIR="$HOME/Qt/6.8.3/macos" sh ./setup.sh --package
# Linux: QT_ROOT_DIR="$HOME/Qt/6.8.3/gcc_64" sh ./setup.sh --package
~~~

The helper configures a Release build, deploys Qt and the media runtime, runs
self-tests and UI smoke checks, and creates native packages. Read
[Building](docs/BUILDING.md) for prerequisites and [packaging](docs/PACKAGING.md)
for release requirements. Use --verify to build, install and verify without packaging, or
--package --archive-only on Windows without an installer compiler.
Windows packages have been verified locally; macOS/Linux configuration still
needs native-platform validation.

All streaming and codec support comes from the Qt Multimedia build shipped with
the package. The app has no dependency on another desktop media player. Python
filters require a separately installed Python 3 interpreter; native filters do
not. Secure stream-profile storage and publisher certificate checks are currently
Windows-only.

## Verification

`tools/generate_test_video.py` creates a six-second, 10 fps AVI with known
timestamps. `Andiya.exe --self-test TEST_DIRECTORY` runs integration checks in a
separate settings namespace and writes `results.txt`. Generate
`TEST_DIRECTORY/fixture.avi` first. Installed Windows offscreen tests require Qt's
`plugins/platforms/qoffscreen.dll` relative to the install root; ordinary
Windows desktop use requires `qwindows.dll`.

See [implementation notes](docs/IMPLEMENTATION.md) and the
[plugin developer guide](PLUGIN_DEVELOPMENT.md) for behavior and limits.
