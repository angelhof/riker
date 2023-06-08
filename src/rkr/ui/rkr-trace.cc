#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "data/Trace.hh"
#include "ui/commands.hh"
#include "util/TracePrinter.hh"
//#include "util/constants.hh"

using std::cout;
using std::ofstream;
using std::string;
using std::vector;

namespace fs = std::filesystem;

/**
 * Run the `trace` subcommand
 * \param output    The name of the output file, or "-" for stdout
 */
void do_trace(vector<string> args, string output, fs::path dbDir) noexcept {
  auto DatabaseFilename = dbDir / "db";
  auto trace = TraceReader::load(DatabaseFilename);
  FAIL_IF(!trace) << "A trace could not be loaded. Run a full build first.";

  // Are we printing to stdout or a file?
  if (output == "-") {
    trace->sendTo(TracePrinter(cout));
  } else {
    trace->sendTo(TracePrinter(ofstream(output)));
  }
}
