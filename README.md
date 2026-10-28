# AFTEREFFECT_WRAP

**After Effects 네이티브 효과 Edge Light Wrap 2.0.0 설치 패키지.**
인물 알파의 가장자리에 배경의 색과 빛을 더합니다. 설치하면 **Effect > Hyun2 > Edge Light Wrap**으로
적용하고 Effect Controls에서 조절합니다. Mac용과 Windows용이 각각 `mac/`, `windows/` 폴더에 있습니다.

## 학생은 이렇게 시작합니다

공통강의의 Git 세팅을 마친 컴퓨터에서 Codex 또는 Claude Code를 열고 이 문장을 붙여 넣으세요.

> https://github.com/hyun2xyz/AFTEREFFECT_WRAP 를 제 GitHub 계정으로 포크해서 문서(Documents) 폴더에
> 받고, 받은 폴더의 AGENTS.md대로 Edge Light Wrap을 제 컴퓨터에 설치해 주세요.
> After Effects는 제가 저장하고 직접 종료하겠습니다. Windows 관리자 확인 창은 제가 [예]를 누르겠습니다.
> 설치 위치와 확인 결과를 알려 주세요.

에이전트가 내 포크를 만들어 받고, 내 OS에 맞는 폴더를 골라 설치한 뒤 결과를 확인합니다.
강사가 새 버전을 올리면 [포크로 시작하고 업데이트 받기](docs/fork-and-update.md)의 3번으로 pull한 뒤
설치기를 다시 실행합니다. 자세한 흐름과 직접 설치하는 방법은 [START-HERE.md](START-HERE.md)에 있습니다.

| | Mac | Windows |
| --- | --- | --- |
| 폴더 | [`mac/`](mac/) | [`windows/`](windows/) |
| 플러그인 | `EdgeLightWrap.plugin` (Apple Silicon + Intel) | `EdgeLightWrap.aex` (x64) |
| 직접 설치 | `Install Edge Light Wrap.command` | `Install Edge Light Wrap.cmd` 더블 클릭 |
| 설치 위치 | `~/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/` | `C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\` |
| 권한 | 관리자 암호 없음 | 관리자 확인 창(UAC) 1회 |
| 실제 AE 검증 | AE 26.3에서 설치·메뉴·렌더 확인 | 실제 AE에서 설치·메뉴·적용 확인(2026-10-28), **렌더 결과 비교는 미확인** |

## 에이전트용 요약

설치 요청을 받으면 [AGENTS.md](AGENTS.md)를 먼저 읽습니다. 핵심만 옮기면 다음과 같습니다.

1. 학생 계정에 포크해서 받습니다(`gh repo fork hyun2xyz/AFTEREFFECT_WRAP --clone`).
   Git 세팅 전인 컴퓨터만 `curl` ZIP으로 받습니다. 브라우저 다운로드는 쓰지 않습니다.
2. After Effects가 실행 중이면 학생에게 저장 후 종료를 요청합니다. 강제로 종료하지 않습니다.
3. 현재 OS의 설치기만 실행합니다. 먼저 `--dry-run`, 그다음 실제 설치입니다.

```sh
bash "mac/Install Edge Light Wrap.command" --dry-run
bash "mac/Install Edge Light Wrap.command"
```

```powershell
powershell.exe -NoProfile -Command "& '.\windows\install.cmd' --dry-run; exit $LASTEXITCODE"
powershell.exe -NoProfile -Command "& '.\windows\install.cmd'; exit $LASTEXITCODE"
```

4. 설치기의 마지막 메시지와 설치 기록을 확인하고, 학생에게 AE에서 효과가 보이는지 확인해 달라고 합니다.

보안 설정(Gatekeeper, 격리 표시, PowerShell 실행 정책, Defender)은 바꾸지 않습니다.

## 문서

- [처음 시작하기](START-HERE.md): 붙여 넣을 문장, 직접 설치, 제거
- [포크로 시작하고 업데이트 받기](docs/fork-and-update.md): 내 포크, 강사 업데이트 pull
- [사용법](docs/usage.md): 효과 순서, 컨트롤, 빛이 안 보일 때
- [검증 범위](docs/validation.md): Mac/Windows별로 실제로 확인한 것과 남은 것
- [빌드와 패키징](docs/build.md): 개발자용. Adobe SDK는 포함하지 않습니다
- [예제](example/README.md): 플러그인 설치 후 여는 640×360 데모 AEP

## 저장소 구성

```text
mac/        Mac 플러그인, 설치·제거 파일, payload.sha256
windows/    Windows 플러그인, 설치·제거 파일, payload.sha256, 런타임 라이선스
example/    네이티브 효과 데모 AEP와 전후 이미지
docs/       사용법, 검증 기록, 빌드
source/     효과 소스, CMake, Mac/Windows 빌드 스크립트, PiPL 생성기
tests/      패키지·설치 리허설 검사 (Python 표준 라이브러리)
```

```sh
python3 -B -m unittest discover -s tests -v
```

새 코드와 문서는 [MIT](LICENSE)입니다. Adobe 제품·SDK의 권리는 포함하지 않습니다.
[출처·라이선스](THIRD_PARTY_NOTICES.md)를 확인하세요.
