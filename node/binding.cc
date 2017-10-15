#include "kcpuv_sess.h"
#include <iostream>
#include <nan.h>
#include <node.h>
#include <uv.h>
#include <v8.h>

namespace kcpuv_addons {
using std::cout;
using v8::Context;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::Isolate;
using v8::Local;
using v8::MaybeLocal;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;

class KcpuvSessBinding : public Nan::ObjectWrap {
public:
  explicit KcpuvSessBinding() {
    sess = kcpuv_create();
    sess->data = this;
  }
  virtual ~KcpuvSessBinding() {}

  static Persistent<Function> constructor;
  static void Create(const FunctionCallbackInfo<Value> &args);

  void Free() {
    kcpuv_free(sess);
    is_freed = 1;
  };

  kcpuv_sess *GetSess() { return sess; }

  Handle<Function> js_cb;

private:
  int is_freed = 0;
  kcpuv_sess *sess;
};

Persistent<Function> KcpuvSessBinding::constructor;

void KcpuvSessBinding::Create(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();

  if (args.IsConstructCall()) {
    KcpuvSessBinding *binding = new KcpuvSessBinding();
    binding->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    const int argc = 0;
    Local<Value> argv[argc] = {};

    Local<Function> cons =
        Local<Function>::New(isolate, KcpuvSessBinding::constructor);
    Local<Context> context = isolate->GetCurrentContext();
    // TODO: ToLocalChecked
    Local<Object> instance =
        cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(instance);
  }
}

static NAN_METHOD(Initialize) { kcpuv_initialize(); }

static NAN_METHOD(Destruct) { kcpuv_destruct(); }

static NAN_METHOD(Free) {
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());
  obj->Free();
}

// TODO:
void binding_cb(kcpuv_sess *sess, char *data, int len) {
  KcpuvSessBinding *binding = static_cast<KcpuvSessBinding *>(sess->data);
  Handle<Function> js_cb = binding->js_cb;

  Isolate *isolate = Isolate::GetCurrent();
  Local<Context> context = isolate->GetCurrentContext();
  Handle<Value> args[0];
  MaybeLocal<Value> js_res = js_cb->Call(context, context->Global(), 0, args);
}

static NAN_METHOD(Listen) {
  Isolate *isolate = info.GetIsolate();

  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());
  int port = static_cast<double>(info[1]->ToNumber(isolate)->Value());

  if (!info[2]->IsFunction()) {
    isolate->ThrowException(Exception::Error(
        String::NewFromUtf8(isolate, "expect a callback function")));
    return;
  }

  obj->js_cb = Handle<Function>::Cast(info[2]);
  kcpuv_sess *sess = obj->GetSess();
  kcpuv_listen(sess, port, &binding_cb);
}

static NAN_METHOD(InitSend) {
  Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());
  String::Utf8Value utf8Str(info[1]->ToString());
  char *addr = *utf8Str;
  int port = static_cast<double>(info[2]->ToNumber(isolate)->Value());

  kcpuv_init_send(obj->GetSess(), addr, port);
}

static NAN_MODULE_INIT(Init) {
  Isolate *isolate = target->GetIsolate();

  Local<FunctionTemplate> tpl =
      FunctionTemplate::New(isolate, KcpuvSessBinding::Create);
  tpl->SetClassName(String::NewFromUtf8(isolate, "KcpuvSessBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  KcpuvSessBinding::constructor.Reset(isolate, tpl->GetFunction());

  Nan::Set(target, String::NewFromUtf8(isolate, "create"), tpl->GetFunction());
  Nan::SetMethod(target, "initialize", Initialize);
  Nan::SetMethod(target, "destruct", Destruct);
  Nan::SetMethod(target, "free", Free);
  Nan::SetMethod(target, "listen", Listen);
  Nan::SetMethod(target, "initSend", InitSend);
}

NODE_MODULE(binding, Init)
} // namespace kcpuv_addons
