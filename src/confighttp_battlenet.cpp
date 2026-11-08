#include "battlenet.h"
#include "config.h"
#include "confighttp.h"
#include "file_handler.h"

namespace confighttp {
  void getBattleNetStatus(resp_https_t response, req_https_t request) {
    if (!authenticate(response, request)) return;
    try {
      const auto source = platf::battlenet::discover();
      std::lock_guard lock {apps_file_mutex()};
      const auto settings = config::battlenet;
      nlohmann::json games = nlohmann::json::array();
      for (const auto &game : source.games) {
        const auto path = game.install_dir.u8string();
        games.push_back({{"id", game.id}, {"name", game.name}, {"install_dir", std::string(path.begin(), path.end())}});
      }
      send_response(response, nlohmann::json {
        {"status", true}, {"available", !source.launcher.empty()}, {"complete", source.complete},
        {"warning", source.warning}, {"enabled", settings.enabled}, {"auto_sync", settings.auto_sync},
        {"start_minimized", settings.start_minimized}, {"launch_timeout", settings.launch_timeout},
        {"game_count", games.size()}, {"games", std::move(games)}});
    } catch (const std::exception &error) { bad_request(response, request, error.what()); }
  }
  void postBattleNetSync(resp_https_t response, req_https_t request) {
    if (!authenticate(response, request) || !validateContentType(response, request, "application/json")) return;
    try {
      config::battlenet_t settings;
      { std::lock_guard lock {apps_file_mutex()}; settings = config::battlenet; }
      if (!settings.enabled) { bad_request(response, request, "Battle.net integration is disabled"); return; }
      const auto source = platf::battlenet::discover();
      if (source.launcher.empty()) { bad_request(response, request, "Battle.net launcher was not found"); return; }
      std::lock_guard lock {apps_file_mutex()};
      auto root = nlohmann::json::parse(file_handler::read_file(config::stream.file_apps.c_str()));
      const auto changed = platf::battlenet::reconcile(root, source, settings.remove_uninstalled);
      if (changed && !refresh_client_apps_cache(root, false)) { bad_request(response, request, "Unable to save applications"); return; }
      send_response(response, nlohmann::json {{"status", true}, {"changed", changed}, {"complete", source.complete},
                                            {"warning", source.warning}, {"game_count", source.games.size()}});
    } catch (const std::exception &error) { bad_request(response, request, error.what()); }
  }
}
