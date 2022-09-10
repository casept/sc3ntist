#include "debugger.h"
#include "debugprotocol_debugger.h"
#include "debugprotocol.h"

#include <cstdint>
#include <stdexcept>
#include <mutex>
#include <chrono>

#include <QDebug>

using namespace Dbg::Proto;
namespace Dbg {

bool Breakpoint::operator==(const Breakpoint& other) const {
  return (this->address == other.address &&
          this->scriptBuffer == other.scriptBuffer);
}

Debugger::Debugger(const char* host, uint16_t port)
    : conn(Impl::Connection(host, port)),
      scriptBuf2Tids({}),
      tid2ScriptBuf({}),
      breakpoints({}),
      newlyHit({}) {
  // Sync up with the threads running currently
  const Cmd::Cmd cmd = {
      .type = Cmd::Type::GetThreads,
      .cmd = Cmd::GetThreads{},
  };
  conn.SendCmd(cmd);

  // We expect the target to respond in a reasonable timeframe
  auto start = std::chrono::steady_clock::now();
  // FIXME: This is inefficient
  while (scriptBuf2Tids.empty()) {
    Update();
    auto now = std::chrono::steady_clock::now();
    if ((now - start) > std::chrono::seconds{5}) {
      throw std::runtime_error(
          "Target did not send initial thread list in time");
    }
  }
}

void Debugger::setBreakpoint(const char* scriptbufName, uint32_t addr) {
  std::lock_guard<std::mutex> lck(mtx);

  // Multiple threads may be executing the same script
  for (const auto tid : scriptBuf2Tids.at(scriptbufName)) {
    const Cmd::Cmd cmd = {.type = Cmd::Type::SetBreakpoint,
                          .cmd = Cmd::SetBreakpoint{.tid = tid, .addr = addr}};
    conn.SendCmd(cmd);
  }

  const Breakpoint bp = {
      .scriptBuffer = {scriptbufName},
      .address = addr,
  };
  breakpoints.push_back(bp);
}

void Debugger::unsetBreakpoint(const char* scriptbufName, uint32_t addr) {
  std::lock_guard<std::mutex> lck(mtx);

  // Multiple threads may be executing the same script
  for (const auto tid : scriptBuf2Tids.at(scriptbufName)) {
    const Cmd::Cmd cmd = {
        .type = Cmd::Type::UnsetBreakpoint,
        .cmd = Cmd::UnsetBreakpoint{.tid = tid, .addr = addr}};
    conn.SendCmd(cmd);
  }

  const Breakpoint bp = {
      .scriptBuffer = {scriptbufName},
      .address = addr,
  };
  const auto pred = [&bp](const Breakpoint& bpInVec) { return bpInVec == bp; };
  breakpoints.erase(
      std::remove_if(breakpoints.begin(), breakpoints.end(), pred),
      breakpoints.end());
}

void Debugger::Update() {
  std::lock_guard<std::mutex> lck(mtx);

  while (true) {
    const auto reply = conn.RecvReply();
    if (!reply.has_value()) {
      return;
    }

    switch (reply->type) {
      using Rt = Reply::Type;
      case Rt::GetThreads: {
        scriptBuf2Tids = std::get<Reply::GetThreads>(reply->reply).threads;
        tid2ScriptBuf.clear();
        for (const auto& entry : scriptBuf2Tids) {
          for (const auto tid : entry.second) {
            tid2ScriptBuf[tid] = entry.first;
          }
        }
        break;
      };
      case Rt::BreakpointHit: {
        const auto hit = std::get<Reply::BreakpointHit>(reply->reply);
        for (const Breakpoint& bp : breakpoints) {
          if (bp.address == hit.addr &&
              bp.scriptBuffer == tid2ScriptBuf.at(hit.tid)) {
            newlyHit.push_back(bp);
          }
        }
        break;
      };
      default: {
        throw std::runtime_error(
            "Debugger::Update(): Got unimplemented reply type " +
            std::to_string(static_cast<uint8_t>(reply->type)));
      };
    }
  }
}

void Debugger::runBreakpointHandler(BreakpointHandler handler) {
  std::lock_guard<std::mutex> lck(mtx);

  for (const Breakpoint& bp : newlyHit) {
    handler(bp);
  }
  newlyHit.clear();
}
}  // namespace Dbg