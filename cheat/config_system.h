#pragma once
#include <fstream>
#include <string>
#include "cfg.h"
#include "json.hpp"

using json = nlohmann::json;

namespace config_system
{
    inline void Save(const std::string& name)
    {
        json j;

        j["legitmode"] = cfg::legitmode;
        j["fovSize"] = cfg::fovSize;
        j["smoothing"] = cfg::smoothing;
        j["aimKey"] = cfg::aimKey;
        j["aimBone"] = cfg::aimBone;
        j["fovDistScale"] = cfg::fovDistScale;
        j["humanize"] = cfg::humanize;
        j["aimReactMs"] = cfg::aimReactMs;
        j["maxSnapDeg"] = cfg::maxSnapDeg;
        j["triggerJitter"] = cfg::triggerJitter;
        j["wallCheck"] = cfg::wallCheck;
        j["autoStop"] = cfg::autoStop;
        j["instaKill"] = cfg::instaKill;

        j["triggerbot"] = cfg::triggerbot;
        j["triggerDelay"] = cfg::triggerDelay;
        j["triggerKey"] = cfg::triggerKey;

        j["rcs"] = cfg::rcs;
        j["rcsStrength"] = cfg::rcsStrength;

        j["espOn"] = cfg::espOn;
        j["bones"] = cfg::bones;
        j["healthBar"] = cfg::healthBar;
        j["healthText"] = cfg::healthText;

        j["bhop"] = cfg::bhop;
        j["noflash"] = cfg::noflash;
        j["bombtimer"] = cfg::bombtimer;
        j["speclist"] = cfg::speclist;

        j["streamproof"] = cfg::streamproof;
        j["proofPauseMem"] = cfg::proofPauseMem;
        j["soundesp"] = cfg::soundesp;

        j["radar2D"] = cfg::radar2D;
        j["radarSize"] = cfg::radarSize;

        j["nadeESP"] = cfg::nadeESP;
        j["hitmarker"] = cfg::hitmarker;
        j["hitSound"] = cfg::hitSound;
        j["killSound"] = cfg::killSound;
        j["perfSaver"] = cfg::perfSaver;
        j["crosshair"] = cfg::crosshair;
        j["crossSize"] = cfg::crossSize;
        j["crossGap"] = cfg::crossGap;
        j["crossDot"] = cfg::crossDot;
        j["specWarn"] = cfg::specWarn;
        for (int i = 0; i < 4; i++) j["crossColor"].push_back(cfg::crossColor[i]);

        j["aimbot"] = cfg::aimbot;
        j["keyAimbot"] = cfg::keyAimbot;
        j["keyTrigger"] = cfg::keyTrigger;
        j["keyRCS"] = cfg::keyRCS;
        j["keyESP"] = cfg::keyESP;
        j["keyBones"] = cfg::keyBones;
        j["keyBhop"] = cfg::keyBhop;
        j["keyNoFlash"] = cfg::keyNoFlash;
        j["keyRadar"] = cfg::keyRadar;

        j["chams"] = cfg::chams;
        j["chamsStyle"] = cfg::chamsStyle;
        j["keyChams"] = cfg::keyChams;
        for (int i = 0; i < 4; i++) {
            j["chamsColor"].push_back(cfg::chamsColor[i]);
            j["chamsTeamColor"].push_back(cfg::chamsTeamColor[i]);
        }

        std::ofstream file("configs/" + name + ".json");
        file << j.dump(4);
    }

    inline void Load(const std::string& name)
    {
        std::ifstream file("configs/" + name + ".json");
        if (!file.good()) return;

        json j;
        file >> j;

        cfg::legitmode = j.value("legitmode", cfg::legitmode);
        cfg::fovSize = j.value("fovSize", cfg::fovSize);
        cfg::smoothing = j.value("smoothing", cfg::smoothing);
        cfg::aimKey = j.value("aimKey", cfg::aimKey);
        cfg::aimBone = j.value("aimBone", cfg::aimBone);
        cfg::fovDistScale = j.value("fovDistScale", cfg::fovDistScale);
        cfg::humanize = j.value("humanize", cfg::humanize);
        cfg::aimReactMs = j.value("aimReactMs", cfg::aimReactMs);
        cfg::maxSnapDeg = j.value("maxSnapDeg", cfg::maxSnapDeg);
        cfg::triggerJitter = j.value("triggerJitter", cfg::triggerJitter);
        cfg::wallCheck = j.value("wallCheck", cfg::wallCheck);
        cfg::autoStop = j.value("autoStop", cfg::autoStop);
        cfg::instaKill = j.value("instaKill", cfg::instaKill);

        cfg::triggerbot = j.value("triggerbot", cfg::triggerbot);
        cfg::triggerDelay = j.value("triggerDelay", cfg::triggerDelay);
        cfg::triggerKey = j.value("triggerKey", cfg::triggerKey);

        cfg::rcs = j.value("rcs", cfg::rcs);
        cfg::rcsStrength = j.value("rcsStrength", cfg::rcsStrength);

        cfg::espOn = j.value("espOn", cfg::espOn);
        cfg::bones = j.value("bones", cfg::bones);
        cfg::healthBar = j.value("healthBar", cfg::healthBar);
        cfg::healthText = j.value("healthText", cfg::healthText);

        cfg::bhop = j.value("bhop", cfg::bhop);
        cfg::noflash = j.value("noflash", cfg::noflash);
        cfg::bombtimer = j.value("bombtimer", cfg::bombtimer);
        cfg::speclist = j.value("speclist", cfg::speclist);

        cfg::streamproof = j.value("streamproof", cfg::speclist);
        cfg::proofPauseMem = j.value("proofPauseMem", cfg::proofPauseMem);
        cfg::soundesp = j.value("soundesp", cfg::speclist);

        cfg::radar2D = j.value("radar2D", cfg::radar2D);
        cfg::radarSize = j.value("radarSize", cfg::radarSize);

        cfg::nadeESP = j.value("nadeESP", cfg::nadeESP);
        cfg::hitmarker = j.value("hitmarker", cfg::hitmarker);
        cfg::hitSound = j.value("hitSound", cfg::hitSound);
        cfg::killSound = j.value("killSound", cfg::killSound);
        cfg::perfSaver = j.value("perfSaver", cfg::perfSaver);
        cfg::crosshair = j.value("crosshair", cfg::crosshair);
        cfg::crossSize = j.value("crossSize", cfg::crossSize);
        cfg::crossGap = j.value("crossGap", cfg::crossGap);
        cfg::crossDot = j.value("crossDot", cfg::crossDot);
        cfg::specWarn = j.value("specWarn", cfg::specWarn);
        if (j.contains("crossColor") && j["crossColor"].is_array() && j["crossColor"].size() == 4)
            for (int i = 0; i < 4; i++) cfg::crossColor[i] = j["crossColor"][i].get<float>();

        cfg::aimbot = j.value("aimbot", cfg::aimbot);
        cfg::keyAimbot = j.value("keyAimbot", cfg::keyAimbot);
        cfg::keyTrigger = j.value("keyTrigger", cfg::keyTrigger);
        cfg::keyRCS = j.value("keyRCS", cfg::keyRCS);
        cfg::keyESP = j.value("keyESP", cfg::keyESP);
        cfg::keyBones = j.value("keyBones", cfg::keyBones);
        cfg::keyBhop = j.value("keyBhop", cfg::keyBhop);
        cfg::keyNoFlash = j.value("keyNoFlash", cfg::keyNoFlash);
        cfg::keyRadar = j.value("keyRadar", cfg::keyRadar);

        cfg::chams = j.value("chams", cfg::chams);
        cfg::chamsStyle = j.value("chamsStyle", cfg::chamsStyle);
        cfg::keyChams = j.value("keyChams", cfg::keyChams);
        if (j.contains("chamsColor") && j["chamsColor"].is_array() && j["chamsColor"].size() == 4)
            for (int i = 0; i < 4; i++) cfg::chamsColor[i] = j["chamsColor"][i].get<float>();
        if (j.contains("chamsTeamColor") && j["chamsTeamColor"].is_array() && j["chamsTeamColor"].size() == 4)
            for (int i = 0; i < 4; i++) cfg::chamsTeamColor[i] = j["chamsTeamColor"][i].get<float>();
    }
}
