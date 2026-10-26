# Mac 설치 파일

- `EdgeLightWrap.plugin`: 효과 본체(Apple Silicon + Intel, macOS 13.4 이상). 직접 옮기지 마세요.
- `Install Edge Light Wrap.command`: 현재 사용자 계정에 설치합니다. 관리자 암호가 필요 없습니다.
- `Uninstall Edge Light Wrap.command`: 설치한 플러그인을 백업 폴더로 옮깁니다.
- `payload.sha256`: 설치기가 플러그인 파일이 바뀌지 않았는지 확인하는 목록입니다.

AE를 저장 후 종료하고, 터미널에서 실행합니다.

```sh
bash "Install Edge Light Wrap.command"
```

설치 위치: `~/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/EdgeLightWrap.plugin`

브라우저로 받은 ZIP은 격리 표시 때문에 설치기가 멈춥니다. 보안 설정을 끄지 말고
[START-HERE](../START-HERE.md)의 에이전트 설치나 터미널 다운로드를 사용하세요.
