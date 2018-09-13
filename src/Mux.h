#ifndef KCPUV_MUX_H
#define KCPUV_MUX_H

#include "KcpuvSess.h"
#include "kcpuv.h"
#include "utils.h"

namespace kcpuv {

class Mux;
class Conn;

typedef void (*MuxOnConnectionCb)(Conn *);
typedef void (*MuxOnCloseCb)(Mux *mux, const char *error_msg);
typedef void (*ConnOnMsgCb)(Conn *conn, const char *buffer, int length);
typedef void (*ConnOnCloseCb)(Conn *conn, unsigned int errorCode);
typedef void (*ConnOnOthersideEnd)(Conn *conn);

class Mux {
public:
  Mux(KcpuvSess *s = NULL);
  ~Mux();

  // Get data from raw data.
  static unsigned int Decode(const char *buffer, int *cmd, int *length);

  // Add data with protocol info.
  static void Encode(char *buffer, unsigned int id, int cmd, int length);

  // Set mux timeout.
  static void SetEnableTimeout(short);

  // Func to digest input data.
  static void UpdateMux(uv_timer_t *timer);

  Conn *CreateConn(unsigned int id = 0);

  unsigned int GetIncreaseID();

  void SetZeroID();

  void AddConnToList(Conn *);

  kcpuv_link *RemoveConnFromList(Conn *);

  kcpuv_link *GetConns_() { return conns; };

  void Input(const char *data, unsigned int len, unsigned int id, int cmd);

  // Release all relative resources and trigger on close.
  void Close();

  // Bind close event.
  void BindClose(MuxOnCloseCb);

  // Bind new connections event.
  void BindConnection(MuxOnConnectionCb cb);

  bool HasConnWithId(unsigned int id);

  int GetConnLength();

  bool IsIdFromOtherSide(unsigned int);

  KcpuvSess *sess;
  kcpuv_link *conns;
  unsigned int count;
  MuxOnCloseCb on_close_cb;
  void *data;

private:
  void InitMux(KcpuvSess *);
  MuxOnConnectionCb on_connection_cb;
};

class Conn {
public:
  Conn(Mux *, unsigned int id = 0);
  ~Conn();

  // Tell conn to send data.
  int Send(const char *content, int length, int cmd);

  // Bind on message callback.
  void BindMsg(ConnOnMsgCb);

  // The conn won't send any more data.
  void SendStopSending();

  // Event when the other side won't send any more data.
  void BindOthersideEnd(ConnOnOthersideEnd cb);

  // Tell conn to send closing message.
  void SendClose(unsigned int errorCode = 0);

  // Bind on close callback.
  void BindClose(ConnOnCloseCb);

  // Trigger close event and prevent conn from sending data.
  // Users are expected to free conn after callback.
  void Close(unsigned int errorCode = 0);

  unsigned int GetId() { return id; }

  void SetTimeout(unsigned long t) { timeout = t; }

  unsigned long GetTimeout() { return timeout; }

  void SetErrorCode(unsigned int code) { this->receiveErrorCode = code; }
  unsigned int GetErrorCode() { return receiveErrorCode; }

  void *data;
  short send_state;
  short recv_state;
  ConnOnMsgCb on_msg_cb;
  ConnOnCloseCb on_close_cb;
  ConnOnOthersideEnd on_otherside_end;
  IUINT32 ts;
  Mux *mux;

private:
  unsigned int id;
  unsigned long timeout;
  unsigned int receiveErrorCode;
};
} // namespace kcpuv

#endif
