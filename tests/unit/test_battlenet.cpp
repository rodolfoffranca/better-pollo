#include <gtest/gtest.h>
#include "src/battlenet.h"
#include "src/config_battlenet.h"
#include "src/steam_process_tracker.h"

#include <fstream>

namespace {
  namespace bnet = platf::battlenet;
  using namespace std::chrono_literals;

  std::string varint(std::uint64_t value) {
    std::string bytes;
    do { bytes += static_cast<char>((value & 127) | (value >= 128 ? 128 : 0)); value >>= 7; } while (value);
    return bytes;
  }
  std::string field(unsigned number, const std::string &bytes) {
    return varint((number << 3) | 2) + varint(bytes.size()) + bytes;
  }
  std::string database(const std::string &uid, const std::string &path) {
    return field(1, field(1, uid) + field(2, "WTCG") + field(3, field(1, path)));
  }

  class BattleNetCatalog: public testing::Test {
  protected:
    void SetUp() override {
      folder = std::filesystem::temp_directory_path() /
               ("better-pollo-bnet-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
      std::filesystem::create_directories(folder);
      std::ofstream(folder / "Hearthstone.exe").put('x');
    }
    void TearDown() override { std::error_code error; std::filesystem::remove_all(folder, error); }
    bnet::catalog source() {
      return {folder / "Battle.net.exe", bnet::installed_games({{"hs_beta", "WTCG", folder}}), true, {}};
    }
    std::filesystem::path folder;
  };

  TEST_F(BattleNetCatalog, ReadsRealDatabaseWireLayoutAndIgnoresUnknownFields) {
    const auto encoded = database("hs_beta", folder.generic_string()) + field(10, "future metadata") + varint(16) + varint(1);
    const auto records = bnet::parse_product_db(encoded);
    ASSERT_TRUE(records);
    ASSERT_EQ(records->size(), 1);
    EXPECT_EQ(records->front().uid, "hs_beta");
    EXPECT_EQ(records->front().path, folder);
    const auto games = bnet::installed_games(*records);
    ASSERT_EQ(games.size(), 1);
    EXPECT_EQ(games.front().name, "Hearthstone");
  }

  TEST(BattleNetDatabase, RejectsTruncationInvalidWireTypesAndEmptyFiles) {
    EXPECT_FALSE(bnet::parse_product_db(""));
    EXPECT_FALSE(bnet::parse_product_db(std::string("\x0a\xff", 2)));
    EXPECT_FALSE(bnet::parse_product_db(std::string("\x0b", 1)));
    EXPECT_FALSE(bnet::parse_product_db(std::string("\0", 1)));
    auto bytes = database("hs_beta", "C:/Games/Hearthstone");
    bytes.pop_back();
    EXPECT_FALSE(bnet::parse_product_db(bytes));
    EXPECT_FALSE(bnet::parse_product_db(std::string(12, static_cast<char>(0xff))));
  }

  TEST_F(BattleNetCatalog, ImportsOnlyInstalledKnownGamesAndDeduplicatesSources) {
    const auto games = bnet::installed_games({{"hs_beta", {}, folder}, {"hsb", {}, folder},
      {"hs_beta", {}, folder / "missing"}, {"hs_beta", {}, "relative/path"},
      {"hs_beta & calc", {}, folder}, {"battle.net", {}, folder}});
    ASSERT_EQ(games.size(), 1);
    EXPECT_EQ(games.front().id, "hs_beta");
    EXPECT_TRUE(bnet::installed_games({{"wow", {}, folder}}).empty());
  }

  TEST_F(BattleNetCatalog, ReconcileIsStableAndPreservesOtherProvidersAndCovers) {
    nlohmann::json root {{"version", 2}, {"apps", nlohmann::json::array({
      {{"name", "Steam game"}, {"steam-id", "42"}}, {{"name", "Manual app"}, {"cmd", "custom.exe"}}})}};
    ASSERT_TRUE(bnet::reconcile(root, source(), true));
    ASSERT_EQ(root["apps"].size(), 3);
    const auto uuid = root["apps"][2]["uuid"];
    root["apps"][2]["image-path"] = "custom-cover.png";
    root["apps"][2]["virtual-display"] = true;
    EXPECT_FALSE(bnet::reconcile(root, source(), true));
    EXPECT_EQ(root["apps"][2]["uuid"], uuid);
    EXPECT_EQ(root["apps"][2]["image-path"], "custom-cover.png");
    EXPECT_EQ(root["apps"][2]["virtual-display"], true);
    EXPECT_EQ(root["apps"][0]["steam-id"], "42");
    EXPECT_EQ(root["apps"][1]["cmd"], "custom.exe");
  }

  TEST_F(BattleNetCatalog, IncompleteScansNeverRemoveGames) {
    nlohmann::json root {{"apps", nlohmann::json::array()}};
    ASSERT_TRUE(bnet::reconcile(root, source(), true));
    auto missing = source();
    missing.games.clear();
    missing.complete = false;
    EXPECT_FALSE(bnet::reconcile(root, missing, true));
    EXPECT_EQ(root["apps"].size(), 1);
    missing.complete = true;
    EXPECT_FALSE(bnet::reconcile(root, missing, false));
    EXPECT_TRUE(bnet::reconcile(root, missing, true));
    EXPECT_TRUE(root["apps"].empty());
  }

  TEST_F(BattleNetCatalog, MissingLauncherNeverChangesCatalogAndManualEntriesStayManual) {
    nlohmann::json root {{"apps", nlohmann::json::array({
      {{"name", "My Hearthstone"}, {"battlenet-id", "hs_beta"}, {"cmd", "custom.exe"}}})}};
    EXPECT_FALSE(bnet::reconcile(root, source(), true));
    EXPECT_EQ(root["apps"].size(), 1);
    auto no_launcher = source();
    no_launcher.launcher.clear();
    EXPECT_FALSE(bnet::reconcile(root, no_launcher, true));
    EXPECT_EQ(root["apps"][0]["cmd"], "custom.exe");
  }

  TEST(BattleNetCommands, OnlyKnownProductsCanBePassedToTheLauncherOrHelper) {
    EXPECT_EQ(bnet::client_command("C:/Program Files/Battle.net/Battle.net.exe", "hs_beta"),
              "\"C:/Program Files/Battle.net/Battle.net.exe\" --exec=\"launch WTCG\"");
    EXPECT_TRUE(bnet::client_command("client.exe", "hs_beta & calc").empty());
    EXPECT_TRUE(bnet::client_command("bad\"path", "hs_beta").empty());
    EXPECT_TRUE(bnet::helper_command("host.exe", "unknown", 60, true).empty());
    EXPECT_TRUE(bnet::helper_command("host.exe", "hs_beta", 0, true).empty());
    EXPECT_EQ(bnet::helper_command("C:/Host/sunshine.exe", "hs_beta", 60, true),
              "\"C:/Host/sunshine.exe\" --battlenet-launch hs_beta 60 1");
  }

  TEST(BattleNetConfig, DefaultsAndTimeoutValidation) {
    std::unordered_map<std::string, std::string> vars;
    auto settings = config::parse_battlenet(vars);
    EXPECT_FALSE(settings.enabled);
    EXPECT_FALSE(settings.auto_sync);
    EXPECT_TRUE(settings.start_minimized);
    EXPECT_EQ(settings.launch_timeout, 60);
    vars = {{"battlenet_enabled", "enabled"}, {"battlenet_start_minimized", "false"}, {"battlenet_launch_timeout", "90"}};
    settings = config::parse_battlenet(vars);
    EXPECT_TRUE(settings.enabled);
    EXPECT_FALSE(settings.start_minimized);
    EXPECT_EQ(settings.launch_timeout, 90);
    EXPECT_TRUE(vars.empty());
    for (const auto value : {"14", "181", "-1", "60junk"}) {
      vars = {{"battlenet_launch_timeout", value}};
      EXPECT_EQ(config::parse_battlenet(vars).launch_timeout, 60);
    }
  }

  class simulated_launcher: public bnet::launch_controller {
  public:
    bool launcher_running() override { return warm || starts > 0; }
    bool launcher_ready() override { return !never_ready && time >= ready_at; }
    bool game_running() override { return (already_running || requests > 0) && !never_game && time >= game_at && time < game_ends; }
    bool start_launcher(bool minimized) override { ++starts; minimized_start = minimized; return start_ok; }
    bool launch_game() override { ++requests; return request_ok; }
    void minimize_launcher() override { ++minimizes; }
    std::chrono::milliseconds now() const override { return time; }
    void sleep(std::chrono::milliseconds duration) override { time += duration; }
    std::chrono::milliseconds time {0}, ready_at {1000}, game_at {3000}, game_ends {5000};
    int starts = 0, requests = 0, minimizes = 0;
    bool warm = false, already_running = false, never_ready = false, never_game = false;
    bool start_ok = true, request_ok = true, minimized_start = false;
  };

  TEST(BattleNetLaunch, ColdStartWaitsForClientThenLaunchesAndTracksGame) {
    simulated_launcher client;
    EXPECT_EQ(bnet::run_launch(client, 60, true), 0);
    EXPECT_EQ(client.starts, 1);
    EXPECT_EQ(client.requests, 1);
    EXPECT_TRUE(client.minimized_start);
    EXPECT_EQ(client.minimizes, 1);
    EXPECT_GE(client.time, client.game_ends);
  }
  TEST(BattleNetLaunch, WarmClientIsReusedWithoutMinimizingItsExistingWindow) {
    simulated_launcher client;
    client.warm = true;
    client.ready_at = 0ms;
    EXPECT_EQ(bnet::run_launch(client, 60, true), 0);
    EXPECT_EQ(client.starts, 0);
    EXPECT_EQ(client.requests, 1);
    EXPECT_EQ(client.minimizes, 0);
  }
  TEST(BattleNetLaunch, ExistingGameIsMonitoredWithoutStartingAnotherInstance) {
    simulated_launcher client;
    client.already_running = true;
    client.game_at = 0ms;
    EXPECT_EQ(bnet::run_launch(client, 60, true), 0);
    EXPECT_EQ(client.starts, 0);
    EXPECT_EQ(client.requests, 0);
  }
  TEST(BattleNetLaunch, StartupAndLoginTimeoutsAreBounded) {
    simulated_launcher client;
    client.never_ready = true;
    EXPECT_EQ(bnet::run_launch(client, 15, true), 4);
    EXPECT_LE(client.time, 15s);
    simulated_launcher login;
    login.never_game = true;
    EXPECT_EQ(bnet::run_launch(login, 15, true), 6);
    EXPECT_LE(login.time, 15s);
    EXPECT_EQ(login.requests, 2);
  }
  TEST(BattleNetLaunch, StartAndCommandFailuresAreReportedWithoutRetryLoops) {
    simulated_launcher start;
    start.start_ok = false;
    EXPECT_EQ(bnet::run_launch(start, 60, true), 3);
    EXPECT_EQ(start.requests, 0);
    simulated_launcher request;
    request.request_ok = false;
    EXPECT_EQ(bnet::run_launch(request, 60, true), 5);
    EXPECT_EQ(request.requests, 1);
  }
  TEST(BattleNetTracking, LauncherAndAgentAreNeverAssociatedWithTheGame) {
    namespace lifecycle = platf::steam::lifecycle;
    lifecycle::process_info launcher, agent, game;
    launcher.pid = 1; launcher.executable = "C:/Games/Hearthstone/Battle.net.exe";
    agent.pid = 2; agent.executable = "C:/Games/Hearthstone/Agent.exe";
    game.pid = 3; game.executable = "C:/Games/Hearthstone/Hearthstone.exe";
    lifecycle::process_snapshot after;
    after.processes = {{1, launcher}, {2, agent}, {3, game}};
    const auto result = lifecycle::associate({}, after, "C:/Games/Hearthstone");
    ASSERT_TRUE(result.associated());
    ASSERT_EQ(result.tree.processes.size(), 1);
    EXPECT_TRUE(result.tree.processes.contains(3));
  }
}
