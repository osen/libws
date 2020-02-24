#include <ws/ws.h>

#include <unistd.h>

#include <stdio.h>

ref(sstream) echoHtml;
int running;

void handle_http(ref(WsHttpEvent) http)
{
  ref(WsHttpRequest) request = NULL;
  ref(WsHttpResponse) response = NULL;

  request = WsHttpEventRequest(http);
  response = WsHttpEventResponse(http);
  printf("HTTP Request [%s]\n", sstream_cstr(WsHttpRequestPath(request)));

  if(strcmp(sstream_cstr(WsHttpRequestPath(request)), "/quit") == 0)
  {
    running = 0;
  }
  else if(strcmp(sstream_cstr(WsHttpRequestPath(request)), "/hello") == 0)
  {
    WsHttpResponseWrite(response, "Hello World");
  }
  else if(strcmp(sstream_cstr(WsHttpRequestPath(request)), "/echo") == 0)
  {
    WsHttpResponseWrite(response, sstream_cstr(echoHtml));
  }
  else
  {
    WsHttpResponseSetStatusCode(response, 404);
    WsHttpResponseWrite(response, "<h1>404 Not Found</h1><hr><i>raptor httpd</i>");
  }

  WsHttpResponseSend(response);
}

ref(sstream) load_file(char *path)
{
  ref(sstream) rtn = NULL;
  ref(ifstream) file = NULL;
  ref(sstream) line = NULL;

  file = ifstream_open_cstr(path);

  if(!file)
  {
    panic("Failed to open file");
  }

  rtn = sstream_new();
  line = sstream_new();

  while(!ifstream_eof(file))
  {
    ifstream_getline(file, line);
    sstream_append(rtn, line);
    sstream_append_char(rtn, '\n');
  }

  ifstream_close(file);
  sstream_delete(line);

  return rtn;
}

int main()
{
  ref(WsServer) server = NULL;
  struct WsEvent event = {0};

  echoHtml = load_file("www/echo.html");
  server = WsServerListen(1987);
  running = 1;

  while(running)
  {
    while(WsServerPoll(server, 1000, &event))
    {
      if(event.type == WS_HTTP_REQUEST)
      {
        handle_http(event.http);
      }
      else
      {
        printf("Unhandled Event: %i\n", event.type);
      }
    }

    printf("Tick\n");
  }

  WsServerClose(server);
  sstream_delete(echoHtml);

  return 0;
}

