#include <stdio.h>
#include "termkey.h"

int main(int argc, char *argv[])
{
  TermKey   *tk;

  printf("1..2\n");

  tk = termkey_new(0, TERMKEY_FLAG_NOTERMIOS);

  printf(tk ? "" : "not ");
  printf("ok 1 - termkey_new\n");

  termkey_destroy(tk);

  printf("ok 2 - termkey_free\n");

  return 0;
}
