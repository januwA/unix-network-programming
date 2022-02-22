#include "sock.h"

ssize_t readn(int fd, void *buf, size_t rn)
{
  size_t nleft = rn; // 剩余要读取的字节数
  void *bufp = buf;

  while (nleft > 0)
  {
    // 返回读取了多少字节
    size_t n = read(fd, bufp, nleft);
    if (n < 0) // 读取错误
    {
      if (errno == EINTR)
        continue;
      return -1;
    }

    if (n == 0)          // 对等方关闭了
      return rn - nleft; // 返回以读取字节数

    bufp += n; // 偏移指针，避免数据覆盖
    nleft -= n;
  }

  return rn;
}

ssize_t writen(int fd, const void *buf, size_t wn)
{
  size_t nleft = wn; // 剩余要写入的字节数
  const void *bufp = buf;

  while (nleft > 0)
  {
    // 返回写入了多少字节
    size_t n = write(fd, bufp, nleft);
    if (n < 0) // 读取错误
    {
      if (errno == EINTR)
        continue;
      return -1;
    }

    if (n == 0) // 什么都没有发送
      continue;

    bufp += n;  // 偏移指针，避免发送重复数据
    nleft -= n; // 减去已经写入的字节数
  }

  return wn;
}

int read_timeout(int fd, size_t s)
{
  int ret = 0;
  if (s > 0)
  {
    fd_set rset;
    struct timeval timeout;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    // 设置时间
    timeout.tv_sec = s;
    timeout.tv_usec = 0;

    do
    {
      ret = select(fd + 1, &rset, NULL, NULL, &timeout);
    } while (ret < 0 && errno == EINTR); // 如果信号中断，继续select

    // 超时返回 0
    if (ret == 0)
    {
      ret = -1;
      errno = ETIMEDOUT;
    }

    // 在时间内发生了可读事件
    if (ret == 1)
      ret = 0;
  }

  return ret;
}

int write_timeout(int fd, size_t s)
{
  int ret = 0;
  if (s > 0)
  {
    fd_set wset;
    struct timeval timeout;
    FD_ZERO(&wset);
    FD_SET(fd, &wset);

    // 设置时间
    timeout.tv_sec = s;
    timeout.tv_usec = 0;

    do
    {
      ret = select(fd + 1, NULL, &wset, NULL, &timeout);
    } while (ret < 0 && errno == EINTR); // 如果信号中断，继续select

    // 超时返回 0
    if (ret == 0)
    {
      ret = -1;
      errno = ETIMEDOUT;
    }

    // 在时间内发生了可写事件
    if (ret == 1)
      ret = 0;
  }

  return ret;
}

int accept_timeout(int fd, struct sockaddr_in *addr, size_t s)
{
  int ret = 0;
  socklen_t addrlen = sizeof(struct sockaddr_in);

  if (s > 0)
  {
    fd_set rset;
    struct timeval timeout;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    // 设置时间
    timeout.tv_sec = s;
    timeout.tv_usec = 0;

    do
    {
      ret = select(fd + 1, &rset, NULL, NULL, &timeout);
    } while (ret < 0 && errno == EINTR); // 如果信号中断，继续select

    // 失败
    if (ret == -1)
      return -1;

    // 超时
    if (ret == 0)
    {
      errno = ETIMEDOUT;
      return -1;
    }
  }

  if (addr != NULL)
    ret = accept(fd, (struct sockaddr *)addr, &addrlen);
  else
    ret = accept(fd, NULL, NULL);

  if (ret == -1)
    ERR_EXIT("accept");

  return ret;
}

void activate_monblock(int fd)
{
  int ret;

  // 获取文件描述符的标志
  int flags = fcntl(fd, F_GETFL);

  if (flags == -1)
    ERR_EXIT("fcntl");

  // 添加非阻塞标志
  flags |= O_NONBLOCK;
  ret = fcntl(fd, F_SETFL, flags);

  if (flags == -1)
    ERR_EXIT("fcntl");
}

void deactivate_monblock(int fd)
{
  int ret;

  // 获取文件描述符的标志
  int flags = fcntl(fd, F_GETFL);

  if (flags == -1)
    ERR_EXIT("fcntl");

  // 去掉非阻塞标志
  flags &= ~O_NONBLOCK;
  ret = fcntl(fd, F_SETFL, flags);

  if (flags == -1)
    ERR_EXIT("fcntl");
}

int connect_timeout(int fd, struct sockaddr *addr, size_t s)
{
  int ret = 0;
  socklen_t addrlen = sizeof(struct sockaddr);

  if (s > 0)
    activate_monblock(fd);

  // 设置为非阻塞标志，connect现在不在阻塞
  ret = connect(fd, addr, addrlen);

  // EINPROGRESS 连接正在处理中
  if (ret < 0 && errno == EINPROGRESS)
  {
    fd_set wset;
    struct timeval timeout;
    FD_ZERO(&wset);
    FD_SET(fd, &wset);

    // 设置时间
    timeout.tv_sec = s;
    timeout.tv_usec = 0;

    do
    {
      // 一旦连接建立，套接字就可写
      ret = select(fd + 1, NULL, &wset, NULL, &timeout);
    } while (ret < 0 && errno == EINTR); // 如果信号中断，继续select

    // 超时
    if (ret == 0)
    {
      ret = -1;
      errno = ETIMEDOUT;
    }

    if (ret < 0)
      return -1;

    // 返回1，可能有两种情况，建立成功或则套接字产生错误
    if (ret == 1)
    {

      // 调用 getsockopt 来获取socket的错误
      int err;
      socklen_t socklen = sizeof(err);
      int sockopt = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);

      // 获取 opt 失败
      if (sockopt < 0)
        return -1;

      if (err == 0) // socket没有错误
        ret = 0;
      else
      {
        // socket 有错误
        ret = -1;
        errno = err;
      }
    }
  }

  // 成功返回0后，取消掉描述符的非阻塞模式
  if (s > 0)
    deactivate_monblock(fd);

  return ret;
}