#include "sock.h"

int main(int argc, char **argv)
{
  struct packet a;
  a.len = 10;
  printf("%lu\n", sizeof(a.len));
}