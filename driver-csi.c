#include "termkey.h"
#include "termkey-internal.h"

#include <stdio.h>
#include <string.h>

struct keyinfo {
  termkey_type type;
  termkey_keysym sym;
  int modifier_mask;
  int modifier_set;
};

typedef struct {
  termkey_t *tk;

  // There are 32 C0 codes
  struct keyinfo c0[32];

  // There are 64 codes 0x40 - 0x7F
  struct keyinfo csi_ss3s[64];
  struct keyinfo ss3s[64];
  char ss3_kpalts[64];

  int ncsifuncs;
  struct keyinfo *csifuncs;
} termkey_csi;

static termkey_keysym register_c0(termkey_csi *csi, termkey_keysym sym, unsigned char ctrl, const char *name);
static termkey_keysym register_csi_ss3(termkey_csi *csi, termkey_type type, termkey_keysym sym, unsigned char cmd, const char *name);
static termkey_keysym register_ss3kpalt(termkey_csi *csi, termkey_type type, termkey_keysym sym, unsigned char cmd, const char *name, char kpalt);
static termkey_keysym register_csifunc(termkey_csi *csi, termkey_type type, termkey_keysym sym, int number, const char *name);

static termkey_keysym register_c0_full(termkey_csi *csi, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char ctrl, const char *name);
static termkey_keysym register_csi_ss3_full(termkey_csi *csi, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char cmd, const char *name);
static termkey_keysym register_ss3kpalt_full(termkey_csi *csi, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char cmd, const char *name, char kpalt);
static termkey_keysym register_csifunc_full(termkey_csi *csi, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, int number, const char *name);

static void *new_driver(termkey_t *tk)
{
  termkey_csi *csi = malloc(sizeof *csi);

  csi->tk = tk;

  int i;

  for(i = 0; i < 32; i++) {
    csi->c0[i].sym = TERMKEY_SYM_UNKNOWN;
  }

  for(i = 0; i < 64; i++) {
    csi->csi_ss3s[i].sym = TERMKEY_SYM_UNKNOWN;
    csi->ss3s[i].sym     = TERMKEY_SYM_UNKNOWN;
    csi->ss3_kpalts[i] = 0;
  }

  csi->ncsifuncs = 32;

  csi->csifuncs = malloc(sizeof(csi->csifuncs[0]) * csi->ncsifuncs);

  for(i = 0; i < csi->ncsifuncs; i++)
    csi->csifuncs[i].sym = TERMKEY_SYM_UNKNOWN;

  register_c0(csi, TERMKEY_SYM_BACKSPACE, 0x08, "Backspace");
  register_c0(csi, TERMKEY_SYM_TAB,       0x09, "Tab");
  register_c0(csi, TERMKEY_SYM_ENTER,     0x0d, "Enter");
  register_c0(csi, TERMKEY_SYM_ESCAPE,    0x1b, "Escape");

  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_UP,    'A', "Up");
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DOWN,  'B', "Down");
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RIGHT, 'C', "Right");
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_LEFT,  'D', "Left");
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BEGIN, 'E', "Begin");
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,   'F', "End");
  register_csi_ss3(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,  'H', "Home");
  register_csi_ss3(csi, TERMKEY_TYPE_FUNCTION, 1, 'P', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_FUNCTION, 2, 'Q', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_FUNCTION, 3, 'R', NULL);
  register_csi_ss3(csi, TERMKEY_TYPE_FUNCTION, 4, 'S', NULL);

  register_csi_ss3_full(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_TAB, TERMKEY_KEYMOD_SHIFT, TERMKEY_KEYMOD_SHIFT, 'Z', NULL);

  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPENTER,  'M', "KPEnter",  0);
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPEQUALS, 'X', "KPEquals", '=');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMULT,   'j', "KPMult",   '*');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPLUS,   'k', "KPPlus",   '+');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPCOMMA,  'l', "KPComma",  ',');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMINUS,  'm', "KPMinus",  '-');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPERIOD, 'n', "KPPeriod", '.');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPDIV,    'o', "KPDiv",    '/');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP0,      'p', "KP0",      '0');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP1,      'q', "KP1",      '1');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP2,      'r', "KP2",      '2');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP3,      's', "KP3",      '3');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP4,      't', "KP4",      '4');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP5,      'u', "KP5",      '5');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP6,      'v', "KP6",      '6');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP7,      'w', "KP7",      '7');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP8,      'x', "KP8",      '8');
  register_ss3kpalt(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP9,      'y', "KP9",      '9');

  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_FIND,      1, "Find");
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_INSERT,    2, "Insert");
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DELETE,    3, "Delete");
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SELECT,    4, "Select");
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEUP,    5, "PageUp");
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEDOWN,  6, "PageDown");
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,      7, "Home");
  register_csifunc(csi, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,       8, "End");

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
}

static void free_driver(void *private)
{
  termkey_csi *csi = private;

  free(csi->csifuncs); csi->csifuncs = NULL;

  free(csi);
}

#define UTF8_INVALID 0xFFFD

static int utf8_seqlen(int codepoint)
{
  if(codepoint < 0x0000080) return 1;
  if(codepoint < 0x0000800) return 2;
  if(codepoint < 0x0010000) return 3;
  if(codepoint < 0x0200000) return 4;
  if(codepoint < 0x4000000) return 5;
  return 6;
}

static void fill_utf8(termkey_key *key)
{
  int codepoint = key->code.codepoint;
  int nbytes = utf8_seqlen(codepoint);

  key->utf8[nbytes] = 0;

  // This is easier done backwards
  int b = nbytes;
  while(b > 1) {
    b--;
    key->utf8[b] = 0x80 | (codepoint & 0x3f);
    codepoint >>= 6;
  }

  switch(nbytes) {
    case 1: key->utf8[0] =        (codepoint & 0x7f); break;
    case 2: key->utf8[0] = 0xc0 | (codepoint & 0x1f); break;
    case 3: key->utf8[0] = 0xe0 | (codepoint & 0x0f); break;
    case 4: key->utf8[0] = 0xf0 | (codepoint & 0x07); break;
    case 5: key->utf8[0] = 0xf8 | (codepoint & 0x03); break;
    case 6: key->utf8[0] = 0xfc | (codepoint & 0x01); break;
  }
}

static inline void do_codepoint(termkey_t *tk, int codepoint, termkey_key *key)
{
  termkey_csi *csi = tk->driver_info;

  if(codepoint < 0x20) {
    // C0 range
    key->code.codepoint = 0;
    key->modifiers = 0;

    if(!(tk->flags & TERMKEY_FLAG_NOINTERPRET) && csi->c0[codepoint].sym != TERMKEY_SYM_UNKNOWN) {
      key->code.sym = csi->c0[codepoint].sym;
      key->modifiers |= csi->c0[codepoint].modifier_set;
    }

    if(!key->code.sym) {
      key->type = TERMKEY_TYPE_UNICODE;
      key->code.codepoint = codepoint + 0x40;
      key->modifiers = TERMKEY_KEYMOD_CTRL;
    }
    else {
      key->type = TERMKEY_TYPE_KEYSYM;
    }
  }
  else if(codepoint == 0x20 && !(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
    // ASCII space
    key->type = TERMKEY_TYPE_KEYSYM;
    key->code.sym = TERMKEY_SYM_SPACE;
    key->modifiers = 0;
  }
  else if(codepoint == 0x7f && !(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
    // ASCII DEL
    key->type = TERMKEY_TYPE_KEYSYM;
    key->code.sym = TERMKEY_SYM_DEL;
    key->modifiers = 0;
  }
  else if(codepoint >= 0x20 && codepoint < 0x80) {
    // ASCII lowbyte range
    key->type = TERMKEY_TYPE_UNICODE;
    key->code.codepoint = codepoint;
    key->modifiers = 0;
  }
  else if(codepoint >= 0x80 && codepoint < 0xa0) {
    // UTF-8 never starts with a C1 byte. So we can be sure of these
    key->type = TERMKEY_TYPE_UNICODE;
    key->code.codepoint = codepoint - 0x40;
    key->modifiers = TERMKEY_KEYMOD_CTRL|TERMKEY_KEYMOD_ALT;
  }
  else {
    // UTF-8 codepoint
    key->type = TERMKEY_TYPE_UNICODE;
    key->code.codepoint = codepoint;
    key->modifiers = 0;
  }

  if(key->type == TERMKEY_TYPE_UNICODE)
    fill_utf8(key);
}

#define CHARAT(i) (tk->buffer[tk->buffstart + (i)])

static termkey_result getkey_csi(termkey_t *tk, size_t introlen, termkey_key *key)
{
  termkey_csi *csi = tk->driver_info;

  size_t csi_end = introlen;

  while(csi_end < tk->buffcount) {
    if(CHARAT(csi_end) >= 0x40 && CHARAT(csi_end) < 0x80)
      break;
    csi_end++;
  }

  if(csi_end >= tk->buffcount) {
    if(tk->waittime)
      return TERMKEY_RES_AGAIN;

    do_codepoint(tk, '[', key);
    key->modifiers |= TERMKEY_KEYMOD_ALT;
    (*tk->method.eat_bytes)(tk, introlen);
    return TERMKEY_RES_KEY;
  }

  unsigned char cmd = CHARAT(csi_end);
  int arg[16];
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

  (*tk->method.eat_bytes)(tk, csi_end + 1);

  if(args > 1 && arg[1] != -1)
    key->modifiers = arg[1] - 1;
  else
    key->modifiers = 0;

  if(cmd == '~') {
    key->type = TERMKEY_TYPE_KEYSYM;

    if(arg[0] == 27) {
      int mod = key->modifiers;
      do_codepoint(tk, arg[2], key);
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

    if(key->code.sym == TERMKEY_SYM_UNKNOWN)
      fprintf(stderr, "CSI function key %d\n", arg[0]);
  }
  else {
    // We know from the logic above that cmd must be >= 0x40 and < 0x80
    key->type = csi->csi_ss3s[cmd - 0x40].type;
    key->code.sym = csi->csi_ss3s[cmd - 0x40].sym;
    key->modifiers &= ~(csi->csi_ss3s[cmd - 0x40].modifier_mask);
    key->modifiers |= csi->csi_ss3s[cmd - 0x40].modifier_set;

    if(key->code.sym == TERMKEY_SYM_UNKNOWN)
      fprintf(stderr, "CSI arg1=%d arg2=%d cmd=%c\n", arg[0], arg[1], cmd);
  }

  return TERMKEY_RES_KEY;
}

static termkey_result getkey_ss3(termkey_t *tk, size_t introlen, termkey_key *key)
{
  termkey_csi *csi = tk->driver_info;

  if(tk->buffcount < introlen + 1) {
    if(tk->waittime)
      return TERMKEY_RES_AGAIN;

    do_codepoint(tk, 'O', key);
    key->modifiers |= TERMKEY_KEYMOD_ALT;
    (*tk->method.eat_bytes)(tk, tk->buffcount);
    return TERMKEY_RES_KEY;
  }

  unsigned char cmd = CHARAT(introlen);

  (*tk->method.eat_bytes)(tk, introlen + 1);

  if(cmd < 0x40 || cmd >= 0x80)
    return TERMKEY_SYM_UNKNOWN;

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

      return TERMKEY_RES_KEY;
    }

    key->type = csi->ss3s[cmd - 0x40].type;
    key->code.sym = csi->ss3s[cmd - 0x40].sym;
    key->modifiers = csi->ss3s[cmd - 0x40].modifier_set;
  }

  if(key->code.sym == TERMKEY_SYM_UNKNOWN)
    fprintf(stderr, "SS3 %c (0x%02x)\n", cmd, cmd);

  return TERMKEY_RES_KEY;
}

static termkey_result getkey(termkey_t *tk, termkey_key *key)
{
  if(tk->buffcount == 0)
    return tk->is_closed ? TERMKEY_RES_EOF : TERMKEY_RES_NONE;

  // Now we're sure at least 1 byte is valid
  unsigned char b0 = CHARAT(0);

  if(b0 == 0x1b) {
    if(tk->buffcount == 1) {
      // This might be an <Esc> press, or it may want to be part of a longer
      // sequence
      if(tk->waittime)
        return TERMKEY_RES_AGAIN;

      do_codepoint(tk, b0, key);
      (*tk->method.eat_bytes)(tk, 1);
      return TERMKEY_RES_KEY;
    }

    unsigned char b1 = CHARAT(1);

    if(b1 == '[')
      return getkey_csi(tk, 2, key);

    if(b1 == 'O')
      return getkey_ss3(tk, 2, key);

    if(b1 == 0x1b) {
      do_codepoint(tk, b0, key);
      (*tk->method.eat_bytes)(tk, 1);
      return TERMKEY_RES_KEY;
    }

    tk->buffstart++;

    termkey_result metakey_result = termkey_getkey(tk, key);

    switch(metakey_result) {
      case TERMKEY_RES_KEY:
        key->modifiers |= TERMKEY_KEYMOD_ALT;
        tk->buffstart--;
        (*tk->method.eat_bytes)(tk, 1);
        break;

      case TERMKEY_RES_NONE:
      case TERMKEY_RES_EOF:
      case TERMKEY_RES_AGAIN:
        break;
    }

    return metakey_result;
  }
  else if(b0 == 0x8f) {
    return getkey_ss3(tk, 1, key);
  }
  else if(b0 == 0x9b) {
    return getkey_csi(tk, 1, key);
  }
  else if(b0 < 0xa0) {
    // Single byte C0, G0 or C1 - C1 is never UTF-8 initial byte
    do_codepoint(tk, b0, key);
    (*tk->method.eat_bytes)(tk, 1);
    return TERMKEY_RES_KEY;
  }
  else if(tk->flags & TERMKEY_FLAG_UTF8) {
    // Some UTF-8
    int nbytes;
    int codepoint;

    key->type = TERMKEY_TYPE_UNICODE;
    key->modifiers = 0;

    if(b0 < 0xc0) {
      // Starts with a continuation byte - that's not right
      do_codepoint(tk, UTF8_INVALID, key);
      (*tk->method.eat_bytes)(tk, 1);
      return TERMKEY_RES_KEY;
    }
    else if(b0 < 0xe0) {
      nbytes = 2;
      codepoint = b0 & 0x1f;
    }
    else if(b0 < 0xf0) {
      nbytes = 3;
      codepoint = b0 & 0x0f;
    }
    else if(b0 < 0xf8) {
      nbytes = 4;
      codepoint = b0 & 0x07;
    }
    else if(b0 < 0xfc) {
      nbytes = 5;
      codepoint = b0 & 0x03;
    }
    else if(b0 < 0xfe) {
      nbytes = 6;
      codepoint = b0 & 0x01;
    }
    else {
      do_codepoint(tk, UTF8_INVALID, key);
      (*tk->method.eat_bytes)(tk, 1);
      return TERMKEY_RES_KEY;
    }

    if(tk->buffcount < nbytes)
      return tk->waittime ? TERMKEY_RES_AGAIN : TERMKEY_RES_NONE;

    for(int b = 1; b < nbytes; b++) {
      unsigned char cb = CHARAT(b);
      if(cb < 0x80 || cb >= 0xc0) {
        do_codepoint(tk, UTF8_INVALID, key);
        (*tk->method.eat_bytes)(tk, b - 1);
        return TERMKEY_RES_KEY;
      }

      codepoint <<= 6;
      codepoint |= cb & 0x3f;
    }

    // Check for overlong sequences
    if(nbytes > utf8_seqlen(codepoint))
      codepoint = UTF8_INVALID;

    // Check for UTF-16 surrogates or invalid codepoints
    if((codepoint >= 0xD800 && codepoint <= 0xDFFF) ||
       codepoint == 0xFFFE ||
       codepoint == 0xFFFF)
      codepoint = UTF8_INVALID;

    do_codepoint(tk, codepoint, key);
    (*tk->method.eat_bytes)(tk, nbytes);
    return TERMKEY_RES_KEY;
  }
  else {
    // Non UTF-8 case - just report the raw byte
    key->type = TERMKEY_TYPE_UNICODE;
    key->code.codepoint = b0;
    key->modifiers = 0;

    key->utf8[0] = key->code.codepoint;
    key->utf8[1] = 0;

    (*tk->method.eat_bytes)(tk, 1);

    return TERMKEY_RES_KEY;
  }

  return TERMKEY_SYM_NONE;
}

static termkey_keysym register_c0(termkey_csi *csi, termkey_keysym sym, unsigned char ctrl, const char *name)
{
  return register_c0_full(csi, sym, 0, 0, ctrl, name);
}

static termkey_keysym register_c0_full(termkey_csi *csi, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char ctrl, const char *name)
{
  if(ctrl >= 0x20) {
    fprintf(stderr, "Cannot register C0 key at ctrl 0x%02x - out of bounds\n", ctrl);
    return -1;
  }

  if(name)
    sym = termkey_register_keyname(csi->tk, sym, name);

  csi->c0[ctrl].sym = sym;
  csi->c0[ctrl].modifier_set = modifier_set;
  csi->c0[ctrl].modifier_mask = modifier_mask;

  return sym;
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
  .new_driver  = new_driver,
  .free_driver = free_driver,

  .getkey = getkey,
};
