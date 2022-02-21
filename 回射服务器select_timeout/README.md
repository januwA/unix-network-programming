超时处理

## select 限制

用select实现的并发服务器，能达到的并发数，受到两方面限制

- 一个进程能打开的最大文件描述符限制，这可以通过调整内核参数。

  查看和设置一个进程最多能打开多少文件描述符:
  ```sh
  # ulimit -n
  1024

  # ulimit -n 2048

  # ulimit -n
  2048
  ```

  也可以通过编程的方法获取和修改:
  ```c
  #include <sys/resource.h>

  int getrlimit(int resource, struct rlimit *rlp);
  int setrlimit(int resource, const struct rlimit *rlp);
  ```
- select 中的fd_set集合容量限制(FD_SETSIZE)，这需要重新编译内核
  

