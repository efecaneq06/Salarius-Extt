#pragma once

namespace cfg {
	bool teamCheck = true;
	inline bool wallCheck = true; 

	inline bool triggerbot = false;
    inline int triggerDelay = 10; 
    inline int triggerKey = VK_XBUTTON2; 

    bool bhop = false;
    bool noflash = false;
    bool bombtimer = false;

	bool speclist = true;
	bool streamproof = false;
	inline bool proofPauseMem = true; // proof acikken oyunun render'ladigi modulleri (glow) duraklat

	bool soundesp = false;
	inline bool nadeESP = false;   // el bombasi / ates ESP
	inline bool hitmarker = false; // isabet X + hasar logu
	inline bool hitSound = false;  // isabette tik sesi
	inline bool killSound = false; // killde iki ton ses + yazi
	inline bool perfSaver = false; // entity taramayi yavaslat (dusuk CPU)
	inline bool crosshair = false; // ozel crosshair overlay
	inline int crossSize = 6;      // cizgi uzunlugu px
	inline int crossGap = 4;       // merkez bosluk px
	inline bool crossDot = true;   // merkez nokta
	inline float crossColor[4] = { 0.0f, 1.0f, 0.4f, 1.0f };
	inline bool specWarn = true;   // izlenince buyuk uyari

    bool rcs = false;
    float rcsStrength = 2.0f;
	bool espOn = true;
	bool bones = true;
	bool healthBar = true;
	bool healthText = true;
	bool weaponESP = true;
	bool distanceESP = true;
	bool snapLines = true;
	float boxColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float boneColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	bool legitmode = false;
	bool autoStop = false;
	bool instaKill = false;
	bool drawFov = true;
	float smoothing = 5.0f;
	float fovSize = 50.f;
	int aimKey = 0x01;
	int aimBone = 0; // 0 Head, 1 Neck, 2 Chest, 3 Pelvis
	int aimMode = 0;
	inline bool fovDistScale = true; // mesafeye gore FOV olcegi
	inline bool humanize = true;     // smoothing'e +- jitter
	inline int aimReactMs = 30;      // hedef degisince bekleme (ms)
	inline float maxSnapDeg = 10.0f; // tick basina max donus (anti-snap)
	inline int triggerJitter = 15;   // trigger delay'e ek rastgele ms
	float fovColor[4] = { 1.0f, 1.0f, 1.0f, 0.6f };

	bool radar2D = false;
	int radarStyle = 0;
	float radarX = 150.0f;
	inline bool showPerf = true;
	inline bool chams = false;
	inline float chamsColor[4] = { 1.0f, 0.2f, 0.2f, 1.0f };
	inline float chamsTeamColor[4] = { 0.2f, 0.5f, 1.0f, 1.0f };
	inline int chamsStyle = 0; // 0 = glow (duvar arkasi), 1 = model tint
	inline bool aimbot = true; // master enable (hotkey ile ac/kapa)
	inline int keyAimbot = 0;
	inline int keyTrigger = 0;
	inline int keyRCS = 0;
	inline int keyESP = 0;
	inline int keyBones = 0;
	inline int keyBhop = 0;
	inline int keyNoFlash = 0;
	inline int keyRadar = 0;
	inline int keyChams = 0;
	float radarY = 150.0f;
	float radarSize = 200.0f;
	float radarRange = 3000.0f;
	float radarAlpha = 0.7f;
	bool radarRotate = true;
	bool radarShowDistance = false;
}
