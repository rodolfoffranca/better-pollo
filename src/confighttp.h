/**
 * @file src/confighttp.h
 * @brief Declarations for the Web UI Config HTTP server.
 */
#pragma once

// standard includes
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

// third-party includes
#include <nlohmann/json.hpp>
#include <Simple-Web-Server/server_https.hpp>

// local includes
#include "thread_safe.h"

#define WEB_DIR SUNSHINE_ASSETS_DIR "/web/"

using namespace std::chrono_literals;

namespace confighttp {
  using resp_https_t = std::shared_ptr<typename SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response>;
  using req_https_t = std::shared_ptr<typename SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request>;

  constexpr auto PORT_HTTPS = 1;
  constexpr auto SESSION_EXPIRE_DURATION = 24h * 15;
  void start();

  bool authenticate(resp_https_t response, req_https_t request, bool needsRedirect = false);
  bool validateContentType(resp_https_t response, req_https_t request, const std::string_view &contentType);
  inline bool check_content_type(resp_https_t response, req_https_t request, const std::string_view &contentType) {
    return validateContentType(response, request, contentType);
  }
  void send_response(resp_https_t response, const nlohmann::json &output_tree);
  void bad_request(resp_https_t response, req_https_t request, const std::string &error_message = "Bad Request");

  std::recursive_mutex &apps_file_mutex();
  bool refresh_client_apps_cache(nlohmann::json &file_tree, bool sort_by_name = true);

  // Steam API endpoints
  void getSteamStatus(resp_https_t response, req_https_t request);
  void getSteamGames(resp_https_t response, req_https_t request);
  void postSteamForceSync(resp_https_t response, req_https_t request);
  void postSteamLaunch(resp_https_t response, req_https_t request);
  void getBattleNetStatus(resp_https_t response, req_https_t request);
  void postBattleNetSync(resp_https_t response, req_https_t request);
}  // namespace confighttp

// mime types map
const std::map<std::string, std::string> mime_types = {
  {"css", "text/css"},
  {"gif", "image/gif"},
  {"htm", "text/html"},
  {"html", "text/html"},
  {"ico", "image/x-icon"},
  {"jpeg", "image/jpeg"},
  {"jpg", "image/jpeg"},
  {"js", "application/javascript"},
  {"json", "application/json"},
  {"png", "image/png"},
  {"svg", "image/svg+xml"},
  {"ttf", "font/ttf"},
  {"txt", "text/plain"},
  {"woff2", "font/woff2"},
  {"xml", "text/xml"},
};
