#include "debugprotocol_debugger.h"
#include "bitsery/deserializer.h"
#include "debugprotocol.h"

#include <asio.hpp>
#include <bitsery/adapter/buffer.h>
#include <bitsery/brief_syntax/vector.h>
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
using OutputAdapter = bitsery::OutputBufferAdapter<Buf>;
using InputAdapter = bitsery::InputBufferAdapter<Buf>;

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

void Connection::SendCmd(const Cmd::Cmd& cmd) {
  Buf payload;
  const size_t sz = bitsery::quickSerialization<OutputAdapter>(payload, cmd);
  // Prepend size
  if (sz > UINT16_MAX) {
    throw std::runtime_error("Connection::SendCmd() failed: Message too large");
  }
  const quint16 sz_u16_be = qToBigEndian(static_cast<quint16>(sz));
  Buf buf{};
  buf.reserve(2 + sz);
  buf.insert(buf.begin(), sz_u16_be & 0x00FF);
  buf.insert(buf.begin() + 1, (sz_u16_be & 0xFF00) >> 8);
  buf.insert(buf.end(), payload.begin(), payload.begin() + sz);

  qCDebug(debugProtocol) << "Writing debug protocol message of size"
                         << QString::fromStdString(std::to_string(sz)) << ":"
                         << QByteArray::fromRawData(
                                reinterpret_cast<char*>(buf.data()),
                                buf.size());

  asio::write(sock, asio::buffer(buf));
}

std::optional<Reply::Reply> Connection::RecvReply() {
  // peek is discouraged, so read all bytes into our buffer first
  const size_t avail = sock.available();
  if (avail > 0) {
    recvBuf.resize(recvBuf.size() + avail);
    sock.read_some(asio::buffer(recvBuf));
  }

  // Do we at least know the message size?
  if (recvBuf.size() >= 2) {
    // Yes - is the entire message available?
    const quint16 sz_u16_be = static_cast<quint16>(recvBuf.at(0)) |
                              (static_cast<quint16>(recvBuf.at(1)) << 8);
    const quint16 sz_u16 = qFromBigEndian(sz_u16_be);
    const size_t sz = static_cast<size_t>(sz_u16);
    if ((recvBuf.size() - 2) >= sz) {
      qCDebug(debugProtocol)
          << "Read debug protocol message of size"
          << QString::fromStdString(std::to_string(sz)) << ":"
          << QByteArray::fromRawData(reinterpret_cast<char*>(recvBuf.data()),
                                     recvBuf.size());
      Reply::Reply reply{};
      bitsery::quickDeserialization<InputAdapter>({recvBuf.begin(), sz}, reply);
      recvBuf.erase(recvBuf.begin(), recvBuf.begin() + 2 + sz);
      return {reply};
    }
  }
  return {};
}
}  // namespace Dbg::Proto::Impl
