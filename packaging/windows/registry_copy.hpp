#pragma once
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>

namespace thinkcad_update {
inline void registryCheck(LSTATUS code, const char *operation) {
  if (code != ERROR_SUCCESS)
    throw std::runtime_error(std::string(operation) + " (Windows error " +
                             std::to_string(code) + ")");
}
struct RegistryKey {
  HKEY value = nullptr;
  ~RegistryKey() {
    if (value)
      RegCloseKey(value);
  }
  RegistryKey() = default;
  RegistryKey(const RegistryKey &) = delete;
  RegistryKey &operator=(const RegistryKey &) = delete;
};
// Copy data only. RegCopyTree also copies security descriptors, which is
// neither required nor appropriate for a per-user uninstall-registration
// backup.
inline void copyRegistryData(HKEY source, HKEY destination) {
  DWORD maxName = 0, maxData = 0, maxSubkey = 0;
  registryCheck(RegQueryInfoKeyW(source, nullptr, nullptr, nullptr, nullptr,
                                 &maxSubkey, nullptr, nullptr, &maxName,
                                 &maxData, nullptr, nullptr),
                "Inspect uninstall registration");
  std::vector<wchar_t> name(size_t(maxName) + 1);
  std::vector<BYTE> data(size_t(maxData) + 1);
  for (DWORD index = 0;; ++index) {
    DWORD length = DWORD(name.size()), bytes = DWORD(data.size()), type = 0;
    const auto result = RegEnumValueW(source, index, name.data(), &length,
                                      nullptr, &type, data.data(), &bytes);
    if (result == ERROR_NO_MORE_ITEMS)
      break;
    registryCheck(result, "Read uninstall registration value");
    registryCheck(
        RegSetValueExW(destination, name.data(), 0, type, data.data(), bytes),
        "Write uninstall registration value");
  }
  name.resize(size_t(maxSubkey) + 1);
  for (DWORD index = 0;; ++index) {
    DWORD length = DWORD(name.size());
    const auto result = RegEnumKeyExW(source, index, name.data(), &length,
                                      nullptr, nullptr, nullptr, nullptr);
    if (result == ERROR_NO_MORE_ITEMS)
      break;
    registryCheck(result, "Read uninstall registration subkey");
    RegistryKey childSource, childDestination;
    registryCheck(
        RegOpenKeyExW(source, name.data(), 0, KEY_READ, &childSource.value),
        "Open uninstall registration subkey");
    registryCheck(RegCreateKeyExW(destination, name.data(), 0, nullptr, 0,
                                  KEY_READ | KEY_WRITE, nullptr,
                                  &childDestination.value, nullptr),
                  "Create uninstall registration subkey");
    copyRegistryData(childSource.value, childDestination.value);
  }
}
} // namespace thinkcad_update
