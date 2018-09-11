#include "Mux.h"
#include "KcpuvSess.h"
#include <string.h>

namespace kcpuv {

static unsigned long MUX_CONN_DEFAULT_TIMEOUT = 30000;

static short enable_timeout = KCPUV_MUX_CONN_TIMEOUT;

void Mux::SetEnableTimeout(short value) { enable_timeout = value; }

static unsigned int bytes_to_int(const unsigned char *buffer) {
  return (unsigned int)(buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 |
                        buffer[3]);
}

// TODO: drop invalid msg
static void OnRecvMsg(KcpuvSess *sess, const char *data, unsigned int len) {
  int cmd;
  int content_length;
  unsigned int id;
  unsigned int offset = 0;

  while (offset < len) {
    id = Mux::Decode((const char *)(data + offset), &cmd, &content_length);

    Mux *mux = (Mux *)sess->mux;
    mux->Input((const char *)(data + offset + KCPUV_MUX_PROTOCOL_OVERHEAD),
               content_length, id, cmd);

    offset += content_length + KCPUV_MUX_PROTOCOL_OVERHEAD;
  }
}

// TODO: The sess to be freed.
static void SessCloseFirst(KcpuvSess *sess) {
  assert(sess->mux != NULL);
  fprintf(stderr, "sess is closed before closing mux\n");
  assert(0);

  // delete sess;
  // mux->Close();
}

static void int_to_bytes(unsigned char *buffer, unsigned int id) {
  buffer[0] = (id >> 24) & 0xFF;
  buffer[1] = (id >> 16) & 0xFF;
  buffer[2] = (id >> 8) & 0xFF;
  buffer[3] = id & 0xFF;
}

// Return sid, set cmd and length.
unsigned int Mux::Decode(const char *buffer, int *cmd, int *length) {
  const unsigned char *buf = (const unsigned char *)buffer;

  *cmd = (int)buf[0];
  *length = (unsigned int)(buf[5] << 8 | buf[6]);

  return bytes_to_int(buf + 1);
}

void Mux::Encode(char *buffer, unsigned int id, int cmd, int length) {
  unsigned char *buf = (unsigned char *)buffer;
  buf[0] = cmd;
  int_to_bytes(buf + 1, id);
  buf[5] = (length >> 8) & 0xFF;
  buf[6] = (length)&0xFF;
}

Mux::Mux(KcpuvSess *s) {
  if (s == NULL) {
    sess = new KcpuvSess;
  } else {
    sess = s;
  }

  conns = kcpuv_link_create(NULL);
  on_connection_cb = NULL;
  on_close_cb = NULL;
  sess->mux = this;

  SetZeroID();

  sess->BindListen(OnRecvMsg);
  sess->BindClose(SessCloseFirst);
}

Mux::~Mux() {
  delete this->conns;
  this->on_connection_cb = NULL;
  this->on_close_cb = NULL;
}

void Mux::SetZeroID() { this->count = sess->GetPassive() ? 2 : 1; }

unsigned int Mux::GetIncreaseID() {
  unsigned int count = this->count;
  this->count += 2;

  if (count <= 65535) {
    return count;
  }

  SetZeroID();
  return this->count;
}

bool Mux::HasConnWithId(unsigned int id) {
  bool has = 0;
  kcpuv_link *cur = this->conns;

  while (cur->next != NULL) {
    Conn *conn = static_cast<Conn *>(cur->next->node);

    if (conn->GetId() == id) {
      has = 1;
      break;
    }

    cur = cur->next;
  }

  return has;
}

int Mux::GetConnLength() {
  int length = 0;
  kcpuv_link *cur = this->conns;

  while (cur->next != NULL) {
    length += 1;
    cur = cur->next;
  }

  return length;
}

bool Mux::IsIdFromOtherSide(unsigned int id) {
  return id % 2 == (sess->GetPassive() ? 1 : 0);
}

void Mux::Input(const char *data, unsigned int len, unsigned int id, int cmd) {
  Mux *mux = this;
  KcpuvSess *sess = mux->sess;
  Conn *conn = NULL;

  // find conn
  kcpuv_link *link = mux->conns->next;

  while (link != NULL && ((Conn *)link->node)->GetId() != id) {
    link = link->next;
  }

  if (link != NULL) {
    conn = (Conn *)link->node;
  }

  if (cmd == KCPUV_MUX_CMD_CONNECT && IsIdFromOtherSide(id)) {
    // when the conn is connected
    if (conn != NULL) {
      // Possible the conn starts the conversation.
      if (conn->recv_state != KCPUV_CONN_RECV_NOT_CONNECTED) {
        // And it should not receive KCPUV_MUX_CMD_CONNECT before.
        fprintf(stderr, "%s: %d\n", "receive invalid CONNECT cmd, state",
                conn->recv_state);
        return;
      }
    } else if (!mux->HasConnWithId(id)) {
      // Create a new one passively.
      conn = mux->CreateConn(id);

      if (mux->on_connection_cb != NULL) {
        MuxOnConnectionCb cb = mux->on_connection_cb;
        cb(conn);
      } else {
        // TODO: Should not exit.
        assert(0);
      }
    } else {
      // Fin sended and ignore.
      // TODO: this may happen
      // fprintf(stderr, "receive invalid msg with id: %d, currenct: %d\n", id,
      //         mux->count);
    }
  }

  // NOTE: happend when the conn has been freed
  if (conn == NULL) {
    return;
  }

  // The conn have receive fin.
  if (conn->recv_state == KCPUV_CONN_RECV_STOP) {
    fprintf(stderr, "%s %d %d\n", "drop invalid msg", id, cmd);
    return;
  }

  // ready for data
  conn->recv_state = KCPUV_CONN_RECV_READY;
  // update ts
  conn->ts = sess->recvTs;

  // handle cmd
  if (cmd == KCPUV_MUX_CMD_PUSH || cmd == KCPUV_MUX_CMD_CONNECT) {
    if (conn->on_msg_cb != NULL) {
      ConnOnMsgCb cb = conn->on_msg_cb;
      cb(conn, data, len);
    } else {
      // Should never happen.
      assert(0);
    }
  } else if (cmd == KCPUV_MUX_CMD_FIN) {
    conn->recv_state = KCPUV_CONN_RECV_STOP;
    if (conn->on_otherside_end != NULL) {
      ConnOnOthersideEnd cb = conn->on_otherside_end;
      cb(conn);
    }
  } else if (cmd == KCPUV_MUX_CMD_CLS) {
    conn->send_state = KCPUV_CONN_SEND_STOPPED;
    conn->recv_state = KCPUV_CONN_RECV_STOP;
    conn->Close();
  } else {
    // drop invalid cmd
    fprintf(stderr, "%s\n", "receive invalid cmd");
  }
}

static void RemoveAndTriggerMuxClose(KcpuvCallbackInfo *info) {
  Mux *mux = reinterpret_cast<Mux *>(info->data);

  assert(mux->on_close_cb);
  MuxOnCloseCb cb = mux->on_close_cb;
  cb(mux, NULL);

  delete info;
}

static void DeleteSess(KcpuvSess *sess) {
  Mux *mux = reinterpret_cast<Mux *>(sess->mux);

  delete sess;

  KcpuvCallbackInfo *info = new KcpuvCallbackInfo;
  info->data = mux;
  info->cb = RemoveAndTriggerMuxClose;

  Loop::NextTick(info);
}

void Mux::Close() {
  KcpuvSess *sess = this->sess;
  kcpuv_link *link = conns;

  while (link->next != NULL) {
    // Force close all conns.
    Conn *conn = (Conn *)link->next->node;
    conn->Close();
    // INVALID: Expect js to call the free func and should close more than one
    // conn in the callback.
    link = link->next;
  }

  // Close sess.
  sess->BindClose(DeleteSess);
  sess->Close();
}

void Mux::BindConnection(MuxOnConnectionCb cb) { on_connection_cb = cb; }

void Mux::BindClose(MuxOnCloseCb cb) { on_close_cb = cb; }

void Mux::AddConnToList(Conn *conn) {
  kcpuv_link *link = kcpuv_link_create(conn);
  kcpuv_link_add(this->conns, link);
};

kcpuv_link *Mux::RemoveConnFromList(Conn *conn) {
  return kcpuv_link_get_pointer(this->conns, conn);
};

Conn *Mux::CreateConn(unsigned int id) {
  Conn *conn = new Conn(this, id);
  return conn;
}

Conn::Conn(Mux *mux, unsigned int i) {
  // init conn data
  // TODO: over two bytes
  // side when connecting

  // Use new id if not provided.
  id = i ? i : mux->GetIncreaseID();
  timeout = MUX_CONN_DEFAULT_TIMEOUT;
  ts = iclock();
  this->mux = mux;
  on_msg_cb = NULL;
  on_close_cb = NULL;
  on_otherside_end = NULL;
  recv_state = KCPUV_CONN_RECV_NOT_CONNECTED;
  send_state = KCPUV_CONN_SEND_NOT_CONNECTED;

  // add to mux conns
  mux->AddConnToList(this);
}

// Popout the link from the queue and close it.
// Users have to free Conn mannually.
Conn::~Conn() {}

static void RemoveAndTriggerConnClose(KcpuvCallbackInfo *info) {
  Conn *conn = reinterpret_cast<Conn *>(info->data);
  delete info;

  kcpuv_link *ptr = conn->mux->RemoveConnFromList(conn);
  assert(ptr != NULL);
  delete ptr;

  ConnOnCloseCb cb = conn->on_close_cb;
  // NOTE: Have to bind close before deleting.
  assert(conn->on_close_cb != NULL);
  // TODO: Do we need error msg.
  cb(conn, NULL);
}

// After closing, any io will stop immediatly.
void Conn::Close() {
  // TODO: may incorrectly
  if (recv_state == KCPUV_CONN_RECV_STOP &&
      send_state == KCPUV_CONN_SEND_STOPPED) {
    return;
  }

  // Tell the other side to close.
  bool allowSendClose = this->send_state == KCPUV_CONN_SEND_READY;

  if (allowSendClose) {
    SendClose();
  }

  this->recv_state = KCPUV_CONN_RECV_STOP;
  this->send_state = KCPUV_CONN_SEND_STOPPED;

  KcpuvCallbackInfo *info = new KcpuvCallbackInfo;
  info->cb = RemoveAndTriggerConnClose;
  info->data = this;

  Loop::NextTick(info);
}

void Conn::BindMsg(ConnOnMsgCb cb) { on_msg_cb = cb; }

void Conn::BindClose(ConnOnCloseCb cb) { on_close_cb = cb; }

void Conn::BindOthersideEnd(ConnOnOthersideEnd cb) { on_otherside_end = cb; }

int Conn::Send(const char *content, int len, int cmd) {
  Conn *conn = this;
  Mux *mux = conn->mux;
  KcpuvSess *sess = mux->sess;

  unsigned long s = 0;
  int send_cmd = cmd;

  // prevent sending when closed
  if (conn->send_state == KCPUV_CONN_SEND_STOPPED) {
    return -1;
  }

  // send cmd with empty content
  if (len == 0) {
    if (!cmd) {
      if (conn->send_state == KCPUV_CONN_SEND_NOT_CONNECTED) {
        conn->send_state = KCPUV_CONN_SEND_READY;
        send_cmd = KCPUV_MUX_CMD_CONNECT;
      } else {
        send_cmd = KCPUV_MUX_CMD_PUSH;
      }
    }

    unsigned int total_len = KCPUV_MUX_PROTOCOL_OVERHEAD;
    char *encoded_content = new char[total_len];
    Mux::Encode(encoded_content, conn->GetId(), send_cmd, len);
    sess->Send(encoded_content, total_len);

    delete[] encoded_content;
    return 0;
  }

  while (s < (unsigned int)len) {
    if (!cmd) {
      if (conn->send_state == KCPUV_CONN_SEND_NOT_CONNECTED) {
        conn->send_state = KCPUV_CONN_SEND_READY;
        send_cmd = KCPUV_MUX_CMD_CONNECT;
      } else {
        send_cmd = KCPUV_MUX_CMD_PUSH;
      }
    }

    unsigned long e = s + MAX_MUX_CONTENT_LEN;

    if (e > (unsigned int)len) {
      e = len;
    }

    unsigned int part_len = e - s;
    unsigned int total_len = part_len + KCPUV_MUX_PROTOCOL_OVERHEAD;

    // TODO: refactor: copy only once when sending
    char *encoded_content = new char[total_len];
    Mux::Encode(encoded_content, conn->GetId(), send_cmd, part_len);

    memcpy(encoded_content + KCPUV_MUX_PROTOCOL_OVERHEAD, content + s,
           part_len);

    sess->Send(encoded_content, total_len);

    delete[] encoded_content;
    s = e;
  }

  return 0;
}

void Conn::SendClose() { Send(NULL, 0, KCPUV_MUX_CMD_CLS); }

void Conn::SendStopSending() {
  Send(NULL, 0, KCPUV_MUX_CMD_FIN);
  send_state = KCPUV_CONN_SEND_STOPPED;
}

// Mainly to check timeout.
static void mux_check(Mux *mux) {
  IUINT32 current = iclock();
  kcpuv_link *link = mux->conns;

  while (link->next != NULL) {
    Conn *conn = (Conn *)link->next->node;

    // check conns timeout
    if (enable_timeout) {
      if (conn->GetTimeout()) {
        if (conn->ts + conn->GetTimeout() <= current) {
          conn->Close();
        }
      }
    }

    // TODO: ? check closing mark
    link = link->next;
  }
}

void Mux::UpdateMux(uv_timer_t *timer) {
  // update sessions
  KcpuvSess::KcpuvUpdateKcpSess_(timer);

  assert(KcpuvSess::KcpuvGetSessList() != NULL);

  kcpuv_link *link = KcpuvSess::KcpuvGetSessList()->list;

  while (link->next != NULL) {
    link = link->next;

    Mux *mux = (Mux *)(((KcpuvSess *)(link->node))->mux);

    if (mux != NULL) {
      mux_check(mux);
    }
  }
}

} // namespace kcpuv
