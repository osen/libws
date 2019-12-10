#ifndef WS_TCPSOCKET_H
#define WS_TCPSOCKET_H

#include "stent.h"

struct WsTcpSocket;

ref(WsTcpSocket) WsTcpListen(int port);
void WsTcpSocketClose(ref(WsTcpSocket) ctx);
int WsTcpSocketConnected(ref(WsTcpSocket) ctx);

void WsTcpSocketSend(ref(WsTcpSocket) ctx,
  vector(unsigned char) data);

void WsTcpSocketRecv(ref(WsTcpSocket) ctx,
  vector(unsigned char) data);

int WsTcpSocketsReady(vector(ref(WsTcpSocket)) reads,
  vector(ref(WsTcpSocket)) writes, int timeout);

int WsTcpSocketReady(ref(WsTcpSocket) ctx);
ref(WsTcpSocket) WsTcpSocketAccept(ref(WsTcpSocket) ctx);

void _WsPanic(char *message);

#endif
