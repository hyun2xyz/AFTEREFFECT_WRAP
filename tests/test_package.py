"""Checks the shipped mac/ and windows/ payloads. Standard library only; runs on macOS and Windows."""
import hashlib
import json
import os
import pathlib
import plistlib
import subprocess
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests"))
sys.path.insert(0, str(ROOT / "source" / "scripts"))
import pe  # noqa: E402
import pipl  # noqa: E402

MAC = ROOT / "mac"
WIN = ROOT / "windows"
BUNDLE = MAC / "EdgeLightWrap.plugin"
AEX = WIN / "EdgeLightWrap.aex"
HOST_TEST = ROOT / "tests" / "windows" / "ae-host-tests.exe"
AE_EVIDENCE = ROOT / "docs" / "validation" / "macos-ae26.3-2026-10-20.json"
WINDOWS_AE_EVIDENCE = ROOT / "docs" / "validation" / "windows-ae-2026-10-28.json"
WINDOWS_SYSTEM_DLLS = {"kernel32.dll"}


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def payload(folder):
    entries = {}
    for line in (folder / "payload.sha256").read_text(encoding="ascii").splitlines():
        digest, name = line.split("  ", 1)
        entries[name] = digest
    return entries


def mac_pipl():
    return pipl.decode(pipl.rsrc_pipl(BUNDLE / "Contents/Resources/EdgeLightWrap.rsrc"), "mac")


class MacPayloadTests(unittest.TestCase):
    def test_payload_lists_every_bundle_file(self):
        files = {p.relative_to(MAC).as_posix(): sha256(p) for p in BUNDLE.rglob("*") if p.is_file()}
        self.assertEqual(payload(MAC), files)

    def test_bundle_is_the_binary_verified_in_after_effects(self):
        evidence = json.loads(AE_EVIDENCE.read_text(encoding="utf-8"))
        shipped = {p.relative_to(BUNDLE).as_posix(): sha256(p) for p in BUNDLE.rglob("*") if p.is_file()}
        self.assertEqual(shipped, evidence["installed_bundle_files_sha256"])
        self.assertTrue(evidence["checks"]["actual_effect_menu_registered"])

    def test_bundle_identity(self):
        info = plistlib.loads((BUNDLE / "Contents/Info.plist").read_bytes())
        self.assertEqual(info["CFBundleIdentifier"], "com.hyun2xyz.edgelightwrap")
        props = dict(mac_pipl())
        self.assertEqual((props["mi64"], props["ma64"]), ("EffectMain", "EffectMain"))
        self.assertEqual(props["eMNA"], "Hyun2 Edge Light Wrap")

    @unittest.skipUnless(sys.platform == "darwin", "codesign is macOS only")
    def test_ad_hoc_signature_survives_checkout(self):
        subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", str(BUNDLE)], check=True)
        result = subprocess.run(["/usr/bin/lipo", str(BUNDLE / "Contents/MacOS/EdgeLightWrap"), "-archs"],
                                capture_output=True, text=True, check=True)
        self.assertEqual(sorted(result.stdout.split()), ["arm64", "x86_64"])


class WindowsPayloadTests(unittest.TestCase):
    def setUp(self):
        self.pe = pe.PEFile(AEX)

    def test_payload_hash(self):
        self.assertEqual(payload(WIN), {"EdgeLightWrap.aex": sha256(AEX)})

    def test_aex_is_the_binary_seen_in_windows_after_effects(self):
        evidence = json.loads(WINDOWS_AE_EVIDENCE.read_text(encoding="utf-8"))
        self.assertEqual(evidence["payload_sha256"], {"EdgeLightWrap.aex": sha256(AEX)})
        self.assertTrue(evidence["checks"]["actual_effect_menu_registered"])

    def test_x64_dll_with_ae_entry_points(self):
        self.assertTrue(self.pe.is_x64_dll)
        self.assertTrue({"EffectMain", "PluginDataEntryFunction2"} <= set(self.pe.exports()))

    def test_needs_only_windows_system_libraries(self):
        for name in self.pe.imports():
            lower = name.lower()
            self.assertTrue(lower in WINDOWS_SYSTEM_DLLS or lower.startswith("api-ms-win-crt-"), name)

    def test_pipl_matches_the_mac_plugin(self):
        resources = self.pe.resources("PiPL", 16000)
        self.assertEqual(list(resources), [0x409])
        win = dict(pipl.decode(resources[0x409], "win"))
        mac = dict(mac_pipl())
        self.assertEqual(win.pop("8664"), "EffectMain")
        mac.pop("mi64"), mac.pop("ma64")
        self.assertEqual(win, mac)


class LauncherTests(unittest.TestCase):
    def test_windows_scripts_are_ascii_crlf(self):
        for path in WIN.glob("*.cmd"):
            data = path.read_bytes()
            data.decode("ascii")
            self.assertNotIn(b"\n", data.replace(b"\r\n", b""), path.name)
            self.assertNotIn(b"ExecutionPolicy", data, path.name)

    def test_double_click_launchers_call_the_installer(self):
        install = (WIN / "Install Edge Light Wrap.cmd").read_text(encoding="ascii")
        uninstall = (WIN / "Uninstall Edge Light Wrap.cmd").read_text(encoding="ascii")
        self.assertIn('call "%~dp0install.cmd" --pause', install)
        self.assertIn('call "%~dp0install.cmd" --uninstall --pause', uninstall)

    def test_mac_scripts_are_lf_and_parse(self):
        for path in MAC.glob("*.command"):
            self.assertNotIn(b"\r", path.read_bytes(), path.name)
            if os.name == "posix":
                subprocess.run(["/bin/bash", "-n", str(path)], check=True)


@unittest.skipUnless(os.name == "nt", "Windows loader checks")
class WindowsLoaderTests(unittest.TestCase):
    """What After Effects does first: load the .aex, find the PiPL by name, resolve the entry point."""

    def test_windows_loads_plugin_and_finds_pipl(self):
        import ctypes
        from ctypes import wintypes
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.FindResourceW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, wintypes.LPCWSTR]
        kernel32.FindResourceW.restype = ctypes.c_void_p
        kernel32.SizeofResource.argtypes = [wintypes.HMODULE, ctypes.c_void_p]
        kernel32.LoadResource.argtypes = [wintypes.HMODULE, ctypes.c_void_p]
        kernel32.LoadResource.restype = ctypes.c_void_p
        kernel32.LockResource.argtypes = [ctypes.c_void_p]
        kernel32.LockResource.restype = ctypes.c_void_p
        kernel32.GetProcAddress.argtypes = [wintypes.HMODULE, ctypes.c_char_p]
        kernel32.GetProcAddress.restype = ctypes.c_void_p
        module = ctypes.WinDLL(str(AEX))
        handle = wintypes.HMODULE(module._handle)
        for name in (b"EffectMain", b"PluginDataEntryFunction2"):
            self.assertTrue(kernel32.GetProcAddress(handle, name), name)
        info = kernel32.FindResourceW(handle, ctypes.c_void_p(16000), "PiPL")
        self.assertTrue(info, ctypes.get_last_error())
        size = kernel32.SizeofResource(handle, info)
        data = ctypes.string_at(kernel32.LockResource(kernel32.LoadResource(handle, info)), size)
        self.assertEqual(data, pe.PEFile(AEX).resource("PiPL", 16000))

    def test_plugin_renders_in_windows_host_test(self):
        """tests/windows/ae-host-tests.exe is the source/tests/AEHostTests.cpp host, built with MinGW."""
        result = subprocess.run([str(HOST_TEST), str(AEX)], capture_output=True, text=True, timeout=300)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.count("PASS:"), 3, result.stdout)


if __name__ == "__main__":
    unittest.main()
