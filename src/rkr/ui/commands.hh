#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

void do_build(std::vector<std::string> args,
              std::optional<fs::path> stats_log_path,
              std::string command_output,
              fs::path dbDir) noexcept;

void do_audit(std::vector<std::string> args, std::string command_output) noexcept;

void do_check(std::vector<std::string> args, fs::path dbDir) noexcept;

void do_trace(std::vector<std::string> args, std::string output, fs::path dbDir) noexcept;

void do_graph(std::vector<std::string> args,
              std::string output,
              std::string type,
              bool show_all,
              bool no_render,
              fs::path dbDir) noexcept;

void do_stats(std::vector<std::string> args, bool list_artifacts, fs::path dbDir) noexcept;
