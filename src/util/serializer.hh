#pragma once

#include <exception>
#include <memory>
#include <string>

using std::string;
using std::unique_ptr;

class Build;

class db_version_exception : std::exception {
 public:
  db_version_exception(uint32_t version) : _version(version) {}
  uint32_t getVersion() { return _version; }

 private:
  uint32_t _version;
};

/**
 * Load a serialized build
 * \param filename  The name of the file to load the build from
 * \returns A pointer to the loaded build, or to nullptr if loading failed
 */
Build load_build(string filename);

/**
 * Serialize a build to a file
 * \param filename  The name of the file to save the build to
 * \param b         The build that should be saved
 */
void save_build(string filename, Build& b);
