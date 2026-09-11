#pragma once
// Hitmarker + hasar logu: dusman cani dususlerini yakalar.
// Poll() mainThread'den (ImGui yok), Draw() render loop'tan cagrilir.
// SADECE main.cpp TU'sunda kullanilir.
#include "cstrike.h"
#include "cfg.h"
#include <mutex>
#include <unordered_map>

namespace hitmarker
{
    struct DmgEvt {
        char txt[48] = {};
        Vec3 pos = {};
        double t = 0.0;
    };

    inline std::vector<DmgEvt> evts;
    inline std::mutex evtx;
    inline std::unordered_map<uintptr_t, int> lastHp;
    inline std::unordered_map<uintptr_t, double> lastDmgT; // pawn -> son vurus ani
    inline double lastHitT = -100.0;
    inline double killFlashT = -100.0;
    inline char killTxt[64] = {};
    inline int kills = 0; // oturum kill sayaci

    inline double Now() {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Beep blokladigi icin ayri thread'de cal. Proof acikken sessiz (yayin sesine cikar).
    inline void BeepAsync(int freq, int ms) {
        if (cfg::streamproof) return;
        std::thread([freq, ms]() { Beep(freq, ms); }).detach();
    }
    inline void KillSoundAsync() {
        if (cfg::streamproof) return;
        std::thread([]() { Beep(880, 70); Beep(1320, 110); }).detach();
    }

    inline void Poll() {
        if (!cfg::hitmarker && !cfg::hitSound && !cfg::killSound) {
            if (!lastHp.empty()) lastHp.clear();
            if (!lastDmgT.empty()) lastDmgT.clear();
            return;
        }
        auto snap = entities::snapshot();
        std::unordered_map<uintptr_t, int> cur;
        cur.reserve(snap.size() * 2 + 1);
        double now = Now();
        for (auto& a : snap) {
            if (!a.actorBase) continue;
            cur[a.actorBase] = a.health;
            auto it = lastHp.find(a.actorBase);
            if (it == lastHp.end()) continue;
            int prev = it->second;
            // gercek hasar: can dustu (olum dahil)
            if (a.health < prev && prev <= 100 && a.health >= 0) {
                bool isEnemy = (!cfg::teamCheck) || (a.team != localplayer::teamid);
                if (!isEnemy) continue;
                int dmg = prev - a.health;
                if (dmg <= 0 || dmg > 100) continue;
                // kill: bu pawn'a daha once vurduysak (2sn icinde) bizim kill'imiz say
                bool myKill = false;
                if (a.health <= 0) {
                    auto dt = lastDmgT.find(a.actorBase);
                    if (dt != lastDmgT.end() && now - dt->second < 2.0) myKill = true;
                }
                lastDmgT[a.actorBase] = now;
                if (cfg::hitSound) BeepAsync(1250, 25);
                if (myKill) {
                    kills++;
                    snprintf(killTxt, sizeof(killTxt), "KILL  %s", a.name.c_str());
                    killFlashT = now;
                    if (cfg::killSound) KillSoundAsync();
                }
                if (!cfg::hitmarker) continue;
                DmgEvt e;
                snprintf(e.txt, sizeof(e.txt), "%s -%d", a.name.c_str(), dmg);
                e.pos = a.PlayerPos;
                e.t = now;
                std::lock_guard<std::mutex> lk(evtx);
                evts.push_back(e);
                if (evts.size() > 32) evts.erase(evts.begin(), evts.begin() + (evts.size() - 32));
                lastHitT = now;
            }
        }
        lastHp.swap(cur);
        // eski event temizligi (Poll'de de buda, Draw rahat calsin)
        std::lock_guard<std::mutex> lk(evtx);
        while (!evts.empty() && now - evts.front().t > 4.0)
            evts.erase(evts.begin());
        if (lastDmgT.size() > 64) {
            for (auto it = lastDmgT.begin(); it != lastDmgT.end();) {
                if (now - it->second > 5.0) it = lastDmgT.erase(it);
                else ++it;
            }
        }
    }

    inline void Draw() {
        double now = Now();
        ImDrawList* dl = ImGui::GetBackgroundDrawList();

        // kill yazisi (killSound aciksa hitmarker kapaliyken de goster)
        if ((cfg::hitmarker || cfg::killSound) && now - killFlashT < 1.2) {
            float a = 1.0f - (float)((now - killFlashT) / 1.2);
            ImVec2 ts = ImGui::CalcTextSize(killTxt);
            ImVec2 tp(SDK::screenCenter.x - ts.x / 2, SDK::screenCenter.y + 44.0f);
            dl->AddText(ImVec2(tp.x + 1, tp.y + 1), IM_COL32(0, 0, 0, (int)(255 * a)), killTxt);
            dl->AddText(tp, IM_COL32(255, 200, 60, (int)(255 * a)), killTxt);
        }

        if (!cfg::hitmarker) return;

        // crosshair X (0.35sn)
        if (now - lastHitT < 0.35) {
            ImVec2 c = SDK::screenCenter;
            float g = 7.0f, l = 6.0f;
            ImU32 col = IM_COL32(255, 60, 60, 255);
            dl->AddLine(ImVec2(c.x - g - l, c.y - g - l), ImVec2(c.x - g, c.y - g), col, 2.0f);
            dl->AddLine(ImVec2(c.x + g, c.y - g), ImVec2(c.x + g + l, c.y - g - l), col, 2.0f);
            dl->AddLine(ImVec2(c.x - g - l, c.y + g + l), ImVec2(c.x - g, c.y + g), col, 2.0f);
            dl->AddLine(ImVec2(c.x + g, c.y + g), ImVec2(c.x + g + l, c.y + g + l), col, 2.0f);
        }

        std::vector<DmgEvt> copy;
        { std::lock_guard<std::mutex> lk(evtx); copy = evts; }
        if (copy.empty()) return;

        // yukselen hasar sayilari (1sn)
        for (auto& e : copy) {
            double age = now - e.t;
            if (age > 1.0) continue;
            Vec3 wp = e.pos;
            wp.z += 62.0f - (float)(age * 28.0);
            Vec3 sp;
            if (!W2S(wp, sp, cstrike::matrix)) continue;
            float a = 1.0f - (float)(age / 1.0);
            ImVec2 ts = ImGui::CalcTextSize(e.txt);
            ImVec2 tp(sp.x - ts.x / 2, sp.y);
            dl->AddText(ImVec2(tp.x + 1, tp.y + 1), IM_COL32(0, 0, 0, (int)(255 * a)), e.txt);
            dl->AddText(tp, IM_COL32(255, 90, 90, (int)(255 * a)), e.txt);
        }

        // sag ust log (4sn, son 6)
        float sw = (float)GetSystemMetrics(SM_CXSCREEN);
        float ly = 90.0f;
        if (kills > 0) {
            char kb[24];
            snprintf(kb, sizeof(kb), "Kills: %d", kills);
            ImVec2 ts = ImGui::CalcTextSize(kb);
            ImVec2 tp(sw - ts.x - 16.0f, ly);
            dl->AddText(ImVec2(tp.x + 1, tp.y + 1), IM_COL32(0, 0, 0, 255), kb);
            dl->AddText(tp, IM_COL32(255, 200, 60, 255), kb);
            ly += 20.0f;
        }
        int shown = 0;
        for (int i = (int)copy.size() - 1; i >= 0 && shown < 6; --i) {
            double age = now - copy[i].t;
            if (age > 4.0) continue;
            float a = age > 3.0 ? (float)(4.0 - age) : 1.0f;
            ImVec2 ts = ImGui::CalcTextSize(copy[i].txt);
            ImVec2 tp(sw - ts.x - 16.0f, ly);
            dl->AddText(ImVec2(tp.x + 1, tp.y + 1), IM_COL32(0, 0, 0, (int)(255 * a)), copy[i].txt);
            dl->AddText(tp, IM_COL32(230, 230, 230, (int)(230 * a)), copy[i].txt);
            ly += 18.0f;
            shown++;
        }
    }
}
