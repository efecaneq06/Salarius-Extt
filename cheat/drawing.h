#ifndef DRAWING_H
#define DRAWING_H
#include "overlay.h"
#include "math.h"

class Drawing {
public:

    void drawBox(const ImVec2& topLeft, const ImVec2& bottomRight, const ImColor& color, float thickness = 1.0f, float rounding = 0.0f) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        drawList->AddRect(topLeft, bottomRight, color, rounding, 0, thickness);
    }

    void drawFilledBox(const ImVec2& topLeft, const ImVec2& bottomRight, const ImColor& color, float thickness = 1.0f, float rounding = 0.0f) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        drawList->AddRectFilled(topLeft, bottomRight, color, rounding, 0);
    }

    void drawLine(const ImVec2& start, const ImVec2& end, const ImColor& color, float thickness = 1.0f) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        drawList->AddLine(start, end, color, thickness);
    }

    void drawLineFG(const ImVec2& start, const ImVec2& end, const ImColor& color, float thickness = 1.0f) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        drawList->AddLine(start, end, color, thickness);
    }

    void drawCircle(const ImVec2& center, float radius, const ImColor& color, float thickness = 1.0f) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        drawList->AddCircle(center, radius, color, 0, thickness);
    }

    void drawFilledCircle(const ImVec2& center, float radius, const ImColor& color) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        drawList->AddCircleFilled(center, radius, color);
    }

    void drawFilledBox(const ImVec2& topLeft, const ImVec2& bottomRight, const ImColor& color, float rounding = 0.0f) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        drawList->AddRectFilled(topLeft, bottomRight, color, rounding);
    }

    void drawText(const ImVec2& position, const char* text, const ImColor& color, float size = 16.0f) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        ImGuiIO& io = ImGui::GetIO();
        float originalFontSize = io.FontGlobalScale;
        io.FontGlobalScale = size / 16.0f;
        drawList->AddText(
            position,
            color,
            text
        );

        io.FontGlobalScale = originalFontSize;
    }
};

#endif
