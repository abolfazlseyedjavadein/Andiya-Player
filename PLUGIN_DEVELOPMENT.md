# Create an Andiya plugin

Andiya, its public SDK and the examples are MIT licensed. Your plugin may use
your own license, including a commercial or proprietary license. No paid plugin
is required to use the player.

The current API is for CPU image filters used by image/video preview and exports.
It does not provide custom UI panels, audio filters, GPU kernels, resizing or
payment processing. Start with Python for a simple prototype or C++ for native
performance. Both execute in a separate process.

## 1. Prepare your tools

Use a source checkout for the commands below; run them from its root. A packaged
SDK also includes these examples under `docs/examples`, the header under
`sdk/include/andiya`, and the packager under `sdk/tools`. See
[installed locations](docs/PACKAGING.md#installed-layout).

Python plugins require Python 3.9+ on the app's PATH. Native plugins require
CMake 3.24+ and a compiler matching the destination OS and architecture. The
native starter uses only the public header and the C++ standard library; Qt is
not required to compile it.

Build or install Andiya first to obtain `AndiyaPluginValidator` and
`AndiyaFilterWorker` (`.exe` on Windows). Keep the complete installed runtime
together so the helpers can find their libraries.

## 2. Create a Python filter

Copy [the Python starter](docs/examples/python-invert) into an external working
folder. It contains `filter.py`, `plugin.json` and an MIT `LICENSE`:

~~~powershell
New-Item -ItemType Directory -Force ../Andiya-local/plugin-work
Copy-Item -Recurse docs/examples/python-invert ../Andiya-local/plugin-work/my-invert
~~~

On macOS/Linux:

~~~sh
mkdir -p ../Andiya-local/plugin-work
cp -R docs/examples/python-invert ../Andiya-local/plugin-work/my-invert
~~~

Change the manifest's ID, name, vendor and description for your product. Use a
stable ID such as `com.yourcompany.invert`; updates and presets use this identity.
The entrypoint is relative to the folder containing `plugin.json`.

The starter inverts blue, green and red while preserving alpha. Its optional
`configure(parameters)` callback stores an `amount` setting; the manifest defines
the corresponding slider. The required function is:

~~~python
def process_frame(width, height, stride, pixel_format, timestamp_us, data):
    # data contains stride * height bytes in BGRA32 order.
    return data
~~~

Return a same-length `bytes` or `bytearray`. Returning `None` deliberately
bypasses the filter. Do not resize, modify the input bytes, or write to the
binary stdout pipe. Normal Python `print()` is redirected to stderr, but the
app currently discards worker stderr. For diagnostic details, run the bridge using the
[test harness](#5-test-your-plugin).

The bridge does not install dependencies. If you use third-party packages, make
them available to the interpreter selected by the app; see
[Python interpreter selection](docs/PLUGIN_REFERENCE.md#python-runtime).

## 3. Create a native filter

[The native starter](docs/examples/native-invert) supplies a complete C++ filter,
CMake project and platform-generated manifest. Build it outside the checkout:

~~~powershell
cmake -S docs/examples/native-invert -B ../Andiya-local/native-invert -G "Visual Studio 17 2022" -A x64 "-DANDIYA_SDK_INCLUDE=$((Resolve-Path include).Path)"
cmake --build ../Andiya-local/native-invert --config Release
~~~

On macOS/Linux, with Ninja installed:

~~~sh
cmake -S docs/examples/native-invert -B ../Andiya-local/native-invert -G Ninja -DCMAKE_BUILD_TYPE=Release -DANDIYA_SDK_INCLUDE="$PWD/include"
cmake --build ../Andiya-local/native-invert
~~~

The installable folder is `../Andiya-local/native-invert/package`. It contains
the generated `plugin.json`, library and license. Build separately for each
destination OS and architecture. For a packaged SDK, pass its absolute
`sdk/include` directory instead of the source `include` directory.

Before using your own ID, change it in **both** `plugin.cpp` and
`plugin.json.in`. The exported `andiya_plugin_initialize` function fills the
callbacks defined in the [public header](include/andiya/plugin_api.h).
Write into the host's output buffer and preserve its dimensions, format and
ownership. Do not let C++ exceptions cross the C ABI.

The starter has no custom parameters; the app's strength control still works.
For configurable native filters, the optional export is:

~~~cpp
extern "C" ANDIYA_PLUGIN_EXPORT int32_t ANDIYA_PLUGIN_CALL
andiya_plugin_configure(void *context, const char *parameters_json);
~~~

This optional symbol is resolved by name and is not declared in the public
header. Parse the UTF-8 JSON, store settings in your plugin context, and return
zero on success. See the shipped `plugins/soft-contrast/plugin.cpp` for a
parameterized example using Qt Core for JSON parsing.

## 4. Validate, install and enable

For a Windows installed tree created by the setup helper:

~~~powershell
$runtime = (Resolve-Path ../Andiya-local/build-windows/stage/bin).Path
& "$runtime/AndiyaPluginValidator.exe" ../Andiya-local/plugin-work/my-invert
& "$runtime/AndiyaPluginValidator.exe" ../Andiya-local/native-invert/package
~~~

Adjust the build directory if you used `--build-dir`. On macOS the helper is
inside `Andiya.app/Contents/MacOS`; on Linux it is under `opt/andiya/bin`.
Validation prints JSON and exits nonzero if the package is invalid. It checks
metadata and files; it does not prove a plugin processes frames correctly.

In Andiya, open **Workspace → Filters → Install folder…**, select the plugin
folder, review the publisher and permission disclosures, then enable it.
Installation copies the folder. After editing your original development folder,
install it again; **Rescan** alone does not copy edits into the installed version.

Plugins are discovered without executing their code. A manifest's
`enabled: true` does not grant approval. Changed content requires another
review. **Restart** retries workers; order, strength, sliders and presets are
available in Filters. An update preserves one prior version for **Rollback**.

## 5. Test your plugin

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
For an installed SDK, the Python bridge is in the runtime's `python_runtime`
directory. Adapt the expected pixels when developing a different effect.

Also test in the app: enable the filter, change strength/order, open an image
and a video, compare original/filtered frames, and export a full-resolution PNG.
Test transparent pixels, your slider extremes, repeated frames and slow inputs.
Preview uses a smaller resolution than export, and the two use independent
worker instances.

Crashes, timeouts and malformed worker responses are bypassed in preview and
stop an export with an error. There is a current native API limitation:
a nonzero `process_frame` return or invalid output buffer is replaced with the
input frame inside the native host and can appear successful to the exporter.
Do not rely on a native error return to abort an export.

## 6. Package and distribute

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
