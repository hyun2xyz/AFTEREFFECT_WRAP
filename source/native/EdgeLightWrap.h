#pragma once
#define PF_DEEP_COLOR_AWARE 1
#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "SPBasic.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "WrapCore.h"

enum Param { Input = 0, Background, EdgeWidth, EdgeFeather, WrapSpread, WrapSoftness,
             BackgroundBlur, Strength, PreserveAlpha, OutputView, NumParams };
constexpr const char* ELW_NAME = "Edge Light Wrap";
constexpr const char* ELW_MATCH_NAME = "Hyun2 Edge Light Wrap";
constexpr const char* ELW_CATEGORY = "Hyun2";
constexpr A_u_long ELW_FLAGS = PF_OutFlag_DEEP_COLOR_AWARE;
constexpr A_u_long ELW_FLAGS2 = PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_FLOAT_COLOR_AWARE |
                              PF_OutFlag2_SUPPORTS_THREADED_RENDERING;
extern "C" DllExport PF_Err EffectMain(PF_Cmd, PF_InData*, PF_OutData*, PF_ParamDef*[], PF_LayerDef*, void*);
