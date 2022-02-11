#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
  char *a = malloc(1024);

  strcpy(a, "hello world");

  a = realloc(a, 100);
  printf("%s\n", a);
  printf("%lu\n", strlen(a));
  free(a);
}