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
#ifndef THINKERCAD_PUBLISHER_SHA256
#define THINKERCAD_PUBLISHER_SHA256 ""
#endif
namespace {
bool verify(const wchar_t* path) {
    WINTRUST_FILE_INFO file{};file.cbStruct=sizeof(file);file.pcwszFilePath=path;
    WINTRUST_DATA data{};data.cbStruct=sizeof(data);data.dwUIChoice=WTD_UI_NONE;
    data.fdwRevocationChecks=WTD_REVOKE_WHOLECHAIN;data.dwUnionChoice=WTD_CHOICE_FILE;
    data.pFile=&file;data.dwStateAction=WTD_STATEACTION_VERIFY;
    GUID policy=WINTRUST_ACTION_GENERIC_VERIFY_V2;bool valid=false;
    if(WinVerifyTrust(nullptr,&policy,&data)==ERROR_SUCCESS) {
        auto* provider=WTHelperProvDataFromStateData(data.hWVTStateData);
        auto* signer=provider?WTHelperGetProvSignerFromChain(provider,0,FALSE,0):nullptr;
        if(signer && signer->csCertChain>0) {
            BYTE hash[32]{};DWORD size=sizeof(hash);
            if(CertGetCertificateContextProperty(signer->pasCertChain[0].pCert,CERT_SHA256_HASH_PROP_ID,hash,&size) && size==32) {
                const char hex[]="0123456789abcdef";std::string fingerprint;
                for(auto b:hash){fingerprint+=hex[b>>4];fingerprint+=hex[b&15];}
                std::string expected=THINKERCAD_PUBLISHER_SHA256;
                for(auto& c:expected)if(c>='A'&&c<='F')c=static_cast<char>(c-'A'+'a');
                valid=fingerprint==expected;
            }
        }
    }
    data.dwStateAction=WTD_STATEACTION_CLOSE;WinVerifyTrust(nullptr,&policy,&data);return valid;
}
int fail(const wchar_t* message) {MessageBoxW(nullptr,message,L"ThinkerCad Update",MB_OK|MB_ICONERROR);return 1;}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    int argc=0;auto** argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(!argv || argc!=4){if(argv)LocalFree(argv);return fail(L"Ungültige Update-Argumente.");}
    wchar_t* end=nullptr;const auto pid=wcstoul(argv[1],&end,10);
    if(!pid || *end){LocalFree(argv);return fail(L"Ungültige Prozess-ID.");}
    const std::wstring installer=argv[2],dir=argv[3];LocalFree(argv);
    if(dir.find(L'"')!=std::wstring::npos || installer.find(L'"')!=std::wstring::npos)return fail(L"Ungültiger Update-Pfad.");
    HANDLE parent=OpenProcess(SYNCHRONIZE,FALSE,static_cast<DWORD>(pid));
    if(parent) {
        const auto result=WaitForSingleObject(parent,30000);CloseHandle(parent);
        if(result!=WAIT_OBJECT_0)return fail(L"ThinkerCad wurde nicht rechtzeitig beendet. Das Update wurde nicht installiert.");
    } else if(GetLastError()!=ERROR_INVALID_PARAMETER) return fail(L"ThinkerCad-Prozess konnte nicht geprüft werden.");
    if(!verify(installer.c_str()))return fail(L"Die Update-Signatur ist ungültig. Installation abgebrochen.");
    const std::wstring args=L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS /RESTARTTHINKERCAD=1 /DIR=\""+dir+L"\"";
    SHELLEXECUTEINFOW launch{};launch.cbSize=sizeof(launch);launch.fMask=SEE_MASK_NOCLOSEPROCESS;
    launch.lpFile=installer.c_str();launch.lpParameters=args.c_str();launch.nShow=SW_HIDE;
    if(!ShellExecuteExW(&launch))return fail(L"Der Update-Installer konnte nicht gestartet werden.");
    if(launch.hProcess){WaitForSingleObject(launch.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(launch.hProcess,&code);CloseHandle(launch.hProcess);if(code)return fail(L"Der Installer meldet einen Fehler. Die vorherige Installation ggf. erneut starten.");}
    return 0;
}
