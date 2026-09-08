# Third-party components

Andiya's own code, public plugin SDK and examples are covered by [MIT](LICENSE).
Third-party plugins may use their own licenses, including commercial licenses.
The main player does not require a paid plugin.

Packaged builds use dynamically linked Qt 6 (Core, GUI, Network, QML, Quick,
Quick Controls, Multimedia and their runtime plugins). Qt and its bundled
dependencies retain their original licenses; Andiya's MIT license does not
replace them. See [Qt licensing](https://doc.qt.io/qt-6/licensing.html) and the
[Qt 6.8.3 source archive](https://download.qt.io/archive/qt/6.8/6.8.3/single/).
The media backend also deploys FFmpeg libraries from the Qt kit. Their build
configuration determines applicable terms; see [FFmpeg licensing](https://ffmpeg.org/legal.html).

Windows packages contain Microsoft's redistributable runtime libraries from the
build toolchain. These are not part of Andiya's MIT-licensed source. Python is
an optional, separately installed interpreter; no Python distribution is bundled.

Before publishing a binary release, retain the license texts and notices supplied
with the exact Qt/FFmpeg build, provide the corresponding source and modification
information required by those licenses, and record the SDK/compiler versions.
Use a dynamically linked LGPL-compatible Qt distribution or a suitable Qt
commercial license. The package scripts do not grant redistribution rights or
supply signing certificates. Local Windows builds are unsigned; macOS setup uses an ad-hoc signature, not a production publisher identity.
See [packaging](docs/PACKAGING.md) for release preparation.

The plugin starter examples are MIT licensed and include their license text.
Authors may distribute their own plugins commercially, but must retain any
notices required by code and dependencies they incorporate. The host does not
provide billing or license activation. See the [plugin creation guide](PLUGIN_DEVELOPMENT.md).
