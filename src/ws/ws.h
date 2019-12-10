#ifndef WS_WS_H
#define WS_WS_H

#include <stent/stent.h>

#include <stddef.h>

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
struct WsHttpResponse;
struct WsHttpRequest;

struct WsMessageEvent
{
  char data[1024];
  size_t length;
};

struct WsDisconnectEvent
{
  int reason;
};

struct WsHttpEvent
{
  ref(WsHttpRequest) request;
  ref(WsHttpResponse) response;
};

struct WsEvent
{
  int type;
  ref(WsConnection) connection;
  ref(WsHttpEvent) http;
  ref(WsMessageEvent) message;
  ref(WsDisconnectEvent) disconnect;
};

ref(WsServer) WsServerListen(int port);
int WsSend(ref(WsConnection) connection, const char *message, size_t length);
int WsServerPoll(ref(WsServer) server, int timeout, struct WsEvent *event);
void WsServerClose(ref(WsServer) server);

ref(WsHttpRequest) WsHttpEventRequest(ref(WsHttpEvent) ctx);
ref(WsHttpResponse) WsHttpEventResponse(ref(WsHttpEvent) ctx);

void WsHttpResponseWrite(ref(WsHttpResponse) response, char *data);
ref(sstream) WsHttpRequestPath(ref(WsHttpRequest) request);

#endif
