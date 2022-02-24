#include "sock.h"

char username[16];
USER_LIST clint_list{0};

void parse_cmd(char *cmdline, int sockfd, struct sockaddr_in *servaddr)
{
  char cmd[0];

  char *p;
  p = strchr(cmdline, ' '); // 在cmdline找空格开始的位置
  if (p != NULL)
    *p = '\0';

  strcmp(cmd, cmdline);

  if (strcmp(cmd, "exit") == 0)
  {
    MESSAGE msg;
    ZERO_MSG(msg);
    msg.cmd = htonl(C2S_LOGOUT);
    strcpy(msg.body, username);

    if (sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)servaddr, sizeof(struct sockaddr)) < 0)
      ERR_EXIT("sendto");

    exit(EXIT_SUCCESS);
  } else if(strcmp(cmd, "send") == 0) 
  {

  }
}
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

  MESSAGE msg;

  // 先登录
  while (1)
  {
    memset(username, 0, sizeof(username));
    ZERO_MSG(msg);

    printf("请输入用户名登录: ");
    fflush(stdout);
    scanf("%s", username);

    msg.cmd = htonl(C2S_LOGIN);
    strcpy(msg.body, username);
    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&servaddr, addrlen);

    ZERO_MSG(msg);
    recvfrom(sockfd, &msg, sizeof(msg), 0, NULL, NULL);
    int cmd = ntohl(msg.cmd);
    if (cmd == S2C_ALREADY_LOGINED)
      printf("此用户已经登陆\n");
    else if (cmd == S2C_LOGIN_OK)
    {
      printf("成功登陆\n");
      break;
    }
  }

  // 获取在线人数
  int count;
  recvfrom(sockfd, &count, sizeof(int), 0, NULL, NULL);

  int n = ntohl(count);
  printf("当前在线人数：%d人\n", n);

  // 获取当前在线客户信息
  for (size_t i = 0; i < n; i++)
  {
    USERINFO user;
    recvfrom(sockfd, &user, sizeof(USERINFO), 0, NULL, NULL);
    clint_list.push_back(user);

    in_addr tmp;
    tmp.s_addr = user.ip;
    printf("%d %s <-> %s:%d\n", i, user.username, inet_ntoa(tmp), ntohs(user.port));
  }

  // 输出提示信息
  printf("\n可用命令行:\n");
  printf("send <username> <msg>\n");
  printf("list\n");
  printf("exit\n\n");

  fd_set rset;
  FD_ZERO(&rset);
  int nready;
  struct sockaddr_in peeraddr;
  socklen_t peeraddrlen = sizeof(peeraddr);
  while (1)
  {
    FD_SET(STDIN_FILENO, &rset);
    FD_SET(sockfd, &rset);

    nready = select(sockfd + 1, &rset, NULL, NULL, NULL);

    if (nready < 0)
      ERR_EXIT("selct");

    if (nready == 0)
      continue;

    // 读取sock消息
    if (FD_ISSET(sockfd, &rset))
    {
      ZERO_MSG(msg);
      recvfrom(sockfd, &msg, sizeof(MESSAGE), 0, (struct sockaddr *)&peeraddr, &peeraddrlen);
      int cmd = ntohl(msg.cmd);
      switch (cmd)
      {
      case S2C_SOMEONE_LOGIN:
        do_someone_login(msg);
        break;

      case S2C_SOMEONE_LOGOUT:
        do_someone_logout(msg);
        break;

      case C2C_CHAT:
        do_chat(msg);
        break;
      default:
        break;
      }
    }

    // 获取标准输入，发送给服务器
    if (FD_ISSET(STDIN_FILENO, &rset))
    {
      char cmdline[100];
      if (fgets(cmdline, sizeof(cmdline), stdin) == NULL)
        break;

      if (cmdline[0] == '\n')
        continue;

      cmdline[strlen(cmdline) - 1] = '\0';
      parse_cmd(cmdline, sockfd, &servaddr);
    }
  }

  return 0;
}
