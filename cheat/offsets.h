#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <iostream>
#include <Windows.h>

namespace Offsets {

    // build 2026-09-10 - a2x cs2-dumper (2026-09-10 12:36 UTC)
    uintptr_t m_pObserverServices = 0x1220; // C_BasePlayerPawn
    uintptr_t m_hObserverTarget = 0x4C;     // CPlayer_ObserverServices
    uintptr_t m_iObserverMode = 0x48;    // CPlayer_ObserverServices
    uintptr_t m_hController = 0x13D0; // C_BasePlayerPawn
    uintptr_t m_hObserverPawn = 0x918; // CCSPlayerController
    uintptr_t m_sSanitizedPlayerName = 0x868; // CCSPlayerController

    uintptr_t m_flTimerLength = 0x11D8; // C_PlantedC4
    uintptr_t m_iRoundTime = 0x68; // C_CSGameRules
    uintptr_t m_flNextGlow = 0x11C8; // C_PlantedC4

    uintptr_t m_flEmitSoundTime = 0x1C80; // C_CSPlayerPawn GameTime_t

    uintptr_t dwEntityList = 0x2577BE0;
    uintptr_t dwLocalPlayerPawn = 0x23CCC08;
    uintptr_t dwLocalPlayerController = 0x23A78D0;
    uintptr_t dwViewMatrix = 0x23D21F0;
    uintptr_t dwCSGOInput = 0x23E2610;
    uintptr_t dwViewAngles = 0x23E2C98;
    uintptr_t dwSensitivity = 0x23C9F18;
    uintptr_t dwSensitivity_sensitivity = 0x58;
    uintptr_t dwGlobalVars = 0x20B57C0;
    uintptr_t m_vecAbsVelocity = 0x3F8; // C_BaseEntity
    uintptr_t m_entitySpottedState = 0x1C60; // C_CSPlayerPawn
    uintptr_t m_bSpotted = 0x8; // EntitySpottedState_t
    //uintptr_t dwLocalPlayerController = 0x22F8028;
    uintptr_t m_flCurrentTime = 0x660; //client.cs
    //uintptr_t m_pObserverServices = 0x13F0; //client.dll.hpp < CBasePlayerPawn
    uintptr_t m_vecVelocity = 0x430; // C_BaseEntity
    uintptr_t m_iszPlayerName = 0x6F4; //client.dll.hpp < CBasePlayerController
    uintptr_t m_hPawn = 0x6BC; // CBasePlayerController
    uintptr_t dwPlantedC4 = 0x23973B8;//offsets.hpp
    uintptr_t m_flC4Blow = 0x11D0; // C_PlantedC4

    //BUTTONS
    uintptr_t jump = 0x20BA010;
    //uintptr_t left = 0x2053CF0;
    //uintptr_t right = 0x2053D80;
    //uintptr_t forward = 0x2053BD0;
    // uintptr_t duck = 0x2050F30;

    /// 

    uintptr_t m_bSpottedByMask = 0xC; // EntitySpottedState_t
    uintptr_t m_iIDEntIndex = 0x342C; // C_CSPlayerPawn
    uintptr_t m_hPlayerPawn = 0x914; // CCSPlayerController
    uintptr_t m_iHealth = 0x34C; // C_BaseEntity
    uintptr_t m_iTeamNum = 0x3E7; // C_BaseEntity
    uintptr_t m_vOldOrigin = 0x13B8; // C_BasePlayerPawn
    uintptr_t m_pGameSceneNode = 0x330; // C_BaseEntity
    uintptr_t m_modelState = 0x140; // CSkeletonInstance
    uintptr_t m_angEyeAngles = 0x3350; // C_CSPlayerPawn
    uintptr_t m_iShotsFired = 0x1C8C; // C_CSPlayerPawn
    //uintptr_t m_aimPunchAngle = 0x14D4;
    uintptr_t m_pAimPunchServices = 0x14B8; // C_CSPlayerPawn
    uintptr_t m_unpredictableBaseTick = 0xA0; // CCSPlayer_AimPunchServices
    uintptr_t m_aimPunchCache = 0xE98; // legacy, unused (rcs uses services+0x88)
    uintptr_t m_vecLastClipCameraPos = 0x3DA4; // legacy, unused
    uintptr_t m_bIsScoped = 0x1C78; // C_CSPlayerPawn
    uintptr_t m_flFlashDuration = 0x1428; // C_CSPlayerPawnBase
    uintptr_t m_flFlashMaxAlpha = 0x1424; // C_CSPlayerPawnBase float32
    uintptr_t m_flCurTime = 0xC;

    uintptr_t m_bBombTicking = 0x11A0; // C_PlantedC4
    uintptr_t m_bBeingDefused = 0x11DC; // C_PlantedC4
    uintptr_t m_flDefuseCountDown = 0x11F0; // C_PlantedC4
    uintptr_t m_bBombDefused = 0x11F4; // C_PlantedC4
    uintptr_t m_bHasExploded = 0x11D5; // C_PlantedC4
    //uintptr_t m_flCurTime = 0x30; // GlobalVars içindeki offset



    // uintptr_t m_pClippingWeapon = 0x3DC0;
    uintptr_t m_pWeaponServices = 0x1208; // C_BasePlayerPawn
    uintptr_t m_hActiveWeapon = 0x60; // CPlayer_WeaponServices

    uintptr_t m_szName = 0x720;
    uintptr_t m_nSubclassID = 0x380;
    uintptr_t m_fFlags = 0x3F4; // C_BaseEntity
    uintptr_t m_AttributeManager = 0x11A8; // C_EconEntity
    uintptr_t m_Item = 0x50; // C_AttributeContainer
    uintptr_t m_iItemDefinitionIndex = 0x1BA; // C_EconItemView

    uintptr_t m_vecViewOffset = 0xE78; // C_BaseModelEntity

    // ---- chams / glow (C_BaseModelEntity + CGlowProperty, 2026-09-10 dumper) ----
    uintptr_t m_Glow = 0xDE0; // C_BaseModelEntity
    uintptr_t m_clrRender = 0xC98; // C_BaseModelEntity Color
    uintptr_t m_fGlowColor = 0x8; // CGlowProperty Vector
    uintptr_t m_iGlowType = 0x30; // CGlowProperty int32 (3 = solid through-wall)
    uintptr_t m_glowColorOverride = 0x40; // CGlowProperty Color
    uintptr_t m_bGlowing = 0x51; // CGlowProperty bool
    uintptr_t dwGlowManager = 0x23C93F8; // client.dll (bilgi, su an kullanilmiyor)

    // ---- legit + nade esp (2026-09-10 dumper) ----
    uintptr_t m_bDormant = 0x103; // CGameSceneNode
    uintptr_t m_vecAbsOrigin = 0xC8; // CGameSceneNode
    uintptr_t m_designerName = 0x20; // CEntityIdentity (CUtlSymbolLarge -> char*)
    uintptr_t dwWeaponC4 = 0x2345728; // client.dll

}
