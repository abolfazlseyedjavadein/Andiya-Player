# Plugin reference

This describes the Andiya 0.1.8 implementation. Start with the
[creation tutorial](../PLUGIN_DEVELOPMENT.md) for complete runnable examples.

## Manifest

Every plugin folder needs a UTF-8 `plugin.json` and an existing entrypoint.
[The example manifest](plugin-manifest.example.json) matches the Python Invert
starter. Native builds generate the same metadata with a platform library name.

Required validation fields:

- `id`: 1–128 ASCII characters matching
  `^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$`. Keep it stable across releases.
- `version`: three numeric components with an optional suffix, matching
  `^\d+\.\d+\.\d+([+-][A-Za-z0-9.-]+)?$`.
- `andiyaApi`: the string `"1.0"`; the C header's integer ABI version is `1`.
- `execution`: `"native"` or `"python"`.
- `runtime`: `"c-abi"` for native; `"process"` for Python.
  The legacy Python runtime value `"python"` is also accepted.
- `entrypoint`: a relative file path contained in the plugin folder.

Provide `name`, `vendor`, `description`, `capabilities`, `permissions`,
`performance` and `accent` for useful presentation. Current filters disclose
`frame-access`. These declarations do not enable new host APIs or restrict OS
access. A vendor name is not verified publisher identity. `platforms` metadata
is not enforced, and the catalog does not choose the appropriate architecture.

Optional `parameters` declares up to 32 numeric sliders:

~~~json
"parameters": [
  {"key": "amount", "label": "Amount", "min": 0, "max": 1, "default": 1, "step": 0.01}
]
~~~

Keys must be unique and match `^[A-Za-z][A-Za-z0-9_]{0,63}$`. Use finite numeric
bounds with `min < max`, an in-range default and a positive step.
The UI supplies parameter values, initially using the manifest defaults.
A standalone worker caller must supply values itself or rely on the plugin's
own defaults; the worker does not read the manifest's parameter schema.
Omitting parameters is valid.
When an available plugin declares parameters or actions, Andiya exposes its **Settings**
popup from **Tools** and uses the same settings in **Workspace → Filters**.
The plugin declares the controls in its manifest and consumes their values
through the worker configuration.

An optional `rectangle` object adds a **Draw rectangle** button to the plugin's
settings. Its values map the rectangle's four fields to numeric parameter keys.

```json
"rectangle": {"x": "left", "y": "top", "width": "regionWidth", "height": "regionHeight"}
```

Declare all four keys in `parameters`. X and Y use a range of 0 to 1. Width
and height use a maximum of 1 and may declare a positive minimum. Values are
fractions of the displayed frame, measured from its top-left corner. Andiya
maps the drawn selection to these fractions and sends the four updated values
together through the normal parameter configuration. The existing sliders
remain available in **Workspace → Filters** and **Tools → Settings**.
See the [Redaction Blur walkthrough](../PLUGIN_DEVELOPMENT.md#a7-add-settings-and-a-drawable-rectangle)
for the full manifest and Python example. For compatibility, the host supplies
the known rectangle mapping for Redaction Blur 1.0.0. New plugins declare
their own mapping.

An optional `files` object maps every payload path to its SHA-256 digest,
excluding `plugin.json` and `plugin.json.p7s`. If supplied and nonempty, the
hash map must cover all payload files. Use the packager to generate it.

**Remove** is an Andiya action on every plugin card. It removes the plugin from
the current profile. For a user-installed plugin it also deletes the installed
package and rollback copy. Reinstalling a package makes it available again.

## Plugin actions

A Python filter can declare up to 16 actions in its manifest's `actions`
array. Each action has a unique `id`, a `label` displayed in Settings, and
a `type` of `button` or `rectangle`. A rectangle action also requires the
manifest's `rectangle` mapping. Enable the plugin before using its actions.

```json
"actions": [
  {"id": "draw_rectangle", "label": "Draw rectangle", "type": "rectangle"},
  {"id": "reset_rectangle", "label": "Reset rectangle", "type": "button"}
]
```

Andiya calls `on_action(action_id, event, parameters)` in a separate Python
worker. Match `action_id` to the manifest ID to route the event. The
`parameters` dictionary contains the saved values at the start of the
action. The worker also calls `configure(parameters)` before handling events.

For a button, `event` is `{"type": "click"}`. Return a dictionary of declared
parameter keys and numeric values, or an empty dictionary if no setting
changes. Andiya validates and applies all returned values together. If
settings changed while the button was running, the result is discarded.

For a rectangle action, Andiya sends the following events while the editor
is open.

- An `open` event has `type` and `selection`, where `selection` is
  `[x, y, width, height]` from the saved settings.
- A `drag` event has the selection at mouse-down, `origin` and `position`
  as `[x, y]` pairs, and `handleSize` as `[width, height]`. All values
  are normalized to the picture. The host can coalesce moves, so use the
  origin and current position to calculate the result independently.
- Return `[x, y, width, height]` within the picture. Andiya displays that
  selection. **Apply rectangle** commits it to the mapped settings.
  **Cancel** discards it. The action worker receives no video frames.

Older manifests can still use the standard rectangle editor or the
`rectangleInteraction` opt-in with `rectangle_event(event)`. A declared
rectangle action uses `on_action` and replaces the legacy draw button.

## Frame and worker lifecycle

Both runtimes currently process BGRA32 buffers, pixel format `1`, with unchanged
dimensions. The current host tags native frames as sRGB; this is not a general
HDR/color-conversion contract. Other enums in the C header do not mean the
current pipeline sends those formats. Preserve alpha unless your filter
explicitly changes transparency. Timestamp values are in microseconds.

The native host owns both frame buffers. Fill all output bytes; never replace
its data pointer, resize the frame, free its memory or retain frame pointers
after the callback. Copy row padding if present and respect stride. Allocate
private state separately and release it in `destroy`.
The native `plugin_id` callback must match the manifest exactly.

Preview and export have separate serial pipelines and worker instances.
Video preview is bounded to a 960-pixel long edge; exports keep decoded
resolution. The host applies filter order and strength blending. Changing
parameters or the stack can recreate workers; do not depend on persistent
process state across edits. Slow preview frames can be skipped.

Each dimension is limited to 16384 pixels, with a 256 MiB frame payload limit.
Process startup allows about 4 seconds, writes 3 seconds and reads 5 seconds
per phase. These are phase budgets, not a single total processing deadline.
A crash, timeout or invalid protocol response fails the worker: preview bypasses
it, while export reports failure. Restart retries the pipeline.

Native processing has a narrower error-reporting limitation: a nonzero callback
return or invalid output buffer is bypassed inside the native host, so the
worker may return a successful unchanged frame and the export may continue.
For deliberate native bypass, copy the input and return zero.
Python `None` is deliberate successful bypass; an exception or wrong-sized
return is an error.

## Python runtime

The entry module must provide
`process_frame(width, height, stride, pixel_format, timestamp_us, data)`.
Input is immutable bytes; return same-length bytes/bytearray or `None`.
Optional `configure(parameters)` runs at process startup before frames.
It receives a dictionary keyed by the manifest's parameter names, including
the four mapped rectangle values when configured. The rectangle arrives as
normalized numbers through this hook. Pointer events go to the separate
interaction worker when the plugin opts in. Use each frame's width, height,
and stride to find the affected
pixels. See the [Python code walkthrough](../PLUGIN_DEVELOPMENT.md#read-plugin-parameters-in-python)
for configuration, bounds conversion, and a test with known pixels.

The module's directory is added to the import path and used as the working
directory. Bytecode-cache generation is disabled. The app does not run pip,
create a virtual environment or install a requirements file. You do not start
Python from Command Prompt to use a plugin in the player; Andiya launches the
interpreter itself as a hidden worker process. See
[How Python plugins run](../PLUGIN_DEVELOPMENT.md#how-python-plugins-run-you-do-not-use-cmd-in-the-player).

Interpreter search on Windows tries `py`, `python3`, then `python`;
other platforms try `python3`, then `python`. Install Python 3.9+ and ensure
dependencies belong to the interpreter the app actually finds. An activated
virtual environment does not necessarily override the Windows launcher.
Apps launched from a desktop icon or Finder can have a different PATH from
your terminal. There is no interpreter-path selector in the current UI.

Normal Python stdout is redirected to stderr to protect the binary protocol.
The app discards worker stderr, so inspect error messages through a standalone
bridge invocation such as the example smoke test. Do not write directly to
stdout's underlying file descriptor.

## Discovery, approval and updates

The app scans child folders in its writable application-data `plugins`
directory first, then `plugins` beside its executable. An installed user plugin
takes precedence over a bundled plugin with the same ID. Hidden directories and
symbolic links are not plugin discovery targets.

Discovery validates metadata without executing code. Enabling requires review;
approval is bound to a fingerprint of manifest bytes and package files.
Changing any file can invalidate approval, including documentation or caches.
Installation copies source files into app data. Reinstall after editing the
source copy. Rescan refreshes discovery, and Restart retries workers.
Updates preserve one previous version for Rollback; another update replaces
that backup.

Workers run with the user's OS privileges. Permissions are disclosures.
Process isolation contains failures but is not an OS security sandbox.

## Bundles and signatures

The portable format is a JSON object with base64-encoded file contents:

~~~json
{"format":"andiya-plugin-bundle","files":{"plugin.json":"BASE64","filter.py":"BASE64"}}
~~~

Use ordinary nonhidden ASCII filenames made from letters, digits, underscores,
hyphens and dots, with forward slashes between directories. Bundle extraction
rejects absolute paths, empty/dot/parent segments, trailing dots and unsafe
characters. Links are forbidden. Folder copying excludes hidden files, so do
not depend on hidden payloads.

The packager allows at most 2000 files and 40 MiB of raw contents. Downloads and
bundles are limited to 64 MiB; folder validation permits at most 2000 files
(including metadata) and 512 MiB. A manifest or detached signature may be at most 1 MiB.
The smaller packaging limits account for base64/JSON overhead.

Normal packaging creates hashes in the embedded manifest without rewriting the
source. `--hash-only` writes the hashed source manifest before signing.
`--preserve-manifest` checks that its hashes still match and preserves its bytes.
A signed manifest cannot be automatically rewritten by the packager.

On Windows, `plugin.json.p7s` is checked as a detached CMS/PKCS#7 signature over
the exact manifest bytes, followed by certificate-chain validation.
Invalid signatures are rejected. A valid signature with an untrusted certificate
is labeled untrusted; unsigned packages are unverified. Online revocation is
not checked. Signed bundles are rejected on macOS/Linux because publisher-signature
verification is not implemented there. Distribute separate unsigned packages for
those platforms. Content hashes detect changes but do not authenticate a publisher.

Catalogs may be local files or HTTPS URLs. Package locations resolve relative to
the catalog; HTTPS redirects cannot downgrade to HTTP. Downloaded packages
undergo separate validation and are installed disabled. Catalog entries do not
automatically filter by OS, architecture or license.

## Troubleshooting

- **Unavailable card or validator failure:** inspect its JSON output, API version,
  entrypoint and parameter bounds. Run validation against the folder containing
  the manifest, not the entrypoint alone.
- **Native worker cannot load:** check OS/architecture, exported initialization
  symbol, matching IDs and dependent shared libraries. Configure platform loader
  paths for dependencies; copying a library is not proof it can be resolved.
- **Python filter cannot start:** check interpreter discovery, imports and working
  directory. Run the smoke-test harness to see captured bridge stderr.
- **Edits do not appear:** reinstall the edited source folder and approve its
  changed fingerprint. Use Restart after correcting a worker failure.
- **Hash or signature mismatch:** regenerate hashes for unsigned development
  files; signed packages must be rebuilt and signed again after modification.
- **Preview works but export is slow:** exports process full-size frames.
  Optimize per-pixel loops or use a native implementation; no GPU API is present.
- **Package rejected:** keep the output bundle outside the input folder, remove
  caches/build files, and check file names and size limits.
