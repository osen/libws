#include "stent.h"

#define WS_FRAME_CLOSE 1
#define WS_FRAME_TEXT 2
#define WS_FRAME_UNKNOWN 3

struct HttpHeader;

struct WsFrameInfo
{
  int masked;
  int final;
  int opcode;
  size_t dataStart;
  size_t dataEnd;
};

char *_WsDecodeFrame(vector(unsigned char) incoming,
  struct WsFrameInfo *fi);

vector(unsigned char) _WsHandshakeResponse(ref(HttpHeader) header);
