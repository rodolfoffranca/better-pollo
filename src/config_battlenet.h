#pragma once
#include <string>
#include <unordered_map>

namespace config {
  struct battlenet_t {
    bool enabled = false;
    bool auto_sync = false;
    bool start_minimized = true;
    bool remove_uninstalled = true;
    int launch_timeout = 60;
  };
  extern battlenet_t battlenet;
  battlenet_t parse_battlenet(std::unordered_map<std::string, std::string> &vars);
  void apply_battlenet(std::unordered_map<std::string, std::string> &vars);
}
namespace platf::battlenet::autosync {
  void configure(config::battlenet_t settings);
  void start();
  void stop();
}
