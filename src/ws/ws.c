#include "ws.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct WsServer
{
  int fd;
  struct WsMessageEvent message;
  struct WsDisconnectEvent disconnect;
  struct WsConnection *connections;
};

struct WsConnection
{
  int fd;
  int established;
  int disconnected;
  char incoming[WS_MESSAGE_SIZE];
  size_t incomingLength;

  char outgoing[WS_MESSAGE_SIZE];
  size_t outgoingLength;
  struct WsConnection *next;
  struct WsConnection *prev;
};

struct WsServer *WsListen(int port)
{
  struct WsServer *rtn = NULL;
  struct sockaddr_in server = {0};

  rtn = calloc(1, sizeof(*rtn));

  if(!rtn)
  {
    return NULL;
  }

  rtn->fd = socket(AF_INET, SOCK_STREAM, 0);

  if(rtn->fd == -1)
  {
    free(rtn);
    return NULL;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  if(setsockopt(rtn->fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) == -1 ||
    fcntl(rtn->fd, F_SETFL, O_NONBLOCK) == -1 ||
    bind(rtn->fd, (struct sockaddr *)&server, sizeof(server)) == -1 ||
    listen(rtn->fd, WS_CLIENT_QUEUE) == -1)
  {
    close(rtn->fd);
    free(rtn);
    return NULL;
  }

  return rtn;
}

int _WsPollConnectionRequests(struct WsServer *server, struct WsEvent *event)
{
  fd_set readfds = {0};
  struct timeval tv = {0};
  struct sockaddr_in client = {0};
  int len = 0;
  struct WsConnection *conn = NULL;

  FD_ZERO(&readfds);
  FD_SET(server->fd, &readfds);

  if(select(server->fd + 1, &readfds, NULL, NULL, &tv) == -1)
  {
    return 0;
  }

  if(!FD_ISSET(server->fd, &readfds))
  {
    return 0;
  }

  conn = calloc(1, sizeof(*event->connection));

  if(!conn)
  {
    return 0;
  }

  conn->fd = accept(server->fd, (struct sockaddr *)&client, (socklen_t*)&len);

  if(conn->fd == -1)
  {
    free(conn);
    return 0;
  }

  if(fcntl(conn->fd, F_SETFL, O_NONBLOCK) == -1)
  {
    close(conn->fd);
    free(conn);
    return 0;
  }

  if(server->connections) server->connections->prev = conn;
  conn->next = server->connections;
  server->connections = conn;

  return 0;
}

int _WsPollSend(struct WsConnection *connection)
{
  int bw = 0;

  if(connection->outgoingLength == 0)
  {
    return 0;
  }

  bw = write(connection->fd, connection->outgoing, connection->outgoingLength);

  if(bw == -1)
  {
    return 1;
  }
  else if(bw == 0)
  {
    return 0;
  }

  connection->outgoingLength -= bw;
  memmove(connection->outgoing, connection->outgoing + bw, connection->outgoingLength);
  memset(connection->outgoing + connection->outgoingLength, 0, WS_MESSAGE_SIZE - connection->outgoingLength);

  return 0;
}

int WsSend(struct WsConnection *connection, char *message, size_t length)
{
  if(_WsPollSend(connection) != 0)
  {
    return 1;
  }

  if(connection->outgoingLength + length > WS_MESSAGE_SIZE - 1)
  {
    return 1;
  }

  memcpy(connection->outgoing + connection->outgoingLength, message, length);
  connection->outgoingLength += length;

  if(_WsPollSend(connection) != 0)
  {
    return 1;
  }

  return 0;
}

int _WsPollHandshake(struct WsServer *server, struct WsConnection *connection, struct WsEvent *event)
{
  size_t i = 0;
  int headerFound = 0;
  int nextStart = 0;

  if(connection->incomingLength < 4)
  {
    return 0;
  }

  for(i = 0; i < connection->incomingLength - 3; i++)
  {
    if(connection->incoming[i] == '\r' &&
      connection->incoming[i + 1] == '\n' &&
      connection->incoming[i + 2] == '\r' &&
      connection->incoming[i + 3] == '\n')
    {
      headerFound = 1;
      nextStart = i + 4;
      break;
    }

    if(connection->incoming[i] == 'h' &&
      connection->incoming[i + 1] == 'i' &&
      connection->incoming[i + 2] == 'h' &&
      connection->incoming[i + 3] == '\n')
    {
      headerFound = 1;
      nextStart = i + 4;
      break;
    }
  }

  if(!headerFound)
  {
    return 0;
  }

  printf("Header: %s\n", connection->incoming);

  connection->incomingLength -= nextStart;
  memmove(connection->incoming, connection->incoming + nextStart, connection->incomingLength);
  memset(connection->incoming + connection->incomingLength, 0, WS_MESSAGE_SIZE - connection->incomingLength);

  printf("[%i] [%s]\n", (int)connection->incomingLength, connection->incoming);
  connection->established = 1;

  event->type = WS_CONNECT;
  event->connection = connection;

  return 1;
}

int _WsPollReceive(struct WsServer *server, struct WsConnection *connection, struct WsEvent *event)
{
  if(connection->incomingLength < 1)
  {
    return 0;
  }

  event->type = WS_MESSAGE;
  event->connection = connection;
  event->message = &server->message;
  memcpy(event->message->data, connection->incoming, connection->incomingLength);
  event->message->length = connection->incomingLength;

  memset(connection->incoming, 0, sizeof(*connection->incoming));
  connection->incomingLength = 0;

  return 1;
}

int _WsPollConnection(struct WsServer *server, struct WsConnection *connection, struct WsEvent *event)
{
  fd_set readfds = {0};
  struct timeval tv = {0};
  int bytes = 0;

  if(_WsPollSend(connection) != 0)
  {
    connection->disconnected = 1;
    if(!connection->established) return 0;
    event->disconnect = &server->disconnect;
    event->disconnect->reason = WS_CLOSED;
    event->type = WS_DISCONNECT;
    event->connection = connection;
    return 1;
  }

  FD_ZERO(&readfds);
  FD_SET(connection->fd, &readfds);

  if(select(connection->fd + 1, &readfds, NULL, NULL, &tv) == -1)
  {
    return 0;
  }

  if(FD_ISSET(connection->fd, &readfds))
  {
    bytes = WS_MESSAGE_SIZE - connection->incomingLength - 1;

    if(bytes == 0)
    {
      connection->disconnected = 1;
      if(!connection->established) return 0;
      event->disconnect = &server->disconnect;
      event->disconnect->reason = WS_OVERFLOW;
      event->type = WS_DISCONNECT;
      event->connection = connection;
      return 1;
    }

    bytes = read(connection->fd,
      connection->incoming + connection->incomingLength, bytes);

    if(bytes == 0)
    {
      connection->disconnected = 1;
      if(!connection->established) return 0;
      event->disconnect = &server->disconnect;
      event->disconnect->reason = WS_CLOSED;
      event->type = WS_DISCONNECT;
      event->connection = connection;
      return 1;
    }
    else if(bytes == -1)
    {
      return 0;
    }

    connection->incomingLength += bytes;
  }

  if(!connection->established)
  {
    return _WsPollHandshake(server, connection, event);
  }

  return _WsPollReceive(server, connection, event);
}

#define LL_REMOVE(ROOT, CURR) \
{ \
  void *toFree = NULL; \
  if(!CURR->prev) \
  { \
    ROOT = CURR->next; \
  } \
  else \
  { \
    CURR->prev->next = CURR->next; \
  } \
 \
  if(CURR->next) \
  { \
    CURR->next->prev = CURR->prev; \
  } \
 \
  toFree = CURR; \
  CURR = CURR->next; \
  free(toFree); \
}

int _WsPollConnections(struct WsServer *server, struct WsEvent *event)
{
  struct WsConnection *conn = NULL;

  conn = server->connections;

  while(conn)
  {
    if(conn->disconnected)
    {
      close(conn->fd);
      LL_REMOVE(server->connections, conn)
      continue;
    }

    if(_WsPollConnection(server, conn, event))
    {
      return 1;
    }

    conn = conn->next;
  }

  return 0;
}

int WsPoll(struct WsServer *server, struct WsEvent *event)
{
  memset(event, 0, sizeof(*event));
  memset(&server->disconnect, 0, sizeof(server->disconnect));
  memset(&server->message, 0, sizeof(server->message));

  if(_WsPollConnections(server, event))
  {
    return 1;
  }

  if(_WsPollConnectionRequests(server, event))
  {
    return 1;
  }

  return 0;
}

