/*
 * rom_select.c - first-run ROM folder selection.
 *
 * The runtime expects a directory containing the decrypted Garou set
 * (253-pd.bin, 253-c1..c8, 253-s1.s1, 253-m1.m1, 253-v1..v4) and the MVS
 * BIOS files.  This module finds such a directory, or asks the user for one
 * with the platform's native folder picker.  If the user points at SNK's
 * original encrypted set, the bundled garou_decrypt helper is run into a
 * per-user cache, so the user does not have to prepare the files by hand.
 *
 * Rules:
 *   - an explicit --rom-path always wins;
 *   - --headless and --help never open a dialog;
 *   - "roms" in the current directory, next to the executable, next to the
 *     AppImage, GAROU_ROM_DIR and the usual per-user locations are tried
 *     before asking;
 *   - --select-rom forces the picker (stripped by main.c).
 *
 * No game data is read here beyond checking for the expected filenames.
 */
#include "rom_select.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#include <objbase.h>
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void trim_eol(char *s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t'))
        s[--n] = 0;
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int dir_exists(const char *path) {
#ifdef _WIN32
    DWORD a = GetFileAttributesA(path);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

static void join_path(char *out, size_t n, const char *a, const char *b) {
    if (!n) return;
    if (!a || !*a) {
        size_t lb = b ? strlen(b) : 0;
        if (lb >= n) lb = n - 1;
        if (lb) memcpy(out, b, lb);
        out[lb] = 0;
        return;
    }
    size_t la = strlen(a);
    size_t lb = b ? strlen(b) : 0;
    int slash = !(a[la - 1] == '/' || a[la - 1] == '\\');
    if (la + (size_t)slash + lb + 1 > n) {
        out[0] = 0;
        return;
    }
    memcpy(out, a, la);
    size_t o = la;
    if (slash) out[o++] = '/';
    if (lb) memcpy(out + o, b, lb);
    out[o + lb] = 0;
}

static int has_decrypted(const char *dir) {
    char p[PATH_MAX];
    join_path(p, sizeof(p), dir, "253-pd.bin");
    return file_exists(p);
}

static int has_encrypted(const char *dir) {
    char p[PATH_MAX];
    join_path(p, sizeof(p), dir, "253-ep1.p1");
    return file_exists(p);
}

static int get_self_dir(char *out, size_t n) {
#ifdef _WIN32
    wchar_t w[PATH_MAX];
    DWORD len = GetModuleFileNameW(NULL, w, (DWORD)(sizeof(w) / sizeof(w[0])));
    if (!len || len >= sizeof(w) / sizeof(w[0])) return -1;
    wchar_t *slash = wcsrchr(w, L'\\');
    if (!slash) return -1;
    *slash = 0;
    if (WideCharToMultiByte(CP_ACP, 0, w, -1, out, (int)n, NULL, NULL) == 0) return -1;
#else
    ssize_t len = readlink("/proc/self/exe", out, n - 1);
    if (len <= 0) return -1;
    out[len] = 0;
    char *slash = strrchr(out, '/');
    if (slash) *slash = 0;
#endif
    return 0;
}

static int get_cache_dir(char *out, size_t n) {
#ifdef _WIN32
    const char *base = getenv("LOCALAPPDATA");
    if (!base || !*base) base = getenv("USERPROFILE");
    if (!base || !*base) return -1;
    snprintf(out, n, "%s\\Garou\\roms", base);
#else
    const char *xdg = getenv("XDG_DATA_HOME");
    if (xdg && *xdg) {
        snprintf(out, n, "%s/garou/roms", xdg);
    } else {
        const char *home = getenv("HOME");
        if (!home || !*home) return -1;
        snprintf(out, n, "%s/.local/share/garou/roms", home);
    }
#endif
    return 0;
}

static void mkdir_one(const char *p) {
#ifdef _WIN32
    _mkdir(p);
#else
    mkdir(p, 0777);
#endif
}

static void mkdir_p(const char *path) {
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char c = *p;
            *p = 0;
            mkdir_one(tmp);
            *p = c;
        }
    }
    mkdir_one(tmp);
}

static int find_decrypt_tool(char *out, size_t n) {
    static const char *names[] = {
#ifdef _WIN32
        "garou_decrypt.exe", "garou_decrypt",
#else
        "garou_decrypt", "garou_decrypt.exe",
#endif
        NULL
    };
    char self[PATH_MAX];
    if (get_self_dir(self, sizeof(self)) == 0) {
        for (int i = 0; names[i]; i++) {
            char p[PATH_MAX];
            join_path(p, sizeof(p), self, names[i]);
            if (file_exists(p)) { snprintf(out, n, "%s", p); return 0; }
        }
    }
    for (int i = 0; names[i]; i++) {
        if (file_exists(names[i])) { snprintf(out, n, "%s", names[i]); return 0; }
    }
    return -1;
}

static int run_decrypt(const char *tool, const char *src, const char *dst) {
#ifdef _WIN32
    char cmd[PATH_MAX * 3];
    snprintf(cmd, sizeof(cmd), "\"%s\" --rom-path \"%s\" --out \"%s\"", tool, src, dst);
    wchar_t wcmd[PATH_MAX * 3];
    if (MultiByteToWideChar(CP_ACP, 0, cmd, -1, wcmd,
                            (int)(sizeof(wcmd) / sizeof(wcmd[0]))) == 0)
        return -1;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessW(NULL, wcmd, NULL, NULL, FALSE, CREATE_NO_WINDOW,
                        NULL, NULL, &si, &pi))
        return -1;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return code == 0 ? 0 : -1;
#else
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execl(tool, tool, "--rom-path", src, "--out", dst, (char *)NULL);
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
#endif
}

static int decrypt_into(const char *src, char *cache, size_t n) {
    if (get_cache_dir(cache, n) != 0) return -1;
    mkdir_p(cache);
    char tool[PATH_MAX];
    if (find_decrypt_tool(tool, sizeof(tool)) != 0) return -1;
    if (run_decrypt(tool, src, cache) != 0) return -1;
    char p[PATH_MAX];
    join_path(p, sizeof(p), cache, "253-pd.bin");
    return file_exists(p) ? 0 : -1;
}

/* 0 = not a usable ROM folder, 1 = usable (out filled), -1 = encrypted but
 * decryption failed. */
static int try_candidate(const char *dir, char *out, size_t n) {
    if (!dir || !*dir) return 0;
    if (has_decrypted(dir)) {
        snprintf(out, n, "%s", dir);
        return 1;
    }
    if (has_encrypted(dir)) {
        char cache[PATH_MAX];
        if (decrypt_into(dir, cache, sizeof(cache)) != 0) return -1;
        snprintf(out, n, "%s", cache);
        return 1;
    }
    return 0;
}

static void show_error(const char *msg) {
#ifdef _WIN32
    MessageBoxA(NULL, msg, "Garou: Mark of the Wolves", MB_ICONERROR | MB_OK);
#else
    fprintf(stderr, "error: %s\n", msg);
    if (getenv("DISPLAY") || getenv("WAYLAND_DISPLAY")) {
        char q[PATH_MAX * 2];
        size_t o = 0;
        q[o++] = '\'';
        for (const char *p = msg; *p && o + 5 < sizeof(q); p++) {
            if (*p == '\'') { memcpy(q + o, "'\\''", 4); o += 4; }
            else q[o++] = *p;
        }
        q[o++] = '\'';
        q[o] = 0;
        char cmd[PATH_MAX * 2 + 128];
        if (system("command -v zenity >/dev/null 2>&1") == 0) {
            snprintf(cmd, sizeof(cmd),
                     "zenity --error --title='Garou' --text=%s 2>/dev/null", q);
            int rc = system(cmd);
            (void)rc;
        } else if (system("command -v kdialog >/dev/null 2>&1") == 0) {
            snprintf(cmd, sizeof(cmd),
                     "kdialog --error %s --title 'Garou' 2>/dev/null", q);
            int rc = system(cmd);
            (void)rc;
        }
    }
#endif
}

#ifdef _WIN32

static int pick_dir(const char *start, char *out, size_t n) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int uninit = SUCCEEDED(hr) && hr != S_FALSE;

    IFileOpenDialog *dlg = NULL;
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileOpenDialog, (void **)&dlg);
    if (FAILED(hr)) {
        if (uninit) CoUninitialize();
        return -1;
    }

    DWORD opts = 0;
    dlg->lpVtbl->GetOptions(dlg, &opts);
    dlg->lpVtbl->SetOptions(dlg, opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM |
                                     FOS_PATHMUSTEXIST);
    dlg->lpVtbl->SetTitle(dlg, L"Select the Garou ROM folder");

    if (start && *start) {
        wchar_t wstart[PATH_MAX];
        if (MultiByteToWideChar(CP_ACP, 0, start, -1, wstart,
                                (int)(sizeof(wstart) / sizeof(wstart[0]))) != 0) {
            IShellItem *item = NULL;
            if (SUCCEEDED(SHCreateItemFromParsingName(wstart, NULL,
                                                      &IID_IShellItem,
                                                      (void **)&item))) {
                dlg->lpVtbl->SetFolder(dlg, item);
                item->lpVtbl->Release(item);
            }
        }
    }

    int ok = -1;
    hr = dlg->lpVtbl->Show(dlg, NULL);
    if (SUCCEEDED(hr)) {
        IShellItem *item = NULL;
        if (SUCCEEDED(dlg->lpVtbl->GetResult(dlg, &item))) {
            PWSTR psz = NULL;
            if (SUCCEEDED(item->lpVtbl->GetDisplayName(item, SIGDN_FILESYSPATH, &psz))) {
                if (WideCharToMultiByte(CP_ACP, 0, psz, -1, out, (int)n,
                                        NULL, NULL) != 0)
                    ok = 0;
                CoTaskMemFree(psz);
            }
            item->lpVtbl->Release(item);
        }
    }

    dlg->lpVtbl->Release(dlg);
    if (uninit) CoUninitialize();
    return ok;
}

#else

static int run_capture(const char *cmd, char *out, size_t n) {
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    char *r = fgets(out, (int)n, p);
    int status = pclose(p);
    if (!r || status != 0) return -1;
    trim_eol(out);
    return out[0] ? 0 : -1;
}

static void shell_quote(const char *in, char *out, size_t n) {
    if (!n) return;
    size_t o = 0;
    if (o + 1 < n) out[o++] = '\'';
    for (const char *p = in; *p && o + 5 < n; p++) {
        if (*p == '\'') { memcpy(out + o, "'\\''", 4); o += 4; }
        else out[o++] = *p;
    }
    if (o + 1 < n) out[o++] = '\'';
    out[o < n ? o : n - 1] = 0;
}

static int pick_dir(const char *start, char *out, size_t n) {
    char q[PATH_MAX];
    shell_quote(start && *start ? start : "", q, sizeof(q));

    char cmd[PATH_MAX * 2];
    if (start && *start) {
        snprintf(cmd, sizeof(cmd),
                 "zenity --file-selection --directory "
                 "--title='Select the Garou ROM folder' --filename=%s/ 2>/dev/null", q);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "zenity --file-selection --directory "
                 "--title='Select the Garou ROM folder' 2>/dev/null");
    }
    if (run_capture(cmd, out, n) == 0) return 0;

    if (start && *start) {
        snprintf(cmd, sizeof(cmd),
                 "kdialog --getexistingdirectory %s "
                 "--title 'Select the Garou ROM folder' 2>/dev/null", q);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "kdialog --getexistingdirectory . "
                 "--title 'Select the Garou ROM folder' 2>/dev/null");
    }
    if (run_capture(cmd, out, n) == 0) return 0;

    if (start && *start) {
        snprintf(cmd, sizeof(cmd),
                 "yad --file --directory "
                 "--title='Select the Garou ROM folder' --filename=%s/ 2>/dev/null", q);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "yad --file --directory "
                 "--title='Select the Garou ROM folder' 2>/dev/null");
    }
    if (run_capture(cmd, out, n) == 0) return 0;

    if (isatty(STDIN_FILENO)) {
        fprintf(stderr, "Garou ROM folder (contains 253-pd.bin or 253-ep1.p1): ");
        fflush(stderr);
        if (fgets(out, (int)n, stdin)) {
            trim_eol(out);
            if (out[0]) return 0;
        }
    }
    return -1;
}

#endif

static int try_defaults(char *out, size_t n) {
    char p[PATH_MAX];

    const char *env = getenv("GAROU_ROM_DIR");
    if (env && *env) {
        int r = try_candidate(env, out, n);
        if (r == 1) return 1;
    }

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        join_path(p, sizeof(p), cwd, "roms");
        int r = try_candidate(p, out, n);
        if (r == 1) return 1;
    }

    char self[PATH_MAX];
    if (get_self_dir(self, sizeof(self)) == 0) {
        join_path(p, sizeof(p), self, "roms");
        int r = try_candidate(p, out, n);
        if (r == 1) return 1;
    }

    const char *appimage = getenv("APPIMAGE");
    if (appimage && *appimage) {
        char d[PATH_MAX];
        snprintf(d, sizeof(d), "%s", appimage);
        char *slash = strrchr(d, '/');
        if (slash) *slash = 0;
        join_path(p, sizeof(p), d, "roms");
        int r = try_candidate(p, out, n);
        if (r == 1) return 1;
    }

#ifndef _WIN32
    const char *home = getenv("HOME");
    if (home && *home) {
        static const char *rels[] = {
            "Emulation/roms/garou", "roms/garou", "roms",
            ".local/share/garou/roms", NULL
        };
        for (int i = 0; rels[i]; i++) {
            join_path(p, sizeof(p), home, rels[i]);
            int r = try_candidate(p, out, n);
            if (r == 1) return 1;
        }
    }
#else
    const char *profile = getenv("USERPROFILE");
    if (profile && *profile) {
        join_path(p, sizeof(p), profile, "roms");
        int r = try_candidate(p, out, n);
        if (r == 1) return 1;
    }
    char cache[PATH_MAX];
    if (get_cache_dir(cache, sizeof(cache)) == 0) {
        int r = try_candidate(cache, out, n);
        if (r == 1) return 1;
    }
#endif

    return 0;
}

int garou_maybe_select_rom(int argc, char **argv, char *out, size_t out_size) {
    int has_rom = 0, headless = 0, help = 0, force = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom-path")) has_rom = 1;
        else if (!strcmp(argv[i], "--headless")) headless = 1;
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) help = 1;
        else if (!strcmp(argv[i], "--select-rom")) force = 1;
    }
    if (has_rom || headless || help) return 0;

    if (!force && try_defaults(out, out_size) == 1) return 1;

    char start[PATH_MAX] = "";
    const char *env = getenv("GAROU_ROM_DIR");
    if (env && *env && dir_exists(env)) {
        snprintf(start, sizeof(start), "%s", env);
    } else {
        char self[PATH_MAX];
        if (get_self_dir(self, sizeof(self)) == 0 && dir_exists(self))
            snprintf(start, sizeof(start), "%s", self);
        else {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd))) snprintf(start, sizeof(start), "%s", cwd);
        }
    }

    for (int attempt = 0; attempt < 3; attempt++) {
        char chosen[PATH_MAX];
        if (pick_dir(start, chosen, sizeof(chosen)) != 0)
            return -1;

        int r = try_candidate(chosen, out, out_size);
        if (r == 1) return 1;
        if (r < 0) {
            show_error("The selected folder contains the encrypted Garou set, "
                       "but garou_decrypt could not prepare it. Run "
                       "garou_decrypt (or Prepare-ROMs.ps1 on Windows) and "
                       "select the decrypted output folder.");
        } else {
            show_error("That folder does not contain the Garou ROM set. "
                       "Choose the folder with 253-pd.bin (decrypted) or "
                       "253-ep1.p1 (the original encrypted set).");
        }
        snprintf(start, sizeof(start), "%s", chosen);
    }
    return -1;
}
