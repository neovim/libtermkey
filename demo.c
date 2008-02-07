#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "termkey.h"

int main(int argc, char *argv[]) {
  struct termios termios;

  if(tcgetattr(0, &termios)) {
    perror("ioctl(TCIOGETS)");
    exit(1);
  }

  int old_lflag = termios.c_lflag;
  termios.c_lflag &= ~(ICANON|ECHO);

  tcsetattr(0, TCSANOW, &termios);

  termkey_t *tk = termkey_new(0, TERMKEY_FLAG_CONVERTKP);

  termkey_key key;

  while(termkey_waitkey(tk, &key) && key.code != TERMKEY_SYM_EOF) {
    if(key.flags & TERMKEY_KEYFLAG_SPECIAL)
      printf("Key %s%s%s%s (code %d)\n",
          key.modifiers & TERMKEY_KEYMOD_SHIFT ? "S-" : "",
          key.modifiers & TERMKEY_KEYMOD_ALT   ? "A-" : "",
          key.modifiers & TERMKEY_KEYMOD_CTRL  ? "C-" : "",
          termkey_describe_sym(key.code),
          key.code);
    else
      printf("Key %s%s%s%s (U+%04X)\n",
          key.modifiers & TERMKEY_KEYMOD_SHIFT ? "S-" : "",
          key.modifiers & TERMKEY_KEYMOD_ALT   ? "A-" : "",
          key.modifiers & TERMKEY_KEYMOD_CTRL  ? "C-" : "",
          key.utf8,
          key.code);

    if(key.modifiers & TERMKEY_KEYMOD_CTRL && key.code == 'C')
      break;
  }

  termios.c_lflag = old_lflag;
  tcsetattr(0, TCSANOW, &termios);
}
