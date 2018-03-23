#include <ws/ws.h>

#include <unistd.h>

#include <stdio.h>
#include <string.h>

const char *payload =
  "<html><head><title>WebSocket Test</title><script>"
  "window.onload = function()"
  "{"
  "  var socket = new WebSocket('ws://'+location.host);"

  "  socket.addEventListener('open', function(event)"
  "  {"
  "    console.log('Connection opened');"
  "    for(var i = 0; i < 5; i++)"
  "    {"
  "      socket.send('Hello Server!');"
  "    }"
  "  });"

  "  socket.addEventListener('message', function(event)"
  "  {"
  "    console.log('Message from server ', event.data);"
  "  });"
  "};"
  "</script></head><body></body></html>";

int main()
{
  WsServer *server = NULL;
  WsEvent event = {0};

  server = WsListen(2080);

  while(1)
  {
    while(WsPoll(server, &event))
    {
      if(event.type == WS_HTTP_REQUEST)
      {
        WsSend(event.connection, payload, strlen(payload));
      }
      else if(event.type == WS_CONNECT)
      {
        printf("Client connected\n");
        //WsSend(event.connection, "Welcome", 7);
      }
      else if(event.type == WS_MESSAGE)
      {
        printf("Message received [%i]: %s\n",
          (int)event.message->length,
          event.message->data);

        //WsSend(event.connection, event.message->data, event.message->length);
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
