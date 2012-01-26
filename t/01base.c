#include <stdio.h>
#include "../termkey.h"
#include "taplib.h"

int main(int argc, char *argv[])
{
  TermKey   *tk;

  plan_tests(2);

  tk = termkey_new_abstract("vt100", 0);

  ok(!!tk, "termkey_new_abstract");

  termkey_destroy(tk);

  ok(1, "termkey_free");

  return exit_status();
}
