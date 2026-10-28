---
name: aftereffect-wrap
description: Install the Edge Light Wrap native After Effects effect on a student's macOS or Windows computer from this repository's prebuilt mac/ or windows/ payload, verify the install, then teach the effect order and controls. Use in this repository when a student shares its link or asks to install, update, remove or use Edge Light Wrap.
---

# Aftereffect Wrap

Repository-bound skill. Run from the repository root that contains `mac/` and `windows/`.
Installation steps live in [AGENTS](../../../AGENTS.md); effect usage in [usage](../../../docs/usage.md);
what has and has not been verified per OS in [validation](../../../docs/validation.md).

## First interaction

1. Inspect the actual OS, shell and whether After Effects is installed or running.
2. Get the repository as `AGENTS.md` describes, then dry-run and run only the matching installer.
3. Explain each step in one or two sentences before running it. On Windows, warn about the
   UAC prompt before it appears.
4. After the installer succeeds, ask the student to open AE and apply the effect.

## Non-negotiable rules

- Prebuilt payloads only. `payload.sha256` must pass; never edit, re-sign or rebuild the plug-in
  during a student install.
- No security workarounds: no quarantine removal, Gatekeeper or execution-policy changes,
  `sudo`, or force-quitting AE. A refused step is reported, not bypassed.
- Installed, verified on disk, and seen working in AE are three different results. Report each.
- Windows real AE: the menu and applying the effect were confirmed on 2026-10-28; the render
  output was not compared. Do not claim more than that.

## Teaching the effect

- Order on the subject layer: Object Matte / Roto Brush / keying → Advanced Spill Suppressor →
  Edge Light Wrap. Light wrap before the matte has no alpha edge to work on.
- Set Background Layer. Use comp-sized precomps for subject and background; put background
  scale/position inside its precomp.
- Debug with View = Edge Matte, then Strength 100% briefly, then back to Composite.
- Defaults: Edge Width 3 px, Edge Feather 1.5 px, Wrap Spread 4 px, Wrap Softness 6 px,
  Background Blur 25 px, Strength 40%.
