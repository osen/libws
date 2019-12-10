#include <ws/ws.h>

int main()
{
  ref(WsServer) server = NULL;
  struct WsEvent event = {0};
  ref(WsHttpResponse) response = NULL;
  int count = 0;

  server = WsServerListen(8080);

  while(count < 10)
  {
    while(WsServerPoll(server, 1000, &event))
    {
      if(event.type == WS_HTTP_REQUEST)
      {
        response = WsHttpEventResponse(event.http);
        WsHttpResponseWrite(response, "<h1>It Works!</h1>");
        WsHttpResponseSend(response);
        count++;
      }
    }
  }

  WsServerClose(server);

  return 0;
}

