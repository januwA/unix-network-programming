## epoll

### epoll与select和poll的区别


- 相比于select与poll，epoll最大的好处在于不会随着监听fd的数量增大而降低效率
- 内核中的select与poll的实现是采用轮询来处理的，轮询的fd数目越多，自然耗时越多
- epoll的实现是基于回调，如果fd有期望事件发生就通过回调函数将其加入epoll就绪队列中
- 内核/用户空间内存拷贝问题，select/poll采取了内存拷贝方法。而epoll采用了共享内存的方式
- epoll不仅会告诉应用程序有I/O时间到来，还会告诉应用程序相关的信息，这些信息是应用程序填充的，因此根据这些信息应用程序就能定位到事件，而不必遍历整个fd集合


```c
#include <sys/epoll.h>

int epoll_create(int size);

// 与 epoll_create 相同，但带有 FLAGS 参数。 未使用的 SIZE 参数已被删除
// 现代程序中基本都是用这个函数代替 epoll_create
int epoll_create1(int flags);
```