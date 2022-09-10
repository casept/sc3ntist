#pragma once

/*
 * Storage of debugger state (volatile) and high-level interface to the debugger
 * network protocol.
 *
 * Serves a similar role in the architecture as project.h does
 * for non-volatile project state.
 */

#include "debugprotocol_debugger.h"
#include "debugprotocol.h"

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <mutex>

namespace Dbg {
struct Breakpoint {
  std::string scriptBuffer;
  uint32_t address;

  bool operator==(const Breakpoint& other) const;
};

using BreakpointHandler = std::function<void(const Breakpoint&)>;

class Debugger {
 public:
  /// Create a new instance bound to the given target.
  Debugger(const char* host, uint16_t port);
  Debugger() = delete;
  /// Update debugger state based on any new replies from the target.
  void Update();

  /// Run the given function if new breakpoints have been hit since last
  /// invocation.
  /// Should probably be called after an `Update()`.
  void runBreakpointHandler(BreakpointHandler handler);
  /// Set a breakpoint for any threads running the given script buffer.
  void setBreakpoint(const char* scriptbufName, uint32_t addr);
  /// Unset a breakpoint for any threads running the given script buffer.
  void unsetBreakpoint(const char* scriptbufName, uint32_t addr);

 private:
  /// Because multiple widgets may access something at the same time
  std::mutex mtx;
  Proto::Impl::Connection conn;
  /// Mapping of threads to their script.
  /// Needed because users set e.g. breakpoints on a per-script basis,
  /// but the target can be much simpler if the protocol only deals with thread
  /// IDs.
  std::map<std::string, std::vector<Proto::ThreadID>> scriptBuf2Tids;
  /// More optimal for some queries
  std::map<Proto::ThreadID, std::string> tid2ScriptBuf;
  std::vector<Breakpoint> breakpoints;
  std::vector<Breakpoint> newlyHit;
};
}  // namespace Dbg