#pragma once
#include "cstrike.h"
#include "cfg.h"
#include "sdk.h"

namespace NoFlash
{
    inline void Run()
    {
        if (!cfg::noflash)
            return;

        uintptr_t localPawn = localplayer::G_Pawn();
        if (!localPawn)
            return;

        memory->write<float>(localPawn + Offsets::m_flFlashDuration, 0.0f);
        memory->write<float>(localPawn + Offsets::m_flFlashMaxAlpha, 0.0f);
    }
}