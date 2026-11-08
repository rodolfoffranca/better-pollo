#include "battlenet.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef _WIN32
  #include "platform/windows/misc.h"
  #include "platform/windows/steam_discovery.h"
#endif

namespace platf::battlenet {
  namespace {
    namespace fs = std::filesystem;
    struct product {
      const char *id;
      const char *alias;
      const char *name;
      const char *code;
      std::vector<fs::path> binaries;
    };
    // Product identifiers are protocol metadata. References: PlayniteExtensions
    // BattleNetGames.cs and Lutris battlenet.py; neither project is a dependency.
    const std::vector<product> products {
      {"wow", "wow", "World of Warcraft", "WoW", {"_retail_/Wow.exe", "Wow.exe"}},
      {"wow_classic", "wow_classic", "World of Warcraft Classic", "WoW_wow_classic", {"_classic_/WowClassic.exe", "WowClassic.exe"}},
      {"wow_classic_era", "wow_classic_era", "World of Warcraft Classic Era", "WoW_wow_classic_era", {"_classic_era_/WowClassic.exe", "WowClassic.exe"}},
      {"diablo3", "d3", "Diablo III", "D3", {"x64/Diablo III64.exe", "Diablo III.exe"}},
      {"fenris", "fen", "Diablo IV", "Fen", {"Diablo IV.exe"}},
      {"osi", "osi", "Diablo II: Resurrected", "OSI", {"D2R.exe"}},
      {"anbs", "anbs", "Diablo Immortal", "ANBS", {"Engine/Binaries/Win64/DiabloImmortal.exe", "DiabloImmortal.exe"}},
      {"prometheus", "pro", "Overwatch 2", "Pro", {"_retail_/Overwatch.exe", "Overwatch.exe"}},
      {"hs_beta", "hsb", "Hearthstone", "WTCG", {"Hearthstone.exe"}},
      {"heroes", "hero", "Heroes of the Storm", "Hero", {"Heroes of the Storm.exe", "Support64/HeroesSwitcher_x64.exe"}},
      {"s1", "s1", "StarCraft", "S1", {"x86_64/StarCraft.exe", "x86/StarCraft.exe", "StarCraft.exe"}},
      {"s2", "s2", "StarCraft II", "S2", {"SC2.exe", "Support64/SC2Switcher_x64.exe"}},
      {"w3", "w3", "Warcraft III: Reforged", "W3", {"_retail_/x86_64/Warcraft III.exe", "x86_64/Warcraft III.exe"}},
      {"w1r", "w1r", "Warcraft I: Remastered", "W1R", {"WarcraftRemastered.exe", "Warcraft I Remastered.exe"}},
      {"w2r", "w2r", "Warcraft II: Remastered", "W2R", {"Warcraft II Remastered.exe", "Warcraft2Remastered.exe"}},
      {"viper", "viper", "Call of Duty: Black Ops 4", "VIPR", {"BlackOps4.exe"}},
      {"odin", "odin", "Call of Duty: Modern Warfare", "ODIN", {"ModernWarfare.exe"}},
      {"zeus", "zeus", "Call of Duty: Black Ops Cold War", "ZEUS", {"BlackOpsColdWar.exe"}},
      {"fore", "fore", "Call of Duty: Vanguard", "FORE", {"Vanguard.exe"}},
      {"auks", "auks", "Call of Duty: Modern Warfare II", "AUKS", {"cod.exe", "_retail_/cod.exe"}},
      {"codhq", "codhq", "Call of Duty HQ", "CODHQ", {"_retail_/cod.exe", "cod.exe"}},
      {"wlby", "wlby", "Crash Bandicoot 4", "WLBY", {"Lava/Binaries/Win64/Lava-Win64-Shipping.exe"}}
    };

    std::string lower(std::string value) {
      std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
      return value;
    }
    const product *find_product(std::string_view id) {
      const auto value = lower(std::string(id));
      for (const auto &p : products) {
        if (value == p.id || value == p.alias || value == lower(p.code)) {
          return &p;
        }
      }
      return nullptr;
    }
    bool file(const fs::path &path) {
      std::error_code error;
      return fs::is_regular_file(path, error);
    }
    std::string utf8(const fs::path &path) {
      const auto text = path.u8string();
      return {text.begin(), text.end()};
    }
    std::string quoted(const fs::path &path) {
      const auto value = utf8(path);
      if (value.empty() || value.find_first_of("\"\r\n") != std::string::npos) {
        return {};
      }
      return "\"" + value + "\"";
    }
    std::uint64_t varint(std::string_view bytes, std::size_t &offset) {
      std::uint64_t value = 0;
      for (unsigned shift = 0; shift < 64; shift += 7) {
        if (offset >= bytes.size()) throw std::runtime_error("truncated varint");
        const auto byte = static_cast<unsigned char>(bytes[offset++]);
        if (shift == 63 && byte > 1) throw std::runtime_error("varint overflow");
        value |= std::uint64_t(byte & 127) << shift;
        if (!(byte & 128)) return value;
      }
      throw std::runtime_error("invalid varint");
    }
    struct field { unsigned number; std::string_view bytes; };
    std::vector<field> fields(std::string_view bytes) {
      std::vector<field> result;
      std::size_t offset = 0;
      while (offset < bytes.size()) {
        const auto key = varint(bytes, offset);
        if (!(key >> 3) || (key >> 3) > 0x1fffffff) throw std::runtime_error("invalid field");
        const auto wire = key & 7;
        std::uint64_t size = 0;
        if (wire == 0) { varint(bytes, offset); continue; }
        if (wire == 1) size = 8;
        else if (wire == 5) size = 4;
        else if (wire == 2) size = varint(bytes, offset);
        else throw std::runtime_error("unsupported wire type");
        if (size > bytes.size() - offset) throw std::runtime_error("truncated field");
        if (wire == 2) result.push_back({static_cast<unsigned>(key >> 3), bytes.substr(offset, size)});
        offset += size;
        if (result.size() > 100000) throw std::runtime_error("too many fields");
      }
      return result;
    }
    std::string_view value(const std::vector<field> &data, unsigned number) {
      for (const auto &item : data) if (item.number == number) return item.bytes;
      return {};
    }

#ifdef _WIN32
    struct registry_key {
      HKEY handle = nullptr;
      ~registry_key() { if (handle) RegCloseKey(handle); }
    };
    std::wstring reg_string(HKEY key, const wchar_t *name) {
      DWORD size = 0;
      if (RegGetValueW(key, nullptr, name, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS || size > 65536) return {};
      std::wstring value(size / sizeof(wchar_t), L'\0');
      if (RegGetValueW(key, nullptr, name, RRF_RT_REG_SZ, nullptr, value.data(), &size) != ERROR_SUCCESS) return {};
      value.resize(wcsnlen(value.c_str(), value.size()));
      return value;
    }
    void uninstall_records(HKEY hive, const std::wstring &prefix, REGSAM view,
                           std::vector<installation> &records, fs::path &launcher) {
      registry_key parent;
      const auto name = prefix + L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
      if (RegOpenKeyExW(hive, name.c_str(), 0, KEY_READ | view, &parent.handle) != ERROR_SUCCESS) return;
      for (DWORD index = 0;; ++index) {
        wchar_t child[1024];
        DWORD size = 1024;
        const auto status = RegEnumKeyExW(parent.handle, index, child, &size, nullptr, nullptr, nullptr, nullptr);
        if (status == ERROR_NO_MORE_ITEMS) break;
        if (status != ERROR_SUCCESS) continue;
        registry_key key;
        if (RegOpenKeyExW(parent.handle, child, 0, KEY_READ | view, &key.handle) != ERROR_SUCCESS) continue;
        const auto command = lower(platf::to_utf8(reg_string(key.handle, L"UninstallString")));
        const auto location = fs::path(reg_string(key.handle, L"InstallLocation"));
        const auto start = command.find("--uid=");
        if (start == std::string::npos || command.find("battle.net") == std::string::npos || location.empty()) continue;
        const auto end = command.find_first_of(" \t\"", start + 6);
        const auto uid = command.substr(start + 6, end == std::string::npos ? end : end - start - 6);
        if (uid == "battle.net" && file(location / "Battle.net.exe")) launcher = location / "Battle.net.exe";
        else if (find_product(uid)) records.push_back({uid, {}, location});
      }
    }
#endif
  }

  bool known_id(std::string_view id) { return find_product(id) != nullptr; }

  std::optional<std::vector<installation>> parse_product_db(std::string_view bytes) {
    if (bytes.empty() || bytes.size() > 64 * 1024 * 1024) return std::nullopt;
    try {
      std::vector<installation> result;
      for (const auto &item : fields(bytes)) {
        if (item.number != 1) continue;
        const auto data = fields(item.bytes);
        const auto install = fields(value(data, 3));
        const auto path = value(install, 1);
        if (path.find('\0') != std::string_view::npos) return std::nullopt;
        const auto chars = reinterpret_cast<const char8_t *>(path.data());
        result.push_back({std::string(value(data, 1)), std::string(value(data, 2)), fs::path(std::u8string(chars, path.size()))});
      }
      return result;
    } catch (const std::exception &) { return std::nullopt; }
  }

  std::vector<game> installed_games(const std::vector<installation> &records) {
    std::map<std::string, game> found;
    for (const auto &record : records) {
      const auto *p = find_product(record.uid);
      if (!p || record.path.empty() || !record.path.is_absolute()) continue;
      for (const auto &binary : p->binaries) {
        if (!file(record.path / binary)) continue;
        found.try_emplace(p->id, game {p->id, p->name, p->code, record.path, record.path / binary});
        break;
      }
    }
    std::vector<game> result;
    for (auto &[id, game] : found) result.push_back(std::move(game));
    return result;
  }

  catalog discover() {
    catalog result;
#ifdef _WIN32
    // Reuse the existing interactive-user scope; never scan SYSTEM's HKCU.
    steam::windows_discovery::discovery_scope scope;
    if (!scope.user_hive()) {
      result.complete = false;
      result.warning = "No interactive Windows user is available; existing Battle.net games will be preserved.";
      return result;
    }
    std::vector<installation> records;
    for (const auto view : {KEY_WOW64_32KEY, KEY_WOW64_64KEY}) {
      uninstall_records(HKEY_LOCAL_MACHINE, L"", view, records, result.launcher);
      if (scope.user_hive()) uninstall_records(HKEY_USERS, *scope.user_hive() + L"\\", view, records, result.launcher);
    }
    if (result.launcher.empty()) {
      for (const auto env : {L"ProgramFiles(x86)", L"ProgramFiles"}) {
        wchar_t path[32768];
        const auto size = GetEnvironmentVariableW(env, path, 32768);
        if (size && size < 32768 && file(fs::path(path) / "Battle.net/Battle.net.exe")) {
          result.launcher = fs::path(path) / "Battle.net/Battle.net.exe";
          break;
        }
      }
    }
    wchar_t data[32768];
    const auto size = GetEnvironmentVariableW(L"ProgramData", data, 32768);
    const auto database = fs::path(size && size < 32768 ? data : L"C:\\ProgramData") / "Battle.net/Agent/product.db";
    std::error_code error;
    if (fs::exists(database, error)) {
      const auto length = fs::file_size(database, error);
      std::ifstream input(database, std::ios::binary);
      std::optional<std::vector<installation>> parsed;
      if (!error && length <= 64 * 1024 * 1024 && input) {
        std::string bytes(static_cast<std::size_t>(length), '\0');
        if (input.read(bytes.data(), bytes.size())) parsed = parse_product_db(bytes);
      }
      if (parsed) records.insert(records.end(), parsed->begin(), parsed->end());
      else { result.complete = false; result.warning = "Battle.net product.db could not be read; existing games will be preserved."; }
    } else if (error) {
      result.complete = false;
      result.warning = "Battle.net installation data is inaccessible; existing games will be preserved.";
    }
    result.games = installed_games(records);
#else
    result.complete = false;
    result.warning = "Battle.net integration is currently supported on Windows only.";
#endif
    return result;
  }

  std::string client_command(const fs::path &client, std::string_view id) {
    const auto *p = find_product(id);
    const auto executable = quoted(client);
    return p && !executable.empty() ? executable + " --exec=\"launch " + p->code + "\"" : std::string {};
  }

  std::string helper_command(const fs::path &host, std::string_view id, int timeout, bool minimized) {
    const auto *p = find_product(id);
    const auto executable = quoted(host);
    if (!p || executable.empty() || timeout < 15 || timeout > 180) return {};
    return executable + " --battlenet-launch " + p->id + " " + std::to_string(timeout) + (minimized ? " 1" : " 0");
  }

  bool reconcile(nlohmann::json &root, const catalog &source, bool remove_uninstalled) {
    if (source.launcher.empty()) return false;
    if (!root.contains("apps") || !root["apps"].is_array()) throw std::runtime_error("Invalid apps catalog");
    const auto before = root;
    auto &apps = root["apps"];
    std::set<std::string> seen;
    std::map<std::string, const game *> games;
    for (const auto &game : source.games) if (known_id(game.id)) games[game.id] = &game;
    auto update = [&](nlohmann::json &entry, const game &game) {
      entry["name"] = game.name;
      entry["battlenet-id"] = game.id;
      entry["battlenet-managed"] = "auto";
      entry["battlenet-install-dir"] = utf8(game.install_dir);
      entry["cmd"] = client_command(source.launcher, game.id);
      entry["auto-detach"] = false;
      entry["wait-all"] = false;
      entry["elevated"] = false;
    };
    for (auto it = apps.begin(); it != apps.end();) {
      if (!it->is_object()) { ++it; continue; }
      if (it->contains("remote-monitor-profile") || it->value("battlenet-managed", std::string {}) != "auto") {
        if (it->contains("battlenet-id") && (*it)["battlenet-id"].is_string()) seen.insert((*it)["battlenet-id"].get<std::string>());
        ++it;
        continue;
      }
      const auto id = it->value("battlenet-id", std::string {});
      if (const auto found = games.find(id); found != games.end()) {
        if (!seen.insert(id).second) { it = apps.erase(it); continue; }
        update(*it, *found->second);
      } else if (source.complete && remove_uninstalled) { it = apps.erase(it); continue; }
      ++it;
    }
    const auto uuid_namespace = boost::uuids::string_generator()("05b98b9b-0b4d-4f96-8792-c873972b8459");
    for (const auto &[id, game] : games) {
      if (seen.contains(id)) continue;
      nlohmann::json entry {{"uuid", boost::uuids::to_string(boost::uuids::name_generator_sha1(uuid_namespace)("battlenet:" + id))},
                            {"image-path", "box.png"}, {"exit-timeout", 5}};
      update(entry, *game);
      apps.push_back(std::move(entry));
    }
    return root != before;
  }

  int run_launch(launch_controller &controller, int timeout_seconds, bool minimized) {
    if (timeout_seconds < 15 || timeout_seconds > 180) return 2;
    const auto deadline = controller.now() + std::chrono::seconds(timeout_seconds);
    const auto tick = std::chrono::milliseconds(250);
    bool started = false;
    if (!controller.game_running()) {
      if (!controller.launcher_running()) {
        if (!controller.start_launcher(minimized)) return 3;
        started = true;
      }
      while (!controller.launcher_ready()) {
        if (controller.now() >= deadline) return 4;
        controller.sleep(std::min(tick, deadline - controller.now()));
      }
      // Let the launcher's IPC handlers settle after its first window appears.
      if (started) {
        if (controller.now() + std::chrono::seconds(1) >= deadline) return 4;
        controller.sleep(std::chrono::milliseconds(1000));
      }
      if (controller.now() >= deadline) return 4;
      if (!controller.launch_game()) return 5;
      bool retried = false;
      const auto requested_at = controller.now();
      while (!controller.game_running()) {
        if (controller.now() >= deadline) return 6;
        controller.sleep(std::min(tick, deadline - controller.now()));
        if (!retried && controller.now() - requested_at >= std::chrono::seconds(10)) {
          // At most one retry: cold launch/login can consume the first IPC request.
          if (!controller.launch_game()) return 5;
          retried = true;
        }
      }
    }
    // Do not minimize login/update dialogs while waiting for the game.
    if (started && minimized) controller.minimize_launcher();
    while (controller.game_running()) controller.sleep(tick);
    return 0;
  }
}
