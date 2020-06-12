#pragma once

#include <memory>
#include <string>

#include "artifacts/Artifact.hh"

using std::shared_ptr;
using std::string;

class Command;
class Reference;
class Version;
class ContentVersion;

class DirArtifact final : public Artifact {
 public:
  DirArtifact(Env& env, bool committed,
              shared_ptr<MetadataVersion> mv = make_shared<MetadataVersion>()) noexcept :
      Artifact(env, committed, mv) {}

  virtual string getTypeName() const noexcept final { return "Dir"; }
};