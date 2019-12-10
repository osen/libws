#include <stent.h>

struct HttpHeader;

ref(HttpHeader) HttpHeaderCreate(vector(unsigned char) data);
void HttpHeaderDestroy(ref(HttpHeader) ctx);

int HttpHeaderContains(ref(HttpHeader) ctx, const char *key);
ref(sstream) HttpHeaderPath(ref(HttpHeader) ctx);
vector(unsigned char) HttpHeaderSerialize(ref(HttpHeader) ctx);
