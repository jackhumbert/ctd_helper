#include <RED4ext/RED4ext.hpp>
#include "Addresses.hpp"

struct ScriptFile {
  uint32_t hash1;
  uint32_t hash2;
  RED4ext::CString filename;
  RED4ext::DynArray<void*> unk28;
  RED4ext::DynArray<void*> unk38;
  uint32_t unk48;
};

enum EBreakpointState : unsigned __int8 {
  Continue = 0x0,
  StepOver = 0x1,
  StepInto = 0x2,
  StepOut = 0x3,
  Pause = 0x4,
};

struct ScriptHost {
  // Just after "PortRange"
  // 1.6  RVA: 0x30E74C0
  // 1.61hf RVA: 0x30EF5C0
  // 1.62 RVA: 0x3126DC0
  /// @pattern 50 6F 72 74 52 61 6E 67 65 00 00 00 00 00 00 00
  /// @offset -16
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

  // gets unk24
  virtual inline void sub_48() {};

  // something with (),
  virtual inline bool sub_50(RED4ext::CString * a1) {
    RED4ext::RelocFunc<decltype(&ScriptHost::sub_50)> call(VFT, 0x50);
    return call(this, a1);
  };

  // something else with (),
  virtual inline bool sub_58(bool * a1, RED4ext::CString * a2) {
    RED4ext::RelocFunc<decltype(&ScriptHost::sub_58)> call(VFT, 0x58);
    return call(this, a1, a2);
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
  /// @pattern 40 53 48 83 EC 20 65 48 8B 04 25 58 00 00 00 8B 0D ? ? ? 04 BA 9C 07 00 00 48 8B 0C C8 8B 04
  /// @nth 25/0
  static ScriptHost * Get() {
    RED4ext::RelocFunc<decltype(&ScriptHost::Get)> call(ScriptHost_Get_Addr);
    return call();
  };

  void *vft2;
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
RED4EXT_ASSERT_OFFSET(ScriptHost, files, 0x30);
 //char (*__kaboom)[offsetof(ScriptHost, unk10)] = 1;

//const uintptr_t ScriptsHost_p = 0x3F17738;
