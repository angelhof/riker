#include "constants.hh"
#include <filesystem>
#include "util/options.hh"

namespace fs = std::filesystem;

namespace constants {
   auto OutputDir = options::db_dir;
   auto DatabaseFilename = options::db_dir / "db";
   auto NewDatabaseFilename = options::db_dir / "newdb";
   auto CacheDir = options::db_dir / "cache";
   auto NewCacheDir = options::db_dir / "newcache";
}

