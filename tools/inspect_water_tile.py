#!/usr/bin/env python3
import argparse
import os
import struct
import zlib


class Buffer:
    def __init__(self, data):
        self.data = data
        self.off = 0
        self.length = len(data)

    def get_u8(self):
        v = self.data[self.off]
        self.off += 1
        return v

    def get_i32(self):
        v = struct.unpack_from("<i", self.data, self.off)[0]
        self.off += 4
        return v

    def get_u16(self):
        v = struct.unpack_from("<H", self.data, self.off)[0]
        self.off += 2
        return v

    def get_f32(self):
        v = struct.unpack_from("<f", self.data, self.off)[0]
        self.off += 4
        return v

    def get_utf16(self, byte_len):
        raw = self.data[self.off:self.off + byte_len]
        self.off += byte_len
        return raw.decode("utf-16le", errors="ignore").replace("\r", "")

    def find_token(self, token):
        while self.off < self.length:
            found = self.get_i32()
            if found == token:
                return True
            size = self.get_i32()
            self.off += size
        return False


def read_tsre_file(path):
    data = bytearray(open(path, "rb").read())
    bom = struct.unpack_from("<H", data, 0)[0] if len(data) >= 2 else 0
    if bom != 65279 and len(data) > 16 and data[7] == ord("F"):
        payload = bytes(data[16:])
        try:
            out = zlib.decompress(payload)
        except zlib.error:
            out = zlib.decompress(payload, -zlib.MAX_WBITS)
        return bytes(data[:16]) + out
    if bom == 65279 and len(data) > 34 and data[16] == ord("F"):
        payload = bytes(data[34:])
        try:
            out = zlib.decompress(payload)
        except zlib.error:
            out = zlib.decompress(payload, -zlib.MAX_WBITS)
        return bytes(data[:34]) + out
    return bytes(data)


def parse_t(path):
    b = Buffer(read_tsre_file(path))
    info = {
        "nsamples": None,
        "sample_size": None,
        "floor": None,
        "scale": None,
        "water": None,
        "patches": None,
        "flags": [],
    }

    b.off += 32
    if not b.find_token(136):
        raise RuntimeError("token 136 not found")
    b.off += 5
    for _ in range(6):
        token = b.get_i32()
        offset = b.get_i32()
        body = b.off
        end = body + offset
        if token == 139:
            b.off += 1
            while b.off < end:
                sub = b.get_i32()
                sub_offset = b.get_i32()
                sub_body = b.off
                b.off += 1
                if sub == 140:
                    info["nsamples"] = b.get_i32()
                elif sub == 142:
                    info["floor"] = b.get_f32()
                elif sub == 143:
                    info["scale"] = b.get_f32()
                elif sub == 144:
                    info["sample_size"] = b.get_f32()
                elif sub in (145, 146, 147, 148):
                    strlen = b.get_u16() * 2
                    b.get_utf16(strlen)
                b.off = sub_body + sub_offset
        elif token == 251:
            b.off += 1
            info["water"] = (b.get_f32(), b.get_f32(), b.get_f32(), b.get_f32())
        elif token == 151:
            b.off += 1
            mat_refs = b.get_i32()
            # The material block is not needed for edge analysis.
        elif token == 157:
            b.off += 1
            top_token = b.get_i32()
            top_offset = b.get_i32()
            top_body = b.off
            b.off += 1
            count = b.get_i32()
            for _set in range(count):
                set_token = b.get_i32()
                set_offset = b.get_i32()
                set_body = b.off
                b.off += 1
                for _ in range(3):
                    sub = b.get_i32()
                    sub_offset = b.get_i32()
                    sub_body = b.off
                    if sub == 161:
                        b.off += 1
                        info["patches"] = b.get_i32()
                    elif sub == 163:
                        b.off += 1
                        n = info["patches"]
                        for _patch in range(n * n):
                            _patch_token = b.get_i32()
                            _patch_offset = b.get_i32()
                            _patch_body = b.off
                            b.off += 1
                            info["flags"].append(b.get_i32())
                            b.off += 4 * 6
                            b.off += 4
                            b.off += 4 * 6
                            b.off += 4
                    b.off = sub_body + sub_offset
                b.off = set_body + set_offset
            b.off = top_body + top_offset
        b.off = end
        if b.off >= b.length:
            break
    return info


def read_heights(raw_path, floor, scale, samples):
    raw = open(raw_path, "rb").read()
    vals = struct.unpack_from("<" + "H" * (samples * samples), raw, 0)
    heights = []
    idx = 0
    for z in range(samples):
        row = []
        for x in range(samples):
            row.append(floor + scale * vals[idx])
            idx += 1
        heights.append(row)
    # TSRE duplicates the last row/column in memory.
    heights.append(list(heights[-1]))
    for row in heights:
        row.append(row[-1])
    return heights


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("route")
    ap.add_argument("tile")
    args = ap.parse_args()

    tile_dir = os.path.join(args.route, "tiles")
    terrtex_dir = os.path.join(args.route, "terrtex")
    t_path = os.path.join(tile_dir, args.tile + ".t")
    raw_path = os.path.join(tile_dir, args.tile + "_y.raw")
    info = parse_t(t_path)
    heights = read_heights(raw_path, info["floor"], info["scale"], info["nsamples"])

    samples = info["nsamples"]
    sample_size = info["sample_size"]
    tile_size = samples * sample_size
    patches = info["patches"]
    patch_res = samples // patches
    wsw, wse, wne, wnw = info["water"]

    def water_height(x, z):
        return ((x * z) / (tile_size * tile_size)) * wse + \
            (((tile_size - x) * z) / (tile_size * tile_size)) * wsw + \
            (((tile_size - x) * (tile_size - z)) / (tile_size * tile_size)) * wnw + \
            ((x * (tile_size - z)) / (tile_size * tile_size)) * wne

    def has_water(px, pz):
        return 0 <= px < patches and 0 <= pz < patches and (info["flags"][pz * patches + px] & 0xc0) != 0

    def crosses(a, b):
        return a == 0 or b == 0 or (a < 0 < b) or (a > 0 > b)

    water_patches = [(x, z) for z in range(patches) for x in range(patches) if has_water(x, z)]
    locked = [(x, z) for z in range(patches) for x in range(patches) if info["flags"][z * patches + x] & 1]
    crossings_in_water = 0
    crossings_with_margin = 0
    above_samples_in_water = 0
    below_samples_in_water = 0
    patches_with_crossings = set()
    patches_with_above = set()

    cells_to_scan = set()
    margin_cells = set()
    for x, z in water_patches:
        for sz in range(z * patch_res, min((z + 1) * patch_res, samples)):
            for sx in range(x * patch_res, min((x + 1) * patch_res, samples)):
                cells_to_scan.add((sx, sz))
        for mz in range(max(0, z - 1), min(patches, z + 2)):
            for mx in range(max(0, x - 1), min(patches, x + 2)):
                for sz in range(mz * patch_res, min((mz + 1) * patch_res, samples)):
                    for sx in range(mx * patch_res, min((mx + 1) * patch_res, samples)):
                        margin_cells.add((sx, sz))

    def cell_cross_count(sx, sz):
        x0 = sx * sample_size
        x1 = (sx + 1) * sample_size
        z0 = sz * sample_size
        z1 = (sz + 1) * sample_size
        d00 = heights[sz][sx] - water_height(x0, z0)
        d10 = heights[sz][sx + 1] - water_height(x1, z0)
        d01 = heights[sz + 1][sx] - water_height(x0, z1)
        d11 = heights[sz + 1][sx + 1] - water_height(x1, z1)
        return sum((crosses(d00, d10), crosses(d10, d11), crosses(d01, d11), crosses(d00, d01)))

    for sx, sz in cells_to_scan:
        px = sx // patch_res
        pz = sz // patch_res
        count = cell_cross_count(sx, sz)
        crossings_in_water += count
        if count:
            patches_with_crossings.add((px, pz))
        center = heights[sz][sx] - water_height(sx * sample_size, sz * sample_size)
        if center > 0:
            above_samples_in_water += 1
            patches_with_above.add((px, pz))
        else:
            below_samples_in_water += 1

    for sx, sz in margin_cells:
        crossings_with_margin += cell_cross_count(sx, sz)

    print(f"tile: {args.tile}")
    print(f"nsamples={samples} sample_size={sample_size} patches={patches}")
    print(f"floor={info['floor']:.6f} scale={info['scale']:.9f}")
    print(f"water WSW/WSE/WNE/WNW={info['water']}")
    print(f"water_patches={len(water_patches)} locked_patches={len(locked)}")
    print(f"height_min={min(map(min, heights)):.3f} height_max={max(map(max, heights)):.3f}")
    print(f"water_patch_samples_above={above_samples_in_water} below_or_equal={below_samples_in_water}")
    print(f"crossing_edges_inside_water_patches={crossings_in_water}")
    print(f"crossing_edges_with_1_patch_margin={crossings_with_margin}")
    print(f"water_patches_with_crossings={len(patches_with_crossings)}")
    print(f"water_patches_with_land_poking_above={len(patches_with_above)}")
    if os.path.isdir(terrtex_dir):
        unique = set()
        prefix = args.tile.lower() + "_"
        for name in os.listdir(terrtex_dir):
            lower = name.lower()
            if not lower.startswith(prefix) or not lower.endswith(".ace"):
                continue
            parts = lower[len(prefix):-4].split("_")
            if len(parts) == 2 and parts[0].isdigit() and parts[1].isdigit():
                unique.add((int(parts[1]), int(parts[0])))
        crossing_unique = patches_with_crossings & unique
        print(f"unique_patch_aces={len(unique)}")
        print(f"crossing_patches_with_unique_ace={len(crossing_unique)}")
        print(f"crossing_patches_needing_clone={len(patches_with_crossings - unique)}")
    if water_patches:
        print("water_patch_bounds=x:%d-%d z:%d-%d" % (
            min(x for x, _ in water_patches),
            max(x for x, _ in water_patches),
            min(z for _, z in water_patches),
            max(z for _, z in water_patches),
        ))


if __name__ == "__main__":
    main()
