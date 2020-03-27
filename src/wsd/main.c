#include <ws/ws.h>

#include <stdio.h>

ref(sstream) load_file(ref(sstream) path);

int running;

void handle_message(ref(WsMessageEvent) message)
{
  printf("Websocket message [%s]\n", sstream_cstr(WsMessageEventMessage(message)));
}

void handle_http(ref(WsHttpEvent) http)
{
  ref(WsHttpRequest) request = NULL;
  ref(WsHttpResponse) response = NULL;
  ref(sstream) content = NULL;

  request = WsHttpEventRequest(http);
  response = WsHttpEventResponse(http);
  printf("HTTP Request [%s]\n", sstream_cstr(WsHttpRequestPath(request)));

  if(strcmp(sstream_cstr(WsHttpRequestPath(request)), "/quit") == 0)
  {
    running = 0;
  }
  else if(strcmp(sstream_cstr(WsHttpRequestPath(request)), "/hello") == 0)
  {
    WsHttpResponseWriteCStr(response, "<h1>Hello World</h1><hr><i>raptor httpd</i>");
  }
  else
  {
    content = load_file(WsHttpRequestPath(request));

    if(content)
    {
      WsHttpResponseWriteCStr(response, sstream_cstr(content));
      sstream_delete(content);
    }
    else
    {
      WsHttpResponseSetStatusCode(response, 404);
      WsHttpResponseWriteCStr(response, "<h1>404 Not Found</h1><hr><i>raptor httpd</i>");
    }
  }

  WsHttpResponseSend(response);
}

ref(sstream) load_file(ref(sstream) path)
{
  ref(sstream) lp = NULL;
  ref(ifstream) file = NULL;
  ref(sstream) rtn = NULL;
  ref(sstream) line = NULL;

  lp = sstream_new();
  sstream_str_cstr(lp, "www");
  sstream_append(lp, path);

  file = ifstream_open(lp);
  sstream_delete(lp);

  if(!file)
  {
    return NULL;
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
      else if(event.type == WS_MESSAGE)
      {
        handle_message(event.message);
      }
      else
      {
        printf("Unhandled Event: %i\n", event.type);
      }
    }

/*
    printf("Tick\n");
*/
  }

  WsServerClose(server);

  return 0;
}

