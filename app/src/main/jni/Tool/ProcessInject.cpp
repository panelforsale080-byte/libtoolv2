#include "Tool/ProcessInject.h"

#include <android/log.h>
#include <asm/ptrace.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <utility>
#include <vector>

#define LOG_TAG "ProcessInject"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#ifndef SYS_MMAP
#define SYS_MMAP 222
#endif
#ifndef SYS_MUNMAP
#define SYS_MUNMAP 215
#endif
#ifndef SYS_PROCESS_VM_WRITEV
#define SYS_PROCESS_VM_WRITEV 271
#endif
#ifndef RTLD_NOW
#define RTLD_NOW 2
#endif

namespace
{
struct MapEntry {
    uintptr_t start, end;
    char perms[8];
    uintptr_t offset;
    std::string path;
};

static std::vector<MapEntry> readMaps(pid_t pid) {
    std::vector<MapEntry> out;
    const std::string mpath =
        (pid == 0) ? "/proc/self/maps" : "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream f(mpath);
    std::string line;
    while (std::getline(f, line)) {
        MapEntry e{};
        char perms[8]{}, fpath[512]{};
        unsigned long lo, hi, off;
        if (sscanf(line.c_str(), "%lx-%lx %7s %lx %*s %*s %511s", &lo, &hi, perms, &off, fpath) >= 4) {
            e.start = lo;
            e.end = hi;
            e.offset = off;
            strncpy(e.perms, perms, sizeof(e.perms) - 1);
            e.path = fpath;
            out.push_back(e);
        }
    }
    return out;
}

static bool copyFile(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return false;
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
    chmod(dst, 0755);
    return true;
}

static void mkdirP(const char *path) {
    std::string p(path);
    for (size_t i = 1; i < p.size(); ++i) {
        if (p[i] == '/') {
            p[i] = '\0';
            mkdir(p.c_str(), 0755);
            p[i] = '/';
        }
    }
    mkdir(p.c_str(), 0755);
}

static std::pair<std::string, std::string> resolveInjectPath(const char *srcSo,
                                                             const std::string &virtualHostPkg) {
    if (!srcSo || !srcSo[0]) return {"", ""};
    const char *slash = strrchr(srcSo, '/');
    std::string base = slash ? slash + 1 : srcSo;
    std::string host = virtualHostPkg.empty() ? "inject" : virtualHostPkg;
    std::string dir = "/data/data/" + host + "/files/inject";
    mkdirP(dir.c_str());
    std::string written = dir + "/" + base;
    if (!copyFile(srcSo, written.c_str())) return {"", ""};
    return {written, written};
}

static void writeModeConfig(int mode, const std::string &writtenPath) {
    std::string cfg = writtenPath + ".mode.cfg";
    FILE *f = fopen(cfg.c_str(), "w");
    if (f) {
        fprintf(f, "%d\n", mode);
        fclose(f);
    }
}

static long peekWord(pid_t pid, uintptr_t addr) {
    errno = 0;
    return ptrace(PTRACE_PEEKTEXT, pid, (void *)addr, nullptr);
}

static bool pokeWord(pid_t pid, uintptr_t addr, long val) {
    return ptrace(PTRACE_POKETEXT, pid, (void *)addr, (void *)val) == 0;
}

static void pokeBytes(pid_t pid, uintptr_t addr, const uint8_t *data, size_t len) {
    for (size_t i = 0; i + sizeof(long) <= len; i += sizeof(long)) {
        long w;
        memcpy(&w, data + i, sizeof(long));
        pokeWord(pid, addr + i, w);
    }
    const size_t tail = len & (sizeof(long) - 1);
    if (tail) {
        const size_t i = len - tail;
        errno = 0;
        long w = peekWord(pid, addr + i);
        if (!errno) {
            memcpy(&w, data + i, tail);
            pokeWord(pid, addr + i, w);
        }
    }
}

static bool writeMem(pid_t pid, uintptr_t addr, const void *buf, size_t len) {
    iovec lv = {(void *)buf, len};
    iovec rv = {(void *)addr, len};
    if (syscall(SYS_PROCESS_VM_WRITEV, (long)pid, &lv, 1UL, &rv, 1UL, 0UL) == (ssize_t)len)
        return true;
    const std::string p = "/proc/" + std::to_string(pid) + "/mem";
    const int fd = open(p.c_str(), O_WRONLY);
    if (fd >= 0) {
        lseek64(fd, (off64_t)addr, SEEK_SET);
        const bool ok = (write(fd, buf, len) == (ssize_t)len);
        close(fd);
        if (ok) return true;
    }
    pokeBytes(pid, addr, (const uint8_t *)buf, len);
    return true;
}

static uintptr_t injectSyscall(pid_t pid, uintptr_t nr, uintptr_t a0, uintptr_t a1, uintptr_t a2,
                               uintptr_t a3, uintptr_t a4, uintptr_t a5) {
    user_pt_regs regs {}, saved {};
    iovec iov = {&regs, sizeof(regs)};
    if (ptrace(PTRACE_GETREGSET, pid, (void *)1L, &iov) < 0) return (uintptr_t)-1;
    memcpy(&saved, &regs, sizeof(regs));

    const uintptr_t pc = regs.pc;
    errno = 0;
    const long orig = peekWord(pid, pc);
    if (errno) return (uintptr_t)-1;

    const long patched = (orig & (long)0xFFFFFFFF00000000LL) | 0xD4000001L;
    pokeWord(pid, pc, patched);

    regs.regs[8] = nr;
    regs.regs[0] = a0;
    regs.regs[1] = a1;
    regs.regs[2] = a2;
    regs.regs[3] = a3;
    regs.regs[4] = a4;
    regs.regs[5] = a5;
    ptrace(PTRACE_SETREGSET, pid, (void *)1L, &iov);
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    waitpid(pid, nullptr, 0);

    ptrace(PTRACE_GETREGSET, pid, (void *)1L, &iov);
    const uintptr_t ret = regs.regs[0];

    pokeWord(pid, pc, orig);
    iov.iov_base = &saved;
    iov.iov_len = sizeof(saved);
    ptrace(PTRACE_SETREGSET, pid, (void *)1L, &iov);
    return ret;
}

static uintptr_t findLinkerBase(const std::vector<MapEntry> &maps) {
    const char *names[] = {"linker64", "/system/bin/linker64",
                           "/apex/com.android.runtime/bin/linker64", nullptr};
    for (const auto &e : maps)
        for (int i = 0; names[i]; i++)
            if (e.path.find(names[i]) != std::string::npos && e.offset == 0) return e.start;
    return 0;
}

static uintptr_t getDlopenInTarget(pid_t pid) {
    const auto selfMaps = readMaps(0);
    const auto targetMaps = readMaps(pid);
    const uintptr_t selfLinker = findLinkerBase(selfMaps);
    const uintptr_t targetLinker = findLinkerBase(targetMaps);
    if (!selfLinker || !targetLinker) return 0;

    void *sym = dlsym(RTLD_DEFAULT, "__loader_dlopen");
    if (!sym) sym = dlsym(RTLD_DEFAULT, "dlopen");
    if (!sym) return 0;

    const uintptr_t offset = (uintptr_t)sym - selfLinker;
    return targetLinker + offset;
}

static uintptr_t callInTarget(pid_t pid, uintptr_t func, uintptr_t a0, uintptr_t a1) {
    user_pt_regs regs {}, saved {};
    iovec iov = {&regs, sizeof(regs)};
    if (ptrace(PTRACE_GETREGSET, pid, (void *)1L, &iov) < 0) return (uintptr_t)-1;
    memcpy(&saved, &regs, sizeof(regs));

    const uintptr_t pc = regs.pc;
    const long orig0 = peekWord(pid, pc);
    const long orig1 = peekWord(pid, pc + sizeof(long));

    const uint32_t stub[2] = {0xD63F0120, 0xD4200000};
    writeMem(pid, pc, stub, sizeof(stub));

    regs.regs[0] = a0;
    regs.regs[1] = a1;
    regs.regs[9] = func;
    regs.regs[30] = pc;
    ptrace(PTRACE_SETREGSET, pid, (void *)1L, &iov);
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    waitpid(pid, nullptr, 0);

    ptrace(PTRACE_GETREGSET, pid, (void *)1L, &iov);
    const uintptr_t ret = regs.regs[0];

    pokeWord(pid, pc, orig0);
    pokeWord(pid, pc + sizeof(long), orig1);
    iov.iov_base = &saved;
    iov.iov_len = sizeof(saved);
    ptrace(PTRACE_SETREGSET, pid, (void *)1L, &iov);
    return ret;
}

static std::string g_apkLibDir;
} // namespace

namespace ProcessInject
{
std::string DetectVirtualHostPkg() {
    std::ifstream f("/proc/self/maps");
    if (!f.is_open()) return "";
    std::string line;
    static const char *PREFIX = "/data/user/0/";
    const size_t PLEN = strlen(PREFIX);
    while (std::getline(f, line)) {
        const size_t pos1 = line.find(PREFIX);
        if (pos1 == std::string::npos) continue;
        const size_t aStart = pos1 + PLEN;
        const size_t aEnd = line.find('/', aStart);
        if (aEnd == std::string::npos) continue;
        const std::string pkgA = line.substr(aStart, aEnd - aStart);
        if (pkgA.find('.') == std::string::npos) continue;
        const size_t pos2 = line.find(PREFIX, aEnd);
        if (pos2 == std::string::npos) continue;
        const size_t bStart = pos2 + PLEN;
        const size_t bEnd = line.find('/', bStart);
        if (bEnd == std::string::npos) continue;
        const std::string pkgB = line.substr(bStart, bEnd - bStart);
        if (pkgB.find('.') == std::string::npos) continue;
        if (pkgA == pkgB) continue;
        return pkgA;
    }
    return "";
}

std::vector<ProcessEntry> ListRunningProcesses() {
    std::vector<ProcessEntry> out;
    const pid_t self = getpid();
    DIR *d = opendir("/proc");
    if (!d) return out;
    while (dirent *ent = readdir(d)) {
        std::string name(ent->d_name);
        bool isNum = !name.empty();
        for (char c : name)
            if (!isdigit(c)) {
                isNum = false;
                break;
            }
        if (!isNum) continue;
        const int pid = std::stoi(name);
        if (pid <= 0 || pid == (int)self) continue;

        std::string label;
        {
            std::ifstream f("/proc/" + name + "/cmdline");
            std::getline(f, label, '\0');
        }
        if (label.empty()) {
            std::ifstream f("/proc/" + name + "/comm");
            std::getline(f, label);
        }
        if (label.empty() || label[0] == '[') continue;
        if (label.size() > 96) label.resize(96);

        out.push_back({pid, label});
    }
    closedir(d);
    std::sort(out.begin(), out.end(), [](const ProcessEntry &a, const ProcessEntry &b) {
        if (a.label != b.label) return a.label < b.label;
        return a.pid < b.pid;
    });
    if (out.size() > 300) out.resize(300);
    return out;
}

int FindPidByPackage(const std::string &package) {
    DIR *d = opendir("/proc");
    if (!d) return -1;
    while (dirent *ent = readdir(d)) {
        std::string name(ent->d_name);
        bool isNum = !name.empty();
        for (char c : name)
            if (!isdigit(c)) {
                isNum = false;
                break;
            }
        if (!isNum) continue;
        std::ifstream f("/proc/" + name + "/cmdline");
        std::string cmd;
        std::getline(f, cmd, '\0');
        if (cmd == package) {
            closedir(d);
            return std::stoi(name);
        }
    }
    closedir(d);
    return -1;
}

void SetApkNativeLibDir(const char *dir) {
    if (dir && dir[0])
        g_apkLibDir = dir;
}

std::string DefaultPayloadPath() {
    if (g_apkLibDir.empty()) return "/sdcard/libmonitor.so";
    if (g_apkLibDir.back() == '/')
        return g_apkLibDir + "libmonitor.so";
    return g_apkLibDir + "/libmonitor.so";
}

bool InjectLibrary(int pid, const char *srcSo, int monitorMode, std::string &err) {
    err.clear();
    if (pid <= 0) {
        err = "bad_pid";
        return false;
    }
    if (!srcSo || !srcSo[0]) {
        err = "empty_payload_path";
        return false;
    }
    struct stat st {};
    if (stat(srcSo, &st) != 0 || !S_ISREG(st.st_mode)) {
        err = std::string("payload_missing: ") + srcSo;
        return false;
    }

    LOGI("attach pid=%d payload=%s host=%s", pid, srcSo, DetectVirtualHostPkg().c_str());

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) != 0) {
        err = std::string("ptrace_attach: ") + strerror(errno);
        LOGE("%s", err.c_str());
        return false;
    }
    waitpid(pid, nullptr, 0);

    const std::string virtualPkg = DetectVirtualHostPkg();
    const auto paths = resolveInjectPath(srcSo, virtualPkg);
    if (paths.second.empty()) {
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        err = "copy_payload_failed";
        return false;
    }
    writeModeConfig(monitorMode, paths.first);

    const uintptr_t dlopenAddr = getDlopenInTarget(pid);
    if (!dlopenAddr) {
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        err = "dlopen_not_found";
        return false;
    }

    const size_t pathLen = paths.second.size() + 1;
    const size_t mapLen = (pathLen + 0xFFF) & ~0xFFFULL;
    const uintptr_t remoteBuf =
        injectSyscall(pid, SYS_MMAP, 0, mapLen, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, (uintptr_t)-1, 0);
    if (remoteBuf == (uintptr_t)-1 || remoteBuf == 0) {
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        err = "remote_mmap_failed";
        return false;
    }
    writeMem(pid, remoteBuf, paths.second.c_str(), pathLen);

    const uintptr_t handle = callInTarget(pid, dlopenAddr, remoteBuf, RTLD_NOW);
    injectSyscall(pid, SYS_MUNMAP, remoteBuf, mapLen, 0, 0, 0, 0);
    ptrace(PTRACE_DETACH, pid, nullptr, nullptr);

    if (handle == 0 || handle == (uintptr_t)-1) {
        err = "dlopen_in_target_failed path=" + paths.second;
        LOGE("%s", err.c_str());
        return false;
    }
    err = "injected handle=0x" + std::to_string(handle) + " " + paths.second;
    LOGI("%s", err.c_str());
    return true;
}
} // namespace ProcessInject
