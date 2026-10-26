#!/usr/bin/env python3
"""Copy built plug-ins into mac/ and windows/ and write their payload.sha256.

  python3 source/scripts/stage.py --mac build/macos/EdgeLightWrap.plugin
  python3 source/scripts/stage.py --windows build/windows/EdgeLightWrap.aex \
      --host-test build/windows/ae-host-tests.exe

Only the generated payload paths below are replaced. Nothing is installed.
"""
import argparse
import hashlib
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
MAC_BUNDLE = ROOT / "mac" / "EdgeLightWrap.plugin"
WIN_AEX = ROOT / "windows" / "EdgeLightWrap.aex"
HOST_TEST = ROOT / "tests" / "windows" / "ae-host-tests.exe"


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_payload(folder, files):
    lines = [f"{sha256(p)}  {p.relative_to(folder).as_posix()}" for p in sorted(files)]
    (folder / "payload.sha256").write_text("\n".join(lines) + "\n", encoding="ascii", newline="\n")
    print(f"{folder / 'payload.sha256'}: {len(lines)} files")


def stage_mac(bundle):
    bundle = pathlib.Path(bundle)
    subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", str(bundle)], check=True)
    if MAC_BUNDLE.exists():
        shutil.rmtree(MAC_BUNDLE)
    shutil.copytree(bundle, MAC_BUNDLE, symlinks=True)
    subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", str(MAC_BUNDLE)], check=True)
    write_payload(MAC_BUNDLE.parent, [p for p in MAC_BUNDLE.rglob("*") if p.is_file()])


def stage_windows(aex, host_test=None):
    shutil.copyfile(aex, WIN_AEX)
    write_payload(WIN_AEX.parent, [WIN_AEX])
    if host_test:
        HOST_TEST.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(host_test, HOST_TEST)
        print(HOST_TEST)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mac", help="built EdgeLightWrap.plugin")
    parser.add_argument("--windows", help="built EdgeLightWrap.aex")
    parser.add_argument("--host-test", help="built Windows ae-host-tests.exe (CI runtime check)")
    args = parser.parse_args(argv)
    if not (args.mac or args.windows):
        parser.error("pass --mac and/or --windows")
    if args.mac:
        stage_mac(args.mac)
    if args.windows:
        stage_windows(args.windows, args.host_test)
    return 0


if __name__ == "__main__":
    sys.exit(main())
