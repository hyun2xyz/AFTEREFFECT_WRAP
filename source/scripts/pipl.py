#!/usr/bin/env python3
"""Encode and decode the Edge Light Wrap PiPL resource without Adobe's PiPLtool.

macOS uses Rez with native/mac/EdgeLightWrapPiPL.r. Windows has no Rez, so this
script writes the same properties as a little-endian PiPL in a .rc file.
The mac encoding exists to prove this encoder against the Rez output.

  python3 pipl.py --rc OUT.rc --sdk "$AE_SDK_ROOT"
  python3 pipl.py --compare-rsrc EdgeLightWrap.rsrc --sdk "$AE_SDK_ROOT"
  python3 pipl.py --dump-pe EdgeLightWrap.aex
"""
import argparse
import pathlib
import re
import struct
import sys

ENTRY_POINT = "EffectMain"
# Same order and values as native/mac/EdgeLightWrapPiPL.r. "code" expands to the
# platform entry points. Spec version comes from the SDK headers at build time.
PROPERTIES = [
    ("kind", "eFKT"),
    ("name", "Edge Light Wrap"),
    ("catg", "Hyun2"),
    ("code", ENTRY_POINT),
    ("ePVR", (2, 0)),
    ("eSVR", None),
    ("eVER", 1050113),          # PF_VERSION(2, 0, 0, PF_Stage_RELEASE, 1)
    ("eINF", 0),
    ("eGLO", 0x02000000),       # PF_OutFlag_DEEP_COLOR_AWARE
    ("eGL2", 0x08001400),       # SMART_RENDER | FLOAT_COLOR_AWARE | THREADED_RENDERING
    ("eMNA", "Hyun2 Edge Light Wrap"),
    ("aeFL", 0),
    ("eURL", "https://github.com/hyun2xyz/edge-light-wrap"),
]
PSTRINGS = {"name", "catg", "eMNA", "eURL", "mi64", "ma64"}
CSTRINGS = {"8664"}
SHORT_PAIRS = {"ePVR", "eSVR"}
LONGS = {"eVER", "eGLO", "eGL2", "aeFL"}


def spec_version(sdk_root):
    header = pathlib.Path(sdk_root, "Examples/Headers/AE_EffectVers.h").read_text(errors="replace")
    values = [int(re.search(rf"#define\s+{name}\s+(\d+)", header).group(1))
              for name in ("PF_PLUG_IN_VERSION", "PF_PLUG_IN_SUBVERS")]
    return tuple(values)


def expand(platform, spec):
    for key, value in PROPERTIES:
        if key == "code":
            keys = ["mi64", "ma64"] if platform == "mac" else ["8664"]
            for code_key in keys:
                yield code_key, value
        else:
            yield key, spec if key == "eSVR" else value


def encode(platform, spec):
    """Rez layout on mac (big-endian, padding outside the length); Windows layout
    as written by Adobe's PiPLtool (little-endian, reversed codes, strings padded
    inside the length, 4-byte eINF)."""
    win = platform == "win"
    order = "<" if win else ">"
    fourcc = (lambda c: c.encode("ascii")[::-1]) if win else (lambda c: c.encode("ascii"))
    pad = lambda data: data + b"\0" * (-len(data) % 4)
    props = list(expand(platform, spec))
    out = bytearray(b"\x01\x00" if win else b"")
    out += struct.pack(order + "II", 0, len(props))
    for key, value in props:
        if key == "kind":
            body = fourcc(value)
        elif key in PSTRINGS:
            raw = value.encode("ascii")
            body = bytes([len(raw)]) + raw
            body = pad(body) if win else body
        elif key in CSTRINGS:
            body = pad(value.encode("ascii") + b"\0")
        elif key in SHORT_PAIRS:
            body = struct.pack(order + "HH", *value)
        elif key == "eINF":
            body = struct.pack(order + ("I" if win else "H"), value)
        elif key in LONGS:
            body = struct.pack(order + "I", value)
        else:
            raise KeyError(key)
        out += fourcc("8BIM") + fourcc(key) + struct.pack(order + "II", 0, len(body)) + body
        if not win:
            out += b"\0" * (-len(body) % 4)
    return bytes(out)


def decode(data, platform):
    """Return [(key, value)] from a PiPL resource; values as in PROPERTIES."""
    win = platform == "win"
    order = "<" if win else ">"
    code = (lambda b: b[::-1].decode("ascii")) if win else (lambda b: b.decode("ascii"))
    offset = 2 if win else 0
    if win and data[:2] != b"\x01\x00":
        raise ValueError("Windows PiPL must start with the reserved word 1")
    version, count = struct.unpack_from(order + "II", data, offset)
    if version != 0:
        raise ValueError(f"unexpected PiPL version {version}")
    offset += 8
    props = []
    for _ in range(count):
        vendor, key = code(data[offset:offset + 4]), code(data[offset + 4:offset + 8])
        _, length = struct.unpack_from(order + "II", data, offset + 8)
        body = data[offset + 16:offset + 16 + length]
        if vendor != "8BIM" or len(body) != length:
            raise ValueError(f"bad property {vendor}/{key} at {offset}")
        if key == "kind":
            value = code(body)
        elif key in PSTRINGS:
            value = body[1:1 + body[0]].decode("ascii")
        elif key in CSTRINGS:
            value = body.split(b"\0", 1)[0].decode("ascii")
        elif key in SHORT_PAIRS:
            value = struct.unpack(order + "HH", body)
        elif key == "eINF":
            value = struct.unpack(order + ("I" if len(body) == 4 else "H"), body)[0]
        else:
            value = struct.unpack(order + "I", body)[0]
        props.append((key, value))
        offset += 16 + length + (0 if win else -length % 4)
    if offset != len(data):
        raise ValueError(f"{len(data) - offset} trailing bytes after the last property")
    return props


def rsrc_pipl(path):
    """Extract 'PiPL' id 16000 from a classic resource file written by Rez -useDF."""
    data = pathlib.Path(path).read_bytes()
    data_offset, map_offset = struct.unpack_from(">II", data, 0)
    type_list = map_offset + struct.unpack_from(">H", data, map_offset + 24)[0]
    for t in range(struct.unpack_from(">H", data, type_list)[0] + 1):
        rtype, count, ref_offset = struct.unpack_from(">4sHH", data, type_list + 2 + 8 * t)
        if rtype != b"PiPL":
            continue
        for r in range(count + 1):
            ref = type_list + ref_offset + 12 * r
            rid, _, attrs_offset = struct.unpack_from(">hHI", data, ref)
            if rid == 16000:
                start = data_offset + (attrs_offset & 0xFFFFFF)
                length = struct.unpack_from(">I", data, start)[0]
                return data[start + 4:start + 4 + length]
    raise ValueError("PiPL 16000 not found")


def write_rc(path, spec):
    data = encode("win", spec)
    escaped = "".join(f"\\x{b:02x}" for b in data)
    lines = ["// Generated by source/scripts/pipl.py. Do not edit.",
             "LANGUAGE 0x09, 0x01",
             f'16000 PiPL DISCARDABLE BEGIN "{escaped}" END', ""]
    pathlib.Path(path).write_text("\n".join(lines), encoding="ascii")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--sdk", help="Adobe After Effects SDK root (for the effect spec version)")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--rc", help="write the Windows PiPL .rc file")
    mode.add_argument("--compare-rsrc", help="check this encoder against a Rez-built .rsrc")
    mode.add_argument("--dump-pe", help="print the PiPL embedded in a Windows .aex")
    args = parser.parse_args(argv)
    if args.dump_pe:
        sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "tests"))
        import pe  # noqa: E402
        for key, value in decode(pe.PEFile(args.dump_pe).resource("PIPL", 16000), "win"):
            print(f"{key}: {value}")
        return 0
    if not args.sdk:
        parser.error("--sdk is required")
    spec = spec_version(args.sdk)
    if args.rc:
        write_rc(args.rc, spec)
        print(f"Wrote {args.rc} (effect spec {spec[0]}.{spec[1]})")
        return 0
    rez = rsrc_pipl(args.compare_rsrc)
    ours = encode("mac", spec)
    if rez != ours:
        print("MISMATCH: encoder differs from Rez", file=sys.stderr)
        print(decode(rez, "mac"), file=sys.stderr)
        print(decode(ours, "mac"), file=sys.stderr)
        return 1
    win = dict(decode(encode("win", spec), "win"))
    mac = dict(decode(rez, "mac"))
    assert win.pop("8664") == mac.pop("mi64") == mac.pop("ma64") == ENTRY_POINT
    assert win == mac, (win, mac)
    print(f"PASS: encoder matches Rez byte-for-byte ({len(rez)} bytes); Windows properties match mac")
    return 0


if __name__ == "__main__":
    sys.exit(main())
