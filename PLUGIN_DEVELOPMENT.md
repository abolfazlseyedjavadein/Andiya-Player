# Create an Andiya plugin

Andiya, its public SDK and the examples are MIT licensed. Your plugin may use
your own license, including a commercial or proprietary license. No paid plugin
is required to use the player.

The current API is for CPU image filters used by image/video preview and exports.
It does not provide custom UI panels, audio filters, GPU kernels, resizing or
payment processing. Start with Python for a simple prototype or C/C++ for native
performance. Both execute in a **separate process** that Andiya starts itself.

## How Python plugins run (you do not use CMD in the player)

You never open Command Prompt, PowerShell or a terminal to *use* a Python
plugin inside Andiya.

When a Python filter is enabled:

1. Andiya looks on the system PATH for a Python 3.9+ interpreter (`py`, then
   `python3`, then `python` on Windows).
2. It starts that interpreter as a hidden background process.
3. It runs Andiya's bridge script `python_runtime/andiya_python_host.py`.
4. The bridge loads your `filter.py` and talks to the player over a binary
   pipe (frames in, frames out). No console window is shown.

You only use a terminal while **developing**: copying files, running the
smoke test, or packaging a `.andiyaplugin` bundle.

Install Python 3.9+ once from [python.org](https://www.python.org/downloads/)
and tick **Add python.exe to PATH**. Restart Andiya after installing Python so
it can find the interpreter. A plugin that needs extra packages (NumPy, etc.)
must be able to `import` them in *that* interpreter; Andiya does not run
`pip`, create a venv, or open a CMD window for you.

Native C/C++ plugins do not need Python at all. Andiya loads them through
`AndiyaFilterWorker` in a separate process.

Use a source checkout for the commands below; run them from its root. A packaged
SDK also includes these examples under `docs/examples`, the header under
`sdk/include/andiya`, and the packager under `sdk/tools`. See
[installed locations](docs/PACKAGING.md#installed-layout).

---

## Tutorial A — Python plugin, step by step

### A1. Install Python

- Windows: install Python 3.9+ and enable **Add python.exe to PATH**.
- macOS/Linux: `python3 --version` should print 3.9 or newer.

You do **not** run this plugin from CMD later. This is only so Andiya can
find an interpreter.

### A2. Copy the starter

The starter inverts RGB and keeps alpha. Copy it out of the source tree:

~~~powershell
New-Item -ItemType Directory -Force ../Andiya-local/plugin-work
Copy-Item -Recurse docs/examples/python-invert ../Andiya-local/plugin-work/my-invert
~~~

On macOS/Linux:

~~~sh
mkdir -p ../Andiya-local/plugin-work
cp -R docs/examples/python-invert ../Andiya-local/plugin-work/my-invert
~~~

The folder must contain `plugin.json` and `filter.py` (plus a license file).

### A3. Give it your identity

Edit `../Andiya-local/plugin-work/my-invert/plugin.json`:

- `id`: stable ASCII id such as `com.yourcompany.invert` (updates use this)
- `name`, `vendor`, `description`
- `entrypoint`: `"filter.py"` (relative to this folder)
- `execution`: `"python"` and `runtime`: `"process"`

### A4. Write the filter

Open `filter.py`. The only required function is:

~~~python
def process_frame(width, height, stride, pixel_format, timestamp_us, data):
    # data is stride * height bytes, BGRA32, one pixel = B,G,R,A
    output = bytearray(data)
    # change pixels here; do not resize
    return bytes(output)
~~~

Rules:

- Return `bytes` or `bytearray` of the **same length**, or `None` to skip
  (bypass) this filter.
- Do not resize, do not write to stdout, do not modify the input `data` object.
- Optional `configure(parameters)` runs once when Andiya starts the worker.
  The starter uses it for an `amount` slider declared in `plugin.json`.

The starter inverts blue/green/red. Change the loop to implement your effect.

### A5. Smoke-test from a terminal (optional, development only)

This is the only time you use CMD/PowerShell for Python. It proves the bridge
can load your file; the player uses the same bridge automatically.

~~~powershell
py -3 -B docs/examples/check_filter.py python runtime/python/andiya_python_host.py ../Andiya-local/plugin-work/my-invert/filter.py
~~~

On macOS/Linux use `python3` instead of `py -3`. For an installed SDK, the
bridge is next to the app in `python_runtime/andiya_python_host.py`.

### A6. Install it in Andiya

1. Start Andiya (double-click the app or installer shortcut — not CMD).
2. Open **Workspace → Filters → Install folder…**
3. Select `../Andiya-local/plugin-work/my-invert`
4. Review the publisher and permissions, then **enable** the plugin.
5. Open an image or video, press **K** for original vs. filtered compare.

Installation **copies** the folder. After you edit `filter.py`, install the
folder again. **Rescan** does not copy your edits.

---

## Tutorial B — C/C++ plugin, step by step

Native plugins are a shared library (`.dll` / `.so` / `.dylib`) plus
`plugin.json`. Qt is **not** required. You need CMake 3.24+ and a compiler
for the target OS/architecture (MSVC 2022 on Windows x64).

### B1. Copy and build the starter

The native starter inverts RGB through the public C ABI.

Windows (from the source root):

~~~powershell
cmake -S docs/examples/native-invert -B ../Andiya-local/native-invert -G "Visual Studio 17 2022" -A x64 "-DANDIYA_SDK_INCLUDE=$((Resolve-Path include).Path)"
cmake --build ../Andiya-local/native-invert --config Release
~~~

macOS/Linux (Ninja):

~~~sh
cmake -S docs/examples/native-invert -B ../Andiya-local/native-invert -G Ninja -DCMAKE_BUILD_TYPE=Release -DANDIYA_SDK_INCLUDE="$PWD/include"
cmake --build ../Andiya-local/native-invert
~~~

For a packaged SDK, pass its `sdk/include` directory instead of `include`.
The installable folder is `../Andiya-local/native-invert/package` (`plugin.json`,
the library, and `LICENSE`).

Build separately for each OS and architecture. A Windows `.dll` will not load
on macOS or Linux.

### B2. Change the plugin id

Before shipping your own product, change the id in **both**:

- `docs/examples/native-invert/plugin.cpp` (`pluginId()` return value)
- `docs/examples/native-invert/plugin.json.in` (`"id"`)

Then rebuild.

### B3. Implement `process_frame`

The host calls the C function exported as `andiya_plugin_initialize`. That
fills `AndiyaImageFilterApi` with your callbacks. `process_frame` must:

- Read BGRA32 pixels from `input->data`
- Write the same size/format into the host-owned `output->data`
- Preserve dimensions, stride and buffer ownership (do not free or replace
  the pointer)
- Return `0` on success
- Never let a C++ exception cross the C ABI

See [the public header](include/andiya/plugin_api.h) and the starter
`plugin.cpp`. The shipped `plugins/soft-contrast/plugin.cpp` shows optional
JSON parameters via:

~~~cpp
extern "C" ANDIYA_PLUGIN_EXPORT int32_t ANDIYA_PLUGIN_CALL
andiya_plugin_configure(void *context, const char *parameters_json);
~~~

This optional symbol is resolved by name and is not declared in the public
header. Return zero on success. The app's strength slider still applies even
if you have no custom parameters.

### B4. Validate, then install in the app

Windows, with a setup-helper stage:

~~~powershell
$runtime = (Resolve-Path ../Andiya-local/build-windows/stage/bin).Path
& "$runtime/AndiyaPluginValidator.exe" ../Andiya-local/native-invert/package
~~~

On macOS the helper is in `Andiya.app/Contents/MacOS`; on Linux it is under
`opt/andiya/bin`. Validation checks metadata and files; it does not prove the
pixels are correct.

Then in Andiya: **Workspace → Filters → Install folder…** and select the
`package` folder. Enable it and open media. No extra CMD step is needed at
runtime — Andiya launches `AndiyaFilterWorker` itself.

---

## Validate, install and enable (both runtimes)

For a Windows installed tree created by the setup helper:

~~~powershell
$runtime = (Resolve-Path ../Andiya-local/build-windows/stage/bin).Path
& "$runtime/AndiyaPluginValidator.exe" ../Andiya-local/plugin-work/my-invert
& "$runtime/AndiyaPluginValidator.exe" ../Andiya-local/native-invert/package
~~~

Adjust the build directory if you used `--build-dir`. Validation prints JSON
and exits nonzero if the package is invalid.

Plugins are discovered without executing their code. A manifest's
`enabled: true` does not grant approval. Changed content requires another
review. **Restart** retries workers; order, strength, sliders and presets are
available in Filters. An update preserves one prior version for **Rollback**.

## Test your plugin

The examples include a standard-library-only
[worker smoke test](docs/examples/check_filter.py). It sends two known BGRA
pixels through the actual Python bridge or native worker and checks RGB
inversion, alpha preservation and the response frame. Use the unmodified
starter for these expected results:

~~~powershell
py -3 -B docs/examples/check_filter.py python runtime/python/andiya_python_host.py ../Andiya-local/plugin-work/my-invert/filter.py
py -3 -B docs/examples/check_filter.py native "$runtime/AndiyaFilterWorker.exe" ../Andiya-local/native-invert/package
~~~

On macOS/Linux replace `py -3` with `python3` and use the platform's worker path.
Adapt the expected pixels when developing a different effect.

Also test in the app: enable the filter, change strength/order, open an image
and a video, compare original/filtered frames (**K**), and export a PNG.
Test transparent pixels, slider extremes, repeated frames and slow inputs.
Preview uses a smaller resolution than export, and the two use independent
worker instances.

Crashes, timeouts and malformed worker responses are bypassed in preview and
stop an export with an error. There is a current native API limitation:
a nonzero `process_frame` return or invalid output buffer is replaced with the
input frame inside the native host and can appear successful to the exporter.
Do not rely on a native error return to abort an export.

## Package and distribute

~~~powershell
py -3 -B tools/package_plugin.py ../Andiya-local/plugin-work/my-invert ../Andiya-local/my-invert.andiyaplugin
~~~

Write the bundle **outside** its source plugin folder. The packager embeds a
manifest with SHA-256 hashes; ordinary packaging does not rewrite the source
manifest. Distribute the resulting `.andiyaplugin` file and install it using
**Install bundle…**. Bundle only runtime files and required licenses: exclude
build trees, caches, secrets and the generated bundle itself.

For signing, first run `tools/package_plugin.py FOLDER --hash-only`, which
writes hashes into the source manifest. Sign those exact bytes with your
CMS-capable tool and save the detached DER signature as `plugin.json.p7s`.
Then run `tools/package_plugin.py FOLDER OUTPUT.andiyaplugin --preserve-manifest`.
Do not modify signed files afterward. Windows performs publisher-chain checks;
signing keys never belong in the package. Signed bundles are currently rejected
on macOS/Linux; publish separate unsigned packages for those systems.

A local or HTTPS catalog can list downloadable packages:

~~~json
{
  "format": "andiya-catalog",
  "plugins": [
    {
      "name": "My Invert - Windows x64",
      "version": "1.0.0",
      "package": "my-invert-windows-x64.andiyaplugin"
    }
  ]
}
~~~

Package addresses resolve relative to the catalog. Use **Load** or
**Local catalog…** in Filters, then **Install / update**. Native platform
selection is currently manual: platform metadata does not automatically select
or block packages. Publish clearly labeled OS/architecture builds.

See the [plugin reference](docs/PLUGIN_REFERENCE.md) for exact manifest rules,
limits, trust behavior and troubleshooting. Commercial distribution may be
handled by your own website or catalog; the player has no built-in billing,
license activation or automatic background-update service.
