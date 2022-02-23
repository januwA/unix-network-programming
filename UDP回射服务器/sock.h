#ifndef _SOCK_H
#define _SOCK_H 1

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/epoll.h>

#include <signal.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>

#define ERR_EXIT(msg)   \
  do                    \
  {                     \
    perror(msg);        \
    exit(EXIT_FAILURE); \
  } while (0)

#define imax(a, b) (a > b) ? a : b

struct packet
{
  int len;        // 数据长度
  char buf[1024]; // 存放数据
};

// 读取定长包，双端固定包的字节数
ssize_t readn(int fd, void *buf, size_t rn);

// 写入定长包
ssize_t writen(int fd, const void *buf, size_t wn);

/**
 * 读超时检测操作，不含读操作
 * fd - 文件描述符
 * s - 等待秒数
 * 超时返回-1，并且errno=ETIMEDOUT，反之返回0
 */
int read_timeout(int fd, size_t s);

/**
 * 写超时检测操作，不含写操作
 *
 * 超时返回-1，并且errno=ETIMEDOUT，反之返回0
 */
int write_timeout(int fd, size_t s);

/**
 * 带超时的 accept 函数
 *
 * 超时返回-1，并且errno=ETIMEDOUT，反之返回0
 */
int accept_timeout(int fd, struct sockaddr_in *addr, size_t s);

/**
 * 将 I/O 设置为非阻塞模式
 */
void activate_nonblock(int fd);

/**
 * 将 I/O 设置为阻塞模式
 */
void deactivate_nonblock(int fd);

/**
 * 带超时的 connect 函数
 *
 * SYN包到返回SYN+ACK需要RTT的时间，系统默认为75s，这段时间内 connect 将一直阻塞
 *
 * 超时返回-1，并且errno=ETIMEDOUT，反之返回0
 */
int connect_timeout(int fd, struct sockaddr *addr, size_t s);

#endif
