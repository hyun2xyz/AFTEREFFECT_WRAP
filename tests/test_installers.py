"""Installer rehearsals in a throwaway folder (--sandbox-root). Never touches the real Adobe folders."""
import hashlib
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def sha256(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


class Rehearsal(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix="elw-installer-")
        self.addCleanup(temp.cleanup)
        self.temp = pathlib.Path(temp.name)
        self.sandbox = self.temp / "test account with spaces"
        self.sandbox.mkdir()

    def run_ok(self, *args, **kwargs):
        result = self.run_script(*args, **kwargs)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    def run_fails(self, *args, **kwargs):
        result = self.run_script(*args, **kwargs)
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout + result.stderr

    def backups(self, pattern):
        return sorted((self.sandbox / "Support" / "Backups").glob(pattern))


@unittest.skipUnless(sys.platform == "darwin", "macOS installer")
class MacInstallerTests(Rehearsal):
    package = ROOT / "mac"

    def run_script(self, *args, package=None, script="Install Edge Light Wrap.command"):
        path = (package or self.package) / script
        return subprocess.run(["/bin/bash", str(path), "--sandbox-root", str(self.sandbox), *args],
                              capture_output=True, text=True, stdin=subprocess.DEVNULL)

    def test_install_update_lock_tamper_quarantine_and_removal(self):
        target = self.sandbox / "MediaCore" / "EdgeLightWrap.plugin"
        binary = target / "Contents/MacOS/EdgeLightWrap"
        self.run_ok("--dry-run")
        self.assertFalse((self.sandbox / "MediaCore").exists())
        self.run_ok()
        subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", str(target)], check=True)
        self.assertTrue((self.sandbox / "Support/install-receipt.txt").is_file())
        self.assertIn("같은 버전이 이미 설치되어 있습니다", self.run_ok())
        self.assertEqual(self.backups("*.plugin"), [])
        # A changed installed copy is backed up and replaced.
        with open(binary, "ab") as handle:
            handle.write(b"\0")
        self.run_ok()
        self.assertEqual(len(self.backups("EdgeLightWrap-*.plugin")), 1)
        original = sha256(binary)
        with open(binary, "ab") as handle:
            handle.write(b"\0")
        changed = sha256(binary)
        # A held lock refuses to start and keeps the installed copy.
        lock = self.sandbox / "Support/.install-lock"
        lock.mkdir()
        self.run_fails()
        lock.rmdir()
        self.assertEqual(sha256(binary), changed)
        # A tampered package is refused before anything is touched.
        tampered = self.temp / "tampered package"
        shutil.copytree(self.package, tampered, symlinks=True)
        with open(tampered / "EdgeLightWrap.plugin/Contents/Info.plist", "ab") as handle:
            handle.write(b"\n")
        self.run_fails(package=tampered)
        self.assertEqual(sha256(binary), changed)
        # A browser-quarantined package is refused instead of having the flag stripped.
        quarantined = self.temp / "quarantined package"
        shutil.copytree(self.package, quarantined, symlinks=True)
        subprocess.run(["/usr/bin/xattr", "-w", "com.apple.quarantine", "0081;00000000;Safari;",
                        str(quarantined / "EdgeLightWrap.plugin/Contents/MacOS/EdgeLightWrap")], check=True)
        self.assertIn("git clone", self.run_fails(package=quarantined))
        self.run_ok()
        self.assertEqual(sha256(binary), original)
        # Removal moves only this plug-in to the backup folder.
        unrelated = self.sandbox / "MediaCore/Unrelated.plugin"
        unrelated.mkdir()
        self.run_ok(script="Uninstall Edge Light Wrap.command")
        self.assertFalse(target.exists())
        self.assertTrue(unrelated.is_dir())
        self.assertEqual(len(self.backups("Removed-EdgeLightWrap-*.plugin")), 1)
        self.run_ok(script="Uninstall Edge Light Wrap.command")


@unittest.skipUnless(os.name == "nt", "Windows installer")
class WindowsInstallerTests(Rehearsal):
    package = ROOT / "windows"

    def run_script(self, *args, package=None, script="install.cmd"):
        path = (package or self.package) / script
        quoted = " ".join(f'"{a}"' if " " in a else a for a in args)
        # /s keeps cmd from mangling the quoted script path and arguments.
        command = f'cmd.exe /d /s /c ""{path}" --sandbox-root "{self.sandbox}" {quoted}"'
        return subprocess.run(command, capture_output=True, text=True, stdin=subprocess.DEVNULL)

    def test_install_update_lock_tamper_foreign_and_removal(self):
        target = self.sandbox / "MediaCore" / "EdgeLightWrap.aex"
        expected = sha256(self.package / "EdgeLightWrap.aex")
        self.run_ok("--dry-run")
        self.assertFalse((self.sandbox / "MediaCore").exists())
        self.run_ok()
        self.assertEqual(sha256(target), expected)
        self.assertTrue((self.sandbox / "Support/install-receipt.txt").is_file())
        self.assertIn("same version is already installed", self.run_ok())
        self.assertEqual(self.backups("*.aex"), [])
        # A changed installed copy is backed up and replaced.
        with open(target, "ab") as handle:
            handle.write(b"\0")
        self.run_ok()
        self.assertEqual(sha256(target), expected)
        self.assertEqual(len(self.backups("EdgeLightWrap-*.aex")), 1)
        # A held lock refuses to start and keeps the installed copy.
        with open(target, "ab") as handle:
            handle.write(b"\0")
        changed = sha256(target)
        lock = self.sandbox / "Support/.install-lock"
        lock.mkdir()
        self.assertIn("Another install", self.run_fails())
        lock.rmdir()
        self.assertEqual(sha256(target), changed)
        # A tampered package is refused before anything is touched.
        tampered = self.temp / "tampered package"
        shutil.copytree(self.package, tampered)
        with open(tampered / "EdgeLightWrap.aex", "ab") as handle:
            handle.write(b"\0")
        self.assertIn("damaged or changed", self.run_fails(package=tampered))
        self.assertEqual(sha256(target), changed)
        self.run_ok()
        self.assertEqual(sha256(target), expected)
        # Removal moves only this plug-in; another plug-in with the same file name is never touched.
        unrelated = self.sandbox / "MediaCore/Unrelated.aex"
        unrelated.write_bytes(b"MZ unrelated")
        self.run_ok("--uninstall")
        self.assertFalse(target.exists())
        self.assertTrue(unrelated.is_file())
        self.assertEqual(len(self.backups("Removed-EdgeLightWrap-*.aex")), 1)
        self.assertIn("not installed", self.run_ok("--uninstall"))
        target.write_bytes(b"MZ another vendor's plug-in")
        self.assertIn("Another plug-in", self.run_fails())
        self.assertIn("Other plug-ins are not removed", self.run_fails("--uninstall"))
        self.assertEqual(target.read_bytes(), b"MZ another vendor's plug-in")


if __name__ == "__main__":
    unittest.main()
