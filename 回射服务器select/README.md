select 能同时管理多个io (使用单进程IO复用来处理多个客户)


select,	pselect,  FD_CLR,  FD_ISSET, FD_SET, FD_ZERO - 同步 I/O 多路复用

select() 和 pselect() 允许程序监视多个文件描述符，等待一个或多个文件描述符“准备好”用于某种类型的 I/O 操作（例如，可能的输入）。 如果可以在没有阻塞的情况下执行相应的 I/O 操作（例如 read(2)），则认为文件描述符已准备就绪。

  select() 和 pselect() 的操作是相同的，但有三个不同之处：

(i) select() 使用的超时时间是 struct timeval（秒和微秒），而 pselect() 使用 struct timespec（秒和纳秒）。

(ii) select() 可能会更新超时参数以指示还剩多少时间。 pselect() 不会更改此参数。

(iii) select() 没有 sigmask 参数，其行为与使用 NULL sigmask 调用的 pselect() 一样。 

---

## close 与 shutdown 区别

- close 终止两个方向的数据传输
- shutdown 可以选择某个方向或则两个方向
- shutdown how=1可以保证对对等方收到 EOF字符，而不管其他进程是否还在使用套接口，而close只有套接口引用计数为0时才发送。

```c
#include <sys/socket.h>

int shutdown(int socket, int how);
```

shutdown() 函数将关闭与文件描述符套接字相关联的套接字上的全部或部分全双工连接。

shutdown() 函数采用以下参数：

socket 指定套接字的文件描述符。

how 指定关闭的类型。 值如下：

  SHUT_RD 禁用进一步的接收操作。

  SHUT_WR 禁用进一步的发送操作。

  SHUT_RDWR 禁用进一步的发送和接收操作。

shutdown() 函数根据 how 参数的值禁用套接字上的后续发送和/或接收操作。