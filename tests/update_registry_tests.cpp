#define NOMINMAX
#include "../packaging/windows/registry_copy.hpp"
#include <algorithm>
#include <iostream>
int main() {
  const auto root = std::wstring(L"Software\\ThinkCadRegistryTests\\") +
                    std::to_wstring(GetCurrentProcessId());
  try {
    thinkcad_update::RegistryKey source, destination, child;
    using thinkcad_update::registryCheck;
    registryCheck(RegCreateKeyExW(HKEY_CURRENT_USER,
                                  (root + L"\\source").c_str(), 0, nullptr, 0,
                                  KEY_ALL_ACCESS, nullptr, &source.value,
                                  nullptr),
                  "Create test source");
    registryCheck(RegCreateKeyExW(HKEY_CURRENT_USER,
                                  (root + L"\\destination").c_str(), 0, nullptr,
                                  0, KEY_READ | KEY_WRITE, nullptr,
                                  &destination.value, nullptr),
                  "Create restricted test destination");
    const wchar_t value[] = L"ThinkCad Ünicode";
    registryCheck(RegSetValueExW(source.value, L"DisplayName", 0, REG_SZ,
                                 reinterpret_cast<const BYTE *>(value),
                                 sizeof(value)),
                  "Write string");
    const BYTE binary[] = {0, 255, 0, 1};
    registryCheck(RegSetValueExW(source.value, L"", 0, REG_BINARY, binary,
                                 sizeof(binary)),
                  "Write default binary");
    registryCheck(RegCreateKeyExW(source.value, L"nested", 0, nullptr, 0,
                                  KEY_READ | KEY_WRITE, nullptr, &child.value,
                                  nullptr),
                  "Create child");
    const DWORD version = 135;
    registryCheck(RegSetValueExW(child.value, L"version", 0, REG_DWORD,
                                 reinterpret_cast<const BYTE *>(&version),
                                 sizeof(version)),
                  "Write nested integer");
    thinkcad_update::copyRegistryData(source.value, destination.value);
    wchar_t actual[64]{};
    DWORD bytes = sizeof(actual), type = 0;
    registryCheck(RegQueryValueExW(destination.value, L"DisplayName", nullptr,
                                   &type, reinterpret_cast<BYTE *>(actual),
                                   &bytes),
                  "Read copied string");
    if (type != REG_SZ || std::wstring(actual) != value)
      throw std::runtime_error("String copy mismatch");
    BYTE copied[4]{};
    bytes = sizeof(copied);
    registryCheck(RegQueryValueExW(destination.value, L"", nullptr, &type,
                                   copied, &bytes),
                  "Read default binary");
    if (type != REG_BINARY || bytes != 4 ||
        !std::equal(std::begin(binary), std::end(binary), std::begin(copied)))
      throw std::runtime_error("Binary copy mismatch");
    DWORD copiedVersion = 0;
    bytes = sizeof(copiedVersion);
    registryCheck(RegGetValueW(destination.value, L"nested", L"version",
                               RRF_RT_REG_DWORD, nullptr, &copiedVersion,
                               &bytes),
                  "Read nested integer");
    if (copiedVersion != version)
      throw std::runtime_error("Nested copy mismatch");
    // Repeat in the recovery direction, using only data access rights.
    thinkcad_update::copyRegistryData(destination.value, source.value);
  } catch (const std::exception &e) {
    RegDeleteTreeW(HKEY_CURRENT_USER, root.c_str());
    std::cerr << e.what() << '\n';
    return 1;
  }
  RegDeleteTreeW(HKEY_CURRENT_USER, root.c_str());
  std::cout << "Uninstall registry backup and recovery passed\n";
}
