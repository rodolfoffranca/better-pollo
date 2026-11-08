#include "steam_resolution.h"

#include <algorithm>
#include <limits>

namespace platf::steam::resolution {
  namespace {
    bool file(const std::filesystem::path &path) {
      std::error_code error;
      return std::filesystem::is_regular_file(path, error);
    }

    bool directory(const std::filesystem::path &path) {
      std::error_code error;
      return std::filesystem::is_directory(path, error);
    }
  }

  bool valid_profile(std::string_view profile) {
    return profile == "auto" || profile == "unity" || profile == "unreal" || profile == "source";
  }

  std::string detect_profile(const std::filesystem::path &install_dir) {
    if (install_dir.empty() || !directory(install_dir)) {
      return {};
    }
    if (file(install_dir / "UnityPlayer.dll") || file(install_dir / "UnityPlayer.so")) {
      return "unity";
    }
    if (directory(install_dir / "Engine" / "Binaries")) {
      return "unreal";
    }
    if (file(install_dir / "bin" / "engine.dll") ||
        file(install_dir / "game" / "bin" / "win64" / "engine2.dll") ||
        file(install_dir / "gameinfo.txt") || file(install_dir / "gameinfo.gi")) {
      return "source";
    }
    // Inspect one level only: no recursive scan of potentially large libraries.
    std::error_code error;
    std::filesystem::directory_iterator it(install_dir, error), end;
    for (; !error && it != end; it.increment(error)) {
      const auto &path = it->path();
      if (path.filename().string().ends_with("_Data") &&
          file(path / "globalgamemanagers")) {
        return "unity";
      }
      if (file(path / "gameinfo.txt") || file(path / "gameinfo.gi")) {
        return "source";
      }
    }
    return {};
  }

  std::string arguments(std::string_view profile, std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) {
      return {};
    }
    const auto w = std::to_string(width), h = std::to_string(height);
    if (profile == "unity") {
      return "-screen-width " + w + " -screen-height " + h;
    }
    if (profile == "unreal") {
      return "-ResX=" + w + " -ResY=" + h + " -ForceRes";
    }
    if (profile == "source") {
      return "-w " + w + " -h " + h;
    }
    return {};
  }

  std::string launch_command(const std::filesystem::path &steam_executable,
                             std::string_view app_id, std::string_view args) {
    if (app_id.empty() || app_id.size() > 10 ||
        !std::all_of(app_id.begin(), app_id.end(), [](char c) { return c >= '0' && c <= '9'; })) {
      return {};
    }
    const auto id = std::stoull(std::string(app_id));
    const auto path = steam_executable.string();
    if (id == 0 || id > std::numeric_limits<std::uint32_t>::max() || path.empty() ||
        path.find_first_of("\"\r\n") != std::string::npos || args.empty()) {
      return {};
    }
    return "\"" + path + "\" -applaunch " + std::to_string(id) + " " + std::string(args);
  }
}
