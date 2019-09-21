#pragma once

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <sys/types.h>

#include "core/FileDescriptor.hh"

struct BuildGraph;
struct Command;
struct File;

struct Process : public std::enable_shared_from_this<Process> {
  pid_t thread_id;
  std::map<int, FileDescriptor> fds;
  std::set<File*> mmaps;

  /****** Constructors ******/

  Process(pid_t thread_id, std::string cwd, std::shared_ptr<Command> command);

  // Disallow Copy
  Process(const Process&) = delete;
  Process& operator=(const Process&) = delete;

  // Allow Move
  Process(Process&&) = default;
  Process& operator=(Process&&) = default;

  /****** Non-trivial methods ******/

  void traceMmap(BuildGraph& graph, int fd);

  void traceChdir(std::string newdir);

  void traceChroot(std::string newroot);

  void traceClose(int fd);

  std::shared_ptr<Process> traceFork(pid_t child_pid);

  void traceExec(BuildGraph& trace, std::string executable, const std::list<std::string>& args);

  void traceExit();

  /****** Getters and setters ******/

  std::shared_ptr<Command> getCommand() { return _command; }

 private:
  std::shared_ptr<Command> _command;
  std::string _cwd;
  std::string _root;
};
