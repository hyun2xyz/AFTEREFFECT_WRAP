# 빌드와 패키징 (개발자용)

학생 설치에는 필요하지 않습니다. 저장소의 `mac/`, `windows/` 페이로드는 이미 빌드되어 있습니다.
Adobe After Effects SDK는 Adobe에서 각자 받아 로컬에 둡니다. SDK 파일은 저장소에 넣지 않습니다.

```sh
export AE_SDK_ROOT='/path/to/ae25.6_61.64bit.AfterEffectsSDK'
```

## macOS

Xcode Command Line Tools, CMake가 필요합니다. 빌드 후 CTest, `codesign`, `lipo`, PiPL 대조, Rosetta
테스트 호스트까지 실행합니다.

```sh
bash source/scripts/build-macos.sh
```

## Windows (macOS에서 크로스 빌드)

```sh
brew install mingw-w64
bash source/scripts/build-windows.sh
```

`source/scripts/pipl.py`가 SDK의 효과 스펙 버전을 읽어 PiPL `.rc`를 만들고, windres가 `.aex`에 넣습니다.
MinGW 런타임은 정적으로 링크하므로 `.aex` 옆에 DLL이 필요 없습니다. 같은 소스는 같은 파일을 만듭니다.
런타임 라이선스 고지는 `windows/licenses/`에 있습니다.

PiPL 생성기를 Mac Rez 결과와 대조하려면:

```sh
python3 -B source/scripts/pipl.py --sdk "$AE_SDK_ROOT" \
  --compare-rsrc source/build/macos/EdgeLightWrap.plugin/Contents/Resources/EdgeLightWrap.rsrc
```

## 페이로드 교체

실제 AE에서 확인한 빌드만 올립니다. `stage.py`는 지정한 파일을 복사하고 `payload.sha256`을 다시 씁니다.
설치하거나 게시하지 않습니다.

```sh
python3 -B source/scripts/stage.py --mac source/build/macos/EdgeLightWrap.plugin
python3 -B source/scripts/stage.py --windows source/build/windows/EdgeLightWrap.aex \
  --host-test source/build/windows/ae-host-tests.exe
python3 -B -m unittest discover -s tests -v
```

Mac 번들을 바꾸면 `docs/validation/`에 새 실기 기록을 추가하고 테스트가 가리키는 기록을 갱신합니다.
버전을 올리면 `native/EdgeLightWrap.cpp`, `native/mac/EdgeLightWrapPiPL.r`, `scripts/pipl.py`의
버전 값과 두 설치기의 `VERSION`을 함께 바꿉니다.
