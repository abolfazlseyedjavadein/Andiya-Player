"""MIT-licensed BGRA inversion example; no third-party dependencies."""
import math

_amount = 1.0


def configure(parameters):
    global _amount
    amount = float(parameters.get("amount", 1.0))
    if not math.isfinite(amount) or not 0.0 <= amount <= 1.0:
        raise ValueError("amount must be finite and between zero and one")
    _amount = amount


def process_frame(width, height, stride, pixel_format, timestamp_us, data):
    if pixel_format != 1 or stride < width * 4 or len(data) != stride * height:
        raise ValueError("Expected a same-sized BGRA32 frame")
    output = bytearray(data)
    for y in range(height):
        for x in range(width):
            offset = y * stride + x * 4
            for channel in range(3):
                original = data[offset + channel]
                output[offset + channel] = round(original + _amount * (255 - 2 * original))
    return bytes(output)
