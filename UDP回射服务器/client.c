#include "sock.h"

int main(int argc, char **argv)
{
  int sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0)
    ERR_EXIT("socket");

  struct sockaddr_in servaddr;
  socklen_t addrlen = sizeof(servaddr);

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(8888);
  inet_aton("127.0.0.1", &servaddr.sin_addr);

  // UDP 也能调用connect，但是之后这个UDP只能向这个 服务器发送消息
  // connect(sockfd, (struct sockaddr*)&servaddr, addrlen);

  char wbuf[1024];
  char rbuf[1024];
  memset(wbuf, 0, sizeof(wbuf));
  memset(rbuf, 0, sizeof(rbuf));

  while (fgets(wbuf, sizeof(wbuf), stdin) != NULL)
  {
    sendto(sockfd, wbuf, strlen(wbuf), 0, (struct sockaddr *)&servaddr, addrlen);

    recvfrom(sockfd, rbuf, sizeof(rbuf), 0, NULL, NULL);
    fputs(rbuf, stdout);

    memset(wbuf, 0, sizeof(wbuf));
    memset(rbuf, 0, sizeof(rbuf));
  }
  return 0;
}
