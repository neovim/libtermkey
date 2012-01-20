#include "termkey.h"
#include "termkey-internal.h"

#include <stdio.h>
#include <string.h>

// There are 64 codes 0x40 - 0x7F
int keyinfo_initialised = 0;
struct keyinfo csi_ss3s[64];
struct keyinfo ss3s[64];
char ss3_kpalts[64];
struct keyinfo csifuncs[35]; /* This value must be increased if more CSI function keys are added */
#define NCSIFUNCS (sizeof(csifuncs)/sizeof(csifuncs[0]))

typedef struct {
  TermKey *tk;
} TermKeyCsi;

static void register_csi_ss3_full(TermKeyType type, TermKeySym sym, int modifier_set, int modifier_mask, unsigned char cmd)
{
  if(cmd < 0x40 || cmd >= 0x80) {
    fprintf(stderr, "Cannot register CSI/SS3 key at cmd 0x%02x - out of bounds\n", cmd);
    return;
  }

  csi_ss3s[cmd - 0x40].type = type;
  csi_ss3s[cmd - 0x40].sym = sym;
  csi_ss3s[cmd - 0x40].modifier_set = modifier_set;
  csi_ss3s[cmd - 0x40].modifier_mask = modifier_mask;
}

static void register_csi_ss3(TermKeyType type, TermKeySym sym, unsigned char cmd)
{
  register_csi_ss3_full(type, sym, 0, 0, cmd);
}

static void register_ss3kpalt(TermKeyType type, TermKeySym sym, unsigned char cmd, char kpalt)
{
  if(cmd < 0x40 || cmd >= 0x80) {
    fprintf(stderr, "Cannot register SS3 key at cmd 0x%02x - out of bounds\n", cmd);
    return;
  }

  ss3s[cmd - 0x40].type = type;
  ss3s[cmd - 0x40].sym = sym;
  ss3s[cmd - 0x40].modifier_set = 0;
  ss3s[cmd - 0x40].modifier_mask = 0;
  ss3_kpalts[cmd - 0x40] = kpalt;
}

static void register_csifunc(TermKeyType type, TermKeySym sym, int number)
{
  if(number >= NCSIFUNCS) {
    fprintf(stderr, "Cannot register CSI function key at number %d - out of bounds\n", number);
    return;
  }

  csifuncs[number].type = type;
  csifuncs[number].sym = sym;
  csifuncs[number].modifier_set = 0;
  csifuncs[number].modifier_mask = 0;
}

static int register_keys(void)
{
  int i;

  for(i = 0; i < 64; i++) {
    csi_ss3s[i].sym = TERMKEY_SYM_UNKNOWN;
    ss3s[i].sym     = TERMKEY_SYM_UNKNOWN;
    ss3_kpalts[i] = 0;
  }

  for(i = 0; i < NCSIFUNCS; i++)
    csifuncs[i].sym = TERMKEY_SYM_UNKNOWN;

  register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_UP,    'A');
  register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DOWN,  'B');
  register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RIGHT, 'C');
  register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_LEFT,  'D');
  register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BEGIN, 'E');
  register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,   'F');
  register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,  'H');
  register_csi_ss3(TERMKEY_TYPE_FUNCTION, 1, 'P');
  register_csi_ss3(TERMKEY_TYPE_FUNCTION, 2, 'Q');
  register_csi_ss3(TERMKEY_TYPE_FUNCTION, 3, 'R');
  register_csi_ss3(TERMKEY_TYPE_FUNCTION, 4, 'S');

  register_csi_ss3_full(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_TAB, TERMKEY_KEYMOD_SHIFT, TERMKEY_KEYMOD_SHIFT, 'Z');

  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPENTER,  'M', 0);
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPEQUALS, 'X', '=');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMULT,   'j', '*');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPLUS,   'k', '+');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPCOMMA,  'l', ',');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMINUS,  'm', '-');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPERIOD, 'n', '.');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPDIV,    'o', '/');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP0,      'p', '0');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP1,      'q', '1');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP2,      'r', '2');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP3,      's', '3');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP4,      't', '4');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP5,      'u', '5');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP6,      'v', '6');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP7,      'w', '7');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP8,      'x', '8');
  register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP9,      'y', '9');

  register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_FIND,      1);
  register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_INSERT,    2);
  register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DELETE,    3);
  register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SELECT,    4);
  register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEUP,    5);
  register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEDOWN,  6);
  register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,      7);
  register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,       8);

  register_csifunc(TERMKEY_TYPE_FUNCTION, 1,  11);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 2,  12);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 3,  13);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 4,  14);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 5,  15);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 6,  17);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 7,  18);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 8,  19);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 9,  20);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 10, 21);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 11, 23);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 12, 24);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 13, 25);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 14, 26);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 15, 28);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 16, 29);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 17, 31);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 18, 32);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 19, 33);
  register_csifunc(TERMKEY_TYPE_FUNCTION, 20, 34);

  keyinfo_initialised = 1;
  return 1;
}

static void *new_driver(TermKey *tk, const char *term)
{
  if(strncmp(term, "xterm", 5) == 0) {
    // We want "xterm" or "xtermc" or "xterm-..."
    if(term[5] != 0 && term[5] != '-' && term[5] != 'c')
      return NULL;
  }
  else if(strcmp(term, "screen") == 0) {
    /* Also apply for screen, because it might be transporting xterm-like
     * sequences. Yes, this sucks. We shouldn't need to rely on this behaviour
     * but there's no other way to know, and if we don't then we won't
     * recognise its sequences.
     */
  }
  else
    return NULL;

  // Excellent - we'll continue

  if(!keyinfo_initialised)
    if(!register_keys())
      return NULL;

  TermKeyCsi *csi = malloc(sizeof *csi);
  if(!csi)
    return NULL;

  csi->tk = tk;

  return csi;
}

static void free_driver(void *info)
{
  TermKeyCsi *csi = info;

  free(csi);
}

#define CHARAT(i) (tk->buffer[tk->buffstart + (i)])

static TermKeyResult peekkey_csi(TermKey *tk, TermKeyCsi *csi, size_t introlen, TermKeyKey *key, int force, size_t *nbytep)
{
  size_t csi_end = introlen;

  while(csi_end < tk->buffcount) {
    if(CHARAT(csi_end) >= 0x40 && CHARAT(csi_end) < 0x80)
      break;
    csi_end++;
  }

  if(csi_end >= tk->buffcount) {
    if(!force)
      return TERMKEY_RES_AGAIN;

    (*tk->method.emit_codepoint)(tk, '[', key);
    key->modifiers |= TERMKEY_KEYMOD_ALT;
    *nbytep = introlen;
    return TERMKEY_RES_KEY;
  }

  unsigned char cmd = CHARAT(csi_end);
  long arg[16];
  char present = 0;
  int args = 0;

  size_t p = introlen;

  // Now attempt to parse out up number;number;... separated values
  while(p < csi_end) {
    unsigned char c = CHARAT(p);

    if(c >= '0' && c <= '9') {
      if(!present) {
        arg[args] = c - '0';
        present = 1;
      }
      else {
        arg[args] = (arg[args] * 10) + c - '0';
      }
    }
    else if(c == ';') {
      if(!present)
        arg[args] = -1;
      present = 0;
      args++;

      if(args > 16)
        break;
    }

    p++;
  }

  if(!present)
    arg[args] = -1;

  args++;

  if(args > 1 && arg[1] != -1)
    key->modifiers = arg[1] - 1;
  else
    key->modifiers = 0;

  if(cmd == '~') {
    key->type = TERMKEY_TYPE_KEYSYM;

    if(arg[0] == 27) {
      int mod = key->modifiers;
      (*tk->method.emit_codepoint)(tk, arg[2], key);
      key->modifiers |= mod;
    }
    else if(arg[0] >= 0 && arg[0] < NCSIFUNCS) {
      key->type = csifuncs[arg[0]].type;
      key->code.sym = csifuncs[arg[0]].sym;
      key->modifiers &= ~(csifuncs[arg[0]].modifier_mask);
      key->modifiers |= csifuncs[arg[0]].modifier_set;
    }
    else
      key->code.sym = TERMKEY_SYM_UNKNOWN;

    if(key->code.sym == TERMKEY_SYM_UNKNOWN) {
#ifdef DEBUG
      fprintf(stderr, "CSI: Unknown function key %ld\n", arg[0]);
#endif
      return TERMKEY_RES_NONE;
    }
  }
  else if(cmd == 'u') {
    int mod = key->modifiers;
    key->type = TERMKEY_TYPE_KEYSYM;
    (*tk->method.emit_codepoint)(tk, arg[0], key);
    key->modifiers |= mod;
  }
  else if(cmd == 'M') {
    size_t csi_len = csi_end + 1;

    tk->buffstart += csi_len;
    tk->buffcount -= csi_len;

    TermKeyResult mouse_result = (*tk->method.peekkey_mouse)(tk, key, nbytep);

    tk->buffstart -= csi_len;
    tk->buffcount += csi_len;

    if(mouse_result == TERMKEY_RES_KEY)
      *nbytep += csi_len;

    return mouse_result;
  }
  else {
    // We know from the logic above that cmd must be >= 0x40 and < 0x80
    key->type = csi_ss3s[cmd - 0x40].type;
    key->code.sym = csi_ss3s[cmd - 0x40].sym;
    key->modifiers &= ~(csi_ss3s[cmd - 0x40].modifier_mask);
    key->modifiers |= csi_ss3s[cmd - 0x40].modifier_set;

    if(key->code.sym == TERMKEY_SYM_UNKNOWN) {
#ifdef DEBUG
      switch(args) {
      case 1:
        fprintf(stderr, "CSI: Unknown arg1=%ld cmd=%c\n", arg[0], (char)cmd);
        break;
      case 2:
        fprintf(stderr, "CSI: Unknown arg1=%ld arg2=%ld cmd=%c\n", arg[0], arg[1], (char)cmd);
        break;
      case 3:
        fprintf(stderr, "CSI: Unknown arg1=%ld arg2=%ld arg3=%ld cmd=%c\n", arg[0], arg[1], arg[2], (char)cmd);
        break;
      default:
        fprintf(stderr, "CSI: Unknown arg1=%ld arg2=%ld arg3=%ld ... args=%d cmd=%c\n", arg[0], arg[1], arg[2], args, (char)cmd);
        break;
      }
#endif
      return TERMKEY_RES_NONE;
    }
  }

  *nbytep = csi_end + 1;

  return TERMKEY_RES_KEY;
}

static TermKeyResult peekkey_ss3(TermKey *tk, TermKeyCsi *csi, size_t introlen, TermKeyKey *key, int force, size_t *nbytep)
{
  if(tk->buffcount < introlen + 1) {
    if(!force)
      return TERMKEY_RES_AGAIN;

    (*tk->method.emit_codepoint)(tk, 'O', key);
    key->modifiers |= TERMKEY_KEYMOD_ALT;
    *nbytep = tk->buffcount;
    return TERMKEY_RES_KEY;
  }

  unsigned char cmd = CHARAT(introlen);

  if(cmd < 0x40 || cmd >= 0x80)
    return TERMKEY_RES_NONE;

  key->type = csi_ss3s[cmd - 0x40].type;
  key->code.sym = csi_ss3s[cmd - 0x40].sym;
  key->modifiers = csi_ss3s[cmd - 0x40].modifier_set;

  if(key->code.sym == TERMKEY_SYM_UNKNOWN) {
    if(tk->flags & TERMKEY_FLAG_CONVERTKP && ss3_kpalts[cmd - 0x40]) {
      key->type = TERMKEY_TYPE_UNICODE;
      key->code.codepoint = ss3_kpalts[cmd - 0x40];
      key->modifiers = 0;

      key->utf8[0] = key->code.codepoint;
      key->utf8[1] = 0;
    }
    else {
      key->type = ss3s[cmd - 0x40].type;
      key->code.sym = ss3s[cmd - 0x40].sym;
      key->modifiers = ss3s[cmd - 0x40].modifier_set;
    }
  }

  if(key->code.sym == TERMKEY_SYM_UNKNOWN) {
#ifdef DEBUG
    fprintf(stderr, "CSI: Unknown SS3 %c (0x%02x)\n", (char)cmd, cmd);
#endif
    return TERMKEY_RES_NONE;
  }

  *nbytep = introlen + 1;

  return TERMKEY_RES_KEY;
}

static TermKeyResult peekkey(TermKey *tk, void *info, TermKeyKey *key, int force, size_t *nbytep)
{
  if(tk->buffcount == 0)
    return tk->is_closed ? TERMKEY_RES_EOF : TERMKEY_RES_NONE;

  TermKeyCsi *csi = info;

  // Now we're sure at least 1 byte is valid
  unsigned char b0 = CHARAT(0);

  if(b0 == 0x1b && tk->buffcount > 1 && CHARAT(1) == '[') {
    return peekkey_csi(tk, csi, 2, key, force, nbytep);
  }
  else if(b0 == 0x1b && tk->buffcount > 1 && CHARAT(1) == 'O') {
    return peekkey_ss3(tk, csi, 2, key, force, nbytep);
  }
  else if(b0 == 0x8f) {
    return peekkey_ss3(tk, csi, 1, key, force, nbytep);
  }
  else if(b0 == 0x9b) {
    return peekkey_csi(tk, csi, 1, key, force, nbytep);
  }
  else
    return TERMKEY_RES_NONE;
}

struct TermKeyDriver termkey_driver_csi = {
  .name        = "CSI",

  .new_driver  = new_driver,
  .free_driver = free_driver,

  .peekkey = peekkey,
};
