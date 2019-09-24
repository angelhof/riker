#pragma once

#include <memory>

#include <sys/types.h>

#include <kj/common.h>

struct BuildGraph;
struct Command;

struct InitialFdEntry {
  int parent_fd;
  int child_fd;
};

pid_t start_command(BuildGraph& graph, std::shared_ptr<Command> cmd,
                    kj::ArrayPtr<InitialFdEntry const> initial_fds);
void trace_step(BuildGraph& graph, pid_t child, int wait_status);
