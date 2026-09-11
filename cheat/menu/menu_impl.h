#pragma once
// project/ sablonundan portlandi, tum kontroller cfg::'ye bagli.
// SADECE main.cpp tarafindan include edilir (tek TU).

#include "../overlay.h"
#include "../cfg.h"
#include "../config_system.h"
#include "../offset_updater.h"
#include "fonts.h"
#include "images.h"
#include "../../ext/nanosvg.h"
#include "../../ext/nanosvgrast.h"
#include "menu.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <filesystem>

namespace ui
{
    ImFont* inter_bold = nullptr;
    ImFont* inter_bold_13 = nullptr;
    ImFont* inter_semibold = nullptr;
    ImFont* inter_semibold_10 = nullptr;
    ImFont* inter_semibold_11 = nullptr;
    ImFont* inter_semibold_11_02 = nullptr;
    ImFont* inter_semibold_11_58 = nullptr;
    ImFont* inter_semibold_12_8 = nullptr;
    ImFont* inter_semibold_18 = nullptr;
    ImFont* inter_semibold_21 = nullptr;
    ImFont* inter_semibold_24 = nullptr;

    ImTextureID top_textures_grey[3] = { (ImTextureID)0, (ImTextureID)0, (ImTextureID)0 };
    ImTextureID top_textures_blue[3] = { (ImTextureID)0, (ImTextureID)0, (ImTextureID)0 };

    ImTextureID side_textures_grey[4] = { (ImTextureID)0, (ImTextureID)0, (ImTextureID)0, (ImTextureID)0 };
    ImTextureID side_textures_blue[4] = { (ImTextureID)0, (ImTextureID)0, (ImTextureID)0, (ImTextureID)0 };

    ImTextureID CreateGPUTexture(unsigned char* pixels, int width, int height)
    {
        if (!overlay::g_pd3dDevice || !pixels) return (ImTextureID)0;

        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = pixels;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;

        ID3D11Texture2D* pTexture = nullptr;
        if (FAILED(overlay::g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture)))
            return (ImTextureID)0;

        ID3D11ShaderResourceView* pTextureView = nullptr;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

        if (FAILED(overlay::g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &pTextureView))) {
            pTexture->Release();
            return (ImTextureID)0;
        }

        pTexture->Release();
        return (ImTextureID)pTextureView;
    }

    static ImTextureID LoadTextureFromSvg(const unsigned char* memory_data, int memory_len)
    {
        if (!memory_data || memory_len < 1) return (ImTextureID)0;

        std::vector<char> svg_buffer((size_t)memory_len + 1);
        std::memcpy(svg_buffer.data(), memory_data, (size_t)memory_len);
        svg_buffer[memory_len] = '\0';

        NSVGimage* image = nsvgParse(svg_buffer.data(), "px", 96.0f);
        if (!image) return (ImTextureID)0;

        int width = (int)(image->width + 0.5f);
        int height = (int)(image->height + 0.5f);
        if (width < 1) width = 1;
        if (height < 1) height = 1;

        static NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
        if (!rasterizer) {
            nsvgDelete(image);
            return (ImTextureID)0;
        }

        size_t pixel_count = (size_t)width * (size_t)height * 4;
        unsigned char* pixels = (unsigned char*)std::malloc(pixel_count);
        if (!pixels) {
            nsvgDelete(image);
            return (ImTextureID)0;
        }

        nsvgRasterize(rasterizer, image, 0.0f, 0.0f, 1.0f, pixels, width, height, width * 4);
        nsvgDelete(image);

        ImTextureID texture = CreateGPUTexture(pixels, width, height);
        std::free(pixels);
        return texture;
    }

    ImTextureID LoadTexture(const unsigned char* memory_data, int memory_len)
    {
        return LoadTextureFromSvg(memory_data, memory_len);
    }

    void DrawIcon(ImDrawList* dl, ImVec2 pos, ImTextureID texture, ImVec2 size = ImVec2(18.0f, 18.0f))
    {
        if (texture) {
            dl->AddImage(texture, pos, ImVec2(pos.x + size.x, pos.y + size.y));
        }
    }

    void ShadeVertsGradient(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 p0, ImVec2 p1, ImColor col0, ImColor col1)
    {
        ImVec2 d = ImVec2(p1.x - p0.x, p1.y - p0.y);
        float len2 = d.x * d.x + d.y * d.y;
        if (len2 < 0.00001f) return;
        ImDrawVert* v_start = draw_list->VtxBuffer.Data + vert_start_idx;
        ImDrawVert* v_end = draw_list->VtxBuffer.Data + vert_end_idx;
        ImVec4 c0 = col0.Value;
        ImVec4 c1 = col1.Value;
        for (ImDrawVert* v = v_start; v < v_end; ++v) {
            float t = ((v->pos.x - p0.x) * d.x + (v->pos.y - p0.y) * d.y) / len2;
            t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
            float r = c0.x + t * (c1.x - c0.x);
            float g = c0.y + t * (c1.y - c0.y);
            float b = c0.z + t * (c1.z - c0.z);
            float a = c0.w + t * (c1.w - c0.w);
            float original_alpha = (float)((v->col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
            v->col = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, a * original_alpha));
        }
    }

    float AnimateFloat(const char* label, float target, float speed = 12.0f)
    {
        ImGuiID id = ImGui::GetID(label);
        float* val_ptr = ImGui::GetStateStorage()->GetFloatRef(id, 0.0f);
        *val_ptr += (target - *val_ptr) * ImClamp(ImGui::GetIO().DeltaTime * speed, 0.0f, 1.0f);
        return *val_ptr;
    }

    ImColor LerpColor(ImColor c1, ImColor c2, float t)
    {
        ImVec4 v1 = c1.Value;
        ImVec4 v2 = c2.Value;
        return ImColor(
            v1.x + t * (v2.x - v1.x),
            v1.y + t * (v2.y - v1.y),
            v1.z + t * (v2.z - v1.z),
            v1.w + t * (v2.w - v1.w)
        );
    }

    const char* KeyName(int vk) {
        static thread_local char buf[16];
        switch (vk) {
        case 0: return "None";
        case VK_LBUTTON: return "LMB";
        case VK_RBUTTON: return "RMB";
        case VK_MBUTTON: return "MMB";
        case VK_XBUTTON1: return "Mouse 4";
        case VK_XBUTTON2: return "Mouse 5";
        case VK_BACK: return "Backsp";
        case VK_TAB: return "Tab";
        case VK_RETURN: return "Enter";
        case VK_SHIFT: return "Shift";
        case VK_CONTROL: return "Ctrl";
        case VK_MENU: return "Alt";
        case VK_CAPITAL: return "Caps";
        case VK_ESCAPE: return "Esc";
        case VK_SPACE: return "Space";
        case VK_PRIOR: return "PgUp";
        case VK_NEXT: return "PgDn";
        case VK_END: return "End";
        case VK_HOME: return "Home";
        case VK_LEFT: return "Left";
        case VK_UP: return "Up";
        case VK_RIGHT: return "Right";
        case VK_DOWN: return "Down";
        case VK_INSERT: return "Insert";
        case VK_DELETE: return "Delete";
        default: break;
        }
        if (vk >= 0x30 && vk <= 0x39) { buf[0] = (char)vk; buf[1] = 0; return buf; }
        if (vk >= 0x41 && vk <= 0x5A) { buf[0] = (char)vk; buf[1] = 0; return buf; }
        if (vk >= VK_F1 && vk <= VK_F24) { snprintf(buf, sizeof(buf), "F%d", vk - VK_F1 + 1); return buf; }
        snprintf(buf, sizeof(buf), "0x%X", vk);
        return buf;
    }

    template<size_t N>
    constexpr int arrcount(const char* (&)[N]) { return (int)N; }

    void initialize()
    {
        auto& io = ImGui::GetIO();

        if (!ui::inter_bold) ui::inter_bold = io.Fonts->AddFontFromMemoryTTF((void*)Fonts::montserrat_bold, Fonts::montserrat_bold_len, 14.0f);
        if (!ui::inter_bold_13) ui::inter_bold_13 = io.Fonts->AddFontFromMemoryTTF((void*)Fonts::montserrat_bold, Fonts::montserrat_bold_len, 13.5f);
        if (!ui::inter_semibold_12_8) ui::inter_semibold_12_8 = io.Fonts->AddFontFromMemoryTTF((void*)Fonts::montserrat_bold, Fonts::montserrat_bold_len, 12.5f);
        if (!ui::inter_semibold_11) ui::inter_semibold_11 = io.Fonts->AddFontFromMemoryTTF((void*)Fonts::montserrat_bold, Fonts::montserrat_bold_len, 11.0f);

        top_textures_grey[0] = LoadTexture(Icons::weapon_top_grey, Icons::weapon_top_len);
        top_textures_blue[0] = LoadTexture(Icons::weapon_top_blue, Icons::weapon_top_len);
        top_textures_grey[1] = LoadTexture(Icons::visuals_top_grey, Icons::visuals_top_len);
        top_textures_blue[1] = LoadTexture(Icons::visuals_top_blue, Icons::visuals_top_len);
        top_textures_grey[2] = LoadTexture(Icons::settings_top_grey, Icons::settings_top_len);
        top_textures_blue[2] = LoadTexture(Icons::settings_top_blue, Icons::settings_top_len);

        side_textures_grey[0] = LoadTexture(Icons::players_side_grey, Icons::players_side_len);
        side_textures_blue[0] = LoadTexture(Icons::players_side_blue, Icons::players_side_len);
        side_textures_grey[1] = LoadTexture(Icons::radar_side_grey, Icons::radar_side_len);
        side_textures_blue[1] = LoadTexture(Icons::radar_side_blue, Icons::radar_side_len);
        side_textures_grey[2] = LoadTexture(Icons::world_side_grey, Icons::world_side_len);
        side_textures_blue[2] = LoadTexture(Icons::world_side_blue, Icons::world_side_len);
        side_textures_grey[3] = LoadTexture(Icons::settings_side_grey, Icons::settings_side_len);
        side_textures_blue[3] = LoadTexture(Icons::settings_side_blue, Icons::settings_side_len);

        ImGuiStyle& style = ImGui::GetStyle();
        style.AntiAliasedLines = true;
        style.AntiAliasedFill = true;
        style.WindowRounding = 8.0f;
        style.FrameRounding = 5.0f;
        style.TabRounding = 5.0f;
        style.ScrollbarRounding = 5.0f;
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.AntiAliasedLinesUseTex = false;
        style.Alpha = 1.0f;
    }

    void render()
    {
        static int top_tab = 1;
        static int side_tab = 0;

        static int* captureTarget = nullptr; // tus atamasi bekleyen cfg degiskeni
        static bool captureArmed = false;    // tiklayan el kalkti mi

        static int prev_top_tab = 1;
        static int prev_side_tab = 0;
        static float content_alpha = 1.0f;
        static float content_dy = 0.0f;

        if (top_tab != prev_top_tab || side_tab != prev_side_tab) {
            content_alpha = 0.0f;
            content_dy = 14.0f;
            prev_top_tab = top_tab;
            prev_side_tab = side_tab;
            if (side_tab > 3) side_tab = 0;
        }
        const float dt = ImGui::GetIO().DeltaTime;
        content_alpha += (1.0f - content_alpha) * ImMin(dt * 14.0f, 1.0f);
        content_dy += (0.0f - content_dy) * ImMin(dt * 16.0f, 1.0f);

        // stream proof main.cpp render loop'ta uygulanir (menu kapaliyken de calisir)

        const float MW = 820.0f, MH = 560.0f;
        ImVec2 disp = ImGui::GetIO().DisplaySize;
        ImVec2 mpos((disp.x - MW) * 0.5f, (disp.y - MH) * 0.5f);

        ImGui::SetNextWindowPos(mpos);
        ImGui::SetNextWindowSize(ImVec2(MW, MH));
        ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();

        ImFont* f_main = ui::inter_semibold_12_8 ? ui::inter_semibold_12_8 : ImGui::GetDefaultFont();
        ImFont* f_title = ui::inter_bold_13 ? ui::inter_bold_13 : ImGui::GetDefaultFont();
        ImFont* f_app = ui::inter_bold ? ui::inter_bold : ImGui::GetDefaultFont();

        const ImVec2 ui_max(wp.x + MW, wp.y + MH);
        const ImColor kShellBg(13, 12, 17);
        const ImColor kPanelLine(24, 23, 30);
        const ImColor kContentBg(9, 8, 12);
        const ImColor kUiOutline(52, 50, 62);

        dl->AddRectFilled(wp, ui_max, kShellBg, 8.0f);

        dl->AddRectFilled(wp, ImVec2(wp.x + MW, wp.y + 80), kShellBg, 8.0f, ImDrawFlags_RoundCornersTop);
        dl->AddLine(ImVec2(wp.x, wp.y + 80), ImVec2(wp.x + MW, wp.y + 80), kPanelLine);

        dl->AddRectFilled(ImVec2(wp.x, wp.y + 80), ImVec2(wp.x + 170, wp.y + MH), kShellBg, 8.0f, ImDrawFlags_RoundCornersBottomLeft);
        dl->AddLine(ImVec2(wp.x + 170, wp.y + 80), ImVec2(wp.x + 170, wp.y + MH), kPanelLine);

        const char* top_labels[] = { "Combat", "Visuals", "Settings" };
        float top_widths[3];
        ImGui::PushFont(f_main);
        for (int i = 0; i < 3; ++i) top_widths[i] = 18 + 8 + ImGui::CalcTextSize(top_labels[i]).x;

        float total_top_w = top_widths[0] + 45 + top_widths[1] + 45 + top_widths[2];
        float top_x = wp.x + (MW / 2) - (total_top_w / 2);

        for (int i = 0; i < 3; ++i) {
            ImGui::PushID("top_nav");
            ImGui::PushID(i);
            ImVec2 pos = ImVec2(top_x, wp.y + 80 - 15 - 18);
            ImGui::SetCursorScreenPos(pos);
            ImGui::InvisibleButton(top_labels[i], ImVec2(top_widths[i], 18));
            if (ImGui::IsItemClicked()) { top_tab = i; side_tab = 0; }

            bool active = (top_tab == i);
            bool hovered = ImGui::IsItemHovered();

            float active_anim = AnimateFloat("active", active ? 1.0f : 0.0f, 14.0f);
            float hover_anim = AnimateFloat("hover", hovered ? 1.0f : 0.0f, 14.0f);

            float text_factor = ImMax(active_anim, hover_anim);
            ImColor color = LerpColor(ImColor(85, 84, 90), ImColor(219, 219, 223), text_factor);

            ImTextureID icon_tex = active ? top_textures_blue[i] : top_textures_grey[i];
            DrawIcon(dl, pos, icon_tex);

            dl->AddText(ImVec2(pos.x + 26, pos.y + 2), color, top_labels[i]);

            top_x += top_widths[i] + 45;
            ImGui::PopID();
            ImGui::PopID();
        }
        ImGui::PopFont();

        ImGui::PushFont(f_app);
        const char* app_title = "Salarius Ext.";
        dl->AddText(ImVec2(wp.x + (MW / 2) - (ImGui::CalcTextSize(app_title).x / 2), wp.y + 15), ImColor(219, 219, 223), app_title);
        ImGui::PopFont();

        const char* side_all[3][4] = {
            { "Aimbot", "Trigger", "RCS", "FOV" },
            { "Players", "Radar", "World", nullptr },
            { "Main", "Config", "Keys", nullptr }
        };
        const int side_count[3] = { 4, 3, 3 };
        if (side_tab >= side_count[top_tab]) side_tab = 0;
        const char** side_labels = side_all[top_tab];

        float side_y = wp.y + 95;
        for (int i = 0; i < side_count[top_tab]; ++i) {
            ImGui::PushID("side_nav");
            ImGui::PushID(i);
            ImVec2 item_pos = ImVec2(wp.x + 12, side_y);
            ImVec2 item_size = ImVec2(146, 38);

            ImGui::SetCursorScreenPos(item_pos);
            ImGui::InvisibleButton(side_labels[i], item_size);
            if (ImGui::IsItemClicked()) side_tab = i;

            bool active = (side_tab == i);
            bool hovered = ImGui::IsItemHovered();

            float active_anim = AnimateFloat("active", active ? 1.0f : 0.0f, 14.0f);
            float hover_anim = AnimateFloat("hover", hovered ? 1.0f : 0.0f, 14.0f);

            if (active_anim > 0.01f) {
                int vtx_start = dl->VtxBuffer.Size;
                dl->AddRectFilled(item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y), ImColor(255, 255, 255, 255), 6.0f);
                int vtx_end = dl->VtxBuffer.Size;
                ShadeVertsGradient(dl, vtx_start, vtx_end, item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y), ImColor(0, 136, 255, (int)(51 * active_anim)), ImColor(2, 10, 20, 0));
            }

            float text_factor = ImMax(active_anim, hover_anim);
            ImColor color = LerpColor(ImColor(85, 84, 90), ImColor(219, 219, 223), text_factor);

            ImTextureID icon_tex = active ? side_textures_blue[i] : side_textures_grey[i];
            DrawIcon(dl, ImVec2(item_pos.x + 15, item_pos.y + 10), icon_tex);

            ImGui::PushFont(f_main);
            dl->AddText(ImVec2(item_pos.x + 45, item_pos.y + 12), color, side_labels[i]);
            ImGui::PopFont();

            side_y += 42;
            ImGui::PopID();
            ImGui::PopID();
        }

        ImFont* f_version = ui::inter_bold_13 ? ui::inter_bold_13 : ImGui::GetDefaultFont();
        ImGui::PushFont(f_version);
        dl->AddText(ImVec2(wp.x + 15, wp.y + MH - 30), ImColor(85, 84, 90), "CS2 v2.0");
        ImGui::PopFont();

        dl->AddRectFilled(ImVec2(wp.x + 170, wp.y + 80), ImVec2(wp.x + MW, wp.y + MH), kContentBg, 8.0f, ImDrawFlags_RoundCornersBottomRight);

        float col1_x = wp.x + 185;
        float col2_x = wp.x + 502.5f;
        float col_w = 302.5f;
        const auto CY = [&](float y) { return wp.y + y + content_dy; };

        auto ApplyAlpha = [](ImColor col, float alpha) -> ImColor {
            ImVec4 v = col.Value;
            return ImColor(v.x, v.y, v.z, v.w * alpha);
            };

        auto DrawCardBg = [&](float x, float y, float h) {
            ImVec2 tl(x, y), br(x + col_w, y + h);
            int vtx_start = dl->VtxBuffer.Size;
            dl->AddRectFilled(tl, br, ApplyAlpha(ImColor(255, 255, 255, 255), content_alpha), 6.0f);
            int vtx_end = dl->VtxBuffer.Size;
            ShadeVertsGradient(dl, vtx_start, vtx_end, tl, ImVec2(tl.x, br.y), ApplyAlpha(ImColor(18, 17, 23), content_alpha), ApplyAlpha(ImColor(13, 12, 17), content_alpha));
            };

        auto PushColorPickerStyle = [&]() {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.05f, 0.047f, 0.067f, 0.98f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.19f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.035f, 0.05f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.07f, 0.065f, 0.09f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.09f, 0.08f, 0.11f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.0f, 136.0f / 255.0f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.0f, 160.0f / 255.0f, 1.0f, 1.0f));
        };

        auto PopColorPickerStyle = [&]() {
            ImGui::PopStyleColor(7);
            ImGui::PopStyleVar(4);
        };

        const ImColor kAccentGradTop(0, 136, 255);
        const ImColor kAccentGradBottom(0, 82, 158);

        auto DrawColorSwatch = [&](ImVec2 pos, const ImVec4& col, float alpha, bool) {
            const ImVec2 br(pos.x + 16.0f, pos.y + 16.0f);
            const ImColor base(col.x, col.y, col.z, col.w);
            const ImColor top = LerpColor(base, ImColor(255, 255, 255), 0.18f);
            const ImColor bottom = LerpColor(base, ImColor(0, 0, 0), 0.22f);

            int vtx_start = dl->VtxBuffer.Size;
            dl->AddRectFilled(pos, br, ApplyAlpha(base, alpha), 3.0f);
            int vtx_end = dl->VtxBuffer.Size;
            ShadeVertsGradient(dl, vtx_start, vtx_end, pos, ImVec2(pos.x, br.y),
                ApplyAlpha(top, alpha), ApplyAlpha(bottom, alpha));
        };

        auto OpenColorPickerPopup = [&](const char* popup_id, const char* title, ImVec4& col) {
            PushColorPickerStyle();
            if (ImGui::BeginPopup(popup_id)) {
                ImGui::PushFont(f_title);
                ImGui::TextUnformatted(title);
                ImGui::PopFont();
                ImGui::Spacing();
                ImGui::ColorPicker4("##picker", (float*)&col,
                    ImGuiColorEditFlags_NoSidePreview |
                    ImGuiColorEditFlags_AlphaBar |
                    ImGuiColorEditFlags_DisplayRGB |
                    ImGuiColorEditFlags_PickerHueWheel |
                    ImGuiColorEditFlags_AlphaPreviewHalf);
                ImGui::EndPopup();
            }
            PopColorPickerStyle();
        };

        auto DrawCheckboxRow = [&](float x, float y, const char* label, bool& v, const char* dim_text = "", ImVec4* inline_color = nullptr) {
            ImVec2 p(x, y);
            ImGui::SetCursorScreenPos(p);
            ImGui::InvisibleButton(label, ImVec2(col_w - 30, 16));
            if (ImGui::IsItemClicked()) v = !v;

            bool hovered = ImGui::IsItemHovered();

            float active_anim = AnimateFloat(label, v ? 1.0f : 0.0f, 14.0f);
            float hover_anim = AnimateFloat(std::string(std::string(label) + "_row_hov").c_str(), hovered ? 1.0f : 0.0f, 14.0f);

            ImColor box_bg = LerpColor(ImColor(12, 11, 16), ImColor(16, 15, 20), hover_anim);
            dl->AddRectFilled(p, ImVec2(p.x + 16, p.y + 16), ApplyAlpha(box_bg, content_alpha), 4.0f);
            if (active_anim > 0.01f) {
                int vtx_start = dl->VtxBuffer.Size;
                dl->AddRectFilled(p, ImVec2(p.x + 16, p.y + 16), ApplyAlpha(ImColor(255, 255, 255, (int)(255 * active_anim)), content_alpha), 4.0f);
                int vtx_end = dl->VtxBuffer.Size;
                ShadeVertsGradient(dl, vtx_start, vtx_end, p, ImVec2(p.x + 16, p.y + 16),
                    ApplyAlpha(kAccentGradTop, content_alpha),
                    ApplyAlpha(kAccentGradBottom, content_alpha));
            }

            float text_factor = ImMax(hover_anim, active_anim);
            ImColor text_color = LerpColor(ImColor(195, 194, 201), ImColor(255, 255, 255), text_factor);

            ImGui::PushFont(f_main);
            float curr_x = p.x + 28;
            dl->AddText(ImVec2(curr_x, p.y + 1), ApplyAlpha(text_color, content_alpha), label);
            curr_x += ImGui::CalcTextSize(label).x + 4;

            if (dim_text[0] != '\0') {
                dl->AddText(ImVec2(curr_x, p.y + 1), ApplyAlpha(ImColor(88, 87, 96), content_alpha), dim_text);
            }
            ImGui::PopFont();

            if (inline_color) {
                ImVec2 swatch_pos(curr_x + 8.0f, p.y);
                ImGui::PushID("inline_color");
                ImGui::SetCursorScreenPos(swatch_pos);
                ImGui::InvisibleButton("##swatch", ImVec2(16.0f, 16.0f));
                if (ImGui::IsItemClicked()) {
                    ImGui::OpenPopup("##color_picker");
                }
                DrawColorSwatch(swatch_pos, *inline_color, content_alpha, false);
                OpenColorPickerPopup("##color_picker", label, *inline_color);
                ImGui::PopID();
            }
            };

        // ayni sayfada birden fazla renkli checkbox varsa ID cakismamasi icin sarmala
        auto ChkC = [&](float x, float y, const char* label, bool& v, ImVec4& col) {
            ImGui::PushID(label);
            DrawCheckboxRow(x, y, label, v, "", &col);
            ImGui::PopID();
        };

        auto DrawColorRow = [&](float x, float y, const char* label, ImVec4& col) {
            ImVec2 p(x, y);
            ImGui::PushID(label);

            ImGui::SetCursorScreenPos(p);
            ImGui::InvisibleButton("##swatch", ImVec2(16.0f, 16.0f));
            const bool swatch_hovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) {
                ImGui::OpenPopup("##color_picker");
            }

            ImGui::SetCursorScreenPos(ImVec2(p.x + 22.0f, p.y));
            ImGui::InvisibleButton("##row", ImVec2(col_w - 52.0f, 16.0f));
            const bool row_hovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked()) {
                ImGui::OpenPopup("##color_picker");
            }

            const bool hovered = swatch_hovered || row_hovered;
            float hover_anim = AnimateFloat("hover", hovered ? 1.0f : 0.0f, 14.0f);

            DrawColorSwatch(p, col, content_alpha, hovered);
            OpenColorPickerPopup("##color_picker", label, col);

            ImColor text_color = LerpColor(ImColor(195, 194, 201), ImColor(255, 255, 255), hover_anim);
            ImGui::PushFont(f_main);
            dl->AddText(ImVec2(p.x + 28.0f, p.y + 1.0f), ApplyAlpha(text_color, content_alpha), label);
            ImGui::PopFont();

            ImGui::PopID();
            };

        auto DrawSliderVal = [&](float x, float y, const char* id, const char* label, const char* val, float& v) {
            ImVec2 p(x, y);
            const float track_w = col_w - 30.0f;
            const float track_h = 14.0f;
            const float rounding = 7.0f;

            ImGui::PushFont(f_main);
            dl->AddText(p, ApplyAlpha(ImColor(195, 194, 201), content_alpha), label);
            ImVec2 vsz = ImGui::CalcTextSize(val);
            dl->AddText(ImVec2(p.x + track_w - vsz.x, p.y), ApplyAlpha(ImColor(0, 150, 255), content_alpha), val);
            ImGui::PopFont();

            ImVec2 t_p(p.x, p.y + 18.5f);
            ImVec2 t_br(t_p.x + track_w, t_p.y + track_h);

            ImGui::PushID(id);
            ImGui::SetCursorScreenPos(t_p);
            ImGui::InvisibleButton("track", ImVec2(track_w, track_h));
            const bool dragging = ImGui::IsItemActive();
            if (dragging) {
                const float t = (ImGui::GetIO().MousePos.x - t_p.x) / track_w;
                v = ImClamp(t, 0.0f, 1.0f);
            }
            ImGui::PopID();

            dl->AddRectFilled(t_p, t_br, ApplyAlpha(ImColor(7, 6, 9), content_alpha), rounding);

            const float fill_w = v * track_w;
            if (fill_w > 0.5f) {
                const ImVec2 fill_br(t_p.x + fill_w, t_p.y + track_h);
                const bool fill_full = fill_w >= track_w - 0.5f;
                const ImDrawFlags fill_corners = fill_full ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft;
                const float fill_rounding = fill_full ? rounding : ImMin(rounding, fill_w * 0.5f);

                dl->PushClipRect(t_p, t_br, true);
                int vtx_start = dl->VtxBuffer.Size;
                dl->AddRectFilled(t_p, fill_br, ApplyAlpha(ImColor(255, 255, 255, 255), content_alpha), fill_rounding, fill_corners);
                int vtx_end = dl->VtxBuffer.Size;
                ShadeVertsGradient(dl, vtx_start, vtx_end, t_p, ImVec2(fill_br.x, t_p.y),
                    ApplyAlpha(kAccentGradTop, content_alpha),
                    ApplyAlpha(kAccentGradBottom, content_alpha));
                dl->PopClipRect();
            }
        };

        // cfg float araligini 0..1 slider'a bagla (config load ile de senkron kalir)
        auto SldMap = [&](float x, float y, const char* id, const char* label, float& cfgv, float mn, float mx, const char* fmt) {
            float t = (cfgv - mn) / (mx - mn);
            t = ImClamp(t, 0.0f, 1.0f);
            char valbuf[32];
            snprintf(valbuf, sizeof(valbuf), fmt, (double)cfgv);
            DrawSliderVal(x, y, id, label, valbuf, t);
            cfgv = mn + t * (mx - mn);
        };
        auto SldMapInt = [&](float x, float y, const char* id, const char* label, int& cfgv, int mn, int mx) {
            float t = (float)(cfgv - mn) / (float)(mx - mn);
            t = ImClamp(t, 0.0f, 1.0f);
            char valbuf[32];
            snprintf(valbuf, sizeof(valbuf), "%d", cfgv);
            DrawSliderVal(x, y, id, label, valbuf, t);
            cfgv = mn + (int)(t * (float)(mx - mn) + 0.5f);
        };

        // tek satir: yazi solda, deger sagda, tiklayinca siradaki secenek
        auto DrawCycleRow = [&](float x, float y, const char* label, const char* const* opts, int count, int& idx) {
            if (idx < 0 || idx >= count) idx = 0;
            ImVec2 p(x, y);
            ImGui::PushID(label);
            ImGui::SetCursorScreenPos(p);
            ImGui::InvisibleButton("##cycle", ImVec2(col_w - 30, 16));
            if (ImGui::IsItemClicked()) idx = (idx + 1) % count;
            bool hovered = ImGui::IsItemHovered();
            float hover_anim = AnimateFloat("hov", hovered ? 1.0f : 0.0f, 14.0f);
            ImColor text_color = LerpColor(ImColor(195, 194, 201), ImColor(255, 255, 255), hover_anim);
            ImGui::PushFont(f_main);
            dl->AddText(ImVec2(p.x, p.y + 1), ApplyAlpha(text_color, content_alpha), label);
            ImVec2 vsz = ImGui::CalcTextSize(opts[idx]);
            dl->AddText(ImVec2(p.x + (col_w - 30) - vsz.x, p.y + 1), ApplyAlpha(ImColor(0, 150, 255), content_alpha), opts[idx]);
            ImGui::PopFont();
            ImGui::PopID();
        };

        auto DrawButtonRow = [&](float x, float y, float w, const char* label) -> bool {
            ImVec2 p(x, y);
            ImGui::PushID(label);
            ImGui::SetCursorScreenPos(p);
            ImGui::InvisibleButton("##btn", ImVec2(w, 28));
            bool clicked = ImGui::IsItemClicked();
            bool hovered = ImGui::IsItemHovered();
            float hover_anim = AnimateFloat("hov", hovered ? 1.0f : 0.0f, 14.0f);
            ImColor bg0 = LerpColor(ImColor(18, 17, 23), ImColor(0, 110, 220), hover_anim);
            ImColor bg1 = LerpColor(ImColor(13, 12, 17), ImColor(0, 70, 140), hover_anim);
            int vtx_start = dl->VtxBuffer.Size;
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 28), ApplyAlpha(ImColor(255, 255, 255, 255), content_alpha), 5.0f);
            int vtx_end = dl->VtxBuffer.Size;
            ShadeVertsGradient(dl, vtx_start, vtx_end, p, ImVec2(p.x + w, p.y),
                ApplyAlpha(bg0, content_alpha), ApplyAlpha(bg1, content_alpha));
            ImGui::PushFont(f_main);
            ImVec2 tsz = ImGui::CalcTextSize(label);
            dl->AddText(ImVec2(p.x + (w - tsz.x) * 0.5f, p.y + 7), ApplyAlpha(ImColor(230, 230, 235), content_alpha), label);
            ImGui::PopFont();
            ImGui::PopID();
            return clicked;
        };

        // tus atama satiri: tikla -> sonraki basilan tus atanir (Esc iptal, Backspace temizler)
        auto DrawKeyRow = [&](float x, float y, const char* label, int& key) {
            ImVec2 p(x, y);
            ImGui::PushID(label);
            ImGui::SetCursorScreenPos(p);
            ImGui::InvisibleButton("##key", ImVec2(col_w - 30, 16));
            if (ImGui::IsItemClicked()) { captureTarget = &key; captureArmed = false; }
            bool hovered = ImGui::IsItemHovered();
            float hover_anim = AnimateFloat("hov", hovered ? 1.0f : 0.0f, 14.0f);
            ImColor text_color = LerpColor(ImColor(195, 194, 201), ImColor(255, 255, 255), hover_anim);
            ImGui::PushFont(f_main);
            dl->AddText(ImVec2(p.x, p.y + 1), ApplyAlpha(text_color, content_alpha), label);
            const char* kn = (captureTarget == &key) ? "..." : KeyName(key);
            ImVec2 vsz = ImGui::CalcTextSize(kn);
            dl->AddText(ImVec2(p.x + (col_w - 30) - vsz.x, p.y + 1), ApplyAlpha(ImColor(0, 150, 255), content_alpha), kn);
            ImGui::PopFont();
            ImGui::PopID();
        };

        auto DrawSectionTitle = [&](float x, float y, const char* title) {
            ImGui::PushFont(f_title);
            dl->AddText(ImVec2(x, y), ApplyAlpha(ImColor(219, 219, 223), content_alpha), title);
            ImGui::PopFont();
        };

        auto DrawInfoLine = [&](float x, float y, const char* text) {
            ImGui::PushFont(f_main);
            dl->AddText(ImVec2(x, y), ApplyAlpha(ImColor(195, 194, 201), content_alpha), text);
            ImGui::PopFont();
        };

        // cfg renkleri <-> ImVec4 koprusu (config load ile senkron kalir)
        auto CfgColor = [&](float* f) -> ImVec4 { return ImVec4(f[0], f[1], f[2], f[3]); };
        auto WriteColor = [&](float* f, const ImVec4& c) { f[0] = c.x; f[1] = c.y; f[2] = c.z; f[3] = c.w; };

        static const char* keyNames[] = { "Left Mouse", "Right Mouse", "Middle Mouse", "Side 1", "Side 2", "Shift", "Ctrl", "Alt" };
        static const int keyValues[] = { 0x01, 0x02, 0x04, 0x05, 0x06, VK_SHIFT, VK_CONTROL, VK_MENU };
        auto KeyIndex = [&](int key) {
            for (int i = 0; i < 8; i++) if (keyValues[i] == key) return i;
            return 0;
        };

        const float kRow = 26.0f;
        const float kSlider = 52.0f;
        const float kCardTop = 95.0f;
        const float kCardHeader = 40.0f;
        const float kCardGap = 12.0f;
        const float kRowStart = 40.5f;

        auto CardHeight = [&](float body) { return kCardHeader + body; };
        auto CardTitleY = [&](float card_top) { return card_top + 15.0f; };
        auto CardRowY = [&](float card_top) { return card_top + kRowStart; };

        // config listesi (1sn'de bir tara)
        static std::vector<std::string> configs;
        static int selected = -1;
        static auto lastScan = std::chrono::steady_clock::now();
        {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastScan).count() > 1000) {
                std::string keep = (selected >= 0 && selected < (int)configs.size()) ? configs[selected] : "";
                configs.clear();
                selected = -1;
                namespace fs = std::filesystem;
                if (fs::exists("configs")) {
                    for (const auto& entry : fs::directory_iterator("configs")) {
                        if (entry.path().extension() == ".json")
                            configs.push_back(entry.path().stem().string());
                    }
                }
                if (!keep.empty()) {
                    for (int i = 0; i < (int)configs.size(); i++)
                        if (configs[i] == keep) { selected = i; break; }
                }
                lastScan = now;
            }
        }

        // ============================== CONTENT ==============================
        if (top_tab == 0) { // Combat
            if (side_tab == 0) { // Aimbot
                float h1 = CardHeight(7.0f * kRow + 2.0f * kSlider);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "Aimbot");
                float y = CardRowY(kCardTop);
                DrawCheckboxRow(col1_x + 15, CY(y), "Enabled", cfg::aimbot); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Legit Mode", cfg::legitmode); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Wall Check", cfg::wallCheck); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Dist FOV", cfg::fovDistScale); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Humanize", cfg::humanize); y += kRow;
                {
                    static const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
                    DrawCycleRow(col1_x + 15, CY(y), "Bone", bones, 4, cfg::aimBone); y += kRow;
                }
                int aki = KeyIndex(cfg::aimKey);
                DrawCycleRow(col1_x + 15, CY(y), "Aim Key", keyNames, 8, aki); cfg::aimKey = keyValues[aki]; y += kRow;
                SldMap(col1_x + 15, CY(y), "fov", "FOV Size", cfg::fovSize, 1.0f, 300.0f, "%.0f px"); y += kSlider;
                SldMap(col1_x + 15, CY(y), "smooth", "Smoothing", cfg::smoothing, 1.0f, 30.0f, "%.1f");

                float h2 = CardHeight(2.0f * kRow + 2.0f * kSlider);
                DrawCardBg(col2_x, CY(kCardTop), h2);
                DrawSectionTitle(col2_x + 15, CY(CardTitleY(kCardTop)), "Target");
                y = CardRowY(kCardTop);
                DrawCheckboxRow(col2_x + 15, CY(y), "Team Check", cfg::teamCheck); y += kRow;
                DrawCheckboxRow(col2_x + 15, CY(y), "Draw FOV", cfg::drawFov); y += kRow;
                SldMapInt(col2_x + 15, CY(y), "react", "React", cfg::aimReactMs, 0, 200); y += kSlider;
                SldMap(col2_x + 15, CY(y), "snap", "Max Snap", cfg::maxSnapDeg, 2.0f, 30.0f, "%.0f");
            }
            else if (side_tab == 1) { // Trigger
                float h1 = CardHeight(2.0f * kRow + 2.0f * kSlider);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "Triggerbot");
                float y = CardRowY(kCardTop);
                DrawCheckboxRow(col1_x + 15, CY(y), "Triggerbot", cfg::triggerbot); y += kRow;
                int tki = KeyIndex(cfg::triggerKey);
                DrawCycleRow(col1_x + 15, CY(y), "Trigger Key", keyNames, 8, tki); cfg::triggerKey = keyValues[tki]; y += kRow;
                SldMapInt(col1_x + 15, CY(y), "tdelay", "Shot Delay", cfg::triggerDelay, 0, 100); y += kSlider;
                SldMapInt(col1_x + 15, CY(y), "tjitter", "Jitter", cfg::triggerJitter, 0, 50);
            }
            else if (side_tab == 2) { // RCS
                float h1 = CardHeight(1.0f * kRow + kSlider);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "Recoil Control");
                float y = CardRowY(kCardTop);
                DrawCheckboxRow(col1_x + 15, CY(y), "Enable RCS", cfg::rcs); y += kRow;
                SldMap(col1_x + 15, CY(y), "rcsstr", "RCS Strength", cfg::rcsStrength, 0.0f, 2.0f, "%.2f");
            }
            else { // FOV
                float h1 = CardHeight(1.0f * kRow + kSlider);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "Field of View");
                float y = CardRowY(kCardTop);
                DrawCheckboxRow(col1_x + 15, CY(y), "Draw FOV", cfg::drawFov); y += kRow;
                SldMap(col1_x + 15, CY(y), "fov2", "FOV Size", cfg::fovSize, 1.0f, 300.0f, "%.0f px");

                ImVec4 cFov = CfgColor(cfg::fovColor);
                float h2 = CardHeight(1.0f * kRow);
                DrawCardBg(col2_x, CY(kCardTop), h2);
                DrawSectionTitle(col2_x + 15, CY(CardTitleY(kCardTop)), "Color");
                y = CardRowY(kCardTop);
                DrawColorRow(col2_x + 15, CY(y), "FOV Color", cFov);
                WriteColor(cfg::fovColor, cFov);
            }
        }
        else if (top_tab == 1) { // Visuals
            if (side_tab == 0) { // Players
                ImVec4 cBox = CfgColor(cfg::boxColor);
                ImVec4 cBone = CfgColor(cfg::boneColor);
                ImVec4 cChams = CfgColor(cfg::chamsColor);
                ImVec4 cChamsTeam = CfgColor(cfg::chamsTeamColor);
                float h1 = CardHeight(7.0f * kRow);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "Player ESP");
                float y = CardRowY(kCardTop);
                ChkC(col1_x + 15, CY(y), "Box ESP", cfg::espOn, cBox); y += kRow;
                ChkC(col1_x + 15, CY(y), "Bone ESP", cfg::bones, cBone); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Health Bar", cfg::healthBar); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Health Text", cfg::healthText); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Weapon ESP", cfg::weaponESP); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Distance", cfg::distanceESP); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Snap Lines", cfg::snapLines);
                WriteColor(cfg::boxColor, cBox);
                WriteColor(cfg::boneColor, cBone);

                float h2 = CardHeight(5.0f * kRow);
                DrawCardBg(col2_x, CY(kCardTop), h2);
                DrawSectionTitle(col2_x + 15, CY(CardTitleY(kCardTop)), "Chams");
                y = CardRowY(kCardTop);
                DrawCheckboxRow(col2_x + 15, CY(y), "Team Check", cfg::teamCheck); y += kRow;
                DrawCheckboxRow(col2_x + 15, CY(y), "Glow Chams", cfg::chams); y += kRow;
                DrawColorRow(col2_x + 15, CY(y), "Enemy", cChams); y += kRow;
                DrawColorRow(col2_x + 15, CY(y), "Team", cChamsTeam); y += kRow;
                {
                    static const char* styles[] = { "Glow", "Tint" };
                    DrawCycleRow(col2_x + 15, CY(y), "Style", styles, 2, cfg::chamsStyle);
                }
                WriteColor(cfg::chamsColor, cChams);
                WriteColor(cfg::chamsTeamColor, cChamsTeam);
            }
            else if (side_tab == 1) { // Radar
                float h1 = CardHeight(3.0f * kRow + 2.0f * kSlider);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "Radar");
                float y = CardRowY(kCardTop);
                DrawCheckboxRow(col1_x + 15, CY(y), "Enable Radar", cfg::radar2D); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Rotate with View", cfg::radarRotate); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Show Distance", cfg::radarShowDistance); y += kRow;
                SldMap(col1_x + 15, CY(y), "rsize", "Size", cfg::radarSize, 100.0f, 400.0f, "%.0f"); y += kSlider;
                SldMap(col1_x + 15, CY(y), "rrange", "Range", cfg::radarRange, 1000.0f, 6000.0f, "%.0f");
            }
            else { // World
                float h1 = CardHeight(8.0f * kRow);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "World");
                float y = CardRowY(kCardTop);
                DrawCheckboxRow(col1_x + 15, CY(y), "Bomb Timer", cfg::bombtimer); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Spectator List", cfg::speclist); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Spectate Warn", cfg::specWarn); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Sound ESP", cfg::soundesp); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Nade ESP", cfg::nadeESP); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Hitmarker", cfg::hitmarker); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Hit Sound", cfg::hitSound); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Kill Sound", cfg::killSound);

                ImVec4 cCross = CfgColor(cfg::crossColor);
                float h2 = CardHeight(3.0f * kRow + 2.0f * kSlider);
                DrawCardBg(col2_x, CY(kCardTop), h2);
                DrawSectionTitle(col2_x + 15, CY(CardTitleY(kCardTop)), "Crosshair");
                y = CardRowY(kCardTop);
                DrawCheckboxRow(col2_x + 15, CY(y), "Crosshair", cfg::crosshair); y += kRow;
                DrawCheckboxRow(col2_x + 15, CY(y), "Dot", cfg::crossDot); y += kRow;
                DrawColorRow(col2_x + 15, CY(y), "Color", cCross); y += kRow;
                SldMapInt(col2_x + 15, CY(y), "xsize", "Size", cfg::crossSize, 2, 15); y += kSlider;
                SldMapInt(col2_x + 15, CY(y), "xgap", "Gap", cfg::crossGap, 0, 10);
                WriteColor(cfg::crossColor, cCross);
            }
        }
        else { // Settings
            if (side_tab == 0) { // Main
                float h1 = CardHeight(7.0f * kRow);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "General");
                float y = CardRowY(kCardTop);
                DrawCheckboxRow(col1_x + 15, CY(y), "Team Check", cfg::teamCheck); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Bunny Hop", cfg::bhop); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "No Flash", cfg::noflash); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Stream Proof", cfg::streamproof); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Proof Pauses Glow", cfg::proofPauseMem); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Perf Stats", cfg::showPerf); y += kRow;
                DrawCheckboxRow(col1_x + 15, CY(y), "Perf Saver", cfg::perfSaver);
            }
            else if (side_tab == 1) { // Config
                float h1 = CardHeight(190.0f);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "Configs");
                float list_y = CardRowY(kCardTop);
                ImGui::PushFont(f_main);
                for (size_t i = 0; i < configs.size(); i++) {
                    ImGui::SetCursorScreenPos(ImVec2(col1_x + 15, CY(list_y + (float)i * 20.0f)));
                    if (ImGui::Selectable(configs[i].c_str(), selected == (int)i, 0, ImVec2(col_w - 30, 18)))
                        selected = (int)i;
                }
                ImGui::PopFont();
                float by = list_y + 150.0f;
                if (DrawButtonRow(col1_x + 15, CY(by), (col_w - 30 - 8) * 0.5f, "Save")) {
                    int index = 1;
                    while (true) {
                        std::string nm = "config_" + std::to_string(index);
                        namespace fs = std::filesystem;
                        if (!fs::exists("configs/" + nm + ".json")) {
                            config_system::Save(nm);
                            configs.push_back(nm);
                            break;
                        }
                        index++;
                    }
                }
                ImGui::SetCursorScreenPos(ImVec2(col1_x + 15 + (col_w - 30 + 8) * 0.5f, CY(by)));
                if (DrawButtonRow(col1_x + 15 + (col_w - 30 + 8) * 0.5f, CY(by), (col_w - 30 - 8) * 0.5f, "Load")) {
                    if (selected >= 0 && selected < (int)configs.size())
                        config_system::Load(configs[selected]);
                }

                float h2 = CardHeight(kRow + 12.0f + 64.0f);
                DrawCardBg(col2_x, CY(kCardTop), h2);
                DrawSectionTitle(col2_x + 15, CY(CardTitleY(kCardTop)), "App");
                if (DrawButtonRow(col2_x + 15, CY(CardRowY(kCardTop)), col_w - 30, "Exit Cheat"))
                    overlay::ShouldQuit = true;
                float iy = CardRowY(kCardTop) + 34.0f;
                DrawInfoLine(col2_x + 15, CY(iy), "Salarius Ext. | CS2 External"); iy += 20.0f;
                DrawInfoLine(col2_x + 15, CY(iy), "Menu: INSERT / Unload: END"); iy += 20.0f;
                DrawInfoLine(col2_x + 15, CY(iy), offset_updater::status.c_str());
            }
            else { // Keys - acma/kapama tuslari (satir tikla, tusa bas)
                float h1 = CardHeight(9.0f * kRow);
                DrawCardBg(col1_x, CY(kCardTop), h1);
                DrawSectionTitle(col1_x + 15, CY(CardTitleY(kCardTop)), "Toggle Keys");
                float y = CardRowY(kCardTop);
                DrawKeyRow(col1_x + 15, CY(y), "Aimbot", cfg::keyAimbot); y += kRow;
                DrawKeyRow(col1_x + 15, CY(y), "Triggerbot", cfg::keyTrigger); y += kRow;
                DrawKeyRow(col1_x + 15, CY(y), "RCS", cfg::keyRCS); y += kRow;
                DrawKeyRow(col1_x + 15, CY(y), "Box ESP", cfg::keyESP); y += kRow;
                DrawKeyRow(col1_x + 15, CY(y), "Bone ESP", cfg::keyBones); y += kRow;
                DrawKeyRow(col1_x + 15, CY(y), "Bunny Hop", cfg::keyBhop); y += kRow;
                DrawKeyRow(col1_x + 15, CY(y), "No Flash", cfg::keyNoFlash); y += kRow;
                DrawKeyRow(col1_x + 15, CY(y), "Radar", cfg::keyRadar); y += kRow;
                DrawKeyRow(col1_x + 15, CY(y), "Chams", cfg::keyChams);

                float h2 = CardHeight(2.0f * kRow);
                DrawCardBg(col2_x, CY(kCardTop), h2);
                DrawSectionTitle(col2_x + 15, CY(CardTitleY(kCardTop)), "Help");
                y = CardRowY(kCardTop);
                DrawInfoLine(col2_x + 15, CY(y), "Satira tikla, tusa bas"); y += kRow;
                DrawInfoLine(col2_x + 15, CY(y), "Esc: iptal, Backsp: sil");
            }
        }

        // tus yakalama (Keys sayfasi)
        if (captureTarget) {
            int pressed = 0;
            bool any = false;
            for (int vk = 0x01; vk <= 0xFE; ++vk) {
                if (GetAsyncKeyState(vk) & 0x8000) { any = true; pressed = vk; break; }
            }
            if (!captureArmed) {
                if (!any) captureArmed = true;
            }
            else if (any) {
                if (pressed == VK_ESCAPE) { /* iptal */ }
                else if (pressed == VK_BACK) *captureTarget = 0;
                else *captureTarget = pressed;
                captureTarget = nullptr;
            }
        }

        dl->AddRect(wp, ui_max, kUiOutline, 8.0f, 0, 1.5f);

        ImGui::End();
    }
}