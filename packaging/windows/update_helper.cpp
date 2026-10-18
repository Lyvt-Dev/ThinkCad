// No Qt: this process stays outside the installation and survives its replacement.
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <algorithm>
#include "verify_attestation.hpp"
#include "update_transaction.hpp"
namespace fs = std::filesystem;
namespace {
bool german = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_GERMAN;
std::wstring text(const std::wstring &s) {
    static const std::map<std::wstring, std::wstring> de{
        {L"Cancel", L"Abbrechen"},
        {L"Cancel and restore the previous installation?",
         L"Abbrechen und die vorherige Installation wiederherstellen?"},
        {L"Preparing update...", L"Update vorbereiten…"},
        {L"Recovering interrupted update...", L"Unterbrochenes Update wiederherstellen…"},
        {L"Waiting for ThinkCad to close; projects must already be saved...",
         L"Warte auf das Beenden von ThinkCad. Projekte müssen bereits gespeichert sein…"},
        {L"Rechecking GitHub/Sigstore provenance...", L"GitHub-/Sigstore-Buildnachweis erneut prüfen…"},
        {L"Backing up the old installation and preserving local files...",
         L"Alte Installation sichern und lokale Dateien beibehalten…"},
        {L"Extracting and replacing application files. Projects and settings stay local...",
         L"Programmdateien entpacken und ersetzen. Projekte und Einstellungen bleiben lokal…"},
        {L"Extracting and replacing application files...", L"Programmdateien entpacken und ersetzen…"},
        {L"Applying configuration...", L"Konfiguration übernehmen…"},
        {L"Cleaning update cache...", L"Update-Cache aufräumen…"},
        {L"Checking the new application before committing the update...",
         L"Neue Anwendung vor dem Abschluss prüfen…"},
        {L"Preserving configuration and recording successful installation...",
         L"Konfiguration beibehalten und erfolgreiche Installation protokollieren…"},
        {L"Complete. Backup retained next to the installation.",
         L"Abgeschlossen. Die Sicherung bleibt neben der Installation erhalten."},
        {L"Restoring the previous installation...", L"Vorherige Installation wiederherstellen…"}};
    auto it = de.find(s);
    return german && it != de.end() ? it->second : s;
}
HWND window, status, bar, cancelButton;
bool cancel = false, active = true;
std::ofstream logFile;
fs::path installerProgress;
void pump() {
    MSG m;
    while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
}
LRESULT CALLBACK proc(HWND w, UINT m, WPARAM a, LPARAM b) {
    if (m == WM_COMMAND && LOWORD(a) == 1)
        return proc(w, WM_CLOSE, 0, 0);
    if (m == WM_CLOSE) {
        if (!active || MessageBoxW(w, text(L"Cancel and restore the previous installation?").c_str(),
                                   L"ThinkCad Update", MB_YESNO | MB_ICONQUESTION) == IDYES)
            cancel = true;
        return 0;
    }
    return DefWindowProcW(w, m, a, b);
}
void phase(const std::wstring &name, int percent = -1) {
    static std::wstring last;
    static int lastPercent = -2;
    static ULONGLONG tick = 0;
    if (last == name && percent == lastPercent)
        return;
    if (last == name && GetTickCount64() - tick < 50)
        return;
    last = name;
    lastPercent = percent;
    tick = GetTickCount64();
    const auto label = text(name) + (percent >= 0 ? L" (" + std::to_wstring(percent) + L"%)" : L"");
    SetWindowTextW(status, label.c_str());
    auto style = GetWindowLongPtrW(bar, GWL_STYLE);
    SetWindowLongPtrW(bar, GWL_STYLE, percent < 0 ? (style | PBS_MARQUEE) : (style & ~PBS_MARQUEE));
    SendMessageW(bar, PBM_SETMARQUEE, percent < 0, 40);
    if (percent >= 0)
        SendMessageW(bar, PBM_SETPOS, percent, 0);
    logFile << thinkcad_update::utf8(fs::path(name)) << '\n';
    logFile.flush();
    pump();
}
void checkCancel() {
    pump();
    if (cancel)
        throw std::runtime_error("Cancelled by user");
}
void installerTick() {
    if (installerProgress.empty())
        return;
    std::ifstream f(installerProgress);
    std::string name;
    int value = -1;
    if (f >> name >> value && value >= 0 && value <= 100)
        phase(name == "configuration" ? L"Applying configuration..."
                                      : L"Extracting and replacing application files...",
              value);
}
DWORD run(const std::wstring &exe, const std::wstring &args, DWORD timeout, bool cancellable) {
    std::wstring command = L"\"" + exe + L"\" " + args;
    std::vector<wchar_t> data(command.begin(), command.end());
    data.push_back(0);
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (!job)
        throw std::runtime_error("Cannot create installer job");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        CloseHandle(job);
        throw std::runtime_error("Cannot protect installer process");
    }
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION child{};
    if (!CreateProcessW(exe.c_str(), data.data(), nullptr, nullptr, FALSE,
                        CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child)) {
        CloseHandle(job);
        throw std::runtime_error("Cannot start installer or health check");
    }
    if (!AssignProcessToJobObject(job, child.hProcess)) {
        TerminateProcess(child.hProcess, 1);
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
        CloseHandle(job);
        throw std::runtime_error("Cannot isolate installer process tree");
    }
    ResumeThread(child.hThread);
    CloseHandle(child.hThread);
    const auto begin = GetTickCount64();
    DWORD code = 1;
    while (WaitForSingleObject(child.hProcess, 50) == WAIT_TIMEOUT) {
        pump();
        installerTick();
        if ((cancellable && cancel) || GetTickCount64() - begin > timeout) {
            TerminateJobObject(job, 1);
            WaitForSingleObject(child.hProcess, 10000);
            CloseHandle(child.hProcess);
            CloseHandle(job);
            throw std::runtime_error(cancellable && cancel ? "Installation cancelled; restoring backup"
                                                           : "Installer or health check timed out");
        }
    }
    GetExitCodeProcess(child.hProcess, &code);
    CloseHandle(child.hProcess);
    CloseHandle(job);
    return code;
}
void runOnce(const fs::path &helper, const fs::path &journal, bool enable) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", 0,
                        nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        throw std::runtime_error("Cannot register interrupted-update recovery");
    if (enable) {
        const auto value = L"\"" + helper.wstring() + L"\" --recover \"" + journal.wstring() + L"\"";
        const auto result = RegSetValueExW(key, L"ThinkCadUpdateRecovery", 0, REG_SZ,
                                           reinterpret_cast<const BYTE *>(value.c_str()),
                                           DWORD((value.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
        if (result != ERROR_SUCCESS)
            throw std::runtime_error("Cannot save recovery command");
    } else {
        RegDeleteValueW(key, L"ThinkCadUpdateRecovery");
        RegCloseKey(key);
    }
}
void registry(bool restore, const std::wstring &snapshot) {
    const wchar_t *path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{D184B517-892D-48D8-"
                          L"A537-F54197762D2F}_is1";
    HKEY source = nullptr, dest = nullptr;
    const auto src = restore ? snapshot.c_str() : path;
    const auto dst = restore ? path : snapshot.c_str();
    if (!restore)
        RegDeleteTreeW(HKEY_CURRENT_USER, snapshot.c_str());
    if (RegOpenKeyExW(HKEY_CURRENT_USER, src, 0, KEY_READ | KEY_WOW64_64KEY, &source) != ERROR_SUCCESS)
        return;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, dst, 0, nullptr, 0, KEY_WRITE | KEY_READ | KEY_WOW64_64KEY,
                        nullptr, &dest, nullptr) != ERROR_SUCCESS) {
        RegCloseKey(source);
        throw std::runtime_error("Cannot save uninstall registration");
    }
    const auto result = RegCopyTreeW(source, nullptr, dest);
    RegCloseKey(source);
    RegCloseKey(dest);
    if (result != ERROR_SUCCESS)
        throw std::runtime_error("Cannot copy uninstall registration");
}
} // namespace
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    const auto started = GetTickCount64();
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&controls);
    WNDCLASSW cls{};
    cls.lpfnWndProc = proc;
    cls.hInstance = instance;
    cls.lpszClassName = L"ThinkCadUpdateProgress";
    cls.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&cls);
    window =
        CreateWindowExW(0, cls.lpszClassName, L"ThinkCad Update", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                        CW_USEDEFAULT, CW_USEDEFAULT, 650, 220, nullptr, nullptr, instance, nullptr);
    status = CreateWindowW(L"STATIC", L"Preparing update...", WS_CHILD | WS_VISIBLE, 18, 18, 605, 50, window,
                           nullptr, instance, nullptr);
    bar = CreateWindowW(PROGRESS_CLASSW, nullptr, WS_CHILD | WS_VISIBLE | PBS_MARQUEE, 18, 82, 605, 24,
                        window, nullptr, instance, nullptr);
    cancelButton =
        CreateWindowW(L"BUTTON", text(L"Cancel").c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                      510, 122, 110, 30, window, HMENU(1), instance, nullptr);
    for (auto child : {status, cancelButton})
        SendMessageW(child, WM_SETFONT, WPARAM(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    SendMessageW(bar, PBM_SETRANGE32, 0, 100);
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    int argc = 0;
    auto **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 1;
    if (argc == 7)
        german = std::wstring(argv[6]) == L"de";
    SetWindowTextW(cancelButton, text(L"Cancel").c_str());
    wchar_t module[32768]{};
    GetModuleFileNameW(nullptr, module, 32768);
    const fs::path helper(module), cache = helper.parent_path();
    logFile.open(cache / "installation.log", std::ios::app | std::ios::binary);
    std::optional<thinkcad_update::Transaction> transaction;
    const auto journal = cache / "transaction.journal";
    const auto regSnapshot = L"Software\\Lyvt-Dev\\ThinkCadUpdate\\UninstallBackup";
    HANDLE lock = INVALID_HANDLE_VALUE;
    try {
        if (!logFile)
            throw std::runtime_error("Cannot open local installation log");
        if (argc == 3 && std::wstring(argv[1]) == L"--recover") {
            if (fs::path(argv[2]) != journal)
                throw std::runtime_error("Recovery journal is outside this update session");
            transaction = thinkcad_update::Transaction::load(journal);
            phase(L"Recovering interrupted update...");
            if (!transaction->committed) {
                transaction->rollback();
                registry(true, regSnapshot);
            }
            runOnce(helper, journal, false);
            LocalFree(argv);
            return 0;
        }
        if (argc != 5 && argc != 6 && argc != 7)
            throw std::runtime_error("Invalid updater arguments");
        wchar_t *end = nullptr;
        const auto pid = wcstoul(argv[1], &end, 10);
        if (!pid || *end)
            throw std::runtime_error("Invalid application process ID");
        const fs::path installer(argv[2]), target(argv[3]);
        const std::wstring commit = argv[4], version = argc >= 6 ? argv[5] : L"";
        if (!version.empty()) {
            if (std::count(version.begin(), version.end(), L'.') != 2 || version.front() == L'.' ||
                version.back() == L'.' || version.find(L"..") != std::wstring::npos)
                throw std::runtime_error("Invalid update version");
            for (auto c : version)
                if (c != L'.' && (c < L'0' || c > L'9'))
                    throw std::runtime_error("Invalid update version");
            SetWindowTextW(window, (L"ThinkCad " + version + L" Update").c_str());
        }
        LocalFree(argv);
        argv = nullptr;
        if (!thinkercad_update::safe(target.wstring()) || !thinkercad_update::safe(installer.wstring()))
            throw std::runtime_error("Invalid update paths");
        lock = CreateFileW((target.wstring() + L".update-lock").c_str(), GENERIC_WRITE, 0, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
        if (lock == INVALID_HANDLE_VALUE)
            throw std::runtime_error("Another update is running or installation parent is not writable");
        phase(L"Waiting for ThinkCad to close; projects must already be saved...");
        HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, DWORD(pid));
        if (parent) {
            auto start = GetTickCount64();
            while (WaitForSingleObject(parent, 50) == WAIT_TIMEOUT) {
                checkCancel();
                if (GetTickCount64() - start > 30000) {
                    CloseHandle(parent);
                    throw std::runtime_error("ThinkCad is still running; update was not installed");
                }
            }
            CloseHandle(parent);
        } else if (GetLastError() != ERROR_INVALID_PARAMETER)
            throw std::runtime_error("Cannot verify that ThinkCad stopped");
        phase(L"Rechecking GitHub/Sigstore provenance...");
        const auto verifyArgs =
            thinkercad_update::arguments((cache / "verifier").wstring(), installer.wstring(), commit);
        if (verifyArgs.empty() ||
            run((cache / "verifier" / "gh.exe").wstring(), verifyArgs, 60000, true) != 0)
            throw std::runtime_error("Repository/workflow/branch/commit verification failed");
        checkCancel();
        if (fs::exists(journal)) {
            auto previous = thinkcad_update::Transaction::load(journal);
            if (!previous.committed)
                throw std::runtime_error("Unfinished update exists; run recovery before retrying");
        }
        const auto token = std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64());
        DWORD pendingBytes = 0;
        if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                         L"ThinkCadUpdateRecovery", RRF_RT_REG_SZ, nullptr, nullptr,
                         &pendingBytes) == ERROR_SUCCESS)
            throw std::runtime_error("An interrupted update is pending. Complete its recovery at the next "
                                     "Windows sign-in before starting another update.");
        transaction = thinkcad_update::Transaction::create(target, journal, token);
        transaction->save();
        registry(false, regSnapshot);
        runOnce(helper, journal, true);
        phase(L"Backing up the old installation and preserving local files...", 0);
        transaction->prepare([](auto bytes, auto total) {
            phase(L"Backing up the old installation and preserving local files...",
                  total ? int(bytes * 100 / total) : 100);
            checkCancel();
        });
        phase(L"Extracting and replacing application files. Projects and settings stay local...");
        const auto installLog = cache / "installer.log";
        installerProgress = cache / "installer-progress.txt";
        fs::remove(installerProgress);
        // Inno reports actual progress. A missing status file stays indeterminate.
        const auto args =
            L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /NOCLOSEAPPLICATIONS /NORESTARTAPPLICATIONS /LOG=\"" +
            installLog.wstring() + L"\" /THINKCADPROGRESS=\"" + installerProgress.wstring() + L"\" /DIR=\"" +
            target.wstring() + L"\"";
        checkCancel();
        const auto installResult = run(installer.wstring(), args, 30 * 60 * 1000, true);
        installerProgress.clear();
        if (installResult != 0)
            throw std::runtime_error(
                "Installer returned an error; check installer.log (permissions, antivirus or locked files)");
        phase(L"Checking the new application before committing the update...");
        SetEnvironmentVariableW(L"QT_QPA_PLATFORM", L"offscreen");
        const auto health =
            run((target / L"ThinkCad.exe").wstring(),
                L"--smoke-test" + (version.empty() ? std::wstring() : L" --update-health-check=" + version),
                120000, true);
        SetEnvironmentVariableW(L"QT_QPA_PLATFORM", nullptr);
        if (health != 0)
            throw std::runtime_error("New application health check failed");
        phase(L"Preserving configuration and recording successful installation...");
        const auto receipt = target / L"thinkcad-update-receipt.ini";
        SYSTEMTIME time;
        GetSystemTime(&time);
        char date[64];
        sprintf_s(date, "%04u-%02u-%02uT%02u:%02u:%02uZ", time.wYear, time.wMonth, time.wDay, time.wHour,
                  time.wMinute, time.wSecond);
        {
            std::ofstream f(receipt, std::ios::binary | std::ios::trunc);
            f << "[General]\nversion=" << thinkcad_update::utf8(fs::path(version)) << "\ndate=" << date
              << "\npath=" << std::quoted(thinkcad_update::utf8(target))
              << "\nstatus=success\nseconds=" << (GetTickCount64() - started) / 1000 << '\n';
            f.flush();
            if (!f)
                throw std::runtime_error("Cannot record successful installation");
        }
        checkCancel();
        transaction->commit();
        try {
            runOnce(helper, journal, false);
        } catch (const std::exception &cleanup) {
            logFile << "Cleanup warning: " << cleanup.what() << '\n';
        }
        phase(L"Cleaning update cache...");
        std::error_code cleanupError;
        if (installer.parent_path() == cache) {
            fs::remove(installer, cleanupError);
            if (cleanupError)
                logFile << "Installer cleanup: " << cleanupError.message() << '\n';
            fs::remove(fs::path(installer.wstring() + L".sigstore.jsonl"), cleanupError);
        }
        fs::remove_all(cache / "verifier", cleanupError);
        if (cleanupError)
            logFile << "Verifier cleanup: " << cleanupError.message() << '\n';
        phase(L"Complete. Backup retained next to the installation.", 100);
        active = false;
        ShellExecuteW(nullptr, L"open", (target / L"ThinkCad.exe").c_str(), nullptr, target.c_str(),
                      SW_SHOWNORMAL);
        CloseHandle(lock);
        return 0;
    } catch (const std::exception &e) {
        installerProgress.clear();
        logFile << "ERROR: " << e.what() << '\n';
        logFile.flush();
        std::string result = e.what();
        try {
            if (transaction && !transaction->committed) {
                phase(L"Restoring the previous installation...");
                transaction->rollback();
                registry(true, regSnapshot);
                runOnce(helper, journal, false);
                result += "\nPrevious installation restored. Failed files were retained separately.";
            }
        } catch (const std::exception &rollback) {
            try {
                runOnce(helper, journal, true);
            } catch (...) {
            }
            result += "\nRecovery could not finish: ";
            result += rollback.what();
            result += "\nKeep the backup and transaction.journal; retry recovery after releasing file locks.";
        }
        const std::wstring message =
            (german ? L"Installation fehlgeschlagen. Prüfen Sie Dateisperren, Virenschutz, freien Speicher "
                      L"und Schreibrechte. Details und Wiederherstellungsstatus:\n"
                    : L"Installation failed. Check file locks, antivirus, free space and write permissions. "
                      L"Details and recovery status:\n") +
            std::wstring(result.begin(), result.end());
        MessageBoxW(window, (message + L"\n\nLocal logs: " + cache.wstring()).c_str(), L"ThinkCad Update",
                    MB_OK | MB_ICONERROR);
        if (argv)
            LocalFree(argv);
        if (lock != INVALID_HANDLE_VALUE)
            CloseHandle(lock);
        return 1;
    }
}
