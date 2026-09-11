#pragma once
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "cstrike.h"
#include "cfg.h"
#include "sdk.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <chrono>

namespace aimbot {

    struct AimbotTarget {
        bool hasTarget = false;
        uintptr_t targetEntity = 0;
        Vec3 lastTargetPos = { 0, 0, 0 };
        double acquireTime = 0.0; // hedefin kilitlendigi an (react delay icin)
    };

    inline AimbotTarget currentTarget;

    // cfg::aimBone -> kemik indexi (entity worker 30 kemik okuyor)
    inline int AimBoneIndex() {
        switch (cfg::aimBone) {
        case 1: return 6;  // Neck
        case 2: return 23; // Chest
        case 3: return 1;  // Pelvis
        default: return 7; // Head
        }
    }

    inline double NowSec() {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    inline float NormalizeYaw(float yaw) {
        while (yaw > 180.f) yaw -= 360.f;
        while (yaw < -180.f) yaw += 360.f;
        return yaw;
    }

    inline float NormalizePitch(float pitch) {
        if (pitch > 89.f) pitch = 89.f;
        if (pitch < -89.f) pitch = -89.f;
        return pitch;
    }

    inline Vec2 CalcAngle(Vec3 src, Vec3 dst) {
        Vec3 delta = dst - src;
        float dist2d = sqrtf(delta.x * delta.x + delta.y * delta.y);
        return { NormalizePitch(-atan2f(delta.z, dist2d) * (180.f / (float)M_PI)),
                 NormalizeYaw(atan2f(delta.y, delta.x) * (180.f / (float)M_PI)) };
    }

    inline bool IsVisible(const entities::PLAYER& actor) {
        if (!cfg::wallCheck) return true;
        uintptr_t spottedState = actor.actorBase + Offsets::m_entitySpottedState;
        if (!spottedState) return false;

        bool isSpotted = memory->read<bool>(spottedState + Offsets::m_bSpotted);
        if (!isSpotted) return false;
        return true;
    }

    void Shoot() {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        //std::this_thread::sleep_for(std::chrono::milliseconds(2));
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    inline void FindClosestTarget(float fov) {
        float closestDistance = FLT_MAX;
        uintptr_t bestEntity = 0;
        Vec3 bestPos = { 0, 0, 0 };
        view_matrix_t viewMatrix = memory->read<view_matrix_t>(client + Offsets::dwViewMatrix);

        const int TARGET_INDEX = AimBoneIndex();
        const Vec3 myPos = localplayer::cameraPos;

        auto currentPlayers = entities::snapshot();

        for (const auto& actor : currentPlayers) {
            if (!actor.actorBase || actor.health <= 0 || actor.bones.empty()) continue;
            if (cfg::teamCheck && actor.team == localplayer::teamid) continue;

            if (actor.bones.size() <= (size_t)TARGET_INDEX) continue;

            if (!IsVisible(actor)) continue;

            // mesafeye gore FOV: yakin hedefte dar, uzakta genis tolerans
            float effFov = fov;
            if (cfg::fovDistScale) {
                Vec3 d = actor.PlayerPos - myPos;
                float dist3d = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
                if (dist3d > 1.0f) {
                    float k = 1200.0f / dist3d;
                    if (k < 0.6f) k = 0.6f;
                    if (k > 1.5f) k = 1.5f;
                    effFov = fov * k;
                }
            }

            ImVec2 screenPos = SDK::getBonePosition(actor, viewMatrix, TARGET_INDEX, nullptr);
            if (!SDK::IsOnScreen(screenPos)) continue;

            float screenDist = sqrtf(powf(screenPos.x - SDK::screenCenter.x, 2) + powf(screenPos.y - SDK::screenCenter.y, 2));

            if (screenDist <= effFov && screenDist < closestDistance) {
                closestDistance = screenDist;
                bestEntity = actor.actorBase;
                bestPos = actor.bones[TARGET_INDEX];
            }
        }
        if (bestEntity != currentTarget.targetEntity)
            currentTarget.acquireTime = NowSec();
        currentTarget.hasTarget = (bestEntity != 0);
        currentTarget.targetEntity = bestEntity;
        currentTarget.lastTargetPos = bestPos;
    }

    inline void RunTriggerbot() {
        if (!cfg::triggerbot) return;
        if (!(GetAsyncKeyState(cfg::triggerKey) & 0x8000)) return;

        Vec2 currentViewAngles = memory->read<Vec2>(client + Offsets::dwViewAngles);
        Vec3 localPos = localplayer::cameraPos;

        auto currentPlayers = entities::snapshot();

        for (const auto& actor : currentPlayers) {
            if (!actor.actorBase || actor.health <= 0 || actor.bones.empty()) continue;
            if (cfg::teamCheck && actor.team == localplayer::teamid) continue;
            if (!IsVisible(actor)) continue;

            for (const auto& bonePos : actor.bones) {
                if (bonePos.IsZero()) continue;

                Vec2 angleToTarget = CalcAngle(localPos, bonePos);
                float deltaPitch = NormalizePitch(angleToTarget.x - currentViewAngles.x);
                float deltaYaw = NormalizeYaw(angleToTarget.y - currentViewAngles.y);
                float distSq = (deltaPitch * deltaPitch) + (deltaYaw * deltaYaw);

                if (distSq < 2.25f) {
                    int wait = cfg::triggerDelay;
                    if (cfg::triggerJitter > 0)
                        wait += (rand() % (cfg::triggerJitter + 1));
                    if (wait > 0)
                        std::this_thread::sleep_for(std::chrono::milliseconds(wait));
                    Shoot();
                    std::this_thread::sleep_for(std::chrono::milliseconds(0));
                    return;
                }
            }
        }
    }

    inline void AimAtTarget(float smooth) {
        if (!currentTarget.hasTarget || currentTarget.targetEntity <= 0) return;

        // reaction delay: yeni hedefe aninda yapisma
        if (cfg::aimReactMs > 0) {
            double heldMs = (NowSec() - currentTarget.acquireTime) * 1000.0;
            if (heldMs < (double)cfg::aimReactMs) return;
        }

        Vec3 localPos = localplayer::cameraPos;
        Vec2 currentAngles = memory->read<Vec2>(client + Offsets::dwViewAngles);
        Vec3 finalTarget = currentTarget.lastTargetPos;
        Vec2 targetAngle = CalcAngle(localPos, finalTarget);

        targetAngle.x = NormalizePitch(targetAngle.x);
        targetAngle.y = NormalizeYaw(targetAngle.y);

        Vec2 delta = { NormalizePitch(targetAngle.x - currentAngles.x), NormalizeYaw(targetAngle.y - currentAngles.y) };

        // anti-snap: tek tick'te asiri donus yok
        if (cfg::maxSnapDeg > 0.0f) {
            if (delta.x > cfg::maxSnapDeg) delta.x = cfg::maxSnapDeg;
            if (delta.x < -cfg::maxSnapDeg) delta.x = -cfg::maxSnapDeg;
            if (delta.y > cfg::maxSnapDeg) delta.y = cfg::maxSnapDeg;
            if (delta.y < -cfg::maxSnapDeg) delta.y = -cfg::maxSnapDeg;
        }

        float effSmooth = smooth;
        if (cfg::humanize)
            effSmooth = smooth * (0.85f + (float)(rand() % 31) / 100.0f); // +-~%15
        if (effSmooth < 1.0f) effSmooth = 1.0f;

        Vec2 nextAngles = { currentAngles.x + (delta.x / effSmooth), currentAngles.y + (delta.y / effSmooth) };
        memory->write<Vec2>(client + Offsets::dwViewAngles, { NormalizePitch(nextAngles.x), NormalizeYaw(nextAngles.y) });
    }


    inline void doAimbot() {
          FindClosestTarget(cfg::fovSize);
          AimAtTarget(cfg::smoothing);
    }
}