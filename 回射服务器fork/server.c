#include "sock.h"

int main(int argc, char **argv)
{
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
  int pid;
  while (1)
  {
    // 从已完成队列中返回第一个连接，如果没有则阻塞等待
    if ((connfd = accept(listenfd, (struct sockaddr *)&connaddr, &connlen)) < 0)
      ERR_EXIT("accept");
    char *connip = inet_ntoa(connaddr.sin_addr);
    printf("[[CONNECT]] %s:%d\n", connip, connaddr.sin_port); // 客户 ip:port

    // 获取到客户后，分配一个子进程来处理这个客户
    pid = fork();

    switch (pid)
    {
    case -1:
      ERR_EXIT("fork");
      break;
    case 0: // 在子进程中返回0
    {
      close(listenfd); // 引用 -1

      char readbuf[1024];
      int retn;

      // 一直阻塞等待获取客户发送的消息
      while (1)
      {
        retn = recv(connfd, readbuf, sizeof(readbuf), 0);
        if (retn == 0) // 客户主动关闭
          break;
        if (retn < 0)
          ERR_EXIT("recv");
        readbuf[retn] = 0;
        // fputs(readbuf, stdout);
        printf("[[MSG]] %s:%d: %s", connip, connaddr.sin_port, readbuf);
        write(connfd, readbuf, retn);
      }

      printf("[[CLSOE]] %s:%d\n", connip, connaddr.sin_port);
      // 客户关闭，关闭子进程
      exit(EXIT_SUCCESS);
    }
    break;

    default:         // 父进程中返回子进程的pid
      close(connfd); // 引用 -1
      break;
    }
  }

  return 0;
}
