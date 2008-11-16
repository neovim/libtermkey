#include <stdio.h>

#include "termkey.h"

int main(int argc, char *argv[]) {
  TERMKEY_CHECK_VERSION;

  char buffer[50];
  termkey_t *tk = termkey_new(0, 0);

  if(!tk) {
    fprintf(stderr, "Cannot allocate termkey instance\n");
    exit(1);
  }

  termkey_result ret;
  termkey_key key;

  while((ret = termkey_waitkey(tk, &key)) != TERMKEY_RES_EOF) {
    termkey_snprint_key(tk, buffer, sizeof buffer, &key, TERMKEY_FORMAT_VIM);
    printf("%s\n", buffer);

    if(key.type == TERMKEY_TYPE_UNICODE && 
       key.modifiers & TERMKEY_KEYMOD_CTRL &&
       (key.code.codepoint == 'C' || key.code.codepoint == 'c'))
      break;
  }

  termkey_destroy(tk);
}
