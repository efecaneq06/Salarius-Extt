#pragma once
#include <algorithm>
#include "sdk.h"
#include "cfg.h"

namespace rcs {
    inline Vec2 oldPunch = { 0.0f, 0.0f };

    inline void Run() {
        if (!cfg::rcs) {
            oldPunch = { 0.0f, 0.0f };
            return;
        }

        uintptr_t pPawn = localplayer::G_Pawn();
        if (!pPawn) return;

        int shotsFired = memory->read<int>(pPawn + Offsets::m_iShotsFired);

        if (shotsFired < 1) {
            oldPunch = { 0.0f, 0.0f };
            return;
        }

        uintptr_t aimPunchServices = memory->read<uintptr_t>(pPawn + Offsets::m_pAimPunchServices);
        if (!aimPunchServices) return;

        uintptr_t cacheAddr = aimPunchServices + 0x88;
        uint32_t count = memory->read<uint32_t>(cacheAddr);
        uintptr_t dataPtr = memory->read<uintptr_t>(cacheAddr + 0x8);

        if (count <= 0 || !dataPtr) return;

        Vec2 punch = memory->read<Vec2>(dataPtr + (static_cast<uint64_t>(count) - 1) * 12);

        Vec2 currentPunch = {
            punch.x * cfg::rcsStrength,
            punch.y * cfg::rcsStrength
        };

        Vec2 delta = {
            currentPunch.x - oldPunch.x,
            currentPunch.y - oldPunch.y
        };

        if (std::abs(delta.x) > 0.01f || std::abs(delta.y) > 0.01f) {
            Vec2 currentAngles = memory->read<Vec2>(client + Offsets::dwViewAngles);

            Vec2 newAngles;
            newAngles.x = currentAngles.x - delta.x;
            newAngles.y = currentAngles.y - delta.y;

            newAngles.x = std::clamp(newAngles.x, -89.0f, 89.0f);
            while (newAngles.y > 180.0f) newAngles.y -= 360.0f;
            while (newAngles.y < -180.0f) newAngles.y += 360.0f;

            memory->write<Vec2>(client + Offsets::dwViewAngles, newAngles);
        }

        oldPunch = currentPunch;
    }
}