#include "sha1.h"
#include "base64.h"
#include "WebSocket.h"

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

vector(unsigned char) _WsHandshakeResponse(vector(unsigned char) request)
{
  char *s = NULL;
  unsigned char *accept = NULL;
  vector(unsigned char) rtn = NULL;
  char *req = NULL;
  size_t di = 0;
  int marker = -1;

  /*
   * Incoming stream not large enough to contain 4 byte header separator
   * consisting of "\r\n\r\n". Early out.
   */
  if(vector_size(request) < 4)
  {
    return NULL;
  }

  /*
   * Find the header separator (\r\n\r\n) and store the position of the
   * start of subsequent content.
   */
  for(di = 0; di < vector_size(request) - 3; di++)
  {
    if(vector_at(request, di) == '\r' &&
      vector_at(request, di + 1) == '\n' &&
      vector_at(request, di + 2) == '\r' &&
      vector_at(request, di + 3) == '\n')
    {
      marker = di;

      break;
    }
  }

  /*
   * The header separator was not found. Early exit.
   */
  if(marker == -1)
  {
    return NULL;
  }

  req = &vector_at(request, 0);

  for(s = strtok(req, "\r\n"); s != NULL; s = strtok(NULL, "\r\n"))
  {
    if(strstr(s, WS_HS_REQ) != NULL)
    {
      break;
    }
  }

  s = strtok(s,    " ");
  s = strtok(NULL, " ");

  if(!s)
  {
    return NULL;
  }
	
  accept = _WsHandshakeAccept(s);

  rtn = vector_new(unsigned char);
  vector_resize(rtn, WS_HS_ACCLEN);
  //rtn = malloc(sizeof(char) * WS_HS_ACCLEN);
  strcpy(&vector_at(rtn, 0), WS_HS_ACCEPT);
  strcat(&vector_at(rtn, 0), (const char *)accept);
  strcat(&vector_at(rtn, 0), "\r\n\r\n");

  free(accept);

  vector_erase(request, 0, marker + 4);

/*
  {
    size_t ci = 0;

    for(ci = 0; ci < vector_size(request); ci++)
    {
      printf("%c", vector_at(request, ci));
    }

    printf("\n");
  }
*/

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
 * [ type            ]
 * [ length          ]
 * < extended length >
 * <                 >
 * < extended length >
 * <                 >
 * <                 >
 * <                 >
 * [ mask 1          ]
 * [ mask 2          ]
 * [ mask 3          ]
 * [ mask 4          ]
 * < data            >
 *
 * frame     - The current buffer containing a potentially complete frame.
 * length    - The lengh of data (to avoid strlen).
 * decodeLen - Output the contained frames length (to avoid strlen).
 * end       - Output the position of the first byte of the next message.
 *
 * Returns the decoded frames message.
 *
 ****************************************************************************/
char *_WsDecodeFrame(vector(unsigned char) incoming,
  struct WsFrameInfo *fi)
{
  char *msg = NULL;       /* Decoded message         */
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
    return NULL;
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

    if(fi->dataEnd > vector_size(incoming))
    {
      return NULL;
    }

    masks[0] = vector_at(incoming, maskStart);
    masks[1] = vector_at(incoming, maskStart + 1);
    masks[2] = vector_at(incoming, maskStart + 2);
    masks[3] = vector_at(incoming, maskStart + 3);

    msg = malloc(sizeof(unsigned char) * (payloadLength + 1));

    for(i = fi->dataStart, j = 0; i < fi->dataEnd; i++, j++)
    {
      msg[j] = vector_at(incoming, i) ^ masks[j % 4];
    }

    msg[j] = '\0';
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

  return msg;
}

