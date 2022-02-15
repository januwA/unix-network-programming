#ifndef _SOCK_H
#define _SOCK_H 1

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define ERR_EXIT(msg)   \
  do                    \
  {                     \
    perror(msg);        \
    exit(EXIT_FAILURE); \
  } while (0)

struct packet
{
  int len;        // 数据长度
  char buf[1024]; // 存放数据
};

// 读取定长包，双端固定包的字节数
ssize_t readn(int fd, void *buf, size_t rn);

// 写入定长包
ssize_t writen(int fd, const void *buf, size_t wn);

#endif
