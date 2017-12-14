#define NDEBUG

#include "kcpuv_sess.h"
#include "mux.h"
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
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;

class KcpuvSessBinding : public Nan::ObjectWrap {
public:
  explicit KcpuvSessBinding() {
    sess = kcpuv_create();
    sess->data = this;
    sess->timeout = 0;
  }

  virtual ~KcpuvSessBinding() {
    if (sess) {
      Free();
    }
  }

  static Persistent<Function> constructor;
  static void Create(const FunctionCallbackInfo<Value> &args);

  void Free() {
    if (!is_freed) {
      is_freed = 1;
      kcpuv_stop_listen(sess);
      kcpuv_free(sess);
      sess = 0;
    }
  };

  kcpuv_sess *GetSess() { return sess; }

  Nan::CopyablePersistentTraits<Function>::CopyablePersistent listen_cb;
  Nan::CopyablePersistentTraits<Function>::CopyablePersistent close_cb;
  Nan::CopyablePersistentTraits<Function>::CopyablePersistent udp_send_cb;
  kcpuv_sess *sess;
  int is_freed = 0;
};

Persistent<Function> KcpuvSessBinding::constructor;

class KcpuvMuxConnBinding : public Nan::ObjectWrap {
public:
  explicit KcpuvMuxConnBinding() {}
  virtual ~KcpuvMuxConnBinding() { delete conn; }

  static Persistent<Function> constructor;
  static void Create(const FunctionCallbackInfo<Value> &args);
  // void WrapObject(Local<Object> object) { this->Wrap(object); }
  // TODO: confusing
  void WrapConn(kcpuv_mux_conn *default_conn) {
    conn = default_conn;
    conn->data = this;
  }

  kcpuv_mux_conn *conn;
  Nan::CopyablePersistentTraits<Function>::CopyablePersistent on_message;
  Nan::CopyablePersistentTraits<Function>::CopyablePersistent on_close;
};

Persistent<Function> KcpuvMuxConnBinding::constructor;

void KcpuvMuxConnBinding::Create(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();

  if (args.IsConstructCall()) {
    KcpuvMuxConnBinding *conn = new KcpuvMuxConnBinding();
    kcpuv_mux_conn *c = new kcpuv_mux_conn;
    conn->WrapConn(c);
    conn->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    Local<Function> cons =
        Local<Function>::New(isolate, KcpuvMuxConnBinding::constructor);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> instance = cons->NewInstance(context, 0, 0).ToLocalChecked();
    args.GetReturnValue().Set(instance);
  }
}

class KcpuvMuxBinding : public Nan::ObjectWrap {
public:
  explicit KcpuvMuxBinding() {
    //
    mux.data = this;
  }
  virtual ~KcpuvMuxBinding() {}

  static Persistent<Function> constructor;
  static void Create(const FunctionCallbackInfo<Value> &args);

  kcpuv_mux mux;
  Nan::CopyablePersistentTraits<Function>::CopyablePersistent on_close;
  Nan::CopyablePersistentTraits<Function>::CopyablePersistent on_connection;
};

Persistent<Function> KcpuvMuxBinding::constructor;

void KcpuvMuxBinding::Create(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();

  if (args.IsConstructCall()) {
    KcpuvMuxBinding *mux = new KcpuvMuxBinding();
    mux->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    Local<Function> cons =
        Local<Function>::New(isolate, KcpuvMuxBinding::constructor);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> instance = cons->NewInstance(context, 0, 0).ToLocalChecked();
    args.GetReturnValue().Set(instance);
  }
}

// void buffer_delete_callback(char *data, void *hint) { free(data); }

static void on_listen_cb(kcpuv_sess *sess, const char *data, int len) {
  KcpuvSessBinding *binding = static_cast<KcpuvSessBinding *>(sess->data);
  // NOTE: Create a scope for the allocation of v8 memories
  // as we are calling a js function outside the v8
  Nan::HandleScope scope;

  // NOTE: V8 will free these memories so that we have to
  // make a copy.
  char *buf_data = new char[len];
  memcpy(buf_data, data, len);

  const int argc = 1;
  Local<Value> args[argc] = {Nan::NewBuffer(buf_data, len).ToLocalChecked()};

  Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                    Nan::New(binding->listen_cb), argc, args);
}

static void closing_cb(kcpuv_sess *sess, void *data) {
  KcpuvSessBinding *binding = static_cast<KcpuvSessBinding *>(sess->data);

  Nan::HandleScope scope;
  Isolate *isolate = Isolate::GetCurrent();
  const char *error_msg = reinterpret_cast<const char *>(data);
  const int argc = 1;
  Local<Value> args[argc] = {Nan::Undefined()};
  // if (!isolate) {
  //   isolate = Isolate::New();
  //   isolate->Enter();
  // }

  if (error_msg) {
    // TODO: memcpy
    args[0] = String::NewFromUtf8(isolate, error_msg);
  }

  Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                    Nan::New(binding->close_cb), argc, args);
}

static void on_udp_send(kcpuv_sess *sess, uv_buf_t *buf, int buf_count,
                        const struct sockaddr *addr) {
  KcpuvSessBinding *binding = static_cast<KcpuvSessBinding *>(sess->data);
  Nan::HandleScope scope;
  Isolate *isolate = Isolate::GetCurrent();

  char *buf_data = new char[buf->len];
  memcpy(buf_data, buf->base, buf->len);

  char address[16];
  uv_ip4_name(reinterpret_cast<const struct sockaddr_in *>(addr), address, 16);
  int port =
      ntohs((reinterpret_cast<const struct sockaddr_in *>(addr))->sin_port);

  Local<Value> js_address = String::NewFromUtf8(isolate, address);
  Local<Value> js_port = Number::New(isolate, port);

  const int argc = 3;
  Local<Value> args[argc] = {
      Nan::NewBuffer(buf_data, buf->len).ToLocalChecked(), js_address, js_port};

  Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                    Nan::New(binding->udp_send_cb), argc, args);
}

void KcpuvSessBinding::Create(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();

  if (args.IsConstructCall()) {
    KcpuvSessBinding *binding = new KcpuvSessBinding();
    binding->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    Local<Function> cons =
        Local<Function>::New(isolate, KcpuvSessBinding::constructor);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> instance = cons->NewInstance(context, 0, 0).ToLocalChecked();
    args.GetReturnValue().Set(instance);
  }
}

static NAN_METHOD(UseDefaultLoop) {
  bool use = info[0]->BooleanValue();
  int val = 0;
  if (use) {
    val = 1;
  }

  kcpuv_use_default_loop(val);
}

static NAN_METHOD(Initialize) { kcpuv_initialize(); }

static NAN_METHOD(Destruct) { kcpuv_destruct(); }

static NAN_METHOD(InitCryptor) {
  Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  String::Utf8Value utf8Str(info[1]->ToString());
  const char *key = *utf8Str;
  int keylen = info[2]->ToNumber(isolate)->Int32Value();

  kcpuv_sess_init_cryptor(obj->GetSess(), key, keylen);
}

static NAN_METHOD(SetWndSize) {
  Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  int snd_wnd = info[1]->ToNumber(isolate)->Int32Value();
  int rcv_wnd = info[2]->ToNumber(isolate)->Int32Value();

  ikcp_wndsize(obj->GetSess()->kcp, snd_wnd, rcv_wnd);
}

static NAN_METHOD(SetNoDelay) {
  Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  int nodelay = info[1]->ToNumber(isolate)->Int32Value();
  int interval = info[2]->ToNumber(isolate)->Int32Value();
  int resend = info[3]->ToNumber(isolate)->Int32Value();
  int nc = info[4]->ToNumber(isolate)->Int32Value();

  ikcp_nodelay(obj->GetSess()->kcp, nodelay, interval, resend, nc);
}

static NAN_METHOD(Free) {
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  obj->Free();
}

static NAN_METHOD(MarkFree) {
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  kcpuv_set_state(obj->GetSess(), KCPUV_STATE_WAIT_FREE);
}

static NAN_METHOD(Input) {
  // Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  char *js_content = node::Buffer::Data(info[1]->ToObject());
  unsigned int content_size = info[2]->Uint32Value();
  char *content = new char[content_size];
  memcpy(content, js_content, content_size);
  const uv_buf_t content_buf = uv_buf_init(content, content_size);

  String::Utf8Value utf8Str(info[3]->ToString());
  char *ip = *utf8Str;
  unsigned int port = info[4]->Uint32Value();

  struct sockaddr addr;
  uv_ip4_addr(reinterpret_cast<const char *>(ip), port,
              reinterpret_cast<struct sockaddr_in *>(&addr));

  kcpuv_input(obj->GetSess(), content_size, &content_buf, &addr);
}

static NAN_METHOD(Listen) {
  Isolate *isolate = info.GetIsolate();

  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());
  int port = info[1]->ToNumber(isolate)->Int32Value();

  if (info[2]->IsFunction()) {
    obj->listen_cb = Nan::Persistent<Function>(info[2].As<Function>());
  }

  // else {
  // isolate->ThrowException(Exception::Error(
  //     String::NewFromUtf8(isolate, "`Listen` expect a callback
  //     function")));
  // return;
  // }

  kcpuv_sess *sess = obj->GetSess();
  int rval = kcpuv_listen(sess, port, &on_listen_cb);

  if (rval != 0) {
    return info.GetReturnValue().Set(
        String::NewFromUtf8(isolate, uv_strerror(rval)));
  }
}

static NAN_METHOD(BindListen) {
  Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  if (!info[1]->IsFunction()) {
    isolate->ThrowException(Exception::Error(String::NewFromUtf8(
        isolate, "`BindListen` expect a callback function")));
    return;
  }

  obj->listen_cb = Nan::Persistent<Function>(info[1].As<Function>());
}

static NAN_METHOD(GetPort) {
  Isolate *isolate = info.GetIsolate();
  char addr[17];
  int namelen = 0;
  int port = 0;

  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  kcpuv_get_address(obj->GetSess(), (char *)&addr, &namelen, &port);

  Local<Number> port_value = Number::New(isolate, port);
  info.GetReturnValue().Set(port_value);
}

static NAN_METHOD(StopListen) {
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

static NAN_METHOD(BindUdpSend) {
  Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  if (!info[1]->IsFunction()) {
    isolate->ThrowException(Exception::Error(String::NewFromUtf8(
        isolate, "`BindUdpSend` expect a callback function")));
    return;
  }

  obj->udp_send_cb = Nan::Persistent<Function>(info[1].As<Function>());
  kcpuv_bind_udp_send(obj->GetSess(), &on_udp_send);
}

static NAN_METHOD(Close) {
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  bool sendClose = Local<Boolean>::Cast(info[1])->BooleanValue();
  unsigned int closeVal = 0;

  if (sendClose) {
    closeVal = 1;
  }

  kcpuv_close(obj->GetSess(), closeVal, NULL);
}

static NAN_METHOD(InitSend) {
  Isolate *isolate = info.GetIsolate();
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());
  String::Utf8Value utf8Str(info[1]->ToString());
  // TODO: Do we need to free strings from js?
  char *addr = *utf8Str;
  int port = static_cast<double>(info[2]->ToNumber(isolate)->Value());

  kcpuv_init_send(obj->GetSess(), addr, port);
}

static NAN_METHOD(Send) {
  KcpuvSessBinding *obj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[0]->ToObject());

  unsigned char *buf = (unsigned char *)node::Buffer::Data(info[1]->ToObject());
  unsigned int size = info[2]->Uint32Value();

  kcpuv_send(obj->GetSess(), reinterpret_cast<char *>(buf), size);
}

static NAN_METHOD(StartLoop) { kcpuv_start_loop(kcpuv__mux_updater); }

static NAN_METHOD(StopLoop) { kcpuv_stop_loop(); }

static NAN_METHOD(MuxInit) {
  // Isolate *isolate = info.GetIsolate();
  KcpuvMuxBinding *mux_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxBinding>(info[0]->ToObject());
  KcpuvSessBinding *sessObj =
      Nan::ObjectWrap::Unwrap<KcpuvSessBinding>(info[1]->ToObject());

  kcpuv_mux_init(&mux_obj->mux, sessObj->sess);
}

// TODO:
static NAN_METHOD(MuxFree) {
  KcpuvMuxBinding *mux_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxBinding>(info[0]->ToObject());

  kcpuv_mux_free(&mux_obj->mux);
}

static void mux_binding_on_close_cb(kcpuv_mux *mux, const char *error_msg) {
  KcpuvMuxBinding *mux_obj = static_cast<KcpuvMuxBinding *>(mux->data);
  Nan::HandleScope scope;
  Isolate *isolate = Isolate::GetCurrent();

  const int argc = 1;
  Local<Value> argv[argc] = {};

  if (error_msg) {
    argv[0] = String::NewFromUtf8(isolate, error_msg);
  } else {
    argv[0] = Nan::Undefined();
  }

  Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                    Nan::New(mux_obj->on_close), argc, argv);
}

static NAN_METHOD(MuxBindClose) {
  Isolate *isolate = info.GetIsolate();
  KcpuvMuxBinding *mux_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxBinding>(info[0]->ToObject());

  if (!info[1]->IsFunction()) {
    isolate->ThrowException(Exception::Error(String::NewFromUtf8(
        isolate, "`MuxBindClose` expect a callback function")));
    return;
  }

  mux_obj->on_close = Nan::Persistent<Function>(info[1].As<Function>());
  kcpuv_mux_bind_close(&mux_obj->mux, mux_binding_on_close_cb);
}

static void mux_binding_on_connection_cb(kcpuv_mux_conn *conn) {
  kcpuv_mux *mux = static_cast<kcpuv_mux *>(conn->mux);
  KcpuvMuxBinding *mux_obj = static_cast<KcpuvMuxBinding *>(mux->data);

  Nan::HandleScope scope;
  Isolate *isolate = Isolate::GetCurrent();
  Local<Context> context = isolate->GetCurrentContext();

  Local<Function> cons =
      Local<Function>::New(isolate, KcpuvMuxConnBinding::constructor);
  Local<Object> conn_instance =
      cons->NewInstance(context, 0, 0).ToLocalChecked();
  KcpuvMuxConnBinding *conn_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxConnBinding>(conn_instance->ToObject());

  conn_obj->WrapConn(conn);

  const int argc = 1;
  Local<Value> argv[argc] = {conn_instance};

  Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                    Nan::New(mux_obj->on_connection), argc, argv);
}

static NAN_METHOD(MuxBindConnection) {
  Isolate *isolate = info.GetIsolate();
  KcpuvMuxBinding *mux_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxBinding>(info[0]->ToObject());

  if (!info[1]->IsFunction()) {
    isolate->ThrowException(Exception::Error(String::NewFromUtf8(
        isolate, "`MuxBindConnection` expect a callback function")));
    return;
  }

  mux_obj->on_connection = Nan::Persistent<Function>(info[1].As<Function>());
  kcpuv_mux_bind_connection(&mux_obj->mux, mux_binding_on_connection_cb);
}

static NAN_METHOD(ConnInit) {
  KcpuvMuxBinding *mux_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxBinding>(info[0]->ToObject());
  KcpuvMuxConnBinding *conn_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxConnBinding>(info[1]->ToObject());

  kcpuv_mux_conn_init(&mux_obj->mux, conn_obj->conn);
}

static NAN_METHOD(ConnFree) {
  KcpuvMuxConnBinding *conn_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxConnBinding>(info[0]->ToObject());

  kcpuv_mux_conn_free(conn_obj->conn, NULL);
}

static NAN_METHOD(ConnSend) {
  KcpuvMuxConnBinding *conn_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxConnBinding>(info[0]->ToObject());

  unsigned char *buf = (unsigned char *)node::Buffer::Data(info[1]->ToObject());
  unsigned int size = info[2]->Uint32Value();

  kcpuv_mux_conn_send(conn_obj->conn, reinterpret_cast<char *>(buf), size, 0);
}

static NAN_METHOD(ConnSendClose) {
  KcpuvMuxConnBinding *conn_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxConnBinding>(info[0]->ToObject());

  kcpuv_mux_conn_send_close(conn_obj->conn);
}

static void conn_binding_on_message_cb(kcpuv_mux_conn *conn, const char *data,
                                       int len) {
  KcpuvMuxConnBinding *conn_obj =
      static_cast<KcpuvMuxConnBinding *>(conn->data);
  Nan::HandleScope scope;

  // NOTE: V8 will free these memories so that we have to
  // make a copy.
  char *buf_data = new char[len];
  memcpy(buf_data, data, len);

  const int argc = 1;
  Local<Value> args[argc] = {Nan::NewBuffer(buf_data, len).ToLocalChecked()};

  Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                    Nan::New(conn_obj->on_message), argc, args);
}

static NAN_METHOD(ConnListen) {
  Isolate *isolate = info.GetIsolate();

  KcpuvMuxConnBinding *conn_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxConnBinding>(info[0]->ToObject());

  if (!info[1]->IsFunction()) {
    isolate->ThrowException(Exception::Error(String::NewFromUtf8(
        isolate, "`ConnListen` expect a callback function")));
    return;
  }

  conn_obj->on_message = Nan::Persistent<Function>(info[1].As<Function>());
  kcpuv_mux_conn_listen(conn_obj->conn, conn_binding_on_message_cb);
}

static void conn_binding_on_close_cb(kcpuv_mux_conn *conn,
                                     const char *error_msg) {
  KcpuvMuxConnBinding *conn_obj =
      static_cast<KcpuvMuxConnBinding *>(conn->data);
  Nan::HandleScope scope;
  Isolate *isolate = Isolate::GetCurrent();

  const int argc = 1;
  Local<Value> argv[argc] = {};

  if (error_msg) {
    argv[0] = String::NewFromUtf8(isolate, error_msg);
  } else {
    argv[0] = Nan::Undefined();
  }

  Nan::MakeCallback(Nan::GetCurrentContext()->Global(),
                    Nan::New(conn_obj->on_close), argc, argv);
}

static NAN_METHOD(ConnBindClose) {
  Isolate *isolate = info.GetIsolate();
  KcpuvMuxConnBinding *conn_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxConnBinding>(info[0]->ToObject());

  if (!info[1]->IsFunction()) {
    isolate->ThrowException(Exception::Error(String::NewFromUtf8(
        isolate, "`ConnBindClose` expect a callback function")));
    return;
  }

  conn_obj->on_close = Nan::Persistent<Function>(info[1].As<Function>());
  kcpuv_mux_conn_bind_close(conn_obj->conn, conn_binding_on_close_cb);
}

static NAN_METHOD(ConnSetTimeout) {
  KcpuvMuxConnBinding *conn_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxConnBinding>(info[0]->ToObject());

  unsigned int timeout = info[1]->Uint32Value();
  conn_obj->conn->timeout = timeout;
}

static NAN_METHOD(ConnEmitClose) {
  KcpuvMuxConnBinding *conn_obj =
      Nan::ObjectWrap::Unwrap<KcpuvMuxConnBinding>(info[0]->ToObject());

  kcpuv_mux_conn_emit_close(conn_obj->conn);
}

static NAN_MODULE_INIT(Init) {
  Isolate *isolate = target->GetIsolate();

  Local<FunctionTemplate> tpl =
      FunctionTemplate::New(isolate, KcpuvSessBinding::Create);
  tpl->SetClassName(String::NewFromUtf8(isolate, "KcpuvSessBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  KcpuvSessBinding::constructor.Reset(isolate, tpl->GetFunction());
  Nan::Set(target, String::NewFromUtf8(isolate, "create"), tpl->GetFunction());

  tpl = FunctionTemplate::New(isolate, KcpuvMuxConnBinding::Create);
  tpl->SetClassName(String::NewFromUtf8(isolate, "KcpuvMuxConnBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  KcpuvMuxConnBinding::constructor.Reset(isolate, tpl->GetFunction());
  Nan::Set(target, String::NewFromUtf8(isolate, "createMuxConn"),
           tpl->GetFunction());

  tpl = FunctionTemplate::New(isolate, KcpuvMuxBinding::Create);
  tpl->SetClassName(String::NewFromUtf8(isolate, "KcpuvMuxBinding"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  KcpuvMuxBinding::constructor.Reset(isolate, tpl->GetFunction());
  Nan::Set(target, String::NewFromUtf8(isolate, "createMux"),
           tpl->GetFunction());

  // sess method
  Nan::SetMethod(target, "initCryptor", InitCryptor);
  Nan::SetMethod(target, "setWndSize", SetWndSize);
  Nan::SetMethod(target, "setNoDelay", SetNoDelay);
  Nan::SetMethod(target, "useDefaultLoop", UseDefaultLoop);
  Nan::SetMethod(target, "free", Free);
  Nan::SetMethod(target, "markFree", MarkFree);
  Nan::SetMethod(target, "input", Input);
  Nan::SetMethod(target, "listen", Listen);
  Nan::SetMethod(target, "getPort", GetPort);
  Nan::SetMethod(target, "stopListen", StopListen);
  Nan::SetMethod(target, "initSend", InitSend);
  Nan::SetMethod(target, "send", Send);
  Nan::SetMethod(target, "bindClose", BindClose);
  Nan::SetMethod(target, "bindListen", BindListen);
  Nan::SetMethod(target, "bindUdpSend", BindUdpSend);
  Nan::SetMethod(target, "close", Close);
  Nan::SetMethod(target, "initialize", Initialize);
  Nan::SetMethod(target, "destruct", Destruct);
  Nan::SetMethod(target, "startLoop", StartLoop);
  Nan::SetMethod(target, "stopLoop", StopLoop);

  // mux method
  Nan::SetMethod(target, "muxInit", MuxInit);
  Nan::SetMethod(target, "muxFree", MuxFree);
  Nan::SetMethod(target, "muxBindClose", MuxBindClose);
  Nan::SetMethod(target, "muxBindConnection", MuxBindConnection);

  // conn method
  Nan::SetMethod(target, "connInit", ConnInit);
  Nan::SetMethod(target, "connFree", ConnFree);
  Nan::SetMethod(target, "connSend", ConnSend);
  Nan::SetMethod(target, "connSendClose", ConnSendClose);
  Nan::SetMethod(target, "connListen", ConnListen);
  Nan::SetMethod(target, "connBindClose", ConnBindClose);
  Nan::SetMethod(target, "connSetTimeout", ConnSetTimeout);
  Nan::SetMethod(target, "connEmitClose", ConnEmitClose);
}

NODE_MODULE(binding, Init)
} // namespace kcpuv_addons
