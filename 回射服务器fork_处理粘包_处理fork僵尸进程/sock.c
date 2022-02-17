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