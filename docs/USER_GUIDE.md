# Using Andiya

Andiya plays local video, audio and images, opens network streams, and provides
frame review, filter comparison and PNG exports. The core is MIT licensed and
works without a paid plugin.

## Open media and navigate

Use **Open** or **Ctrl+O** for a file, and **Ctrl+U** for a stream. The stream
address is visible while entering or reviewing it in the dialog.
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
**K** opens original vs. filtered comparison. Press **Play** to watch matching
frames on both sides. Live comparison adapts the filtered preview resolution
to processing speed. Paused comparisons and saved captures use decoded
resolution. Settings are available with **Ctrl+,** and Workspace with
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

Live filtered video previews start with a 960-pixel limit on the longest edge
and lower the resolution when filters fall behind. Paused video previews use the
960-pixel limit. Filtered exports process full-resolution frames and can take
longer. Original/filtered comparison
lets you inspect the effect. Worker crashes and timeouts can be bypassed during
preview but fail exports. Some native callback errors currently result in an
unchanged frame; see [runtime limits](PLUGIN_REFERENCE.md#frame-and-worker-lifecycle).

## Install and manage filters

Open **Workspace → Filters** to install a folder or `.andiyaplugin` bundle,
load a local/HTTPS catalog, review a publisher and enable a filter.
Python filters need a separate Python 3.9 or newer installation that Andiya can
find through the Windows Python launcher or PATH. If a preview worker cannot
start, its plugin card in **Tools** shows the reason.
Choose a native package matching your operating system and architecture.

To bring back a bundled filter you removed, open **Tools → Install plugin** and
choose **Restore** followed by the filter name. Turn it on and review the
permission prompt when you're ready to use it.

Filters support order, strength, declared parameter sliders and named presets.

### Change plugin settings

1. Open **Tools**, find the plugin, and turn on its switch to see its effect.
2. Click **Settings** on the card. Andiya shows this button when an available
   plugin declares adjustable parameters or actions. You can also open **Workspace →
   Filters** and scroll to the same plugin.
3. Drag a slider and release it to save the value. The number on the right
   shows the setting. For example, **Soft Contrast** has **Contrast** and
   **Warmth** sliders. **Strength** blends the overall effect with the input
   frame. At 100%, Andiya uses the full filtered result.
4. Close the popup when finished. Reopen it or check **Workspace → Filters**
   to see the same saved values. To preview filters while a video plays,
   enable **Show filters during video playback** in **Workspace → Filters**.

### Draw and resize a rectangle

1. Load an image or video. Open **Tools → Redaction Blur → Settings**, then
   click **Draw rectangle** above the sliders. Other plugins show this button
   when they declare a rectangle tool.
2. Start dragging on the picture outside the existing box to draw a new one.
   Drag inside the box to move it. Grab the small square in the **lower-right
   corner** and drag it to resize the box.
3. Select **Apply rectangle** to save the new position and size, or **Cancel**
   to discard your edits. Reopen **Settings** to fine-tune **X** and **Y**
   (position) or **Width** and **Height** (size). These values are fractions
   of the picture. For example, **X = 0.10** and **Width = 0.30** cover the
   area from 10% to 40% of the picture's width.

In Redaction Blur 1.3.0, **Reset rectangle** restores the default position
and size while keeping the current blur radius. Enable the plugin to use
either action. The yellow outline and dark edge show the draft selection
while you drag. The blur moves to the new region when you apply it.

Use **Blur radius** to adjust the amount of blur. For a filtered screenshot,
use **S** in the player or **K → Save filtered**. Check the saved image
before sharing. Use an opaque fill when sensitive text must be fully hidden.
See the [Redaction Blur Word guide](Redaction-Blur-Guide.docx) for the worked
example.

### Remove or reinstall a plugin

Choose **Remove** on any plugin card and confirm to remove it from your Andiya
profile. This also deletes the local files and rollback copy for a plugin you
installed. Use **Install plugin** to add a removed plugin back.

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
