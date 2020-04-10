#include "TcpSocket.h"

#define STENT_IMPLEMENTATION
#include "stent.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef _WIN32
  #define USE_WINSOCK
#else
  #define USE_POSIX
#endif

#define TCPSOCKET_BACKLOG 32

#ifdef USE_POSIX
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/select.h>
  #include <signal.h>
#endif

#ifdef USE_WINSOCK
  //#define WIN32_LEAN_AND_MEAN
  //#include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
#endif

#define STATE_DISCONNECTED 0
#define STATE_CONNECTED 1
#define STATE_LISTEN 2

struct WsTcpSocket
{
#ifdef USE_WINSOCK
  SOCKET fd;
#endif
#ifdef USE_POSIX
  int fd;
#endif
  int state;
  int handshake;

  SSL *ssl;
  SSL_CTX *sslContext;
  vector(unsigned char) outgoing;
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

  // TODO: Clean up
  signal(SIGPIPE, SIG_IGN);

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

  ERR_clear_error();
  _(rtn).sslContext = SSL_CTX_new(SSLv23_server_method());

  if(!_(rtn).sslContext)
  {
#ifdef USE_POSIX
    close(_(rtn).fd);
#endif
#ifdef USE_WINSOCK
    closesocket(_(rtn).fd);
#endif
    release(rtn);
    _WsPanic("Failed to create SSL context");
  }

  SSL_CTX_set_ecdh_auto(_(rtn).sslContext, 1);

  if(SSL_CTX_use_certificate_file(_(rtn).sslContext, "cert.pem", SSL_FILETYPE_PEM) <= 0)
  {
    _WsPanic("Failed to apply certificate");
  }

  if(SSL_CTX_use_PrivateKey_file(_(rtn).sslContext, "key.pem", SSL_FILETYPE_PEM) <= 0 )
  {
    _WsPanic("Failed to apply private key");
  }

  _(rtn).state = STATE_LISTEN;

  return rtn;
}

void WsTcpSocketClose(ref(WsTcpSocket) ctx)
{
  if(_(ctx).handshake)
  {
    // TODO: Send to completion.
    SSL_shutdown(_(ctx).ssl);
  }

  /*
   * A client socket has slightly different data to a listen socket.
   */
  if(_(ctx).outgoing)
  {
    vector_delete(_(ctx).outgoing);
    SSL_free(_(ctx).ssl);
  }
  else
  {
    SSL_CTX_free(_(ctx).sslContext);
  }

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
  if(_(ctx).state == STATE_DISCONNECTED)
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
  ref(WsTcpSocket) sock = NULL;

  tv.tv_sec = timeout / 1000;
  tv.tv_usec = timeout % 1000;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);

  for(i = 0; i < vector_size(reads); i++)
  {
    sock = vector_at(reads, i);

    cfd = _(sock).fd;
    FD_SET(cfd, &readfds);

    if(cfd > mfd)
    {
      mfd = cfd;
    }
  }

  for(i = 0; i < vector_size(writes); i++)
  {
    sock = vector_at(writes, i);

    cfd = _(sock).fd;
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

int DoneHandshake(ref(WsTcpSocket) ctx)
{
  int sslResult = 0;

  if(_(ctx).handshake)
  {
    return 1;
  }

  ERR_clear_error();
  sslResult = SSL_accept(_(ctx).ssl);
  //printf("[handshake] %i\n", sslResult);

  if(sslResult < 1)
  {
    sslResult = SSL_get_error(_(ctx).ssl, sslResult);

    if(sslResult == SSL_ERROR_WANT_READ || sslResult == SSL_ERROR_WANT_WRITE)
    {
      //printf("[handshake_notready]\n");
    }
    else
    {
      //printf("[handshake_error] %i\n", sslResult);
      _(ctx).state = STATE_DISCONNECTED;
    }

    return 0;
  }

  //printf("[handshake_success]\n");
  _(ctx).handshake = 1;

  return 1;
}

void WsTcpSocketSend(ref(WsTcpSocket) ctx,
  vector(unsigned char) data)
{
  int res = 0;

  if(_(ctx).state != STATE_CONNECTED)
  {
    return;
  }

  if(!DoneHandshake(ctx))
  {
    return;
  }

  if(vector_size(data) < 1)
  {
    panic("Cannot send 0 bytes");
  }

  /*
   * If no outgoing buffer, then set that as the new outgoing to avoid the
   * sliding data limitation of OpenSSL.
   */
  if(vector_size(_(ctx).outgoing) < 1)
  {
    vector_insert(_(ctx).outgoing, 0, data, 0, vector_size(data));
  }

  ERR_clear_error();
  res = SSL_write(_(ctx).ssl, &vector_at(_(ctx).outgoing, 0), vector_size(_(ctx).outgoing));

  if(res < 1)
  {
    res = SSL_get_error(_(ctx).ssl, res);

    if(res != SSL_ERROR_WANT_READ && res != SSL_ERROR_WANT_WRITE)
    {
      _(ctx).state = STATE_DISCONNECTED;
    }

    return;
  }

  if(vector_size(data) < vector_size(_(ctx).outgoing))
  {
    panic("Sliding data not supported. Data is smaller");
  }

  if(memcmp(&vector_at(data, 0), &vector_at(_(ctx).outgoing, 0), vector_size(_(ctx).outgoing)) != 0)
  {
    panic("Sliding data not supported. Data differs");
  }

  vector_erase(data, 0, vector_size(_(ctx).outgoing));
  vector_clear(_(ctx).outgoing);
}

void WsTcpSocketRecv(ref(WsTcpSocket) ctx,
  vector(unsigned char) data)
{
  int res = 0;

  if(_(ctx).state != STATE_CONNECTED)
  {
    vector_clear(data);
    return;
  }

  if(!DoneHandshake(ctx))
  {
    vector_clear(data);
    return;
  }

  res = vector_size(data);
  ERR_clear_error();
  res = SSL_read(_(ctx).ssl, &vector_at(data, 0), res);

  if(res < 1)
  {
    res = SSL_get_error(_(ctx).ssl, res);
    vector_clear(data);

    if(res != SSL_ERROR_WANT_READ && res != SSL_ERROR_WANT_WRITE)
    {
      _(ctx).state = STATE_DISCONNECTED;
    }

    return;
  }

  vector_resize(data, res);
}

ref(WsTcpSocket) WsTcpSocketWrapFd(int fd, ref(WsTcpSocket) l)
{
  ref(WsTcpSocket) rtn = NULL;

  rtn = allocate(WsTcpSocket);
  _(rtn).fd = fd;
  _(rtn).outgoing = vector_new(unsigned char);

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

  ERR_clear_error();
  _(rtn).ssl = SSL_new(_(l).sslContext);

  if(!_(rtn).ssl)
  {
    _WsPanic("Failed to create new SSL instance");
  }

  SSL_set_fd(_(rtn).ssl, _(rtn).fd);

  _(rtn).state = STATE_CONNECTED;

  return rtn;
}

ref(WsTcpSocket) WsTcpSocketAccept(ref(WsTcpSocket) ctx)
{
#ifdef USE_POSIX
  struct sockaddr_in client = {0};
  int val = 1;
  fd_set readfds = {0};
  struct timeval tv = {0};
#endif
#ifdef USE_WINSOCK
  unsigned long val = 1;
#endif
  ref(WsTcpSocket) rtn = NULL;
  int len = 0;
  int sslResult = 0;
  int acceptFd = 0;

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
    return NULL;
  }

  acceptFd =
#ifdef USE_POSIX
    accept(_(ctx).fd, (struct sockaddr *)&client, (socklen_t *)&len);
#endif
#ifdef USE_WINSOCK
    accept(_(ctx).fd, NULL, NULL);
#endif

  if(
#ifdef USE_POSIX
    acceptFd == -1
#endif
#ifdef USE_WINSOCK
    acceptFd == INVALID_SOCKET
#endif
  )
  {
    _WsPanic("Failed to accept socket");
  }

  rtn = WsTcpSocketWrapFd(acceptFd, ctx);

  return rtn;
}

