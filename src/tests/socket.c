#include <ws/TcpSocket.h>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

#include <stdio.h>

void wait()
{
#ifdef _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif
}

void handle_client(ref(WsTcpSocket) client)
{
  vector(unsigned char) incoming = NULL;
  vector(unsigned char) outgoing = NULL;

  incoming = vector_new(unsigned char);
  outgoing = vector_new(unsigned char);

  while(WsTcpSocketConnected(client))
  {
    vector_resize(incoming, 128);
    WsTcpSocketRecv(client, incoming);

    if(vector_size(incoming) > 0)
    {
      printf("Data received\n");
      vector_insert(outgoing, vector_size(outgoing), incoming, 0, vector_size(incoming));
      vector_push_back(incoming, '\0');
      printf("Received [%i]: [%s]\n", (int)vector_size(incoming), (char *)&vector_at(incoming, 0));
    }

    if(vector_size(outgoing) > 0)
    {
      printf("Sending [%i]\n", (int)vector_size(outgoing));
      WsTcpSocketSend(client, outgoing);
    }

    wait();
  }

  printf("Client disconnected\n");
  WsTcpSocketClose(client);
  vector_delete(incoming);
  vector_delete(outgoing);
}

int main()
{
  ref(WsTcpSocket) server = NULL;
  ref(WsTcpSocket) client = NULL;
  int count = 0;

  server = WsTcpListen(1987);

  while(1)
  {
    client = WsTcpSocketAccept(server);

    if(client)
    {
      printf("Client accepted\n");
      handle_client(client);
      count++;
    }

    if(count >= 3) break;

    wait();
  }

  WsTcpSocketClose(server);

  return 0;
}
