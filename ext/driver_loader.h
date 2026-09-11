#pragma once

#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>

#include "utils.hpp"
#include "intel_driver.hpp"
#include "kdmapper.hpp"
#include "embedded_driver.h"

namespace driver_loader {

    static const wchar_t* DRIVER_PATH = L"C:\\Windows\\System32\\drivers\\salarius.sys";

    static bool IsDriverLoaded()
    {
        HANDLE h = CreateFileW(L"\\\\.\\DragonBurn-kmd", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE)
        {
            CloseHandle(h);
            return true;
        }
        return false;
    }

    static bool DriverFileExists()
    {
        DWORD attr = GetFileAttributesW(DRIVER_PATH);
        return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
    }

    static bool ExtractDriverToSystem32()
    {
        std::ofstream file(DRIVER_PATH, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            return false;

        file.write(reinterpret_cast<const char*>(embedded_driver::data), embedded_driver::size);
        file.close();
        return true;
    }

    static bool MapDriverSys()
    {
        if (!DriverFileExists())
        {
            if (!ExtractDriverToSystem32())
                return false;
        }

        if (!NT_SUCCESS(intel_driver::Load()))
            return false;

        NTSTATUS exitCode = 0;
        if (!kdmapper::MapDriver(
            const_cast<uint8_t*>(embedded_driver::data),
            0, 0,
            false,
            true,
            kdmapper::AllocationMode::AllocatePool,
            false,
            nullptr,
            &exitCode))
        {
            intel_driver::Unload();
            return false;
        }

        if (!NT_SUCCESS(intel_driver::Unload()))
        {
        }

        Sleep(500);
        return true;
    }

    static bool EnsureDriverLoaded()
    {
        if (!DriverFileExists())
            ExtractDriverToSystem32();

        if (IsDriverLoaded())
            return true;

        if (!MapDriverSys())
            return false;

        if (!IsDriverLoaded())
            return false;

        return true;
    }
}
