#pragma once

#include <cstdint>
#include <optional>

#include <QAbstractSocket>
#include <QObject>
#include <QLoggingCategory>
#include <QString>
#include <QTcpSocket>

#include "debugprotocol.pb.h"

//! The actual implementation of the debugger side of the debug protocol.

Q_DECLARE_LOGGING_CATEGORY(debugProtocol)

namespace Dbg::Proto::Impl {
class Connection : public QObject {
  Q_OBJECT

 public:
  Connection() = delete;
  Connection(const char* addr, uint16_t port);
  /// Send a command to target. Blocks until send completes.
  void SendCmd(const SC3Debug::Request& cmd);

 signals:
  /// The target replied.
  void GotReply(SC3Debug::Reply);
  /// An error was encountered.
  void ErrorEncountered(QString);

 private slots:
  void SocketError(QAbstractSocket::SocketError);
  void DataReceived();

 private:
  QTcpSocket* sock;
  std::vector<uint8_t> recvBuf;
};
}  // namespace Dbg::Proto::Impl
