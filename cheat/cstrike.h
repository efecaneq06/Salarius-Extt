#pragma once
#include "entity.h"
#include "cfg.h"
#include "drawing.h"
#include "math.h"
#include "sdk.h"
#include "weapon_icons.h"

namespace cstrike {
	view_matrix_t matrix;
    view_matrix_t updateMatrix();

	void drawPlayer(entities::PLAYER& actor, ImColor boxCol, ImColor boneCol, const Vec3& localPos);

    Drawing* draw = new Drawing();

    const char* weaponNames[] = {
        "?", "deagle", "elite", "fiveseven", "glock", "?", "?", "ak47", "aug",
        "awp", "famas", "g3sg1", "?", "galil", "m249", "?", "m4a4", "mac10",
        "?", "p90", "?", "?", "?", "mp5sd", "ump45", "xm1014", "bizon",
        "mag7", "negev", "sawedoff", "tec9", "zeus", "p2000", "mp7", "mp9",
        "nova", "p250", "?", "scar20", "sg553", "ssg08", "?", "knife",
        "flashbang", "he", "smoke", "molotov", "decoy", "incendiary", "c4",
        "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
        "m4a1s", "usp", "?", "?", "cz75", "revolver"
    };
}

view_matrix_t cstrike::updateMatrix() {
	return memory->read<view_matrix_t>(client + Offsets::dwViewMatrix);
}

void cstrike::drawPlayer(entities::PLAYER& actor, ImColor boxCol, ImColor boneCol, const Vec3& localPos) {
	if (actor.actorBase == localplayer::pawn)
		return;
	if (actor.PlayerPos.IsZero() || actor.health <= 0)
		return;
    if (cfg::teamCheck && actor.team == localplayer::teamid)
        return;

    Vec3 head_t = { actor.PlayerPos.x, actor.PlayerPos.y, actor.PlayerPos.z + 70.f };
    Vec3 screenPos, screenHead;

    if (!W2S(actor.PlayerPos, screenPos, cstrike::matrix) || !W2S(head_t, screenHead, matrix))
        return;

    float height = std::abs(screenPos.y - screenHead.y);
    float width = height / 2.0f;

    ImVec2 topLeft(screenHead.x - width / 2, screenHead.y);
    ImVec2 bottomRight(topLeft.x + width, topLeft.y + height);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    if (cfg::espOn) {
        dl->AddRect(topLeft, bottomRight, ImColor(0, 0, 0, 200), 0.f, 0, 2.5f);
        dl->AddRect(topLeft, bottomRight, boxCol, 0.f, 0, 1.0f);
    }

    if (cfg::healthBar) {
        float barW = 3.f;
        float barX = topLeft.x - barW - 3.f;
        float healthPct = (float)actor.health / 100.f;
        if (healthPct > 1.f) healthPct = 1.f;

        float barH = height * healthPct;
        float barTop = topLeft.y + (height - barH);

        float r = 1.f - healthPct;
        float g = healthPct;
        ImColor hpColor(r, g, 0.f, 1.f);

        dl->AddRectFilled(ImVec2(barX, topLeft.y), ImVec2(barX + barW, topLeft.y + height), ImColor(0, 0, 0, 180));
        dl->AddRectFilled(ImVec2(barX, barTop), ImVec2(barX + barW, topLeft.y + height), hpColor);
        dl->AddRect(ImVec2(barX, topLeft.y), ImVec2(barX + barW, topLeft.y + height), ImColor(0, 0, 0, 255), 0.f, 0, 1.f);
    }

    if (cfg::healthText) {
        char hpBuf[16];
        snprintf(hpBuf, sizeof(hpBuf), "%d HP", actor.health);
        ImVec2 textSize = ImGui::CalcTextSize(hpBuf);
        ImVec2 textPos(screenHead.x - textSize.x / 2, topLeft.y - textSize.y - 2);
        dl->AddText(ImVec2(textPos.x + 1, textPos.y + 1), ImColor(0, 0, 0, 255), hpBuf);
        dl->AddText(textPos, ImColor(255, 255, 255, 255), hpBuf);
    }

    if (cfg::weaponESP && actor.weaponame > 0) {
        float iconH = 0.f;
        ID3D11ShaderResourceView* icon = weapon_icons::Get(actor.weaponame);
        if (icon) {
            float ih = 16.f;
            float aspect = weapon_icons::GetAspect(actor.weaponame);
            float iw = ih * aspect;
            ImVec2 iconPos(screenHead.x - iw / 2, bottomRight.y + 2);
            dl->AddImage((ImTextureID)icon, iconPos, ImVec2(iconPos.x + iw, iconPos.y + ih),
                ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 230));
            iconH = ih;
        } else {
            const char* wepName = "?";
            if (actor.weaponame >= 0 && actor.weaponame < 65)
                wepName = weaponNames[actor.weaponame];
            ImVec2 textSize = ImGui::CalcTextSize(wepName);
            ImVec2 textPos(screenHead.x - textSize.x / 2, bottomRight.y + 2);
            dl->AddText(ImVec2(textPos.x + 1, textPos.y + 1), ImColor(0, 0, 0, 255), wepName);
            dl->AddText(textPos, ImColor(200, 200, 200, 255), wepName);
            iconH = textSize.y;
        }
    }

    if (cfg::distanceESP) {
        float dx = actor.PlayerPos.x - localPos.x;
        float dy = actor.PlayerPos.y - localPos.y;
        float dz = actor.PlayerPos.z - localPos.z;
        float dist = sqrtf(dx*dx + dy*dy + dz*dz) / 100.f;
        char distBuf[16];
        snprintf(distBuf, sizeof(distBuf), "%.0fm", dist);
        ImVec2 textSize = ImGui::CalcTextSize(distBuf);
        float yOff = cfg::weaponESP ? 16.f : 2.f;
        ImVec2 textPos(screenHead.x - textSize.x / 2, bottomRight.y + yOff);
        dl->AddText(ImVec2(textPos.x + 1, textPos.y + 1), ImColor(0, 0, 0, 255), distBuf);
        dl->AddText(textPos, ImColor(180, 180, 180, 255), distBuf);
    }

    if (cfg::snapLines) {
        float screenW = (float)GetSystemMetrics(SM_CXSCREEN);
        float screenH = (float)GetSystemMetrics(SM_CYSCREEN);
        ImVec2 bottom(screenW / 2, screenH);
        ImVec2 target(screenPos.x, screenPos.y);
        dl->AddLine(bottom, target, ImColor(255, 255, 255, 100), 1.f);
    }

    if (cfg::bones && !actor.bones.empty()) {
        std::vector<ImVec2> screenBones(actor.bones.size());
        std::vector<bool> boneVisible(actor.bones.size(), false);

        for (size_t i = 0; i < actor.bones.size(); ++i) {
            Vec3 out;
            if (W2S(actor.bones[i], out, matrix)) {
                screenBones[i] = ImVec2(out.x, out.y);
                boneVisible[i] = true;
            }
        }

        struct bone_link { int a, b; };
        static bone_link skeleton_links[] = {
            { entities::PELVIS, entities::SPINE1 }, { entities::SPINE1, entities::SPINE2 },
            { entities::SPINE2, entities::CHEST }, { entities::CHEST, entities::NECK },
            { entities::NECK, entities::HEAD },
            { entities::NECK, entities::SHOULDER_L }, { entities::SHOULDER_L, entities::ELBOW_L }, { entities::ELBOW_L, entities::HAND_L },
            { entities::NECK, entities::SHOULDER_R }, { entities::SHOULDER_R, entities::ELBOW_R }, { entities::ELBOW_R, entities::HAND_R },
            { entities::PELVIS, entities::HIP_L }, { entities::HIP_L, entities::KNEE_L }, { entities::KNEE_L, entities::FOOT_HEEL_L },
            { entities::PELVIS, entities::HIP_R }, { entities::HIP_R, entities::KNEE_R }, { entities::KNEE_R, entities::FOOT_HEEL_R }
        };

        for (auto& link : skeleton_links) {
            if (link.a < actor.bones.size() && link.b < actor.bones.size()) {
                if (boneVisible[link.a] && boneVisible[link.b]) {
                    dl->AddLine(screenBones[link.a], screenBones[link.b], boneCol, 1.5f);
                }
            }
        }
    }
}
