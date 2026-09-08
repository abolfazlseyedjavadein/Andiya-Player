# Implementation notes

The main app, SDK and examples remain MIT licensed. Commercial third-party
plugins remain supported by the public runtime and packaging contract.

## Playback and exports

Frame navigation uses decoded timestamps. Backward navigation decodes a short
look-behind window and selects the preceding frame; a timeout reports failure
instead of guessing from the total media duration. Opening another
source invalidates pending preview results and clears loop markers. Seeking within the same source preserves the loop markers.

A separate media decoder processes batch items. Output uses atomic PNG writes,
unique filenames and decoded-resolution images. Contact sheets retain only
scaled thumbnails. The queue is limited to 200 items. Cancellation leaves
completed files intact; a write already in progress may complete.

Preview and capture pipelines run on separate serial threads. Native and Python
plugin code executes in child processes, with framed I/O and bounded waits.
Frames can be skipped by slow previews; diagnostics report those skips separately
from decoder information.

## Workspace and trust

The Workspace dialog exposes bookmarks/notes, loop markers, exports, filter
settings/presets, catalog installation, encrypted stream profiles, diagnostics,
text scaling and configurable shortcuts. Import replaces the corresponding
workspace data after schema checks. Referenced media is not copied.

Windows DPAPI protects complete stream-profile URLs. Portable workspace files
exclude profiles and sanitize media URL credentials/query strings. Signed URLs
should be opened through saved profiles, since history deliberately omits their
secret query components.

Plugin manifests and file paths are validated before loading. Signed packages
use detached CMS signatures and payload hashes; unsigned and untrusted publishers
are labeled accurately. Approval is tied to package content. Process isolation
contains plugin crashes but does not restrict the child's filesystem or network
access.

## Limits to retain in release notes

- Protocol/codec availability depends on the shipped Qt Multimedia/FFmpeg build.
  Accepting a URL is not proof that its transport is implemented. Network devices,
  every streaming protocol and every hardware decoder have not been certified.
- Qt does not expose the exact active hardware decoder or decoder-drop count
  through the public interface used here; diagnostics says so.
- Video previews are bounded CPU processing. A GPU filter API and dimension-changing
  upscaling are future API work.
- Secure profile storage and signature-chain verification currently target Windows.
- Public marketplace hosting, billing and commercial AI model packages require
  external providers; the app includes the catalog/bundle client and SDK.
- Worker permissions are disclosed, not enforced by an OS security sandbox.
- Native callback errors or invalid output buffers currently fall back to the
  input inside the native host; an export may therefore continue with an unchanged
  frame. Crashes, timeouts and malformed worker responses fail the export.
- Platform metadata does not automatically select or block native catalog packages.
- Windows releases have been verified locally; macOS/Linux build and packaging
  configuration still needs validation on those systems.

## Verification — 2026-09-08

- Release build completed with MSVC 2022 and Qt 6.8.3.
- Fifteen integration checks passed: metadata-only discovery; native processing,
  parameters and full resolution; strength blending; successful Python processing
  with omitted parameters; crash containment and event-loop responsiveness;
  forward/backward 10 fps frame navigation; bookmarks/notes/loop markers;
  interval capture and contact sheet; cancellation; image export; encrypted
  profiles and redacted workspace round trip; configurable shortcuts; approval
  and preset persistence; installation/update/rollback; hung-worker timeout.
- Package regression checks passed for bundle round trip, content fingerprints,
  tampering, invalid signatures, escaped paths, invalid parameters/API versions
  and oversized native-worker frame headers.
- A correctly signed package using an ephemeral self-signed certificate was
  accepted and accurately labeled as an untrusted certificate.
- All six workspace tabs rendered without QML diagnostics. The 980-pixel window
  and 150% text-size layout were visually inspected; longer content scrolls.
- The supplied PNG is retained, and the Windows icon contains seven standard sizes.
- Project source and asset filenames/content passed the requested name-exclusion scan.

Reproduce the package checks with:

```powershell
$runtime = (Resolve-Path ../Andiya-local/build-windows/stage/bin).Path
py -3 -B tests/test_plugin_tools.py "$runtime/AndiyaPluginValidator.exe" "$runtime/AndiyaFilterWorker.exe" tools/package_plugin.py
```

The tests use generated local media and isolated settings. They do not certify
every live streaming transport, hardware decoder or third-party plugin.


## UI layout behavior

The library rail and the Tools/Captures drawer are independent on wide
windows (at least 1280 logical pixels wide). On narrower windows, opening one
drawer closes the other so the
library, history and playlist controls remain reachable. Resizing across the
wide-window breakpoint reconciles the open drawer state. Fullscreen mode hides
both sidebars without losing their selection.

## Documentation and SDK examples

The plugin tutorial includes standalone Python and C++ inversion filters, manifest
templates, a native CMake project and a worker-protocol smoke test. See
[Create a plugin](../PLUGIN_DEVELOPMENT.md) and [Plugin reference](PLUGIN_REFERENCE.md).
The install rules copy these examples with the documentation; existing packages
must be rebuilt to include later documentation changes.

For complete installed checks and the platform-specific offscreen setup, follow
[Building and verifying](BUILDING.md). Release artifacts and CI limitations are
documented in [Release packaging](PACKAGING.md).

Documentation examples verified on Windows on 2026-09-08: the standalone native
starter compiled with MSVC, and both starters passed pixel/alpha/protocol checks
through the actual workers. Both manifests and packaged round trips passed the
validator; hash generation and source-manifest preservation were checked.
Python amount bounds and row padding, local Markdown links/anchors, and the
project filename/content exclusion scan also passed.
