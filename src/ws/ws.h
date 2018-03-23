#ifndef WS_WS_H
#define WS_WS_H

#include <stddef.h>

#define WS_MESSAGE_SIZE 2048
#define WS_CLIENT_QUEUE 8

#define WS_HTTP_REQUEST 1
#define WS_CONNECT 2
#define WS_MESSAGE 3
#define WS_DISCONNECT 4

#define WS_CLOSED 1
#define WS_OVERFLOW 2
#define WS_NOSEND 3

struct WsServer;
struct WsConnection;

struct WsMessageEvent
{
  char data[WS_MESSAGE_SIZE];
  size_t length;
};

struct WsDisconnectEvent
{
  int reason;
};

struct WsEvent
{
  int type;
  struct WsConnection *connection;
  struct WsMessageEvent *message;
  struct WsDisconnectEvent *disconnect;
};

struct WsServer *WsListen(int port);
int WsSend(struct WsConnection *connection, const char *message, size_t length);
int WsPoll(struct WsServer *server, struct WsEvent *event);
void WsClose(struct WsServer *server);

typedef struct WsServer WsServer;
typedef struct WsEvent WsEvent;

#endif
