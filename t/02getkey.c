#include <stdio.h>
#include "termkey.h"
#include "taplib.h"

int main(int argc, char *argv[])
{
  int        fd[2];
  TermKey   *tk;
  TermKeyKey key;

  plan_tests(19);

  pipe(fd);

  /* Sanitise this just in case */
  putenv("TERM=vt100");

  tk = termkey_new(fd[0], TERMKEY_FLAG_NOTERMIOS);

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_NONE, "getkey yields RES_NONE when empty");

  write(fd[1], "h", 1);

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_NONE, "getkey yields RES_NONE before advisereadable");

  is_int(termkey_advisereadable(tk), TERMKEY_RES_AGAIN, "advisereadable yields RES_AGAIN after h");

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY after h");

  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type after h");
  is_int(key.code.number, 'h',                  "key.code.number after h");
  is_int(key.modifiers,   0,                    "key.modifiers after h");
  is_str(key.utf8,        "h",                  "key.utf8 after h");

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_NONE, "getkey yields RES_NONE a second time");

  write(fd[1], "\x01", 1);

  is_int(termkey_advisereadable(tk), TERMKEY_RES_AGAIN, "advisereadable yields RES_AGAIN after C-a");

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY after C-a");

  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type after C-a");
  is_int(key.code.number, 'a',                  "key.code.number after C-a");
  is_int(key.modifiers,   TERMKEY_KEYMOD_CTRL,  "key.modifiers after C-a");

  write(fd[1], "\eOA", 3);

  is_int(termkey_advisereadable(tk), TERMKEY_RES_AGAIN, "advisereadable yields RES_AGAIN after Up");

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY after Up");

  is_int(key.type,        TERMKEY_TYPE_KEYSYM,  "key.type after Up");
  is_int(key.code.sym,    TERMKEY_SYM_UP,       "key.code.sym after Up");
  is_int(key.modifiers,   0,                    "key.modifiers after Up");

  termkey_destroy(tk);

  return exit_status();
}