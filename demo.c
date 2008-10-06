#include <stdio.h>

#include "termkey.h"

int main(int argc, char *argv[]) {
  char buffer[50];
  termkey_t *tk = termkey_new(0, 0);

  termkey_result ret;
  termkey_key key;

  while((ret = termkey_waitkey(tk, &key)) != TERMKEY_RES_EOF) {
    termkey_snprint_key(tk, buffer, sizeof buffer, &key, 0);
    printf("%s or ", buffer);
    termkey_snprint_key(tk, buffer, sizeof buffer, &key, ~0);
    printf("%s\n", buffer);

    if(key.type == TERMKEY_TYPE_UNICODE && 
       key.modifiers & TERMKEY_KEYMOD_CTRL &&
       (key.code.codepoint == 'C' || key.code.codepoint == 'c'))
      break;
  }

  termkey_destroy(tk);
}
