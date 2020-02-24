#include "ws.h"
#include "TcpSocket.h"
#include "HttpHeader.h"
#include "WebSocket.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define WS_FIN 128
#define WS_FR_OP_TXT  1

#define WS_STATE_INITIAL 0
#define WS_STATE_HTTP 1
#define WS_STATE_WEBSOCKET 2
#define WS_STATE_DISCONNECTED 3

struct WsServer
{
  ref(WsTcpSocket) socket;
  vector(ref(WsConnection)) connections;
  size_t nextToPoll;
  struct WsEvent events;
};

struct WsConnection
{
  ref(WsTcpSocket) socket;
  int state;
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
  int status;
  vector(unsigned char) data;
};

unsigned char *_WsHandshakeAccept(char *wsKey);

ref(WsServer) WsServerListen(int port)
{
  ref(WsServer) rtn = NULL;

  rtn = allocate(WsServer);

  /*
   * Allocate the event structures
   */
  _(rtn).events.disconnect = allocate(WsDisconnectEvent);
  _(rtn).events.message = allocate(WsMessageEvent);
  _(_(rtn).events.message).data = vector_new(unsigned char);

  _(rtn).events.http = allocate(WsHttpEvent);
  _(_(rtn).events.http).request = allocate(WsHttpRequest);
  _(_(_(rtn).events.http).request).path = sstream_new();
  _(_(rtn).events.http).response = allocate(WsHttpResponse);
  _(_(_(rtn).events.http).response).data = vector_new(unsigned char);

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

void _WsConnectionPollSend(ref(WsConnection) connection)
{
  /*
   * If the buffer is empty, early return.
   */
  if(vector_size(_(connection).outgoing) < 1)
  {
    return;
  }

  WsTcpSocketSend(_(connection).socket, _(connection).outgoing);
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
 ****************************************************************************/
void WsSend(ref(WsConnection) connection, vector(unsigned char) data)
{
  int ci = 0;
  unsigned char frame[10];  /* Frame.          */
  uint8_t idx_first_rData;  /* Index data.     */
  uint64_t len;             /* Message length. */

  if(_(connection).state != WS_STATE_WEBSOCKET)
  {
    _WsPanic("Invalid socket state");
  }

  /* Text data. */
  len = vector_size(data);
  frame[0] = (WS_FIN | WS_FR_OP_TXT);

  if(len <= 125)
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

  for(ci = 0; ci < len; ci++)
  {
    vector_push_back(_(connection).outgoing, vector_at(data, ci));
  }

  _WsConnectionPollSend(connection);
}

/****************************************************************************
 * _WsConnectionProcessInitial
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
int _WsConnectionProcessInitial(ref(WsServer) server, ref(WsConnection) connection,
  struct WsEvent *event)
{
  size_t i = 0;
  int headerFound = 0;
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

  event->connection = connection;
  response = _WsHandshakeResponse(header);

  if(response)
  {
    _(connection).state = WS_STATE_WEBSOCKET;

    vector_insert(_(connection).outgoing,
      vector_size(_(connection).outgoing),
      response, 0, vector_size(response));

    _WsConnectionPollSend(connection);
    vector_delete(response);

    event->type = WS_CONNECT;
  }
  else
  {
    event->type = WS_HTTP_REQUEST;
    event->http = _(server).events.http;

    _(_(event->http).response).connection = connection;
    _(_(event->http).response).headersSent = 0;
    _(_(event->http).response).status = 200;
    vector_clear(_(_(event->http).response).data);

    _(_(event->http).request).connection = connection;
    sstream_str_cstr(_(_(event->http).request).path, "");

    sstream_append_cstr(_(_(event->http).request).path,
      sstream_cstr(HttpHeaderPath(header)));

    _(connection).state = WS_STATE_HTTP;
  }

  HttpHeaderDestroy(header);

  return 1;
}

/****************************************************************************
 * _WsConnectionProcessWebSocket
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
int _WsConnectionProcessWebSocket(ref(WsServer) server, ref(WsConnection) connection,
  struct WsEvent *event)
{
  ref(WsMessageEvent) message = NULL;
  struct WsFrameInfo fi = {0};
  ref(WsDisconnectEvent) disconnect = NULL;
  int fr = 0;

  message = _(server).events.message;
  fr = _WsDecodeFrame(_(connection).incoming, &fi, _(message).data);

  /*
   * Early return with incomplete frame
   */
  if(!fr)
  {
    return 0;
  }

  if(!fi.final)
  {
    printf("Not final...\n");
  }

  if(!fi.masked)
  {
    printf("Not masked...\n");
  }

  if(fi.opcode == WS_FRAME_CLOSE)
  {
    /*
     * Add disconnect event data
     */
    disconnect = _(server).events.disconnect;
    _(disconnect).reason = WS_CLOSED;

    /*
     * Prepare base event structure
     */
    event->type = WS_DISCONNECT;
    event->connection = connection;
    event->disconnect = disconnect;
    _(connection).state = WS_STATE_DISCONNECTED;

    return 1;
  }
  else if(fi.opcode == WS_FRAME_TEXT)
  {
    /*
     * TODO: We may want to allow empty messages so should not check for
     * vector_size(_(message).data) > 0.
     */

    /*
     * Prepare base event structure
     */
    event->type = WS_MESSAGE;
    event->connection = connection;
    event->message = message;

    /*
     * Remove the packet data from the incoming stream
     */
    vector_erase(_(connection).incoming, 0, fi.dataEnd);

    return 1;
  }
  else
  {
    printf("TODO: Unknown opcode");
  }

  return 0;
}

/****************************************************************************
 * _WsConnectionPoll
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
int _WsConnectionPoll(ref(WsServer) server, ref(WsConnection) connection,
  struct WsEvent *event)
{
  ref(WsDisconnectEvent) disconnect = NULL;
  int state = 0;

  /*
   * Send disconnect event and return if socket no longer connected.
   */
  if(WsTcpSocketConnected(_(connection).socket) == 0)
  {
    state = _(connection).state;
    _(connection).state = WS_STATE_DISCONNECTED;

    /*
     * If the state was initial or http, then don't generate a disconnect event.
     * The user was never aware of them connecting in the first place.
     */
    if(state == WS_STATE_INITIAL ||
      state == WS_STATE_HTTP)
    {
      return 0;
    }

    /*
     * Add disconnect event data
     */
    disconnect = _(server).events.disconnect;
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
   * Flush any outgoing data
   */
   _WsConnectionPollSend(connection);

  /*
   * If connection is Http and buffer is empty, assume that the
   * response has been sent and flag for disconnection.
   */
  if(_(connection).state == WS_STATE_HTTP)
  {
    if(vector_size(_(connection).outgoing) < 1)
    {
      _(connection).state = WS_STATE_DISCONNECTED;

      return 0;
    }
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
   * Copy data from buffer into incoming stream.
   */
  vector_insert(_(connection).incoming,
    vector_size(_(connection).incoming),
    _(connection).buffer, 0,
    vector_size(_(connection).buffer));

  if(_(connection).state == WS_STATE_INITIAL)
  {
    return _WsConnectionProcessInitial(server, connection, event);
  }
  else if(_(connection).state == WS_STATE_WEBSOCKET)
  {
    return _WsConnectionProcessWebSocket(server, connection, event);
  }

  _WsPanic("Should not reach");

  return 0;
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
  size_t begin = 0;
  size_t incs = 0;
  ref(WsConnection) conn = NULL;
  size_t ci = 0;
  int rtn = 0;

  /*
   * Early return if no connections to poll.
   */
  if(vector_size(_(server).connections) < 1)
  {
    return 0;
  }

  begin = _(server).nextToPoll;
  ci = begin;

  while(1)
  {
    /*
     * If the end of the vector has been reached, start from beginning again
     * until the end element is encountered (first connection we started with).
     */
    if(ci >= vector_size(_(server).connections))
    {
      ci = 0;
    }

    if(ci == begin && incs > 0)
    {
      break;
    }

    incs++;
    conn = vector_at(_(server).connections, ci);

    if(_WsConnectionPoll(server, conn, event))
    {
      rtn = 1;

      break;
    }

    ci++;
  }

  for(ci = 0; ci < vector_size(_(server).connections); ci++)
  {
    conn = vector_at(_(server).connections, ci);

    if(_(conn).state == WS_STATE_DISCONNECTED)
    {
      _WsConnectionDestroy(conn);
      vector_erase(_(server).connections, ci, 1);
      ci--;
    }
  }

  _(server).nextToPoll++;

  if(_(server).nextToPoll >= vector_size(_(server).connections))
  {
    _(server).nextToPoll = 0;
  }

  return rtn;
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
  struct WsEvent ee = {0};
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
  *event = ee;

  if(_WsPollConnections(server, event))
  {
    return 1;
  }

  return 0;
}

void WsServerClose(ref(WsServer) server)
{
  size_t ci = 0;

  release(_(server).events.disconnect);
  vector_delete(_(_(server).events.message).data);
  release(_(server).events.message);
  vector_delete(_(_(_(server).events.http).response).data);
  release(_(_(server).events.http).response);
  sstream_delete(_(_(_(server).events.http).request).path);
  release(_(_(server).events.http).request);
  release(_(server).events.http);

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

void WsHttpResponseSetStatusCode(ref(WsHttpResponse) ctx, int status)
{
  _(ctx).status = status;
}

void WsHttpResponseWrite(ref(WsHttpResponse) ctx, char *data)
{
  size_t di = 0;
  size_t len = 0;

  if(_(ctx).headersSent != 0)
  {
    _WsPanic("Headers already sent");
  }

  len = strlen(data);

  for(di = 0; di < len; di++)
  {
    vector_push_back(_(ctx).data, data[di]);
  }
}

void WsHttpResponseSend(ref(WsHttpResponse) ctx)
{
  char header[512] = {0};
  int headerLen = 0;
  ref(WsConnection) connection = NULL;
  size_t ci = 0;

  if(_(ctx).headersSent != 0)
  {
    _WsPanic("Headers already sent");
  }

  _(ctx).headersSent = 1;

  /*
   * TODO: Is "OK" standard?
   * "HTTP/1.1 200 OK\r\n"
   */

  sprintf(header,
    "HTTP/1.1 %i\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: %i\r\n"
    "Connection: close\r\n"
    "\r\n", _(ctx).status, (int)vector_size(_(ctx).data));

  headerLen = strlen(header);
  connection = _(ctx).connection;

  for(ci = 0; ci < headerLen; ci++)
  {
    vector_push_back(_(connection).outgoing, header[ci]);
  }

  for(ci = 0; ci < vector_size(_(ctx).data); ci++)
  {
    vector_push_back(_(connection).outgoing, vector_at(_(ctx).data, ci));
  }

  _WsConnectionPollSend(connection);
}

ref(sstream) WsHttpRequestPath(ref(WsHttpRequest) ctx)
{
  return _(ctx).path;
}
