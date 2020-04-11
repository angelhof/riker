#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "core/Artifact.hh"
#include "core/IR.hh"

class Command;
class Graphviz;
class Tracer;

using std::make_shared;
using std::string;
using std::vector;

enum class FingerprintLevel { None, Local, All };

class Build {
 public:
  /****** Constructors ******/
  Build() {}

  Build(string executable, vector<string> arguments);

  // Disallow Copy
  Build(const Build&) = delete;
  Build& operator=(const Build&) = delete;

  // Allow Move
  Build(Build&&) = default;
  Build& operator=(Build&&) = default;

  shared_ptr<Command> getRoot() const { return _root; }

  /****** Non-trivial methods ******/

  bool load(string filename);

  void run(Tracer& tracer);

  void drawGraph(Graphviz& g);

  void printTrace(ostream& o);

  template <class Archive>
  friend void serialize(Archive& archive, Build& g, uint32_t version);

 private:
  shared_ptr<Command> _root;

  shared_ptr<Reference> _stdin_ref;
  shared_ptr<Artifact> _stdin;

  shared_ptr<Reference> _stdout_ref;
  shared_ptr<Artifact> _stdout;

  shared_ptr<Reference> _stderr_ref;
  shared_ptr<Artifact> _stderr;
};
