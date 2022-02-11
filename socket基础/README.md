socket 可以看成是用户进程与内核网络协议栈的编程接口

socket 不仅可以用于本机的进程通信，还可以用于网络上不同主机的进程通信

## IPv4 套接口地址结构:

```c
#include <netinet/in.h>

struct sockaddr_in {
    sa_family_t    sin_family; /* 地址族: AF_INET */
    in_port_t      sin_port;	  /* 网络字节序中的端口 */
    struct in_addr sin_addr;	  /* 互联网地址 */
};

/* Internet address. */
struct in_addr {
    uint32_t	      s_addr;	  /* 网络字节序中的地址 */
};
```

使用`man 7 ip`可以看到相关文档

## 通用地址结构,用来指定与套接字关联的地址:

用于兼容不同的协议族套接字

```c
#include <sys/socket.h>

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
};
```

网络字节序默认为`大端字节序`，主机字节序不同的操作系统可能不一样

## 字节序相关的转换函数:

```c
#include <arpa/inet.h>

// 从主机字节顺序转换为网络字节顺序
uint16_t htons(uint16_t __hostshort); // 2字节通常用于转换端口
uint32_t htonl(uint32_t __hostlong);  // 4字节通常用于转换ipv4地址

// 从网络字节顺序转换为主机字节顺序
uint16_t ntohs(uint16_t __netshort); // 2字节通常用于转换端口
uint32_t ntohl(uint32_t __netlong);  // 4字节通常用于转换ipv4地址

// s 表示 short
// l 表示 long
```


## 地址转换函数：

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// 将字符串ip写入in_addr结构
int inet_aton(const char *cp, struct in_addr *inp);

// 使用 inet_aton 函数代替这个函数
in_addr_t inet_addr(const char *cp);

// 网络字节顺序主机地址 转换为 IPv4点分十进制表示法的字符串
char *inet_ntoa(struct in_addr in);
```


## 套接字类型

SOCK_STREAM 提供有序、可靠、双向、基于连接的字节流。 一种带外数据传输机制——
可能支持nism。

SOCK_DGRAM 支持数据报（固定最大长度的无连接、不可靠消息）。

SOCK_SEQPACKET 为固定最大长度的数据报提供一个有序的、可靠的、基于双向连接的数据传输路径； 消费者需要在每次输入系统调用时读取整个数据包。

SOCK_RAW 提供原始网络协议访问。

SOCK_RDM 提供可靠的数据报层，不保证前序排序。