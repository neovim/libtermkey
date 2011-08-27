#include <stdio.h>
#include "../termkey.h"
#include "taplib.h"

int main(int argc, char *argv[])
{
  int        fd[2];
  TermKey   *tk;
  TermKeyKey key;

  plan_tests(8);

  pipe(fd);

  /* Sanitise this just in case */
  putenv("TERM=vt100");

  tk = termkey_new(fd[0], TERMKEY_FLAG_NOTERMIOS);

  write(fd[1], " ", 1);

  is_int(termkey_waitkey(tk, &key), TERMKEY_RES_KEY, "waitkey yields RES_KEY after space");

  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type after space");
  is_int(key.code.number, ' ',                  "key.code.number after space");
  is_int(key.modifiers,   0,                    "key.modifiers after space");

  termkey_set_flags(tk, TERMKEY_FLAG_SPACESYMBOL);

  write(fd[1], " ", 1);

  is_int(termkey_waitkey(tk, &key), TERMKEY_RES_KEY, "waitkey yields RES_KEY after space");

  is_int(key.type,        TERMKEY_TYPE_KEYSYM, "key.type after space with FLAG_SPACESYMBOL");
  is_int(key.code.number, TERMKEY_SYM_SPACE,   "key.code.sym after space with FLAG_SPACESYMBOL");
  is_int(key.modifiers,   0,                   "key.modifiers after space with FLAG_SPACESYMBOL");

  termkey_destroy(tk);

  return exit_status();
}
