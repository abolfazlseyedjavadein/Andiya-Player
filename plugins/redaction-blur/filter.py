from math import ceil, floor, isfinite


def _number(parameters, key, default, minimum, maximum):
    value = float(parameters.get(key, default))
    if not isfinite(value) or not minimum <= value <= maximum:
        raise ValueError(f"Invalid redaction parameter: {key}")
    return value


def configure(parameters):
    global _rectangle, _radius
    _rectangle = (
        _number(parameters, "x", 0.1, 0, 1),
        _number(parameters, "y", 0.1, 0, 1),
        _number(parameters, "width", 0.3, 0.01, 1),
        _number(parameters, "height", 0.2, 0.01, 1),
    )
    _radius = round(_number(parameters, "radius", 16, 1, 64))


configure({})


def _blur_pass(source, destination, start, lines, length, line_step, pixel_step):
    for line in range(lines):
        offset = start + line * line_step
        for channel in range(3):
            channel_start = offset + channel
            left, right, total = 0, 0, 0
            for pixel in range(length):
                next_right = min(pixel + _radius + 1, length)
                while right < next_right:
                    total += source[channel_start + right * pixel_step]
                    right += 1
                next_left = max(0, pixel - _radius)
                while left < next_left:
                    total -= source[channel_start + left * pixel_step]
                    left += 1
                destination[channel_start + pixel * pixel_step] = total // (right - left)


def process_frame(width, height, stride, pixel_format, timestamp_us, data):
    if pixel_format != 1 or width < 1 or height < 1 or stride < width * 4 or len(data) != stride * height:
        raise ValueError("Redaction Blur requires a valid BGRA32 frame")

    x, y, region_width, region_height = _rectangle
    left = floor(x * width)
    top = floor(y * height)
    right = min(width, ceil((x + region_width) * width))
    bottom = min(height, ceil((y + region_height) * height))
    if left >= right or top >= bottom:
        raise ValueError("The redaction rectangle is outside the frame")

    columns = right - left
    rows = bottom - top
    start = top * stride + left * 4
    intermediate = bytearray(data)
    output = bytearray(data)
    _blur_pass(data, intermediate, start, rows, columns, stride, 4)
    _blur_pass(intermediate, output, start, columns, rows, 4, stride)
    return bytes(output)
