#include "sha1.h"
#include "base64.h"
#include "WebSocket.h"
#include "HttpHeader.h"

#include "stent.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define WS_KEY_LEN 24
#define WS_MS_LEN 36
#define WS_KEYMS_LEN (WS_KEY_LEN + WS_MS_LEN)
#define MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_HS_REQ "Sec-WebSocket-Key"
#define WS_HS_ACCLEN   130

#define WS_HS_ACCEPT                   \
"HTTP/1.1 101 Switching Protocols\r\n" \
"Upgrade: websocket\r\n"               \
"Connection: Upgrade\r\n"              \
"Sec-WebSocket-Accept: "

unsigned char *_WsHandshakeAccept(char *wsKey)
{
  unsigned char *rtn = NULL;
  SHA1Context ctx;
  char *str = malloc(sizeof(char) * (WS_KEY_LEN + WS_MS_LEN + 1));
  unsigned char hash[SHA1HashSize];

  strcpy(str, wsKey);
  strcat(str, MAGIC_STRING);

  SHA1Reset(&ctx);
  SHA1Input(&ctx, (const uint8_t *)str, WS_KEYMS_LEN);
  SHA1Result(&ctx, hash);

  rtn = base64_encode(hash, SHA1HashSize, NULL);

  /* TODO: The simple version does not add a '\n' on the end. */
  *(rtn + strlen((const char *)rtn) - 1) = '\0';

  free(str);

  return rtn;
}

vector(unsigned char) _WsHandshakeResponse(ref(HttpHeader) header)
{
  unsigned char *accept = NULL;
  vector(unsigned char) rtn = NULL;
  ref(sstream) key = NULL;

  if(!HttpHeaderContains(header, "Sec-WebSocket-Key"))
  {
    return NULL;
  }

  key = HttpHeaderGetCStr(header, "Sec-WebSocket-Key");

/*
  printf("Key: [%i][%s]\n", (int)sstream_length(key), sstream_cstr(key));
*/

  accept = _WsHandshakeAccept(sstream_cstr(key));

  rtn = vector_new(unsigned char);
  vector_resize(rtn, WS_HS_ACCLEN);
  strcpy(&vector_at(rtn, 0), WS_HS_ACCEPT);
  strcat(&vector_at(rtn, 0), (const char *)accept);
  strcat(&vector_at(rtn, 0), "\r\n\r\n");

  free(accept);

  return rtn;
}

uint64_t f_uint64(uint8_t *value)
{
  int i = 0;
  uint64_t length = 0;

  for(i = 0; i < 8; i++)
  {
    length = (length << 8) | value[i];
  }

  return length;
}

uint16_t f_uint16(uint8_t *value)
{
  int i = 0;
  uint16_t length = 0;

  for(i = 0; i < 2; i++)
  {
    length = (length << 8) | value[i];
  }

  return length;
}

/****************************************************************************
 * _WsDecodeFrame
 *
 * If first byte suggests a text frame, obtain the frame length to decide
 * if enough bytes have been received for a complete message. If so add them
 * to the decoded buffer whilst unmasking them and return it.
 *
 * [ final           ] 1
 * [ reserved1       ] 1
 * [ reserved2       ] 1
 * [ reserved3       ] 1
 * [ opcode          ] 4
 * [                 ]
 * [                 ]
 * [                 ]
 * [ mask            ] 1
 * [ payload_len     ] 7
 * [                 ]
 * [                 ]
 * [                 ]
 * [                 ]
 * [                 ]
 * [                 ]
 * < extended_len    > 16 (if payload len == 126)
 * <                 > 64 (if payload_len == 127)
 * [ mask 1          ] 4
 * [                 ]
 * [                 ]
 * [                 ]
 * [ mask 2          ] 4
 * [                 ]
 * [                 ]
 * [                 ]
 * [ mask 3          ] 4
 * [                 ]
 * [                 ]
 * [                 ]
 * [ mask 4          ] 4
 * [                 ]
 * [                 ]
 * [                 ]
 * < data            > payload_len or extended_len
 *
 * incoming - The buffer containing all currently received data.
 * fi       - Out variable to populate with information about the frame.
 * data     - Decoded message
 *
 * Returns 1 if the incoming data contained a valid frame.
 *
 ****************************************************************************/
int _WsDecodeFrame(vector(unsigned char) incoming,
  struct WsFrameInfo *fi, vector(unsigned char) data)
{
  uint8_t maskStart;      /* Index where mask starts */
  size_t payloadLength;   /* Data length             */
  uint8_t masks[4];       /* Masking key             */
  size_t i,j;             /* Loop indexes            */
  uint8_t *uframe = NULL; /* Casted bytes            */
  uint8_t first = 0;
  uint8_t second = 0;

  /*
   * Early return, no data to process or buffer not yet large
   * enough for mask and payload info.
   */
  if(vector_size(incoming) < 2)
  {
    return 0;
  }

  /*
   * Store first and second byte for analysis.
   */
  first = vector_at(incoming, 0);
  second = vector_at(incoming, 1);

  uframe = (uint8_t *)&vector_at(incoming, 0);

  if((first & 128 /* 10000000 */) != 0)
  {
    fi->final = 1;
  }

  if((second & 128 /* 10000000 */) != 0)
  {
    fi->masked = 1;
  }

  if((first & 1 /* 00000001 */) != 0)
  {
    fi->opcode = WS_FRAME_TEXT;

    /*
     * Obtain all but the first bit to contain the length.
     */
    payloadLength = second & 0x7F;
    maskStart = 2;

    if(payloadLength == 126)
    {
      payloadLength = f_uint16(&uframe[2]);
      maskStart = 4;
    }
    else if(payloadLength == 127)
    {
      payloadLength = f_uint64(&uframe[2]);
      maskStart = 10;
    }

    fi->dataStart = maskStart + 4;
    fi->dataEnd = fi->dataStart + payloadLength;

    /*
     * Header reports longer payload than buffer. Wait for
     * more bytes to be received.
     */
    if(fi->dataEnd > vector_size(incoming))
    {
      return 0;
    }

    masks[0] = vector_at(incoming, maskStart);
    masks[1] = vector_at(incoming, maskStart + 1);
    masks[2] = vector_at(incoming, maskStart + 2);
    masks[3] = vector_at(incoming, maskStart + 3);

    vector_resize(data, payloadLength);

    for(i = fi->dataStart, j = 0; i < fi->dataEnd; i++, j++)
    {
      vector_at(data, j) = vector_at(incoming, i) ^ masks[j % 4];
    }
  }
  else if((first & 8 /* 00001000 */) != 0)
  {
    fi->opcode = WS_FRAME_CLOSE;
  }
  else
  {
    fi->opcode = WS_FRAME_UNKNOWN;
    printf("Unknown frame opcode\n");
  }

  return 1;
}

