#include "debugger.h"
#include "debugprotocol_debugger.h"
#include "debugprotocol.pb.h"

#include <cstdint>
#include <stdexcept>
#include <mutex>

#include <QtDebug>
#include <QLoggingCategory>
#include <QString>

Q_LOGGING_CATEGORY(debugger, "sc3ntist.debugger")

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
  connect(&conn, &Impl::Connection::ErrorEncountered, this,
          &Debugger::ErrorEncountered);
  connect(&conn, &Impl::Connection::GotReply, this, &Debugger::GotReply);
  // We expect the target to respond in a reasonable timeframe
  initialStateTimeout = new QTimer(this);
  connect(initialStateTimeout, &QTimer::timeout, this, &Debugger::Timeout);
  initialStateTimeout->setSingleShot(true);
  initialStateTimeout->setInterval(1000 * 5);
  initialStateTimeout->start();

  // Initial state sync
  SC3Debug::Request r = SC3Debug::Request();
  r.set_type(SC3Debug::REQUEST_TYPE_GET_STATE);
  conn.SendCmd(r);
}

void Debugger::Timeout() {
  // FIXME: Throwing in a slot is a very bad idea.
  throw std::runtime_error("Target did not send initial state in time");
}

void Debugger::setBreakpoint(const char* scriptbufName, uint32_t addr) {
  throw std::runtime_error("Not implemented");
  /*
  std::lock_guard<std::mutex> lck(mtx);

  // Multiple threads may be executing the same script
  for (const auto tid : scriptBuf2Tids.at(scriptbufName)) {
    const Cmd::Cmd cmd = {.type = Cmd::Type::SetBreakpoint,
                          .cmd = Cmd::SetBreakpoint{.tid = tid, .addr =
  addr}}; conn.SendCmd(cmd);
  }

  const Breakpoint bp = {
      .scriptBuffer = {scriptbufName},
      .address = addr,
  };
  breakpoints.push_back(bp);
  */
}

void Debugger::unsetBreakpoint(const char* scriptbufName, uint32_t addr) {
  throw std::runtime_error("Not implemented");
  /*
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
  const auto pred = [&bp](const Breakpoint& bpInVec) { return bpInVec == bp;
  }; breakpoints.erase( std::remove_if(breakpoints.begin(), breakpoints.end(),
  pred), breakpoints.end());
  */
}

void Debugger::GotReply(SC3Debug::Reply reply) {
  switch (reply.type()) {
    case SC3Debug::ReplyType::REPLY_TYPE_VM_STATE: {
      // Using logging category here causes a mysterious linker error
      qDebug() << "Got VM state update";
      initialStateTimeout->stop();
      // TODO: Process
      break;
    };
    default: {
      // Using logging category here causes a mysterious linker error
      qDebug() << "Got unimplemented reply type" << reply.type();
      break;
    };
  }
}

void Debugger::continueExecution(Breakpoint bp) {
  throw std::runtime_error("Not implemented");
  /*
  std::lock_guard<std::mutex> lck(mtx);

  const auto tid = scriptBuf2Tids.at(bp.scriptBuffer).front();
  const Cmd::Cmd cmd = {
      .type = Cmd::Type::Continue,
      .cmd = Cmd::Continue{.tid = tid},
  };
  conn.SendCmd(cmd);
  */
}

void Debugger::ErrorEncountered(QString msg) {
  // TODO: Pass on to main app
  qWarning() << "Encountered error:" << msg;
}
}  // namespace Dbg
