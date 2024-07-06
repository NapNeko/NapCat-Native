#include <v8.h>
#include <node.h>
#include <WinSock2.h>
#include <Windows.h>
#include <uv.h>
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
        node::CreateEnvironment(isolate_data.get(), context, args, exec_args),
        node::FreeEnvironment);
    v8::MaybeLocal<v8::Value> loadenv_ret = node::LoadEnvironment(
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

int NapCatBoot(int argc, char **argv)
{
  argv = uv_setup_args(argc, argv);
  std::vector<std::string> args(argv, argv + argc);
  std::vector<std::string> exec_args;
  std::vector<std::string> errors;
  int exit_code = node::InitializeNodeWithArgs(&args, &exec_args, &errors);
  for (const std::string &error : errors)
    fprintf(stderr, "%s: %s\n", args[0].c_str(), error.c_str());
  if (exit_code != 0)
  {
    return exit_code;
  }

  std::unique_ptr<node::MultiIsolatePlatform> platform = node::MultiIsolatePlatform::Create(4);
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  // 此函数的内容见下文。
  int ret = RunNodeInstance(platform.get(), args, exec_args);
  // 不用回收了 直接继续
  v8::V8::Dispose();
  // v8::V8::ShutdownPlatform();
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
int NapCatWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  int pNumArgs = 0;   // argc
  char str[MAX_PATH]; // argv
  NapCatRouterConsole();
  LPWSTR CommandLineW = GetCommandLineW();
  auto lpszArgv = CommandLineToArgvW(CommandLineW, &pNumArgs);

  for (int i = 0; i < pNumArgs; i++)
  {

    memset(str, 0, MAX_PATH);
    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpszArgv[i], -1, str, 200, NULL, NULL);
  }
  return NapCatBoot(pNumArgs, (char **)&str);
}