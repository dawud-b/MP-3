#!/usr/bin/env python3
"""
Convert a raw 1920x1080 YCbCr 4:2:2 binary file to an RGB PNG.

Pixel format (as described):
  Each pixel = 16 bits (2 bytes, big-endian or little-endian)
    top 8 bits    = chroma (Cb for even pixels, Cr for odd pixels)
    bottom 8 bits = luma (Y)

  This is YUYV / YUY2 packing, read as 16-bit words:
    word[0]  -> Cb0, Y0   (pixel 0)
    word[1]  -> Cr0, Y1   (pixel 1, shares Cb0/Cr0 with pixel 0)
    word[2]  -> Cb2, Y2   (pixel 2)
    ...

Usage:
    python convert_ycbcr_to_rgb.py input.bin output.png [--be]

Options:
    --be    Treat 16-bit words as big-endian (default: little-endian)
    --swap  Swap chroma byte order (try if colors look wrong)
"""

import sys
import struct
import argparse
import numpy as np
from PIL import Image

WIDTH  = 1920
HEIGHT = 1080
PIXELS = WIDTH * HEIGHT
BYTES  = PIXELS * 2  # 16 bits per pixel


def ycbcr_to_rgb(Y, Cb, Cr):
    """
    BT.601 full-range YCbCr -> RGB conversion.
    Y in [16,235], Cb/Cr in [16,240] (studio swing) — adjust if full-range.
    """
    Y  = Y.astype(np.float32)
    Cb = Cb.astype(np.float32) - 128.0
    Cr = Cr.astype(np.float32) - 128.0

    R = Y                        + 1.402   * Cr
    G = Y - 0.344136 * Cb       - 0.714136 * Cr
    B = Y + 1.772   * Cb

    R = np.clip(R, 0, 255).astype(np.uint8)
    G = np.clip(G, 0, 255).astype(np.uint8)
    B = np.clip(B, 0, 255).astype(np.uint8)
    return R, G, B


def convert(input_path: str, output_path: str, big_endian: bool = False, swap_chroma: bool = False):
    print(f"Reading {input_path} ...")
    with open(input_path, "rb") as f:
        raw = f.read()

    expected = BYTES
    actual   = len(raw)
    if actual != expected:
        print(f"WARNING: expected {expected} bytes ({WIDTH}x{HEIGHT}x2), got {actual}.")
        if actual < expected:
            print("File is too small — padding with zeros.")
            raw = raw + b'\x00' * (expected - actual)
        else:
            print("File is larger than expected — truncating.")
            raw = raw[:expected]

    endian = '>' if big_endian else '<'
    words  = np.frombuffer(raw, dtype=np.dtype(f'{endian}u2')).reshape(HEIGHT, WIDTH)

    # Each 16-bit word: top 8 = chroma, bottom 8 = luma
    chroma = ((words >> 8) & 0xFF).astype(np.uint8)   # Cb or Cr
    luma   = ( words       & 0xFF).astype(np.uint8)   # Y

    if swap_chroma:
        chroma = chroma[:, ::-1]   # reverse chroma byte within word

    # In YUYV 4:2:2, even columns carry Cb, odd columns carry Cr.
    # Each pair of pixels (2n, 2n+1) shares the same Cb and Cr.
    Cb_cols = chroma[:, 0::2]   # even columns → Cb
    Cr_cols = chroma[:, 1::2]   # odd  columns → Cr

    # Upsample Cb/Cr to full width by repeating each sample
    Cb_full = np.repeat(Cb_cols, 2, axis=1)   # (1080, 1920)
    Cr_full = np.repeat(Cr_cols, 2, axis=1)   # (1080, 1920)

    print("Converting YCbCr → RGB ...")
    R, G, B = ycbcr_to_rgb(luma.astype(np.float32),
                            Cb_full.astype(np.float32),
                            Cr_full.astype(np.float32))

    rgb = np.stack([R, G, B], axis=-1)   # (1080, 1920, 3)
    img = Image.fromarray(rgb, mode="RGB")

    print(f"Saving {output_path} ...")
    img.save(output_path)
    print("Done.")


def main():
    parser = argparse.ArgumentParser(description="Convert raw YCbCr 4:2:2 binary to RGB PNG")
    parser.add_argument("input",  help="Input binary file (1920x1080 YUYV)")
    parser.add_argument("output", help="Output PNG file")
    parser.add_argument("--be",   action="store_true", help="Big-endian 16-bit words (default: little-endian)")
    parser.add_argument("--swap", action="store_true", help="Swap chroma byte order within each word")
    args = parser.parse_args()

    convert(args.input, args.output, big_endian=args.be, swap_chroma=args.swap)


if __name__ == "__main__":
    main()