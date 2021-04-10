#include <memory>

#include "artifacts/Artifact.hh"
#include "runtime/Command.hh"
#include "runtime/Ref.hh"

class MetadataVersion;
class ContentVersion;

/**
 * This class processes a build trace that has already been completed, and adds new predicates to
 * check against the state left at the end of a build.
 *
 * The PostBuildChecker class expects a template parameter that is an IRSink, which will receive all
 * of the original trace steps along with the additional steps for post-build checks. A likely use
 * case would be to instantiate a PostBuildChecker<IRBuffer>.
 */
template <class Next>
class PostBuildChecker : public Next {
 public:
  /// Handle an ExpectResult IR step
  virtual void expectResult(const std::shared_ptr<Command>& command,
                            Scenario scenario,
                            Ref::ID ref,
                            int expected) noexcept override {
    if (scenario == Scenario::Build) {
      Next::expectResult(command, Scenario::Build, ref, expected);
      Next::expectResult(command, Scenario::PostBuild, ref, command->getRef(ref)->getResultCode());
    }
  }

  /// Handle a MatchMetadata IR step
  virtual void matchMetadata(const std::shared_ptr<Command>& command,
                             Scenario scenario,
                             Ref::ID ref,
                             std::shared_ptr<MetadataVersion> expected) noexcept override {
    if (scenario == Scenario::Build) {
      // Emit the predicate from the original build phase
      Next::matchMetadata(command, Scenario::Build, ref, expected);

      // Now also emit a predicate to check for the post-build state
      if (command->getRef(ref)->isResolved()) {
        Next::matchMetadata(command, Scenario::PostBuild, ref,
                            command->getRef(ref)->getArtifact()->peekMetadata());
      }
    }
  }

  /// Handle a MatchContent IR step
  virtual void matchContent(const std::shared_ptr<Command>& command,
                            Scenario scenario,
                            Ref::ID ref,
                            std::shared_ptr<ContentVersion> expected) noexcept override {
    if (scenario == Scenario::Build) {
      // Emit the predicate from the original build phase
      Next::matchContent(command, Scenario::Build, ref, expected);

      // Now also emit a predicate to check for the post-build state
      if (command->getRef(ref)->isResolved()) {
        Next::matchContent(command, Scenario::PostBuild, ref,
                           command->getRef(ref)->getArtifact()->peekContent());
      }
    }
  }
};