#pragma once
#include "../ext/imgui/imgui.h"
#include "entity.h"
#include "cfg.h"
#include <cmath>

namespace Radar2D {

    float GetDistance(const Vec3& pos1, const Vec3& pos2) {
        float dx = pos1.x - pos2.x;
        float dy = pos1.y - pos2.y;
        float dz = pos1.z - pos2.z;
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }

    ImVec2 WorldToRadar(const Vec3& worldPos, const Vec3& localPos, float cx, float cy,
                        float radarSize, float range, bool rotate, float viewYaw) {
        float dx = worldPos.x - localPos.x;
        float dy = worldPos.y - localPos.y;

        float yaw = rotate ? viewYaw : 90.0f;
        float yawRad = yaw * (3.14159265f / 180.0f);

        float fwdX = cosf(yawRad), fwdY = sinf(yawRad);
        float rgtX = sinf(yawRad), rgtY = -cosf(yawRad);

        float forwardDist = dx * fwdX + dy * fwdY;
        float rightDist   = dx * rgtX + dy * rgtY;

        float screenX = rightDist;
        float screenY = -forwardDist;

        float scale = (radarSize / 2.0f) / range;
        float radarX = cx + screenX * scale;
        float radarY = cy + screenY * scale;

        float distFromCenter = sqrtf(powf(radarX - cx, 2) + powf(radarY - cy, 2));
        float maxDist = radarSize / 2.0f;
        if (distFromCenter > maxDist) {
            float angle = atan2f(radarY - cy, radarX - cx);
            radarX = cx + cosf(angle) * (maxDist - 5.0f);
            radarY = cy + sinf(angle) * (maxDist - 5.0f);
        }

        return ImVec2(radarX, radarY);
    }

    void DrawRadar() {
        if (!cfg::radar2D) return;

        uintptr_t localPawn = localplayer::G_Pawn();
        if (localPawn == 0) return;

        Vec3 localPos = localplayer::G_Pos();
        if (localPos.IsNull()) return;

        float viewYaw = 0.0f;
        if (cfg::radarRotate) {
            Vec2 viewAngles = memory->read<Vec2>(client + Offsets::dwViewAngles);
            viewYaw = viewAngles.y;
        }

        float sz = cfg::radarSize;
        float r = sz / 2.0f;
        float padding = 10.0f;
        float winSize = sz + padding * 2;

        ImGui::SetNextWindowSize(ImVec2(winSize, winSize + 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(100, 100));

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoBackground;

        ImGui::Begin("##radar_window", nullptr, flags);
        ImVec2 winPos = ImGui::GetWindowPos();
        float cx = winPos.x + winSize / 2.0f;
        float cy = winPos.y + winSize / 2.0f;
        ImGui::End();
        ImGui::PopStyleVar(3);

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.07f, 0.12f, cfg::radarAlpha));
        ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.35f, 0.5f, 1.0f));
        ImU32 gridColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.18f, 0.25f, 0.6f));

        if (cfg::radarStyle == 0) {
            drawList->AddCircleFilled(ImVec2(cx, cy), r, bgColor, 64);
            drawList->AddCircle(ImVec2(cx, cy), r, borderColor, 64, 2.0f);
            drawList->AddCircle(ImVec2(cx, cy), r * 0.33f, gridColor, 64, 1.0f);
            drawList->AddCircle(ImVec2(cx, cy), r * 0.66f, gridColor, 64, 1.0f);
        } else {
            ImVec2 tl(cx - r, cy - r);
            ImVec2 br(cx + r, cy + r);
            drawList->AddRectFilled(tl, br, bgColor, 5.0f);
            drawList->AddRect(tl, br, borderColor, 5.0f, 0, 2.0f);
            drawList->AddLine(ImVec2(cx, tl.y), ImVec2(cx, br.y), gridColor, 1.0f);
            drawList->AddLine(ImVec2(tl.x, cy), ImVec2(br.x, cy), gridColor, 1.0f);
        }

        drawList->AddLine(ImVec2(cx - r, cy), ImVec2(cx + r, cy), gridColor, 1.0f);
        drawList->AddLine(ImVec2(cx, cy - r), ImVec2(cx, cy + r), gridColor, 1.0f);

        ImU32 crossColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.8f));
        drawList->AddLine(ImVec2(cx - 5, cy), ImVec2(cx + 5, cy), crossColor, 1.5f);
        drawList->AddLine(ImVec2(cx, cy - 5), ImVec2(cx, cy + 5), crossColor, 1.5f);

        if (cfg::radarRotate) {
            ImU32 arrowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.9f, 0.4f, 0.9f));
            drawList->AddTriangleFilled(
                ImVec2(cx, cy - r + 12), ImVec2(cx - 5, cy - r + 19), ImVec2(cx + 5, cy - r + 19), arrowColor);
        }

        ImU32 localColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
        ImU32 outlineColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        drawList->AddCircleFilled(ImVec2(cx, cy), 4.0f, localColor, 16);
        drawList->AddCircle(ImVec2(cx, cy), 4.0f, outlineColor, 16, 1.5f);

        for (const auto& actor : entities::snapshot()) {
            if (actor.health <= 0) continue;
            if (actor.actorBase == localplayer::G_Pawn()) continue;

            Vec3 entityPos = actor.PlayerPos;
            if (entityPos.IsNull()) continue;

            ImVec2 radarPos = WorldToRadar(entityPos, localPos, cx, cy, sz, cfg::radarRange, cfg::radarRotate, viewYaw);

            ImU32 dotColor;
            bool isTeammate = (actor.team == localplayer::teamid);
            if (isTeammate) {
                dotColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.5f, 1.0f, 0.9f));
            } else {
                dotColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.15f, 0.15f, 1.0f));
            }

            float dotSize = isTeammate ? 3.5f : 4.5f;

            drawList->AddCircleFilled(radarPos, dotSize, dotColor, 16);
            drawList->AddCircle(radarPos, dotSize, outlineColor, 16, 1.0f);

            if (!isTeammate) {
                float hw = 15.0f, hh = 2.0f;
                float hp = (float)actor.health / 100.0f;
                ImVec2 bs(radarPos.x - hw / 2.0f, radarPos.y - 8.0f);
                ImVec2 be(radarPos.x + hw / 2.0f, radarPos.y - 8.0f + hh);

                drawList->AddRectFilled(bs, be, ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.15f, 0.15f, 0.8f)));

                ImU32 hpColor;
                if (hp > 0.6f) hpColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.0f, 0.9f));
                else if (hp > 0.3f) hpColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 0.0f, 0.9f));
                else hpColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.0f, 0.0f, 0.9f));

                drawList->AddRectFilled(bs, ImVec2(bs.x + hw * hp, be.y), hpColor);
            }

            if (cfg::radarShowDistance && !isTeammate) {
                float dist = GetDistance(localPos, entityPos);
                int meters = (int)(dist * 0.0254f);
                char buf[16];
                snprintf(buf, sizeof(buf), "%dm", meters);
                ImVec2 ts = ImGui::CalcTextSize(buf);
                ImVec2 tp(radarPos.x - ts.x / 2.0f, radarPos.y + 8.0f);
                drawList->AddText(ImVec2(tp.x + 1, tp.y + 1), outlineColor, buf);
                drawList->AddText(tp, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.9f)), buf);
            }
        }
    }
}
