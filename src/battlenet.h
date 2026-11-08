/** @file src/battlenet.h
 * Native local Battle.net discovery and catalog policy. No external launcher manager.
 */
#pragma once
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

namespace platf::battlenet {
  struct installation {
    std::string uid;
    std::string product;
    std::filesystem::path path;
  };
  struct game {
    std::string id;
    std::string name;
    std::string launch_code;
    std::filesystem::path install_dir;
    std::filesystem::path executable;
  };
  struct catalog {
    std::filesystem::path launcher;
    std::vector<game> games;
    bool complete = true;
    std::string warning;
  };
  // Bounded protobuf reader. A malformed database returns nullopt, never a partial list.
  std::optional<std::vector<installation>> parse_product_db(std::string_view bytes);
  std::vector<game> installed_games(const std::vector<installation> &records);
  catalog discover();
  bool known_id(std::string_view id);
  std::string client_command(const std::filesystem::path &client, std::string_view id);
  std::string helper_command(const std::filesystem::path &host, std::string_view id, int timeout, bool minimized);
  bool reconcile(nlohmann::json &root, const catalog &source, bool remove_uninstalled);

  // Injectable launch lifecycle; production uses Win32, tests use a simulated client.
  class launch_controller {
  public:
    virtual ~launch_controller() = default;
    virtual bool launcher_running() = 0;
    virtual bool launcher_ready() = 0;
    virtual bool game_running() = 0;
    virtual bool start_launcher(bool minimized) = 0;
    virtual bool launch_game() = 0;
    virtual void minimize_launcher() = 0;
    virtual std::chrono::milliseconds now() const {
      return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
    }
    virtual void sleep(std::chrono::milliseconds duration) = 0;
  };
  int run_launch(launch_controller &controller, int timeout_seconds, bool minimized);
  // Invoked only in a separate interactive-user process, before normal host startup.
  int launch_helper(std::string_view id, int timeout_seconds, bool minimized);
}
