#pragma once
#include "cfg.h"
#include "entity.h"

namespace thirdperson {

    struct Bytes3 { uint8_t d[3]; };

    static constexpr uintptr_t cmp_patch = 0x8183B4;
    static constexpr Bytes3 original_cmp = { 0x44, 0x38, 0x20 };
    static constexpr Bytes3 patched_cmp = { 0x90, 0x90, 0x90 };

    static bool patch_applied = false;
    static bool currently_active = false;

    static void applyPatch() {
        if (patch_applied) return;
        memory->write<Bytes3>(client + cmp_patch, patched_cmp);
        patch_applied = true;
        printf("[+] Thirdperson patch applied\n");
    }

    static void removePatch() {
        if (!patch_applied) return;
        memory->write<Bytes3>(client + cmp_patch, original_cmp);
        patch_applied = false;
        printf("[+] Thirdperson patch removed\n");
    }

    static void setState(bool state) {
        memory->write<uint8_t>(client + Offsets::dwCSGOInput + 0x251, state ? 1 : 0);
    }

    static bool needsReapply() {
        if (!localplayer::pawn) return true;
        if (currently_active && cfg::thirdperson) {
            uint8_t cur = memory->read<uint8_t>(client + Offsets::dwCSGOInput + 0x251);
            if (cur == 0) return true;
        }
        return false;
    }

    static void tick() {
        if (needsReapply()) {
            currently_active = false;
        }

        if (cfg::thirdperson && !currently_active) {
            applyPatch();
            setState(true);
            currently_active = true;
        }
        else if (!cfg::thirdperson && currently_active) {
            removePatch();
            setState(false);
            currently_active = false;
        }
    }

    static void cleanup() {
        if (currently_active) {
            removePatch();
            setState(false);
            currently_active = false;
        }
    }
}

namespace silentaim {

    struct Bytes8 { uint8_t d[8]; };

    static constexpr uintptr_t vangles_patch = 0x7D8950;
    static constexpr Bytes8 original_vangles = { 0xF2, 0x0F, 0x11, 0x81, 0xA8, 0x14, 0x00, 0x00 };
    static constexpr Bytes8 patched_vangles = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

    static bool patch_applied = false;
    static bool currently_active = false;

    static void applyPatch() {
        if (patch_applied) return;
        memory->write<Bytes8>(client + vangles_patch, patched_vangles);
        patch_applied = true;
        printf("[+] Silent aim patch applied\n");
    }

    static void removePatch() {
        if (!patch_applied) return;
        memory->write<Bytes8>(client + vangles_patch, original_vangles);
        patch_applied = false;
        printf("[+] Silent aim patch removed\n");
    }

    static void tick() {
        if (cfg::silentAim && !currently_active) {
            applyPatch();
            currently_active = true;
        }
        else if (!cfg::silentAim && currently_active) {
            removePatch();
            currently_active = false;
        }
    }

    static void cleanup() {
        if (currently_active) {
            removePatch();
            currently_active = false;
        }
    }
}
