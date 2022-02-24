#include "sock.h"

USER_LIST clint_list{0};

// 登录，储存信息
void do_login(MESSAGE &msg, int sockfd, sockaddr_in *connaddr)
{
  USERINFO user;
  strcpy(user.username, msg.body);
  user.ip = connaddr->sin_addr.s_addr;
  user.port = connaddr->sin_port;

  bool logined = false;
  for (auto &i : clint_list)
  {
    if (strcmp(i.username, user.username) == 0)
    {
      logined = true;
      break;
    }
  }

  MESSAGE reply_msg;
  ZERO_MSG(reply_msg);
  if (logined)
  {
    printf("用户 %s 已经登陆\n", user.username);
    reply_msg.cmd = htonl(S2C_ALREADY_LOGINED);
  }
  else
  {
    char *connip = inet_ntoa(connaddr->sin_addr);
    printf("用户 %s登录 ip:%s\n", user.username, connip);
    clint_list.push_back(user);

    reply_msg.cmd = htonl(S2C_LOGIN_OK);
  }

  // 应答
  sendto(sockfd, &reply_msg, sizeof(reply_msg), 0, (struct sockaddr *)connaddr, sizeof(struct sockaddr));

  if (logined)
    return;

  // 发送当前在线人数
  int count = htonl((int)clint_list.size());
  sendto(sockfd, &count, sizeof(int), 0, (struct sockaddr *)connaddr, sizeof(struct sockaddr));

  // 发送在线人数列表
  for (auto &i : clint_list)
  {
    sendto(sockfd, &i, sizeof(USERINFO), 0, (struct sockaddr *)connaddr, sizeof(struct sockaddr));
  }

  // 向其他用户通知有新用户登录
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  for (auto &i : clint_list)
  {
    if (i.username != user.username)
    {
      memset(&addr, 0, sizeof(addrlen));
      ZERO_MSG(reply_msg);

      addr.sin_family = AF_INET;
      addr.sin_port = i.port;
      addr.sin_addr.s_addr = i.ip;

      // 发送消息
      reply_msg.cmd = htonl(S2C_SOMEONE_LOGIN);
      memcpy(reply_msg.body, &user, sizeof(user));

      if (sendto(sockfd, &reply_msg, sizeof(MESSAGE), 0, (struct sockaddr *)&addr, addrlen) < 0)
        ERR_EXIT("sendto");
    }
  }
}

void do_logout(MESSAGE &msg, int sockfd, sockaddr_in *connaddr)
{
}

void do_sendlist(int sockfd, sockaddr_in *connaddr)
{
}

int main(int argc, char **argv)
{
  signal(SIGPIPE, SIG_IGN); // 客户主动关闭后，再会设置会收到 SIGPIPE 信号
  int sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0)
    ERR_EXIT("socket");

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(8888);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 本机的任意IPv4
  // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 旧的方法，被 inet_aton 取代
  // inet_aton("127.0.0.1", &servaddr.sin_addr);

  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    ERR_EXIT("bind");

  struct sockaddr_in connaddr;
  socklen_t connlen = sizeof(connaddr);
  int n;
  MESSAGE msg;
  while (1)
  {
    ZERO_MSG(msg);

    n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&connaddr, &connlen);

    if (n < 0)
    {
      if (errno == EINTR)
        continue;
      ERR_EXIT("recvfrom");
    }

    int cmd = ntohl(msg.cmd); // 网络字节序转为本机字节序

    switch (cmd)
    {
    case C2S_LOGIN:
      do_login(msg, sockfd, &connaddr);
      break;

    case C2S_LOGOUT:
      do_logout(msg, sockfd, &connaddr);
      break;

    case C2S_ONLINE_USER:
      do_sendlist(sockfd, &connaddr);
      break;

    default:
      break;
    }
  }

  return 0;
}
