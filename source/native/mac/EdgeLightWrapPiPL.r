#include "AEConfig.h"
#include "AE_EffectVers.h"
#include <AE_General.r>

resource 'PiPL' (16000) {{
    Kind { AEEffect },
    Name { "Edge Light Wrap" },
    Category { "Hyun2" },
    CodeMacIntel64 { "EffectMain" },
    CodeMacARM64 { "EffectMain" },
    AE_PiPL_Version { 2, 0 },
    AE_Effect_Spec_Version { PF_PLUG_IN_VERSION, PF_PLUG_IN_SUBVERS },
    AE_Effect_Version { 1050113 },
    AE_Effect_Info_Flags { 0 },
    AE_Effect_Global_OutFlags { 0x02000000 },
    AE_Effect_Global_OutFlags_2 { 0x08001400 },
    AE_Effect_Match_Name { "Hyun2 Edge Light Wrap" },
    AE_Reserved_Info { 0 },
    AE_Effect_Support_URL { "https://github.com/hyun2xyz/edge-light-wrap" }
}};
