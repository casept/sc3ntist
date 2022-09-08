#include "debugprotocol_debugger.h"
#include "bitsery/deserializer.h"
#include "debugprotocol.h"

#include <asio.hpp>
#include <bitsery/adapter/buffer.h>
#include <bitsery/brief_syntax/vector.h>
#include <QtEndian>

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

using asio::ip::tcp;
using Buf = std::vector<uint8_t>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buf>;
using InputAdapter = bitsery::InputBufferAdapter<Buf>;

namespace DbgProto::Debugger {
Connection::Connection(const char* addr, uint16_t port)
    : ctx({}), sock(ctx), recvBuf({}) {
  tcp::resolver resolv(ctx);
  auto endpoints = resolv.resolve(addr, std::to_string(port));
  sock = tcp::socket(ctx);
  asio::connect(sock, endpoints);
  // Reduce latency
  asio::socket_base::send_buffer_size bufsize(32);
  sock.set_option(bufsize);
}

void Connection::SendCmd(const Cmd::Cmd& cmd) {
  Buf buf{};
  const size_t sz = bitsery::quickSerialization<OutputAdapter>(buf, cmd);
  // Prepend size
  if (sz > UINT16_MAX) {
    throw std::runtime_error("Connection::SendCmd() failed: Message too large");
  }
  const quint16 sz_u16_be = qToBigEndian(static_cast<quint16>(sz));
  buf.insert(buf.begin(), sz_u16_be & 0x00FF);
  buf.insert(buf.begin(), (sz_u16_be & 0xFF00) >> 8);

  asio::write(sock, asio::buffer(buf));
}

std::optional<Reply::Reply> Connection::RecvReply() {
  // peek is discouraged, so read all bytes into our buffer first
  if (sock.available() > 0) {
    sock.read_some(asio::buffer(recvBuf));
  }
  // Do we at least know the message size?
  if (recvBuf.size() >= 2) {
    // Yes - is the entire message available?
    const quint16 sz_u16_be = static_cast<quint16>(recvBuf.at(0)) |
                              (static_cast<quint16>(recvBuf.at(1)) >> 8);
    const quint16 sz_u16 = qFromBigEndian(sz_u16_be);
    const size_t sz = static_cast<size_t>(sz_u16);
    if ((recvBuf.size() - 2) >= sz) {
      Reply::Reply reply{};
      bitsery::quickDeserialization<InputAdapter>({recvBuf.begin(), sz}, reply);
      recvBuf.erase(recvBuf.begin(), recvBuf.begin() + 2 + sz);
      return {reply};
    }
  }
  return {};
}
}  // namespace DbgProto::Debugger