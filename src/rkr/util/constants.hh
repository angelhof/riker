#pragma once

#include <filesystem>

namespace fs = std::filesystem;

// Namespace to contain global flags that control build behavior
namespace constants {
  /// What is the name of the build state directory?
  //const fs::path OutputDir = ".rkr";
  extern fs::path OutputDir;

  /// What is the name of the build database?
  extern fs::path DatabaseFilename;

  /// What is the name of the new build database?
  extern fs::path NewDatabaseFilename;

  /// Where are cached files saved?
  extern fs::path CacheDir;

  /// Where are cached files saved?
  extern fs::path NewCacheDir;
}
