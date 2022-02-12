#include "sock.h"

int main(int argc, char **argv)
{
  int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0)
    ERR_EXIT("socket");

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(8888);
  inet_aton("127.0.0.1", &servaddr.sin_addr);

  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    ERR_EXIT("connect");

  char sendbuf[1024];
  char readbuf[1024];

  while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
  {
    write(sockfd, sendbuf, strlen(sendbuf));

    int retn = read(sockfd, readbuf, sizeof(readbuf));
    if (retn < 0)
    {
      printf("server close\n");
      break;
    }

    readbuf[retn] = 0;
    fputs(readbuf, stdout);
    memset(sendbuf, 0, sizeof(sendbuf));
  }

  return 0;
}
