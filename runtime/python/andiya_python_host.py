"""Andiya Python plugin host.

Andiya spawns one instance of this script per *enabled* Python image-filter
plugin (see PythonImageFilterHost.cpp). It loads the user's plugin module,
then loops reading raw video/image frames from stdin and writing filtered
frames back to stdout using a small fixed binary header, so no serialization
library or IPC framework is required on either side.

A crashing or hanging plugin can only take down this one child process --
Andiya bypasses the frame and keeps playing.

Wire format (all integers little-endian, no padding):

  Request  (Andiya -> plugin), struct format "<IIIIIqQ":
    uint32 magic         = 0x414E4652  ("ANFR")
    uint32 width
    uint32 height
    uint32 stride
    uint32 pixel_format    (1 = BGRA32 byte order, the only format today)
    int64  timestamp_us
    uint64 data_size
    <data_size bytes of pixel data follow immediately>

  Response (plugin -> Andiya), struct format "<IiIIIIQ":
    uint32 magic         = 0x414E5253  ("ANRS")
    int32  status          (0 = ok, 1 = intentional bypass, negative = failure)
    uint32 width
    uint32 height
    uint32 stride
    uint32 pixel_format
    uint64 data_size
    <data_size bytes of pixel data follow immediately, only if status == 0>

A plugin module must define:

    def process_frame(width, height, stride, pixel_format, timestamp_us, data):
        '''`data` is a read-only bytes-like buffer of length stride * height.
        Return a bytes-like object of the exact same length to replace the
        frame, or return None to bypass (skip) this frame untouched.'''
        ...

See plugins/python-grayscale/filter.py for a complete minimal example.
"""

import importlib.util
import struct
import sys
import json
from pathlib import Path
sys.dont_write_bytecode = True

REQUEST_MAGIC = 0x414E4652
RESPONSE_MAGIC = 0x414E5253
REQUEST_FORMAT = "<IIIIIqQ"
REQUEST_SIZE = struct.calcsize(REQUEST_FORMAT)
RESPONSE_FORMAT = "<IiIIIIQ"


def _read_exact(stream, size):
    """Reads exactly `size` bytes, or returns None on EOF (pipe closed)."""
    chunks = []
    remaining = size
    while remaining > 0:
        chunk = stream.read(remaining)
        if not chunk:
            return None
        chunks.append(chunk)
        remaining -= len(chunk)
    return b"".join(chunks)


def _load_plugin(script_path):
    sys.path.insert(0, str(Path(script_path).resolve().parent))
    spec = importlib.util.spec_from_file_location("andiya_plugin", script_path)
    if spec is None or spec.loader is None:
        raise ImportError(f"could not load plugin script: {script_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    if not hasattr(module, "process_frame"):
        raise AttributeError("plugin script must define process_frame(...)")
    return module


def main():
    if len(sys.argv) < 2:
        sys.stderr.write("usage: andiya_python_host.py <plugin_script.py>\n")
        return 2

    stdin = sys.stdin.buffer
    stdout = sys.stdout.buffer
    sys.stdout = sys.stderr  # Keep plugin prints out of the binary protocol.

    try:
        module = _load_plugin(sys.argv[1])
        parameters = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {}
        if hasattr(module, 'configure'):
            module.configure(parameters)
    except Exception as exc:  # noqa: BLE001 - report once and exit
        sys.stderr.write(f"andiya_python_host: failed to load plugin: {exc}\n")
        return 1

    sys.stderr.write("andiya_python_host: ready\n")
    sys.stderr.flush()

    while True:
        header_bytes = _read_exact(stdin, REQUEST_SIZE)
        if header_bytes is None:
            break  # Andiya closed the pipe; shut down quietly.

        magic, width, height, stride, pixel_format, timestamp_us, data_size = \
            struct.unpack(REQUEST_FORMAT, header_bytes)
        if (magic != REQUEST_MAGIC or pixel_format != 1 or not 0 < width <= 16384
                or not 0 < height <= 16384 or stride != width * 4
                or data_size != stride * height or data_size > 268435456):
            sys.stderr.write("andiya_python_host: bad frame magic, stopping\n")
            break

        data = _read_exact(stdin, data_size)
        if data is None:
            break

        status = 0
        output = None
        try:
            output = module.process_frame(width, height, stride, pixel_format,
                                          timestamp_us, data)
        except Exception as exc:  # noqa: BLE001 - never let a plugin bug kill the app
            sys.stderr.write(f"andiya_python_host: process_frame raised: {exc}\n")
            status = -1

        if output is None:
            status = status or 1
            output = b""
        elif len(output) != len(data):
            sys.stderr.write(
                "andiya_python_host: process_frame returned "
                f"{len(output)} bytes, expected {len(data)}; bypassing frame\n")
            status = -1
            output = b""

        response = struct.pack(RESPONSE_FORMAT, RESPONSE_MAGIC, status,
                               width, height, stride, pixel_format, len(output))
        stdout.write(response)
        if output:
            stdout.write(output)
        stdout.flush()

    return 0


if __name__ == "__main__":
    sys.exit(main())
