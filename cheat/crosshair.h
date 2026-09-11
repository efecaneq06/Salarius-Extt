#pragma once
// Ozel crosshair overlay: ekran merkezinde ayarlanabilir arti + nokta.
// Oyun ici crosshair'dan bagimsiz, her cozunurlukte ortali.
// SADECE main.cpp TU'sunda kullanilir (render loop'tan Draw cagrilir).
#include "cfg.h"
#include "sdk.h"

namespace crosshair
{
    inline void Draw() {
        if (!cfg::crosshair) return;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImVec2 c = SDK::screenCenter;

        float gap = (float)cfg::crossGap;
        float len = (float)cfg::crossSize;
        ImU32 col = IM_COL32((int)(cfg::crossColor[0] * 255), (int)(cfg::crossColor[1] * 255),
                             (int)(cfg::crossColor[2] * 255), (int)(cfg::crossColor[3] * 255));
        ImU32 shadow = IM_COL32(0, 0, 0, 180);

        auto hline = [&](float x0, float x1, float y) {
            dl->AddLine(ImVec2(x0, y + 1), ImVec2(x1, y + 1), shadow, 1.0f);
            dl->AddLine(ImVec2(x0, y), ImVec2(x1, y), col, 1.0f);
        };
        auto vline = [&](float x, float y0, float y1) {
            dl->AddLine(ImVec2(x + 1, y0), ImVec2(x + 1, y1), shadow, 1.0f);
            dl->AddLine(ImVec2(x, y0), ImVec2(x, y1), col, 1.0f);
        };

        hline(c.x - gap - len, c.x - gap, c.y); // sol
        hline(c.x + gap, c.x + gap + len, c.y); // sag
        vline(c.x, c.y - gap - len, c.y - gap); // ust
        vline(c.x, c.y + gap, c.y + gap + len); // alt

        if (cfg::crossDot)
            dl->AddCircleFilled(ImVec2(c.x, c.y), 1.5f, col);
    }
}
