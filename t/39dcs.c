#include "../termkey.h"
#include "taplib.h"

int main(int argc, char *argv[])
{
  TermKey   *tk;
  TermKeyKey key;
  const char *str;

  plan_tests(23);

  tk = termkey_new_abstract("xterm", 0);

  // 7bit DCS
  termkey_push_bytes(tk, "\x1bP1$r1 q\x1b\\", 10);

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY for DCS");

  is_int(key.type,        TERMKEY_TYPE_DCS, "key.type for DCS");
  is_int(key.modifiers,   0,                "key.modifiers for DCS");

  is_int(termkey_interpret_string(tk, &key, &str), TERMKEY_RES_KEY, "termkey_interpret_string() gives string");
  is_str(str, "1$r1 q", "termkey_interpret_string() yields correct string");

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_NONE, "getkey again yields RES_NONE");

  // 8bit DCS
  termkey_push_bytes(tk, "\x90""1$r2 q""\x9c", 8);

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY for DCS");

  is_int(key.type,        TERMKEY_TYPE_DCS, "key.type for DCS");
  is_int(key.modifiers,   0,                "key.modifiers for DCS");

  is_int(termkey_interpret_string(tk, &key, &str), TERMKEY_RES_KEY, "termkey_interpret_string() gives string");
  is_str(str, "1$r2 q", "termkey_interpret_string() yields correct string");

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_NONE, "getkey again yields RES_NONE");

  // 7bit OSC
  termkey_push_bytes(tk, "\x1b]15;abc\x1b\\", 10);

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY for OSC");

  is_int(key.type,        TERMKEY_TYPE_OSC, "key.type for OSC");
  is_int(key.modifiers,   0,                "key.modifiers for OSC");

  is_int(termkey_interpret_string(tk, &key, &str), TERMKEY_RES_KEY, "termkey_interpret_string() gives string");
  is_str(str, "15;abc", "termkey_interpret_string() yields correct string");

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_NONE, "getkey again yields RES_NONE");

  // False alarm
  termkey_push_bytes(tk, "\x1bP", 2);

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_AGAIN, "getkey yields RES_AGAIN for false alarm");

  is_int(termkey_getkey_force(tk, &key), TERMKEY_RES_KEY, "getkey_force yields RES_KEY for false alarm");

  is_int(key.type,           TERMKEY_TYPE_UNICODE, "key.type for false alarm");
  is_int(key.code.codepoint, 'P',                  "key.code.codepoint for false alarm");
  is_int(key.modifiers,      TERMKEY_KEYMOD_ALT,   "key.modifiers for false alarm");

  termkey_destroy(tk);

  return exit_status();
}
