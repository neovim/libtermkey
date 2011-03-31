#include "../termkey.h"
#include "taplib.h"

int main(int argc, char *argv[])
{
  int        fd[2];
  TermKey   *tk;
  TermKeyKey key;

  plan_tests(21);

  pipe(fd);

  /* Sanitise this just in case */
  putenv("TERM=vt100");

  tk = termkey_new(fd[0], TERMKEY_FLAG_NOTERMIOS|TERMKEY_FLAG_UTF8);

  write(fd[1], "a", 1);

  termkey_advisereadable(tk);
  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY low ASCII");
  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type low ASCII");
  is_int(key.code.number, 'a',                  "key.code.number low ASCII");

  /* 2-byte UTF-8 range is U+0080 to U+07FF (0xDF 0xBF) */
  /* However, we'd best avoid the C1 range, so we'll start at U+00A0 (0xC2 0xA0) */

  write(fd[1], "\xC2\xA0", 2);

  termkey_advisereadable(tk);
  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY UTF-8 2 low");
  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type UTF-8 2 low");
  is_int(key.code.number, 0x00A0,               "key.code.number UTF-8 2 low");

  write(fd[1], "\xDF\xBF", 2);

  termkey_advisereadable(tk);
  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY UTF-8 2 high");
  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type UTF-8 2 high");
  is_int(key.code.number, 0x07FF,               "key.code.number UTF-8 2 high");

  /* 3-byte UTF-8 range is U+0800 (0xE0 0xA0 0x80) to U+FFFD (0xEF 0xBF 0xBD) */

  write(fd[1], "\xE0\xA0\x80", 3);

  termkey_advisereadable(tk);
  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY UTF-8 3 low");
  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type UTF-8 3 low");
  is_int(key.code.number, 0x0800,               "key.code.number UTF-8 3 low");

  write(fd[1], "\xEF\xBF\xBD", 3);

  termkey_advisereadable(tk);
  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY UTF-8 3 high");
  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type UTF-8 3 high");
  is_int(key.code.number, 0xFFFD,               "key.code.number UTF-8 3 high");

  /* 4-byte UTF-8 range is U+10000 (0xF0 0x90 0x80 0x80) to U+10FFFF (0xF4 0x8F 0xBF 0xBF) */

  write(fd[1], "\xF0\x90\x80\x80", 4);

  termkey_advisereadable(tk);
  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY UTF-8 4 low");
  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type UTF-8 4 low");
  is_int(key.code.number, 0x10000,              "key.code.number UTF-8 4 low");

  write(fd[1], "\xF4\x8F\xBF\xBF", 4);

  termkey_advisereadable(tk);
  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY UTF-8 4 high");
  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type UTF-8 4 high");
  is_int(key.code.number, 0x10FFFF,             "key.code.number UTF-8 4 high");

  termkey_destroy(tk);

  return exit_status();
}
