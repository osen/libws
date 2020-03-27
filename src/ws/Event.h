#include "stent.h"

struct WsMessageEvent
{
  vector(unsigned char) data;
  ref(sstream) message;
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

