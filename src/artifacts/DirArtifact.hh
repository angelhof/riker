#pragma once

#include <map>
#include <memory>
#include <string>

#include "artifacts/Artifact.hh"
#include "build/Resolution.hh"
#include "versions/DirVersion.hh"

using std::map;
using std::shared_ptr;
using std::string;
using std::weak_ptr;

class Command;
class Reference;
class Version;
class ContentVersion;

class DirArtifact final : public Artifact {
 public:
  DirArtifact(Env& env, shared_ptr<MetadataVersion> mv, shared_ptr<DirVersion> dv) noexcept;

  /************ Core Artifact Operations ************/

  /// Get the name of this artifact type
  virtual string getTypeName() const noexcept override { return "Dir"; }

  /// Have all modifications to this artifact been committed to the filesystem?
  virtual bool isCommitted() const noexcept override;

  /// Can this artifact be fully committed?
  virtual bool canCommit() const noexcept override;

  /// Commit any un-committed version of this artifact using the provided reference
  virtual void commit(shared_ptr<Reference> ref) noexcept override;

  /// Check the final state of this artifact and save any necessary final fingerprints
  virtual void finalize(shared_ptr<Reference> ref, bool commit) noexcept override;

  /************ Directory Operations ************/

  /**
   * Attempt to access a directory entry in the current artifact.
   * \param c     The command making the access
   * \param ref   A reference that was used to reach this directory
   * \param entry The name of the entry being requested
   * \returns a resolution result, holding either an artifact or error code
   */
  virtual Resolution getEntry(shared_ptr<Command> c,
                              shared_ptr<Reference> ref,
                              string entry) noexcept override;

  /// Apply a link version to this artifact
  virtual void apply(shared_ptr<Command> c,
                     shared_ptr<Reference> ref,
                     shared_ptr<LinkVersion> writing) noexcept override;

  /// Apply an unlink version to this artifact
  virtual void apply(shared_ptr<Command> c,
                     shared_ptr<Reference> ref,
                     shared_ptr<UnlinkVersion> writing) noexcept override;

 private:
  map<string, weak_ptr<Artifact>> _resolved;

  list<shared_ptr<DirVersion>> _dir_versions;

  bool _finalized = false;
};