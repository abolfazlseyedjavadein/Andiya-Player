# Using Andiya

Andiya plays local video, audio and images, opens network streams, and provides
frame review, filter comparison and PNG exports. The core is MIT licensed and
works without a paid plugin.

## Open media and navigate

Use **Open** or **Ctrl+O** for a file, and **Ctrl+U** for a stream.
Transport and codec availability comes from the packaged Qt Multimedia/FFmpeg
build. A URL being accepted does not guarantee that its protocol or codec works.

Use Library, History and Playlist on the left navigation rail. Tools and Captures
open the right drawer. On wide windows both sides can be open; on narrower windows
opening one closes the other so navigation stays accessible. Fullscreen hides
both sidebars and retains their selection.

Seek with the timeline. Comma and period are the default frame-step keys for
seekable video. Frame stepping uses decoded timestamps and may need time to
decode backward. **S** (or **Print Screen**) captures the current frame with
active filters; **Ctrl+Shift+S** or **C** captures the untouched decoded frame.
**K** opens original vs. filtered comparison; press play there to watch both
sides update. Settings are available with **Ctrl+,** and Workspace with
**Ctrl+Shift+W**.
Change the theme in **Workspace → Preferences → Appearance → Theme**. It applies
immediately and persists across launches. The same theme selector is available
in **Settings → General → Interface style**. Common shortcuts can be changed
in Workspace → Preferences.

## Review and export

Workspace exposes bookmarks with notes, A–B loop markers and export controls.
Opening another source clears loop markers. Exports include individual PNG
frames, bookmark frames, intervals, image batches and timestamped contact sheets.
Output is written at decoded resolution, with unique names to avoid overwrites.

Choose an external output folder with enough space. The export queue is bounded
to 200 items. Cancellation leaves completed files intact; an active write may
finish. Media files are not modified.

Filtered video preview uses a bounded 960-pixel resolution; filtered exports
process full-resolution frames and can take longer. Original/filtered comparison
lets you inspect the effect. Worker crashes and timeouts can be bypassed during
preview but fail exports. Some native callback errors currently result in an
unchanged frame; see [runtime limits](PLUGIN_REFERENCE.md#frame-and-worker-lifecycle).

## Install and manage filters

Open **Workspace → Filters** to install a folder or `.andiyaplugin` bundle,
load a local/HTTPS catalog, review a publisher and enable a filter.
Python filters need a separately installed Python interpreter.
Choose a native package matching your operating system and architecture.

Filters support order, strength, declared parameter sliders and named presets.
Use Restart to retry a failed worker. Updates keep one previous version for
Rollback; changes to plugin content require approval again.
Plugin permissions describe access but are not an OS sandbox.
For third-party development, see [Create a plugin](../PLUGIN_DEVELOPMENT.md).

## Save work and preferences

Workspace import/export carries bookmarks, notes, playlist, filter presets,
theme, text size and keyboard preferences. Import replaces the corresponding
workspace data after validation. It references media paths and does not copy
media or install plugin binaries; missing files/plugins must be supplied on the
destination computer.

History and portable workspaces remove URL credentials, query strings and
fragments. Consequently, signed URLs may no longer work when reopened from
history. On Windows, save complete URLs as encrypted stream profiles instead.
Profiles are protected for the Windows user account and are excluded from
portable workspaces. This secure profile storage is currently Windows-only.

Diagnostics helps inspect playback and filter performance. It cannot identify
the exact active hardware decoder or provide decoder-drop counts through the
current Qt API. See [implementation notes](IMPLEMENTATION.md) for remaining limits.
