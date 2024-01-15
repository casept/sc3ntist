#include "connection.h"
#include "debugprotocol.pb.h"

#include <algorithm>
#include <google/protobuf/io/coded_stream.h>
#include <QtEndian>
#include <QDebug>
#include <QLoggingCategory>
#include <QTcpSocket>

// Part of the ugly timeout hack
#if defined(WIN32) || defined(_WIN32) || \
    defined(__WIN32) && !defined(__CYGWIN__)
#include <winsock.h>
#else
extern "C" {
#include <sys/socket.h>
}
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

using Buf = std::vector<uint8_t>;

Q_LOGGING_CATEGORY(debugProtocol, "sc3ntist.debugProtocol")

namespace Dbg::Proto::Impl {
Connection::Connection(const char* addr, uint16_t port) : recvBuf({}) {
  sock = new QTcpSocket(this);
  connect(sock, &QTcpSocket::errorOccurred, this, &Connection::SocketError);
  connect(sock, &QTcpSocket::readyRead, this, &Connection::DataReceived);
  sock->connectToHost(addr, port);
  sock->setSocketOption(QAbstractSocket::SocketOption::LowDelayOption, true);
}

void Connection::SendCmd(const SC3Debug::Request& cmd) {
  if (!cmd.IsInitialized()) {
    qCCritical(debugProtocol)
        << "Attempt to send uninitialized message, dropping!";
    return;
  }
  qCDebug(debugProtocol) << "Writing debug protocol message"
                         << cmd.DebugString().c_str();
  Buf buf{};
  size_t size = cmd.ByteSizeLong();
  buf.resize(size + 4);
  google::protobuf::io::ArrayOutputStream* arr =
      new google::protobuf::io::ArrayOutputStream(buf.data(), buf.size());
  google::protobuf::io::CodedOutputStream* coded =
      new google::protobuf::io::CodedOutputStream(arr);
  coded->WriteLittleEndian32(size);
  cmd.SerializeToCodedStream(coded);

  qCDebug(debugProtocol) << "Wrote" << buf.size() << "bytes";
  QByteArray baBuf{reinterpret_cast<char*>(buf.data()),
                   static_cast<int>(buf.size())};
  sock->write(baBuf);

  delete coded;
  delete arr;
}

void Connection::DataReceived() {
  // peek is discouraged, so read all bytes into our buffer first
  const size_t avail = sock->bytesAvailable();
  if (avail > 0) {
    auto initialSize = recvBuf.size();
    recvBuf.resize(recvBuf.size() + avail);
    auto newData = sock->readAll();
    // TODO: Solve elegantly
    for (size_t i = 0; i < newData.size(); i++) {
      recvBuf.at(initialSize + i) = newData.at(i);
    }
  }

  google::protobuf::io::ArrayInputStream* arr =
      new google::protobuf::io::ArrayInputStream(recvBuf.data(),
                                                 recvBuf.size());
  google::protobuf::io::CodedInputStream* coded =
      new google::protobuf::io::CodedInputStream(arr);

  // Check whether we have enough data to determine size
  std::optional<size_t> size{};
  if (recvBuf.size() >= 4) {
    uint32_t size32;
    coded->ReadLittleEndian32(&size32);
    size.emplace(static_cast<size_t>(size32));
    coded->SetTotalBytesLimit(size.value() + 4);
  }

  // Check whether message is complete
  if (size.has_value() && coded->BytesUntilTotalBytesLimit() >= size) {
    // Message is complete - read and clear buffer
    SC3Debug::Reply r = SC3Debug::Reply();
    r.ParseFromCodedStream(coded);
    // Have to create a new vector and swap, as bytes are in front
    Buf newBuf{};
    const auto left = coded->BytesUntilTotalBytesLimit();
    newBuf.resize(left);
    std::copy(recvBuf.begin() + recvBuf.size() - left, recvBuf.end(),
              newBuf.begin());
    recvBuf.swap(newBuf);
    emit GotReply(r);
  }

  delete coded;
  delete arr;
}

void Connection::SocketError(QAbstractSocket::SocketError err) {
  QString msg = "Encountered socket error: " + sock->errorString();
  qCWarning(debugProtocol) << msg;
  emit ErrorEncountered(msg);
}
}  // namespace Dbg::Proto::Impl
