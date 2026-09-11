#pragma once
// External glow chams: her dusman pawn'in CGlowProperty'sini boyar (duvar arkasi gorunur).
// SADECE main.cpp TU'sunda kullanilir (mainThread'den cagrilir).
#include "entity.h"
#include "cfg.h"
#include <unordered_map>
#include <chrono>

namespace chams
{
    struct GlowColor { uint8_t r, g, b, a; };

    inline GlowColor ToGlow(float* f) {
        GlowColor c{};
        c.r = (uint8_t)(f[0] * 255.0f);
        c.g = (uint8_t)(f[1] * 255.0f);
        c.b = (uint8_t)(f[2] * 255.0f);
        c.a = (uint8_t)(f[3] * 255.0f);
        return c;
    }

    // acikken her 50ms'de bir uygula, kapaninca bir kez temizle.
    // NOT: glow'u OYUN render'lar (overlay degil). Yayin/ekran goruntusu oyunu
    // yakalarsa glow gorunur. streamproof + proofPauseMem aciksa otomatik durur.
    inline void Run() {
        static bool wasOn = false;
        static auto lastRun = std::chrono::steady_clock::now() - std::chrono::seconds(1);
        static std::unordered_map<uintptr_t, uint32_t> lastColor;

        bool allowed = cfg::chams && !(cfg::streamproof && cfg::proofPauseMem);
        if (!allowed) {
            if (wasOn) {
                // bir kez temizle: tum gorunen pawn'lerin glow'unu kapat
                for (const auto& actor : entities::snapshot()) {
                    if (!actor.actorBase) continue;
                    memory->write<bool>(actor.actorBase + Offsets::m_Glow + Offsets::m_bGlowing, false);
                }
                lastColor.clear();
                wasOn = false;
            }
            return;
        }
        wasOn = true;

        auto now = std::chrono::steady_clock::now();
        if (now - lastRun < std::chrono::milliseconds(50)) return;
        lastRun = now;

        GlowColor enemy = ToGlow(cfg::chamsColor);
        GlowColor team = ToGlow(cfg::chamsTeamColor);
        uint32_t enemyPack = *(uint32_t*)&enemy;
        uint32_t teamPack = *(uint32_t*)&team;

        for (const auto& actor : entities::snapshot()) {
            if (!actor.actorBase || actor.health <= 0) continue;
            if (actor.actorBase == localplayer::pawn) continue;
            bool isTeam = (actor.team == localplayer::teamid);
            if (isTeam && cfg::teamCheck) continue;

            uint32_t want = isTeam ? teamPack : enemyPack;
            auto it = lastColor.find(actor.actorBase);
            if (it != lastColor.end() && it->second == want) {
                // renk ayni, ama glow kapali kalmis olabilir -> hizli kontrolsuz tekrar acma
                // (her turde 1 byte okumak yerine periyodik yaziyoruz, asagida dusuyor)
            }

            uintptr_t glowBase = actor.actorBase + Offsets::m_Glow;
            if (cfg::chamsStyle == 1) {
                // model tint: duvar arkasi degil ama ucuz + dusuk profil
                memory->write<GlowColor>(actor.actorBase + Offsets::m_clrRender, isTeam ? team : enemy);
            } else {
                // glow: duvar arkasi solid
                memory->write<GlowColor>(glowBase + Offsets::m_glowColorOverride, isTeam ? team : enemy);
                memory->write<int>(glowBase + Offsets::m_iGlowType, 3);
                memory->write<bool>(glowBase + Offsets::m_bGlowing, true);
            }
            lastColor[actor.actorBase] = want;
        }
        if (lastColor.size() > 128) lastColor.clear();
    }
}
