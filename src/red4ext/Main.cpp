#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Relocation.hpp>
#include <filesystem>
#include <fstream>
#include <queue>
#include <map>
#include <spdlog/spdlog.h>
#include <thread>
#include "RED4ext/ISerializable.hpp"
#include "RED4ext/InstanceType.hpp"
#include "RED4ext/RTTISystem.hpp"
#include "Utils.hpp"
#include "ScriptHost.hpp"
#include "Addresses.hpp"
#include "Registrar.hpp"
#include "Template.hpp"

#define MAX_CALLS 16

// struct Call {
//   RED4ext::CClass *cls;
//   RED4ext::CName fullName;
//   RED4ext::CName shortName;
//   RED4ext::CClass *parentCls;
//   RED4ext::CName parentFullName;
//   RED4ext::CName parentShortName;
//   std::time_t callTime;
//   uint32_t fileIndex;
//   uint32_t unk04;

//   std::string GetFuncName() {
//     std::string fullName(this->fullName.ToString());
//     if (fullName.find(";") != -1) {
//       fullName.replace(fullName.find(";"), 1, "(");
//       while (fullName.find(";") != -1) {
//         fullName.replace(fullName.find(";"), 1, ", ");
//       }
//     } else {
//       fullName.append("(");
//     }
//     fullName.append(")");

//     return fullName;
//   }

//   std::string GetParentFuncName() {
//     std::string fullName(this->parentFullName.ToString());
//     if (fullName.find(";") != -1) {
//       fullName.replace(fullName.find(";"), 1, "(");
//       while (fullName.find(";") != -1) {
//         fullName.replace(fullName.find(";"), 1, ", ");
//       }
//     } else {
//       fullName.append("(");
//     }
//     fullName.append(")");

//     return fullName;
//   }
// };

struct BaseFunction {
  uint8_t raw[sizeof(RED4ext::CBaseFunction)];
};

struct FuncCall {
  BaseFunction func;
  RED4ext::CClass *type;
  std::vector<FuncCall> children;
  RED4ext::CClass *contextType;
  RED4ext::IScriptable * context;
  RED4ext::CString contextString;

  RED4ext::CBaseFunction * get_func() {
    return reinterpret_cast<RED4ext::CBaseFunction*>(&this->func);
  }
  
  std::string GetFuncName() {
    std::string fullName(this->get_func()->fullName.ToString());
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

struct CallPair {
  FuncCall self;
  FuncCall parent;
  RED4ext::WeakHandle<RED4ext::ISerializable> context;
  bool cameBack = false;
};

std::mutex queueLock;
// std::map<std::string, std::deque<Call>> callQueues;
std::map<std::string, std::deque<CallPair>> funcCallQueues;
std::string lastThread;

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

CallPair* Invoke(RED4ext::IScriptable *context, RED4ext::CStackFrame *stackFrame, uintptr_t a3, uintptr_t a4) {
  auto func = *reinterpret_cast<RED4ext::CBaseFunction **>(stackFrame->code + 4);
  // auto thread = std::this_thread::get_id();
  // auto hash = std::hash<std::thread::id>()(thread);
  wchar_t * thread_name;
  HRESULT hr = GetThreadDescription(GetCurrentThread(), &thread_name);
  CallPair * pair_p = nullptr;
  if (func) {
    auto call = CallPair();
    call.self.func = *reinterpret_cast<BaseFunction*>(func);
    call.self.type = func->GetParent();
    if(stackFrame->func) {
      call.parent.func = *reinterpret_cast<BaseFunction*>(stackFrame->func);
      call.parent.type = stackFrame->func->GetParent();
    }
    if (context && context->ref.instance == context) {
      call.self.contextType = call.parent.contextType = context->GetType();
      // call.context.instance = context;
      // call.context.refCount = context->ref.refCount;
      if (call.self.contextType) {
        call.self.contextType->ToString(context, call.self.contextString);
        call.parent.contextType->ToString(context, call.parent.contextString);
      }
    }
    std::wstring ws(thread_name);
    lastThread = std::string(ws.begin(), ws.end());

    std::lock_guard<std::mutex> lock(queueLock);
    if (!funcCallQueues.contains(lastThread)) {
      funcCallQueues.insert_or_assign(lastThread, std::deque<CallPair>());
    }
    auto queue = funcCallQueues.find(lastThread);
    pair_p = &queue->second.emplace_back(call);
    while (queue->second.size() > MAX_CALLS) {
      auto old = queue->second.front();
      if (old.context && old.context.refCount) {
        old.context.refCount->DecRef();
      }
      queue->second.pop_front();
    }
  }
  return pair_p;
}
/// @pattern 48 8B 02 48 83 C0 13 48 89 02 44 0F B6 10 48 FF C0 48 89 02 41 8B C2 4C 8D 15 ? ? ? 03 49 FF
void __fastcall Breakpoint(RED4ext::IScriptable *context, RED4ext::CStackFrame *stackFrame, uintptr_t a3, uintptr_t a4);

REGISTER_HOOK(void __fastcall, Breakpoint, RED4ext::IScriptable *context, RED4ext::CStackFrame *stackFrame, uintptr_t a3, uintptr_t a4) {
  spdlog::info("Redscript breakpoint encountered");
  __debugbreak();
  Breakpoint_Original(context, stackFrame, a3, a4);
}

// 48 83 EC 40 48 8B 02 4C 8B F2 44 0F B7 7A 60
// 1.52 RVA: 0x27A410 / 2597904
// 1.6  RVA: 0x27E1E0 / 2613728
// 1.61 RVA: 0x27E790
// 1.61hf RVA: 0x27E810
/// @pattern 48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 40 48 8B 02 4C
void __fastcall InvokeStatic(RED4ext::IScriptable *, RED4ext::CStackFrame *stackFrame, uintptr_t, uintptr_t);

REGISTER_HOOK(void __fastcall, InvokeStatic, RED4ext::IScriptable *context, RED4ext::CStackFrame *stackFrame, uintptr_t a3, uintptr_t a4) {
  auto pair_p = Invoke(context, stackFrame, a3, a4);
  InvokeStatic_Original(context, stackFrame, a3, a4);
  if (pair_p) {
    pair_p->cameBack = true;
  }
}

/// @pattern 48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 40 48 8B 02 48
void __fastcall InvokeVirtual(RED4ext::IScriptable *, RED4ext::CStackFrame *stackFrame, uintptr_t, uintptr_t);

REGISTER_HOOK(void __fastcall, InvokeVirtual, RED4ext::IScriptable *context, RED4ext::CStackFrame *stackFrame, uintptr_t a3, uintptr_t a4) {
  auto pair_p = Invoke(context, stackFrame, a3, a4);
  InvokeStatic_Original(context, stackFrame, a3, a4);
  if (pair_p) {
    pair_p->cameBack = true;
  }
}

std::unordered_map<std::filesystem::path, std::vector<std::string>> files;

void encode_html(std::string& data) {
    std::string buffer;
    buffer.reserve(data.size());
    for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(&data[pos], 1); break;
        }
    }
    data.swap(buffer);
}

#define LINES_BEFORE_TO_PRINT 2
#define LINES_AFTER_TO_PRINT 5

// void print_source(std::ofstream& htmlLog, uint32_t file_idx, uint32_t line_idx) {
void print_source(std::ofstream& htmlLog, uint32_t file_idx, uint32_t line_idx, std::string func) {
  auto scriptFile = *ScriptHost::Get()->files.Get(file_idx);
  if (scriptFile) {
    auto path = std::filesystem::path(scriptFile->filename.c_str());
    auto is_red = false;
    if(path.is_relative()) {
      path = Utils::GetRootDir() / "tools" / "redmod" / "scripts" / path;
      is_red = true;
    }
    auto rel_path = std::filesystem::relative(path, Utils::GetRootDir());
    htmlLog << "<div class='source'>" << std::endl;
    if (std::filesystem::exists(path)) {
      if (!files.contains(path)) {
        std::ifstream file(path);
        std::string line;
        while (std::getline(file, line)) {
          files[path].emplace_back(line);
        }
        file.close();
      }
      if (is_red) {
        for (int idx = 0; idx < files[path].size(); ++idx) {
          if (files[path][idx].find(func.c_str()) != std::string::npos) {
            line_idx = idx;
            break;
          }
        }
      }
      auto line_index = line_idx;
      htmlLog << fmt::format("<a href='{}'>{}:{}</a>", path.string().c_str(), rel_path.string().c_str(), line_idx) << std::endl;
      if (files[path].size() > line_index) {
        htmlLog << fmt::format("<pre><code class='language-swift' data-ln-start-from='{}'>", line_idx - LINES_BEFORE_TO_PRINT);
        for (int i = -LINES_BEFORE_TO_PRINT; i <= LINES_AFTER_TO_PRINT; i++) {
          if (files[path].size() > (line_index + i)) {
            auto code = files[path][line_index + i];
            encode_html(code);
            htmlLog << code << std::endl;
          }
        }
        htmlLog << fmt::format("</code></pre>") << std::endl;
      } else {
        spdlog::warn("Line number exceded file: {}:{}", path.string().c_str(), line_idx + 1);
      }
    } else {
      htmlLog << fmt::format("<a href='{}'>{}:{}</a>", path.string().c_str(), rel_path.string().c_str(), line_idx) << std::endl;
      spdlog::warn("Could not locate file: {}", path.string().c_str());
    }
    htmlLog << "</div>" << std::endl;
  }
}

FuncCall * FindFunc(std::vector<FuncCall>& map, RED4ext::CName key) {
  if (auto it = find_if(map.begin(), map.end(), [&key](FuncCall& obj) {
    return obj.get_func()->fullName == key;
  }); it != map.end()) {
    return it._Ptr;
  } else {
    for (auto& value : map) {
      if (auto func = FindFunc(value.children, key); func != nullptr) {
        return func;
      }
    }
    return nullptr;
  }
}

auto rtti = RED4ext::CRTTISystem::Get();

void PrintCall(std::ofstream& htmlLog, FuncCall& call) {
  htmlLog << "<div class='call'>" << std::endl;
  if (call.contextString.Length()) {
    htmlLog << "<details>\n<summary>";
  }
  htmlLog << "<span class='call-name'>";
  if (call.type) {
    htmlLog << fmt::format("{}::", rtti->ConvertNativeToScriptName(call.type->GetName()).ToString());
  }
  htmlLog << fmt::format("{}", call.GetFuncName());
  htmlLog << "</span>" << std::endl;
  if (call.contextString.Length()) {
    htmlLog << "</summary>" << std::endl;
    htmlLog << fmt::format("<pre><code>{}</code></pre>", call.contextString.c_str()) << std::endl;
    htmlLog << "</details>" << std::endl;
  }
  if (call.get_func()->bytecode.fileIndex != 0 || call.get_func()->bytecode.unk04 != 0) {
    print_source(htmlLog, call.get_func()->bytecode.fileIndex, call.get_func()->bytecode.unk04, call.GetFuncName());
  }
  for (auto& child : call.children) {
    PrintCall(htmlLog, child);
  }
  htmlLog << "</div>" << std::endl;
}

// 48 8D 68 A1 48 81 EC A0 00 00 00 0F B6 F1
// 1.6  RVA: 0x2B93EF0 / 45694704
// 1.61 RVA: 0x2B99290
// 1.61hf RVA: 0x2B9BC70
/// @pattern 48 8B C4 55 56 57 48 8D 68 A1 48 81 EC A0
/// @nth 2/3
void __fastcall CrashFunc(uint8_t a1, uintptr_t a2);

REGISTER_HOOK(void __fastcall, CrashFunc, uint8_t a1, uintptr_t a2) {

  time_t     now = time(0);
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S.html", &tstruct);

  spdlog::error("Crash! Check {} for details", buf);
  auto ctd_helper_dir = Utils::GetRootDir() / "red4ext" / "logs" / "ctd_helper";
  std::filesystem::create_directories(ctd_helper_dir);
  std::ofstream htmlLog;
  htmlLog.open(ctd_helper_dir / buf);
  htmlLog << CTD_HELPER_HEADER;

  std::map<std::string, std::vector<FuncCall>> orgd;

  for (auto &queue : funcCallQueues) {
    auto thread = queue.first;
    for (auto i = 0; queue.second.size(); i++) {
      auto call = queue.second.front();
      if (!call.cameBack) { // || (call.context.instance && call.context.refCount)) {
          call.self.type->ToString(call.context.instance, call.self.contextString);
          call.parent.contextString = call.self.contextString;
      }
      if (orgd[thread].empty()) {
        call.parent.children.emplace_back(call.self);
        orgd[thread].emplace_back(call.parent);
      } else {
        if (auto func = FindFunc(orgd[thread], call.parent.get_func()->fullName); func != nullptr) {
          func->children.emplace_back(call.self);
        } else {
          call.parent.children.emplace_back(call.self);
          orgd[thread].emplace_back(call.parent);
        }
      }
      queue.second.pop_front();
    }
  }

  for (auto &queue : orgd) {
    auto level = 0;
    std::deque<uint64_t> stack;
    auto crashing = lastThread == queue.first;
    htmlLog << fmt::format("<div class='thread'><h1>{0}{1}</h1>", queue.first, crashing ? " LAST EXECUTED":"") << std::endl;
    uint64_t last = 0;
    for (auto& call : queue.second) {
      PrintCall(htmlLog, call);
    }
    htmlLog << "</div>" << std::endl;
  }

  htmlLog << R"(</body>
</html>)";
  htmlLog.close();

  std::filesystem::copy_file(ctd_helper_dir / buf, ctd_helper_dir / "latest.html", std::filesystem::copy_options::overwrite_existing);

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
    spdlog::info("Starting up CTD Helper v{}.{}.{}", MOD_VERSION_MAJOR, MOD_VERSION_MINOR, MOD_VERSION_PATCH);

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
  aInfo->version = RED4EXT_SEMVER(MOD_VERSION_MAJOR, MOD_VERSION_MINOR, MOD_VERSION_PATCH);
  aInfo->runtime = RED4EXT_RUNTIME_LATEST;
  aInfo->sdk = RED4EXT_SDK_LATEST;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports() { return RED4EXT_API_VERSION_LATEST; }
