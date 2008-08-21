#include <stdio.h>

#include "termkey.h"

int main(int argc, char *argv[]) {
  termkey_t *tk = termkey_new(0, 0);

  termkey_result ret;
  termkey_key key;

  while((ret = termkey_waitkey(tk, &key)) != TERMKEY_RES_EOF) {
    switch(key.type) {
    case TERMKEY_TYPE_KEYSYM:
      printf("Key %s%s%s%s (code %d)\n",
          key.modifiers & TERMKEY_KEYMOD_SHIFT ? "Shift-" : "",
          key.modifiers & TERMKEY_KEYMOD_ALT   ? "Alt-" : "",
          key.modifiers & TERMKEY_KEYMOD_CTRL  ? "Ctrl-" : "",
          termkey_get_keyname(tk, key.code.sym),
          key.code.sym);
      break;
    case TERMKEY_TYPE_FUNCTION:
      printf("Function key %s%s%sF%d\n",
          key.modifiers & TERMKEY_KEYMOD_SHIFT ? "Shift-" : "",
          key.modifiers & TERMKEY_KEYMOD_ALT   ? "Alt-" : "",
          key.modifiers & TERMKEY_KEYMOD_CTRL  ? "Ctrl-" : "",
          key.code.number);
      break;
    case TERMKEY_TYPE_UNICODE:
      printf("Unicode %s%s%s%s (U+%04X)\n",
          key.modifiers & TERMKEY_KEYMOD_SHIFT ? "Shift-" : "",
          key.modifiers & TERMKEY_KEYMOD_ALT   ? "Alt-" : "",
          key.modifiers & TERMKEY_KEYMOD_CTRL  ? "Ctrl-" : "",
          key.utf8,
          key.code.codepoint);
      break;
    }

    if(key.type == TERMKEY_TYPE_UNICODE && 
       key.modifiers & TERMKEY_KEYMOD_CTRL &&
       (key.code.codepoint == 'C' || key.code.codepoint == 'c'))
      break;
  }

  termkey_destroy(tk);
}
