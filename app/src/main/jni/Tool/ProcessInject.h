#pragma once

#include <string>
#include <vector>

namespace ProcessInject
{
    struct ProcessEntry
    {
        int pid = 0;
        std::string label;
    };

    /** Java ActivityManager + same-UID /proc; excludes LibTool APK. Sorted by label. */
    std::vector<ProcessEntry> ListRunningProcesses();

    /** Resolve inject payload: libtool.so in native lib dir, then /sdcard/libtool.so */
    std::string ResolvePayloadPath();
    /** Copy of detectVirtualHostPkg from inject.cpp (dual /data/user/0/ paths in maps). */
    std::string DetectVirtualHostPkg();

    /** Walk /proc for cmdline == package. Returns -1 if not found. */
    int FindPidByPackage(const std::string &package);

    /** Set once from Java nativeLibraryDir (optional default payload path). */
    void SetApkNativeLibDir(const char *dir);

    std::string DefaultPayloadPath();

    /**
     * ptrace attach → copy payload → remote mmap + dlopen in target.
     * srcSo must exist on disk (absolute path). monitorMode written for libmonitor-style payloads.
     */
    bool InjectLibrary(int pid, const char *srcSo, int monitorMode, std::string &err);
}
