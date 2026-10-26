"""Minimal read-only PE32+ reader for checking the Windows .aex on any OS."""
import struct


class PEFile:
    def __init__(self, path):
        with open(path, "rb") as handle:
            self.data = handle.read()
        if self.data[:2] != b"MZ":
            raise ValueError("not a PE file (no MZ header)")
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe:pe + 4] != b"PE\0\0":
            raise ValueError("not a PE file (no PE signature)")
        self.machine, sections, _, _, _, optional_size, self.characteristics = \
            struct.unpack_from("<HHIIIHH", self.data, pe + 4)
        optional = pe + 24
        if struct.unpack_from("<H", self.data, optional)[0] != 0x20B:
            raise ValueError("not a 64-bit PE32+ image")
        count = struct.unpack_from("<I", self.data, optional + 108)[0]
        self.directories = [struct.unpack_from("<II", self.data, optional + 112 + 8 * i) for i in range(count)]
        table = optional + optional_size
        self.sections = []
        for i in range(sections):
            name, vsize, va, raw_size, raw = struct.unpack_from("<8sIIII", self.data, table + 40 * i)
            self.sections.append((name.rstrip(b"\0").decode("ascii", "replace"), va, max(vsize, raw_size), raw))

    @property
    def is_x64_dll(self):
        return self.machine == 0x8664 and bool(self.characteristics & 0x2000)

    def offset(self, rva):
        for _, va, size, raw in self.sections:
            if va <= rva < va + size:
                return raw + rva - va
        raise ValueError(f"RVA {rva:#x} is outside every section")

    def cstring(self, rva):
        start = self.offset(rva)
        return self.data[start:self.data.index(b"\0", start)].decode("ascii")

    def exports(self):
        rva, size = self.directories[0]
        if not size:
            return []
        base = self.offset(rva)
        count = struct.unpack_from("<I", self.data, base + 24)[0]   # NumberOfNames
        names = struct.unpack_from("<I", self.data, base + 32)[0]   # AddressOfNames
        return sorted(self.cstring(struct.unpack_from("<I", self.data, self.offset(names) + 4 * i)[0]) for i in range(count))

    def imports(self):
        rva, size = self.directories[1]
        found = []
        entry = self.offset(rva) if size else None
        while entry is not None:
            name_rva = struct.unpack_from("<I", self.data, entry + 12)[0]
            if not name_rva:
                break
            found.append(self.cstring(name_rva))
            entry += 20
        return sorted(found, key=str.lower)

    def resources(self, rtype, rid):
        """{language id: data} for resource type name `rtype` (case-insensitive) and integer id."""
        rva, size = self.directories[2]
        if not size:
            return {}
        root = self.offset(rva)

        def entries(directory):
            named, ids = struct.unpack_from("<HH", self.data, directory + 12)
            for i in range(named + ids):
                key, target = struct.unpack_from("<II", self.data, directory + 16 + 8 * i)
                if key & 0x80000000:
                    at = root + (key & 0x7FFFFFFF)
                    length = struct.unpack_from("<H", self.data, at)[0]
                    key = self.data[at + 2:at + 2 + 2 * length].decode("utf-16-le")
                yield key, root + (target & 0x7FFFFFFF), bool(target & 0x80000000)

        found = {}
        for key, types, is_dir in entries(root):
            if not (is_dir and isinstance(key, str) and key.upper() == rtype.upper()):
                continue
            for name, langs, is_dir in entries(types):
                if not (is_dir and name == rid):
                    continue
                for lang, leaf, _ in entries(langs):
                    data_rva, data_size = struct.unpack_from("<II", self.data, leaf)
                    start = self.offset(data_rva)
                    found[lang] = self.data[start:start + data_size]
        return found

    def resource(self, rtype, rid):
        found = self.resources(rtype, rid)
        if len(found) != 1:
            raise KeyError(f"expected one {rtype}/{rid} resource, found {len(found)}")
        return next(iter(found.values()))
