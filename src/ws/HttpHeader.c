#include "HttpHeader.h"

#include <stdio.h>

struct Key
{
  ref(sstream) name;
  ref(sstream) value;
};

struct HttpHeader
{
  ref(sstream) path;
  vector(ref(Key)) keys;
};

ref(HttpHeader) HttpHeaderCreate(vector(unsigned char) data)
{
  int found = 0;
  int marker = 0;
  size_t di = 0;
  size_t hi = 0;
  ref(HttpHeader) rtn = NULL;
  ref(sstream) ss = NULL;
  vector(ref(sstream)) hls = NULL;

  /*
   * Incoming stream not large enough to contain 4 byte header separator
   * consisting of "\r\n\r\n". Early out.
   */
  if(vector_size(data) < 4)
  {
    return NULL;
  }

  /*
   * Find the header separator (\r\n\r\n) and store the position of the
   * start of subsequent content.
   */
  for(di = 0; di < vector_size(data) - 3; di++)
  {
    if(vector_at(data, di) == '\r' &&
      vector_at(data, di + 1) == '\n' &&
      vector_at(data, di + 2) == '\r' &&
      vector_at(data, di + 3) == '\n')
    {
      found = 1;
      marker = di;

      break;
    }
  }

  /*
   * No valid header end sequence (\r\n\r\n) found. Early return.
   */
  if(found == 0)
  {
    return NULL;
  }

  /*
   * Convert into sstream for further processing
   */
  ss = sstream_new();

  for(di = 0; di < marker; di++)
  {
    sstream_append_char(ss, vector_at(data, di));
  }

  hls = vector_new(ref(sstream));
  sstream_split(ss, '\n', hls);
  sstream_delete(ss);

  /*
   * First request line is the url. Remove from list, process, store as path.
   */
  ss = vector_at(hls, 0);
  vector_erase(hls, 0, 1);

  for(di = 0; di < sstream_length(ss); di++)
  {
    if(sstream_at(ss, di) == ' ')
    {
      sstream_erase(ss, 0, di + 1);
      break;
    }
  }

  for(di = 0; di < sstream_length(ss); di++)
  {
    if(sstream_at(ss, di) == ' ')
    {
      sstream_erase(ss, di, sstream_length(ss) - di);
      break;
    }
  }

  rtn = allocate(HttpHeader);
  _(rtn).path = ss;

  /*
   * Go through the lines and add key, values to header structure.
   */
  for(hi = 0; hi < vector_size(hls); hi++)
  {
    ss = vector_at(hls, hi);
    printf("Line: %s\n", sstream_cstr(ss));
  }

  /*
   * Clean up intermediate processing.
   */
  for(hi = 0; hi < vector_size(hls); hi++)
  {
    sstream_delete(vector_at(hls, hi));
  }

  vector_delete(hls);

  /*
   * Move marker forward to specify the position of the first byte of content.
   * so this is after the 4 "\r\n\r\n" bytes.
   */
  marker += 4;

  /* TODO: Strip until content */

  return rtn;
}

void HttpHeaderDestroy(ref(HttpHeader) ctx)
{
  sstream_delete(_(ctx).path);
  release(ctx);
}

int HttpHeaderContains(ref(HttpHeader) ctx, const char *key)
{
  return 0;
}

ref(sstream) HttpHeaderPath(ref(HttpHeader) ctx)
{
  return _(ctx).path;
}

vector(unsigned char) HttpHeaderSerialize(ref(HttpHeader) ctx)
{
  return NULL;
}
