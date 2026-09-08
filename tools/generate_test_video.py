"""Write a six-second 128x72, 10 fps uncompressed AVI fixture outside the source tree."""
import struct
import sys
from pathlib import Path

def chunk(tag, data):
    return tag + struct.pack("<I", len(data)) + data + (b"\0" if len(data) % 2 else b"")
def listing(tag, data):
    return chunk(b"LIST", tag + data)
def generate(path):
    width, height, fps, count = 128, 72, 10, 60
    frame_size = width * height * 3
    avih = struct.pack("<14I", 100000, frame_size * fps, 0, 16, count, 0, 1, frame_size, width, height, 0, 0, 0, 0)
    strh = struct.pack("<4s4sIHH8I4h", b"vids", b"DIB ", 0, 0, 0, 0, 1, fps, 0, count, frame_size, 0xffffffff, 0, 0, 0, width, height)
    strf = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 24, 0, frame_size, 0, 0, 0, 0)
    header = listing(b"hdrl", chunk(b"avih", avih) + listing(b"strl", chunk(b"strh", strh) + chunk(b"strf", strf)))
    frames, index, offset = [], [], 4
    for number in range(count):
        data = bytes((number * 4 % 256, 90, 180)) * (width * height)
        item = chunk(b"00db", data)
        frames.append(item)
        index.append(struct.pack("<4sIII", b"00db", 16, offset, len(data)))
        offset += len(item)
    body = b"AVI " + header + listing(b"movi", b"".join(frames)) + chunk(b"idx1", b"".join(index))
    Path(path).write_bytes(b"RIFF" + struct.pack("<I", len(body)) + body)
if __name__ == "__main__":
    generate(sys.argv[1])
