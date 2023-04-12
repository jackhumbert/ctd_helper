#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Relocation.hpp>
#include <queue>
#include <map>
#include <spdlog/spdlog.h>
#include "Utils.hpp"
#include "ScriptHost.hpp"
#include "Version.hpp"
#include "Addresses.hpp"
#include "Registrar.hpp"

#define MAX_CALLS 8

struct Call {
  RED4ext::CClass *cls;
  RED4ext::CName fullName;
  RED4ext::CName shortName;
  RED4ext::CClass *parentCls;
  RED4ext::CName parentFullName;
  RED4ext::CName parentShortName;
  std::time_t callTime;
  uint32_t fileIndex;
  uint32_t unk04;

  std::string GetFunc() {
    std::string fullName(this->fullName.ToString());
    if (fullName.find(";") != -1) {
      fullName.replace(fullName.find(";"), 1, "(");
      while (fullName.find(";") != -1) {
        fullName.replace(fullName.find(";"), 1, ", ");
      }
    } else {
      fullName.append("(");
    }
    fullName.append(")");

    return fullName;
  }

  std::string GetParentFunc() {
    std::string fullName(this->parentFullName.ToString());
    if (fullName.find(";") != -1) {
      fullName.replace(fullName.find(";"), 1, "(");
      while (fullName.find(";") != -1) {
        fullName.replace(fullName.find(";"), 1, ", ");
      }
    } else {
      fullName.append("(");
    }
    fullName.append(")");

    return fullName;
  }
};

std::mutex queueLock;
std::map<size_t, std::deque<Call>> callQueues;
size_t lastThread;

bool scriptLinkingError = false;

wchar_t errorMessage[1000] =
    L"There was an error validating redscript types with their native counterparts. Reference the mod that uses the "
    L"type(s) in the game's message below:\n";
const wchar_t *errorMessageEnd = L"\nYou can press Ctrl+C to copy this message, but it has also been written to the "
                                 L"log at red4ext/logs/ctd_helper.log";
const wchar_t *errorCaption = L"Script Type Validation Error";


// 1.6  RVA: 0xA885B0
// 1.61 RVA: 0xA88980
// 1.61hf RVA: 0xA88F20
/// @pattern 40 55 48 83 EC 40 80 39  00 48 8B EA 0F 84 C5 00 00 00 48 89 7C 24 60 48 8B 79 18 44 8B 47 0C 44
void __fastcall DebugPrint(uintptr_t, RED4ext::CString *);

REGISTER_HOOK(void __fastcall, DebugPrint, uintptr_t a1, RED4ext::CString *a2) {
  spdlog::error(a2->c_str());
  const size_t strSize = strlen(a2->c_str()) + 1;
  wchar_t *wc = new wchar_t[strSize];

  mbstowcs(wc, a2->c_str(), strSize);
  swprintf(errorMessage, 1000, L"%s\n%s", errorMessage, wc);
  scriptLinkingError = true;
  DebugPrint_Original(a1, a2);
}

// 1.52 RVA: 0xA66B50 / 10906448
// 1.6  RVA: 0xA704D0
// 1.61 RVA: 0xA708A0
// 1.61hf1 RVA: 0xA70DE0
/// @pattern 48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 20 55 48 8D 6C 24 A9 48 81 EC B0 00 00 00 0F B6 D9 0F
uintptr_t __fastcall ShowMessageBox(char, char);

REGISTER_HOOK(uintptr_t __fastcall, ShowMessageBox, char a1, char a2) {
  if (scriptLinkingError) {
    swprintf(errorMessage, 1000, L"%s\n%s", errorMessage, errorMessageEnd);
    MessageBoxW(0, errorMessage, errorCaption, MB_SYSTEMMODAL | MB_ICONERROR);
    return 1;
  } else {
    return ShowMessageBox_Original(a1, a2);
  }
}

// 48 83 EC 40 48 8B 02 4C 8B F2 44 0F B7 7A 60
// 1.52 RVA: 0x27A410 / 2597904
// 1.6  RVA: 0x27E1E0 / 2613728
// 1.61 RVA: 0x27E790
// 1.61hf RVA: 0x27E810
/// @pattern 48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 40 48 8B 02 4C
void __fastcall CallFunc(RED4ext::IScriptable *, RED4ext::CStackFrame *stackFrame, uintptr_t, uintptr_t);

REGISTER_HOOK(void __fastcall, CallFunc, RED4ext::IScriptable *context, RED4ext::CStackFrame *stackFrame, uintptr_t a3, uintptr_t a4) {
  auto func = *reinterpret_cast<RED4ext::CBaseFunction **>(stackFrame->code + 4);
  auto thread = std::this_thread::get_id();
  auto hash = std::hash<std::thread::id>()(thread);
  if (func) {
    auto call = Call();
    call.cls = func->GetParent();
    if (context && context->ref.instance == context) {
      call.parentCls = context->GetType();
    }
    call.fullName = func->fullName;
    call.shortName = func->shortName;
    if (stackFrame->func) {
      auto parent = reinterpret_cast<RED4ext::CBaseFunction *>(stackFrame->func);
      call.parentFullName = parent->fullName;
      call.parentShortName = parent->shortName;
      call.fileIndex = stackFrame->func->bytecode.fileIndex;
      call.unk04 = stackFrame->func->bytecode.unk04;
    }
    call.callTime = std::time(0);

    lastThread = hash;

    std::lock_guard<std::mutex> lock(queueLock);
    if (!callQueues.contains(hash)) {
      callQueues.insert_or_assign(hash, std::deque<Call>());
    }
    auto queue = callQueues.find(hash);
    queue->second.emplace_back(call);
    while (queue->second.size() > MAX_CALLS) {
      queue->second.pop_front();
    }
  }
  CallFunc_Original(context, stackFrame, a3, a4);
}

// 48 8D 68 A1 48 81 EC A0 00 00 00 0F B6 F1
// 1.6  RVA: 0x2B93EF0 / 45694704
// 1.61 RVA: 0x2B99290
// 1.61hf RVA: 0x2B9BC70
/// @pattern 48 8B C4 55 56 57 48 8D 68 A1 48 81 EC A0
/// @nth 2/3
void __fastcall CrashFunc(uint8_t a1, uintptr_t a2);

REGISTER_HOOK(void __fastcall, CrashFunc, uint8_t a1, uintptr_t a2) {
  spdlog::error("Crash! Last called functions in each thread:");
  for (auto &queue : callQueues) {
    auto level = 0;
    std::deque<uint64_t> stack;
    auto crashing = lastThread == queue.first;
    spdlog::error("Thread hash: {0}{1}", queue.first, crashing ? " LAST EXECUTED":"");
    uint64_t last = 0;
    for (auto i = 0; queue.second.size(); i++) {
      auto call = queue.second.front();
      auto filename = "<no file>";
      auto scriptFile = ScriptHost::Get()->files.values[call.fileIndex];
      if (scriptFile) {
        filename = scriptFile->filename.c_str();
      }
      if (call.parentFullName) {
        uint64_t parent = call.parentFullName;

        if (last == 0) {
          stack.emplace_front(parent);
          spdlog::error("  Func:   {}", call.GetParentFunc());
          spdlog::error("  Class:  {}", call.parentCls->GetName().ToString());
        } else {
          if (parent == last) {
            stack.emplace_front(parent);
          } else if (stack.front() != parent) {
            stack.pop_front();
            if (stack.size() == 0 || (stack.size() > 0 && stack.front() != parent)) {
              stack.emplace_front(parent);
              spdlog::error("  Func:   {}", call.GetParentFunc());
              spdlog::error("  Class:  {}", call.parentCls->GetName().ToString());
            }
          }
        }



        //if (stack.size() > 0 && parent != stack.front()) {
        //  if (parent != last) {
        //    if (stack.size() > 0 || (stack.size() > 0 && stack.front() != last)) {
        //      stack.pop_front();
        //    }
        //  }
        //} else {
        //  stack.emplace_front(parent);
        //  spdlog::error("{}", call.GetParentFunc());
        //  spdlog::error("Class:  {}", call.parentCls->GetName().ToString());
        //}
      }
      last = call.fullName;
      auto index = fmt::format("{}", MAX_CALLS - i - 1);
      spdlog::error(" {:>{}} Func:   {}", index == "0" ? ">" : index, stack.size() * 2, call.GetFunc());
      if (call.cls) {
        spdlog::error(" {:>{}} Class:  {}", " ", stack.size() * 2, call.cls->GetName().ToString());
      }
      if (call.fileIndex != 0 && call.unk04 != 0) {
        spdlog::error(" {:>{}} Source: {}:{}", " ", stack.size() * 2, filename, call.unk04);
      }
      queue.second.pop_front();
    }
  }
  CrashFunc_Original(a1, a2);
}

// 1.6  RVA: 0x2B90C60 / 45681760
// 1.61 RVA: 0x2B96000
// 1.61hf RVA: 0x2B989E0
/// @pattern 4C 89 4C 24 20 53 55 56 57 48 83 EC 68
__int64 sub_142B90C60(const char *, int, const char *, const char *);

REGISTER_HOOK(__int64, sub_142B90C60, const char* file, int lineNum, const char * func, const char * message) {
  spdlog::error("File: {} @ Line {}", file, lineNum);
  if (func) {
    spdlog::error("  {}", func);
  }
  if (message) {
    spdlog::error("  {}", message);
  }
  return sub_142B90C60_Original(file, lineNum, func, message);
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason,
                                        const RED4ext::Sdk *aSdk) {
  switch (aReason) {
  case RED4ext::EMainReason::Load: {
    // Attach hooks, register RTTI types, add custom states or initalize your
    // application. DO NOT try to access the game's memory at this point, it
    // is not initalized yet.

    Utils::CreateLogger();
    spdlog::info("Starting up CTD Helper v{}.{}.{}", MOD_VER_MAJOR, MOD_VER_MINOR, MOD_VER_PATCH);

    auto ptr = GetModuleHandle(nullptr);
    spdlog::info("Base address: {}", fmt::ptr(ptr));

    ModModuleFactory::GetInstance().Load(aSdk, aHandle);

    break;
  }
  case RED4ext::EMainReason::Unload: {
    // Free memory, detach hooks.
    // The game's memory is already freed, to not try to do anything with it.

    spdlog::info("Shutting down");
    ModModuleFactory::GetInstance().Unload(aSdk, aHandle);
    spdlog::shutdown();
    break;
  }
  }

  return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo *aInfo) {
  aInfo->name = L"CTD Helper";
  aInfo->author = L"Jack Humbert";
  aInfo->version = RED4EXT_SEMVER(MOD_VER_MAJOR, MOD_VER_MINOR, MOD_VER_PATCH);
  aInfo->runtime = RED4EXT_RUNTIME_LATEST;
  aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports() { return RED4EXT_API_VERSION_LATEST; }
