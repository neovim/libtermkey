#include "../termkey.h"
#include "taplib.h"

int main(int argc, char *argv[])
{
  TermKey      *tk;
  TermKeyKey    key;
  char         *endp;

#define CLEAR_KEY do { key.type = -1; key.code.codepoint = -1; key.modifiers = -1; key.utf8[0] = 0; } while(0)

  plan_tests(18);

  tk = termkey_new(0, TERMKEY_FLAG_NOTERMIOS);

  CLEAR_KEY;
  endp = termkey_strpkey(tk, " ", &key, 0);
  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type for SP/unicode");
  is_int(key.code.codepoint, ' ',               "key.code.codepoint for SP/unicode");
  is_int(key.modifiers,   0,                    "key.modifiers for SP/unicode");
  is_str(key.utf8,        " ",                  "key.utf8 for SP/unicode");
  is_str(endp, "", "consumed entire input for SP/unicode");

  CLEAR_KEY;
  endp = termkey_strpkey(tk, "Space", &key, 0);
  is_int(key.type,        TERMKEY_TYPE_UNICODE, "key.type for Space/unicode");
  is_int(key.code.codepoint, ' ',               "key.code.codepoint for Space/unicode");
  is_int(key.modifiers,   0,                    "key.modifiers for Space/unicode");
  is_str(key.utf8,        " ",                  "key.utf8 for Space/unicode");
  is_str(endp, "", "consumed entire input for Space/unicode");

  termkey_set_flags(tk, termkey_get_flags(tk) | TERMKEY_FLAG_SPACESYMBOL);

  CLEAR_KEY;
  endp = termkey_strpkey(tk, " ", &key, 0);
  is_int(key.type,      TERMKEY_TYPE_KEYSYM, "key.type for SP/symbol");
  is_int(key.code.sym,  TERMKEY_SYM_SPACE,   "key.code.codepoint for SP/symbol");
  is_int(key.modifiers, 0,                   "key.modifiers for SP/symbol");
  is_str(endp, "", "consumed entire input for SP/symbol");

  CLEAR_KEY;
  endp = termkey_strpkey(tk, "Space", &key, 0);
  is_int(key.type,      TERMKEY_TYPE_KEYSYM, "key.type for Space/symbol");
  is_int(key.code.sym,  TERMKEY_SYM_SPACE,   "key.code.codepoint for Space/symbol");
  is_int(key.modifiers, 0,                   "key.modifiers for Space/symbol");
  is_str(endp, "", "consumed entire input for Space/symbol");

  termkey_destroy(tk);

  return exit_status();
}
