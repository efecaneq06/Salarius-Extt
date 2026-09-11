#pragma once
#include <vector>
#include <chrono>
#include <unordered_map>
#include <algorithm>
//#include <imgui.h>
#include "math.h"
#include "offsets.h"
#include "entity.h" 

namespace sound_esp {
    struct SoundEffect {
        Vec3 origin;
        double spawnTime;
        uintptr_t controller_addr;
    };

    inline float MaxDistance = 2500.0f;
    inline float EffectSpeed = 150.0f;
    inline float MaxRadius = 80.0f;
    inline float MinMovementSpeed = 20.0f;
    inline double MinSpawnInterval = 0.25f;

    static std::vector<SoundEffect> soundEffects;
    static std::unordered_map<uintptr_t, float> lastSoundTimes;
    static std::unordered_map<uintptr_t, double> lastSpawnTimes;

    inline void RenderSoundCircle(const Vec3& origin, float radius, ImColor color) {
        view_matrix_t viewMatrix = memory->read<view_matrix_t>(client + Offsets::dwViewMatrix);

        const int segmentCount = 40;
        const float step = (3.14159265f * 2.0f) / segmentCount;
        std::vector<ImVec2> screenPoints;

        for (int i = 0; i <= segmentCount; i++) {
            float angle = i * step;
            Vec3 worldPoint = {
                origin.x + cosf(angle) * radius,
                origin.y + sinf(angle) * radius,
                origin.z
            };

            Vec3 screenPos;
            if (W2S(worldPoint, screenPos, viewMatrix)) {
                screenPoints.push_back(ImVec2(screenPos.x, screenPos.y));
            }
        }

        if (screenPoints.size() > 2) {
            ImGui::GetBackgroundDrawList()->AddPolyline(screenPoints.data(), screenPoints.size(), color, false, 1.5f);
        }
    }

    inline void Update() {
        if (!cfg::soundesp) return;

        if (!localplayer::pawn || !memory->IsConnected()) return;

        Vec3 localPos = memory->read<Vec3>(localplayer::pawn + Offsets::m_vOldOrigin);
        double currentTime = ImGui::GetTime();

        for (const auto& player : entities::snapshot()) {
            if (!player.actorBase || !player.controllerBase || (cfg::teamCheck && player.team == localplayer::teamid)) continue;

            Vec3 entPos = memory->read<Vec3>(player.actorBase + Offsets::m_vOldOrigin);
            if (entPos.calcDist(localPos) > MaxDistance) continue;

            float currentSoundTime = memory->read<float>(player.actorBase + Offsets::m_flEmitSoundTime);

            if (lastSoundTimes[player.controllerBase] == currentSoundTime) continue;
            lastSoundTimes[player.controllerBase] = currentSoundTime;

            Vec3 vel = memory->read<Vec3>(player.actorBase + Offsets::m_vecAbsVelocity);
            uint32_t flags = memory->read<uint32_t>(player.actorBase + Offsets::m_fFlags);
            bool isJumping = !(flags & (1 << 0)); 

            if (vel.length() < MinMovementSpeed && !isJumping) continue;

            if (currentTime - lastSpawnTimes[player.controllerBase] < MinSpawnInterval) continue;
            lastSpawnTimes[player.controllerBase] = currentTime;

            bool updated = false;
            for (auto& effect : soundEffects) {
                if (effect.controller_addr == player.controllerBase) {
                    effect.origin = entPos;
                    effect.spawnTime = currentTime;
                    updated = true;
                    break;
                }
            }

            if (!updated) {
                soundEffects.push_back({ entPos, currentTime, player.controllerBase });
            }
        }
    }

    inline void Draw() {
        // efekt uretimi 30Hz'e kisildi: once her frame oyuncu basina 4 RPM vardi
        static auto lastUp = std::chrono::steady_clock::now();
        auto nowT = std::chrono::steady_clock::now();
        if (nowT - lastUp >= std::chrono::milliseconds(33)) {
            Update();
            lastUp = nowT;
        }

        if (soundEffects.empty()) return;

        double currentTime = ImGui::GetTime();
        float duration = MaxRadius / EffectSpeed;

        for (auto it = soundEffects.begin(); it != soundEffects.end();) {
            float elapsed = (float)(currentTime - it->spawnTime);

            if (elapsed > duration) {
                it = soundEffects.erase(it);
                continue;
            }

            float progress = std::clamp(elapsed / duration, 0.0f, 1.0f);
            float radius = (MaxRadius * 0.2f) + (MaxRadius * 0.8f) * progress;

            float r = cfg::boxColor[0];
            float g = cfg::boxColor[1];
            float b = cfg::boxColor[2];
            float dynamicAlpha = cfg::boxColor[3] * (1.0f - progress);

            ImColor color = ImColor(r, g, b, dynamicAlpha);

            RenderSoundCircle(it->origin, radius, color);
            ++it;
        }
    }
}