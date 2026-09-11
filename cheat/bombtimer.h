#pragma once
#include "sdk.h"
#include <Windows.h>
#include <thread>
#include <mutex>
#include <chrono>
#include "cfg.h"

namespace bombtimer
{
    static float lastValue = 0.0f;
    static ULONGLONG lastChangeTime = 0;

    struct Cache {
        bool has = false;
        bool ticking = false;
        bool defused = false;
        bool beingDefused = false;
        float blow = 0.0f;
        float baseTime = 0.0f;
        float timerLength = 40.0f;
        double clock = 0.0;
    };
    inline Cache g_cache;
    inline std::mutex g_mutex;

    inline double nowSec() {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    inline void cacheLoop() {
        while (true) {
            Cache c;
            if (cfg::bombtimer && client && Offsets::dwPlantedC4) {
                uintptr_t plantedC4Base = memory->read<uintptr_t>(client + Offsets::dwPlantedC4);
                if (plantedC4Base) {
                    uintptr_t bomb = memory->read<uintptr_t>(plantedC4Base);
                    if (bomb) {
                        c.has = true;
                        c.ticking = memory->read<bool>(bomb + Offsets::m_bBombTicking);
                        c.defused = memory->read<bool>(bomb + Offsets::m_bBombDefused);
                        c.blow = memory->read<float>(bomb + Offsets::m_flC4Blow);
                        c.baseTime = memory->read<float>(bomb + Offsets::m_flNextGlow);
                        float tl = memory->read<float>(bomb + Offsets::m_flTimerLength);
                        c.timerLength = (tl > 0) ? tl : 40.0f;
                        c.beingDefused = memory->read<bool>(bomb + Offsets::m_bBeingDefused);
                        c.clock = nowSec();
                    }
                }
            }
            {
                std::lock_guard<std::mutex> lk(g_mutex);
                g_cache = c;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    inline void init() {
        std::thread([]() { cacheLoop(); }).detach();
    }

    inline void Draw()
    {
        if (!cfg::bombtimer)
            return;

        Cache c;
        {
            std::lock_guard<std::mutex> lk(g_mutex);
            c = g_cache;
        }
        if (!c.has) return;

        // cache 10Hz tazelenir, aradaki sure yerel saatle yurutulur -> sayac akici
        float currentTime = c.baseTime + (float)(nowSec() - c.clock);
        float timeLeft = c.blow - currentTime;

        ULONGLONG now = GetTickCount64();
        if (timeLeft != lastValue) { lastValue = timeLeft; lastChangeTime = now; }
        if (!c.ticking || c.defused || timeLeft <= 0.0f || timeLeft > 40.5f || (now - lastChangeTime > 2000))
            return;

        const float windowWidth = 180.0f;
        ImVec4 accent = ImVec4(0.00f, 0.55f, 1.00f, 1.00f);
        ImVec4 bg = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);

        ImGuiIO& io = ImGui::GetIO();
        
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - windowWidth * 0.5f, 50.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(windowWidth, 0), ImGuiCond_Always);

        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg);
        ImGui::PushStyleColor(ImGuiCol_Border, accent);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8)); 
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));    

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

        if (ImGui::Begin("##BombTimer", nullptr, flags))
        {
            ImVec2 p = ImGui::GetWindowPos();
            ImGui::GetWindowDrawList()->AddRectFilled(
                p, ImVec2(p.x + ImGui::GetWindowWidth(), p.y + 2),
                ImGui::GetColorU32(accent)
            );

            ImGui::Dummy(ImVec2(0, 2)); 

            ImGui::TextColored(accent, "C4 PLANTED");
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("%.1fs").x - 10);
            ImGui::Text("%.1fs", timeLeft);

            float progress = timeLeft / c.timerLength;

            ImVec4 barColor = accent;
            if (timeLeft < 10.0f) barColor = ImVec4(1.0f, 0.75f, 0.0f, 1.0f);
            if (timeLeft < 5.0f)  barColor = ImVec4(1.0f, 0.20f, 0.20f, 1.00f);

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));
            ImGui::ProgressBar(progress, ImVec2(-1, 4), "");
            ImGui::PopStyleColor(2);

            if (c.beingDefused) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "> DEFUSING");
            }
            else {
                ImGui::Dummy(ImVec2(0, 1));
            }
        }
        ImGui::End();

        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor(2);
    }
}