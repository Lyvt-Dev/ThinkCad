#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <vector>
namespace thinkercad_update {
inline bool safe(const std::wstring &text) {
    return !text.empty() && text.find(L'"') == std::wstring::npos && text.find(L'\n') == std::wstring::npos &&
           text.find(L'\r') == std::wstring::npos;
}
inline std::wstring arguments(const std::wstring &verifierDir, const std::wstring &installer,
                              const std::wstring &commit) {
    if (!safe(verifierDir) || !safe(installer) || commit.size() != 40)
        return {};
    for (auto c : commit)
        if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f')))
            return {};
    const auto exe = verifierDir + L"\\gh.exe", roots = verifierDir + L"\\trusted_root.jsonl",
               bundle = installer + L".sigstore.jsonl";
    for (const auto &path : {exe, roots, bundle, installer})
        if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
            return {};
    return L"attestation verify \"" + installer + L"\" --bundle \"" + bundle +
           L"\" --custom-trusted-root \"" + roots +
           L"\" --repo Lyvt-Dev/ThinkCad --signer-workflow Lyvt-Dev/ThinkCad/.github/workflows/windows.yml "
           L"--source-ref refs/heads/main --source-digest " +
           commit + L" --deny-self-hosted-runners";
}
inline bool verify(const std::wstring &verifierDir, const std::wstring &installer,
                   const std::wstring &commit) {
    const auto args = arguments(verifierDir, installer, commit);
    if (args.empty())
        return false;
    const auto exe = verifierDir + L"\\gh.exe";
    std::wstring command = L"\"" + exe + L"\" " + args;
    std::vector<wchar_t> writable(command.begin(), command.end());
    writable.push_back(0);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe.c_str(), writable.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        verifierDir.c_str(), &startup, &process))
        return false;
    const auto wait = WaitForSingleObject(process.hProcess, 60000);
    DWORD code = 1;
    if (wait == WAIT_OBJECT_0)
        GetExitCodeProcess(process.hProcess, &code);
    else
        TerminateProcess(process.hProcess, 1);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return wait == WAIT_OBJECT_0 && code == 0;
}
} // namespace thinkercad_update
