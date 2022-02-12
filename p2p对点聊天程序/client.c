#include "sock.h"

void handle(pid_t pid)
{
  printf("被动退出\n");
  exit(EXIT_SUCCESS);
}

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

  pid_t pid;

  pid = fork();
  switch (pid)
  {
  case -1:
    ERR_EXIT("fork");
    break;

  case 0:
  {
    // 子线程获取消息
    char rbuf[1024];
    while (1)
    {
      int retn = recv(sockfd, rbuf, sizeof(rbuf), 0);
      if (retn == 0) // 服务器主动关闭
        break;
      if (retn < 0)
        ERR_EXIT("recv");
      rbuf[retn] = 0;
      printf("[接收]: %s", rbuf);
    }

    printf("服务器挂断\n");

    // 服务器关闭后，退出子进程，但是父进程还阻塞在 fgets 需要发送信号通知父进程
    kill(getppid(), SIGUSR1); // 发送信号给父进程

    close(sockfd);
    exit(EXIT_SUCCESS);
    break;
  }
  default:
  {
    signal(SIGUSR1, handle);
    // 主线程发送消息
    char wbuf[1024];
    while (fgets(wbuf, sizeof(wbuf), stdin) != NULL)
    {
      printf("[发送]: %s", wbuf);
      write(sockfd, wbuf, strlen(wbuf));
      memset(wbuf, 0, sizeof(wbuf));
    }
    break;
  }
  }

  close(sockfd);
  return 0;
}
