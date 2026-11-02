#pragma once

#include <string>

namespace DebugLog
{
    void SetFilesDir(const char *dir);
    void Append(const std::string &line);
    const std::string &Text();
    /** Writes Text() to filesDir/libtool_debug.txt */
    bool SaveToFile(std::string &writtenPath, std::string &err);
    void Clear();
}
