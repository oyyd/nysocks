#define NDEBUG

#include "kcpuv_sess.h"
#include <iostream>
#include <nan.h>
#include <node.h>
#include <uv.h>
#include <v8.h>

namespace kcpuv_addons {
using std::cout;
using v8::Boolean;
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

  Nan::CopyablePersistentTraits<Function>::CopyablePersistent bind_cb;
  Nan::CopyablePersistentTraits<Function>::CopyablePersistent close_cb;
  int is_freed = 0;

private:
  kcpuv_sess *sess;
};

Persistent<Function> KcpuvSessBinding::constructor;

void buffer_delete_callback(char *data, void *hint) { free(data); }

void binding_cb(kcpuv_sess *sess, char *data, int len) {
  KcpuvSessBinding *binding = static_cast<KcpuvSessBinding *>(sess->data);
  // NOTE: Create a scope for the allocation of v8 memories
  // as we are calling a js function outside the v8
  Nan::HandleScope scope;
  Isolate *isolate = Isolate::GetCurrent();

  const int argc = 1;
  Local<Value> args[argc] = {Nan::NewBuffer(data, len).ToLocalChecked()};

  Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                    Nan::New(binding->bind_cb), argc, args);
}

void closing_cb(kcpuv_sess *sess) {
  KcpuvSessBinding *binding = static_cast<KcpuvSessBinding *>(sess->data);

  Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                    Nan::New(binding->close_cb), 0, 0);
}

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

static NAN_METHOD(UseDefaultLoop) {
  Isolate *isolate = info.GetIsolate();
  bool use = info[0]->BooleanValue();
  int val = 0;
  if (use) {
    val = 1;
  }
  kcpuv_use_default_loop(val);
}

static NAN_METHOD(Initialize) { kcpuv_initialize(); }

static NAN_METHOD(Destruct) { kcpuv_destruct(); }

static NAN_METHOD(Free) {
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());
  obj->Free();
}

static NAN_METHOD(Listen) {
  Isolate *isolate = info.GetIsolate();

  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());
  int port = info[1]->ToNumber(isolate)->Int32Value();

  if (!info[2]->IsFunction()) {
    isolate->ThrowException(Exception::Error(
        String::NewFromUtf8(isolate, "`Listen` expect a callback function")));
    return;
  }

  obj->bind_cb = Nan::Persistent<Function>(info[2].As<Function>());
  kcpuv_sess *sess = obj->GetSess();
  kcpuv_listen(sess, port, &binding_cb);
}

static NAN_METHOD(StopListen) {
  Isolate *isolate = info.GetIsolate();

  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  kcpuv_stop_listen(obj->GetSess());
}

static NAN_METHOD(BindClose) {
  Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  if (!info[1]->IsFunction()) {
    isolate->ThrowException(Exception::Error(String::NewFromUtf8(
        isolate, "`BindClose` expect a callback function")));
    return;
  }

  obj->close_cb = Nan::Persistent<Function>(info[1].As<Function>());

  kcpuv_sess *sess = obj->GetSess();
  kcpuv_bind_close(sess, &closing_cb);
}

static NAN_METHOD(Close) {
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  bool sendClose = Local<Boolean>::Cast(info[1])->BooleanValue();
  unsigned int closeVal = 0;

  if (sendClose) {
    closeVal = 1;
  }

  kcpuv_sess *sess = obj->GetSess();
  kcpuv_close(sess, closeVal);
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

static NAN_METHOD(Send) {
  Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  unsigned char *buf = (unsigned char *)node::Buffer::Data(info[1]->ToObject());
  unsigned int size = info[2]->Uint32Value();

  kcpuv_send(obj->GetSess(), reinterpret_cast<char *>(buf), size);
}

static NAN_METHOD(StartLoop) { kcpuv_start_loop(); }

static NAN_METHOD(DestroyLoop) { kcpuv_destroy_loop(); }

static NAN_MODULE_INIT(Init) {
  Isolate *isolate = target->GetIsolate();

  Local<FunctionTemplate> tpl =
      FunctionTemplate::New(isolate, KcpuvSessBinding::Create);
  tpl->SetClassName(String::NewFromUtf8(isolate, "KcpuvSessBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  KcpuvSessBinding::constructor.Reset(isolate, tpl->GetFunction());

  Nan::Set(target, "useDefaultLoop", UseDefaultLoop);
  Nan::Set(target, String::NewFromUtf8(isolate, "create"), tpl->GetFunction());
  Nan::SetMethod(target, "free", Free);
  Nan::SetMethod(target, "listen", Listen);
  Nan::SetMethod(target, "stopListen", StopListen);
  Nan::SetMethod(target, "initSend", InitSend);
  Nan::SetMethod(target, "send", Send);
  Nan::SetMethod(target, "bindClose", BindClose);
  Nan::SetMethod(target, "close", Close);
  Nan::SetMethod(target, "initialize", Initialize);
  Nan::SetMethod(target, "destruct", Destruct);
  Nan::SetMethod(target, "startLoop", StartLoop);
  Nan::SetMethod(target, "destroyLoop", DestroyLoop);
}

NODE_MODULE(binding, Init)
} // namespace kcpuv_addons
