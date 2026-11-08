#include <gtest/gtest.h>

#include "src/config_steam.h"
#include "src/steam_resolution.h"

#include <chrono>
#include <fstream>
#include <unordered_map>

namespace {
  namespace resolution = platf::steam::resolution;

  class SteamResolutionTest: public testing::Test {
  protected:
    void SetUp() override {
      folder = std::filesystem::temp_directory_path() /
               ("better-pollo-resolution-" + std::to_string(
                 std::chrono::steady_clock::now().time_since_epoch().count()));
      std::filesystem::create_directory(folder);
    }

    void TearDown() override {
      std::error_code error;
      std::filesystem::remove_all(folder, error);
    }

    void touch(const std::filesystem::path &relative) {
      const auto path = folder / relative;
      std::filesystem::create_directories(path.parent_path());
      std::ofstream(path).put('x');
    }

    std::filesystem::path folder;
  };

  TEST_F(SteamResolutionTest, DetectsUnityAndUsesCurrentSessionDimensions) {
    touch("UnityPlayer.dll");
    EXPECT_EQ(resolution::detect_profile(folder), "unity");
    EXPECT_EQ(resolution::arguments("unity", 2560, 1600), "-screen-width 2560 -screen-height 1600");
    EXPECT_EQ(resolution::arguments("unity", 1280, 720), "-screen-width 1280 -screen-height 720");
  }

  TEST_F(SteamResolutionTest, DetectsLegacyUnity) {
    touch("Game_Data/globalgamemanagers");
    EXPECT_EQ(resolution::detect_profile(folder), "unity");
  }

  TEST_F(SteamResolutionTest, DetectsUnreal) {
    std::filesystem::create_directories(folder / "Engine/Binaries/Win64");
    EXPECT_EQ(resolution::detect_profile(folder), "unreal");
    EXPECT_EQ(resolution::arguments("unreal", 1920, 1080), "-ResX=1920 -ResY=1080 -ForceRes");
  }

  TEST_F(SteamResolutionTest, DetectsSourceAndSource2) {
    touch("game/bin/win64/engine2.dll");
    EXPECT_EQ(resolution::detect_profile(folder), "source");
    EXPECT_EQ(resolution::arguments("source", 3440, 1440), "-w 3440 -h 1440");
  }

  TEST_F(SteamResolutionTest, DetectsSourceFromGameInfo) {
    touch("hl2/gameinfo.txt");
    EXPECT_EQ(resolution::detect_profile(folder), "source");
  }

  TEST_F(SteamResolutionTest, UnknownGamesAndMissingDirectoriesDoNotGuessArguments) {
    EXPECT_TRUE(resolution::detect_profile(folder).empty());
    EXPECT_TRUE(resolution::detect_profile(folder / "missing").empty());
    EXPECT_TRUE(resolution::detect_profile({}).empty());
    EXPECT_TRUE(resolution::arguments("auto", 1920, 1080).empty());
    EXPECT_TRUE(resolution::arguments("unknown", 1920, 1080).empty());
  }

  TEST(SteamResolution, RejectsMissingDimensionsAndInvalidTargets) {
    EXPECT_TRUE(resolution::arguments("unity", 0, 1080).empty());
    EXPECT_TRUE(resolution::arguments("source", 1920, 0).empty());
    for (const auto id : {"", "0", "-1", "4294967296", "999999999999999999999", "42 & calc", "42\n"}) {
      EXPECT_TRUE(resolution::launch_command("C:/Steam/steam.exe", id, "-w 1920 -h 1080").empty());
    }
    EXPECT_TRUE(resolution::launch_command({}, "42", "-w 1920").empty());
    EXPECT_TRUE(resolution::launch_command("bad\"path", "42", "-w 1920").empty());
  }

  TEST(SteamResolution, QuotesSteamPathAndPassesArgumentsWithoutAShell) {
    const auto command = resolution::launch_command("C:/Program Files (x86)/Steam/steam.exe", "892970",
                                                    resolution::arguments("unity", 1920, 1200));
    EXPECT_EQ(command, "\"C:/Program Files (x86)/Steam/steam.exe\" -applaunch 892970 -screen-width 1920 -screen-height 1200");
  }

  TEST(SteamResolutionConfig, DefaultsToDisabledAutomaticDetection) {
    std::unordered_map<std::string, std::string> vars;
    const auto settings = config::parse_steam(vars);
    EXPECT_FALSE(settings.force_resolution);
    EXPECT_EQ(settings.resolution_profile, "auto");
  }

  TEST(SteamResolutionConfig, ParsesGlobalSettingsAndRejectsUnknownProfiles) {
    std::unordered_map<std::string, std::string> vars {
      {"steam_force_resolution", "enabled"}, {"steam_resolution_profile", "unreal"}};
    const auto settings = config::parse_steam(vars);
    EXPECT_TRUE(settings.force_resolution);
    EXPECT_EQ(settings.resolution_profile, "unreal");
    EXPECT_TRUE(vars.empty());
    vars = {{"steam_resolution_profile", "-exec something"}};
    EXPECT_EQ(config::parse_steam(vars).resolution_profile, "auto");
  }
}
