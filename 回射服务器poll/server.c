#include "sock.h"

#define POLL_SETSIZE 2048

int main(int argc, char **argv)
{
  signal(SIGPIPE, SIG_IGN); // 客户主动关闭后，再会设置会收到 SIGPIPE 信号
  int listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listenfd < 0)
    ERR_EXIT("socket");

  // 将socket绑定到一个地址
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(8888);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 本机的任意IPv4

  // TCP主动关闭放会进入 TIME_WAIT 状态，设置 SO_REUSEADDR 选项可以避免等待
  int on = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(1)) < -1)
    ERR_EXIT("setsockopt");

  // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 旧的方法，被inet_aton取代
  // inet_aton("127.0.0.1", &servaddr.sin_addr);
  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    ERR_EXIT("bind");

  // 监听套接字连接并限制传入连接的队列
  // 内核会为这个套接字维护两个队列: 1、客户发送到服务器，服务器正在等待TCP三路握手过程(SYN_RCVD状态) 2、已完成连接的队列(ESTABLISHED状态)
  // 两队列之和不能超过 backlog参数
  if (listen(listenfd, SOMAXCONN) < 0)
    ERR_EXIT("listen");

  struct sockaddr_in connaddr;
  socklen_t connlen = sizeof(connaddr);
  int connfd;
  int nready;
  int maxfd = listenfd;
  struct pollfd client[POLL_SETSIZE];       // 所有已连接客户端
  for (size_t i = 0; i < POLL_SETSIZE; i++) // 全部初始化为 -1
    client[i].fd = -1;

  client[0].fd = listenfd;
  client[0].events = POLLIN; // 关心可读事件
  int savei = 0;
  struct packet rp;
  while (1)
  {
    // timeout 设置为-1，直到有事件才返回
    nready = poll(client, savei + 1, -1);
    if (nready < 0)
    {
      if (errno == EINTR)
        continue;
      ERR_EXIT("poll");
    }

    // 如果设置了超时时间 超时返回0
    if (nready == 0)
      continue;

    // 监听套接口可读了
    if (client[0].revents & POLLIN)
    {
      // 这里的 accept 将不再阻塞
      if ((connfd = accept(listenfd, (struct sockaddr *)&connaddr, &connlen)) < 0)
        ERR_EXIT("accept");

      for (savei = 0; savei < POLL_SETSIZE; savei++)
      {
        // 保存到第一个空闲的位置
        if (client[savei].fd < 0)
        {
          client[savei].fd = connfd;
          client[savei].events = POLLIN;
          break;
        }
      }

      // 未找到储存的位置
      if (savei == POLL_SETSIZE)
        ERR_EXIT("POLL_SETSIZE");

      char *connip = inet_ntoa(connaddr.sin_addr);
      printf("[[CONNECT]] %s:%d\n", connip, connaddr.sin_port); // 客户 ip:port

      // printf("nready: %d\n", nready);
      // 处理完后，可在执行后面的代码
      if (--nready <= 0)
        continue;
    }

    for (size_t i = 1; i < POLL_SETSIZE; i++)
    {
      if ((connfd = client[i].fd) == -1)
        continue;

      getpeername(connfd, (struct sockaddr *)&connaddr, &connlen);
      char *connip = inet_ntoa(connaddr.sin_addr);
      // 客户可读
      if (client[i].revents & POLLIN)
      {
        // 读取时不在阻塞
        int n = readn(connfd, &rp.len, sizeof(rp.len)); // 先读取buf长度
        // printf("n: %d\n", n);
        if (n < 0)
          ERR_EXIT("readn");

        if (n == 0)
        {
          printf("[[CLSOE]] %s:%d\n", connip, connaddr.sin_port);
          close(connfd);
          client[i].fd = -1;
          break;
        }

        if (n != sizeof(rp.len))
        {
          printf("读取包头 len 错误\n");
          close(connfd);
          client[i].fd = -1;
          break;
        }

        int buflen = ntohl(rp.len);
        // printf("buflen: %d\n", buflen);
        int bufn = readn(connfd, &rp.buf, buflen); // 读取buf数据
        // printf("bufn: %d\n", bufn);
        if (bufn < 0)
          ERR_EXIT("readn");

        if (bufn == 0)
        {
          printf("[[CLSOE]] %s:%d\n", connip, connaddr.sin_port);
          close(connfd);
          client[i].fd = -1;
          break;
        }

        if (bufn != buflen)
        {
          printf("读取包体错误，因读(%d),实际(%d)\n", buflen, bufn);
          close(connfd);
          client[i].fd = -1;
          break;
        }

        printf("[[MSG]] %s:%d: %s", connip, connaddr.sin_port, rp.buf);
        // sleep(5); // 模拟延迟回复
        writen(connfd, &rp, sizeof(rp.len) + bufn);
        memset(&rp, 0, sizeof(rp));

        if (--nready <= 0)
          break;
      }
    }
  }

  return 0;
}
