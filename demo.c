#include <stdio.h>
#include <getopt.h>

#include "termkey.h"

int main(int argc, char *argv[])
{
  TERMKEY_CHECK_VERSION;

  int mouse = 0;
  TermKeyFormat format = TERMKEY_FORMAT_VIM;

  char buffer[50];
  TermKey *tk;

  int opt;
  while((opt = getopt(argc, argv, "m::")) != -1) {
    switch(opt) {
    case 'm':
      if(optarg)
        mouse = atoi(optarg);
      else
        mouse = 1000;
      format |= TERMKEY_FORMAT_MOUSE_POS;

      break;
    default:
      fprintf(stderr, "Usage: %s [-m]\n", argv[0]);
      return 1;
    }
  }

  tk = termkey_new(0, TERMKEY_FLAG_SPACESYMBOL);

  if(!tk) {
    fprintf(stderr, "Cannot allocate termkey instance\n");
    exit(1);
  }

  TermKeyResult ret;
  TermKeyKey key;

  if(mouse)
    printf("\e[?%dhMouse mode active\n", mouse);

  while((ret = termkey_waitkey(tk, &key)) != TERMKEY_RES_EOF) {
    termkey_strfkey(tk, buffer, sizeof buffer, &key, format);
    printf("%s\n", buffer);

    if(key.type == TERMKEY_TYPE_UNICODE && 
       key.modifiers & TERMKEY_KEYMOD_CTRL &&
       (key.code.codepoint == 'C' || key.code.codepoint == 'c'))
      break;
  }

  if(mouse)
    printf("\e[?%dlMouse mode deactivated\n", mouse);

  termkey_destroy(tk);
}
