#pragma once

#include <cstdint>
#include <variant>

#include <bitsery/bitsery.h>
#include <bitsery/brief_syntax/variant.h>
#include <bitsery/brief_syntax.h>

/*
This header defines the debug protocol for talking to a remote VM's types.
It only defines the wire format, it's not a complete implementation.
The messages are sent over TCP, the debugger connects to the target.

Note that each message must also be prepended with the length of serialized data
as a big-endian uint16_t, which can't be done in the `serialize` implementations
because the size of the entire message is only known after serialization
completes.

It's designed such as to keep the target as simple as possible,
loading all the complexity into the debugger instead.
This is because there are multiple targets planned (impacto and the official
engine), but only one debugger.
*/

namespace DbgProto {

using ThreadID = uint8_t;

/** Debugger -> Target **/
namespace Cmd {
enum class Type : uint8_t {
  /// Get the instruction pointer.
  GetIP,
  /// Get a 32-bit thread-local variable.
  GetLocalVar,
  /// Get a 32-bit global variable.
  GetGlobalVar,
  /// Get a global flag.
  GetGlobalFlag,
  /// Break execution.
  BreakNow,
  /// Continue execution.
  Continue,
  // TODO: Breakpoints, watchpoints
};

struct GetIP {
  ThreadID tid;

  template <typename S>
  void serialize(S& s) {
    s(tid);
  };
};

struct GetLocalVar {
  ThreadID tid;
  uint32_t whichVar;

  template <typename S>
  void serialize(S& s) {
    s(tid, whichVar);
  };
};

struct GetGlobalVar {
  uint32_t whichVar;

  template <typename S>
  void serialize(S& s) {
    s(whichVar);
  };
};
;

struct GetGlobalFlag {
  uint32_t whichFlag;

  template <typename S>
  void serialize(S& s) {
    s(whichFlag);
  };
};

struct BreakNow {
  ThreadID tid;

  template <typename S>
  void serialize(S& s) {
    s(tid);
  };
};

struct Continue {
  ThreadID tid;

  template <typename S>
  void serialize(S& s) {
    s(tid);
  };
};

struct Cmd {
  Type type;
  std::variant<GetIP, GetLocalVar, GetGlobalVar, GetGlobalFlag, BreakNow,
               Continue>
      cmd;

  template <typename S>
  void serialize(S& s) {
    s(type, cmd);
  };
};
}  // namespace Cmd

/** Target -> Debugger **/

namespace Reply {

struct GetIP {
  uint32_t ip;

  template <typename S>
  void serialize(S& s) {
    s(ip);
  };
};

enum class Type : uint8_t {
  GetIP,
};

struct Reply {
  Type type;
  std::variant<GetIP> reply;

  template <typename S>
  void serialize(S& s) {
    s(type, reply);
  };
};

}  // namespace Reply
}  // namespace DbgProto