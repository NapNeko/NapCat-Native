#include <v8.h>
#include <node.h>
#include <WinSock2.h>
#include <Windows.h>
#include <uv.h>
#include <iostream>
#include <algorithm>
#include <codecvt>

int(__stdcall *oriWinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);
extern bool NapCatInit();
bool inited = NapCatInit();
std::unique_ptr<node::InitializationResult> (*nodeInitializeOncePerProcess)(const std::vector<std::string> &, node::ProcessInitializationFlags::Flags);

using NodeTypeCreatEnvironment = node::Environment *(*)(node::IsolateData *,
                                                        v8::Local<v8::Context>,
                                                        const std::vector<std::string> &,
                                                        const std::vector<std::string> &,
                                                        node::EnvironmentFlags::Flags, node::ThreadId,
                                                        std::unique_ptr<node::InspectorParentHandle>);
using NodeTypeLoadEnvironment = v8::MaybeLocal<v8::Value> (*)(node::Environment *env, std::string_view main_script_source_utf8);

NodeTypeCreatEnvironment node_create_environment = nullptr;
NodeTypeLoadEnvironment node_load_environment = nullptr;
void test()
{
  node_create_environment = (NodeTypeCreatEnvironment)GetProcAddress(GetModuleHandle(0), "?CreateEnvironment@node@@YAPEAVEnvironment@1@PEAVIsolateData@1@V?$Local@VContext@v8@@@v8@@AEBV?$vector@V?$basic_string@DU?$char_traits@D@__Cr@std@@V?$allocator@D@23@@__Cr@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@__Cr@std@@V?$allocator@D@23@@__Cr@std@@@23@@__Cr@std@@2W4Flags@EnvironmentFlags@1@UThreadId@1@V?$unique_ptr@UInspectorParentHandle@node@@U?$default_delete@UInspectorParentHandle@node@@@__Cr@std@@@78@@Z");
  node_load_environment = (NodeTypeLoadEnvironment)GetProcAddress(GetModuleHandle(0), "?LoadEnvironment@node@@YA?AV?$MaybeLocal@VValue@v8@@@v8@@PEAVEnvironment@1@V?$function@$$A6A?AV?$MaybeLocal@VValue@v8@@@v8@@AEBUStartExecutionCallbackInfo@node@@@Z@__Cr@std@@V?$function@$$A6AXPEAVEnvironment@node@@V?$Local@VValue@v8@@@v8@@1@Z@67@@Z");
  GetProcAddress(GetModuleHandle(0), "?Create@ArrayBufferAllocator@node@@SA?AV?$unique_ptr@VArrayBufferAllocator@node@@U?$default_delete@VArrayBufferAllocator@node@@@__Cr@std@@@__Cr@std@@_N@Z");
  GetProcAddress(GetModuleHandle(0), "?Create@MultiIsolatePlatform@node@@SA?AV?$unique_ptr@VMultiIsolatePlatform@node@@U?$default_delete@VMultiIsolatePlatform@node@@@__Cr@std@@@__Cr@std@@HPEAVTracingController@v8@@PEAVPageAllocator@7@@Z");
  GetProcAddress(GetModuleHandle(0), "?InitializeOncePerProcess@node@@YA?AV?$unique_ptr@VInitializationResult@node@@U?$default_delete@VInitializationResult@node@@@__Cr@std@@@__Cr@std@@AEBV?$vector@V?$basic_string@DU?$char_traits@D@__Cr@std@@V?$allocator@D@23@@__Cr@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@__Cr@std@@V?$allocator@D@23@@__Cr@std@@@23@@34@W4Flags@ProcessInitializationFlags@1@@Z");
  GetProcAddress(GetModuleHandle(0), "?NewIsolate@node@@YAPEAVIsolate@v8@@V?$shared_ptr@VArrayBufferAllocator@node@@@__Cr@std@@PEAUuv_loop_s@@PEAVMultiIsolatePlatform@1@PEBVEmbedderSnapshotData@1@AEBUIsolateSettings@1@@Z");
}
extern "C" __declspec(dllexport) void StackWalk64() {}
extern "C" __declspec(dllexport) void SymCleanup() {}
extern "C" __declspec(dllexport) void SymFromAddr() {}
extern "C" __declspec(dllexport) void SymFunctionTableAccess64() {}
extern "C" __declspec(dllexport) void SymGetLineFromAddr64() {}
extern "C" __declspec(dllexport) void SymGetModuleBase64() {}
extern "C" __declspec(dllexport) void SymGetModuleInfo64() {}
extern "C" __declspec(dllexport) void SymGetSymFromAddr64() {}
extern "C" __declspec(dllexport) void SymGetSearchPathW() {}
extern "C" __declspec(dllexport) void SymInitialize() {}
extern "C" __declspec(dllexport) void SymSetOptions() {}
extern "C" __declspec(dllexport) void SymGetOptions() {}
extern "C" __declspec(dllexport) void SymSetSearchPathW() {}
extern "C" __declspec(dllexport) void UnDecorateSymbolName() {}
extern "C" __declspec(dllexport) void MiniDumpWriteDump() {}

namespace moehoo
{
  void *get_call_address(uint8_t *ptr);
  void *search_and_fill_jump(uint64_t baseAddress, void *targetAddress);
  bool hook(uint8_t *callAddr, void *lpFunction);
} // namespace moehoo

inline void *moehoo::get_call_address(uint8_t *ptr)
{
  // 读取操作码
  if (ptr[0] != 0xE8)
  {
    printf("Not a call instruction!\n");
    return 0;
  }

  // 读取相对偏移量
  int32_t relativeOffset = *reinterpret_cast<int32_t *>(ptr + 1);

  // 计算函数地址
  uint8_t *callAddress = ptr + 5; // call 指令占 5 个字节
  void *functionAddress = callAddress + relativeOffset;

  return reinterpret_cast<void *>(functionAddress);
}

inline void *moehoo::search_and_fill_jump(uint64_t baseAddress, void *targetAddress)
{
  uint8_t jumpInstruction[] = {
      0x49, 0xBB,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x41, 0xFF, 0xE3};

  memcpy(jumpInstruction + 2, &targetAddress, 8);

  // Iterate through memory regions
  uint64_t searchStart = baseAddress - 0x7fffffff;
  uint64_t searchEnd = baseAddress + 0x7fffffff;
  while (searchStart < searchEnd - sizeof(jumpInstruction))
  {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(reinterpret_cast<void *>(searchStart), &mbi, sizeof(mbi)) == 0)
      break;
    if (mbi.State == MEM_COMMIT)
    {
      for (char *addr = static_cast<char *>(mbi.BaseAddress); addr < static_cast<char *>(mbi.BaseAddress) + mbi.RegionSize - 1024 * 5; ++addr)
      {

        bool isFree = true;
        for (int i = 0; i < 1024 * 5; ++i)
        {
          if (addr[i] != 0)
          {
            isFree = false;
            break;
          }
        }
        if (isFree)
        {
          DWORD oldProtect;
          addr += 0x200;
          // printf("addr: %p\n", addr);

          if (!VirtualProtect(addr, sizeof(jumpInstruction), PAGE_EXECUTE_READWRITE, &oldProtect))
            break;
          memcpy(addr, jumpInstruction, sizeof(jumpInstruction));
          if (!VirtualProtect(addr, sizeof(jumpInstruction), PAGE_EXECUTE_READ, &oldProtect))
            break;

          return addr;
        }
      }
    }
    searchStart += mbi.RegionSize;
  }
  return nullptr;
}

inline bool moehoo::hook(uint8_t *callAddr, void *lpFunction)
{
  uint64_t startAddr = reinterpret_cast<uint64_t>(callAddr) + 5;
  int64_t distance = reinterpret_cast<uint64_t>(lpFunction) - startAddr;
  // printf("Hooking %p to %p, distance: %lld\n", callAddr, lpFunction, distance);
  DWORD oldProtect;
  DWORD reProtect;
  if (!VirtualProtect(callAddr, 10, PAGE_EXECUTE_READWRITE, &oldProtect))
  {
    printf("VirtualProtect failed\n");
    return false;
  }
  if (distance < INT32_MIN || distance > INT32_MAX)
  {
    void *new_ret = search_and_fill_jump(startAddr, lpFunction);
    if (new_ret == nullptr)
    {
      printf("Can't find a place to jump\n");
      return false;
    }
    distance = reinterpret_cast<uint64_t>(new_ret) - startAddr;
    // printf("new_ret: %p, new_distance: %lld\n", new_ret, distance);
  }
  // 直接进行小跳转
  memcpy(callAddr + 1, reinterpret_cast<int32_t *>(&distance), 4); // 修改 call 地址
  if (!VirtualProtect(callAddr, 10, oldProtect, &reProtect))       // 恢复原来的内存保护属性
  {
    std::cout << GetLastError() << "/" << callAddr << "/" << oldProtect << "/" << reProtect;
    printf("VirtualProtect failed\n");
    return false;
  }
  return true;
}
int RunNodeInstance(node::MultiIsolatePlatform *platform,
                    const std::vector<std::string> &args,
                    const std::vector<std::string> &exec_args)
{
  int exit_code = 0;
  // 设置 libuv 事件循环。
  uv_loop_t loop;
  int ret = uv_loop_init(&loop);
  if (ret != 0)
  {
    fprintf(stderr, "%s: Failed to initialize loop: %s\n",
            args[0].c_str(),
            uv_err_name(ret));
    return 1;
  }

  std::shared_ptr<node::ArrayBufferAllocator> allocator =
      node::ArrayBufferAllocator::Create();

  v8::Isolate *isolate = NewIsolate(allocator, &loop, platform);
  if (isolate == nullptr)
  {
    fprintf(stderr, "%s: Failed to initialize V8 Isolate\n", args[0].c_str());
    return 1;
  }

  {
    v8::Locker locker(isolate);
    v8::Isolate::Scope isolate_scope(isolate);

    std::unique_ptr<node::IsolateData, decltype(&node::FreeIsolateData)> isolate_data(
        node::CreateIsolateData(isolate, &loop, platform, allocator.get()),
        node::FreeIsolateData);

    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = node::NewContext(isolate);
    if (context.IsEmpty())
    {
      fprintf(stderr, "%s: Failed to initialize V8 Context\n", args[0].c_str());
      return 1;
    }

    v8::Context::Scope context_scope(context);

    std::unique_ptr<node::Environment, decltype(&node::FreeEnvironment)> env(
        node_create_environment(isolate_data.get(), context, args, exec_args, node::EnvironmentFlags::kDefaultFlags, {}, {}),
        node::FreeEnvironment);
    v8::MaybeLocal<v8::Value> loadenv_ret = node_load_environment(
        env.get(),
        "const publicRequire ="
        "  require('module').createRequire(process.cwd() + '/');"
        "globalThis.require = publicRequire;"
        "require('vm').runInThisContext(process.argv[1]);");

    if (loadenv_ret.IsEmpty()) // 出现了 JS 异常。
      return 1;

    {
      v8::SealHandleScope seal(isolate);
      bool more;
      do
      {
        uv_run(&loop, UV_RUN_DEFAULT);
        platform->DrainTasks(isolate);

        more = uv_loop_alive(&loop);
        if (more)
          continue;
        node::EmitBeforeExit(env.get());
        more = uv_loop_alive(&loop);
      } while (more == true);
    }

    exit_code = node::EmitExit(env.get());
    node::Stop(env.get());
  }

  // 取消向平台注册 Isolate 并添加监听器，
  // 当平台完成清理它与 Isolate 关联的任何状态时，
  // 则调用该监听器。
  bool platform_finished = false;
  platform->AddIsolateFinishedCallback(isolate, [](void *data)
                                       { *static_cast<bool *>(data) = true; }, &platform_finished);
  platform->UnregisterIsolate(isolate);
  isolate->Dispose();

  // 等到平台清理完所有相关资源。
  while (!platform_finished)
    uv_run(&loop, UV_RUN_ONCE);
  int err = uv_loop_close(&loop);
  assert(err == 0);

  return exit_code;
}

int NapCatBoot(int argc, char *argv[])
{
  MessageBoxA(0, *argv, std::to_string(argc).c_str(), 0);
  argv = uv_setup_args(argc, argv);
  std::vector<std::string> args(argv, argv + argc);
  // 解析 Node.js 命令行选项，
  // 并打印尝试解析它们时发生的任何错误。
  MessageBoxA(0, "123", "1234567890", 0);

  uint64_t flags_accum = node::ProcessInitializationFlags::kNoFlags;
  auto list = {node::ProcessInitializationFlags::kNoInitializeV8,
               node::ProcessInitializationFlags::kNoInitializeNodeV8Platform};
  for (const auto flag : list)
    flags_accum |= static_cast<uint64_t>(flag);
  std::unique_ptr<node::InitializationResult> result = node::InitializeOncePerProcess(args, static_cast<node::ProcessInitializationFlags::Flags>(flags_accum));

  MessageBoxA(0, "123", "123456", 0);
  std::unique_ptr<node::MultiIsolatePlatform> platform = node::MultiIsolatePlatform::Create(4);
  MessageBoxA(0, "123", "12345", 0);
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();
  MessageBoxA(0, "123", "123", 0);
  // 此函数的内容见下文。
  int ret = RunNodeInstance(
      platform.get(), result->args(), result->exec_args());
  MessageBoxA(0, "123", "1234", 0);
  v8::V8::Dispose();
  v8::V8::DisposePlatform();

  node::TearDownOncePerProcess();
  return ret;
}
bool NapCatRouterConsole()
{
  AllocConsole();
  freopen("CONOUT$", "w+t", stdout);
  freopen("CONOUT$", "w+t", stderr);
  freopen("CONIN$", "r+t", stdin);
  return true;
}
extern "C" __declspec(dllexport) int NapCatWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  NapCatRouterConsole();
  char *iagrv = new char[30]{"D:\\AppD\\QQNT\\QQ.exe --version"};
  char **argv = &iagrv;
  return NapCatBoot(1, argv);
}
bool NapCatInit()
{
  uint64_t baseAddr = 0;
  HMODULE wrapperModule = GetModuleHandleW(NULL);
  if (wrapperModule == NULL)
    throw std::runtime_error("Can't GetModuleHandle");
  baseAddr = reinterpret_cast<uint64_t>(wrapperModule);
  printf("baseAddr: %llx\n", baseAddr);
  if (baseAddr == 0)
    throw std::runtime_error("Can't find hook address");
  uint8_t *abscallptr = reinterpret_cast<uint8_t *>(baseAddr + 0x457A8AD);
  oriWinMain = reinterpret_cast<int(__stdcall *)(HINSTANCE, HINSTANCE, LPSTR, int)>(moehoo::get_call_address(abscallptr));
  if (oriWinMain == NULL)
    throw std::runtime_error("error");
  std::cout << "hook success" << std::endl;
  nodeInitializeOncePerProcess = reinterpret_cast<std::unique_ptr<node::InitializationResult> (*)(const std::vector<std::string> &, node::ProcessInitializationFlags::Flags)>(baseAddr + 0x1FFF8A1);
  return moehoo::hook(abscallptr, &NapCatWinMain);
}
