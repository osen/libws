#include "ws.h"

#ifdef _WIN32
  #define USE_WINSOCK
#else
  #define USE_POSIX
#endif

#ifdef USE_POSIX
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/select.h>
#endif

#ifdef USE_WINSOCK
  //#define WIN32_LEAN_AND_MEAN
  //#include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define WS_FIN 128
#define WS_FR_OP_TXT  1

//#define MIN(a, b) (((a)<(b))?(a):(b))

struct WsServer
{
  int fd;
  struct WsMessageEvent message;
  struct WsDisconnectEvent disconnect;
  struct WsConnection *connections;
};

#ifdef USE_WINSOCK
int _wsaInitialized;
void _WsCleanupWinsock(void)
{
  WSACleanup();
}
#endif

struct WsConnection
{
#ifdef USE_WINSOCK
  SOCKET fd;
#endif
#ifdef USE_POSIX
  int fd;
#endif
  int established;
  int www;
  int disconnected;
  char incoming[WS_MESSAGE_SIZE];
  size_t incomingLength;

  char outgoing[WS_MESSAGE_SIZE];
  size_t outgoingLength;
  struct WsConnection *next;
  struct WsConnection *prev;
};

char *_WsHandshakeResponse(char *request);
unsigned char *_WsHandshakeAccept(char *wsKey);

char *_WsDecodeFrame(char *frame, size_t length,
  int *type, size_t *decodeLen, size_t *end);

struct WsServer *WsListen(int port)
{
  struct WsServer *rtn = NULL;
#ifdef USE_POSIX
  struct sockaddr_in server = {0};
#endif
#ifdef USE_WINSOCK
  struct addrinfo hints = {0};
  struct addrinfo *result;
  char portString[128] = {0};
  WSADATA wsaData;

  if(!_wsaInitialized)
  {
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
      return NULL;
    }

    atexit(_WsCleanupWinsock);
    _wsaInitialized = 1;
  }

  if(!itoa(port, portString, 10))
  {
    return NULL;
  }

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  if(getaddrinfo(NULL, portString, &hints, &result) != 0)
  {
    return NULL;
  }
#endif

  rtn = calloc(1, sizeof(*rtn));

  if(!rtn)
  {
    return NULL;
  }

#ifdef USE_POSIX
  rtn->fd = socket(AF_INET, SOCK_STREAM, 0);

  if(rtn->fd == -1)
#endif
#ifdef USE_WINSOCK
  rtn->fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

  if(rtn->fd == INVALID_SOCKET)
#endif
  {
#ifdef USE_WINSOCK
    freeaddrinfo(result);
#endif
    free(rtn);
    return NULL;
  }

#ifdef USE_POSIX
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
#endif

  if(
#ifdef USE_POSIX
    setsockopt(rtn->fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) == -1 ||
    fcntl(rtn->fd, F_SETFL, O_NONBLOCK) == -1 ||
    bind(rtn->fd, (struct sockaddr *)&server, sizeof(server)) == -1 ||
    listen(rtn->fd, WS_CLIENT_QUEUE) == -1
#endif
#ifdef USE_WINSOCK
    ioctlsocket(rtn->fd, FIONBIO, &(int){ 1 }) != NO_ERROR ||
    bind(rtn->fd, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR ||
    listen(rtn->fd, SOMAXCONN) == SOCKET_ERROR
#endif
)
  {
#ifdef USE_POSIX
    close(rtn->fd);
#endif
#ifdef USE_WINSOCK
    freeaddrinfo(result);
    closesocket(rtn->fd);
#endif
    free(rtn);
    return NULL;
  }

#ifdef USE_WINSOCK
  freeaddrinfo(result);
#endif

  return rtn;
}

int _WsPollConnectionRequests(struct WsServer *server, struct WsEvent *event)
{
#ifdef USE_POSIX
  struct sockaddr_in client = {0};
#endif
  fd_set readfds = {0};
  struct timeval tv = {0};
  int len = 0;
  struct WsConnection *conn = NULL;

  FD_ZERO(&readfds);
  FD_SET(server->fd, &readfds);

  if(
#ifdef USE_POSIX
    select(server->fd + 1, &readfds, NULL, NULL, &tv) == -1
#endif
#ifdef USE_WINSOCK
    select(server->fd + 1, &readfds, NULL, NULL, &tv) == SOCKET_ERROR
#endif
  )
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

  conn->fd =
#ifdef USE_POSIX
    accept(server->fd, (struct sockaddr *)&client, (socklen_t*)&len);
#endif
#ifdef USE_WINSOCK
    accept(server->fd, NULL, NULL);
#endif

  if(
#ifdef USE_POSIX
    conn->fd == -1
#endif
#ifdef USE_WINSOCK
    conn->fd == INVALID_SOCKET
#endif
  )
  {
    free(conn);
    return 0;
  }

  if(
#ifdef USE_POSIX
    fcntl(conn->fd, F_SETFL, O_NONBLOCK) == -1
#endif
#ifdef USE_WINSOCK
    ioctlsocket(conn->fd, FIONBIO, &(int){ 1 }) != NO_ERROR
#endif
  )
  {
#ifdef USE_POSIX
    close(conn->fd);
#endif
#ifdef USE_WINSOCK
    closesocket(conn->fd);
#endif
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
    if(connection->www)
    {
      connection->disconnected = 1;
    }

    return 0;
  }

#ifdef USE_POSIX
  //bw = write(connection->fd, connection->outgoing, connection->outgoingLength);
  //bw = send(connection->fd, connection->outgoing, MIN(connection->outgoingLength, 2048), MSG_NOSIGNAL);
  bw = send(connection->fd, connection->outgoing, connection->outgoingLength, MSG_NOSIGNAL);

  if(bw == -1)
#endif
#ifdef USE_WINSOCK
  bw = send(connection->fd, connection->outgoing, connection->outgoingLength, 0);

  if(bw == SOCKET_ERROR)
#endif
  {
    return 0;
  }
  else if(bw == 0)
  {
    return 0;
  }

  connection->outgoingLength -= bw;
  memmove(connection->outgoing, connection->outgoing + bw, connection->outgoingLength);

  // Clear the rest of the buffer
  //memset(connection->outgoing + connection->outgoingLength, 0, WS_MESSAGE_SIZE - connection->outgoingLength);

  connection->outgoing[connection->outgoingLength] = '\0';

  return 0;
}

/****************************************************************************
 * WsSend
 *
 * Attempt to send what is already in the outgoing buffer. If the new data
 * to send is not too large for the buffer then add it and re-attempt to send
 * the outgoing buffer again.
 *
 * connection - The specific connection to send the data.
 * message    - The data in which to send.
 * length     - The lengh of data (to avoid strlen).
 *
 * Returns 1 if an error has occurred otherwise returns 0.
 *
 ****************************************************************************/
int WsSend(struct WsConnection *connection, const char *message, size_t length)
{
  //if(_WsPollSend(connection) != 0)
  //{
  //  return 1;
  //}

  if(!connection->www)
  {
    unsigned char frame[10];  /* Frame.          */
    uint8_t idx_first_rData;  /* Index data.     */
    uint64_t len;             /* Message length. */

    /* Text data. */
    len = length;
    frame[0] = (WS_FIN | WS_FR_OP_TXT);

    if (len <= 125)
    {
      frame[1] = len & 0x7F;
      idx_first_rData = 2;
    }
    else if (len >= 126 && len <= 65535)
    {
      frame[1] = 126;
      frame[2] = (len >> 8) & 255;
      frame[3] = len & 255;
      idx_first_rData = 4;
    }
    else
    {
      frame[1] = 127;
      frame[2] = (unsigned char) ((len >> 56) & 255);
      frame[3] = (unsigned char) ((len >> 48) & 255);
      frame[4] = (unsigned char) ((len >> 40) & 255);
      frame[5] = (unsigned char) ((len >> 32) & 255);
      frame[6] = (unsigned char) ((len >> 24) & 255);
      frame[7] = (unsigned char) ((len >> 16) & 255);
      frame[8] = (unsigned char) ((len >> 8) & 255);
      frame[9] = (unsigned char) (len & 255);
      idx_first_rData = 10;
    }

    if(connection->outgoingLength + idx_first_rData + length > WS_MESSAGE_SIZE - 1)
    {
      printf("Too large\n");
      return 1;
    }

    memcpy(connection->outgoing + connection->outgoingLength, frame, idx_first_rData);
    memcpy(connection->outgoing + connection->outgoingLength + idx_first_rData, message, length);
    connection->outgoingLength += idx_first_rData + length;
  }
  else
  {
    sprintf(connection->outgoing,
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Content-Length: %i\r\n"
      "Connection: close\r\n"
      "\r\n", (int)length);

    connection->outgoingLength = strlen(connection->outgoing);

    if(connection->outgoingLength + length > WS_MESSAGE_SIZE - 1)
    {
      return 1;
    }

    memcpy(connection->outgoing + connection->outgoingLength, message, length);
    connection->outgoingLength += length;
  }

  if(_WsPollSend(connection) != 0)
  {
    return 1;
  }

  return 0;
}

/****************************************************************************
 * _WsPollHandshake
 *
 * If the header is complete, attempt handshake and send response. Ensure
 * that if there was trailing data that we move it to the beginning of the
 * buffer so that subsequent polls read it. Finally flag the connection as
 * established and populate connected event.
 *
 * server     - The server structure containing the message placeholder.
 * connection - The specific connection to process.
 * event      - The event structure to populate.
 *
 * Returns 1 if an event has occurred otherwise returns 0.
 *
 ****************************************************************************/
int _WsPollHandshake(struct WsServer *server, struct WsConnection *connection,
  struct WsEvent *event)
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
  }

  if(!headerFound)
  {
    return 0;
  }

  char *request = strdup(connection->incoming);
  char *response = _WsHandshakeResponse(request);
  free(request);

  connection->established = 1;
  event->connection = connection;

  if(response)
  {
    // TODO: Do we want to handle != 0?
    WsSend(connection, response, strlen(response));
    free(response);

    connection->incomingLength -= nextStart;

    memmove(connection->incoming, connection->incoming + nextStart,
      connection->incomingLength);

    memset(connection->incoming + connection->incomingLength, 0,
      WS_MESSAGE_SIZE - connection->incomingLength);

    event->type = WS_CONNECT;
  }
  else
  {
    event->type = WS_HTTP_REQUEST;
    connection->www = 1;
  }

  return 1;
}

/****************************************************************************
 * _WsPollReceive
 *
 * Attempt to decode incoming. If it fails, make assumption that the rest
 * of the data has not yet been received and continue. If data was decoded
 * successfully then populate message event.
 *
 * server     - The server structure containing the message placeholder.
 * connection - The specific connection to process.
 * event      - The event structure to populate.
 *
 * Returns 1 if an event has occurred otherwise returns 0.
 *
 ****************************************************************************/
int _WsPollReceive(struct WsServer *server, struct WsConnection *connection,
  struct WsEvent *event)
{
  int type = 0;
  size_t decodeLen = 0;
  size_t end = 0;

  char *msg = _WsDecodeFrame(connection->incoming,
    connection->incomingLength, &type, &decodeLen, &end);

  if(msg)
  {
    size_t msgLen = strlen(msg);

    event->type = WS_MESSAGE;
    event->connection = connection;
    event->message = &server->message;
    memcpy(event->message->data, msg, msgLen);
    free(msg);
    event->message->length = msgLen;

    connection->incomingLength -= end;

    memmove(connection->incoming, connection->incoming + end,
      connection->incomingLength);

    // Clear the rest of the buffer
    //memset(connection->incoming + connection->incomingLength, 0,
    //  WS_MESSAGE_SIZE - connection->incomingLength);

    return 1;
  }
/*
  else
  {
    connection->disconnected = 1;
    if(!connection->established) return 0;
    event->disconnect = &server->disconnect;
    event->disconnect->reason = WS_CLOSED;
    event->type = WS_DISCONNECT;
    event->connection = connection;

    return 1;
  }
*/

  return 0;
}

/****************************************************************************
 * _WsPollConnection
 *
 * Attempt to send outgoing. If there are any issues then disconnect. Read
 * bytes into incoming buffer if there are any available. If the buffer
 * becomes too large then disconnect the client. If connection is not
 * established then try for handshake, otherwise process message as normal.
 *
 * server     - The server structure required for event system.
 * connection - The specific connection to process.
 * event      - The event structure to populate.
 *
 * Returns 1 if an event has occurred otherwise returns 0.
 *
 ****************************************************************************/
int _WsPollConnection(struct WsServer *server,
  struct WsConnection *connection, struct WsEvent *event)
{
  fd_set readfds = {0};
  struct timeval tv = {0};
  int bytes = 0;

  if(_WsPollSend(connection) != 0)
  {
    connection->disconnected = 1;
    if(!connection->established) return 0;
    if(connection->www) return 0;
    event->disconnect = &server->disconnect;
    event->disconnect->reason = WS_NOSEND;
    event->type = WS_DISCONNECT;
    event->connection = connection;
    return 1;
  }

  FD_ZERO(&readfds);
  FD_SET(connection->fd, &readfds);

  if(
#ifdef USE_POSIX
    select(connection->fd + 1, &readfds, NULL, NULL, &tv) == -1
#endif
#ifdef USE_WINSOCK
    select(server->fd + 1, &readfds, NULL, NULL, &tv) == SOCKET_ERROR
#endif
  )
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
      if(connection->www) return 0;
      event->disconnect = &server->disconnect;
      event->disconnect->reason = WS_OVERFLOW;
      event->type = WS_DISCONNECT;
      event->connection = connection;
      return 1;
    }

    bytes =
#ifdef USE_POSIX
    read(connection->fd,
      connection->incoming + connection->incomingLength, bytes);
#endif
#ifdef USE_WINSOCK
    recv(connection->fd,
      connection->incoming + connection->incomingLength, bytes, 0);
#endif

    if(bytes == 0)
    {
      connection->disconnected = 1;
      if(!connection->established) return 0;
      if(connection->www) return 0;
      event->disconnect = &server->disconnect;
      event->disconnect->reason = WS_CLOSED;
      event->type = WS_DISCONNECT;
      event->connection = connection;
      return 1;
    }
#ifdef USE_POSIX
    else if(bytes == -1)
#endif
#ifdef USE_WINSOCK
    else if(bytes == SOCKET_ERROR)
#endif
    {
      return 0;
    }

    connection->incomingLength += bytes;
  }

  if(!connection->established)
  {
    return _WsPollHandshake(server, connection, event);
  }

  if(connection->www)
  {
    return 0;
  }

  return _WsPollReceive(server, connection, event);
}

/****************************************************************************
 * WS_LL_REMOVE
 *
 * Remove a node from the given list whilst joining up the next and prev.
 *
 * ROOT - The root node.
 * CURR - The current iterator.
 *
 ****************************************************************************/
#define WS_LL_REMOVE(ROOT, CURR)   \
{                                  \
  void *toFree = NULL;             \
  if(!CURR->prev)                  \
  {                                \
    ROOT = CURR->next;             \
  }                                \
  else                             \
  {                                \
    CURR->prev->next = CURR->next; \
  }                                \
                                   \
  if(CURR->next)                   \
  {                                \
    CURR->next->prev = CURR->prev; \
  }                                \
                                   \
  toFree = CURR;                   \
  CURR = CURR->next;               \
  free(toFree);                    \
}

/****************************************************************************
 * _WsPollConnections
 *
 * If any connections have been disconnected then remove them from the list.
 * For any others, poll them for any events. If one occurs then immediately
 * return it.
 *
 * server - The server structure containing the connections list.
 * event  - The event structure to populate.
 *
 * Returns 1 if an event has occurred otherwise returns 0.
 *
 ****************************************************************************/
int _WsPollConnections(struct WsServer *server, struct WsEvent *event)
{
  struct WsConnection *conn = NULL;

  conn = server->connections;

  while(conn)
  {
    if(conn->disconnected)
    {
#ifdef USE_POSIX
      close(conn->fd);
#endif
#ifdef USE_WINSOCK
      closesocket(conn->fd);
#endif
      WS_LL_REMOVE(server->connections, conn)
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

void _WsWaitForEvent(struct WsServer *server, int timeout)
{
  struct WsConnection *conn = NULL;
  fd_set readfds = {0};
  fd_set writefds = {0};
  struct timeval tv = {0};
  int maxFd = 0;

  tv.tv_sec = timeout / 1000;
  tv.tv_usec = timeout % 1000;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);

  FD_SET(server->fd, &readfds);
  maxFd = server->fd;
  conn = server->connections;

  while(conn)
  {
    FD_SET(conn->fd, &readfds);
    if(maxFd < conn->fd) maxFd = conn->fd;

    if(conn->outgoingLength > 0)
    {
      FD_SET(conn->fd, &writefds);
    }

    conn = conn->next;
  }

  select(maxFd, &readfds, &writefds, NULL, &tv);
}

/****************************************************************************
 * WsPoll
 *
 * Poll the client sockets and listen socket for events. If any occurs then
 * immediately return it.
 *
 * server - The server structure containing the listen socket.
 * event  - The event structure to populate.
 *
 * Returns 1 if an event has occurred otherwise returns 0.
 *
 ****************************************************************************/
int WsPoll(struct WsServer *server, int timeout, struct WsEvent *event)
{
  // TODO: Only clear when needed
  memset(event, 0, sizeof(*event));
  memset(&server->disconnect, 0, sizeof(server->disconnect));
  //memset(&server->message, 0, sizeof(server->message));

  if(timeout > 0)
  {
    _WsWaitForEvent(server, timeout);
  }

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

