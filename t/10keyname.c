#include "termkey.h"
#include "taplib.h"

int main(int argc, char *argv[])
{
  TermKey   *tk;
  TermKeySym sym;

  plan_tests(3);

  tk = termkey_new(0, TERMKEY_FLAG_NOTERMIOS);

  sym = termkey_keyname2sym(tk, "Space");
  is_int(sym, TERMKEY_SYM_SPACE, "keyname2sym Space");

  sym = termkey_keyname2sym(tk, "SomeUnknownKey");
  is_int(sym, TERMKEY_SYM_UNKNOWN, "keyname2sym SomeUnknownKey");

  is_str(termkey_get_keyname(tk, TERMKEY_SYM_SPACE), "Space", "get_keyname SPACE");

  termkey_destroy(tk);

  return exit_status();
}
