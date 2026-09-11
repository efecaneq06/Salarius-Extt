#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winioctl.h>
#include <string_view>
#include <iostream>
#include <memory>
#include <TlHelp32.h>

#define DRAGON_DEVICE 0x8000
#define IOCTL_ATTACH CTL_CODE(DRAGON_DEVICE, 0x4452, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_READ CTL_CODE(DRAGON_DEVICE, 0x4453, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_MODULE_BASE CTL_CODE(DRAGON_DEVICE, 0x4454, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_PID CTL_CODE(DRAGON_DEVICE, 0x4455, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_WRITE CTL_CODE(DRAGON_DEVICE, 0x4456, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

class Memory
{
private:
    DWORD processId = 0;
    HANDLE kernelDriver = nullptr;
    HANDLE hProcess = nullptr;
    bool userMode = false;

    typedef struct _Request
    {
        HANDLE process_id;
        PVOID target;
        PVOID buffer;
        SIZE_T size;
    } Request;

    typedef struct _PID_PACK
    {
        UINT32 pid;
        WCHAR name[1024];
    } PID_PACK;

    typedef struct _MODULE_PACK {
        UINT32 pid;
        UINT64 baseAddress;
        SIZE_T size;
        WCHAR moduleName[1024];
    } MODULE_PACK;

public:
    Memory(std::wstring_view processName) noexcept
    {
        kernelDriver = CreateFileW(
            L"\\\\.\\DragonBurn-kmd",
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        // KERNEL MODE
        if (kernelDriver && kernelDriver != INVALID_HANDLE_VALUE)
        {
            PID_PACK pidPack{};
            wcsncpy_s(pidPack.name, processName.data(), processName.size());

            if (DeviceIoControl(kernelDriver, IOCTL_GET_PID,
                &pidPack, sizeof(pidPack),
                &pidPack, sizeof(pidPack),
                nullptr, nullptr) && pidPack.pid != 0)
            {
                processId = pidPack.pid;

                Request attachReq{};
                attachReq.process_id = ULongToHandle(processId);

                DeviceIoControl(kernelDriver, IOCTL_ATTACH,
                    &attachReq, sizeof(attachReq),
                    &attachReq, sizeof(attachReq),
                    nullptr, nullptr);

                std::cout << "[+] Kernel mode active (PID: " << processId << ")\n";
                return;
            }

            CloseHandle(kernelDriver);
            kernelDriver = nullptr;
        }

        // FALLBACK
        userMode = true;
        kernelDriver = nullptr;

        std::cout << "[!] Kernel fail -> User mode\n";

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            std::cout << "[!] Snapshot fail\n";
            return;
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        if (Process32FirstW(snapshot, &entry))
        {
            do {
                if (!_wcsicmp(entry.szExeFile, processName.data()))
                {
                    processId = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);

        if (!processId)
        {
            std::cout << "[!] Process not found\n";
            return;
        }

        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
            FALSE, processId);

        if (!hProcess)
        {
            std::cout << "[!] OpenProcess fail\n";
            return;
        }

        std::cout << "[+] User mode active (PID: " << processId << ")\n";
    }

    ~Memory()
    {
        if (kernelDriver)
            CloseHandle(kernelDriver);

        if (hProcess)
            CloseHandle(hProcess);
    }

    bool IsConnected() const noexcept
    {
        return processId != 0 && (kernelDriver || hProcess);
    }

    DWORD GetPID() const noexcept
    {
        return processId;
    }

    bool IsKernelMode() const noexcept
    {
        return kernelDriver != nullptr;
    }

    bool IsUserMode() const noexcept
    {
        return userMode;
    }

    std::uintptr_t GetModuleBase(std::wstring_view moduleName) const noexcept
    {
        // KERNEL 
        if (kernelDriver)
        {
            MODULE_PACK modPack{};
            modPack.pid = processId;
            wcsncpy_s(modPack.moduleName, moduleName.data(), moduleName.size());

            if (DeviceIoControl(kernelDriver, IOCTL_GET_MODULE_BASE,
                &modPack, sizeof(modPack),
                &modPack, sizeof(modPack),
                nullptr, nullptr))
            {
                return (std::uintptr_t)modPack.baseAddress;
            }
        }

        // USER MODE 
        if (hProcess)
        {
            MODULEENTRY32W mod{};
            mod.dwSize = sizeof(mod);

            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
            if (snap == INVALID_HANDLE_VALUE)
                return 0;

            if (Module32FirstW(snap, &mod))
            {
                do {
                    if (!_wcsicmp(mod.szModule, moduleName.data()))
                    {
                        CloseHandle(snap);
                        return (std::uintptr_t)mod.modBaseAddr;
                    }
                } while (Module32NextW(snap, &mod));
            }

            CloseHandle(snap);
        }

        return 0;
    }

    template <typename T>
    T read(std::uintptr_t address) const noexcept
    {
        T value{};

        if (!address || address >= 0x7FFFFFFFFFFF)
            return value;

        // KERNEL
        if (kernelDriver && processId)
        {
            Request req{};
            req.process_id = ULongToHandle(processId);
            req.target = (PVOID)address;
            req.buffer = &value;
            req.size = sizeof(T);

            DeviceIoControl(kernelDriver,
                IOCTL_READ,
                &req, sizeof(req),
                &req, sizeof(req),
                nullptr, nullptr);
        }
        // USER
        else if (hProcess)
        {
            ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), nullptr);
        }

        return value;
    }

    template <typename T>
    void write(std::uintptr_t address, const T& value) const noexcept
    {
        if (!address || address >= 0x7FFFFFFFFFFF)
            return;

        if (kernelDriver && processId)
        {
            struct {
                PVOID target;
                SIZE_T size;
                UCHAR data[256];
            } req{};

            req.target = (PVOID)address;
            req.size = sizeof(T);
            memcpy(req.data, &value, sizeof(T));

            DeviceIoControl(kernelDriver,
                IOCTL_WRITE,
                &req, sizeof(req),
                nullptr, 0,
                nullptr, nullptr);
        }
        else if (hProcess)
        {
            WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), nullptr);
        }
    }

    bool read_buffer(std::uintptr_t address, void* buffer, size_t size) const noexcept
    {
        if (!buffer || !size || !address)
            return false;

        if (kernelDriver && processId)
        {
            Request req{};
            req.process_id = ULongToHandle(processId);
            req.target = (PVOID)address;
            req.buffer = buffer;
            req.size = size;

            return DeviceIoControl(kernelDriver,
                IOCTL_READ,
                &req, sizeof(req),
                &req, sizeof(req),
                nullptr, nullptr);
        }
        else if (hProcess)
        {
            return ReadProcessMemory(hProcess, (LPCVOID)address, buffer, size, nullptr);
        }

        return false;
    }

    std::string read_string(std::uintptr_t address, size_t size) const noexcept
    {
        if (!size) return "";

        std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);

        if (!read_buffer(address, buffer.get(), size))
            return "";

        buffer[size - 1] = '\0';
        return std::string(buffer.get());
    }
};
