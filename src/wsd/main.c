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
  ref(dir) dp = NULL;

  lp = sstream_new();
  sstream_str_cstr(lp, "www");
  sstream_append(lp, path);

  if(sstream_length(lp) < 1)
  {
    sstream_append_char(lp, '/');
  }

  dp = dir_open(lp);

  if(dp)
  {
    dir_close(dp);

    if(sstream_at(lp, sstream_length(lp) - 1) != '/')
    {
      sstream_append_char(lp, '/');
    }

    sstream_append_cstr(lp, "index.html");
  }

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
      else if(event.type == WS_CONNECT)
      {
        printf("Client Connected\n");
        vector(unsigned char) msg = vector_new(unsigned char);
        vector_push_back(msg, '1');
        vector_push_back(msg, '\t');
        vector_push_back(msg, '1');
        vector_push_back(msg, '7');
        vector_push_back(msg, '\t');
        vector_push_back(msg, 'T');
        vector_push_back(msg, 'r');
        vector_push_back(msg, 'i');
        vector_push_back(msg, 'a');
        vector_push_back(msg, 'n');
        vector_push_back(msg, 'g');
        vector_push_back(msg, 'l');
        vector_push_back(msg, 'e');
        vector_push_back(msg, 'R');
        vector_push_back(msg, 'e');
        vector_push_back(msg, 'n');
        vector_push_back(msg, 'd');
        vector_push_back(msg, 'e');
        vector_push_back(msg, 'r');
        vector_push_back(msg, 'e');
        vector_push_back(msg, 'r');
        vector_push_back(msg, '\t');

        vector_push_back(msg, '3');
        vector_push_back(msg, '\t');
        vector_push_back(msg, '1');
        vector_push_back(msg, '7');
        vector_push_back(msg, '\t');
        vector_push_back(msg, 'p');
        vector_push_back(msg, 'o');
        vector_push_back(msg, 's');
        vector_push_back(msg, 'i');
        vector_push_back(msg, 't');
        vector_push_back(msg, 'i');
        vector_push_back(msg, 'o');
        vector_push_back(msg, 'n');
        vector_push_back(msg, '\t');
        vector_push_back(msg, '0');
        vector_push_back(msg, ',');
        vector_push_back(msg, '0');
        vector_push_back(msg, ',');
        vector_push_back(msg, '-');
        vector_push_back(msg, '5');
        WsSend(event.connection, msg);

/*
        vector_clear(msg);
        vector_push_back(msg, '2');
        vector_push_back(msg, '\t');
        vector_push_back(msg, '1');
        vector_push_back(msg, '7');
        WsSend(event.connection, msg);
*/

        vector_delete(msg);
      }
      else if(event.type == WS_DISCONNECT)
      {
        printf("Client Disconnected\n");
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

