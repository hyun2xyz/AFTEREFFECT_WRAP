# 검증 범위

2026-10-28 KST 기준. 실기 AE에서 확인한 것, 테스트 호스트에서 확인한 것, 확인하지 못한 것을 나눕니다.
Mac 성공을 Windows AE 성공으로 해석하지 않습니다.

## Mac — `mac/EdgeLightWrap.plugin`

- 2026-10-20 **Mac After Effects 26.3**(macOS 26.4, Apple Silicon)에서 실제 설치, Effect 메뉴 등록,
  Effect Controls 표시, 합성 예제의 8bpc 렌더를 확인한 바로 그 번들입니다. 파일 5개의 SHA-256이
  [검증 기록](validation/macos-ae26.3-2026-10-20.json)과 같은지 테스트가 매번 확인합니다.
- 예제 렌더에서 원 중심 RGB 38/38/38은 유지되고, 가장자리는 38/38/38에서 38/179/255로 바뀌었습니다.
- 2026-10-26 같은 소스를 이 저장소에서 다시 빌드해 독립 테스트 호스트로 8/16/32bpc, ROI, 행 패딩,
  취소, 배경 없음, legacy render를 확인했고, 출하 번들도 arm64와 x86_64(Rosetta)에서 같은 검사를
  통과했습니다.
- Universal(arm64 + x86_64), macOS 13.4 이상, Adobe AE SDK 25.6. ad-hoc 코드 서명이며
  Developer ID 서명·Apple 공증은 없습니다.
- 미확인: AE 앱 안의 16/32bpc 렌더, 실제 Intel Mac.
- 설치기: 임시 폴더에서 점검 모드, 새 설치, 같은 버전 재실행, 변경본 백업 후 교체, 잠금,
  변조 패키지 거부, 격리 표시 거부, 다른 플러그인 보존, 제거를 리허설합니다.

## Windows — `windows/EdgeLightWrap.aex`

- 2026-10-26 같은 효과 소스를 MinGW-w64 GCC 16.2(정적 런타임)와 Adobe AE SDK 25.6으로
  macOS에서 크로스 빌드했습니다. 같은 소스를 다시 빌드하면 같은 SHA-256이 나옵니다.
- PE32+ x64 DLL이며 `EffectMain`, `PluginDataEntryFunction2`를 내보냅니다. 필요한 DLL은
  `KERNEL32.dll`과 Windows 10 이상의 기본 UCRT(`api-ms-win-crt-*`)뿐입니다.
- PiPL(효과 등록 정보)은 `source/scripts/pipl.py`가 만듭니다. 이 생성기는 Mac에서 Rez가 만든
  실기 검증본의 PiPL과 **바이트 단위로 같고**, Windows PiPL의 속성(이름, 분류, match name,
  버전, 플래그)은 Mac 번들의 것과 같습니다. Windows 진입점은 `8664`(x64) `EffectMain`입니다.
- SDK 헤더의 구조체는 `#pragma pack(8)`만 쓰고 `long double`·비트필드가 없어 MSVC로 빌드된
  AE와 같은 메모리 배치를 씁니다.
- CI의 실제 Windows에서: Windows 로더로 `.aex`를 열어 PiPL 리소스와 진입점을 찾고,
  [AEHostTests](../source/tests/AEHostTests.cpp)를 MinGW로 빌드한 `tests/windows/ae-host-tests.exe`로
  8/16/32bpc 렌더·ROI·취소·legacy render를 실행하고, 설치기 리허설을 합니다. 결과는 커밋별
  [Actions](https://github.com/hyun2xyz/AFTEREFFECT_WRAP/actions)에서 확인합니다.
  첫 패키지 커밋의 [Windows 실행 기록](https://github.com/hyun2xyz/AFTEREFFECT_WRAP/actions/runs/40341199275)에서
  위 검사가 모두 통과했습니다(windows-latest, Python 3.13).
- 2026-10-28 강사의 Windows PC에서 **실제 After Effects**로 확인했습니다. 설치기가 설치를 마쳤고,
  설치된 `.aex`의 SHA-256이 `windows/payload.sha256`과 같았으며, 사용자가 **Effect > Hyun2 >
  Edge Light Wrap** 메뉴 등록과 레이어 적용을 확인했습니다. 그 `.aex`가 지금 출하본인지 테스트가
  [검증 기록](validation/windows-ae-2026-10-28.json)으로 매번 확인합니다. AE·Windows 버전은
  기록하지 못했습니다.
- **미확인: 실제 Windows AE의 렌더 결과**(Mac 예제와 같은 픽셀인지), `example/Native Demo.aep`,
  AE 안의 16/32bpc. Windows에서 예제를 열어 본 수강생·강사는 결과와 AE 버전을 알려 주세요.
- 코드 서명 없음. 실행 파일이 아니라 AE가 불러오는 DLL이므로 SmartScreen 실행 경고 대상은 아닙니다.

## 설치 위치 근거

- Mac: 사용자 계정의 `~/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore`.
  위 Mac AE 26.3 실기 검증에서 이 위치를 사용했습니다.
- Windows: AE가 레지스트리 `HKLM\SOFTWARE\Adobe\After Effects\<버전>\CommonPluginInstallPath`에
  기록한 공용 MediaCore 폴더, 없으면 `C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore`.
  Adobe 플러그인 SDK 안내의 설치 위치이며 사용자별 위치는 없어 관리자 확인이 필요합니다.
