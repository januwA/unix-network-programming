## poll

没有 FD_MAXSIZE 限制

```c
#include <poll.h>


/* 描述轮询请求的数据结构.  */
struct pollfd
  {
    int fd;			/* 要轮询的文件描述符.  */
    short int events;		/* poller 关心的事件类型.  */
    short int revents;		/* 实际发生的事件类型.  */
  };

int poll(struct pollfd fds[], nfds_t nfds, int timeout);
```

poll() 函数为应用程序提供了一种通过一组文件描述符多路复用输入/输出的机制。 对于 fds 指向的数组的每个成员，poll() 将检查给定文件描述符的事件中指定的事件。 fds 数组中 pollfd 结构的数量由 nfds 指定。 poll() 函数应识别应用程序可以读取或写入数据的文件描述符，或已发生某些事件的文件描述符。
