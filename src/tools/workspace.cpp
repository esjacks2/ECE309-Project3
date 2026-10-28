#include "tools/workspace.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <exception>

namespace fs = std::filesystem;

bool Workspace::resolve(std::string_view user_path, fs::path* out,
                        std::string* reason) const {
    if (user_path.empty()) {
        *reason = "empty path";
        return false;
    }
    if (user_path.find('\0') != std::string_view::npos) {
        *reason = "path contains null character";
        return false;
    }
    try {
        fs::path p{std::string(user_path)};
        if (p.is_absolute() || p.has_root_path()) {
            *reason = "absolute path not allowed";
            return false;
        }
        fs::path n = p.lexically_normal();
        if (!n.empty() && *n.begin() == "..") {
            *reason = "path escapes workspace";
            return false;
        }
        *out = (n == ".") ? root() : root() / n;
        return true;
    } catch (const std::exception&) {
        *reason = "invalid path";
        return false;
    }
}
