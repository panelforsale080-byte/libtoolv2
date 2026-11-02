#pragma once
#include "imgui/imgui.h"
#include <string>

namespace EspRemote
{
    bool Connect(int pid, std::string &err);
    bool InjectSo(const char *payloadPath, std::string &err);
    int Pid();
    bool Resolve();
    const std::string &Log();
    void DrawPanel();
    void DrawOverlay(ImDrawList *draw, float screenWidth, float screenHeight);
}
