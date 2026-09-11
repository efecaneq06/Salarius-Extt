#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <cmath>
#include <cstdio>
#include "entity.h"
#include "offsets.h"

namespace spectator {
	inline std::vector<std::string> spectators;
	inline std::mutex specMutex;

	inline void UpdateOnce() {
		std::vector<std::string> fresh;
		if (localplayer::pawn && client) {
		int myHealth = memory->read<int>(localplayer::pawn + Offsets::m_iHealth);
		uint32_t targetPawnHandle = 0;
		uint32_t targetControllerHandle = 0;

		if (myHealth > 0) {
			targetPawnHandle = memory->read<uint32_t>(localplayer::pawn + Offsets::m_hPlayerPawn);
			uintptr_t localController = memory->read<uintptr_t>(client + Offsets::dwLocalPlayerController);
			if (localController) {
				targetControllerHandle = memory->read<uint32_t>(localController + Offsets::m_hPlayerPawn);
			}
		}

		else {
			uintptr_t observerServices = memory->read<uintptr_t>(localplayer::pawn + Offsets::m_pObserverServices);
			if (observerServices) {
				targetPawnHandle = memory->read<uint32_t>(observerServices + Offsets::m_hObserverTarget);
			}
		}


		if (targetPawnHandle || targetControllerHandle) {
		for (const auto& player : entities::snapshot()) {
			if (!player.actorBase || !player.controllerBase || player.name.empty()) continue;
			int entHealth = memory->read<int>(player.actorBase + Offsets::m_iHealth);
			if (entHealth > 0) continue;
			uint32_t m_hObsPawn = memory->read<uint32_t>(player.controllerBase + Offsets::m_hObserverPawn);
			if (!m_hObsPawn || m_hObsPawn == 0xFFFFFFFF) continue;
			uintptr_t entityList = memory->read<uintptr_t>(client + Offsets::dwEntityList);
			if (!entityList) continue;
			uintptr_t listEntry = memory->read<uintptr_t>(entityList + 0x8 * ((m_hObsPawn & 0x7FFF) >> 9) + 0x10);
			if (!listEntry) continue;
			uintptr_t obsPawnPtr = memory->read<uintptr_t>(listEntry + 0x70 * (m_hObsPawn & 0x1FF));
			if (!obsPawnPtr) continue;
			uintptr_t obsServices = memory->read<uintptr_t>(obsPawnPtr + Offsets::m_pObserverServices);
			if (!obsServices) continue;
			uint32_t currentObsTarget = memory->read<uint32_t>(obsServices + Offsets::m_hObserverTarget);
			if ((currentObsTarget & 0xFFFF) == (targetPawnHandle & 0xFFFF) ||
				(currentObsTarget & 0xFFFF) == (targetControllerHandle & 0xFFFF)) {
				if (!player.name.empty()) {
					fresh.push_back(player.name);
				}
			}
		}
		}
		}
		std::lock_guard<std::mutex> lk(specMutex);
		spectators.swap(fresh);
	}

	inline void cacheLoop() {
		while (true) {
			if (cfg::speclist) UpdateOnce();
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}
	}

	inline void init() {
		std::thread([]() { cacheLoop(); }).detach();
	}

	// Buyuk uyari: hayattayken seni izleyen varsa ekran ustte yanip soner + yeni izleyicide ses
	inline void DrawWarn() {
		if (!cfg::specWarn) return;
		if (localplayer::health <= 0) return;
		size_t n = 0;
		{
			std::lock_guard<std::mutex> lk(specMutex);
			n = spectators.size();
		}
		static size_t lastN = 0;
		if (n > lastN && lastN > 0 && !cfg::streamproof) {
			// yeni izleyici: tek kisa alarm (proof'ta sessiz, yayin sesine cikar)
			std::thread([]() { Beep(660, 120); }).detach();
		}
		// ilk gorunumde de uyar ama ses verme (olumden dogusa geciste liste dolar)
		lastN = n;
		if (!cfg::speclist) {
			// liste kapaliysa yine de sayiyi bil (throttle'lu, render thread'i bogma)
			static auto lastUpd = std::chrono::steady_clock::now() - std::chrono::seconds(1);
			auto now = std::chrono::steady_clock::now();
			if (now - lastUpd > std::chrono::milliseconds(500)) {
				lastUpd = now;
				UpdateOnce();
			}
		}
		if (n == 0) return;

		double t = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
		float blink = 0.55f + 0.45f * sinf((float)(t * 6.0));
		char buf[48];
		snprintf(buf, sizeof(buf), "IZLENIYORSUN  (%d)", (int)n);

		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		float sw = (float)GetSystemMetrics(SM_CXSCREEN);
		ImVec2 ts = ImGui::CalcTextSize(buf);
		ImVec2 tp(sw / 2 - ts.x / 2, 64.0f);
		dl->AddText(ImVec2(tp.x + 1, tp.y + 1), IM_COL32(0, 0, 0, 255), buf);
		dl->AddText(tp, IM_COL32(255, (int)(60 + 60 * (1.0f - blink)), 60, (int)(140 + 115 * blink)), buf);
	}



	inline void Draw() {
		if (!cfg::speclist) return;
		std::vector<std::string> list;
		{
			std::lock_guard<std::mutex> lk(specMutex);
			list = spectators;
		}
		ImGuiIO& io = ImGui::GetIO();

		ImVec4 accent = ImVec4(0.00f, 0.55f, 1.00f, 1.00f);
		ImVec4 bgTitle = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);

		
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 190, 350), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(170, 0), ImGuiCond_Always);

		ImGui::PushStyleColor(ImGuiCol_Border, accent);
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, bgTitle);
		ImGui::PushStyleColor(ImGuiCol_TitleBg, bgTitle);

		if (ImGui::Begin("Spectator List", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
			ImVec2 lineStart = ImGui::GetCursorScreenPos();
			lineStart.y -= 4.0f;
			ImVec2 lineEnd = ImVec2(lineStart.x + ImGui::GetContentRegionAvail().x, lineStart.y);

			ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, ImGui::GetColorU32(accent), 1.0f);

			ImGui::Dummy(ImVec2(0, 2));

			if (list.empty()) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
				ImGui::Text("No one watching...");
				ImGui::PopStyleColor();
			}
			else {
				for (const auto& name : list) {
					ImGui::TextColored(accent, ">");
					ImGui::SameLine();
					
					ImGui::Text("%s", name.c_str());
				}
			}
		}
		ImGui::End();

		ImGui::PopStyleColor(3);
	}

}