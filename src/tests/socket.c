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
  vector(unsigned char) data = NULL;

  data = vector_new(unsigned char);

  while(1)
  {
    if(WsTcpSocketReady(client))
    {
      printf("Client ready\n");

      if(!WsTcpSocketConnected(client))
      {
        printf("Client disconnected\n");
        break;
      }

      printf("Data received\n");
      vector_resize(data, 128);
      WsTcpSocketRecv(client, data);
      printf("Received: %i\n", (int)vector_size(data));
      WsTcpSocketSend(client, data);
    }

    wait();
  }

  WsTcpSocketClose(client);
  vector_delete(data);
}

int main()
{
  ref(WsTcpSocket) server = NULL;
  ref(WsTcpSocket) client = NULL;
  int count = 0;

  server = WsTcpListen(1987);

  while(1)
  {
    if(WsTcpSocketReady(server))
    {
      printf("Client ready\n");
      client = WsTcpSocketAccept(server);
      printf("Client accepted\n");
      handle_client(client);
      count++;

      if(count >= 3) break;
    }

    wait();
  }

  WsTcpSocketClose(server);

  return 0;
}
