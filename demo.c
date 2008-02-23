#include <stdio.h>

#include "termkey.h"

int main(int argc, char *argv[]) {
  termkey_t *tk = termkey_new(0, 0);

  termkey_result ret;
  termkey_key key;

  while((ret = termkey_waitkey(tk, &key)) != TERMKEY_RES_EOF) {
    if(key.flags & TERMKEY_KEYFLAG_SPECIAL)
      printf("Key %s%s%s%s (code %d)\n",
          key.modifiers & TERMKEY_KEYMOD_SHIFT ? "Shift-" : "",
          key.modifiers & TERMKEY_KEYMOD_ALT   ? "Alt-" : "",
          key.modifiers & TERMKEY_KEYMOD_CTRL  ? "Ctrl-" : "",
          termkey_get_keyname(tk, key.code),
          key.code);
    else
      printf("Unicode %s%s%s%s (U+%04X)\n",
          key.modifiers & TERMKEY_KEYMOD_SHIFT ? "Shift-" : "",
          key.modifiers & TERMKEY_KEYMOD_ALT   ? "Alt-" : "",
          key.modifiers & TERMKEY_KEYMOD_CTRL  ? "Ctrl-" : "",
          key.utf8,
          key.code);

    if(key.modifiers & TERMKEY_KEYMOD_CTRL && (key.code == 'C' || key.code == 'c'))
      break;
  }

  termkey_destroy(tk);
}
