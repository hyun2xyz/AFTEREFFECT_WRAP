# 처음 시작하기

이 저장소는 **Codex 또는 Claude Code가 링크만 받고 포크·설치까지** 하도록 만든 Edge Light Wrap
패키지입니다. 공통강의의 Git 세팅(Git·GitHub CLI 설치와 `gh` 인증)을 마친 컴퓨터를 기준으로 합니다.
After Effects는 학생 자신의 Adobe 계정·라이선스로 미리 설치해 둡니다.

## 1. 에이전트에게 붙여 넣기

에이전트를 아무 폴더에서나 열고 이 문장을 붙여 넣습니다.

> https://github.com/hyun2xyz/AFTEREFFECT_WRAP 를 제 GitHub 계정으로 포크해서 문서(Documents) 폴더에
> 받고, 받은 폴더의 AGENTS.md대로 Edge Light Wrap을 제 컴퓨터에 설치해 주세요.
> After Effects는 제가 저장하고 직접 종료하겠습니다. Windows 관리자 확인 창은 제가 [예]를 누르겠습니다.
> 설치 위치와 확인 결과를 알려 주세요.

에이전트는 이렇게 진행합니다.

1. 내 GitHub 계정에 포크를 만들고 `문서(Documents)/AFTEREFFECT_WRAP`에 받습니다.
   강사 원본은 `upstream`으로 함께 등록됩니다.
2. AE가 켜져 있고 바꿀 파일이 있으면 저장 후 종료해 달라고 합니다. 학생이 직접 종료합니다.
3. Mac은 `mac/`, Windows는 `windows/`의 설치기를 먼저 점검 모드로, 그다음 실제로 실행합니다.
4. 설치 위치와 확인 결과를 알려 줍니다.

**Windows**에서는 설치 중에 "이 앱이 디바이스를 변경하도록 허용하시겠어요?" 창이 한 번 뜹니다.
Adobe 공용 플러그인 폴더(`C:\Program Files\...`)에 쓰기 위해 필요하므로 **[예]** 를 누릅니다.
**Mac**은 관리자 암호를 묻지 않습니다.

에이전트가 실행 권한 승인을 요청하면 내용을 읽고 승인합니다. 설치기는 플러그인 폴더와
백업 폴더 외에는 바꾸지 않습니다.

## 2. 설치 후 확인

1. After Effects를 다시 실행합니다.
2. 인물 레이어를 선택하고 **Effect > Hyun2 > Edge Light Wrap**을 적용합니다.
   Effects & Presets에서 `Edge Light Wrap`을 검색해도 됩니다.
3. Effect Controls의 **Background Layer**에 배경 레이어를 지정합니다.

효과 순서는 **Object Matte / Roto Brush / 키잉 → Advanced Spill Suppressor → Edge Light Wrap**입니다.
빛이 보이지 않으면 [사용법](docs/usage.md)의 확인 순서를 따르세요. 바로 확인하고 싶다면
[`example/Native Demo.aep`](example/README.md)를 엽니다.

## 3. 에이전트 없이 직접 설치

저장소 페이지의 **Code → Download ZIP**으로 받으면 Mac에서는 격리 표시 때문에 설치기가 멈춥니다.
Mac은 터미널에서 받는 방법을 권장합니다.

**Mac**

```sh
cd ~/Documents
curl -fL -o AFTEREFFECT_WRAP.zip https://github.com/hyun2xyz/AFTEREFFECT_WRAP/archive/refs/heads/main.zip
ditto -x -k AFTEREFFECT_WRAP.zip .
bash "AFTEREFFECT_WRAP-main/mac/Install Edge Light Wrap.command"
```

**Windows**: ZIP을 받아 **압축을 완전히 푼 뒤** `windows` 폴더의
`Install Edge Light Wrap.cmd`를 더블 클릭하고, 관리자 확인 창에서 [예]를 누릅니다.
압축 파일 안에서 바로 실행하지 마세요. 인터넷에서 받은 파일이라 Windows가 실행 여부를 물으면,
이 저장소에서 받은 파일이 맞는지 확인한 뒤 진행합니다.

## 4. 업데이트와 제거

- 강사가 새 버전을 올리면 [포크로 시작하고 업데이트 받기](docs/fork-and-update.md)의 3번으로
  강사 원본을 pull한 뒤 같은 설치기를 다시 실행합니다.
- 설치기를 다시 실행하면 이전 버전을 백업 폴더로 옮긴 뒤 새 버전을 설치합니다.
  같은 버전이면 아무것도 바꾸지 않습니다.
- 제거: Mac `Uninstall Edge Light Wrap.command`, Windows `Uninstall Edge Light Wrap.cmd`.
  플러그인은 삭제하지 않고 복구 가능한 백업 폴더로 옮깁니다.

| | 백업과 설치 기록 |
| --- | --- |
| Mac | `~/Library/Application Support/Edge Light Wrap/` |
| Windows | `C:\ProgramData\Edge Light Wrap\` |

같은 플러그인을 AE 앱 폴더의 `Plug-ins`에 따로 넣어 두었다면 하나만 남기세요. 두 곳에 있으면 AE가
중복 경고를 띄웁니다.
