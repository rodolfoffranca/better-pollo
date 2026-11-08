#include <gtest/gtest.h>

#include "src/config.h"
#include "src/confighttp.h"
#include "src/process.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {
  class ProcessCatalogTest: public testing::Test {
  protected:
    void SetUp() override {
      file = std::filesystem::temp_directory_path() /
             ("apollo-catalog-" + std::to_string(
               std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
      std::ofstream(file) << R"({"version":2,"apps":[{"name":"Catalog game","uuid":"e2bdd240-6536-4d5e-963b-17f8b202a568"}]})";
    }

    void TearDown() override {
      std::error_code error;
      std::filesystem::remove(file, error);
    }

    std::filesystem::path file;
    proc::proc_t manager;
  };

  TEST_F(ProcessCatalogTest, RefreshPreservesSessionDisplayState) {
    manager.virtual_display = true;
    manager.display_name = "active virtual monitor";
    manager.initial_display = "original monitor";
    manager.mode_changed_display = "HDR monitor";
    manager.initial_hdr = true;
    manager.allow_client_commands = true;

    ASSERT_TRUE(manager.refresh_apps(file.string()));
    EXPECT_TRUE(manager.virtual_display);
    EXPECT_EQ(manager.display_name, "active virtual monitor");
    EXPECT_EQ(manager.initial_display, "original monitor");
    EXPECT_EQ(manager.mode_changed_display, "HDR monitor");
    EXPECT_TRUE(manager.initial_hdr);
    EXPECT_TRUE(manager.allow_client_commands);
    const auto apps = manager.get_apps();
    ASSERT_FALSE(apps.empty());
    EXPECT_EQ(apps.front().name, "Catalog game");
  }

  TEST_F(ProcessCatalogTest, ReadersRetainTheirSnapshotAcrossRefresh) {
    ASSERT_TRUE(manager.refresh_apps(file.string()));
    const auto snapshot = manager.get_apps();
    std::ofstream(file) << R"({"version":2,"apps":[{"name":"Updated game","uuid":"e2bdd240-6536-4d5e-963b-17f8b202a568"}]})";
    ASSERT_TRUE(manager.refresh_apps(file.string()));
    EXPECT_EQ(snapshot.front().name, "Catalog game");
    EXPECT_EQ(manager.get_apps().front().name, "Updated game");
  }

  TEST_F(ProcessCatalogTest, RefreshKeepsRunningInputOnlySession) {
    std::lock_guard lock {proc::mutex()};
    const auto previous_input_id = proc::input_only_app_id;
    const auto previous_output = config::video.output_name;
    proc::input_only_app_id = 42;
    manager.launch_input_only();
    const auto uuid = manager.get_running_app_uuid();
    EXPECT_EQ(manager.running(), 42);
    EXPECT_TRUE(manager.refresh_apps(file.string()));
    EXPECT_EQ(manager.running(), 42);
    EXPECT_EQ(manager.get_running_app_uuid(), uuid);
    manager.terminate(false, false);
    proc::input_only_app_id = previous_input_id;
    config::video.output_name = previous_output;
  }

  TEST_F(ProcessCatalogTest, FailedSaveDoesNotReportSuccessfulSync) {
    std::lock_guard lock {proc::mutex()};
    const auto previous_file = config::stream.file_apps;
    // Opening a directory as an output file must fail on every platform.
    config::stream.file_apps = std::filesystem::temp_directory_path().string();
    nlohmann::json catalog {{"version", 2}, {"apps", nlohmann::json::array()}};
    EXPECT_FALSE(confighttp::refresh_client_apps_cache(catalog));
    config::stream.file_apps = previous_file;
  }

  TEST_F(ProcessCatalogTest, ConcurrentRefreshAndReadersSeeCompleteCatalogs) {
    ASSERT_TRUE(manager.refresh_apps(file.string()));
    std::atomic<bool> failed {false};
    std::thread writer {[&] {
      for (int i = 0; i < 100; ++i) {
        if (!manager.refresh_apps(file.string())) {
          failed = true;
        }
      }
    }};
    for (int i = 0; i < 1000; ++i) {
      const auto snapshot = manager.get_apps();
      if (snapshot.empty() || snapshot.front().name != "Catalog game") {
        failed = true;
      }
    }
    writer.join();
    EXPECT_FALSE(failed.load());
  }
}
