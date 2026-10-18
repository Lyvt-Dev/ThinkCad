#pragma once
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <functional>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif
namespace thinkcad_update {
namespace fs = std::filesystem;
inline std::string utf8(const fs::path &p) {
    auto s = p.generic_u8string();
    return {reinterpret_cast<const char *>(s.data()), s.size()};
}
struct Transaction {
    fs::path target, backup, failed, journal;
    bool committed = false;
    static Transaction create(const fs::path &target, const fs::path &journal, const std::string &token) {
        if (!target.is_absolute() || target == target.root_path() || fs::is_symlink(target) ||
            !fs::is_directory(target) || token.empty() ||
            token.find_first_not_of("0123456789abcdef-") != std::string::npos)
            throw std::runtime_error("Invalid installation path or transaction identity");
        Transaction t{target,
                      target.parent_path() /
                          (target.filename().native() + fs::path(".thinkcad-backup-" + token).native()),
                      target.parent_path() /
                          (target.filename().native() + fs::path(".thinkcad-failed-" + token).native()),
                      journal};
        if (fs::exists(t.backup) || fs::exists(t.failed))
            throw std::runtime_error("Backup already exists");
        return t;
    }
    void save() {
        auto tmp = journal;
        tmp.concat(".tmp");
        {
            std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
            f << committed << '\n'
              << std::quoted(utf8(target)) << '\n'
              << std::quoted(utf8(backup)) << '\n'
              << std::quoted(utf8(failed)) << '\n';
            f.flush();
            if (!f)
                throw std::runtime_error("Cannot write recovery journal");
        }
#ifdef _WIN32
        HANDLE durable = CreateFileW(tmp.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
        if (durable == INVALID_HANDLE_VALUE)
            throw std::runtime_error("Cannot open recovery journal for flush");
        const bool flushed = FlushFileBuffers(durable) != FALSE;
        CloseHandle(durable);
        if (!flushed)
            throw std::runtime_error("Cannot flush recovery journal to disk");
        if (!MoveFileExW(fs::path(tmp).c_str(), journal.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot commit recovery journal");
#else
        fs::rename(tmp, journal);
#endif
    }
    static Transaction load(const fs::path &journal) {
        Transaction t;
        t.journal = journal;
        std::ifstream f(journal, std::ios::binary);
        std::string a, b, c;
        if (!(f >> t.committed >> std::quoted(a) >> std::quoted(b) >> std::quoted(c)))
            throw std::runtime_error("Invalid recovery journal");
        t.target = fs::path(std::u8string(a.begin(), a.end()));
        t.backup = fs::path(std::u8string(b.begin(), b.end()));
        t.failed = fs::path(std::u8string(c.begin(), c.end()));
        const auto prefix = utf8(t.target.filename());
        if (!t.target.is_absolute() || t.target == t.target.root_path() ||
            t.backup.parent_path() != t.target.parent_path() ||
            t.failed.parent_path() != t.target.parent_path() ||
            !utf8(t.backup.filename()).starts_with(prefix + ".thinkcad-backup-") ||
            !utf8(t.failed.filename()).starts_with(prefix + ".thinkcad-failed-"))
            throw std::runtime_error("Unsafe recovery paths");
        return t;
    }
    void prepare(const std::function<void(std::uintmax_t, std::uintmax_t)> &progress) {
        std::uintmax_t total = 0;
        for (const auto &e : fs::recursive_directory_iterator(target)) {
            if (e.is_symlink() || (!e.is_regular_file() && !e.is_directory()))
                throw std::runtime_error("Unsupported link or special file in installation");
            if (e.is_regular_file())
                total += e.file_size();
        }
        if (fs::space(target.parent_path()).available < total + 64 * 1024 * 1024)
            throw std::runtime_error("Insufficient space for rollback backup");
        save();
        fs::rename(target, backup);
        std::uintmax_t done = 0;
        fs::create_directory(target);
        for (const auto &e : fs::recursive_directory_iterator(backup)) {
            if (e.is_symlink())
                throw std::runtime_error("Installation changed to a link during backup");
            const auto path = target / e.path().lexically_relative(backup);
            if (e.is_directory())
                fs::create_directories(path);
            else {
                std::ifstream in(e.path(), std::ios::binary);
                std::ofstream out(path, std::ios::binary);
                char bytes[65536];
                if (!in || !out)
                    throw std::runtime_error("Cannot clone installation file");
                while (in) {
                    in.read(bytes, sizeof(bytes));
                    auto n = in.gcount();
                    if (n) {
                        out.write(bytes, n);
                        if (!out)
                            throw std::runtime_error("Cannot write installation backup");
                        done += n;
                        progress(done, total);
                    }
                }
                if (!in.eof())
                    throw std::runtime_error("Cannot read installation file");
                out.close();
                if (!out)
                    throw std::runtime_error("Cannot flush installation file");
                fs::permissions(path, e.status().permissions());
                fs::last_write_time(path, e.last_write_time());
            }
        }
    }
    void rollback() {
        if (committed)
            return;
        if (!fs::exists(backup)) {
            if (!fs::exists(target))
                throw std::runtime_error("Both installation and backup are missing");
            fs::remove(journal);
            return;
        }
        if (fs::is_symlink(backup) || fs::is_symlink(target))
            throw std::runtime_error("Recovery paths contain links");
        if (fs::exists(target)) {
            if (fs::exists(failed))
                throw std::runtime_error("Failed-installation path already exists; preserve both copies");
            fs::rename(target, failed);
        }
        fs::rename(backup, target);
        fs::remove(journal);
    }
    void commit() {
        committed = true;
        try {
            save();
        } catch (...) {
            committed = false;
            throw;
        }
    }
};
} // namespace thinkcad_update
