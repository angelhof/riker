#pragma once

#include <filesystem>
#include <memory>
#include <ostream>

#include "util/log.hh"
#include "util/serializer.hh"
#include "versions/Version.hh"

using std::ostream;
using std::shared_ptr;

namespace fs = std::filesystem;

class SymlinkVersion : public Version {
 public:
  /// Create a symlink version with a known destination
  SymlinkVersion(fs::path dest) : _dest(dest) {}

  /// Get the name for this type of version
  virtual string getTypeName() const noexcept override { return "symlink"; }

  /// Commit this version to the filesystem
  void commit(fs::path path) noexcept;

  /// Check if this version has a fingerprint
  virtual bool hasFingerprint() const noexcept override { return true; }

  /// Compare this version to another version
  virtual bool matches(shared_ptr<Version> other) const noexcept override {
    auto other_symlink = other->as<SymlinkVersion>();
    return other_symlink && _dest == other_symlink->_dest;
  }

  /// Get the destination of this symlink
  const fs::path& getDestination() const noexcept { return _dest; }

  /// Print this symlink version
  virtual ostream& print(ostream& o) const noexcept override {
    return o << "[symlink: dest=" << _dest << "]";
  }

 private:
  fs::path _dest;

  // Create a default constructor and declare fields for serialization
  SymlinkVersion() = default;
  SERIALIZE(BASE(Version), _dest);
};
