// Standalone updater executable for SnipOCR
// No Qt dependency; pure Win32 API + minizip for zip extraction
// Usage: updater.exe install --info-json <info.json>
//        updater.exe restart <app_dir>

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <shlobj.h>

#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdio>

#include "nlohmann/json.hpp"
#include "minizip/unzip.h"

using json = nlohmann::json;

// Properly convert UTF-8 string to wide string (handles Chinese characters correctly)
static std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), nullptr, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], len);
    return wstr;
}
// Use FILE* + _wfopen instead of wofstream to avoid wide->narrow
// conversion failure on Chinese paths (wofstream truncates log in C locale)
static FILE* g_logFile = nullptr;
static std::wstring g_logPath;

static void init_log() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring logPath = exePath;
    size_t pos = logPath.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        logPath = logPath.substr(0, pos + 1) + L"updater.log";
    g_logPath = logPath;
    // ccs=UTF-8 enables fputws to write Chinese chars correctly
    g_logFile = _wfopen(logPath.c_str(), L"w, ccs=UTF-8");
}

// ─── Constants ────────────────────────────────────────

static const wchar_t* APP_EXE_NAME = L"SnipOCR.exe";
static const wchar_t* APP_NAME = L"SnipOCR";

static const std::set<std::wstring> USER_STATE_FILES = {
    L".config_settings.json",
    L".version.json",
    L"update_history.log",
    L"_update_info.json",
    L"_update_result.json",
    L"_update_done.flag",
    L"_rollback.flag"
};

// ─── Logging ──────────────────────────────────────────

static void log_msg(const std::wstring& msg) {
    if (!g_logFile) init_log();
    if (g_logFile) {
        fputws(msg.c_str(), g_logFile);
        fputws(L"\n", g_logFile);
        fflush(g_logFile);
    }
    OutputDebugStringW((L"[Updater] " + msg + L"\n").c_str());
}

static void log_err(const std::wstring& msg) {
    if (!g_logFile) init_log();
    if (g_logFile) {
        fputws(L"ERROR: ", g_logFile);
        fputws(msg.c_str(), g_logFile);
        fputws(L"\n", g_logFile);
        fflush(g_logFile);
    }
    OutputDebugStringW((L"[Updater ERROR] " + msg + L"\n").c_str());
}

// ─── Helpers ──────────────────────────────────────────

static bool file_exists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
}

static std::wstring to_lower(std::wstring s) {
    for (auto& c : s) c = towlower(c);
    return s;
}

static std::wstring get_app_dir_from_info(const std::wstring& info_file_path) {
    try {
        std::ifstream ifs(info_file_path);
        if (!ifs.is_open()) return L"";
        json info = json::parse(ifs);
        std::string appDir = info["app_dir"].get<std::string>();
        std::wstring result(appDir.begin(), appDir.end());
        return result;
    } catch (...) {
        return L"";
    }
}

// Convert wide string to multi-byte (ANSI codepage) for minizip
static std::string wstring_to_ansi(const std::wstring& wstr) {
    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (len <= 0) return "";
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &result[0], len, NULL, NULL);
    return result;
}

// Ensure directory exists (create intermediate dirs)
static bool ensure_directory(const std::wstring& dir_path) {
    if (dir_path.empty()) return false;
    if (GetFileAttributesW(dir_path.c_str()) != INVALID_FILE_ATTRIBUTES)
        return true;

    // Try to create parent first
    size_t pos = dir_path.find_last_of(L"\\/");
    if (pos != std::wstring::npos && pos > 0) {
        if (!ensure_directory(dir_path.substr(0, pos)))
            return false;
    }
    return CreateDirectoryW(dir_path.c_str(), nullptr) ? true :
           (GetLastError() == ERROR_ALREADY_EXISTS);
}

// ─── Kill main process ────────────────────────────────

static bool kill_main_exe(const std::wstring& app_dir) {
    std::wstring targetExe = app_dir + L"\\" + APP_EXE_NAME;
    // Unify path separators to backslash: app_dir from Qt QDir::absolutePath() uses forward slash,
    // but QueryFullProcessImageNameW returns backslash. Mismatch causes kill_main_exe to fail.
    for (auto& c : targetExe) {
        if (c == L'/') c = L'\\';
    }
    // Normalize path for comparison
    std::wstring targetNorm = to_lower(targetExe);
    // Remove \\?\ prefix if present
    if (targetNorm.find(L"\\\\?\\") == 0)
        targetNorm = targetNorm.substr(4);

    HANDLE Snipshot = CreateToolhelp32Snipshot(TH32CS_SnipPROCESS, 0);
    if (Snipshot == INVALID_HANDLE_VALUE) {
        log_err(L"CreateToolhelp32Snipshot failed");
        return false;
    }

    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);

    bool killed = false;
    if (Process32FirstW(Snipshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, APP_EXE_NAME) == 0) {
                HANDLE hProc = OpenProcess(
                    PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE,
                    FALSE, pe.th32ProcessID);
                if (hProc) {
                    WCHAR path[MAX_PATH] = {};
                    DWORD size = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProc, 0, path, &size)) {
                        std::wstring procPath = std::wstring(path);
                        for (auto& c : procPath) {
                            if (c == L'/') c = L'\\';
                        }
                        std::wstring procNorm = to_lower(procPath);
                        if (procNorm.find(L"\\\\?\\") == 0)
                            procNorm = procNorm.substr(4);

                        if (procNorm == targetNorm) {
                            if (TerminateProcess(hProc, 0)) {
                                log_msg(L"Terminated SnipOCR PID=" + std::to_wstring(pe.th32ProcessID));
                                killed = true;
                            } else {
                                log_err(L"TerminateProcess failed for PID=" + std::to_wstring(pe.th32ProcessID));
                            }
                        }
                    }
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(Snipshot, &pe));
    }
    CloseHandle(Snipshot);

    if (killed) Sleep(2000);
    return killed;
}

// ─── Backup / restore user state files ────────────────

static std::wstring get_temp_backup_dir(const std::wstring& app_dir) {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring backupDir = std::wstring(tempPath) + L"SnipOCR_update_backup_" +
                             std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(backupDir.c_str(), nullptr);
    return backupDir;
}

static void backup_user_state_files(const std::wstring& app_dir,
                                     const std::wstring& backup_dir) {
    for (const auto& fileName : USER_STATE_FILES) {
        std::wstring src = app_dir + L"\\" + fileName;
        std::wstring dst = backup_dir + L"\\" + fileName;
        if (file_exists(src)) {
            CopyFileW(src.c_str(), dst.c_str(), FALSE);
            log_msg(L"Backed up: " + fileName);
        }
    }
}

static void restore_user_state_files(const std::wstring& backup_dir,
                                      const std::wstring& app_dir) {
    for (const auto& fileName : USER_STATE_FILES) {
        std::wstring src = backup_dir + L"\\" + fileName;
        std::wstring dst = app_dir + L"\\" + fileName;
        if (file_exists(src)) {
            CopyFileW(src.c_str(), dst.c_str(), FALSE);
            log_msg(L"Restored: " + fileName);
        }
    }
}

static void cleanup_backup_dir(const std::wstring& backup_dir) {
    for (const auto& fileName : USER_STATE_FILES) {
        std::wstring f = backup_dir + L"\\" + fileName;
        DeleteFileW(f.c_str());
    }
    RemoveDirectoryW(backup_dir.c_str());
}

// ─── Robust file writer (handles locked exe/dll) ──────
//


//


static const int MAX_RETRIES = 8;
static const int RETRY_SLEEP_MS = 500;

static HANDLE create_output_file_robust(const std::wstring& outPath) {
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        
        HANDLE hFile = CreateFileW(outPath.c_str(), GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            return hFile;
        }

        DWORD err = GetLastError();
        
        if (err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION ||
            err == ERROR_ACCESS_DENIED) {
            std::wstring oldName = outPath + L".old_" +
                                   std::to_wstring(GetTickCount64());
            if (MoveFileExW(outPath.c_str(), oldName.c_str(),
                            MOVEFILE_REPLACE_EXISTING)) {
                
                hFile = CreateFileW(outPath.c_str(), GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (hFile != INVALID_HANDLE_VALUE) {
                    log_msg(L"Renamed locked file and created new: " + outPath);
                    return hFile;
                }
            }
        }

        
        if (attempt < MAX_RETRIES - 1) {
            Sleep(RETRY_SLEEP_MS);
        }
    }
    return INVALID_HANDLE_VALUE;
}

// ─── Zip extraction via minizip ───────────────────────

static bool extract_zip(const std::wstring& zip_path,
                        const std::wstring& dest_dir,
                        std::vector<std::wstring>& failedFiles) {
    log_msg(L"Extracting (minizip): " + zip_path + L" -> " + dest_dir);

    std::string zipPathAnsi = wstring_to_ansi(zip_path);
    if (zipPathAnsi.empty()) {
        log_err(L"Failed to convert zip path");
        return false;
    }

    unzFile uf = unzOpen(zipPathAnsi.c_str());
    if (!uf) {
        log_err(L"Failed to open zip file");
        return false;
    }

    // Get number of entries
    unz_global_info globalInfo;
    if (unzGetGlobalInfo(uf, &globalInfo) != UNZ_OK) {
        // Try to get count by iterating
    }

    int fileCount = 0;
    int err = unzGoToFirstFile(uf);
    while (err == UNZ_OK) {
        fileCount++;
        err = unzGoToNextFile(uf);
    }
    log_msg(L"Zip contains " + std::to_wstring(fileCount) + L" items");

    // Re-wind to first file
    err = unzGoToFirstFile(uf);
    if (err != UNZ_OK) {
        log_err(L"Failed to go to first file");
        unzClose(uf);
        return false;
    }

    const int BUF_SIZE = 65536;
    char* readBuf = new char[BUF_SIZE];

    int extracted = 0;
    do {
        char fileName[512];
        unz_file_info fileInfo;

        err = unzGetCurrentFileInfo(uf, &fileInfo, fileName, sizeof(fileName) - 1,
                                    NULL, 0, NULL, 0);
        if (err != UNZ_OK) {
            log_err(L"Failed to get current file info");
            break;
        }
        fileName[fileInfo.size_filename] = '\0';

        // Convert filename to wide string
        std::wstring wFileName;
        int wLen = MultiByteToWideChar(CP_UTF8, 0, fileName, -1, NULL, 0);
        if (wLen > 0) {
            wchar_t* wBuf = new wchar_t[wLen];
            MultiByteToWideChar(CP_UTF8, 0, fileName, -1, wBuf, wLen);
            wFileName = wBuf;
            delete[] wBuf;
        } else {
            // Fallback: use ANSI
            wLen = MultiByteToWideChar(CP_ACP, 0, fileName, -1, NULL, 0);
            if (wLen > 0) {
                wchar_t* wBuf = new wchar_t[wLen];
                MultiByteToWideChar(CP_ACP, 0, fileName, -1, wBuf, wLen);
                wFileName = wBuf;
                delete[] wBuf;
            } else {
                wFileName = std::wstring(fileName, fileName + strlen(fileName));
            }
        }

        // Skip directories (they end with / or \)
        bool isDir = false;
        if (wFileName.length() > 0) {
            wchar_t last = wFileName[wFileName.length() - 1];
            if (last == L'/' || last == L'\\')
                isDir = true;
        }

        // Build full output path
        std::wstring outPath = dest_dir + L"\\" + wFileName;

        // Normalize forward slashes to backslashes
        for (auto& c : outPath) {
            if (c == L'/') c = L'\\';
        }

        if (isDir) {
            // Create directory
            if (!ensure_directory(outPath)) {
                log_err(L"Failed to create directory: " + wFileName);
                continue;
            }
            log_msg(L"Created dir: " + wFileName);
            continue;
        }

        // Ensure parent directory exists
        size_t lastSlash = outPath.find_last_of(L'\\');
        if (lastSlash != std::wstring::npos) {
            std::wstring parentDir = outPath.substr(0, lastSlash);
            if (!ensure_directory(parentDir)) {
                log_err(L"Failed to create parent dir for: " + wFileName);
                continue;
            }
        }

        log_msg(L"Extracting: " + wFileName + L" (" +
                std::to_wstring(fileInfo.uncompressed_size) + L" bytes)");

        // Open current file
        err = unzOpenCurrentFile(uf);
        if (err != UNZ_OK) {
            log_err(L"Failed to open file in zip: " + wFileName);
            continue;
        }

        // Create output file (robust: handles locked exe/dll)
        HANDLE hFile = create_output_file_robust(outPath);
        if (hFile == INVALID_HANDLE_VALUE) {
            log_err(L"Failed to create output file (after retries): " + wFileName);
            failedFiles.push_back(wFileName);
            unzCloseCurrentFile(uf);
            continue;
        }

        // Read and write
        DWORD totalWritten = 0;
        int readErr = UNZ_OK;
        while (true) {
            int bytesRead = unzReadCurrentFile(uf, readBuf, BUF_SIZE);
            if (bytesRead < 0) {
                log_err(L"Error reading from zip: " + wFileName);
                readErr = bytesRead;
                break;
            }
            if (bytesRead == 0) break;

            DWORD written;
            if (!WriteFile(hFile, readBuf, bytesRead, &written, nullptr) ||
                written != (DWORD)bytesRead) {
                log_err(L"Error writing file: " + wFileName);
                readErr = -1;
                break;
            }
            totalWritten += written;
        }

        CloseHandle(hFile);

        // Set file time if available
        if (readErr == UNZ_OK && fileInfo.dosDate != 0) {
            FILETIME ftLocal, ftUTC;
            WORD dosDate = (WORD)(fileInfo.dosDate >> 16);
            WORD dosTime = (WORD)(fileInfo.dosDate & 0xFFFF);
            if (DosDateTimeToFileTime(dosDate, dosTime, &ftLocal)) {
                LocalFileTimeToFileTime(&ftLocal, &ftUTC);
                HANDLE hTimeFile = CreateFileW(outPath.c_str(), FILE_WRITE_ATTRIBUTES,
                                               FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
                if (hTimeFile != INVALID_HANDLE_VALUE) {
                    SetFileTime(hTimeFile, nullptr, nullptr, &ftUTC);
                    CloseHandle(hTimeFile);
                }
            }
        }

        int closeErr = unzCloseCurrentFile(uf);
        if (closeErr == UNZ_CRCERROR) {
            log_err(L"CRC error in file: " + wFileName);
            // Continue anyway - some tools may produce slightly different CRC
        }

        if (readErr == UNZ_OK) {
            extracted++;
        }

    } while (unzGoToNextFile(uf) == UNZ_OK);

    delete[] readBuf;
    unzClose(uf);

    log_msg(L"Extraction complete: " + std::to_wstring(extracted) +
            L" files extracted, " + std::to_wstring(failedFiles.size()) +
            L" failed");
    for (const auto& fn : failedFiles) {
        log_err(L"  FAILED: " + fn);
    }
    
    return extracted > 0 && failedFiles.empty();
}

// ─── Write files ──────────────────────────────────────

static void write_version_file(const std::wstring& app_dir,
                                const std::string& version) {
    std::wstring versionFile = app_dir + L"\\.version.json";
    std::string ver = version;
    if (!ver.empty() && ver[0] != 'V' && ver[0] != 'v') {
        ver = "V" + ver;
    }

    json obj = {
        {"latest_ver", ver},
        {"updated_at", (long long)time(nullptr)}
    };

    std::ofstream ofs(versionFile.c_str());
    if (ofs.is_open()) {
        ofs << obj.dump();
        ofs.close();
        log_msg(L"Wrote version file: " + versionFile);
    }
}

static void write_update_history(const std::wstring& app_dir,
                                  const std::string& version,
                                  const std::string& update_log) {
    std::wstring historyFile = app_dir + L"\\update_history.log";

    time_t now = time(nullptr);
    struct tm tm_buf;
    localtime_s(&tm_buf, &now);
    char dateStr[64];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", &tm_buf);

    json record = {
        {"version", std::string("V") + version},
        {"update_log", update_log},
        {"update_type", "full"},
        {"updated_at", dateStr}
    };

    std::ofstream ofs(historyFile.c_str(), std::ios::app);
    if (ofs.is_open()) {
        ofs << record.dump() << "\n";
        ofs.close();
        log_msg(L"Wrote update history");
    }
}

static void write_result_file(const std::wstring& result_file,
                               bool ok, const std::string& msg,
                               const std::string& update_type = "full") {
    json result = {
        {"ok", ok},
        {"msg", msg},
        {"update_type", update_type}
    };
    std::ofstream ofs(result_file.c_str());
    if (ofs.is_open()) {
        ofs << result.dump();
        ofs.close();
    }
    log_msg(L"Result written: ok=" + std::wstring(ok ? L"true" : L"false") + L", " +
            utf8_to_wstring(msg));
}

static void write_done_flag(const std::wstring& app_dir) {
    std::wstring flagFile = app_dir + L"\\_update_done.flag";
    HANDLE hFile = CreateFileW(flagFile.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        const char* content = "1";
        WriteFile(hFile, content, 1, &written, nullptr);
        CloseHandle(hFile);
    }
}

// ─── Restart app ──────────────────────────────────────

// Reliable diagnostic log using _wfopen (handles Chinese paths correctly)
static void diag_log(const std::wstring& msg) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    size_t pos = dir.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return;
    std::wstring logPath = dir.substr(0, pos + 1) + L"updater_diag.log";
    FILE* f = _wfopen(logPath.c_str(), L"a, ccs=UTF-8");
    if (f) {
        fwprintf(f, L"%ls\n", msg.c_str());
        fclose(f);
    }
}
static void restart_app(const std::wstring& app_dir) {
    diag_log(L"restart_app: ENTERED");
    diag_log(L"restart_app: app_dir=" + app_dir);
    std::wstring exePath = app_dir + L"\\" + APP_EXE_NAME;
    diag_log(L"restart_app: exePath=" + exePath);
    if (!file_exists(exePath)) {
        diag_log(L"restart_app: FAIL exe not found");
        return;
    }
    diag_log(L"restart_app: launching via ShellExecuteExW");

    // Use ShellExecuteExW instead of CreateProcessW
    // This creates a process through the shell, completely independent of the updater process
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpFile = exePath.c_str();
    sei.lpParameters = L"--restart";
    sei.lpDirectory = app_dir.c_str();
    sei.nShow = SW_SHOWNORMAL;

    BOOL ok = ShellExecuteExW(&sei);
    wchar_t errBuf[64];
    if (ok) {
        swprintf(errBuf, 64, L"restart_app: ShellExecuteExW OK");
        diag_log(errBuf);
        // Keep the process alive briefly to let SnipOCR start
        Sleep(1000);
        CloseHandle(sei.hProcess);
    } else {
        swprintf(errBuf, 64, L"restart_app: ShellExecuteExW FAILED GLE=%d", GetLastError());
        diag_log(errBuf);
    }
}

// ─── Install command ──────────────────────────────────

static int cmd_install(const std::wstring& info_file_path) {
    log_msg(L"===== Starting install (minizip) =====");

    // Read info file
    json info;
    try {
        std::ifstream ifs(info_file_path);
        if (!ifs.is_open()) {
            log_err(L"Cannot open info file: " + info_file_path);
            return 1;
        }
        info = json::parse(ifs);
    } catch (const std::exception& e) {
        log_err(L"JSON parse error");
        return 1;
    }

    std::string pkgPathStr = info["pkg_path"].get<std::string>();
    std::string appDirStr = info["app_dir"].get<std::string>();
    std::string versionStr = info.value("new_ver", "");
    std::string resultFileStr = info["result_file"].get<std::string>();
    std::string updateLogStr = info.value("update_log", "");
    std::string updateTypeStr = info.value("update_type", "full");

    // Strip V/v prefix from version
    if (!versionStr.empty() && (versionStr[0] == 'V' || versionStr[0] == 'v')) {
        versionStr = versionStr.substr(1);
    }

    std::wstring pkgPath = utf8_to_wstring(pkgPathStr);
    std::wstring appDir = utf8_to_wstring(appDirStr);
    std::wstring resultFile = utf8_to_wstring(resultFileStr);

    log_msg(L"App dir: " + appDir);
    log_msg(L"Package: " + pkgPath);
    log_msg(L"Version: " + utf8_to_wstring(versionStr));
    log_msg(L"Update type: " + utf8_to_wstring(updateTypeStr));

    // Step 1: Kill main process
    diag_log(L"Step 1: Killing main process");
        log_msg(L"Step 1: Killing main process");
    kill_main_exe(appDir);

    // Step 2: Backup user state files (full update only)
    log_msg(L"Step 2: Backing up user state files");
    std::wstring backupDir = get_temp_backup_dir(appDir);
    if (updateTypeStr == "full") {
        backup_user_state_files(appDir, backupDir);
    } else {
        // For diff updates, only backup .version.json for safety
        std::wstring verFile = appDir + L"\\.version.json";
        std::wstring verBak = backupDir + L"\\.version.json";
        if (file_exists(verFile)) {
            CopyFileW(verFile.c_str(), verBak.c_str(), FALSE);
        }
    }

    // Step 3: Extract zip
    log_msg(L"Step 3: Extracting update package");
    if (!file_exists(pkgPath)) {
        log_err(L"Package file not found: " + pkgPath);
        write_result_file(resultFile, false, "Package file not found", updateTypeStr);
        if (updateTypeStr == "full") {
            restore_user_state_files(backupDir, appDir);
        }
        cleanup_backup_dir(backupDir);
        return 1;
    }

    std::vector<std::wstring> failedFiles;
    bool extracted = extract_zip(pkgPath, appDir, failedFiles);
    if (!extracted) {
        log_err(L"Extraction failed or incomplete");
        write_result_file(resultFile, false, "Extraction failed", updateTypeStr);
        if (updateTypeStr == "full") {
            restore_user_state_files(backupDir, appDir);
        }
        cleanup_backup_dir(backupDir);
        return 1;
    }

    // Step 4: Restore user state files (full update only)
    log_msg(L"Step 4: Restoring user state files");
    if (updateTypeStr == "full") {
        restore_user_state_files(backupDir, appDir);
    }
    cleanup_backup_dir(backupDir);

    // Step 5: Write version file
    diag_log(L"Step 5: Writing version file");
        log_msg(L"Step 5: Writing version file");
    write_version_file(appDir, versionStr);

    // Step 6: Write done flag
    log_msg(L"Step 6: Writing done flag");
    write_done_flag(appDir);

    // Step 7: Write update history
    log_msg(L"Step 7: Writing update history");
    write_update_history(appDir, versionStr, updateLogStr);

    // Step 8: Write result and restart
    diag_log(L"Step 8: Writing result and restarting");
        log_msg(L"Step 8: Writing result and restarting");
    std::string successMsg = "Updated to V" + versionStr;
    write_result_file(resultFile, true, successMsg, updateTypeStr);

    diag_log(L"cmd_install: before Sleep(500)");
    Sleep(1500);
    diag_log(L"cmd_install: after Sleep, calling restart_app");
    restart_app(appDir);
    diag_log(L"cmd_install: restart_app returned");

    // Clean up info file
    DeleteFileW(info_file_path.c_str());

    log_msg(L"===== Install complete =====");
    return 0;
}

// ─── Restart command ──────────────────────────────────

static int cmd_restart(const std::wstring& app_dir) {
    restart_app(app_dir);
    return 0;
}

// ─── Entry point ──────────────────────────────────────

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    int result = 0;

    if (argc < 2) {
        log_err(L"Usage: updater.exe <command> [args...]");
        result = 1;
    } else {
        std::wstring cmd = argv[1];

        if (cmd == L"install") {
            // Check for --info-json flag
            if (argc >= 4) {
                std::wstring flag = argv[2];
                if (flag == L"--info-json") {
                    std::wstring infoFile = argv[3];
                    result = cmd_install(infoFile);
                } else {
                    log_err(L"Unknown flag: " + flag);
                    result = 1;
                }
            } else {
                log_err(L"Missing arguments. Usage: install --info-json <info.json>");
                result = 1;
            }
        } else if (cmd == L"restart") {
            if (argc >= 3) {
                result = cmd_restart(argv[2]);
            } else {
                log_err(L"Missing arguments. Usage: restart <app_dir>");
                result = 1;
            }
        } else {
            log_err(L"Unknown command: " + cmd);
            result = 1;
        }
    }

    if (argv) LocalFree(argv);
    return result;
}
