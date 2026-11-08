#include "config_battlenet.h"
#include "battlenet.h"
#include "config.h"
#include "confighttp.h"
#include "file_handler.h"
#include "logging.h"
#include "process.h"

#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <thread>

namespace config {
  battlenet_t battlenet;
  battlenet_t parse_battlenet(std::unordered_map<std::string, std::string> &vars) {
    battlenet_t result;
    auto boolean = [&](const char *key, bool &value) {
      if (const auto found = vars.find(key); found != vars.end()) {
        auto text = found->second;
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
        if (text == "true" || text == "enabled" || text == "1" || text == "yes" || text == "on") value = true;
        else if (text == "false" || text == "disabled" || text == "0" || text == "no" || text == "off") value = false;
        vars.erase(found);
      }
    };
    boolean("battlenet_enabled", result.enabled);
    boolean("battlenet_auto_sync", result.auto_sync);
    boolean("battlenet_start_minimized", result.start_minimized);
    boolean("battlenet_remove_uninstalled", result.remove_uninstalled);
    if (const auto found = vars.find("battlenet_launch_timeout"); found != vars.end()) {
      try {
        std::size_t count;
        const auto value = std::stoi(found->second, &count);
        if (count == found->second.size() && value >= 15 && value <= 180) result.launch_timeout = value;
      } catch (const std::exception &) {}
      vars.erase(found);
    }
    return result;
  }
  void apply_battlenet(std::unordered_map<std::string, std::string> &vars) {
    std::lock_guard lock {proc::mutex()};
    battlenet = parse_battlenet(vars);
    platf::battlenet::autosync::configure(battlenet);
  }
}

namespace platf::battlenet::autosync {
  namespace {
    class manager {
    public:
      void configure(config::battlenet_t settings) {
        { std::lock_guard lock {mutex_}; settings_ = settings; ++epoch_; }
        condition_.notify_all();
      }
      void start() {
        std::lock_guard lock {mutex_};
        if (worker_.joinable()) return;
        stopping_ = false;
        worker_ = std::thread([this] { run(); });
      }
      void stop() {
        { std::lock_guard lock {mutex_}; stopping_ = true; }
        condition_.notify_all();
        if (worker_.joinable()) worker_.join();
      }
    private:
      void run() {
        for (;;) {
          config::battlenet_t settings;
          std::uint64_t epoch;
          {
            std::lock_guard lock {mutex_};
            if (stopping_) return;
            settings = settings_;
            epoch = epoch_;
          }
          if (settings.enabled && settings.auto_sync) {
            try {
              const auto source = discover();
              if (!source.launcher.empty()) {
                std::lock_guard lock {confighttp::apps_file_mutex()};
                auto root = nlohmann::json::parse(file_handler::read_file(config::stream.file_apps.c_str()));
                if (reconcile(root, source, settings.remove_uninstalled) && !confighttp::refresh_client_apps_cache(root, false)) {
                  BOOST_LOG(warning) << "Battle.net auto-sync could not save the applications catalog.";
                }
                if (!source.warning.empty()) BOOST_LOG(warning) << source.warning;
              }
            } catch (const std::exception &error) { BOOST_LOG(warning) << "Battle.net auto-sync failed: " << error.what(); }
          }
          std::unique_lock lock {mutex_};
          condition_.wait_for(lock, std::chrono::seconds(30), [&] { return stopping_ || epoch_ != epoch; });
          if (stopping_) return;
        }
      }
      std::mutex mutex_;
      std::condition_variable condition_;
      config::battlenet_t settings_;
      std::uint64_t epoch_ = 0;
      bool stopping_ = false;
      std::thread worker_;
    };
    manager &instance() { static manager value; return value; }
  }
  void configure(config::battlenet_t settings) { instance().configure(settings); }
  void start() { instance().start(); }
  void stop() { instance().stop(); }
}
