#include "HttpHeader.h"
#include "Util.h"

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
  ref(Key) key = NULL;

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
  sstream_split_eol(ss, hls);
  sstream_delete(ss);

  /*
   * Early return if header does not contain at least one line
   */
  if(vector_size(hls) < 1)
  {
    vector_delete(hls);

    return NULL;
  }

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
  _(rtn).keys = vector_new(ref(Key));

  /*
   * Go through the lines and add key, values to header structure.
   */
  for(hi = 0; hi < vector_size(hls); hi++)
  {
    ss = vector_at(hls, hi);

    key = allocate(Key);
    _(key).name = sstream_new();
    _(key).value = sstream_new();

    /*
     * Store up to ':' in key name.
     */
    for(di = 0; di < sstream_length(ss); di++)
    {
      if(sstream_at(ss, di) == ':')
      {
        break;
      }
      else
      {
        sstream_append_char(_(key).name, sstream_at(ss, di));
      }
    }

    /*
     * Ignore space between : and value.
     */
    di++;

    for(; di < sstream_length(ss); di++)
    {
      if(sstream_at(ss, di) != ' ') break;
    }

    for(; di < sstream_length(ss); di++)
    {
      sstream_append_char(_(key).value, sstream_at(ss, di));
    }

    vector_push_back(_(rtn).keys, key);
/*
    printf("Line: [%i] [%s]\n", (int)sstream_length(_(key).value), sstream_cstr(_(key).value));
    printf("Line: %s\n", sstream_cstr(ss));
*/
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

  /*
   * Strip header from the stream.
   */
  vector_erase(data, 0, marker);

  return rtn;
}

void HttpHeaderDestroy(ref(HttpHeader) ctx)
{
  size_t ki = 0;

  for(ki = 0; ki < vector_size(_(ctx).keys); ki++)
  {
    sstream_delete(_(vector_at(_(ctx).keys, ki)).name);
    sstream_delete(_(vector_at(_(ctx).keys, ki)).value);
    release(vector_at(_(ctx).keys, ki));
  }

  vector_delete(_(ctx).keys);
  sstream_delete(_(ctx).path);
  release(ctx);
}

int HttpHeaderContains(ref(HttpHeader) ctx, const char *key)
{
  size_t ki = 0;

  for(ki = 0; ki < vector_size(_(ctx).keys); ki++)
  {
    if(strcmp(key, sstream_cstr(_(vector_at(_(ctx).keys, ki)).name)) == 0)
    {
      return 1;
    }
  }

  return 0;
}

ref(sstream) HttpHeaderGetCStr(ref(HttpHeader) ctx, const char *key)
{
  size_t ki = 0;

  for(ki = 0; ki < vector_size(_(ctx).keys); ki++)
  {
    if(strcmp(key, sstream_cstr(_(vector_at(_(ctx).keys, ki)).name)) == 0)
    {
      return _(vector_at(_(ctx).keys, ki)).value;
    }
  }

  _WsPanic("Requested header variable does not exist");
  return NULL;
}

ref(sstream) HttpHeaderPath(ref(HttpHeader) ctx)
{
  return _(ctx).path;
}

vector(unsigned char) HttpHeaderSerialize(ref(HttpHeader) ctx)
{
  return NULL;
}
