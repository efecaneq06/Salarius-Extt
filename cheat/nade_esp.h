#pragma once
// Nade ESP: ucus halindeki el bombalari + molotof atesi + dusmus C4'u isaretler.
// Entity kimligi entity+0x10 (identity) -> identity+0x20 (designerName) uzerinden cozulur.
// Tarama 400ms'de bir ayri thread'de, cizim render loop'ta.
// SADECE main.cpp TU'sunda kullanilir.
#include "cstrike.h"
#include "cfg.h"
#include <mutex>
#include <cstring>

namespace nade_esp
{
    struct NADE {
        Vec3 pos = {};
        char label[12] = {};
        ImU32 color = 0;
    };

    inline std::vector<NADE> items;
    inline std::mutex mtx;

    inline bool MatchLabel(const char* designer, char* outLabel, size_t outSz, ImU32& color) {
        struct Pair { const char* sub; const char* label; ImU32 col; };
        static const Pair table[] = {
            { "hegrenade_projectile",  "HE",     IM_COL32(255, 70, 70, 255) },
            { "flashbang_projectile",  "FLASH",  IM_COL32(255, 235, 120, 255) },
            { "smokegrenade_projectile","SMOKE", IM_COL32(180, 180, 180, 255) },
            { "molotov_projectile",    "FIRE",   IM_COL32(255, 150, 40, 255) },
            { "inferno",               "FIRE",   IM_COL32(255, 120, 30, 255) },
            { "decoy_projectile",      "DECOY",  IM_COL32(190, 120, 255, 255) },
            { "weapon_c4",             "C4",     IM_COL32(120, 255, 120, 255) },
        };
        for (auto& p : table) {
            if (strstr(designer, p.sub)) {
                strncpy_s(outLabel, outSz, p.label, _TRUNCATE);
                color = p.col;
                return true;
            }
        }
        return false;
    }

    inline void scanLoop() {
        while (true) {
            if (!cfg::nadeESP || !client) {
                { std::lock_guard<std::mutex> lk(mtx); items.clear(); }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            std::vector<NADE> found;
            found.reserve(8);

            uintptr_t entityList = memory->read<uintptr_t>(client + Offsets::dwEntityList);
            if (entityList) {
                // 64..1024: oyuncu disi entity'ler (projectile/inferno/silah)
                uintptr_t chunk = 0;
                int chunkGroup = -1;
                for (int idx = 64; idx < 1024 && found.size() < 24; ++idx) {
                    int group = idx >> 9;
                    if (group != chunkGroup) {
                        chunkGroup = group;
                        chunk = memory->read<uintptr_t>(entityList + (uintptr_t)0x8 * (uintptr_t)group + 0x10);
                        if (!chunk) continue;
                    }
                    if (!chunk) continue;
                    uintptr_t ent = memory->read<uintptr_t>(chunk + (uintptr_t)0x70 * (uintptr_t)(idx & 0x1FF));
                    if (!ent || ent >= 0x7FFFFFFFFFFF) continue;

                    uintptr_t identity = memory->read<uintptr_t>(ent + 0x10);
                    if (!identity || identity >= 0x7FFFFFFFFFFF) continue;
                    uintptr_t namePtr = memory->read<uintptr_t>(identity + Offsets::m_designerName);
                    if (!namePtr || namePtr >= 0x7FFFFFFFFFFF) continue;
                    std::string designer = memory->read_string(namePtr, 48);
                    if (designer.empty()) continue;

                    char label[12] = {};
                    ImU32 color = 0;
                    if (!MatchLabel(designer.c_str(), label, sizeof(label), color)) continue;

                    uintptr_t node = memory->read<uintptr_t>(ent + Offsets::m_pGameSceneNode);
                    if (!node) continue;
                    Vec3 pos = memory->read<Vec3>(node + Offsets::m_vecAbsOrigin);
                    if (pos.IsZero()) continue;

                    NADE n;
                    n.pos = pos;
                    memcpy(n.label, label, sizeof(label));
                    n.color = color;
                    found.push_back(n);
                }
            }
            { std::lock_guard<std::mutex> lk(mtx); items.swap(found); }
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
    }

    inline void Draw(const Vec3& localPos) {
        if (!cfg::nadeESP) return;
        std::vector<NADE> copy;
        { std::lock_guard<std::mutex> lk(mtx); copy = items; }
        if (copy.empty()) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        for (auto& n : copy) {
            Vec3 sp;
            if (!W2S(n.pos, sp, cstrike::matrix)) continue;
            float dx = n.pos.x - localPos.x, dy = n.pos.y - localPos.y, dz = n.pos.z - localPos.z;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz) / 100.f;
            char buf[32];
            snprintf(buf, sizeof(buf), "%s %.0fm", n.label, (double)dist);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            ImVec2 tp(sp.x - ts.x / 2, sp.y - ts.y / 2);
            dl->AddText(ImVec2(tp.x + 1, tp.y + 1), IM_COL32(0, 0, 0, 255), buf);
            dl->AddText(tp, n.color, buf);
        }
    }

    inline void init() {
        std::thread([]() { scanLoop(); }).detach();
    }
}
