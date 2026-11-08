/** @file src/steam_resolution.h
 * @brief Session resolution arguments for Steam games. Never edits Steam settings.
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace platf::steam::resolution {
  bool valid_profile(std::string_view profile);
  std::string detect_profile(const std::filesystem::path &install_dir);
  std::string arguments(std::string_view profile, std::uint32_t width, std::uint32_t height);
  // Windows command line: pass arguments directly to Steam, without cmd.exe.
  // Empty result means an invalid app ID/path; callers retain the configured command.
  std::string launch_command(const std::filesystem::path &steam_executable,
                             std::string_view app_id, std::string_view arguments);
}
