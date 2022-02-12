#include "sock.h"

void handle(pid_t pid)
{
  printf("被动退出\n");
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
  int listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listenfd < 0)
    ERR_EXIT("socket");

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(8888);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 本机的任意IPv4

  // TCP主动关闭放会进入 TIME_WAIT 状态，设置 SO_REUSEADDR 选项可以避免等待
  int on = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(1)) < -1)
    ERR_EXIT("setsockopt");

  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    ERR_EXIT("bind");

  if (listen(listenfd, SOMAXCONN) < 0)
    ERR_EXIT("listen");

  struct sockaddr_in connaddr;
  socklen_t connlen = sizeof(connaddr);
  int connfd;
  int pid;

  printf("等待另一端链接...\n");
  if ((connfd = accept(listenfd, (struct sockaddr *)&connaddr, &connlen)) < 0)
    ERR_EXIT("accept");
  char *connip = inet_ntoa(connaddr.sin_addr);
  printf("[[CONNECT]] %s:%d\n", connip, connaddr.sin_port); // 客户 ip:port

  pid = fork();

  if (pid < 0)
    ERR_EXIT("fork");

  if (pid == 0)
  {
    close(listenfd); // 引用 -1
    char rbuf[1024];
    int retn;

    // 一直阻塞等待获取客户发送的消息
    while (1)
    {
      retn = recv(connfd, rbuf, sizeof(rbuf), 0);
      if (retn == 0) // 客户主动关闭
        break;
      if (retn < 0)
        ERR_EXIT("recv");
      rbuf[retn] = 0;
      printf("[接收]: %s", rbuf);
    }
    printf("客户挂断\n");

    // 客户关闭后，退出子进程，但是父进程还阻塞在 fgets 需要发送信号通知父进程
    kill(getppid(), SIGUSR1); // 发送信号给父进程
    exit(EXIT_SUCCESS);
  }

  signal(SIGUSR1, handle);
  char wbuf[1024];
  while (fgets(wbuf, sizeof(wbuf), stdin) != NULL)
  {
    printf("[发送]: %s", wbuf);
    write(connfd, wbuf, strlen(wbuf));
    memset(wbuf, 0, sizeof(wbuf));
  }

  close(connfd);
  close(listenfd);
  return 0;
}
