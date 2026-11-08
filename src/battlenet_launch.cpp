#include "battlenet.h"
#include "steam_process_tracker.h"

#include <iostream>
#include <algorithm>
#include <cctype>
#include <thread>
#include <set>

#ifdef _WIN32
  #include <windows.h>
  #include "platform/windows/misc.h"
#endif

namespace platf::battlenet {
#ifdef _WIN32
  namespace {
    class windows_launch_controller final: public launch_controller {
    public:
      windows_launch_controller(catalog source, game selected): source_(std::move(source)), game_(std::move(selected)) {
        ProcessIdToSessionId(GetCurrentProcessId(), &session_);
      }
      bool launcher_running() override { return !launcher_pids().empty(); }
      bool launcher_ready() override {
        const auto ids = launcher_pids();
        struct state { const std::set<DWORD> &ids; bool ready = false; } data {ids};
        EnumWindows([](HWND window, LPARAM parameter) -> BOOL {
          auto &data = *reinterpret_cast<state *>(parameter);
          DWORD pid;
          GetWindowThreadProcessId(window, &pid);
          if (data.ids.contains(pid)) { data.ready = true; return FALSE; }
          return TRUE;
        }, reinterpret_cast<LPARAM>(&data));
        return data.ready;
      }
      bool game_running() override {
        const auto snapshot = steam::lifecycle::snapshot_processes();
        if (!snapshot) return last_game_running_;
        for (const auto &[pid, process] : snapshot->processes) {
          DWORD session;
          if (!ProcessIdToSessionId(static_cast<DWORD>(pid), &session) || session != session_) continue;
          // Ignore leftover browsers/crash reporters after the actual game exits.
          auto selected = game_.executable.filename().string();
          std::transform(selected.begin(), selected.end(), selected.begin(), [](unsigned char c) { return std::tolower(c); });
          const bool switcher = selected.find("switcher") != std::string::npos || selected == "sc2.exe";
          std::error_code error;
          const bool direct = std::filesystem::equivalent(process.executable, game_.executable, error) && !error;
          if (!steam::lifecycle::is_protected_steam_process(process) &&
              (direct || switcher) &&
              steam::lifecycle::path_is_within(process.executable, game_.install_dir)) {
            last_game_running_ = true;
            return true;
          }
        }
        if (snapshot->complete) last_game_running_ = false;
        return last_game_running_;
      }
      bool start_launcher(bool minimized) override {
        auto command = quote_client();
        // Respect Battle.net's own "start minimized to tray" preference when set.
        if (minimized) command += " --autostarted";
        return spawn(command, minimized);
      }
      bool launch_game() override { return spawn(client_command(source_.launcher, game_.id), false); }
      void minimize_launcher() override {
        const auto ids = launcher_pids();
        EnumWindows([](HWND window, LPARAM parameter) -> BOOL {
          const auto &ids = *reinterpret_cast<const std::set<DWORD> *>(parameter);
          DWORD pid;
          GetWindowThreadProcessId(window, &pid);
          if (ids.contains(pid) && IsWindowVisible(window)) ShowWindowAsync(window, SW_MINIMIZE);
          return TRUE;
        }, reinterpret_cast<LPARAM>(&ids));
      }
      void sleep(std::chrono::milliseconds duration) override { std::this_thread::sleep_for(duration); }

    private:
      std::string quote_client() const {
        const auto value = source_.launcher.u8string();
        return "\"" + std::string(value.begin(), value.end()) + "\"";
      }
      bool spawn(const std::string &command, bool minimized) {
        if (command.empty()) return false;
        auto text = platf::from_utf8(command);
        STARTUPINFOW startup {};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = minimized ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL;
        PROCESS_INFORMATION process {};
        // The launcher must not belong to the game's job. Cancelling a stream
        // kills only this helper and explicitly tracked game processes.
        if (!CreateProcessW(source_.launcher.c_str(), text.data(), nullptr, nullptr, FALSE,
                            CREATE_BREAKAWAY_FROM_JOB | CREATE_NO_WINDOW, nullptr,
                            source_.launcher.parent_path().c_str(), &startup, &process)) {
          std::cerr << "Battle.net process start failed: " << GetLastError() << '\n';
          return false;
        }
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return true;
      }
      std::set<DWORD> launcher_pids() const {
        std::set<DWORD> ids;
        const auto snapshot = steam::lifecycle::snapshot_processes();
        if (!snapshot) return ids;
        for (const auto &[pid, process] : snapshot->processes) {
          DWORD session;
          std::error_code error;
          if (ProcessIdToSessionId(static_cast<DWORD>(pid), &session) && session == session_ &&
              std::filesystem::equivalent(process.executable, source_.launcher, error) && !error) {
            ids.insert(static_cast<DWORD>(pid));
          }
        }
        return ids;
      }
      catalog source_;
      game game_;
      DWORD session_ = 0;
      bool last_game_running_ = false;
    };
  }
#endif

  int launch_helper(std::string_view id, int timeout_seconds, bool minimized) {
    if (!known_id(id) || timeout_seconds < 15 || timeout_seconds > 180) return 2;
#ifdef _WIN32
    const auto source = discover();
    if (source.launcher.empty()) { std::cerr << "Battle.net is not installed.\n"; return 3; }
    const auto found = std::find_if(source.games.begin(), source.games.end(), [&](const auto &game) { return game.id == id; });
    if (found == source.games.end()) { std::cerr << "Battle.net game is no longer installed.\n"; return 3; }
    windows_launch_controller controller(source, *found);
    const auto result = run_launch(controller, timeout_seconds, minimized);
    if (result) std::cerr << "Battle.net launch failed/timed out (" << result << "). Check login, updates and launcher dialogs.\n";
    return result;
#else
    return 3;
#endif
  }
}
