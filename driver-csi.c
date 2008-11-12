#include "termkey.h"
#include "termkey-internal.h"

#include <stdio.h>
#include <string.h>

typedef struct {
  termkey_t *tk;

  // There are 64 codes 0x40 - 0x7F
  struct keyinfo csi_ss3s[64];
  struct keyinfo ss3s[64];
  char ss3_kpalts[64];

  int ncsifuncs;
  struct keyinfo *csifuncs;
} termkey_csi;

static termkey_keysym register_csi_ss3(termkey_csi *csi, termkey_type type, termkey_keysym sym, unsigned char cmd, const char *name);
static termkey_keysym register_ss3kpalt(termkey_csi *csi, termkey_type type, termkey_keysym sym, unsigned char cmd, const char *name, char kpalt);
static termkey_keysym register_csifunc(termkey_csi *csi, termkey_type type, termkey_keysym sym, int number, const char *name);

static termkey_keysym register_csi_ss3_full(termkey_csi *csi, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char cmd, const char *name);
static termkey_keysym register_ss3kpalt_full(termkey_csi *csi, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char cmd, const char *name, char kpalt);
static termkey_keysym register_csifunc_full(termkey_csi *csi, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, int number, const char *name);

static void *new_driver(termkey_t *tk, const char *term)
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

  termkey_csi *csi = malloc(sizeof *csi);
  if(!csi)
    return NULL;

  csi->tk = tk;

  int i;

  for(i = 0; i < 64; i++) {
    csi->csi_ss3s[i].sym = TERMKEY_SYM_UNKNOWN;
    csi->ss3s[i].sym     = TERMKEY_SYM_UNKNOWN;
    csi->ss3_kpalts[i] = 0;
  }

  csi->ncsifuncs = 32;

  csi->csifuncs = malloc(sizeof(csi->csifuncs[0]) * csi->ncsifuncs);
  if(!csi->csifuncs)
    goto abort_free_csi;

  for(i = 0; i < csi->ncsifuncs; i++)
    csi->csifuncs[i].sym = TERMKEY_SYM_UNKNOWN;

  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_UP,    'A', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DOWN,  'B', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RIGHT, 'C', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_LEFT,  'D', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BEGIN, 'E', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,   'F', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,  'H', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_FUNCTION, 1, 'P', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_FUNCTION, 2, 'Q', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_FUNCTION, 3, 'R', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_FUNCTION, 4, 'S', NULL);

  register_csi_ss3_full(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_TAB, TERMKEY_KEYMOD_SHIFT, TERMKEY_KEYMOD_SHIFT, 'Z', NULL);

  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPENTER,  'M', NULL, 0);
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPEQUALS, 'X', NULL, '=');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMULT,   'j', NULL, '*');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPLUS,   'k', NULL, '+');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPCOMMA,  'l', NULL, ',');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMINUS,  'm', NULL, '-');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPERIOD, 'n', NULL, '.');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPDIV,    'o', NULL, '/');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP0,      'p', NULL, '0');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP1,      'q', NULL, '1');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP2,      'r', NULL, '2');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP3,      's', NULL, '3');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP4,      't', NULL, '4');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP5,      'u', NULL, '5');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP6,      'v', NULL, '6');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP7,      'w', NULL, '7');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP8,      'x', NULL, '8');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP9,      'y', NULL, '9');

  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_FIND,      1, NULL);
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_INSERT,    2, NULL);
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DELETE,    3, NULL);
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SELECT,    4, NULL);
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEUP,    5, NULL);
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEDOWN,  6, NULL);
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,      7, NULL);
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,       8, NULL);

  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 1,  11, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 2,  12, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 3,  13, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 4,  14, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 5,  15, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 6,  17, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 7,  18, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 8,  19, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 9,  20, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 10, 21, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 11, 23, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 12, 24, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 13, 25, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 14, 26, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 15, 28, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 16, 29, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 17, 31, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 18, 32, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 19, 33, NULL);
  register_csifunc(csi, TERMKEY_TYPE_FUNCTION, 20, 34, NULL);

  return csi;

abort_free_csi:
  free(csi);

  return NULL;
}

static void free_driver(void *info)
{
  termkey_csi *csi = info;

  free(csi->csifuncs); csi->csifuncs = NULL;

  free(csi);
}

#define CHARAT(i) (tk->buffer[tk->buffstart + (i)])

static termkey_result getkey_csi(termkey_t *tk, termkey_csi *csi, size_t introlen, termkey_key *key, int force)
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
    (*tk->method.eat_bytes)(tk, introlen);
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
    else if(arg[0] >= 0 && arg[0] < csi->ncsifuncs) {
      key->type = csi->csifuncs[arg[0]].type;
      key->code.sym = csi->csifuncs[arg[0]].sym;
      key->modifiers &= ~(csi->csifuncs[arg[0]].modifier_mask);
      key->modifiers |= csi->csifuncs[arg[0]].modifier_set;
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
  else {
    // We know from the logic above that cmd must be >= 0x40 and < 0x80
    key->type = csi->csi_ss3s[cmd - 0x40].type;
    key->code.sym = csi->csi_ss3s[cmd - 0x40].sym;
    key->modifiers &= ~(csi->csi_ss3s[cmd - 0x40].modifier_mask);
    key->modifiers |= csi->csi_ss3s[cmd - 0x40].modifier_set;

    if(key->code.sym == TERMKEY_SYM_UNKNOWN) {
#ifdef DEBUG
      switch(args) {
      case 1:
        fprintf(stderr, "CSI: Unknown arg1=%ld cmd=%c\n", arg[0], cmd);
        break;
      case 2:
        fprintf(stderr, "CSI: Unknown arg1=%ld arg2=%ld cmd=%c\n", arg[0], arg[1], cmd);
        break;
      case 3:
        fprintf(stderr, "CSI: Unknown arg1=%ld arg2=%ld arg3=%ld cmd=%c\n", arg[0], arg[1], arg[2], cmd);
        break;
      default:
        fprintf(stderr, "CSI: Unknown arg1=%ld arg2=%ld arg3=%ld ... args=%d cmd=%c\n", arg[0], arg[1], arg[2], args, cmd);
        break;
      }
#endif
      return TERMKEY_RES_NONE;
    }
  }

  (*tk->method.eat_bytes)(tk, csi_end + 1);

  return TERMKEY_RES_KEY;
}

static termkey_result getkey_ss3(termkey_t *tk, termkey_csi *csi, size_t introlen, termkey_key *key, int force)
{
  if(tk->buffcount < introlen + 1) {
    if(!force)
      return TERMKEY_RES_AGAIN;

    (*tk->method.emit_codepoint)(tk, 'O', key);
    key->modifiers |= TERMKEY_KEYMOD_ALT;
    (*tk->method.eat_bytes)(tk, tk->buffcount);
    return TERMKEY_RES_KEY;
  }

  unsigned char cmd = CHARAT(introlen);

  if(cmd < 0x40 || cmd >= 0x80)
    return TERMKEY_RES_NONE;

  key->type = csi->csi_ss3s[cmd - 0x40].type;
  key->code.sym = csi->csi_ss3s[cmd - 0x40].sym;
  key->modifiers = csi->csi_ss3s[cmd - 0x40].modifier_set;

  if(key->code.sym == TERMKEY_SYM_UNKNOWN) {
    if(tk->flags & TERMKEY_FLAG_CONVERTKP && csi->ss3_kpalts[cmd - 0x40]) {
      key->type = TERMKEY_TYPE_UNICODE;
      key->code.codepoint = csi->ss3_kpalts[cmd - 0x40];
      key->modifiers = 0;

      key->utf8[0] = key->code.codepoint;
      key->utf8[1] = 0;
    }
    else {
      key->type = csi->ss3s[cmd - 0x40].type;
      key->code.sym = csi->ss3s[cmd - 0x40].sym;
      key->modifiers = csi->ss3s[cmd - 0x40].modifier_set;
    }
  }

  if(key->code.sym == TERMKEY_SYM_UNKNOWN) {
#ifdef DEBUG
    fprintf(stderr, "CSI: Unknown SS3 %c (0x%02x)\n", cmd, cmd);
#endif
    return TERMKEY_RES_NONE;
  }

  (*tk->method.eat_bytes)(tk, introlen + 1);

  return TERMKEY_RES_KEY;
}

static termkey_result getkey(termkey_t *tk, void *info, termkey_key *key, int force)
{
  if(tk->buffcount == 0)
    return tk->is_closed ? TERMKEY_RES_EOF : TERMKEY_RES_NONE;

  termkey_csi *csi = info;

  // Now we're sure at least 1 byte is valid
  unsigned char b0 = CHARAT(0);

  if(b0 == 0x1b && tk->buffcount > 1 && CHARAT(1) == '[') {
    return getkey_csi(tk, csi, 2, key, force);
  }
  else if(b0 == 0x1b && tk->buffcount > 1 && CHARAT(1) == 'O') {
    return getkey_ss3(tk, csi, 2, key, force);
  }
  else if(b0 == 0x8f) {
    return getkey_ss3(tk, csi, 1, key, force);
  }
  else if(b0 == 0x9b) {
    return getkey_csi(tk, csi, 1, key, force);
  }
  else
    return TERMKEY_RES_NONE;
}

static termkey_keysym register_csi_ss3(termkey_csi *csi, termkey_type type, termkey_keysym sym, unsigned char cmd, const char *name)
{
  return register_csi_ss3_full(csi, type, sym, 0, 0, cmd, name);
}

static termkey_keysym register_csi_ss3_full(termkey_csi *csi, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char cmd, const char *name)
{
  if(cmd < 0x40 || cmd >= 0x80) {
    fprintf(stderr, "Cannot register CSI/SS3 key at cmd 0x%02x - out of bounds\n", cmd);
    return -1;
  }

  if(name)
    sym = termkey_register_keyname(csi->tk, sym, name);

  csi->csi_ss3s[cmd - 0x40].type = type;
  csi->csi_ss3s[cmd - 0x40].sym = sym;
  csi->csi_ss3s[cmd - 0x40].modifier_set = modifier_set;
  csi->csi_ss3s[cmd - 0x40].modifier_mask = modifier_mask;

  return sym;
}

static termkey_keysym register_ss3kpalt(termkey_csi *csi, termkey_type type, termkey_keysym sym, unsigned char cmd, const char *name, char kpalt)
{
  return register_ss3kpalt_full(csi, type, sym, 0, 0, cmd, name, kpalt);
}

static termkey_keysym register_ss3kpalt_full(termkey_csi *csi, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char cmd, const char *name, char kpalt)
{
  if(cmd < 0x40 || cmd >= 0x80) {
    fprintf(stderr, "Cannot register SS3 key at cmd 0x%02x - out of bounds\n", cmd);
    return -1;
  }

  if(name)
    sym = termkey_register_keyname(csi->tk, sym, name);

  csi->ss3s[cmd - 0x40].type = type;
  csi->ss3s[cmd - 0x40].sym = sym;
  csi->ss3s[cmd - 0x40].modifier_set = modifier_set;
  csi->ss3s[cmd - 0x40].modifier_mask = modifier_mask;
  csi->ss3_kpalts[cmd - 0x40] = kpalt;

  return sym;
}

static termkey_keysym register_csifunc(termkey_csi *csi, termkey_type type, termkey_keysym sym, int number, const char *name)
{
  return register_csifunc_full(csi, type, sym, 0, 0, number, name);
}

static termkey_keysym register_csifunc_full(termkey_csi *csi, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, int number, const char *name)
{
  if(name)
    sym = termkey_register_keyname(csi->tk, sym, name);

  if(number >= csi->ncsifuncs) {
    struct keyinfo *new_csifuncs = realloc(csi->csifuncs, sizeof(new_csifuncs[0]) * (number + 1));
    // TODO: Handle realloc() failure
    csi->csifuncs = new_csifuncs;

    // Fill in the hole
    for(int i = csi->ncsifuncs; i < number; i++)
      csi->csifuncs[i].sym = TERMKEY_SYM_UNKNOWN;

    csi->ncsifuncs = number + 1;
  }

  csi->csifuncs[number].type = type;
  csi->csifuncs[number].sym = sym;
  csi->csifuncs[number].modifier_set = modifier_set;
  csi->csifuncs[number].modifier_mask = modifier_mask;

  return sym;
}

struct termkey_driver termkey_driver_csi = {
  .name        = "CSI",

  .new_driver  = new_driver,
  .free_driver = free_driver,

  .getkey = getkey,
};
