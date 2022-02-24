#ifndef _SOCK_H
#define _SOCK_H 1

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <signal.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <iostream>

#define ERR_EXIT(msg)   \
  do                    \
  {                     \
    perror(msg);        \
    exit(EXIT_FAILURE); \
  } while (0)

// C2S 客户端发送个服务器的信号
#define C2S_LOGIN 0x01       // 登录
#define C2S_LOGOUT 0x02      // 登出
#define C2S_ONLINE_USER 0x03 // 在线用户列表

// S2C 服务器发送到客户端
#define S2C_LOGIN_OK 0x01        // 登录成功
#define S2C_ALREADY_LOGINED 0x02 // 这个账号已经登录了
#define S2C_SOMEONE_LOGIN 0x03   // 有客户登录，通知其他客户
#define S2C_SOMEONE_LOGOUT 0x04  // 有客户登出，通知其他客户
#define S2C_ONLINE_USER 0x05     // 返回给客户，当前在线的客户列表

// C2C 客户之间的消息发送
#define C2C_CHAT 0x06

#define MSG_LEN 512 // 消息最大字节
typedef struct message
{
  int cmd;
  char body[MSG_LEN];
} MESSAGE;

typedef struct userinfo
{
  char username[16];
  uint32_t ip;
  uint16_t port;
} USERINFO;

typedef struct chat_msg
{
  char username[16];
  char msg[100];
} CHAT_MSG;

// 用户列表类型
typedef std::vector<USERINFO> USER_LIST;

#define ZERO_MSG(msg) memset(&msg, 0, sizeof(msg))

#endif
