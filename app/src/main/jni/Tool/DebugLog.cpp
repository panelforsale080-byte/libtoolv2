#include "Tool/DebugLog.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <sys/stat.h>

namespace
{
std::mutex g_mu;
std::string g_text;
std::string g_filesDir = "/sdcard";
} // namespace

namespace DebugLog
{
void SetFilesDir(const char *dir)
{
    if (dir && dir[0])
        g_filesDir = dir;
}

void Clear()
{
    std::lock_guard<std::mutex> lock(g_mu);
    g_text.clear();
}

void Append(const std::string &line)
{
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    char ts[32];
    std::strftime(ts, sizeof(ts), "%H:%M:%S", std::localtime(&t));
    std::lock_guard<std::mutex> lock(g_mu);
    g_text += "[";
    g_text += ts;
    g_text += "] ";
    g_text += line;
    if (line.empty() || line.back() != '\n')
        g_text += "\n";
}

const std::string &Text()
{
    return g_text;
}

static bool writeTextFile(const std::string &path, const std::string &body)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
        return false;
    out << body;
    out.flush();
    out.close();
    chmod(path.c_str(), 0644);
    return true;
}

bool SaveToFile(std::string &writtenPath, std::string &err)
{
    err.clear();
    std::string body;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        body = g_text;
    }
    if (body.empty())
        body = "(empty debug log)\n";

    std::string dir = g_filesDir;
    if (dir.empty())
        dir = "/sdcard";
    if (dir.back() == '/')
        dir.pop_back();
    writtenPath = dir + "/libtool_debug.txt";

    if (!writeTextFile(writtenPath, body))
    {
        writtenPath = "/sdcard/libtool_debug.txt";
        if (!writeTextFile(writtenPath, body))
        {
            err = "cannot_write_debug_file";
            return false;
        }
    }
    return true;
}

void AutoSaveOnInjectSuccess(const std::string &extraLog, std::string &primaryPath)
{
    primaryPath.clear();
    Append("=== AUTO_SAVE after inject success ===");
    if (!extraLog.empty())
        Append(extraLog);

    std::string body;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        body = g_text;
    }

    std::string dir = g_filesDir;
    if (dir.empty())
        dir = "/sdcard";
    if (!dir.empty() && dir.back() == '/')
        dir.pop_back();

    const std::string internalMain = dir + "/libtool_debug.txt";
    const std::string internalInject = dir + "/libtool_last_inject_ok.txt";
    const std::string sdcardMain = "/sdcard/libtool_debug.txt";
    const std::string sdcardInject = "/sdcard/libtool_last_inject_ok.txt";

    if (writeTextFile(internalMain, body))
        primaryPath = internalMain;
    else if (writeTextFile(sdcardMain, body))
        primaryPath = sdcardMain;

    writeTextFile(internalInject, body);
    writeTextFile(sdcardInject, body);

    Append("AUTO_SAVED primary=" + (primaryPath.empty() ? std::string("(none)") : primaryPath));
    Append("AUTO_SAVED also libtool_last_inject_ok.txt (internal + /sdcard)");
}
} // namespace DebugLog
