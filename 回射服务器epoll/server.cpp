#include "sock.h"
#include <vector>

typedef std::vector<struct epoll_event> EventList;

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

  std::vector<int> clients;                   // 储存所有客户的描述符
  int epollfd = epoll_create1(EPOLL_CLOEXEC); // 创建一个epoll实例

  // 将文件描述符加入到 epoll 实例中管理
  struct epoll_event event;
  event.data.fd = listenfd;
  // EPOLLIN 相关文件可用于 read(2) 操作
  event.events = EPOLLIN | EPOLLET; // 监听感兴趣的描述符事件
  epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);

  EventList events(16);

  struct sockaddr_in connaddr;
  socklen_t connlen = sizeof(connaddr);
  int connfd;
  int i;
  struct packet rp;
  while (1)
  {
    // 等待事件发生
    int nready = epoll_wait(epollfd, &*events.begin(), (int)(events.size()), -1);
    if (nready < 0)
    {
      if (errno == EINTR)
        continue;
      ERR_EXIT("epoll_wait");
    }

    // 如果设置了超时时间 超时返回0
    if (nready == 0)
      continue;

    if ((size_t)nready == events.size())
      events.resize(events.size() * 2);

    for (i = 0; i < nready; i++)
    {
      if (events[i].data.fd == listenfd)
      {
        // 这里的 accept 将不再阻塞
        if ((connfd = accept(listenfd, (struct sockaddr *)&connaddr, &connlen)) < 0)
          ERR_EXIT("accept");

        char *connip = inet_ntoa(connaddr.sin_addr);
        printf("[[CONNECT]] %s:%d\n", connip, connaddr.sin_port); // 客户 ip:port

        // 保存连接的客户
        clients.push_back(connfd);
        activate_nonblock(connfd);

        // 加入 epoll
        event.data.fd = connfd;
        event.events = EPOLLIN | EPOLLET;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event);
      }
      else if (events[i].events & EPOLLIN) // 产生了可读事件
      {
        if ((connfd = events[i].data.fd) < 0)
          continue;
        getpeername(connfd, (struct sockaddr *)&connaddr, &connlen);
        char *connip = inet_ntoa(connaddr.sin_addr);

        // 读取时不在阻塞
        int n = readn(connfd, &rp.len, sizeof(rp.len)); // 先读取buf长度
        if (n < 0)
          ERR_EXIT("readn");

        if (n == 0)
        {
          printf("[[CLSOE]] %s:%d\n", connip, connaddr.sin_port);
          close(connfd);

          // 从 epoll 实例中移除
          event = events[i];
          epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &event);

          // 客户端中删除
          auto noSpaceEnd = std::remove(clients.begin(), clients.end(), connfd);
          clients.erase(noSpaceEnd, clients.end());
          break;
        }

        if (n != sizeof(rp.len))
        {
          printf("读取包头 len 错误\n");
          close(connfd);

          // 从 epoll 实例中移除
          event = events[i];
          epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &event);

          // 客户端中删除
          auto noSpaceEnd = std::remove(clients.begin(), clients.end(), connfd);
          clients.erase(noSpaceEnd, clients.end());
          break;
        }

        int buflen = ntohl(rp.len);
        int bufn = readn(connfd, &rp.buf, buflen); // 读取buf数据
        if (bufn < 0)
          ERR_EXIT("readn");

        if (bufn == 0)
        {
          printf("[[CLSOE]] %s:%d\n", connip, connaddr.sin_port);
          close(connfd);

          // 从 epoll 实例中移除
          event = events[i];
          epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &event);

          // 客户端中删除
          auto noSpaceEnd = std::remove(clients.begin(), clients.end(), connfd);
          clients.erase(noSpaceEnd, clients.end());
          break;
        }

        if (bufn != buflen)
        {
          printf("读取包体错误，因读(%d),实际(%d)\n", buflen, bufn);
          close(connfd);

          // 从 epoll 实例中移除
          event = events[i];
          epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &event);

          // 客户端中删除
          auto noSpaceEnd = std::remove(clients.begin(), clients.end(), connfd);
          clients.erase(noSpaceEnd, clients.end());
          break;
        }

        printf("[[MSG]] %s:%d: %s", connip, connaddr.sin_port, rp.buf);
        // sleep(5); // 模拟延迟回复
        writen(connfd, &rp, sizeof(rp.len) + bufn);
        memset(&rp, 0, sizeof(rp));
      }
    }
  }

  return 0;
}
