// Deliberately no Qt dependency: this executable is copied to the update cache
// before launch, so it holds no locks on the installation being replaced.
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#include <array>
#include <cwchar>
#include <string>
#include "verify_attestation.hpp"
#ifndef THINKERCAD_PUBLISHER_SHA256
#define THINKERCAD_PUBLISHER_SHA256 ""
#endif
namespace {
int fail(const wchar_t* message) {MessageBoxW(nullptr,message,L"ThinkCad Update",MB_OK|MB_ICONERROR);return 1;}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    int argc=0;auto** argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(!argv || argc!=5){if(argv)LocalFree(argv);return fail(L"Ungültige Update-Argumente.");}
    wchar_t* end=nullptr;const auto pid=wcstoul(argv[1],&end,10);
    if(!pid || *end){LocalFree(argv);return fail(L"Ungültige Prozess-ID.");}
    const std::wstring installer=argv[2],dir=argv[3],commit=argv[4];LocalFree(argv);
    if(dir.find(L'"')!=std::wstring::npos || installer.find(L'"')!=std::wstring::npos)return fail(L"Ungültiger Update-Pfad.");
    HANDLE parent=OpenProcess(SYNCHRONIZE,FALSE,static_cast<DWORD>(pid));
    if(parent) {
        const auto result=WaitForSingleObject(parent,30000);CloseHandle(parent);
        if(result!=WAIT_OBJECT_0)return fail(L"ThinkCad wurde nicht rechtzeitig beendet. Das Update wurde nicht installiert.");
    } else if(GetLastError()!=ERROR_INVALID_PARAMETER) return fail(L"ThinkCad-Prozess konnte nicht geprüft werden.");
    wchar_t module[32768]{};GetModuleFileNameW(nullptr,module,32768);std::wstring helperDir=module;helperDir=helperDir.substr(0,helperDir.find_last_of(L"\\/"));
    if(!thinkercad_update::verify(helperDir+L"\\verifier",installer,commit))return fail(L"Die Update-Signatur ist ungültig. Installation abgebrochen.");
    const std::wstring args=L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS /RESTARTTHINKERCAD=1 /DIR=\""+dir+L"\"";
    SHELLEXECUTEINFOW launch{};launch.cbSize=sizeof(launch);launch.fMask=SEE_MASK_NOCLOSEPROCESS;
    launch.lpFile=installer.c_str();launch.lpParameters=args.c_str();launch.nShow=SW_HIDE;
    if(!ShellExecuteExW(&launch))return fail(L"Der Update-Installer konnte nicht gestartet werden.");
    if(launch.hProcess){WaitForSingleObject(launch.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(launch.hProcess,&code);CloseHandle(launch.hProcess);if(code)return fail(L"Der Installer meldet einen Fehler. Die vorherige Installation ggf. erneut starten.");}
    return 0;
}
