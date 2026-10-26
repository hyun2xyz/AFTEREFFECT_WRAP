# 사용법

Edge Light Wrap은 이미 알파가 분리된 인물의 가장자리에 배경의 색과 빛을 더하는 **단일 네이티브 효과**입니다.
별도 패널이나 보조 레이어를 만들지 않습니다. 매트를 직접 추출하지는 않습니다.

## 1. 효과 순서

Effect Controls는 위에서 아래로 처리됩니다. 인물 레이어에서 반드시 이 순서로 놓습니다.

**Object Matte / Roto Brush / 키잉 → Advanced Spill Suppressor → Edge Light Wrap**

Edge Light Wrap을 매트보다 위에 놓으면 인물 알파가 생기기 전에 계산되어 윤곽에 빛이 생기지 않습니다.
아래쪽의 스필 보정도 덧입힌 색을 지울 수 있습니다. 트랙 매트로 분리했다면 매트를 포함한 인물
프리컴프 안으로 먼저 묶습니다.

## 2. 레이어 준비

1. 인물 레이어를 선택하고 **Effect > Hyun2 > Edge Light Wrap**을 적용합니다.
2. **Background Layer**에 배경 레이어를 지정합니다. 지정하지 않으면 원본이 그대로 통과합니다.
3. 인물과 배경을 각각 **컴포지션과 같은 크기의 프리컴프**로 만들고 정렬하면 배경색이 정확히 대응합니다.

배경을 55% 등으로 축소해 두었다면 배경을 **모든 특성을 새 컴포지션으로 이동**으로 프리컴프하고,
축소·위치를 프리컴프 안에 넣습니다. 메인 컴포지션에서는 100%·중앙에 두고 그 프리컴프를
Background Layer로 지정합니다. 레이어 크기가 다르면 배경 소스를 인물 소스 크기로 맞춰 읽습니다.
메인 컴포지션의 배경 변형이나 3D 카메라를 자동으로 따라가지는 않습니다.

## 3. 컨트롤

| Effect Controls | 기본값 | 역할 |
| --- | --- | --- |
| Background Layer | None | 빛의 색을 가져올 배경 |
| Edge Width | 3 px | 인물 중심을 보호하는 안쪽 폭 |
| Edge Feather | 1.5 px | 안쪽 매트 부드러움 |
| Wrap Spread | 4 px | 알파 바깥으로 빛이 퍼지는 폭 |
| Wrap Softness | 6 px | 바깥 매트 부드러움 |
| Background Blur | 25 px | 배경색 블러 |
| Strength | 40% | 빛의 강도. 0%는 원본 통과 |
| Preserve Subject Alpha | Off | 켜면 원본 알파 유지. 끄면 바깥 빛 허용 |
| View | Composite | Composite / Wrap Only / Edge Matte 미리보기 |

8/16/32bpc와 키프레임을 지원하는 CPU 렌더러입니다. GPU 가속은 없습니다.

## 4. 빛이 보이지 않을 때

1. **View → Edge Matte**로 바꿔 인물 윤곽이 보이는지 봅니다. 전체 화면이나 사각형 테두리만 보이면
   알파가 없거나 효과 순서가 틀린 것입니다.
2. Background Layer가 지정되어 있는지 확인합니다.
3. 확인하는 동안만 **Strength를 100%** 로 올려 변화를 봅니다.
4. 다시 View를 **Composite**로 돌립니다.

효과가 메뉴에 없으면 AE를 완전히 종료한 뒤 다시 실행합니다. 그래도 없으면 에이전트에게
`AGENTS.md`의 확인 단계를 다시 실행해 달라고 하고, 결과를 강사에게 알려 주세요.
