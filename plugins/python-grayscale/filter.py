"""Example Andiya Python plugin: converts frames to grayscale.

This demonstrates the out-of-process Python plugin bridge
(runtime/python/andiya_python_host.py). Andiya hands this function BGRA32
buffers -- 4 bytes per pixel (Blue, Green, Red, Alpha) -- laid out row by
row, `stride` bytes per row (stride >= width * 4, extra bytes are padding).
"""

PLUGIN_ID = "example.python-grayscale"


def process_frame(width, height, stride, pixel_format, timestamp_us, data):
    if pixel_format != 1:  # ANDIYA_PIXEL_FORMAT_BGRA32
        return None

    buffer = bytearray(data)
    for y in range(height):
        row_start = y * stride
        for x in range(width):
            offset = row_start + x * 4
            b, g, r = buffer[offset], buffer[offset + 1], buffer[offset + 2]
            # Perceptual (luma) weighting, same coefficients video codecs use.
            gray = (r * 299 + g * 587 + b * 114) // 1000
            buffer[offset] = gray
            buffer[offset + 1] = gray
            buffer[offset + 2] = gray
            # buffer[offset + 3] (alpha) is left untouched.
    return bytes(buffer)
