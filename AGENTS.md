# AFTEREFFECT_WRAP — Edge Light Wrap installer package

A student gives you this repository's link and asks for the plug-in to be installed.
Finish the installation on their computer, one short explanation per step, then report.
Read `.agents/skills/aftereffect-wrap/SKILL.md` for teaching the effect afterwards.
This repository is the source of its project skill; do not install or change global agent rules.

## 1. Get the repository

- If the current folder already has `AGENTS.md`, `mac/` and `windows/`, use it.
- Students work in their own fork (`docs/fork-and-update.md`). After the class Git setup,
  `gh auth status` succeeds: fork into the student's account and clone it into Documents.
  `gh` registers this repository as `upstream`. Do not overwrite an existing folder; if it is
  already the student's fork, update it as `docs/fork-and-update.md` section 3 describes.
- Without the Git setup, download the ZIP with curl (no updates by pull). On macOS, if
  `xcode-select -p` fails, do not run `git` (it opens an Xcode tools installer). Never ask the
  student to download the ZIP with a browser: the quarantine flag makes macOS block the plug-in
  and the installer stops.

```sh
cd ~/Documents && gh repo fork hyun2xyz/AFTEREFFECT_WRAP --clone
# without the Git setup:
curl -fL -o ~/Documents/AFTEREFFECT_WRAP.zip https://github.com/hyun2xyz/AFTEREFFECT_WRAP/archive/refs/heads/main.zip
ditto -x -k ~/Documents/AFTEREFFECT_WRAP.zip ~/Documents   # creates AFTEREFFECT_WRAP-main
```

- Windows (any shell; PowerShell commands, not scripts, so no execution policy is involved):

```powershell
Set-Location "$env:USERPROFILE\Documents"; gh repo fork hyun2xyz/AFTEREFFECT_WRAP --clone
# without the Git setup:
Invoke-WebRequest https://github.com/hyun2xyz/AFTEREFFECT_WRAP/archive/refs/heads/main.zip -OutFile "$env:USERPROFILE\Documents\AFTEREFFECT_WRAP.zip"
Expand-Archive "$env:USERPROFILE\Documents\AFTEREFFECT_WRAP.zip" "$env:USERPROFILE\Documents"
```

## 2. Install for the current OS only

Run from the repository root. Only `mac/` on macOS, only `windows/` on Windows.

1. Dry run, then install. The installer verifies `payload.sha256` before changing anything and
   exits without changes when the same version is already installed.
2. If it stops because After Effects is running, ask the student to save and quit AE, then run
   it again. Never quit AE yourself.

macOS (current user only; no administrator password):

```sh
bash "mac/Install Edge Light Wrap.command" --dry-run
bash "mac/Install Edge Light Wrap.command"
```

Windows (Git Bash, PowerShell or cmd):

```powershell
powershell.exe -NoProfile -Command "& '.\windows\install.cmd' --dry-run; exit $LASTEXITCODE"
powershell.exe -NoProfile -Command "& '.\windows\install.cmd'; exit $LASTEXITCODE"
```

On Windows, tell the student first that a UAC prompt will appear and that they should choose
[Yes]; the shared MediaCore folder under Program Files needs it. The command waits for that
answer, so allow a few minutes before a tool timeout. Cancelling the prompt changes nothing.
In a sandboxed agent (for example Codex), writing to the Adobe folders needs the student's
approval of an escalated command; ask for it rather than working around the sandbox.

## 3. Verify and report

- The installer prints `설치 완료` (Mac) or `Install complete` (Windows), or that the same
  version is already installed. Any other ending is a failure: report the exact message.
- macOS: `codesign --verify --deep --strict` on
  `~/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/EdgeLightWrap.plugin`
  and read `~/Library/Application Support/Edge Light Wrap/install-receipt.txt`.
- Windows: read `%ProgramData%\Edge Light Wrap\install-receipt.txt`; its `plugin=` path must
  exist and its `sha256=` must match `windows/payload.sha256`.
- Ask the student to start AE and apply Effect > Hyun2 > Edge Light Wrap. Optionally open
  `example/Native Demo.aep`. Their answer is the only proof the effect loads in AE.
- Report separately: files installed, installer checks passed, and what the student saw in AE.
  For Windows, say that the menu and applying the effect were confirmed in real Windows AE, but
  its render output was not compared yet (`docs/validation.md`); ask them to tell the instructor
  what they see and their AE version.

## Updates

When the instructor publishes a new version, merge `upstream/main` into the student's fork as
`docs/fork-and-update.md` section 3 describes (never reset, force-push or discard their commits),
push to `origin`, then run the installer again. It backs up the old plug-in before replacing it.

## Never

- Change security settings: Gatekeeper, `xattr` quarantine removal, PowerShell execution policy,
  SmartScreen, Defender, SIP. If something is blocked, stop and explain.
- Force-quit AE, use `sudo`, edit the AE application folder, or touch other plug-ins.
- Rebuild from `source/` (needs the Adobe SDK) or edit the payloads unless the instructor asks.
- Report a failed or unconfirmed step as success.

## Remove

```sh
bash "mac/Uninstall Edge Light Wrap.command"
```

```powershell
powershell.exe -NoProfile -Command "& '.\windows\install.cmd' --uninstall; exit $LASTEXITCODE"
```

Removal moves the plug-in to the backup folder listed in `START-HERE.md`; it deletes nothing.

## Maintainers

Checks: `python3 -B -m unittest discover -s tests -v` (CI runs macOS and Windows).
Building and staging new payloads: `docs/build.md`.
