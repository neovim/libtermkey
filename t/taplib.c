#include "taplib.h"

#include <stdio.h>

static int nexttest = 1;
static int _exit_status = 0;

void plan_tests(int n)
{
  printf("1..%d\n", n);
}

void ok(int cmp, char *name)
{
  printf("%s %d - %s\n", cmp ? "ok" : "not ok", nexttest++, name);
  if(!cmp)
    _exit_status = 1;
}

int exit_status(void)
{
  return _exit_status;
}
