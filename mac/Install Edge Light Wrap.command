#!/bin/bash
# Edge Light Wrap 2.0.0 macOS installer. Installs only for the current user;
# no administrator password, no AE quit, no change to security settings.
#   --dry-run            check the package and print the target only
#   --sandbox-root DIR   rehearse in DIR instead of the real Adobe folder (tests)
set -euo pipefail
PACKAGE_DIR="$(cd "$(dirname "$0")" && pwd)"
VERSION='2.0.0'
BUNDLE_ID='com.hyun2xyz.edgelightwrap'
DRY_RUN=0
SANDBOX_ROOT=''
show_failure() {
  local status="$1"
  if [[ "$status" -ne 0 ]]; then
    printf '\n설치가 완료되지 않았습니다. 위의 오류를 해결한 뒤 다시 실행하세요.\n' >&2
    if [[ -t 0 && -z "$SANDBOX_ROOT" ]]; then
      read -r -p 'Enter를 누르면 창을 닫을 수 있습니다. ' _ || true
    fi
  fi
}
early_exit() {
  local status=$?
  trap - EXIT
  show_failure "$status"
  exit "$status"
}
trap early_exit EXIT
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run) DRY_RUN=1; shift ;;
    --sandbox-root) [[ $# -ge 2 ]] || exit 2; SANDBOX_ROOT="$2"; shift 2 ;;
    *) echo "알 수 없는 옵션: $1" >&2; exit 2 ;;
  esac
done
[[ "$(uname -s)" == Darwin ]] || { echo '이 설치 파일은 macOS 전용입니다. Windows는 windows 폴더를 사용하세요.' >&2; exit 1; }
if [[ -n "$SANDBOX_ROOT" ]]; then
  [[ "$SANDBOX_ROOT" == /* && "$SANDBOX_ROOT" != / && -d "$SANDBOX_ROOT" && ! -L "$SANDBOX_ROOT" ]] || {
    echo '리허설 폴더는 이미 존재하는 절대 경로여야 합니다.' >&2; exit 1;
  }
  SANDBOX_ROOT="$(cd "$SANDBOX_ROOT" && pwd -P)"
  [[ "$SANDBOX_ROOT" != "$HOME" ]] || exit 1
  PLUGIN_DIR="$SANDBOX_ROOT/MediaCore"
  SUPPORT_DIR="$SANDBOX_ROOT/Support"
else
  PLUGIN_DIR="$HOME/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore"
  SUPPORT_DIR="$HOME/Library/Application Support/Edge Light Wrap"
fi
SOURCE="$PACKAGE_DIR/EdgeLightWrap.plugin"
TARGET="$PLUGIN_DIR/EdgeLightWrap.plugin"
[[ -d "$SOURCE" && ! -L "$SOURCE" && -f "$PACKAGE_DIR/payload.sha256" ]] || {
  echo 'mac 폴더 안에 EdgeLightWrap.plugin과 payload.sha256이 함께 있어야 합니다. 저장소 전체를 받으세요.' >&2; exit 1;
}
(cd "$PACKAGE_DIR" && /usr/bin/shasum -a 256 -c payload.sha256)
/usr/bin/codesign --verify --deep --strict "$SOURCE"
# Browser downloads carry a quarantine flag that makes macOS block the plug-in
# inside AE. Do not strip it here; download the repository with git or curl.
if /usr/bin/xattr -p com.apple.quarantine "$SOURCE/Contents/MacOS/EdgeLightWrap" >/dev/null 2>&1; then
  cat >&2 <<'MSG'
브라우저로 받은 파일에는 macOS 격리 표시가 있어 AE가 이 플러그인을 막을 수 있습니다.
보안 설정을 끄지 말고, 에이전트에게 저장소를 git clone(또는 curl)로 다시 받아 설치해 달라고 하세요.
MSG
  exit 1
fi
printf '\nEdge Light Wrap %s (macOS Universal)\n설치 위치: %s\n' "$VERSION" "$TARGET"
if [[ $DRY_RUN -eq 1 ]]; then
  echo '사전 확인 완료. 파일을 설치하거나 AE를 종료하지 않았습니다.'
  exit 0
fi
for path in "$PLUGIN_DIR" "$SUPPORT_DIR" "$SUPPORT_DIR/Backups" "$SUPPORT_DIR/install-receipt.txt" "$TARGET"; do
  [[ ! -L "$path" ]] || { echo "심볼릭 링크에는 설치하지 않습니다: $path" >&2; exit 1; }
done
[[ ! -e "$SUPPORT_DIR/install-receipt.txt" || -f "$SUPPORT_DIR/install-receipt.txt" ]] || {
  echo '설치 기록 경로에 다른 폴더가 있습니다.' >&2; exit 1;
}
if [[ -e "$TARGET" ]]; then
  [[ -d "$TARGET" && -f "$TARGET/Contents/Info.plist" ]] || { echo '같은 이름의 다른 파일이 있습니다.' >&2; exit 1; }
  IDENTIFIER="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$TARGET/Contents/Info.plist")"
  [[ "$IDENTIFIER" == "$BUNDLE_ID" ]] || { echo '동일 이름의 다른 플러그인을 덮어쓰지 않습니다.' >&2; exit 1; }
  # payload.sha256 lists EdgeLightWrap.plugin/..., so checking it from PLUGIN_DIR checks the installed copy.
  if (cd "$PLUGIN_DIR" && /usr/bin/shasum -a 256 -c "$PACKAGE_DIR/payload.sha256" >/dev/null 2>&1) \
     && /usr/bin/codesign --verify --deep --strict "$TARGET" 2>/dev/null; then
    printf '\n같은 버전이 이미 설치되어 있습니다. 변경하지 않았습니다.\n'
    exit 0
  fi
fi
if [[ -z "$SANDBOX_ROOT" ]] && { /usr/bin/pgrep -x 'After Effects' >/dev/null || /usr/bin/pgrep -x 'After Effects Render Engine' >/dev/null; }; then
  echo 'After Effects에서 작업을 저장하고 직접 종료한 뒤 설치 파일을 다시 실행하세요.' >&2
  exit 1
fi
mkdir -p "$PLUGIN_DIR" "$SUPPORT_DIR/Backups"
LOCK="$SUPPORT_DIR/.install-lock"
mkdir "$LOCK" 2>/dev/null || { echo '다른 설치 또는 제거가 진행 중입니다.' >&2; exit 1; }
STAGING=''
BACKUP=''
OLD_MOVED=0
NEW_MOVED=0
COMMITTED=0
cleanup() {
  local status=$?
  trap - EXIT
  if [[ $COMMITTED -eq 0 ]]; then
    if [[ $NEW_MOVED -eq 1 && -d "$TARGET" ]]; then /bin/rm -rf "$TARGET"; fi
    if [[ $OLD_MOVED -eq 1 && -d "$BACKUP" ]]; then /bin/mv "$BACKUP" "$TARGET"; fi
  fi
  [[ -z "$STAGING" || ! -d "$STAGING" ]] || /bin/rm -rf "$STAGING"
  /bin/rmdir "$LOCK" 2>/dev/null || true
  show_failure "$status"
  exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
STAGING="$(/usr/bin/mktemp -d "$PLUGIN_DIR/.edge-light-wrap-stage.XXXXXX")"
/usr/bin/ditto "$SOURCE" "$STAGING/EdgeLightWrap.plugin"
/usr/bin/codesign --verify --deep --strict "$STAGING/EdgeLightWrap.plugin"
if [[ -e "$TARGET" ]]; then
  BACKUP="$SUPPORT_DIR/Backups/EdgeLightWrap-$(date +%Y%m%d-%H%M%S)-$$.plugin"
  /bin/mv "$TARGET" "$BACKUP"
  OLD_MOVED=1
fi
/bin/mv "$STAGING/EdgeLightWrap.plugin" "$TARGET"
NEW_MOVED=1
/usr/bin/codesign --verify --deep --strict "$TARGET"
RECEIPT="$STAGING/install-receipt.txt"
{
  printf 'version=%s\ninstalled_at=%s\nplugin=%s\nbackup=%s\n' "$VERSION" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$TARGET" "$BACKUP"
  /usr/bin/shasum -a 256 "$TARGET/Contents/MacOS/EdgeLightWrap"
} > "$RECEIPT"
/bin/mv "$RECEIPT" "$SUPPORT_DIR/install-receipt.txt"
COMMITTED=1
printf '\n설치 완료. AE를 열고 Effect > Hyun2 > Edge Light Wrap을 선택하세요.\n'
[[ -z "$BACKUP" ]] || printf '이전 버전 백업: %s\n' "$BACKUP"
if [[ -t 0 && -z "$SANDBOX_ROOT" ]]; then read -r -p 'Enter를 누르면 창을 닫을 수 있습니다. ' _; fi
