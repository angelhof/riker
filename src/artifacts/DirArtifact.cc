#include "DirArtifact.hh"

#include <list>
#include <memory>
#include <string>
#include <utility>

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

#include "build/Build.hh"
#include "build/Env.hh"
#include "build/Resolution.hh"
#include "core/IR.hh"
#include "util/log.hh"
#include "versions/MetadataVersion.hh"

using std::list;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::tie;

DirArtifact::DirArtifact(Env& env,
                         shared_ptr<MetadataVersion> mv,
                         shared_ptr<BaseDirVersion> dv) noexcept :
    Artifact(env, mv) {
  _base_dir_version = dv;
  appendVersion(dv);
}

bool DirArtifact::canCommit(shared_ptr<Version> v) const noexcept {
  if (auto dv = v->as<DirVersion>()) {
    return dv->canCommit();
  } else {
    return Artifact::canCommit(v);
  }
}

void DirArtifact::commit(shared_ptr<Version> v) noexcept {
  // The base directory version must always be committed to commit any other version
  auto path = getCommittedPath();
  ASSERT(path.has_value()) << "Committing to a directory with no path";
  _base_dir_version->commit(this->as<DirArtifact>(), path.value());

  if (auto dv = v->as<DirVersion>()) {
    dv->commit(this->as<DirArtifact>(), path.value());
  } else {
    Artifact::commit(v);
  }
}

bool DirArtifact::canCommitAll() const noexcept {
  // If this artifact's metadata cannot be committed, stop now
  if (!Artifact::canCommitAll()) return false;

  // Can the base version be committed? If not, stop now.
  if (!_base_dir_version->canCommit()) return false;

  // Can every entry be committed?
  for (auto& [name, info] : _entries) {
    auto& [version, artifact] = info;
    if (!version->canCommit()) return false;
  }

  // Everything is committable
  return true;
}

// Commit all final versions of this artifact to the filesystem
void DirArtifact::commitAll() noexcept {
  auto path = getCommittedPath();
  ASSERT(path.has_value()) << "Directory has no path";

  // Commit the versions needed for each entry
  for (auto& [name, info] : _entries) {
    auto& [version, artifact] = info;
    commit(version);
  }

  // Commit metadata through the Artifact base class
  Artifact::commitAll();
}

// Command c requires that this artifact exists in its current state. Create dependency edges.
void DirArtifact::mustExist(Build& build, shared_ptr<Command> c) noexcept {
  build.observeInput(c, shared_from_this(), _metadata_version, InputType::Exists);
  build.observeInput(c, shared_from_this(), _base_dir_version, InputType::Exists);

  for (auto [entry, info] : _entries) {
    auto [version, artifact] = info;
    build.observeInput(c, shared_from_this(), version, InputType::Exists);
  }
}

// Compare all final versions of this artifact to the filesystem state
void DirArtifact::checkFinalState(Build& build, fs::path path) noexcept {
  // Recursively check the final state of all known entries
  for (auto& [name, info] : _entries) {
    auto& [version, artifact] = info;

    // Do we expect the entry to point to an artifact?
    if (artifact) {
      // Yes. Make sure that artifact is in the expected final state
      artifact->checkFinalState(build, path / name);
    }

    // If the entry doesn't reference an artifact, we don't need to check for its absence. We only
    // have a record of this artifact being missing because some other part of the build accessed
    // it. That means there will be some earlier reference that was expected to succeed or fail that
    // will now have changed.
  }

  // Check the metadata state as well
  Artifact::checkFinalState(build, path);
}

// Commit any pending versions and save fingerprints for this artifact
void DirArtifact::applyFinalState(fs::path path) noexcept {
  // First, commit this artifact and its metadata
  // TODO: Should we just commit the base version, then commit entries on demand?
  commitAll();

  // Fingerprint/commit any remaining metadata
  Artifact::applyFinalState(path);

  // Recursively apply final state for all known entries
  for (auto& [name, info] : _entries) {
    auto& [version, artifact] = info;
    if (artifact) artifact->applyFinalState(path / name);
  }
}

// Get a version that lists all the entries in this directory
shared_ptr<ListedDir> DirArtifact::getDirList(Build& build,
                                              shared_ptr<Command> c,
                                              InputType t) noexcept {
  auto result = _base_dir_version->getList(_env, as<DirArtifact>());

  // The command listing this directory depends on its base version
  build.observeInput(c, shared_from_this(), _base_dir_version, t);

  for (auto [name, info] : _entries) {
    // Get the version and artifact for this entry
    auto [version, artifact] = info;

    // If this entry is from the base version, we've already covered it
    if (version == _base_dir_version) continue;

    // Otherwise the entry is from some other version. Update the list.
    if (artifact) {
      result->addEntry(name);
    } else {
      result->removeEntry(name);
    }

    // The listing command depends on whatever version is responsible for this entry
    build.observeInput(c, shared_from_this(), version, t);
  }

  return result;
}

// Check to see if this artifact's directory list matches a known version
void DirArtifact::match(Build& build,
                        shared_ptr<Command> c,
                        shared_ptr<ListedDir> expected) noexcept {
  // Get the directory list. All dependencies only care if an entry exists, not on its specific
  // contents
  auto observed = getDirList(build, c, InputType::Exists);

  // Compare versions
  if (!observed->matches(expected)) {
    // Report the mismatch
    build.observeMismatch(c, shared_from_this(), observed, expected);
  }
}

Resolution DirArtifact::resolve(Build& build,
                                shared_ptr<Command> c,
                                shared_ptr<Artifact> prev,
                                fs::path::iterator current,
                                fs::path::iterator end,
                                shared_ptr<Access> ref,
                                bool committed) noexcept {
  // If the path has a trailing slash, the final entry will be empty. Advance past any empty
  // entries
  while (current != end && current->empty()) current++;

  // If this is the last entry on the path, resolution reaches this artifact
  if (current == end) {
    // If the requested access is not allowed, return EACCES
    if (!checkAccess(build, c, ref->getFlags())) return EACCES;

    // Access was allowed, so return this artifact
    return shared_from_this();
  }

  // If the remaining path is not empty, make sure we have execute permission in this directory
  if (!checkAccess(build, c, AccessFlags{.x = true})) return EACCES;

  // We must be looking for an entry in this directory. Get the entry name and advance the
  // iterator
  fs::path entry = *current++;

  // Are we looking for the current directory?
  if (entry == ".") return resolve(build, c, shared_from_this(), current, end, ref, committed);

  // Are we looking for the parent directory?
  if (entry == "..") {
    auto parent = getParentDir();
    ASSERT(parent.has_value()) << "Directory has no parent";
    return parent.value()->resolve(build, c, shared_from_this(), current, end, ref, committed);
  }

  // We'll track the result of the resolution here
  Resolution result;

  // Check the map of known entries for a match
  auto entries_iter = _entries.find(entry);
  if (entries_iter != _entries.end()) {
    // Found a match.
    // Get the version responsible for this entry and the artifact it mapped (possibly null)
    auto [v, a] = entries_iter->second;

    // Make sure the version is committed if requested
    if (committed) commit(v);

    // Is there an artifact to resolve to?
    if (a) {
      result = a;
    } else {
      result = ENOENT;
    }

    // Add a path resolution input from the version that matched
    build.observeInput(c, shared_from_this(), v, InputType::PathResolution);

  } else {
    // There's no match in the directory entry map. We need to check the base version for a match
    result = _base_dir_version->getEntry(build, _env, as<DirArtifact>(), entry);
    if (result) {
      shared_ptr<Artifact> a = result;
      _entries[entry] = {_base_dir_version, a};
    } else {
      _entries[entry] = {_base_dir_version, nullptr};
    }

    // Add a path resolution input from the base version
    build.observeInput(c, shared_from_this(), _base_dir_version, InputType::PathResolution);
  }

  // We now have either a resolved artifact or an error code. The next step depends on whether
  // this is the last part of the path, or if there is more path left to resolve.
  if (current == end) {
    // This is the last entry in the resolution path

    auto flags = ref->getFlags();

    // Was the reference required to create this entry?
    if (flags.create && flags.exclusive && result) return EEXIST;

    // If the resolution failed, can this access create it?
    if (flags.create && result == ENOENT) {
      // Can we write to this directory? If not, return an error
      if (!checkAccess(build, c, AccessFlags{.w = true})) return EACCES;

      // Create a new file
      auto newfile = _env.createFile(build, c, flags, committed);

      // Mark the final reference as resolved so we can link the file
      ref->resolvesTo(newfile);

      // Link the new file into this directory
      auto link_version = make_shared<AddEntry>(entry, ref);
      link_version->createdBy(c);
      if (committed) link_version->setCommitted();
      apply(build, c, link_version);

      // The resolution result is now the newly-created file
      result = newfile;

      // THe newly-created file is linked in this directory
      result->addLinkUpdate(as<DirArtifact>(), entry, link_version);
    }

    // If the result was an error, return it
    if (!result) return result;

    // Otherwise continue with resolution, which may follow symlinks
    return result->resolve(build, c, shared_from_this(), current, end, ref, committed);

  } else {
    // There is still path left to resolve. Recursively resolve if the result succeeded
    if (result) {
      return result->resolve(build, c, shared_from_this(), current, end, ref, committed);
    }

    // Otherwise return the error from the resolution
    return result;
  }
}

// Apply a link version to this artifact
void DirArtifact::apply(Build& build,
                        shared_ptr<Command> c,
                        shared_ptr<AddEntry> writing) noexcept {
  auto entry = writing->getEntryName();
  auto artifact = writing->getTarget()->getArtifact();

  // Check for an existing entry with the same name
  auto iter = _entries.find(entry);
  if (iter != _entries.end()) {
    // TODO: We will overwrite the old entry. How do we track that?
    // TODO: AddEntry versions should be tagged with an `overwrite` flag
  }

  // For this link to be committed, we need the artifact to exist or be committable
  artifact->mustExist(build, c);

  // Inform the artifact of its new link
  artifact->addLinkUpdate(as<DirArtifact>(), entry, writing);

  // Add the new entry to the entries map
  _entries[entry] = {writing, artifact};

  // Notify the build of this output
  build.observeOutput(c, shared_from_this(), writing);

  // Record this version in the artifact
  appendVersion(writing);
}

// Apply an unlink version to this artifact
void DirArtifact::apply(Build& build,
                        shared_ptr<Command> c,
                        shared_ptr<RemoveEntry> writing) noexcept {
  auto entry = writing->getEntryName();

  // Do we have a record of an entry with the given name?
  auto iter = _entries.find(entry);
  if (iter != _entries.end()) {
    // Get the version that added this entry, and the artifact it maps to
    auto& [version, artifact] = iter->second;

    // If there is an artifact at this entry, inform it of an unlink operation
    if (artifact) artifact->addLinkUpdate(as<DirArtifact>(), entry, writing);

    // Is the version that linked this entry uncommitted?
    if (!version->isCommitted()) {
      // Not committed. If the uncommitted version is an AddEntry version, we can cancel it out.
      if (auto add = version->as<AddEntry>()) {
        // The new RemoveEntry version cancels out the uncommitted AddEntry version
        add->setCommitted();
        writing->setCommitted();
      }
    }
  }

  // Update the entries map
  _entries[entry] = {writing, nullptr};

  // Notify the build of this output
  build.observeOutput(c, shared_from_this(), writing);

  // Record this version in the artifact as well
  appendVersion(writing);
}
