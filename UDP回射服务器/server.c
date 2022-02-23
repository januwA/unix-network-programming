#include "sock.h"

int main(int argc, char **argv)
{
  signal(SIGPIPE, SIG_IGN); // 客户主动关闭后，再会设置会收到 SIGPIPE 信号
  int listenfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (listenfd < 0)
    ERR_EXIT("socket");

  // 将socket绑定到一个地址
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(8888);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 本机的任意IPv4
  // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 旧的方法，被 inet_aton 取代
  // inet_aton("127.0.0.1", &servaddr.sin_addr);

  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    ERR_EXIT("bind");

  struct sockaddr_in connaddr;
  socklen_t connlen = sizeof(connaddr);
  char rbuf[1024];
  int n;
  while (1)
  {
    memset(rbuf, 0, sizeof(rbuf));

    n = recvfrom(listenfd, &rbuf, sizeof(rbuf), 0, (struct sockaddr *)&connaddr, &connlen);

    if (n < 0)
      ERR_EXIT("recvfrom");

    char *connip = inet_ntoa(connaddr.sin_addr);
    rbuf[n] = 0;
    printf("[[MSG]] %s:%d: %s", connip, connaddr.sin_port, rbuf);
    sendto(listenfd, rbuf, n, 0, (struct sockaddr *)&connaddr, connlen);
  }

  return 0;
}
