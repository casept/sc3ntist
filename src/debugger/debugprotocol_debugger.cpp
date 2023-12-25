#include "debugprotocol_debugger.h"
#include "debugprotocol.pb.h"

#include <algorithm>
#include <asio.hpp>
#include <google/protobuf/io/coded_stream.h>
#include <QtEndian>
#include <QDebug>
#include <QLoggingCategory>

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

using asio::ip::tcp;
using Buf = std::vector<uint8_t>;

Q_LOGGING_CATEGORY(debugProtocol, "sc3ntist.debugProtocol")

namespace Dbg::Proto::Impl {
Connection::Connection(const char* addr, uint16_t port)
    : ctx({}), sock(ctx), recvBuf({}) {
  tcp::resolver resolv(ctx);
  auto endpoints = resolv.resolve(addr, std::to_string(port));
  sock = tcp::socket(ctx);
  // This assumes an asio implementation detail by dealing with raw underlying
  // OS sockets, because apparently it's acceptable for "mature" C++ libraries
  // to lack basic features.
  const int timeoutMs = 1000;
  ::setsockopt(sock.native_handle(), SOL_SOCKET, SO_RCVTIMEO,
               (const char*)&timeoutMs, sizeof timeoutMs);
  ::setsockopt(sock.native_handle(), SOL_SOCKET, SO_SNDTIMEO,
               (const char*)&timeoutMs, sizeof timeoutMs);
  asio::connect(sock, endpoints);
  // Reduce latency
  asio::socket_base::send_buffer_size bufsize(32);
  sock.set_option(bufsize);
}

void Connection::SendCmd(const SC3Debug::Request& cmd) {
  qCDebug(debugProtocol) << "Writing debug protocol message"
                         << cmd.DebugString().c_str();
  Buf buf{};
  size_t size = cmd.ByteSizeLong();
  buf.reserve(4 + size);
  google::protobuf::io::ArrayOutputStream* arr =
      new google::protobuf::io::ArrayOutputStream(buf.data(), 4 + size);
  google::protobuf::io::CodedOutputStream* coded =
      new google::protobuf::io::CodedOutputStream(arr);
  coded->WriteLittleEndian32(size);
  cmd.SerializeToCodedStream(coded);

  asio::write(sock, asio::buffer(buf));

  delete coded;
  delete arr;
}

std::optional<SC3Debug::Reply> Connection::RecvReply() {
  // peek is discouraged, so read all bytes into our buffer first
  const size_t avail = sock.available();
  if (avail > 0) {
    recvBuf.resize(recvBuf.size() + avail);
    sock.read_some(asio::buffer(recvBuf));
  }

  google::protobuf::io::ArrayInputStream* arr =
      new google::protobuf::io::ArrayInputStream(recvBuf.data(),
                                                 recvBuf.size());
  google::protobuf::io::CodedInputStream* coded =
      new google::protobuf::io::CodedInputStream(arr);

  // Check whether we have enough data to determine size
  std::optional<size_t> size{};
  if (coded->BytesUntilTotalBytesLimit() >= 4) {
    uint32_t size32;
    coded->ReadLittleEndian32(&size32);
    size.emplace(static_cast<size_t>(size32));
  }

  // Check whether message is complete
  if (size.has_value() && coded->BytesUntilTotalBytesLimit() >= size) {
    // Message is complete - read and clear buffer
    SC3Debug::Reply r = SC3Debug::Reply();
    r.ParseFromCodedStream(coded);
    // Have to create a new vector and swap, as bytes are in front
    Buf newBuf{};
    const auto left = coded->BytesUntilTotalBytesLimit();
    newBuf.reserve(left);
    std::copy(recvBuf.begin() + (recvBuf.size() - left), recvBuf.end(),
              newBuf.begin());
    recvBuf.swap(newBuf);
    delete coded;
    delete arr;
    return {r};
  }

  delete coded;
  delete arr;
  return {};
}
}  // namespace Dbg::Proto::Impl
