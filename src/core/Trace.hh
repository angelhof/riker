#pragma once

#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>

#include <cereal/archives/binary.hpp>

#include "core/SpecialRefs.hh"
#include "core/TraceHandler.hh"

using std::ifstream;
using std::list;
using std::map;
using std::ofstream;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

namespace fs = std::filesystem;

class Build;
class Command;
class InputTrace;

using command_id_t = uint32_t;

/// A trace is saved on disk as a series of records. Sub-classes are defined in Trace.cc
struct Record {
  Record() noexcept = default;
  virtual ~Record() = default;

  virtual bool isEnd() const noexcept { return false; }

  virtual void handle(InputTrace& input, TraceHandler& handler) noexcept = 0;

  template <class Archive>
  void serialize(Archive& archive) {}
};

/**
 * An input trace is a build trace loaded from disk
 */
class InputTrace {
 public:
  /// Load an input trace from a given path, or produce a default starting trace if no trace exists
  InputTrace(string filename) noexcept;

  // Disallow copy
  InputTrace(const InputTrace&) = delete;
  InputTrace& operator=(const InputTrace&) = delete;

  // Allow move
  InputTrace(InputTrace&&) = default;
  InputTrace& operator=(InputTrace&&) = default;

  /// Send the loaded trace to a trace handler
  void sendTo(TraceHandler& handler) noexcept;

  /// Send the loaded trace to a trace handler
  void sendTo(TraceHandler&& handler) noexcept { sendTo(handler); }

  /// Add a command with a known ID to this input trace. If the command ID has already been loaded,
  /// the original instance will be used and not the new one.
  void addCommand(command_id_t id, shared_ptr<Command> cmd) {
    auto iter = _commands.find(id);
    if (iter == _commands.end()) _commands.emplace_hint(iter, id, cmd);
  }

  /// Get a command from its ID
  shared_ptr<Command> getCommand(command_id_t id) { return _commands.at(id); }

 private:
  /// Initialize the list of steps to a default trace
  void initDefault() noexcept;

 private:
  /// The map from command IDs to command instances. Startup steps run in command 0
  map<command_id_t, shared_ptr<Command>> _commands = {{0, nullptr}};

  /// The list of records loaded from the trace file
  list<unique_ptr<Record>> _records;
};

/**
 * An output trace is used to write a trace to disk
 */
class OutputTrace : public TraceHandler {
 public:
  /// Create a trace at the given path
  OutputTrace(string filename) noexcept : _filename(filename) {}

  // Disallow copy
  OutputTrace(const OutputTrace&) = delete;
  OutputTrace& operator=(const OutputTrace&) = delete;

  // Add a new command to the output trace and return its unique ID
  command_id_t addCommand(shared_ptr<Command> cmd) noexcept {
    command_id_t id = _commands.size();
    _commands.emplace(cmd, id);
    return id;
  }

  // Get the ID for a command instance
  size_t getCommandID(shared_ptr<Command> cmd) const noexcept { return _commands.at(cmd); }

  /// Trace output is finished
  virtual void finish() noexcept override;

  /// Add a SpecialRef IR step to the output trace
  virtual void specialRef(shared_ptr<Command> command,
                          SpecialRef entity,
                          shared_ptr<RefResult> output) noexcept override;

  /// Add a PipeRef IR step to the output trace
  virtual void pipeRef(shared_ptr<Command> command,
                       shared_ptr<RefResult> read_end,
                       shared_ptr<RefResult> write_end) noexcept override;

  /// Add a FileRef IR step to the output trace
  virtual void fileRef(shared_ptr<Command> command,
                       mode_t mode,
                       shared_ptr<RefResult> output) noexcept override;

  /// Add a SymlinkRef IR step to the output trace
  virtual void symlinkRef(shared_ptr<Command> command,
                          fs::path target,
                          shared_ptr<RefResult> output) noexcept override;

  /// Add a DirRef IR step to the output trace
  virtual void dirRef(shared_ptr<Command> command,
                      mode_t mode,
                      shared_ptr<RefResult> output) noexcept override;

  /// Add a PathRef IR step to the output trace
  virtual void pathRef(shared_ptr<Command> command,
                       shared_ptr<RefResult> base,
                       fs::path path,
                       AccessFlags flags,
                       shared_ptr<RefResult> output) noexcept override;

  /// Add a ExpectResult IR step to the output trace
  virtual void expectResult(shared_ptr<Command> command,
                            shared_ptr<RefResult> ref,
                            int expected) noexcept override;

  /// Add a MatchMetadata IR step to the output trace
  virtual void matchMetadata(shared_ptr<Command> command,
                             shared_ptr<RefResult> ref,
                             shared_ptr<MetadataVersion> version) noexcept override;

  /// Add a MatchContent IR step to the output trace
  virtual void matchContent(shared_ptr<Command> command,
                            shared_ptr<RefResult> ref,
                            shared_ptr<Version> version) noexcept override;

  /// Add a UpdateMetadata IR step to the output trace
  virtual void updateMetadata(shared_ptr<Command> command,
                              shared_ptr<RefResult> ref,
                              shared_ptr<MetadataVersion> version) noexcept override;

  /// Add a UpdateContent IR step to the output trace
  virtual void updateContent(shared_ptr<Command> command,
                             shared_ptr<RefResult> ref,
                             shared_ptr<Version> version) noexcept override;

  /// Add a Launch IR step to the output trace
  virtual void launch(shared_ptr<Command> command, shared_ptr<Command> child) noexcept override;

  /// Add a Join IR step to the output trace
  virtual void join(shared_ptr<Command> command,
                    shared_ptr<Command> child,
                    int exit_status) noexcept override;

  /// Add a Exit IR step to the output trace
  virtual void exit(shared_ptr<Command> command, int exit_status) noexcept override;

 private:
  /// The path where this trace will be written
  string _filename;

  /// The list of records to write
  list<unique_ptr<Record>> _records;

  /// The map from commands to their IDs in the output trace
  map<shared_ptr<Command>, command_id_t> _commands = {{nullptr, 0}};
};