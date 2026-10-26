# 출처와 별도 라이선스

이 저장소의 효과 소스, 설치기, 문서는 [MIT](LICENSE)입니다. After Effects와 Adobe SDK,
학생의 영상·프로젝트에 대한 권리는 포함하지 않습니다.

## 원본

효과 소스와 Mac 번들은 [hyun2xyz/edge-light-wrap](https://github.com/hyun2xyz/edge-light-wrap)
commit `086ffce77a5ab81c6f697443bf2c41136aee50f1`(Edge Light Wrap 2.0.0)에서 가져왔습니다.
이 저장소에서 Windows 빌드(CMake, PiPL 생성기, 테스트 호스트 로더)를 추가했습니다.
1.1 ScriptUI `.jsx`와 이전 데모 파일은 포함하지 않습니다.

## 외부 구성 요소

| 구성 요소 | 출처 | 포함 범위 |
| --- | --- | --- |
| After Effects | https://www.adobe.com/products/aftereffects.html | 상용, 학생의 별도 라이선스 필요. 미포함 |
| Adobe After Effects SDK 25.6 | https://developer.adobe.com/after-effects/ | 빌드 때만 사용. 헤더·예제·PiPLtool 미포함 |
| MinGW-w64 runtime | https://www.mingw-w64.org/ | `EdgeLightWrap.aex`에 정적 링크. 고지: [windows/licenses/MinGW-w64-runtime.txt](windows/licenses/MinGW-w64-runtime.txt) |
| winpthreads | https://www.mingw-w64.org/ | `EdgeLightWrap.aex`에 정적 링크. 고지: [windows/licenses/winpthreads.txt](windows/licenses/winpthreads.txt) |
| GCC runtime (libstdc++, libgcc) | https://gcc.gnu.org/ | 정적 링크. GPLv3 + GCC Runtime Library Exception |

PiPL 바이너리 배치는 SDK의 `AE_General.r` 템플릿과 Mac Rez 결과로 확인했고, Windows 쪽 정렬 규칙은
[AdrianEddy/after-effects](https://github.com/AdrianEddy/after-effects)의 `pipl` 구현을 참고했습니다.
코드는 복사하지 않았습니다.
