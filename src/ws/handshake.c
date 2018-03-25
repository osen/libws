#include "sha1.h"
#include "base64.h"

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

#define WS_FIN 128
#define WS_FR_OP_TXT  1
#define WS_FR_OP_CLSE 8
#define WS_FR_OP_UNSUPPORTED 0xF

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
  *(rtn + strlen((const char *)rtn) - 1) = '\0';
  free(str);

  return rtn;
}

char *_WsHandshakeResponse(char *request)
{
  char *s;
  unsigned char *accept;
  char *rtn = NULL;

  for(s = strtok(request, "\r\n"); s != NULL; s = strtok(NULL, "\r\n"))
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

  rtn = malloc(sizeof(char) * WS_HS_ACCLEN);
  strcpy(rtn, WS_HS_ACCEPT);
  strcat(rtn, (const char *)accept);
  strcat(rtn, "\r\n\r\n");

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
 * type      - Output what the frame represented.
 * decodeLen - Output the contained frames length (to avoid strlen).
 * end       - Output the position of the first byte of the next message.
 *
 * Returns the decoded frames message.
 *
 ****************************************************************************/
char *_WsDecodeFrame(char *frame, size_t length, int *type, size_t *decodeLen,
  size_t *end)
{
  char *msg;              /* Decoded message.   */
  uint8_t mask;           /* Payload is masked? */
  uint8_t flength;        /* Raw length.        */
  uint8_t idx_first_mask; /* Index masking key. */
  uint8_t idx_first_data; /* Index data.        */
  size_t data_length;     /* Data length.       */
  uint8_t masks[4];       /* Masking key.       */
  size_t i,j;             /* Loop indexes.      */
  unsigned char *uframe;  /* Casted bytes       */

  uframe = (unsigned char *)frame;
  msg = NULL;

  if(length < 1) return NULL;

  if(uframe[0] == (WS_FIN | WS_FR_OP_TXT))
  {
    *type = WS_FR_OP_TXT;
    idx_first_mask = 2;
    if(length < 2) return NULL;
    mask = uframe[1];
    flength = mask & 0x7F;

    if(flength == 126)
    {
      idx_first_mask = 4;
      flength = f_uint16(&frame[2]);
    }
    else if(flength == 127)
    {
      idx_first_mask = 10;
      flength = f_uint64(&frame[2]);
    }

    data_length = flength;
    *decodeLen = data_length;
    idx_first_data = idx_first_mask + 4;
    *end = idx_first_data + data_length;

    if(*end > length)
    {
      return NULL;
    }

    masks[0] = uframe[idx_first_mask];
    masks[1] = uframe[idx_first_mask + 1];
    masks[2] = uframe[idx_first_mask + 2];
    masks[3] = uframe[idx_first_mask + 3];

    msg = malloc(sizeof(unsigned char) * (data_length + 1));

    for(i = idx_first_data, j = 0; i < *end; i++, j++)
    {
      msg[j] = uframe[i] ^ masks[j % 4];
    }

    msg[j] = '\0';
  }
  else if(uframe[0] == (WS_FIN | WS_FR_OP_CLSE) )
  {
    *type = WS_FR_OP_CLSE;
  }
  else
  {
    *type = uframe[0] & 0x0F;
  }

  return msg;
}

