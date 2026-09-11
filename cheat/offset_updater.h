#pragma once
// Auto offset updater: acilista a2x cs2-dumper'dan guncel dw*'lari ceker,
// Offsets:: globals'i uzerine yazar. Internet yoksa derlenmis default'lar kalir.
// Netvar'lar (m_iHealth vb.) neredeyse hic degismedigi icin sadece dw* + jump guncellenir.
// SADECE main.cpp TU'sunda kullanilir, initMemory()'den ONCE cagrilir.
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include <string>
#include <vector>
#include "offsets.h"
#include "json.hpp"

using json = nlohmann::json;

namespace offset_updater
{
    inline std::string status = "compiled defaults (2026-09-10)";
    inline bool ok = false;

    inline bool FetchHttps(const wchar_t* host, const wchar_t* path, std::string& out, DWORD timeoutMs = 6000) {
        out.clear();
        HINTERNET hSession = WinHttpOpen(L"SalariusExt/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return false;
        WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

        HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

        HINTERNET hReq = WinHttpOpenRequest(hConnect, L"GET", path,
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!hReq) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

        bool res = false;
        if (WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(hReq, NULL)) {
            DWORD code = 0, sz = sizeof(code);
            WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                NULL, &code, &sz, NULL);
            if (code == 200) {
                std::vector<char> buf;
                DWORD avail = 0, rd = 0;
                do {
                    avail = 0;
                    if (!WinHttpQueryDataAvailable(hReq, &avail)) break;
                    if (!avail) break;
                    size_t base = buf.size();
                    buf.resize(base + avail);
                    if (!WinHttpReadData(hReq, buf.data() + base, avail, &rd)) break;
                    buf.resize(base + rd);
                    if (rd == 0) break;
                } while (avail > 0);
                if (!buf.empty()) { out.assign(buf.data(), buf.size()); res = true; }
            }
        }
        WinHttpCloseHandle(hReq);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return res;
    }

    inline void ApplyOffsetsJson(const std::string& body, int& applied) {
        json j = json::parse(body, nullptr, false);
        if (j.is_discarded() || !j.is_object()) return;
        auto get = [&](const char* mod, const char* key, uintptr_t& dst) {
            if (j.contains(mod) && j[mod].contains(key) && j[mod][key].is_number_unsigned()) {
                dst = (uintptr_t)j[mod][key].get<uint64_t>();
                applied++;
            }
        };
        get("client.dll", "dwEntityList", Offsets::dwEntityList);
        get("client.dll", "dwLocalPlayerPawn", Offsets::dwLocalPlayerPawn);
        get("client.dll", "dwLocalPlayerController", Offsets::dwLocalPlayerController);
        get("client.dll", "dwViewMatrix", Offsets::dwViewMatrix);
        get("client.dll", "dwCSGOInput", Offsets::dwCSGOInput);
        get("client.dll", "dwViewAngles", Offsets::dwViewAngles);
        get("client.dll", "dwSensitivity", Offsets::dwSensitivity);
        get("client.dll", "dwGlobalVars", Offsets::dwGlobalVars);
        get("client.dll", "dwPlantedC4", Offsets::dwPlantedC4);
        get("client.dll", "dwGlowManager", Offsets::dwGlowManager);
        get("client.dll", "dwWeaponC4", Offsets::dwWeaponC4);
    }

    inline void ApplyButtonsJson(const std::string& body, int& applied) {
        json j = json::parse(body, nullptr, false);
        if (j.is_discarded() || !j.is_object()) return;
        if (j.contains("client.dll") && j["client.dll"].contains("jump") &&
            j["client.dll"]["jump"].is_number_unsigned()) {
            Offsets::jump = (uintptr_t)j["client.dll"]["jump"].get<uint64_t>();
            applied++;
        }
    }

    // initMemory'den once cagir. Blocking ama kisa timeout'lu.
    inline bool RunOnce() {
        int applied = 0;
        std::string body;
        if (FetchHttps(L"raw.githubusercontent.com",
                L"/a2x/cs2-dumper/main/output/offsets.json", body)) {
            try { ApplyOffsetsJson(body, applied); } catch (...) {}
        }
        if (FetchHttps(L"raw.githubusercontent.com",
                L"/a2x/cs2-dumper/main/output/buttons.json", body)) {
            try { ApplyButtonsJson(body, applied); } catch (...) {}
        }
        if (applied >= 10) {
            ok = true;
            status = "auto-updated (" + std::to_string(applied) + " offsets, a2x latest)";
        } else if (applied > 0) {
            ok = true;
            status = "partial update (" + std::to_string(applied) + "), check dumper";
        } else {
            ok = false;
            status = "offline - compiled defaults (2026-09-10)";
        }
        return ok;
    }
}
