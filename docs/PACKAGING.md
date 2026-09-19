# Release packaging

The current project version is **0.1.4**. The Windows x64 packages are built
from this source revision with the creator name **Seyed Abolfazl Seyed
Javadein**. The installed verification suite covers Windows; macOS/Linux
packages still require native-platform build and distribution validation.

Use [the setup helper](BUILDING.md) as the authoritative release procedure.
It packages the installed tree after verification, including the player,
workers, bundled filters, Python bridge, SDK header, packager, documentation,
starter examples and icons. Runtime deployment collects Qt/QML and media
dependencies from the selected kit.

## Artifacts and platform status

- Windows x64: `Andiya-0.1.4-Windows-x64.zip` and
  `Andiya-0.1.4-Windows-x64-setup.exe`. The NSIS installer installs per user under
  local application data without requiring administrator rights. The current
  local ZIP and installer have been built and verified.
- macOS: `Andiya-0.1.4-Darwin-<arch>.dmg` containing `Andiya.app`.
  Build on macOS with the selected architecture. The helper applies and verifies
  an ad-hoc signature; this is not Developer ID signing or notarization.
  Native build and distribution validation remain required.
- Linux: `Andiya-0.1.4-Linux-x86_64.tar.gz` and `.deb`. Current metadata is
  fixed to x86_64 and declares glibc 2.35+ plus runtime dependencies.
  This minimum is not proof of compatibility with every Qt build or distribution.
  Build and test on the intended baseline before distributing.

The helper writes artifacts under `BUILD/packages` and emits `.sha256` sidecars.
The Windows ZIP is portable as a complete extracted tree; do not move only the
executable. Windows local builds are unsigned. macOS ad-hoc signing does not
establish a trusted publisher.

The Linux archive contains `opt/andiya` and desktop integration under `usr/share`.
Its executable can be run from the extracted `opt/andiya/bin` when system
dependencies are met. The desktop launcher references `/opt/andiya/bin/Andiya`;
desktop integration requires installation at that location. There is no RPM,
AppImage, mobile installer or universal architecture package in this release.

## Installed layout

Paths below are relative to the installed tree or extracted archive:

- Windows runtime: `bin`; documentation/data: `share/andiya`.
- macOS runtime: `Andiya.app/Contents/MacOS`; documentation/data:
  `Andiya.app/Contents/Resources`.
- Linux runtime: `opt/andiya/bin`; documentation/data:
  `opt/andiya/share/andiya`.

The runtime directory contains `Andiya`, `AndiyaFilterWorker` and
`AndiyaPluginValidator` (with `.exe` on Windows), app filters under
`plugins`, and the bridge under `python_runtime`. Qt's runtime plugin
directories are separate from the app's filter folders.

Within the data directory, find:

- `sdk/include/andiya/plugin_api.h` and `sdk/tools/package_plugin.py`.
- `PLUGIN_DEVELOPMENT.md`, `README.md`, `THIRD_PARTY.md` and `LICENSE`.
- `docs`, including `examples/python-invert`, `examples/native-invert` and
  `examples/check_filter.py`.
- `icons`.

Tutorial commands using repository-root `include`, `tools`, `runtime` or
`plugins` paths assume a source checkout. In an installed SDK, use the paths
above. Application sources, full setup scripts and integration tests require
the source checkout; source-code links in installed Markdown refer to that
checkout, not additional bundled files.

## Release procedure

1. Set the version in the root CMake project for a new version and synchronize
   versioned documentation. Record the OS, architecture, compiler, Qt, Python
   and CMake versions used.
2. Run `setup.ps1 --package` or `sh setup.sh --package` with the matching Qt
   prefix. Check `verification/results.txt`, regression logs and UI snapshots.
3. Test the resulting package on a clean target machine or VM, including local
   media, a network stream, subtitles, native/Python filters and PNG export.
   Python filters require a separate interpreter.
4. Retain the exact dependency notices and required license/source materials;
   see [third-party components](../THIRD_PARTY.md). Preserve the MIT core and
   let independent plugin authors specify their own terms.
5. Apply production signing/notarization with the maintainer's credentials where
   required. Recompute hashes after any signing or packaging change.
6. Publish the reviewed packages, matching source version, hashes and release
   notes. List tested platforms and known limits explicitly.

Local output filenames can be reused when rebuilding the same version; use a
new external build directory if you need to preserve an earlier release.
A documentation change updates source now; existing ZIPs/installers retain the
documentation captured when they were built and need rebuilding to include it.

## CI status

The GitHub Actions matrix runs the same setup helper used for local release
verification on Windows, macOS and Linux. It builds into the runner's temporary
directory, tests the installed runtime, packages the verified stage and uploads
both packages and verification output as workflow artifacts. The Windows CI job
creates the portable ZIP and the per-user NSIS installer. Pushing a `v*` tag
publishes the matrix packages to a GitHub Release after every platform job succeeds.

A platform should be described as CI-verified only after its matrix job succeeds.
A successful CI package is still not a substitute for an installation test on a
clean target system. macOS Developer ID signing and notarization require the
maintainer's credentials; CI uses only the helper's ad-hoc signature.