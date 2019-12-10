#include "ws.h"
#include "TcpSocket.h"
#include "HttpHeader.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define WS_FIN 128
#define WS_FR_OP_TXT  1

struct WsServer
{
  ref(WsTcpSocket) socket;
  vector(ref(WsConnection)) connections;
  size_t nextToPoll;

  ref(WsHttpEvent) http;
  ref(WsMessageEvent) message;
  ref(WsDisconnectEvent) disconnect;
};

struct WsConnection
{
  ref(WsTcpSocket) socket;
  int established;
  int www;
  int websocket;
  int disconnected;
  vector(unsigned char) buffer;
  vector(unsigned char) incoming;
  vector(unsigned char) outgoing;
};

struct WsHttpRequest
{
  ref(WsConnection) connection;
  ref(sstream) path;
};

struct WsHttpResponse
{ 
  ref(WsConnection) connection;
  int headersSent;
};

vector(unsigned char) _WsHandshakeResponse(vector(unsigned char) request);
unsigned char *_WsHandshakeAccept(char *wsKey);

char *_WsDecodeFrame(vector(unsigned char) frame,
  int *type, size_t *decodeLen, size_t *end);

ref(WsServer) WsServerListen(int port)
{
  ref(WsServer) rtn = NULL;

  rtn = allocate(WsServer);

  /*
   * Allocate the event structures
   */
  _(rtn).disconnect = allocate(WsDisconnectEvent);
  _(rtn).message = allocate(WsMessageEvent);

  _(rtn).http = allocate(WsHttpEvent);
  _(_(rtn).http).request = allocate(WsHttpRequest);
  _(_(_(rtn).http).request).path = sstream_new();
  _(_(rtn).http).response = allocate(WsHttpResponse);

  _(rtn).connections = vector_new(ref(WsConnection));
  _(rtn).socket = WsTcpListen(port);

  return rtn;
}

void _WsConnectionDestroy(ref(WsConnection) ctx)
{
  WsTcpSocketClose(_(ctx).socket);
  vector_delete(_(ctx).buffer);
  vector_delete(_(ctx).incoming);
  vector_delete(_(ctx).outgoing);
  release(ctx);
}

void _WsPollConnectionRequests(ref(WsServer) server)
{
  ref(WsConnection) conn = NULL;

  /*
   * Keep processing whilst clients waiting
   */
  while(WsTcpSocketReady(_(server).socket) == 1)
  {
    /*
     * Wrap the socket as a connection and add to list
     */
    conn = allocate(WsConnection);
    _(conn).socket = WsTcpSocketAccept(_(server).socket);
    _(conn).buffer = vector_new(unsigned char);
    _(conn).incoming = vector_new(unsigned char);
    _(conn).outgoing = vector_new(unsigned char);
    vector_push_back(_(server).connections, conn);
  }
}

// TODO Do we need to return anything?
int _WsPollSend(ref(WsConnection) connection)
{
  if(vector_size(_(connection).outgoing) == 0)
  {
    if(_(connection).www)
    {
      _(connection).disconnected = 1;
    }

    return 0;
  }

  WsTcpSocketSend(_(connection).socket, _(connection).outgoing);

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
int WsSend(ref(WsConnection) connection, const char *message, size_t length)
{
  int ci = 0;

  if(_(connection).websocket)
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

    for(ci = 0; ci < idx_first_rData; ci++)
    {
      vector_push_back(_(connection).outgoing, frame[ci]);
    }

    for(ci = 0; ci < length; ci++)
    {
      vector_push_back(_(connection).outgoing, message[ci]);
    }

    /*printf("Message: %s\n", connection->outgoing);*/
    /*printf("Message: %s\n", message);*/
  }
  else if(_(connection).www)
  {
    char header[512] = {0};
    int headerLen = 0;

    sprintf(header,
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Content-Length: %i\r\n"
      "Connection: close\r\n"
      "\r\n", (int)length);

    headerLen = strlen(header);

    for(ci = 0; ci < headerLen; ci++)
    {
      vector_push_back(_(connection).outgoing, header[ci]);
    }

    for(ci = 0; ci < length; ci++)
    {
      vector_push_back(_(connection).outgoing, message[ci]);
    }
  }
  else
  {
    printf("TODO: Do we reach here?\n");

    for(ci = 0; ci < length; ci++)
    {
      vector_push_back(_(connection).outgoing, message[ci]);
    }
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
int _WsPollHandshake(ref(WsServer) server, ref(WsConnection) connection,
  struct WsEvent *event)
{
  size_t i = 0;
  int headerFound = 0;
  size_t nextStart = 0;
  char *request = NULL;
  vector(unsigned char) response = NULL;
  ref(HttpHeader) header = NULL;

  /*
   * Attempt to obtain header from the current incoming buffer.
   */
  header = HttpHeaderCreate(_(connection).incoming);

  /*
   * No complete header could be found. Early return for now.
   */
  if(header == NULL)
  {
    return 0;
  }

  response = _WsHandshakeResponse(_(connection).incoming);

  _(connection).established = 1;
  event->connection = connection;

  if(response)
  {
    WsSend(connection, &vector_at(response, 0), vector_size(response));
    _(connection).websocket = 1;
    vector_delete(response);
    vector_erase(_(connection).incoming, 0, nextStart);
    event->type = WS_CONNECT;
  }
  else
  {
    event->type = WS_HTTP_REQUEST;
    event->http = _(server).http;

    _(_(event->http).response).connection = connection;
    _(_(event->http).response).headersSent = 0;

    _(_(event->http).request).connection = connection;
    sstream_str_cstr(_(_(event->http).request).path, "");

    sstream_append_cstr(_(_(event->http).request).path,
      sstream_cstr(HttpHeaderPath(header)));

    _(connection).www = 1;

    /* TODO: Send default request? */
    //WsSend(connection, "Test data", 9);
  }

  HttpHeaderDestroy(header);

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
int _WsPollReceive(ref(WsServer) server, ref(WsConnection) connection,
  struct WsEvent *event)
{
  int type = 0;
  size_t decodeLen = 0;
  size_t end = 0;
  ref(WsMessageEvent) message = NULL;

  char *msg = _WsDecodeFrame(_(connection).incoming,
    &type, &decodeLen, &end);

  if(msg)
  {
    size_t msgLen = strlen(msg);

    /*
     * Add message event data
     */
    message = _(server).message;
    memcpy(_(message).data, msg, msgLen);
    free(msg);
    _(message).length = msgLen;

    /*
     * Prepare base event structure
     */
    event->type = WS_MESSAGE;
    event->connection = connection;
    event->message = message;

    vector_erase(_(connection).incoming, 0, end);

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
int _WsPollConnection(ref(WsServer) server, ref(WsConnection) connection,
  struct WsEvent *event)
{
  ref(WsDisconnectEvent) disconnect = NULL;

  /*
   * Flush remaining outgoing data
   */
   _WsPollSend(connection);

  /*
   * Send disconnect event and return if socket no longer connected.
   */
  if(WsTcpSocketConnected(_(connection).socket) == 0)
  {
    _(connection).disconnected = 1;
    if(!_(connection).established) return 0;
    if(_(connection).www) return 0;

    /*
     * Add disconnect event data
     */
    disconnect = _(server).disconnect;
    _(disconnect).reason = WS_NOSEND;

    /*
     * Prepare base event structure
     */
    event->type = WS_DISCONNECT;
    event->connection = connection;
    event->disconnect = disconnect;

    return 1;
  }

  /*
   * Return if no data is waiting.
   */
  if(WsTcpSocketReady(_(connection).socket) == 0)
  {
    return 0;
  }

  vector_resize(_(connection).buffer, 1024);
  WsTcpSocketRecv(_(connection).socket, _(connection).buffer);

  /*
   * If no data was received, this means connection broken.
   */
  if(vector_size(_(connection).buffer) == 0)
  {
    printf("This should never happen\n");

    /*
     * Flag that connection is ready to be removed.
     */
    _(connection).disconnected = 1;

    /*
     * Ignore if connection never established or http so no initial
     * connection event was broadcast.
     */
    if(!_(connection).established) return 0;
    if(_(connection).www) return 0;

    /*
     * Add disconnect event data
     */
    disconnect = _(server).disconnect;
    _(disconnect).reason = WS_CLOSED;

    /*
     * Prepare base event structure
     */
    event->type = WS_DISCONNECT;
    event->connection = connection;
    event->disconnect = disconnect;

    return 1;
  }

  /*
   * Copy data from buffer into incoming stream.
   */
  vector_insert(_(connection).incoming,
    vector_size(_(connection).incoming),
    _(connection).buffer, 0,
    vector_size(_(connection).buffer));

  if(!_(connection).established)
  {
    return _WsPollHandshake(server, connection, event);
  }

  if(_(connection).www)
  {
    printf("Warning: HTTP client is sending more data?\n");
    return 0;
  }

  return _WsPollReceive(server, connection, event);
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
int _WsPollConnections(ref(WsServer) server, struct WsEvent *event)
{
  size_t curr = 0;
  size_t end = 0;
  ref(WsConnection) conn = NULL;

  curr = _(server).nextToPoll;
  end = curr;

  while(1)
  {
    if(vector_size(_(server).connections) < 1)
    {
      _(server).nextToPoll = 0;

      return 0;
    }

    if(curr >= vector_size(_(server).connections))
    {
      curr = 0;
    }

    if(end >= vector_size(_(server).connections))
    {
      end = vector_size(_(server).connections) - 1;
    }

    conn = vector_at(_(server).connections, curr);

    if(_(conn).disconnected == 1)
    {
      _WsConnectionDestroy(conn);
      vector_erase(_(server).connections, curr, 1);
    }
    else
    {
      if(_WsPollConnection(server, conn, event))
      {
        _(server).nextToPoll++;

        return 1;
      }

      curr++;

      if(curr >= vector_size(_(server).connections))
      {
        curr = 0;
      }

      if(curr == end) break;
    }
  }

  return 0;
}

int _WsWaitForEvent(ref(WsServer) server, int timeout)
{
  vector(ref(WsTcpSocket)) reads = NULL;
  vector(ref(WsTcpSocket)) writes = NULL;
  size_t ci = 0;
  ref(WsConnection) conn = NULL;
  int rtn = 0;

  reads = vector_new(ref(WsTcpSocket));
  writes = vector_new(ref(WsTcpSocket));
  vector_push_back(reads, _(server).socket);

  for(ci = 0; ci < vector_size(_(server).connections); ci++)
  {
    conn = vector_at(_(server).connections, ci);
    vector_push_back(reads, _(conn).socket);

    if(vector_size(_(conn).outgoing) > 0)
    {
      vector_push_back(writes, _(conn).socket);
    }
  }

  rtn = WsTcpSocketsReady(reads, writes, timeout);
  vector_delete(reads);
  vector_delete(writes);

  return rtn;
}

/****************************************************************************
 * WsServerPoll
 *
 * Poll the client sockets and listen socket for events. If any occurs then
 * immediately return it.
 *
 * server - The server structure containing the listen socket.
 * event  - The event structure to populate.
 *
 * Returns event if one has occurred otherwise returns NULL.
 *
 ****************************************************************************/
int WsServerPoll(ref(WsServer) server, int timeout, struct WsEvent *event)
{
  // TODO: Only clear when needed
  //sreset(_(server).disconnect);
  //sreset(_(server).message);
  //sreset(_(server).http);

  /*
   * If timeout specified or infinite (-1) then batch select to wait for
   * an event to occur in that amount of time. If 0 timeout then just
   * process sockets as usual because waiting has some overhead.
   */
  if(timeout != 0)
  {
    /*
     * If sockets don't report any events, then early exit.
     */
    if(!_WsWaitForEvent(server, timeout))
    {
      return 0;
    }
  }

  _WsPollConnectionRequests(server);

  if(_WsPollConnections(server, event))
  {
    return 1;
  }

  return 0;
}

void WsServerClose(ref(WsServer) server)
{
  size_t ci = 0;

  release(_(server).disconnect);
  release(_(server).message);
  sstream_delete(_(_(_(server).http).request).path);
  release(_(_(server).http).response);
  release(_(_(server).http).request);
  release(_(server).http);

  for(ci = 0; ci < vector_size(_(server).connections); ci++)
  {
    _WsConnectionDestroy(vector_at(_(server).connections, ci));
  }

  vector_delete(_(server).connections);
  WsTcpSocketClose(_(server).socket);
  release(server);
}

ref(WsHttpRequest) WsHttpEventRequest(ref(WsHttpEvent) ctx)
{
  return _(ctx).request;
}

ref(WsHttpResponse) WsHttpEventResponse(ref(WsHttpEvent) ctx)
{
  return _(ctx).response;
}

void WsHttpResponseWrite(ref(WsHttpResponse) response, char *data)
{
  if(_(response).headersSent != 0)
  {
    _WsPanic("Headers already sent");
  }

  WsSend(_(response).connection, data, strlen(data));
  _(response).headersSent = 1;
}

ref(sstream) WsHttpRequestPath(ref(WsHttpRequest) request)
{
  return _(request).path;
}
