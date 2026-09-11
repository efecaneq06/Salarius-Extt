namespace bhop
{
    inline float old_yaw = 0.0f;

    inline void Run()
    {
        if (!cfg::bhop || !(GetAsyncKeyState(VK_SPACE) & 0x8000)) return;

        uintptr_t localPawn = localplayer::G_Pawn();
        if (!localPawn) return;

        uint32_t flags = memory->read<uint32_t>(localPawn + Offsets::m_fFlags);
        bool onGround = (flags & (1 << 0));

        if (onGround) {
            memory->write<int>(client + Offsets::jump, 65537);
        }
        else {
            memory->write<int>(client + Offsets::jump, 256);

            float current_yaw = memory->read<float>(localPawn + Offsets::m_angEyeAngles + 0x4);
            float delta = current_yaw - old_yaw;

            if (delta > 180.0f) delta -= 360.0f;
            if (delta < -180.0f) delta += 360.0f;

            old_yaw = current_yaw;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}