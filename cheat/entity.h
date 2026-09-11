#pragma once
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cstring>
#include <unordered_map>
#include <iostream>
#include "../ext/memory.h"
#include "offsets.h"
#include "math.h"
#include "cfg.h"

inline Memory* memory = nullptr;
inline uintptr_t client = 0;
inline uintptr_t engine2 = 0;

inline void initMemory() {
    memory = new Memory{ L"cs2.exe" };
    client = memory->GetModuleBase(L"client.dll");
    engine2 = memory->GetModuleBase(L"engine2.dll");
}

namespace localplayer {
    inline uintptr_t pawn = 0;
    inline uint32_t pawnHandle = 0; 
    inline int teamid = 0;
    inline int health = 0;
    inline int localIndex = -1;
    inline Vec2 viewAngle = { 0, 0 };
    inline Vec2 aimPunchAngle = { 0, 0 };
    inline DWORD shotsFired = 0;
    inline float sensitivity = 1.0f;
    inline bool isScoped = false;
    inline float flashDuration = 0.f;
    inline Vec3 cameraPos = { 0, 0, 0 };

    inline uintptr_t G_Pawn() { return pawn; }
    inline int G_Team() { return teamid; }
    inline int G_Health() { return health; }
    inline Vec3 G_Pos() { return memory->read<Vec3>(pawn + Offsets::m_vOldOrigin); }

    inline void loop() {
        // sleep YOK ama driver'i bogmamak icin guncelleme 1ms'de bir (1000Hz - fazlasiyla yeterli)
        auto last = std::chrono::steady_clock::now() - std::chrono::seconds(1);
        while (true) {
            uintptr_t tempPawn = memory->read<uintptr_t>(client + Offsets::dwLocalPlayerPawn);
            if (!tempPawn) {
                pawn = 0;
                std::this_thread::yield();
                continue;
            }
            auto now = std::chrono::steady_clock::now();
            if (now - last < std::chrono::milliseconds(1)) {
                std::this_thread::yield();
                continue;
            }
            last = now;

            uintptr_t localController = memory->read<uintptr_t>(client + Offsets::dwLocalPlayerController);
            if (localController) {
                pawnHandle = memory->read<uint32_t>(localController + Offsets::m_hPlayerPawn);
            }

            pawn = tempPawn;
            localIndex = memory->read<int>(pawn + Offsets::m_iIDEntIndex);
            teamid = memory->read<int>(pawn + Offsets::m_iTeamNum);
            health = memory->read<int>(pawn + Offsets::m_iHealth);
            viewAngle = memory->read<Vec2>(pawn + Offsets::m_angEyeAngles);
            //aimPunchAngle = memory->read<Vec2>(pawn + Offsets::m_aimPunchAngle);
            shotsFired = memory->read<DWORD>(pawn + Offsets::m_iShotsFired);
            isScoped = memory->read<bool>(pawn + Offsets::m_bIsScoped);
            flashDuration = memory->read<float>(pawn + Offsets::m_flFlashDuration);

            Vec3 origin = memory->read<Vec3>(pawn + Offsets::m_vOldOrigin);
            Vec3 viewOffset = memory->read<Vec3>(pawn + Offsets::m_vecViewOffset);
            cameraPos = origin + viewOffset;

            uintptr_t sensitivityPtr = memory->read<uintptr_t>(client + Offsets::dwSensitivity);
            if (sensitivityPtr)
                sensitivity = memory->read<float>(sensitivityPtr + Offsets::dwSensitivity_sensitivity);
        }
    }

    inline void init() {
        std::thread([]() { loop(); }).detach();
    }
}


uintptr_t GetEntityFromHandle(uintptr_t handle)
{
    uintptr_t entityList = memory->read<uintptr_t>(client + Offsets::dwEntityList);
    if (entityList == 0 || handle == 0xFFFFFFFF) return 0;

    uintptr_t listEntry = memory->read<uintptr_t>(entityList + 0x10 + 8 * ((handle & 0x7FFF) >> 9));
    if (listEntry == 0) return 0;

    uintptr_t ent = memory->read<uintptr_t>(listEntry + 0x70 * (handle & 0x1FF));
    if (ent == 0) return 0;

    return ent;
}


namespace entities {
    enum BoneIndices : int {
        PELVIS = 1, SPINE1 = 3, SPINE2 = 4, NECK = 6, HEAD = 7,
        SHOULDER_L = 9, ELBOW_L = 10, HAND_L = 11,
        SHOULDER_R = 13, ELBOW_R = 14, HAND_R = 15,
        HIP_L = 17, KNEE_L = 18, FOOT_HEEL_L = 19,
        HIP_R = 20, KNEE_R = 21, FOOT_HEEL_R = 22,
        CHEST = 23
    };

    struct PLAYER {
        int health = 0, team = 0;
        uintptr_t actorBase = 0;
        uintptr_t controllerBase = 0;
        std::string name = "Unknown";
        std::vector<Vec3> bones;
        bool spotted = false;
        Vec3 PlayerPos;
        Vec3 velocity;   // m_vecAbsVelocity - kayma duzeltmesi icin
        double ts = 0.0; // verinin okundugu an (steady_clock, saniye)
        short weaponame = 0;
    };

    constexpr int WORKERS = 2; // entity tarama thread sayisi (cekirdek somurmekte ozgur)
    inline std::vector<PLAYER> partials[WORKERS]; // worker basina son liste
    inline std::mutex listMutex;
    inline std::atomic<uint64_t> passCount[WORKERS] = {}; // perf olcumu: worker basina tur sayisi

    inline std::vector<PLAYER> snapshot() {
        std::lock_guard<std::mutex> lk(listMutex);
        std::vector<PLAYER> out;
        out.reserve(16);
        for (int w = 0; w < WORKERS; ++w)
            out.insert(out.end(), partials[w].begin(), partials[w].end());
        return out;
    }

    inline void workerLoop(int workerId) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
        // slot->worker eslesmesi sabit oldugu icin cache'ler thread-local, kilitsiz
        std::unordered_map<uintptr_t, std::string> nameCache;
        std::unordered_map<uintptr_t, uintptr_t> boneCache;   // pawn -> bonearray
        std::unordered_map<uint32_t, uintptr_t> entryCache;   // pawnHandle -> listEntry2
        std::unordered_map<uintptr_t, std::pair<uint32_t, short>> weaponCache; // pawn -> (weaponHandle, defIndex)
        uint32_t pass = 0;
        while (true) {
            if ((++pass % 500) == 0) { // slot'lar donusumlu kullanilir, bayat adres kalmasin
                boneCache.clear();
                entryCache.clear();
                weaponCache.clear();
                if (nameCache.size() > 256) nameCache.clear();
            }
            std::vector<PLAYER> local;
            local.reserve(8);
            uintptr_t entityList = memory->read<uintptr_t>(client + Offsets::dwEntityList);
            if (!entityList) {
                std::this_thread::yield(); // oyunda degil
                continue;
            }

            uintptr_t listEntry = memory->read<uintptr_t>(entityList + 0x10);
            if (!listEntry) {
                std::this_thread::yield();
                continue;
            }

            // Controller slotlari 0x70 stride ile dizili (bitisik degil).
            // ONCEKI BUG: listEntry+0x70'den 63*8 byte bitisik okunuyordu.
            // Bu sadece ilk ~4 slotu dogru verip gerisini cop okuyordu -> 2-3 kisiden fazlasi gelmiyordu.
            // FIX: her slot ayri okunur (slot i -> listEntry + (i+1)*0x70, i=0..63 => entity index 1..64).
            const double passTs = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();

            const bool needWeapon = cfg::weaponESP;
            const bool needBones = cfg::bones;
            const uintptr_t localPawnCached = localplayer::pawn;
            const int localTeamCached = localplayer::teamid;
            const bool teamCheck = cfg::teamCheck;

            for (int i = workerId; i < 64; i += WORKERS) {
                uintptr_t currentController = memory->read<uintptr_t>(listEntry + (uintptr_t)(i + 1) * 0x70);
                if (!currentController) continue;

                uint32_t pawnHandle = memory->read<uint32_t>(currentController + Offsets::m_hPlayerPawn);
                if (!pawnHandle || pawnHandle == 0xFFFFFFFF) continue;

                uintptr_t listEntry2 = 0;
                auto ec = entryCache.find(pawnHandle);
                if (ec != entryCache.end()) {
                    listEntry2 = ec->second;
                } else {
                    listEntry2 = memory->read<uintptr_t>(entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10);
                    if (listEntry2) entryCache[pawnHandle] = listEntry2;
                    else continue;
                }

                uintptr_t entityPawn = memory->read<uintptr_t>(listEntry2 + 0x70 * (pawnHandle & 0x1FF));
                if (!entityPawn || entityPawn == localPawnCached) continue;

                // dormant eleme: ag guncellemesi durmus (olu/uzak) pawn'i tasiyip cizme
                {
                    uintptr_t gs = memory->read<uintptr_t>(entityPawn + Offsets::m_pGameSceneNode);
                    if (!gs) continue;
                    if (memory->read<bool>(gs + Offsets::m_bDormant)) continue;
                }

                // health+team TEK okuma: pawn+0x348'den 160 byte (health @+4, team @+0x9F)
                uint8_t ht[0xA0];
                if (!memory->read_buffer(entityPawn + 0x348, ht, sizeof(ht))) continue;
                int health;
                memcpy(&health, ht + 0x4, sizeof(health));
                if (health <= 0 || health > 100) continue;
                int team = ht[0x9F];
                if (teamCheck && team == localTeamCached) continue;

                PLAYER ent;
                ent.actorBase = entityPawn;
                ent.controllerBase = currentController;
                ent.health = health;
                ent.team = team;
                ent.PlayerPos = memory->read<Vec3>(entityPawn + Offsets::m_vOldOrigin);
                ent.velocity = memory->read<Vec3>(entityPawn + Offsets::m_vecAbsVelocity);
                ent.ts = passTs;

                if (needWeapon) {
                    uintptr_t weaponServices = memory->read<uintptr_t>(entityPawn + Offsets::m_pWeaponServices);
                    if (weaponServices) {
                        uint32_t hActWep = memory->read<uint32_t>(weaponServices + Offsets::m_hActiveWeapon);
                        if (hActWep != 0xFFFFFFFF) {
                            auto wc = weaponCache.find(entityPawn);
                            if (wc != weaponCache.end() && wc->second.first == hActWep) {
                                ent.weaponame = wc->second.second; // silah degismemis, 0 okuma
                            } else {
                                uintptr_t weapon = GetEntityFromHandle(hActWep);
                                if (weapon) {
                                    short def = memory->read<short>(weapon + Offsets::m_AttributeManager + Offsets::m_Item + Offsets::m_iItemDefinitionIndex);
                                    ent.weaponame = def;
                                    weaponCache[entityPawn] = { hActWep, def };
                                }
                            }
                        }
                    }
                }

                if (needBones) {
                    uintptr_t bonearray = 0;
                    auto bc = boneCache.find(entityPawn);
                    if (bc != boneCache.end()) {
                        bonearray = bc->second;
                    } else {
                        uintptr_t gamescene = memory->read<uintptr_t>(entityPawn + Offsets::m_pGameSceneNode);
                        if (gamescene)
                            bonearray = memory->read<uintptr_t>(gamescene + Offsets::m_modelState + 0x80);
                        if (bonearray)
                            boneCache[entityPawn] = bonearray;
                    }
                    if (bonearray) {
                        // TEK read: 30 kemik * 32 byte stride
                        uint8_t raw[30 * 32];
                        if (memory->read_buffer(bonearray, raw, sizeof(raw))) {
                            ent.bones.reserve(30);
                            for (int j = 0; j < 30; ++j) {
                                Vec3 v;
                                memcpy(&v, raw + j * 32, sizeof(Vec3));
                                ent.bones.push_back(v);
                            }
                        }
                    }
                }

                auto it = nameCache.find(currentController);
                if (it != nameCache.end()) {
                    ent.name = it->second;
                } else {
                    ent.name = memory->read_string(currentController + Offsets::m_iszPlayerName, 32);
                    if (!ent.name.empty())
                        nameCache[currentController] = ent.name;
                }
                local.push_back(std::move(ent));
            }
            {
                std::lock_guard<std::mutex> lk(listMutex);
                partials[workerId].swap(local);
            }
            passCount[workerId].fetch_add(1, std::memory_order_relaxed);
            // perfSaver aciksa tur basina 1ms: RPM yuku duser, ESP hala ~500Hz
            if (cfg::perfSaver)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            // sleep YOK (normal mod) - driver cevaplari zaten donguyu dogal hizinda tutar
        }
    }

    inline void init() {
        for (int i = 0; i < WORKERS; ++i)
            std::thread([i]() { workerLoop(i); }).detach();
    }
}