# Windows 설치 파일

- `EdgeLightWrap.aex`: 효과 본체(Windows 10/11 x64). 직접 옮기지 마세요.
- `Install Edge Light Wrap.cmd`: 더블 클릭으로 설치합니다. 관리자 확인 창에서 [예]를 누릅니다.
- `Uninstall Edge Light Wrap.cmd`: 설치한 플러그인을 백업 폴더로 옮깁니다.
- `install.cmd`: 위 두 파일이 부르는 설치기 본체. 에이전트는 이 파일을 직접 실행합니다.
- `payload.sha256`: 설치기가 플러그인 파일이 바뀌지 않았는지 확인하는 값입니다.
- `licenses/`: `.aex`에 포함된 MinGW-w64 런타임의 라이선스 고지입니다.

AE를 저장 후 종료하고 실행합니다. 설치 위치는 AE가 기록한 공용 플러그인 폴더이며 보통
`C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\EdgeLightWrap.aex`입니다.
백업과 설치 기록은 `C:\ProgramData\Edge Light Wrap\`에 남습니다.

설치기 메시지는 영어로 나옵니다.

| 메시지 | 뜻 |
| --- | --- |
| `Install complete.` | 설치 완료. AE를 열고 Effect > Hyun2 > Edge Light Wrap을 선택합니다. |
| `The same version is already installed.` | 이미 같은 버전이 있어 바꾸지 않았습니다. |
| `Save your work and quit After Effects yourself` | AE를 저장 후 종료하고 다시 실행합니다. |
| `The administrator prompt was cancelled` | 관리자 확인 창에서 [예]를 누르지 않아 아무것도 바꾸지 않았습니다. |
| `Package file is damaged or changed` | 저장소를 다시 받습니다. |
| `Another plug-in uses this file name` | 같은 이름의 다른 파일이 있어 덮어쓰지 않았습니다. |

이 Windows 빌드는 2026-10-28 실제 Windows After Effects에서 메뉴 등록과 적용까지 확인했고,
렌더 결과 비교는 아직입니다. 설치한 뒤 효과의 결과와 AE 버전을 강사에게 알려 주세요. [검증 범위](../docs/validation.md)
