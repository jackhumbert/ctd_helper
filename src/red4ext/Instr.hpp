#pragma once

#include <RED4ext/Common.hpp>

namespace RED4ext::Instr {

#pragma pack(push, 1)
struct Invoke {
  uint16_t exitLabel;
  uint16_t lineNumber;
};

struct InvokeStatic : Invoke
{
  RED4ext::CScriptedFunction * func;
  uint16_t paramFlags;
};

struct InvokeVirtual : Invoke
{
  RED4ext::CName funcName;
  uint16_t paramFlags;
};
#pragma pack(pop, 1)

}