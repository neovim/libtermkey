#include <stdio.h>
#include "termkey.h"
#include "taplib.h"

int main(int argc, char *argv[])
{
  TermKey   *tk;

  plan_tests(2);

  tk = termkey_new(0, TERMKEY_FLAG_NOTERMIOS);

  ok(!!tk, "termkey_new");

  termkey_destroy(tk);

  ok(1, "termkey_free");

  return 0;
}
