#!/bin/bash
# Moves the installed Edge Light Wrap plug-in to a recoverable backup folder.
#   --sandbox-root DIR   rehearse in DIR instead of the real Adobe folder (tests)
set -euo pipefail
SANDBOX_ROOT=''
if [[ $# -gt 0 ]]; then
  [[ $# -eq 2 && "$1" == --sandbox-root ]] || { echo '알 수 없는 옵션' >&2; exit 2; }
  SANDBOX_ROOT="$2"
fi
if [[ -n "$SANDBOX_ROOT" ]]; then
  [[ "$SANDBOX_ROOT" == /* && "$SANDBOX_ROOT" != / && -d "$SANDBOX_ROOT" && ! -L "$SANDBOX_ROOT" ]] || exit 1
  SANDBOX_ROOT="$(cd "$SANDBOX_ROOT" && pwd -P)"
  [[ "$SANDBOX_ROOT" != "$HOME" ]] || exit 1
  PLUGIN_DIR="$SANDBOX_ROOT/MediaCore"
  SUPPORT_DIR="$SANDBOX_ROOT/Support"
else
  PLUGIN_DIR="$HOME/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore"
  SUPPORT_DIR="$HOME/Library/Application Support/Edge Light Wrap"
fi
TARGET="$PLUGIN_DIR/EdgeLightWrap.plugin"
if [[ -z "$SANDBOX_ROOT" ]] && { /usr/bin/pgrep -x 'After Effects' >/dev/null || /usr/bin/pgrep -x 'After Effects Render Engine' >/dev/null; }; then
  echo 'After Effects에서 작업을 저장하고 직접 종료한 뒤 다시 실행하세요.' >&2; exit 1
fi
for path in "$PLUGIN_DIR" "$SUPPORT_DIR" "$SUPPORT_DIR/Backups" "$TARGET"; do
  [[ ! -L "$path" ]] || { echo "심볼릭 링크를 수정하지 않습니다: $path" >&2; exit 1; }
done
[[ -e "$TARGET" ]] || { echo '이 사용자 계정에는 Edge Light Wrap이 설치되어 있지 않습니다.'; exit 0; }
IDENTIFIER="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$TARGET/Contents/Info.plist")"
[[ "$IDENTIFIER" == com.hyun2xyz.edgelightwrap ]] || { echo '다른 플러그인은 제거하지 않습니다.' >&2; exit 1; }
mkdir -p "$SUPPORT_DIR/Backups"
LOCK="$SUPPORT_DIR/.install-lock"
mkdir "$LOCK" 2>/dev/null || { echo '다른 설치 또는 제거가 진행 중입니다.' >&2; exit 1; }
trap '/bin/rmdir "$LOCK" 2>/dev/null || true' EXIT
BACKUP="$SUPPORT_DIR/Backups/Removed-EdgeLightWrap-$(date +%Y%m%d-%H%M%S)-$$.plugin"
/bin/mv "$TARGET" "$BACKUP"
printf 'removed_at=%s\nbackup=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$BACKUP" > "$SUPPORT_DIR/uninstall-receipt.txt"
printf '제거 완료. 플러그인은 복구 가능한 위치에 보관했습니다.\n%s\n' "$BACKUP"
if [[ -t 0 && -z "$SANDBOX_ROOT" ]]; then read -r -p 'Enter를 누르면 창을 닫을 수 있습니다. ' _; fi
