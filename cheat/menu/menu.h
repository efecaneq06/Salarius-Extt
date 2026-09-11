#pragma once

// Yeni menu (project/ sablonundan portlandi, cfg::'ye bagli)
// Kullanim (main.cpp, tek TU - overlay/cfg header'lari inline oldugu icin baska TU'ya ekleme):
//   #include "cheat/menu/menu_impl.h"  -> main.cpp icinde
//   ui::initialize();  -> device + ImGui hazir olduktan sonra, ilk frame'den once
//   ui::render();      -> menu gorunurse her frame

namespace ui
{
    void initialize();
    void render();
}
