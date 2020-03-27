#ifndef WS_WS_H
#define WS_WS_H

#include "stent.h"

#include <stddef.h>

#define WS_CLIENT_QUEUE 8

#define WS_HTTP_REQUEST 1
#define WS_CONNECT 2
#define WS_MESSAGE 3
#define WS_DISCONNECT 4

#define WS_CLOSED 1
#define WS_OVERFLOW 2

struct WsServer;
struct WsConnection;
struct WsHttpResponse;
struct WsHttpRequest;

struct WsMessageEvent;
struct WsDisconnectEvent;
struct WsHttpEvent;

struct WsEvent
{
  int type;
  ref(WsConnection) connection;
  ref(WsHttpEvent) http;
  ref(WsMessageEvent) message;
  ref(WsDisconnectEvent) disconnect;
};

ref(WsServer) WsServerListen(int port);
void WsSend(ref(WsConnection) connection, vector(unsigned char) data);
int WsServerPoll(ref(WsServer) server, int timeout, struct WsEvent *event);
void WsServerClose(ref(WsServer) server);

ref(WsHttpRequest) WsHttpEventRequest(ref(WsHttpEvent) ctx);
ref(WsHttpResponse) WsHttpEventResponse(ref(WsHttpEvent) ctx);

void WsHttpResponseSetStatusCode(ref(WsHttpResponse) ctx, int status);
void WsHttpResponseWriteCStr(ref(WsHttpResponse) ctx, char *data);
void WsHttpResponseSend(ref(WsHttpResponse) ctx);
ref(sstream) WsHttpRequestPath(ref(WsHttpRequest) ctx);

ref(sstream) WsMessageEventMessage(ref(WsMessageEvent) ctx);
vector(unsigned char) WsMessageEventData(ref(WsMessageEvent) ctx);

#endif
