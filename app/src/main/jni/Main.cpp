#include <jni.h>     // for JNI_ERR, JNIEnv, jclass, JNINativeM...
#include <pthread.h> // for pthread_create
#include <thread>
#include <unistd.h>              // for sleep
#include "Il2cpp/Il2cpp.h"       // for EnsureAttached, Init
#include "Il2cpp/il2cpp-class.h" // for Il2CppImage, Il2CppObject
#include "Includes/Logger.h"     // for LOGD, LOGI
#include "Includes/Utils.h"      // for isGameLibLoaded, isLibraryLoaded
#include "Includes/obfuscate.h"  // for make_obfuscator, OBFUSCATE
#include "Menu/ImGui.h"
#include "Tool/Keyboard.h"
#include "Tool/EspTab.h"
#include "Tool/EspRemote.h"
#include "Tool/ProcessInject.h"
#include "Tool/ProcessTab.h"
#include "Tool/SoMonitor.h"
#include "Tool/Tool.h"
#include "Tool/Util.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "sstream"
#include <android/native_window_jni.h>
#include "Tool/Unity.h"
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <vector>

void logcatJson(nlohmann::ordered_json &json)
{
    auto str = json.dump(4, '#');
    std::istringstream iss(str);
    std::string line;
    while (std::getline(iss, line))
    {
        usleep(100);
        LOGD("%s", line.c_str());
    }
}

template <typename T>
void ConfigSet(const char *key, T value);

Il2CppImage *g_Image = nullptr;
std::vector<MethodInfo *> g_Methods;
extern std::unordered_map<void *, HookerData> hookerMap;
extern int maxLine;

extern ImVec2 initialScreenSize;

// config
bool collapsed = false;
bool fullScreen = false;
bool resetWindow = false;
int selectedScale = 3;
int selectedTheme = 0;

bool doChangeScale = false;
bool doChangeTheme = false;


constexpr std::array<const char *, 7> possibleScale = {
    "Tiny", "Compact", "Small", "Normal", "Medium", "Large", "XL",
};
constexpr std::array<float, 7> scaleFactors = {0.45f, 0.55f, 0.65f, 0.75f, 0.9f, 1.0f, 1.2f};

constexpr std::array<const char *, 3> possibleThemes = {
    "Dark", "Light", "Classic",
};

ImGuiStyle initialStyle;

// ============================================================
// Axcel Modified tools - custom UI style (rounded, cyan accent)
// ============================================================
static void ApplyAxcelStyle()
{
    ImGuiStyle &s = ImGui::GetStyle();
    ImVec4 *c = s.Colors;

    // Tighter, cleaner layout (scaled later by ScaleAllSizes)
    s.WindowPadding = ImVec2(10.0f, 10.0f);
    s.FramePadding = ImVec2(8.0f, 5.0f);
    s.ItemSpacing = ImVec2(8.0f, 6.0f);
    s.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    s.ScrollbarSize = 12.0f;
    s.GrabMinSize = 10.0f;
    s.WindowRounding = 10.0f;
    s.ChildRounding = 6.0f;
    s.FrameRounding = 6.0f;
    s.PopupRounding = 8.0f;
    s.ScrollbarRounding = 10.0f;
    s.GrabRounding = 6.0f;
    s.TabRounding = 6.0f;
    s.WindowBorderSize = 1.0f;
    s.FrameBorderSize = 1.0f;
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    if (selectedTheme != 1) // skip recolor on Light theme
    {
        const ImVec4 panel = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
        const ImVec4 panelHover = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
        const ImVec4 accent = ImVec4(0.10f, 0.65f, 0.95f, 1.00f);      // cyan
        const ImVec4 accentActive = ImVec4(0.06f, 0.80f, 1.00f, 1.00f);

        c[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.10f, 0.94f);
        c[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.09f, 0.11f, 0.90f);
        c[ImGuiCol_PopupBg] = panel;
        c[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.27f, 0.60f);
        c[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.06f, 0.08f, 1.00f);
        c[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.35f, 0.55f, 1.00f);
        c[ImGuiCol_FrameBg] = panel;
        c[ImGuiCol_FrameBgHovered] = panelHover;
        c[ImGuiCol_FrameBgActive] = ImVec4(0.10f, 0.40f, 0.60f, 1.00f);
        c[ImGuiCol_Button] = ImVec4(0.10f, 0.45f, 0.68f, 0.70f);
        c[ImGuiCol_ButtonHovered] = accent;
        c[ImGuiCol_ButtonActive] = accentActive;
        c[ImGuiCol_Header] = ImVec4(0.10f, 0.45f, 0.68f, 0.35f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.10f, 0.55f, 0.80f, 0.60f);
        c[ImGuiCol_HeaderActive] = ImVec4(0.10f, 0.65f, 0.95f, 0.80f);
        c[ImGuiCol_CheckMark] = accentActive;
        c[ImGuiCol_SliderGrab] = accent;
        c[ImGuiCol_SliderGrabActive] = accentActive;
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        c[ImGuiCol_ScrollbarGrabHovered] = panelHover;
        c[ImGuiCol_ScrollbarGrabActive] = accent;
        c[ImGuiCol_Tab] = ImVec4(0.10f, 0.30f, 0.45f, 0.80f);
        c[ImGuiCol_TabHovered] = accent;
        c[ImGuiCol_TableHeaderBg] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
        c[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
        c[ImGuiCol_Separator] = ImVec4(0.20f, 0.22f, 0.27f, 0.80f);
        c[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.58f, 0.62f, 1.00f);
    }
}

const char *title = OBFUSCATE("Axcel Modified tools");
bool useJava = false;

void draw_thread()
{
    static ImVec2 lastSize = ImVec2(0, 0);
    static ImVec2 lastPos = ImVec2(0, 0);

    if (useJava)
    {
        auto drawList = ImGui::GetBackgroundDrawList();
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        if (drawList && display.x > 0.f && display.y > 0.f)
            EspTab::DrawOverlay(drawList, display.x, display.y);
        collapsed = !ImGui::Begin(title, nullptr, 0);
        EspTab::Draw();
        ImGui::End();
        Tool::DrawNotifications();
        return;
    }

    if (resetWindow)
    {
        resetWindow = false;
        if (fullScreen)
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            auto screenSize = ImGui::GetIO().DisplaySize;
            ImGui::SetNextWindowSize(screenSize);
        }
        else
        {
            ImGui::SetNextWindowPos(lastPos);
            ImGui::SetNextWindowSize(lastSize);
        }
    }
    if (fullScreen)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, ImGui::GetFrameHeight()));
    }
    int i = 0;
    auto drawList = ImGui::GetBackgroundDrawList();
    {
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        EspTab::DrawOverlay(drawList, display.x, display.y);
    }

    for (auto &v : HookerData::visited)
    {
        if (v.name.empty())
            continue;
        char label[256]{0};
        sprintf(label, "%s", v.name.c_str());
        if (v.hitCount > 0)
        {
            sprintf(label, "%s (%dx)", label, v.hitCount);
        }
        auto labelSize = ImGui::CalcTextSize(label);
        ImVec2 labellPos{20, 150 + (labelSize.y * i)};

        auto dt = ImGui::GetIO().DeltaTime;
        constexpr ImVec4 GREEN = {0.f, 1.f, 0.f, 1.f};
        ImColor color = ImColor(1.f, 1.f, 1.f, 1.f);
        if (v.time > 0.f)
        {
            v.time -= dt;
            auto t = v.time;
            color = ImColor(ImLerp(color.Value.x, GREEN.x, t), ImLerp(color.Value.y, GREEN.y, t),
                            ImLerp(color.Value.z, GREEN.z, t), 1.f);
        }
        v.goneTime -= dt;
        if (v.goneTime > 0.f && v.goneTime <= 1.f)
        {
            auto t = v.goneTime;
            color.Value.w = ImLerp(0.f, color.Value.w, t);
        }
        if (v.goneTime <= 0.f)
        {
            v.name = "";
        }

        drawList->AddRectFilled(labellPos, {labellPos.x + labelSize.x, labellPos.y + labelSize.y},
                                IM_COL32(0, 0, 0, 100));
        drawList->AddText(labellPos, color, label);
        i++;
    }
    collapsed = !ImGui::Begin(title, nullptr, (fullScreen ? ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove : 0));
    if (fullScreen)
    {
        ImGui::PopStyleVar();
    }

#ifdef __DEBUG__
    static bool showDemoWindow = false;
#endif
    Keyboard::Update();
    static bool changeToToolsTab = false;
    if (ImGui::BeginTabBar("mainTabber"))
    {
        if (ImGui::BeginTabItem("Tools", nullptr, changeToToolsTab ? ImGuiTabItemFlags_SetSelected : 0))
        {
            changeToToolsTab = false;
            if (ImGui::Checkbox("Fullscreen", &fullScreen))
            {
                if (fullScreen)
                {
                    lastSize = ImGui::GetWindowSize();
                    lastPos = ImGui::GetWindowPos();
                }
                resetWindow = true;
            }
            Tool::Draw();
            ImGui::EndTabItem();
        }

        if (!hookerMap.empty())
        {
            if (ImGui::BeginTabItem("Tracer"))
            {
                ImGui::Text("Traced method count : %zu", hookerMap.size());
                std::vector<HookerData *> sortedHooker;
                for (auto &[name, data] : hookerMap)
                {
                    if (data.hitCount > 0)
                        sortedHooker.push_back(&data);
                }
                if (!sortedHooker.empty())
                {
                    if (ImGui::Button("Quick Restore"))
                    {
                        ImGui::OpenPopup("QuickRestorePopup");
                    }
                    std::sort(sortedHooker.begin(), sortedHooker.end(),
                              [](const HookerData *a, const HookerData *b) { return a->hitCount > b->hitCount; });
                    ImGui::BeginChild("TracerList", ImVec2(0, 0), ImGuiChildFlags_None,
                                      ImGuiWindowFlags_HorizontalScrollbar);
                    auto &tab = Tool::GetFirstTab();
                    for (auto v : sortedHooker)
                    {
                        char label[256]{0};
                        sprintf(label, "%s::%s (%dx)###%p", v->method->getClass()->getName(), v->method->getName(),
                                v->hitCount, v->method);
                        // if (ImGui::Button(label, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                        // {
                        //     toBeErased = v;
                        // }

                        ImGui::PushID(v->method);
                        bool opened =
                            tab.MethodViewer(v->method->getClass(), v->method, tab.getCachedParams(v->method));
                        if (!opened && !changeToToolsTab && ImGui::IsItemHeld())
                        {
                            LOGD("IsItemHeld %s", v->method->getName());
                            changeToToolsTab = true;
                            Tool::OpenNewTabFromClass(v->method->getClass()).setOpenedTab = true;
                            ;
                            ImGui::PopID();
                            break;
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndChild();
                    static auto &io = ImGui::GetIO();
                    ImGui::SetNextWindowSizeConstraints(ImVec2(io.DisplaySize.x / 1.2f, 0),
                                                        ImVec2(io.DisplaySize.x / 1.2f, io.DisplaySize.y / 2));
                    if (ImGui::BeginPopup("QuickRestorePopup",
                                          ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar))
                    {
                        HookerData *toBeErased = nullptr;
                        for (auto v : sortedHooker)
                        {
                            char label[256]{0};
                            sprintf(label, "%s::%s (%dx)###%p", v->method->getClass()->getName(), v->method->getName(),
                                    v->hitCount, v->method);
                            if (ImGui::Button(label, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                            {
                                toBeErased = v;
                            }
                        }
                        if (toBeErased)
                        {
                            Tool::ToggleHooker(toBeErased->method, 0);
                        }
                        ImGui::EndPopup();
                    }
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                    ImGui::Text("No traced method has been called yet");
                    ImGui::PopStyleColor();
                }
                ImGui::EndTabItem();
            }
        }
        if (ImGui::BeginTabItem("Strings"))
        {
            Tool::Strings();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Processes"))
        {
            ProcessTab::Draw();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("So Monitor"))
        {
            SoMonitor::Draw();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("ESP"))
        {
            EspTab::Draw();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Dumper"))
        {
            Tool::Dumper();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings"))
        {
            ImGui::TextColored(ImVec4(0.10f, 0.65f, 0.95f, 1.0f), "Display");
            ImGui::Separator();
            // ImGui::SliderFloat("Scale##Global", &font->Scale, .1f, 2.0f, "%.2f");
            auto preview = possibleScale[selectedScale];
            if (ImGui::BeginCombo("Scale##Global", preview))
            {
                for (int i = 0; i < possibleScale.size(); i++)
                {
                    // bool selected = strcmp(possibleScale[i], preview) == 0;
                    bool selected = i == selectedScale;
                    if (ImGui::Selectable(possibleScale[i], selected))
                    {
                        selectedScale = i;
                        ConfigSet("selectedScale", selectedScale);
                        doChangeScale = true;
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Checkbox("Enable if keyboard input is not being received", &Keyboard::check);
#ifdef __DEBUG__
            if (ImGui::Checkbox("Show Demo Window", &showDemoWindow))
            {
            }
#endif
            ImGui::TextColored(ImVec4(0.10f, 0.65f, 0.95f, 1.0f), "Info");
            ImGui::Separator();
            ImGui::Text("Menu: Axcel Modified tools v3.2");
            static auto packageName = Il2cpp::getPackageName();
            static auto unityVersion = Il2cpp::getUnityVersion();
            static auto gameVersion = Il2cpp::getGameVersion();
            ImGui::Text("Package: %s", packageName.c_str());
            ImGui::Text("Version: %s", gameVersion.c_str());
            ImGui::Text("Unity: %s", unityVersion.c_str());
#ifdef __aarch64__
            ImGui::Text("Arch: %s", "arm64-v8a");
#else
            ImGui::Text("Arch: %s", "armeabi-v7a");
#endif
            ImGui::Separator();

            // here you can add your telegram id/channel link
           
            if (ImGui::Button("Telegram: Axcel Modified", ImVec2(-1, 0)))
            {
                auto Application = Il2cpp::FindClass("UnityEngine.Application");
                auto OpenURL = Application->getMethod("OpenURL", 1);
                if (OpenURL)
                {
                    OpenURL->invoke_static<void>(Il2cpp::NewString(OBFUSCATE("https://t.me/axcelLOki")));
                }
                else
                {
                    Keyboard::Open(OBFUSCATE("https://t.me/axcelLOki"), nullptr);
                }
            }
            if (ImGui::Button("YouTube: AtoomsBm", ImVec2(-1, 0)))
            {
                auto Application = Il2cpp::FindClass("UnityEngine.Application");
                auto OpenURL = Application->getMethod("OpenURL", 1);
                if (OpenURL)
                {
                    OpenURL->invoke_static<void>(Il2cpp::NewString(OBFUSCATE("https://www.youtube.com/@AtoomsBm")));
                }
                else
                {
                    Keyboard::Open(OBFUSCATE("https://www.youtube.com/@AtoomsBm"), nullptr);
                }
            }
            ImGui::Separator();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    static bool doCalculate = false;
    if (doCalculate)
    {
        doCalculate = false;
        Tool::CalculateSomething();
    }

    if (doChangeTheme)
    {
        doChangeTheme = false;
        if (selectedTheme == 0)
            ImGui::StyleColorsDark();
        else if (selectedTheme == 1)
            ImGui::StyleColorsLight();
        else if (selectedTheme == 2)
            ImGui::StyleColorsClassic();

        // Apply the Axcel Modified tools custom style on top of the base theme
        ApplyAxcelStyle();
        // Update initialStyle to keep new colors but we will need to re-apply scale
        initialStyle = ImGui::GetStyle();
        doChangeScale = true;
    }

    if (doChangeScale)
    {
        doChangeScale = false;
        static auto font = ImGui::GetFont();

        font->Scale = scaleFactors[selectedScale];
        auto style = initialStyle;
        style.ScaleAllSizes(font->Scale);
        ImGui::GetStyle() = style;
        doCalculate = true;
        if (fullScreen)
            resetWindow = true;
    }

#ifdef __DEBUG__
    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow();
    }
#endif
    ImGui::Separator();
    ImGui::TextDisabled("Axcel Modified tools");
    ImGui::End();
    Tool::DrawNotifications();
}

static nlohmann::ordered_json gConf;

template <typename T>
void ConfigSet(const char *key, T value)
{
    LOGD(__FUNCTION__);
    gConf[key] = value;
    LOGD("ConfigWrite %s = %s", key, gConf[key].dump().c_str());
    Util::FileWriter fileWriter("tool_conf.json");
    fileWriter.write(gConf.dump(2).c_str());
}

template <typename T>
T ConfigGet(const char *key, T defaultValue)
{
    LOGD(__FUNCTION__);
    if (gConf.contains(key))
    {
        LOGD("ConfigGet %s = %s", key, gConf[key].dump().c_str());
        return gConf[key].get<T>();
    }
    else
    {
        ConfigSet(key, defaultValue);
    }
    return defaultValue;
}

void ConfigInit()
{
    Util::FileReader fileReader("tool_conf.json");
    if (fileReader.exists())
    {
        auto data = fileReader.read();
        try
        {
            gConf = nlohmann::json::parse(data);
            logcatJson(gConf);
        }
        catch (nlohmann::json::exception &e)
        {
            LOGE("ConfigInit error : %s", e.what());
            Util::FileWriter fileWriter("tool_conf.json");
            fileWriter.write("{}");
        }
    }
    else
    {
        Util::FileWriter fileWriter("tool_conf.json");
        fileWriter.write("{}");
    }
}

static void makeDumpDirs(const std::string &path)
{
    std::string current;
    current.reserve(path.size());
    for (char ch : path)
    {
        current.push_back(ch);
        if (ch == '/' && current.size() > 1)
            mkdir(current.c_str(), 0775);
    }
    mkdir(path.c_str(), 0775);
}

static bool canWriteDumpDir(const std::string &dir)
{
    makeDumpDirs(dir);
    const std::string probe = dir + "/.dump_write_probe";
    FILE *fp = std::fopen(probe.c_str(), "w");
    if (!fp)
        return false;
    std::fclose(fp);
    std::remove(probe.c_str());
    return true;
}

static std::string g_dumpPackage;
static std::string g_dumpDirReason;

static std::string chooseDumpFilesDir()
{
    char packageName[256] = {};
    FILE *fp = std::fopen("/proc/self/cmdline", "r");
    if (fp)
    {
        std::fread(packageName, 1, sizeof(packageName) - 1, fp);
        std::fclose(fp);
    }
    g_dumpPackage = packageName;

    std::vector<std::string> candidates;
    if (packageName[0])
    {
        candidates.push_back(std::string("/storage/emulated/0/Android/data/") + packageName + "/files");
        candidates.push_back(std::string("/sdcard/Android/data/") + packageName + "/files");
        candidates.push_back(std::string("/data/data/") + packageName + "/files");
        candidates.push_back(std::string("/data/user/0/") + packageName + "/files");
    }
    candidates.push_back("/storage/emulated/0/Download");
    candidates.push_back("/sdcard/Download");

    for (const auto &dir : candidates)
    {
        if (canWriteDumpDir(dir))
        {
            g_dumpDirReason = "writable " + dir;
            return dir;
        }
    }
    g_dumpDirReason = "no writable directory";
    return {};
}

static void appendDumpLog(const std::string &logPath, const char *msg)
{
    LOGI("%s", msg);
    if (logPath.empty())
        return;
    FILE *fp = std::fopen(logPath.c_str(), "a");
    if (!fp)
        return;
    std::fprintf(fp, "%s\n", msg);
    std::fclose(fp);
}

static void RunStartupDump(const std::string &filesDir, const std::string &logPath)
{
    if (filesDir.empty())
    {
        appendDumpLog(logPath, "ERROR: no writable files directory");
        return;
    }

    const std::string dumpPath = filesDir + "/dump.cs";
    appendDumpLog(logPath, ("dump path=" + dumpPath).c_str());
    Il2cpp::SetDumpLogPath(logPath);

    try
    {
        if (!Il2cpp::IsRuntimeReady())
        {
            appendDumpLog(logPath, "ERROR: IL2CPP runtime not ready; skip dump");
            return;
        }
        if (Il2cpp::Dump(dumpPath))
            appendDumpLog(logPath, "SUCCESS dump.cs written");
        else
            appendDumpLog(logPath, "ERROR: Dump() returned false");
    }
    catch (...)
    {
        appendDumpLog(logPath, "ERROR: dump exception");
    }
}

void on_init()
{
    LOGD(__FUNCTION__);

    initialStyle = ImGui::GetStyle();

    if (useJava || EspRemote::Pid() > 0 || Il2cpp::GetRuntimeLibraryName().empty())
    {
        selectedScale = 3;
        selectedTheme = 0;
        ImGui::StyleColorsDark();
        ApplyAxcelStyle();
        initialStyle = ImGui::GetStyle();
        auto *font = ImGui::GetFont();
        if (font)
        {
            font->Scale = scaleFactors[selectedScale];
            auto style = initialStyle;
            style.ScaleAllSizes(font->Scale);
            ImGui::GetStyle() = style;
        }
        LOGI("APK host mode (useJava=%d PID=%d); skip IL2CPP and tool_conf", useJava ? 1 : 0,
             EspRemote::Pid());
        return;
    }

    ConfigInit();
    selectedScale = ConfigGet<int>("selectedScale", selectedScale);
    if (selectedScale < 0 || selectedScale >= scaleFactors.size())
    {
        selectedScale = 3;
        ConfigSet("selectedScale", selectedScale);
    }
    selectedTheme = ConfigGet<int>("selectedTheme", selectedTheme);
    if (selectedTheme < 0 || selectedTheme >= possibleThemes.size())
    {
        selectedTheme = 0;
        ConfigSet("selectedTheme", selectedTheme);
    }
    doChangeTheme = true;
    doChangeScale = true;

    LOGI("%s has been loaded", Il2cpp::GetRuntimeLibraryName().c_str());

    Il2cpp::Init();
    Il2cpp::EnsureAttached();

    Keyboard::Init();

    LOGD("HOOKING...");

#ifndef LIB_INPUT
    if (Il2cpp::IsRuntimeReady())
        Unity::HookInput();
#endif

    if (auto assembly = Il2cpp::GetAssembly("Assembly-CSharp"))
        g_Image = assembly->getImage();
    auto images = Il2cpp::GetImages();
    Tool::Init(g_Image, images);

    for (auto image : images)
    {
        for (auto klass : image->getClasses())
        {
            for (auto m : klass->getMethods())
            {
                if (!m->methodPointer)
                    continue;
                g_Methods.emplace_back(m);
            }
        }
    }
    LOGD("%zu methods", g_Methods.size());
    LOGD("SORTING");
    if (!g_Methods.empty())
    {
        std::sort(g_Methods.begin(), g_Methods.end(),
                  [](const auto &a, const auto &b) { return a->methodPointer < b->methodPointer; });
        LOGPTR(g_Methods.front()->methodPointer);
        LOGPTR(g_Methods.back()->methodPointer);
    }
    LOGD("SORTED");
    LOGD("HOOKED!");
}

// we will run our hacks in a new thread so our while loop doesn't block process main thread
void *hack_thread(void *)
{
    logger::Clear();

    LOGI("pthread created");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const std::string filesDir = chooseDumpFilesDir();
    const std::string logPath = filesDir.empty() ? std::string() : filesDir + "/log.txt";
    Il2cpp::SetDumpLogPath(logPath);
    appendDumpLog(logPath, "hack_thread start");
    appendDumpLog(logPath, ("package=" + g_dumpPackage).c_str());
    appendDumpLog(logPath, ("files dir=" + (filesDir.empty() ? std::string("(none)") : filesDir)).c_str());
    appendDumpLog(logPath, ("dir reason=" + g_dumpDirReason).c_str());

    if (useJava && Il2cpp::GetRuntimeLibraryName().empty())
    {
        appendDumpLog(logPath, "APK host: no IL2CPP in this process, skip wait");
        appendDumpLog(logPath, "starting LibTool UI");
        return nullptr;
    }

    for (int i = 0; i < 180 && Il2cpp::GetRuntimeLibraryName().empty(); ++i)
    {
        if (i == 0 || i % 5 == 0)
            appendDumpLog(logPath, ("waiting for IL2CPP library " + std::to_string(i) + "s").c_str());
        sleep(1);
    }

    const std::string runtimeLib = Il2cpp::GetRuntimeLibraryName();
    appendDumpLog(logPath, ("runtime lib=" + (runtimeLib.empty() ? std::string("(none)") : runtimeLib)).c_str());
    Il2cpp::Init();
    for (int i = 0; i < 90 && !Il2cpp::IsRuntimeReady(); ++i)
    {
        if (i == 0 || i % 5 == 0)
            appendDumpLog(logPath, ("waiting for runtime ready " + std::to_string(i) + "s").c_str());
        sleep(1);
    }
    appendDumpLog(logPath, Il2cpp::IsRuntimeReady() ? "runtime ready=1" : "runtime ready=0");
    if (Il2cpp::EnsureAttached())
        RunStartupDump(filesDir, logPath);
    else
        appendDumpLog(logPath, "ERROR: EnsureAttached failed; skip dump");

    appendDumpLog(logPath, "starting LibTool UI");
    LOGINT(useJava);
    if (!useJava)
        initModMenu((void *)draw_thread, (void *)on_init);
    return nullptr;
}

extern int glWidth;
extern int glHeight;
extern "C"
{
    JNIEXPORT void JNICALL Java_imgui_il2cpp_tool_NativeMethods_onDrawFrame(JNIEnv *env, jclass clazz)
    {
        if (glWidth <= 0 || glHeight <= 0)
            return;
        internalDrawMenu(glWidth, glHeight);
    }

    JNIEXPORT void JNICALL Java_imgui_il2cpp_tool_NativeMethods_onSurfaceChanged(JNIEnv *env, jclass clazz, jint width,
                                                                                 jint height)
    {
        LOGD(__FUNCTION__);
        if (width <= 0 || height <= 0)
            return;
        glWidth = width;
        glHeight = height;
        if (initialScreenSize.x <= 0.f || initialScreenSize.y <= 0.f)
            initialScreenSize = ImVec2((float)width, (float)height);
        LOGD("glWidth = %d, glHeight = %d", glWidth, glHeight);
        setupMenu();
    }

    JNIEXPORT void JNICALL Java_imgui_il2cpp_tool_NativeMethods_onSurfaceCreate(JNIEnv *env, jclass clazz)
    {
        LOGD(__FUNCTION__);
        useJava = true;
        static bool booted = false;
        if (!booted)
        {
            initModMenu((void *)draw_thread, (void *)on_init, true);
            booted = true;
        }
    }

    JNIEXPORT void JNICALL Java_imgui_il2cpp_tool_NativeMethods_onTouchEvent(JNIEnv *env, jclass clazz, jint action,
                                                                              jfloat x, jint x_orig, jfloat y,
                                                                              jint y_orig)
    {
        handleInputEvent(action, x, y);
    }

    JNIEXPORT void JNICALL Java_imgui_il2cpp_tool_NativeMethods_onSurfaceCreated(JNIEnv *env, jclass clazz, jobject surface)
    {
        if (surface) {
            ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
            setNativeWindow(window);
        } else {
            setNativeWindow(nullptr);
        }
    }

    JNIEXPORT jstring JNICALL Java_imgui_il2cpp_tool_NativeMethods_getWindowRect(JNIEnv *env, jclass clazz)
    {
        return env->NewStringUTF(Tool::GetWindowRect().c_str());
    }

    JNIEXPORT jboolean JNICALL Java_imgui_il2cpp_tool_NativeMethods_connect(JNIEnv *env, jclass clazz, jint pid)
    {
        std::string err;
        const bool ok = EspRemote::Connect((int)pid, err);
        useJava = true;
        return ok ? JNI_TRUE : JNI_FALSE;
    }

    JNIEXPORT jstring JNICALL Java_imgui_il2cpp_tool_NativeMethods_connectError(JNIEnv *env, jclass clazz)
    {
        return env->NewStringUTF(EspRemote::Log().c_str());
    }

    JNIEXPORT void JNICALL Java_imgui_il2cpp_tool_NativeMethods_setNativeLibDir(JNIEnv *env, jclass clazz,
                                                                                 jstring dir)
    {
        if (!dir) return;
        const char *utf = env->GetStringUTFChars(dir, nullptr);
        if (utf)
        {
            ProcessInject::SetApkNativeLibDir(utf);
            env->ReleaseStringUTFChars(dir, utf);
        }
    }

    JNIEXPORT void JNICALL Java_imgui_il2cpp_tool_NativeMethods_setApkFilesDir(JNIEnv *env, jclass clazz,
                                                                                jstring dir)
    {
        if (!dir) return;
        const char *utf = env->GetStringUTFChars(dir, nullptr);
        if (utf)
        {
            Il2cpp::SetApkHostFilesDir(utf);
            env->ReleaseStringUTFChars(dir, utf);
        }
    }
}

JavaVM *g_vm = nullptr;
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, [[maybe_unused]] void *reserved)
{
    g_vm = vm;
    JNIEnv *env;
    vm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (env->FindClass("imgui/il2cpp/tool/NativeMethods") == nullptr)
    {
        LOGE("Could not find class imgui/il2cpp/tool/NativeMethods");
        env->ExceptionClear();
    }
    else
    {
        useJava = true;
    }

    return JNI_VERSION_1_6;
}
