#include <ws/ws.h>

#include <unistd.h>

#include <stdio.h>

int main()
{
  WsServer *server = NULL;
  WsEvent event = {0};

  server = WsListen(2080);

  while(1)
  {
    while(WsPoll(server, &event))
    {
      if(event.type == WS_CONNECT)
      {
        printf("Client connected\n");
        WsSend(event.connection, "Welcome", 7);
      }
      else if(event.type == WS_MESSAGE)
      {
        printf("Message received [%i]: %s\n",
          (int)event.message->length,
          event.message->data);

        WsSend(event.connection, event.message->data, event.message->length);
      }
      else if(event.type == WS_DISCONNECT)
      {
        printf("Client disconnected: %i\n", event.disconnect->reason);
      }
    }

    sleep(1);
  }

  //WsClose(server);

  return 0;
}
