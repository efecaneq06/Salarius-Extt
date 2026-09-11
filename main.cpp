#include <iostream>
#include "ext/driver_loader.h"
#include "cheat/weapon_icons.h"
#include "cheat/cstrike.h"
#include "cheat/aimbot.h"
#include "cheat/radar.h"
#include "cheat/rcs.h"
#include "cheat/bhop.h"
#include "cheat/noflash.h"
#include "cheat/spectator.h"
#include "cheat/bombtimer.h"
//#include "json.hpp"
#include "cheat/config_system.h"
#include "cheat/soundesp.h"
#include "cheat/chams.h"
#include "cheat/nade_esp.h"
#include "cheat/hitmarker.h"
#include "cheat/crosshair.h"
#include "cheat/offset_updater.h"
#include "cheat/menu/menu_impl.h"
#include <filesystem>
#include <windows.h>
namespace fs = std::filesystem;

#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
//using json = nlohmann::json;


void mainThread() {
    while (true) {
        if (cfg::triggerbot) aimbot::RunTriggerbot();
        rcs::Run();
        if (cfg::noflash) NoFlash::Run();
        chams::Run();
        hitmarker::Poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    }
}

void aimbotThread() {
    timeBeginPeriod(1);
    while (true) {
        bool aiming = cfg::aimbot && (GetAsyncKeyState(cfg::aimKey) & 0x8000);

        if (aiming) {
            aimbot::FindClosestTarget(cfg::fovSize);
            aimbot::AimAtTarget(cfg::smoothing);
        }
        else {
            aimbot::currentTarget.hasTarget = false;
        }

        if (cfg::legitmode) {
            Sleep(1);
        }
        else {
            Sleep(0);
        }
    }

    timeEndPeriod(1);

}

void bhopThread() {
    while (true) {
        bhop::Run();
        // kapaliyken %100 CPU spin yapip entity/render threadlerini aclockiyordu
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void hotkeyThread() {
    struct Bind { int* key; bool* var; bool was = false; };
    Bind binds[] = {
        { &cfg::keyAimbot, &cfg::aimbot },
        { &cfg::keyTrigger, &cfg::triggerbot },
        { &cfg::keyRCS, &cfg::rcs },
        { &cfg::keyESP, &cfg::espOn },
        { &cfg::keyBones, &cfg::bones },
        { &cfg::keyBhop, &cfg::bhop },
        { &cfg::keyNoFlash, &cfg::noflash },
        { &cfg::keyRadar, &cfg::radar2D },
        { &cfg::keyChams, &cfg::chams },
    };
    while (true) {
        for (auto& b : binds) {
            int k = *b.key;
            if (!k) { b.was = false; continue; }
            bool down = (GetAsyncKeyState(k) & 0x8000) != 0;
            if (down && !b.was) *b.var = !*b.var;
            b.was = down;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void setStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(10, 6);
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;

    style.WindowRounding = 4.0f;
    style.ChildRounding = 3.0f;
    style.FrameRounding = 2.0f;

    ImVec4* c = style.Colors;

    ImVec4 bgMain = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
    ImVec4 bgChild = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);

    ImVec4 textWhite = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);

    ImVec4 accent = ImVec4(0.10f, 0.50f, 0.90f, 1.00f);
    ImVec4 accentH = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);

    c[ImGuiCol_Text] = textWhite;
    c[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    c[ImGuiCol_WindowBg] = bgMain;
    c[ImGuiCol_ChildBg] = bgChild;
    c[ImGuiCol_Border] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);

    c[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

    c[ImGuiCol_Tab] = bgMain;
    c[ImGuiCol_TabHovered] = accent;
    c[ImGuiCol_TabActive] = accent;

    c[ImGuiCol_CheckMark] = accent;
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = accentH;

    c[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_ButtonHovered] = accent;
    c[ImGuiCol_ButtonActive] = accentH;

    c[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_HeaderHovered] = accent;
    c[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    bool driyver = driver_loader::EnsureDriverLoaded();

    offset_updater::RunOnce(); // internet varsa dw*'lari tazele, yoksa compiled defaults

    initMemory();

    if (!memory->IsConnected()) {
        MessageBoxA(nullptr, "\"cs2.exe\" bulunamadi. Oyunun acik oldugundan emin olun.", "Hata", MB_OK | MB_ICONERROR);
        std::cin.get();
        return 1;
    }

    if (client == 0) {
        MessageBoxA(nullptr, "\"client.dll\" bulunamadi. Oyunun acik oldugundan emin olun.", "Hata", MB_OK | MB_ICONERROR);
        return 1;
    }

    //std::cout << "[+] client.dll -> 0x" << std::hex << client << std::dec << "\n";
    // MessageBoxA(nullptr, "Hile aktifleþtirildi!", "Info", MB_OK | MB_ICONINFORMATION);

    localplayer::init();
    entities::init();
    spectator::init();
    bombtimer::init();
    nade_esp::init();
    std::thread(hotkeyThread).detach();
    std::thread(aimbotThread).detach();
    std::thread(mainThread).detach();
    std::thread(bhopThread).detach();

    overlay::SetupWindow();

    if (!(overlay::CreateDeviceD3D(overlay::Window)))
        return 1;

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig font_config;

    font_config.RasterizerMultiply = 1.3f;
    font_config.OversampleH = 3;
    font_config.OversampleV = 3;
    font_config.GlyphOffset.y = -1;

    char winDir[MAX_PATH];
    GetWindowsDirectoryA(winDir, MAX_PATH);
    std::string fontPath = std::string(winDir) + "\\Fonts\\segoeui.ttf";

    if (!io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 17.0f, &font_config)) {
        io.Fonts->AddFontDefault();
    }

    setStyle();

    weapon_icons::LoadAll(overlay::g_pd3dDevice);
    ui::initialize();

    for (int i = 0; i < 5; i++) {
        SetWindowPos(overlay::Window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        BringWindowToTop(overlay::Window);
        Sleep(100);
    }

    const char* boneNames[] = { "Head", "Neck", "Upper Arm L", "Upper Arm R", "Lower Arm L", "Lower Arm R", "Hand L", "Hand R", "Pelvis", "Upper Leg L", "Lower Leg L", "Upper Leg R", "Lower Leg R" };

    bool menuVisible = true;
    bool insertWasDown = false;

    while (!overlay::ShouldQuit)
    {
        overlay::Render();

        if (GetAsyncKeyState(VK_END) & 0x8000) {
            overlay::ShouldQuit = true;
        }

        bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        if (insertDown && !insertWasDown)
            menuVisible = !menuVisible;
        insertWasDown = insertDown;

        cstrike::matrix = memory->read<view_matrix_t>(client + Offsets::dwViewMatrix);

        // stream proof: menu acik/kapali fark etmez, her frame garanti uygula.
        // overlay gizlenir (ESP/menu/radar/hitmarker/crosshair yayina cikmaz),
        // glow/ses modulleri kendi icinde duraklar (oyun karesine/sesine islerler).
        {
            static bool lastProof = false;
            static bool firstProof = true;
            if (firstProof || cfg::streamproof != lastProof) {
                SetWindowDisplayAffinity(overlay::Window,
                    cfg::streamproof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
                SetWindowPos(overlay::Window, HWND_TOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
                lastProof = cfg::streamproof;
                firstProof = false;
            }
        }
        // SpecList::Draw();

        if (menuVisible) {
            ui::render();
            if (false) { // legacy menu (yedek) - yeni menu ustte ciziliyor
            static int currentTab = 0;
            const float menuW = 540.0f, menuH = 420.0f;
            const float sideW = 130.0f;
            const float pad = 14.0f;

            ImVec4 accent(0.0f, 0.47f, 1.0f, 1.0f);
            ImVec4 accentBright(0.2f, 0.6f, 1.0f, 1.0f);
            ImVec4 whiteText(1.0f, 1.0f, 1.0f, 1.0f);

            ImGui::SetNextWindowSize(ImVec2(menuW, menuH), ImGuiCond_FirstUseEver);

            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.03f, 1.0f));
            ImGui::Begin("##MainMenu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

            ImVec2 wp = ImGui::GetWindowPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();

            for (int i = 0; i < 3; i++) {
                ImU32 barCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.47f, 1.0f, 0.8f - i * 0.25f));
                dl->AddRectFilled(ImVec2(wp.x, wp.y + i), ImVec2(wp.x + menuW, wp.y + i + 1), barCol);
            }

            ImGui::SetCursorPos(ImVec2(pad, 10));
            ImGui::PushStyleColor(ImGuiCol_Text, accentBright);
            ImGui::Text("Salarius Ext.");
            ImGui::PopStyleColor();

            ImGui::SameLine(menuW - 100);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            ImGui::Text("v1.4");
            ImGui::PopStyleColor();

            {
                float btnSize = 20.0f;
                ImVec2 btnMin(wp.x + menuW - btnSize - 8, wp.y + 6);
                ImVec2 btnMax(btnMin.x + btnSize, btnMin.y + btnSize);
                ImVec2 mouse = ImGui::GetIO().MousePos;
                bool hovered = (mouse.x >= btnMin.x && mouse.x <= btnMax.x && mouse.y >= btnMin.y && mouse.y <= btnMax.y);
                bool clicked = hovered && ImGui::GetIO().MouseClicked[0];

                ImU32 btnBg = hovered ? IM_COL32(255, 40, 40, 255) : IM_COL32(180, 20, 20, 180);
                dl->AddRectFilled(btnMin, btnMax, btnBg, 4.0f);

                float p = 5.0f;
                dl->AddLine(ImVec2(btnMin.x + p, btnMin.y + p), ImVec2(btnMax.x - p, btnMax.y - p), IM_COL32(255, 255, 255, 255), 2.0f);
                dl->AddLine(ImVec2(btnMax.x - p, btnMin.y + p), ImVec2(btnMin.x + p, btnMax.y - p), IM_COL32(255, 255, 255, 255), 2.0f);

                if (clicked) overlay::ShouldQuit = true;
            }

            ImGui::SetCursorPos(ImVec2(0, 35));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.04f, 0.06f, 1.0f));
            ImGui::BeginChild("##sidebar", ImVec2(sideW, menuH - 35), false);
            ImGui::Dummy(ImVec2(0, 6));

            const char* tabs[] = { "Visuals", "Combat", "Radar", "Misc", "Config" };
            for (int i = 0; i < 5; i++) {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 8));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

                if (currentTab == i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.47f, 1.0f, 0.25f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.47f, 1.0f, 0.35f));
                    ImGui::PushStyleColor(ImGuiCol_Text, whiteText);

                    ImVec2 cp = ImGui::GetCursorScreenPos();
                    dl->AddRectFilled(ImVec2(cp.x, cp.y), ImVec2(cp.x + 3, cp.y + 32), ImGui::ColorConvertFloat4ToU32(accent), 2.0f);
                }
                else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.08f, 0.12f, 0.18f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                }

                if (ImGui::Button(tabs[i], ImVec2(sideW - 1, 32)))
                    currentTab = i;

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(2);
            }

            ImGui::SetCursorPosY(menuH - 35 - 65);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::SetCursorPosX(12);
            ImGui::Text("Menu key: INSERT");
            ImGui::PopStyleColor();

            ImGui::SetCursorPosX(8);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 0.9f));       // Canlý Kýrmýzý
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, whiteText);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            if (ImGui::Button("Exit Cheat", ImVec2(sideW - 16, 28))) {
                overlay::ShouldQuit = true;
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);

            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::SetCursorPos(ImVec2(sideW, 35));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, pad));
            ImGui::BeginChild("##content", ImVec2(menuW - sideW, menuH - 35), false);
            ImGui::PushStyleColor(ImGuiCol_Text, whiteText);

            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 20.0f);

            if (currentTab == 0) { // VISUALS
                ImGui::Indent(15.0f);
                ImGui::BeginGroup();
                ImGui::Spacing();
                ImGui::TextColored(accentBright, "ESP Settings");
                ImGui::Separator(); ImGui::Spacing();

                ImGui::Checkbox("Box ESP", &cfg::espOn);
                ImGui::Checkbox("Bone ESP", &cfg::bones);
                ImGui::Checkbox("Sound ESP", &cfg::soundesp);
                ImGui::Checkbox("Health Bar", &cfg::healthBar);
                ImGui::Checkbox("Health Text", &cfg::healthText);
                ImGui::Checkbox("Weapon ESP", &cfg::weaponESP);
                ImGui::Checkbox("Distance", &cfg::distanceESP);
                ImGui::Checkbox("Snap Lines", &cfg::snapLines);
                ImGui::EndGroup();

                ImGui::SameLine(220.0f);

                ImGui::BeginGroup();

                ImGui::Spacing();
                ImGui::TextColored(accentBright, "Colors");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::ColorEdit4("Box Color", cfg::boxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                ImGui::ColorEdit4("Bone Color", cfg::boneColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                ImGui::ColorEdit4("FOV Color", cfg::fovColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(accentBright, "HUD");
                ImGui::Spacing();

                ImGui::Checkbox("Bomb Timer", &cfg::bombtimer);
                ImGui::Checkbox("Spectator List", &cfg::speclist);
                static bool lastStreamProofState = false;

                if (ImGui::Checkbox("Stream Proof", &cfg::streamproof))
                {

                    if (cfg::streamproof != lastStreamProofState)
                    {

                        SetWindowDisplayAffinity(
                            overlay::Window,
                            cfg::streamproof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE
                        );


                        SetWindowPos(overlay::Window, HWND_TOPMOST, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);


                        lastStreamProofState = cfg::streamproof;
                    }
                }

                ImGui::EndGroup();
            }
            else if (currentTab == 1) {
                const char* keyNames[] = { "Left Mouse", "Right Mouse", "Middle Mouse", "Side 1", "Side 2", "Shift", "Ctrl", "Alt" };
                int keyValues[] = { 0x01, 0x02, 0x04, 0x05, 0x06, VK_SHIFT, VK_CONTROL, VK_MENU };

                int aimKeyIdx = 0;
                for (int i = 0; i < 8; i++) { if (cfg::aimKey == keyValues[i]) aimKeyIdx = i; }
                int trigKeyIdx = 0;
                for (int i = 0; i < 8; i++) { if (cfg::triggerKey == keyValues[i]) trigKeyIdx = i; }


                ImGui::Indent(15.0f);
                ImGui::Spacing();


                float availableWidth = ImGui::GetContentRegionAvail().x - 10.0f;
                float childWidth = availableWidth * 0.5f;

                ImGui::BeginChild("AimbotSection", ImVec2(childWidth, 0), false);
                ImGui::TextColored(accentBright, "Aimbot Configuration");
                ImGui::Spacing();

                ImGui::Checkbox("Legit Mode (Low CPU)", &cfg::legitmode);
                ImGui::Checkbox("Wall Check", &cfg::wallCheck);

                ImGui::Spacing();
                ImGui::Text("Activation Key:");
                ImGui::SetNextItemWidth(childWidth * 0.85f);
                if (ImGui::BeginCombo("##AimKey", keyNames[aimKeyIdx])) {
                    for (int i = 0; i < 8; i++) {
                        if (ImGui::Selectable(keyNames[i], cfg::aimKey == keyValues[i]))
                            cfg::aimKey = keyValues[i];
                    }
                    ImGui::EndCombo();
                }

                ImGui::Spacing();
                ImGui::Text("Field of View (FOV):");
                ImGui::SetNextItemWidth(childWidth * 0.85f);
                ImGui::SliderFloat("##FOV", &cfg::fovSize, 1.f, 300.f, "%.0f px");

                ImGui::Spacing();
                ImGui::Text("Smoothing:");
                ImGui::SetNextItemWidth(childWidth * 0.85f);
                ImGui::SliderFloat("##Smooth", &cfg::smoothing, 1.f, 30.f, "%.1f");
                ImGui::EndChild();

                ImGui::SameLine(0, 10.0f);

                ImGui::BeginChild("AssistSection", ImVec2(childWidth, 0), false);
                ImGui::TextColored(accentBright, "Trigger & Recoil");
                ImGui::Spacing();

                ImGui::Checkbox("Triggerbot", &cfg::triggerbot);

                ImGui::Spacing();
                ImGui::Text("Trigger Key:");
                ImGui::SetNextItemWidth(childWidth * 0.85f);
                if (ImGui::BeginCombo("##TrigKey", keyNames[trigKeyIdx])) {
                    for (int i = 0; i < 8; i++) {
                        if (ImGui::Selectable(keyNames[i], cfg::triggerKey == keyValues[i]))
                            cfg::triggerKey = keyValues[i];
                    }
                    ImGui::EndCombo();
                }

                ImGui::Spacing();
                ImGui::Text("Shot Delay:");
                ImGui::SetNextItemWidth(childWidth * 0.85f);
                ImGui::SliderInt("##Delay", &cfg::triggerDelay, 0, 100, "%d ms");

                ImGui::Spacing();

                ImGui::Checkbox("Enable RCS", &cfg::rcs);
                ImGui::Spacing();
                ImGui::Text("RCS Strength:");


                ImGui::SetNextItemWidth(childWidth * 0.85f);
                ImGui::SliderFloat("##RCSStr", &cfg::rcsStrength, 0.0f, 2.0f, "%.2f");

                ImGui::EndChild();

                ImGui::Unindent(15.0f);
            }
            else if (currentTab == 2) { // RADAR
                ImGui::Indent(15.0f);
                ImGui::Spacing();
                ImGui::TextColored(accentBright, "Radar Settings");
                ImGui::Separator(); ImGui::Spacing();

                ImGui::Checkbox("Enable Radar", &cfg::radar2D);
                ImGui::Checkbox("Rotate with View", &cfg::radarRotate);
                ImGui::Checkbox("Show Distance", &cfg::radarShowDistance);

                ImGui::Spacing();
                ImGui::SetNextItemWidth(180);
                ImGui::SliderFloat("Size", &cfg::radarSize, 100.f, 400.f, "%.0f");
                ImGui::Unindent(15.0f);
            }
            else if (currentTab == 3) { // MISC
                ImGui::Indent(15.0f);
                ImGui::Spacing();
                ImGui::TextColored(accentBright, "Miscellaneous");
                ImGui::Separator(); ImGui::Spacing();

                ImGui::Checkbox("Team Check", &cfg::teamCheck);

                ImGui::Spacing();

                ImGui::Checkbox("Bunny Hop", &cfg::bhop);

                ImGui::Spacing();

                ImGui::Checkbox("No Flash", &cfg::noflash);

                ImGui::Spacing();

                ImGui::Unindent(15.0f);
            }
            else if (currentTab == 4) {
                ImGui::Indent(15.0f);
                ImGui::Spacing();

                ImGui::TextColored(accentBright, "Config System");
                ImGui::Separator();
                ImGui::Spacing();

                static std::vector<std::string> configs;
                static int selected = -1;

                static auto lastScan = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();

                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastScan).count() > 1000)
                {
                    configs.clear();

                    if (fs::exists("configs"))
                    {
                        for (const auto& entry : fs::directory_iterator("configs"))
                        {
                            if (entry.path().extension() == ".json")
                                configs.push_back(entry.path().stem().string());
                        }
                    }

                    lastScan = now;
                }

                ImGui::Text("Configs:");
                ImGui::BeginChild("cfg_list", ImVec2(200, 120), true);

                for (int i = 0; i < (int)configs.size(); i++)
                {
                    if (ImGui::Selectable(configs[i].c_str(), selected == i))
                        selected = i;
                }

                ImGui::EndChild();

                ImGui::Spacing();

                if (ImGui::Button("Save Config", ImVec2(150, 30)))
                {
                    int index = 1;

                    while (true)
                    {
                        std::string name = "config_" + std::to_string(index);
                        std::string path = "configs/" + name + ".json";

                        if (!fs::exists(path))
                        {
                            config_system::Save(name.c_str());
                            configs.push_back(name);
                            break;
                        }
                        index++;
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Load Config", ImVec2(150, 30)))
                {
                    if (selected != -1 && selected < configs.size())
                    {
                        config_system::Load(configs[selected].c_str());
                    }
                }

                ImGui::Spacing();
                ImGui::Unindent(15.0f);
            }
            ImGui::PopStyleVar();

            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::End();
            ImGui::PopStyleColor();
        }
        } // if (false) legacy

        ImColor fovCol(cfg::fovColor[0], cfg::fovColor[1], cfg::fovColor[2], cfg::fovColor[3]);
        ImColor boxCol(cfg::boxColor[0], cfg::boxColor[1], cfg::boxColor[2], cfg::boxColor[3]);
        ImColor boneCol(cfg::boneColor[0], cfg::boneColor[1], cfg::boneColor[2], cfg::boneColor[3]);

        if (cfg::drawFov) {
            float fovPixelRadius = cfg::fovSize;
            ImGui::GetBackgroundDrawList()->AddCircle(SDK::screenCenter, fovPixelRadius, fovCol, 64, 1.f);
        }

        Radar2D::DrawRadar();
        spectator::Draw();
        bombtimer::Draw();

        sound_esp::Draw();

        // frame basina 1 kopya + 1 local pos RPM (once her actor icin ayri RPM vardi)
        std::vector<entities::PLAYER> frame = entities::snapshot();
        Vec3 localPosFrame = localplayer::pawn ? memory->read<Vec3>(localplayer::pawn + Offsets::m_vOldOrigin) : Vec3{};
        const double nowSec = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();

        for (entities::PLAYER& actor : frame) {
            // kayma duzeltmesi: veri okundugundan beri gecen surede hiz*dt kadar ilerlet
            // (harici ESP origin'i islenmemis okur, hareketli hedef geride kalirdi)
            double dt = nowSec - actor.ts;
            if (dt < 0.0) dt = 0.0;
            if (dt > 0.1) dt = 0.1; // dormant/olu veri ucup gitmesin
            if (dt > 0.0 && !actor.velocity.IsZero()) {
                Vec3 d = actor.velocity * (float)dt;
                actor.PlayerPos = actor.PlayerPos + d;
                for (Vec3& b : actor.bones) b = b + d;
            }
            cstrike::drawPlayer(actor, boxCol, boneCol, localPosFrame);
        }

        nade_esp::Draw(localPosFrame);
        hitmarker::Draw();
        crosshair::Draw();
        spectator::DrawWarn();

        if (cfg::showPerf) {
            static double dispHz = 0.0;
            static auto lastCalc = std::chrono::steady_clock::now();
            static uint64_t lastC0 = 0, lastC1 = 0;
            auto nowP = std::chrono::steady_clock::now();
            double el = std::chrono::duration<double>(nowP - lastCalc).count();
            if (el >= 0.5) {
                uint64_t c0 = entities::passCount[0].load(std::memory_order_relaxed);
                uint64_t c1 = entities::passCount[1].load(std::memory_order_relaxed);
                dispHz = (double)(c0 - lastC0 + c1 - lastC1) / el;
                lastC0 = c0; lastC1 = c1; lastCalc = nowP;
            }
            char pbuf[64];
            snprintf(pbuf, sizeof(pbuf), "ESP %.0fHz | %.0f FPS", dispHz, (double)ImGui::GetIO().Framerate);
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(12, 12), IM_COL32(0, 255, 150, 255), pbuf);
        }

        overlay::EndRender();
    }

    overlay::CloseOverlay();
}
