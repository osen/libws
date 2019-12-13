#include "TcpSocket.h"

#define STENT_IMPLEMENTATION
#include "stent.h"

#ifdef _WIN32
  #define USE_WINSOCK
#else
  #define USE_POSIX
#endif

#define TCPSOCKET_BACKLOG 8

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

struct WsTcpSocket
{
#ifdef USE_WINSOCK
  SOCKET fd;
#endif
#ifdef USE_POSIX
  int fd;
#endif
  int state;
};

void _WsPanic(char *message)
{
  printf("Error: %s\n", message);
  abort();
}

#ifdef USE_WINSOCK
static int wsaInitialized;

static void WsCleanupWinsock(void)
{
  WSACleanup();
}
#endif

ref(WsTcpSocket) WsTcpListen(int port)
{
  ref(WsTcpSocket) rtn = NULL;

#ifdef USE_POSIX
  struct sockaddr_in server = {0};
  int val = 1;
#endif
#ifdef USE_WINSOCK
  struct addrinfo hints = {0};
  struct addrinfo *result = NULL;
  char portString[128] = {0};
  WSADATA wsaData = {0};
  unsigned long val = 1;

  if(!wsaInitialized)
  {
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
      _WsPanic("Failed to initialize Winsock");
    }

    atexit(WsCleanupWinsock);
    wsaInitialized = 1;
  }

  if(!itoa(port, portString, 10))
  {
    _WsPanic("Invalid port specified");
  }

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  if(getaddrinfo(NULL, portString, &hints, &result) != 0)
  {
    _WsPanic("Failed to obtain address hints");
  }
#endif

  rtn = allocate(WsTcpSocket);

#ifdef USE_POSIX
  /* TODO: (SOCK_STREAM | SOCK_NONBLOCK) 100% portable? */

  _(rtn).fd = socket(AF_INET, SOCK_STREAM, 0);

  if(_(rtn).fd == -1)
#endif
#ifdef USE_WINSOCK
  _(rtn).fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

  if(_(rtn).fd == INVALID_SOCKET)
#endif
  {
#ifdef USE_WINSOCK
    freeaddrinfo(result);
#endif
    release(rtn);
    _WsPanic("Failed to create socket");
  }

#ifdef USE_POSIX
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
#endif

  if(
#ifdef USE_POSIX
    setsockopt(_(rtn).fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1 ||
    fcntl(_(rtn).fd, F_SETFL, O_NONBLOCK) == -1 ||
    bind(_(rtn).fd, (struct sockaddr *)&server, sizeof(server)) == -1 ||
    listen(_(rtn).fd, TCPSOCKET_BACKLOG) == -1
#endif
#ifdef USE_WINSOCK
    ioctlsocket(_(rtn).fd, FIONBIO, &val) != NO_ERROR ||
    bind(_(rtn).fd, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR ||
    listen(_(rtn).fd, SOMAXCONN) == SOCKET_ERROR
#endif
  )
  {
#ifdef USE_POSIX
    close(_(rtn).fd);
#endif
#ifdef USE_WINSOCK
    freeaddrinfo(result);
    closesocket(_(rtn).fd);
#endif
    release(rtn);
    _WsPanic("Failed to bind and listen socket");
  }

#ifdef USE_WINSOCK
  freeaddrinfo(result);
#endif

  _(rtn).state = 1;

  return rtn;
}

void WsTcpSocketClose(ref(WsTcpSocket) ctx)
{
#ifdef USE_POSIX
  close(_(ctx).fd);
#endif
#ifdef USE_WINSOCK
  closesocket(_(ctx).fd);
#endif

  release(ctx);
}

int WsTcpSocketConnected(ref(WsTcpSocket) ctx)
{
  if(_(ctx).state == 0)
  {
    return 0;
  }

  WsTcpSocketReady(ctx);

  if(_(ctx).state == 0)
  {
    return 0;
  }

  return 1;
}

int WsTcpSocketsReady(vector(ref(WsTcpSocket)) reads,
  vector(ref(WsTcpSocket)) writes, int timeout)
{
  fd_set readfds = {0};
  fd_set writefds = {0};
  struct timeval tv = {0};
  size_t i = 0;
  int cfd = 0;
  int mfd = 0;

  tv.tv_sec = timeout / 1000;
  tv.tv_usec = timeout % 1000;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);

  for(i = 0; i < vector_size(reads); i++)
  {
    cfd = _(vector_at(reads, i)).fd;
    FD_SET(cfd, &readfds);

    if(cfd > mfd)
    {
      mfd = cfd;
    }
  }

  for(i = 0; i < vector_size(writes); i++)
  {
    cfd = _(vector_at(writes, i)).fd;
    FD_SET(cfd, &writefds);

    if(cfd > mfd)
    {
      mfd = cfd;
    }
  }

  if(
#ifdef USE_POSIX
    select(mfd + 1, &readfds, &writefds, NULL, (timeout >= 0 ? &tv : NULL)) == -1
#endif
#ifdef USE_WINSOCK
    select(mfd + 1, &readfds, &writefds, NULL, (timeout >= 0 ? &tv : NULL)) == SOCKET_ERROR
#endif
  )
  {
    _WsPanic("Failed to poll socket");
  }

  for(i = 0; i < vector_size(reads); i++)
  {
    cfd = _(vector_at(reads, i)).fd;

    if(FD_ISSET(cfd, &readfds))
    {
      return 1;
    }
  }

  for(i = 0; i < vector_size(writes); i++)
  {
    cfd = _(vector_at(writes, i)).fd;

    if(FD_ISSET(cfd, &writefds))
    {
      return 1;
    }
  }

  return 0;
}

/* TODO: Take a read and write socket */
int WsTcpSocketReady(ref(WsTcpSocket) ctx)
{
  fd_set readfds = {0};
  struct timeval tv = {0};
  int bytes = 0;
  char buf = 0;

  if(_(ctx).state == 0)
  {
    return 0;
  }

  FD_ZERO(&readfds);
  FD_SET(_(ctx).fd, &readfds);

  if(
#ifdef USE_POSIX
    select(_(ctx).fd + 1, &readfds, NULL, NULL, &tv) == -1
#endif
#ifdef USE_WINSOCK
    select(_(ctx).fd + 1, &readfds, NULL, NULL, &tv) == SOCKET_ERROR
#endif
  )
  {
    _WsPanic("Failed to poll socket");
  }

  if(!FD_ISSET(_(ctx).fd, &readfds))
  {
    return 0;
  }

  /*
   * If socket is a client socket (not a listen socket), peek at the data
   * to see if the event was due to a message received or due to disconnect.
   * If disconnect, set the state to 0 (disconnected).
   */
  if(_(ctx).state == 2)
  {
    bytes = recv(_(ctx).fd, &buf, 1, MSG_PEEK);

    if(bytes < 1)
    {
      _(ctx).state = 0;

      return 0;
    }

    return 1;
  }
  else if(_(ctx).state == 1)
  {
    return 1;
  }
  else
  {
    _WsPanic("Socket in unknown state had event waiting");
  }

  return 0;
}

void WsTcpSocketSend(ref(WsTcpSocket) ctx,
  vector(unsigned char) data)
{
  int bytes = 0;

  if(_(ctx).state == 0)
  {
    _WsPanic("Socket is disconnected");
  }

  if(_(ctx).state != 2)
  {
    _WsPanic("Socket is in an invalid state");
  }

  bytes = send(_(ctx).fd, &vector_at(data, 0), vector_size(data), MSG_NOSIGNAL);
  vector_erase(data, 0, bytes);
}

void WsTcpSocketRecv(ref(WsTcpSocket) ctx,
  vector(unsigned char) data)
{
  int bytes = 0;

  if(_(ctx).state == 0)
  {
    _WsPanic("Socket is disconnected");
  }

  if(_(ctx).state != 2)
  {
    _WsPanic("Socket is in an invalid state");
  }

  bytes = vector_size(data);

  bytes =
#ifdef USE_POSIX
    read(_(ctx).fd,
      &vector_at(data, 0), bytes);
#endif
#ifdef USE_WINSOCK
    recv(_(ctx).fd,
      &vector_at(data, 0), bytes, 0);
#endif

  if(bytes < 1)
  {
    _WsPanic("Failed to receive data");
  }

  if(bytes < vector_size(data))
  {
    vector_resize(data, bytes);
  }
}

ref(WsTcpSocket) WsTcpSocketAccept(ref(WsTcpSocket) ctx)
{
#ifdef USE_POSIX
  struct sockaddr_in client = {0};
  int val = 1;
#endif
#ifdef USE_WINSOCK
  unsigned long val = 1;
#endif
  ref(WsTcpSocket) rtn = NULL;
  int len = 0;

  rtn = allocate(WsTcpSocket);

  _(rtn).fd =
#ifdef USE_POSIX
    accept(_(ctx).fd, (struct sockaddr *)&client, (socklen_t *)&len);
#endif
#ifdef USE_WINSOCK
    accept(_(ctx).fd, NULL, NULL);
#endif

  if(
#ifdef USE_POSIX
    _(rtn).fd == -1
#endif
#ifdef USE_WINSOCK
    _(rtn).fd == INVALID_SOCKET
#endif
  )
  {
    release(rtn);
    _WsPanic("Failed to accept socket");
  }

  if(
#ifdef USE_POSIX
    fcntl(_(rtn).fd, F_SETFL, O_NONBLOCK) == -1
#endif
#ifdef USE_WINSOCK
    ioctlsocket(_(rtn).fd, FIONBIO, &val) != NO_ERROR
#endif
  )
  {
#ifdef USE_POSIX
    close(_(rtn).fd);
#endif
#ifdef USE_WINSOCK
    closesocket(_(rtn).fd);
#endif
    release(rtn);
    _WsPanic("Failed to set socket to non-blocking");
  }

  _(rtn).state = 2;

  return rtn;
}

