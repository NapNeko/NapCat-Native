#include <v8.h>
#include <windows.h>
#include <node.h>

namespace NapCatNative {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

void OpenElectronEnv(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  SetEnvironmentStringsW(L"ELECTRON_RUN_AS_NODE=1");
  args.GetReturnValue().Set(String::NewFromUtf8(
      isolate, "Set").ToLocalChecked());
}

void Initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "OpenElectronEnv", OpenElectronEnv);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

} 