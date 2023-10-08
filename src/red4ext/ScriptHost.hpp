#include <RED4ext/RED4ext.hpp>
#include "Addresses.hpp"

struct ScriptFile {
  // Murmur3
  uint32_t hash1;
  uint32_t hash2;
  RED4ext::CString filename;
  RED4ext::DynArray<void*> unk28;
  RED4ext::DynArray<void*> unk38;
  uint32_t unk48;
};

// CRC32B
enum EScriptAction : uint32_t
{
  ScriptProcessInfo = 0x260B1475,
  ScriptBreakpointUnbound = 0xD416A296,
  ScriptBreakpointConfirmation = 0xF5CECB4B,
  ScriptBinaryReload = 0xD3CDA57D,
};

struct IScriptAction {
  virtual void * sub_00();
  virtual void * sub_08();
  virtual void * sub_10();
  virtual void * sub_18();
  virtual bool IsActionType(EScriptAction type);

  uint32_t id;  // 08
  uint32_t unk0C; // 0C
};

struct ScriptBreakpointConfirmation : IScriptAction {

};

struct ScriptBinaryReloaded : IScriptAction {

};

struct ScriptBreakRequest : IScriptAction
{
  RED4ext::CString type;
  uint32_t thread;
};


enum EBreakpointState : unsigned __int8 {
  Continue = 0x0,
  StepOver = 0x1,
  StepInto = 0x2,
  StepOut = 0x3,
  Pause = 0x4,
};

// 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B FA 48 8B DA 48 C1 EF 02 48 8B F1 48 85 FF 75
/// @pattern 48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 4C 8B DA 48 8B F2 8B FA 49 C1 EB 02 49 8B D3 48 8B
uint32_t __fastcall Murmur3_32(char *a1, unsigned __int64 length) {
  RED4ext::RelocFunc<uint32_t (*)(char *a1, unsigned __int64 length)> func(Murmur3_32_Addr);
  return func(a1, length);
}

struct ScriptInterface {
  /// @pattern 50 6F 6F 6C 48 54 54 50 00 00 00 00 00 00 00
  /// @offset -0x10
  static constexpr const uintptr_t VFT = ScriptInterface_VFT_Addr;

  virtual ~ScriptInterface() = default;
  virtual bool sub_08(IScriptAction ** scriptAction, void* debugger);
  
  RED4ext::DynArray<void *> unk10;
  uint32_t unk20; // state?
  uint16_t unk24;
  uint16_t unk26;
  uint64_t unk28;
  RED4ext::Map<uint32_t, ScriptFile *> files;
  uint32_t breakpointThread;
  EBreakpointState breakpointState;
  uint8_t unk5D;
  uint8_t unk5E;
  uint8_t unk5F;
};
RED4EXT_ASSERT_OFFSET(ScriptInterface, files, 0x28);

struct ScriptHost {
  /// @pattern 50 6F 6F 6C 48 54 54 50 00 00 00 00 00 00 00
  /// @offset -0x20
  static constexpr const uintptr_t VFT = ScriptHost_VFT_Addr;

  virtual inline void sub_00() {}; // empty
  virtual inline void sub_08() {}; // load
  virtual inline void sub_10() {};
  virtual inline void sub_18() {};
  virtual inline void sub_20() {};

  // something with files
  virtual inline void sub_28() {};

  virtual inline void sub_30() {}; // empty

  // if unk20 == 0
  virtual inline void sub_38() {};

  // sets unk24
  virtual inline void sub_40() {};

  // gets unk24, called by StartProfiling
  virtual inline void sub_48() {};

  // something with (), global exec|native functions
  virtual inline bool sub_50(RED4ext::CString * a1) {
    RED4ext::RelocFunc<decltype(&ScriptHost::sub_50)> call(VFT, 0x50);
    return call(this, a1);
  };

  // something else with (), scripted functions, exec || event
  virtual inline bool sub_58(RED4ext::IScriptable * aContext, RED4ext::CString * a2) {
    RED4ext::RelocFunc<decltype(&ScriptHost::sub_58)> call(VFT, 0x58);
    return call(this, aContext, a2);
  };

  virtual inline void sub_60() {};
  virtual inline void sub_68() {};
  virtual inline void sub_70() {};
  virtual inline void sub_78() {};
  virtual inline void sub_80() {};
  virtual inline void sub_88() {};
  virtual inline void sub_90() {};
  virtual inline void sub_98() {};

  // 1.6  RVA: 0x26BA70 / 2538096
  // 1.62 RVA: 0x26C0A0 / 2539680
  /// @pattern 48 83 EC 28 65 48 8B 04 25 58 00 00 00 BA 10 00 00 00 48 8B 08 8B 04 0A 39 05 36 97 6A 03 7F 0C
  static ScriptHost * Get() {
    RED4ext::RelocFunc<decltype(&ScriptHost::Get)> call(ScriptHost_Get_Addr);
    return call();
  };

  ScriptInterface interface;
  uint64_t unk60;
  RED4ext::HashMap<uint64_t, uint64_t> unk68;
  RED4ext::SharedMutex unk68MUTX;
  uint8_t unk99;
  uint8_t unk9A;
  uint8_t unk9B;
  uint8_t unk9C;
  uint8_t unk9D;
  uint8_t unk9E;
  uint8_t unk9F;
  void *psa;
};
 //char (*__kaboom)[offsetof(ScriptHost, unk10)] = 1;

//const uintptr_t ScriptsHost_p = 0x3F17738;
