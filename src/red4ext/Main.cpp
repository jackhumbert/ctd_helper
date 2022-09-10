#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Relocation.hpp>
#include <queue>
#include <map>
#include <spdlog/spdlog.h>
#include "Utils.hpp"

struct Call {
  RED4ext::CClass *cls;
  RED4ext::CName fullName;
  RED4ext::CName shortName;
  RED4ext::CClass *parentCls;
  RED4ext::CName parentFullName;
  RED4ext::CName parentShortName;
  std::time_t callTime;
};

std::mutex queueLock;
std::map<size_t, std::deque<Call>> callQueues;

bool scriptLinkingError = false;

wchar_t errorMessage[1000] =
    L"There was an error validating redscript types with their native counterparts. Reference the mod that uses the "
    L"type(s) in the game's message below:\n";
const wchar_t *errorMessageEnd = L"\nYou can press Ctrl+C to copy this message, but it has also been written to the "
                                 L"log at red4ext/logs/ctd_helper.log";
const wchar_t *errorCaption = L"Script Type Validation Error";

// 40 55 48 83 EC 40 80 39  00 48 8B EA 0F 84 C5 00 00 00 48 89 7C 24 60 48 8B 79 18 44 8B 47 0C 44
void __fastcall DebugPrint(uintptr_t, RED4ext::CString *);
constexpr uintptr_t DebugPrintAddr = 0xA885B0;
decltype(&DebugPrint) DebugPrint_Original;

void __fastcall DebugPrint(uintptr_t a1, RED4ext::CString *a2) {
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
// 48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 20 55 48 8D 6C 24 A9 48 81 EC B0 00 00 00 0F B6 D9 0F
uintptr_t __fastcall ShowMessageBox(char, char);
constexpr uintptr_t ShowMessageBoxAddr = 0xA704D0;
decltype(&ShowMessageBox) ShowMessageBox_Original;

uintptr_t __fastcall ShowMessageBox(char a1, char a2) {
  if (scriptLinkingError) {
    swprintf(errorMessage, 1000, L"%s\n%s", errorMessage, errorMessageEnd);
    MessageBoxW(0, errorMessage, errorCaption, MB_SYSTEMMODAL | MB_ICONERROR);
    return 1;
  } else {
    return ShowMessageBox_Original(a1, a2);
  }
}

// 48 83 EC 40 48 8B 02 4C 8B F2 44 0F B7 7A 60
// 1.6  RVA:  0x27A410
// 48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 40 48 8B 02 4C
void __fastcall CallFunc(RED4ext::IScriptable *, RED4ext::CStackFrame *stackFrame, uintptr_t, uintptr_t);
constexpr uintptr_t CallFuncAddr = 0x27A410;
decltype(&CallFunc) CallFunc_Original;

void __fastcall CallFunc(RED4ext::IScriptable *context, RED4ext::CStackFrame *stackFrame, uintptr_t a3, uintptr_t a4) {
  auto func = *reinterpret_cast<RED4ext::CBaseFunction **>(stackFrame->code + 4);
  if (func) {
    auto call = new Call();
    call->cls = func->GetParent();
    if (context && context->ref.instance == context) {
      call->parentCls = context->GetType();
    }
    call->fullName = func->fullName;
    call->shortName = func->shortName;
    if (stackFrame->func) {
      auto parent = reinterpret_cast<RED4ext::CBaseFunction *>(stackFrame->func);
      call->parentFullName = parent->fullName;
      call->parentShortName = parent->shortName;
    }
    call->callTime = std::time(0);

    auto thread = std::this_thread::get_id();
    auto hash = std::hash<std::thread::id>()(thread);

    std::lock_guard<std::mutex> lock(queueLock);
    if (!callQueues.contains(hash)) {
      callQueues.insert_or_assign(hash, std::deque<Call>());
    }
    auto queue = callQueues.find(hash);
    queue->second.emplace_back(*call);
    while (queue->second.size() > 8) {
      queue->second.pop_front();
    }
  }
  CallFunc_Original(context, stackFrame, a3, a4);
}

// 48 8D 68 A1 48 81 EC A0 00 00 00 0F B6 F1
// 1.6 RVA: 0x2B93EF0 / 45694704
// 48 8B C4 55 56 57 48 8D 68 A1 48 81 EC A0 00 00 00 0F B6 F1 48 8B FA 48 8B 0D AA 5B 10 02 48 83
void __fastcall CrashFunc(uint8_t a1, uintptr_t a2);
constexpr uintptr_t CrashFuncAddr = 0x2B93EF0;
decltype(&CrashFunc) CrashFunc_Original;

void __fastcall CrashFunc(uint8_t a1, uintptr_t a2) {
  spdlog::error("Crash! Last called functions in each thread:");
  for (auto &queue : callQueues) {
    spdlog::error("Thread hash: {0}", queue.first);
    uint64_t last = 0;
    uint64_t lastParent = 0;
    for (auto i = 0; queue.second.size(); i++) {
      auto call = queue.second.front();
      if (call.parentCls && call.parentShortName) {
        uint64_t parent = call.parentCls->GetName() ^ call.parentShortName;
        if (parent == last) {
          lastParent = last;
        } else if (lastParent != parent) {
          lastParent = parent;
          spdlog::error("  {0}::{1}", call.parentCls->GetName().ToString(), call.parentShortName.ToString());
        }
      } else {
        lastParent = 0;
      }
      if (call.cls) {
        last = call.cls->GetName() ^ call.shortName;
        spdlog::error("{0} {1}- {2}::{3}", i + 1, lastParent == last ? "  " : "", call.cls->GetName().ToString(),
                      call.shortName.ToString());
      } else {
        spdlog::error("{0} {1}- {2}", i + 1, lastParent == last ? "  " : "", call.fullName.ToString());
      }
      queue.second.pop_front();
    }
  }
  CrashFunc_Original(a1, a2);
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::PluginHandle aHandle, RED4ext::EMainReason aReason,
                                        const RED4ext::Sdk *aSdk) {
  switch (aReason) {
  case RED4ext::EMainReason::Load: {
    // Attach hooks, register RTTI types, add custom states or initalize your
    // application. DO NOT try to access the game's memory at this point, it
    // is not initalized yet.

    Utils::CreateLogger();
    spdlog::info("Starting up");

    //RED4ext::RTTIRegistrator::Add(RegisterTypes, PostRegisterTypes);

    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(DebugPrintAddr), &DebugPrint,
                                  reinterpret_cast<void **>(&DebugPrint_Original)))
      ;
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(ShowMessageBoxAddr), &ShowMessageBox,
                                  reinterpret_cast<void **>(&ShowMessageBox_Original)))
      ;
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(CallFuncAddr), &CallFunc,
                                  reinterpret_cast<void **>(&CallFunc_Original)))
      ;
    while (!aSdk->hooking->Attach(aHandle, RED4EXT_OFFSET_TO_ADDR(CrashFuncAddr), &CrashFunc,
                                  reinterpret_cast<void **>(&CrashFunc_Original)))
      ;

    break;
  }
  case RED4ext::EMainReason::Unload: {
    // Free memory, detach hooks.
    // The game's memory is already freed, to not try to do anything with it.

    spdlog::info("Shutting down");
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(DebugPrintAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(ShowMessageBoxAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(CallFuncAddr));
    aSdk->hooking->Detach(aHandle, RED4EXT_OFFSET_TO_ADDR(CrashFuncAddr));
    spdlog::shutdown();
    break;
  }
  }

  return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::PluginInfo *aInfo) {
  aInfo->name = L"CTD Helper";
  aInfo->author = L"Jack Humbert";
  aInfo->version = RED4EXT_SEMVER(0, 0, 1);
  aInfo->runtime = RED4EXT_RUNTIME_LATEST;
  aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports() { return RED4EXT_API_VERSION_LATEST; }
