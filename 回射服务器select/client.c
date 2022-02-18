#include "sock.h"

int main(int argc, char **argv)
{
  int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0)
    ERR_EXIT("socket");

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(8888);
  inet_aton("127.0.0.1", &servaddr.sin_addr);

  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    ERR_EXIT("connect");

  struct packet sp;
  struct packet rp;
  memset(&sp, 0, sizeof(struct packet));
  memset(&rp, 0, sizeof(struct packet));

  int nready;
  int maxfd;
  fd_set rset;
  FD_ZERO(&rset);
  int stdinfd = fileno(stdin);
  maxfd = fmax(stdinfd, sockfd);
  int stdineof = 0;
  while (1)
  {
    if (stdineof == 0) // 标准输入终止后不再添加
      FD_SET(stdinfd, &rset);
    FD_SET(sockfd, &rset);

    nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
    if (nready < 0)
      ERR_EXIT("select");

    // sockfd 可读
    if (FD_ISSET(sockfd, &rset))
    {
      int n = readn(sockfd, &rp.len, sizeof(rp.len)); // 先读取buf长度
      if (n < 0)
        ERR_EXIT("readn");
      if (n == 0)
        break;
      if (n != sizeof(rp.len))
      {
        printf("读取包头 len 错误\n");
        break;
      }

      int rbl = ntohl(rp.len);
      int bufn = readn(sockfd, &rp.buf, rbl); // 读取buf
      if (bufn < 0)
        ERR_EXIT("readn");
      if (bufn == 0)
        break;
      if (bufn != rbl)
      {
        printf("读取包体错误，因读(%d),实际(%d)\n", rbl, bufn);
        break;
      }
      fputs(rp.buf, stdout);
      memset(&rp, 0, sizeof(struct packet));
    }

    // 标准输入可读
    if (FD_ISSET(stdinfd, &rset))
    {
      if (fgets(sp.buf, sizeof(sp.buf), stdin) == NULL)
      {
        stdineof = 1;
        shutdown(sockfd, SHUT_WR); // 关闭写入方
      }
      else
      {
        int sbl = strlen(sp.buf);               // 用户输入数据长度
        sp.len = htonl(sbl);                    // 将主机字节序转为网络字节序
        int packet_size = sizeof(sp.len) + sbl; // 发送包的实际大小
        // printf("send buf len: %d\n", sbl);
        // printf("send packet size: %d\n", packet_size);
        writen(sockfd, &sp, packet_size); // 发送实际数据大小的字节数据
        memset(&sp, 0, sizeof(struct packet));
      }
    }
  }

  return 0;
}
