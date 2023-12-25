#pragma once

#include <asio.hpp>

#include <cstdint>
#include <optional>
#include <QLoggingCategory>
#include "debugprotocol.pb.h"

/*
The actual implementation of the debugger side of the debug protocol.

TODO: Rewrite to use Qt Network instead of asio and drop the dependency.
*/

Q_DECLARE_LOGGING_CATEGORY(debugProtocol)

namespace Dbg::Proto::Impl {
class Connection {
 public:
  Connection() = delete;
  Connection(const char* addr, uint16_t port);
  /// Send a command to target. Blocks until send completes.
  void SendCmd(const SC3Debug::Request& cmd);
  /// Read a reply from target, if available. Non-blocking.
  std::optional<SC3Debug::Reply> RecvReply();

 private:
  asio::io_context ctx;
  asio::ip::tcp::socket sock;
  std::vector<uint8_t> recvBuf;
};
}  // namespace Dbg::Proto::Impl
