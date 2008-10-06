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
  // There are 32 C0 codes
  struct keyinfo c0[32];

  // There are 64 codes 0x40 - 0x7F
  struct keyinfo csi_ss3s[64];
  struct keyinfo ss3s[64];
  char ss3_kpalts[64];

  int ncsifuncs;
  struct keyinfo *csifuncs;
} termkey_csi;

static void *new_driver(termkey_t *tk)
{
  termkey_csi *csi = malloc(sizeof *csi);

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

  return csi;
}

static void free_driver(void *private)
{
  termkey_csi *csi = private;

  free(csi->csifuncs); csi->csifuncs = NULL;

  free(csi);
}

static inline void eatbytes(termkey_t *tk, size_t count)
{
  if(count >= tk->buffcount) {
    tk->buffstart = 0;
    tk->buffcount = 0;
    return;
  }

  tk->buffstart += count;
  tk->buffcount -= count;

  size_t halfsize = tk->buffsize / 2;

  if(tk->buffstart > halfsize) {
    memcpy(tk->buffer, tk->buffer + halfsize, halfsize);
    tk->buffstart -= halfsize;
  }
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
    eatbytes(tk, introlen);
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

  eatbytes(tk, csi_end + 1);

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
    eatbytes(tk, tk->buffcount);
    return TERMKEY_RES_KEY;
  }

  unsigned char cmd = CHARAT(introlen);

  eatbytes(tk, introlen + 1);

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
      eatbytes(tk, 1);
      return TERMKEY_RES_KEY;
    }

    unsigned char b1 = CHARAT(1);

    if(b1 == '[')
      return getkey_csi(tk, 2, key);

    if(b1 == 'O')
      return getkey_ss3(tk, 2, key);

    if(b1 == 0x1b) {
      do_codepoint(tk, b0, key);
      eatbytes(tk, 1);
      return TERMKEY_RES_KEY;
    }

    tk->buffstart++;

    termkey_result metakey_result = termkey_getkey(tk, key);

    switch(metakey_result) {
      case TERMKEY_RES_KEY:
        key->modifiers |= TERMKEY_KEYMOD_ALT;
        tk->buffstart--;
        eatbytes(tk, 1);
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
    eatbytes(tk, 1);
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
      eatbytes(tk, 1);
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
      eatbytes(tk, 1);
      return TERMKEY_RES_KEY;
    }

    if(tk->buffcount < nbytes)
      return tk->waittime ? TERMKEY_RES_AGAIN : TERMKEY_RES_NONE;

    for(int b = 1; b < nbytes; b++) {
      unsigned char cb = CHARAT(b);
      if(cb < 0x80 || cb >= 0xc0) {
        do_codepoint(tk, UTF8_INVALID, key);
        eatbytes(tk, b - 1);
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
    eatbytes(tk, nbytes);
    return TERMKEY_RES_KEY;
  }
  else {
    // Non UTF-8 case - just report the raw byte
    key->type = TERMKEY_TYPE_UNICODE;
    key->code.codepoint = b0;
    key->modifiers = 0;

    key->utf8[0] = key->code.codepoint;
    key->utf8[1] = 0;

    eatbytes(tk, 1);

    return TERMKEY_RES_KEY;
  }

  return TERMKEY_SYM_NONE;
}

termkey_keysym termkey_register_c0(termkey_t *tk, termkey_keysym sym, unsigned char ctrl, const char *name)
{
  return termkey_register_c0_full(tk, sym, 0, 0, ctrl, name);
}

termkey_keysym termkey_register_c0_full(termkey_t *tk, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char ctrl, const char *name)
{
  termkey_csi *csi = tk->driver_info;

  if(ctrl >= 0x20) {
    fprintf(stderr, "Cannot register C0 key at ctrl 0x%02x - out of bounds\n", ctrl);
    return -1;
  }

  if(name)
    sym = termkey_register_keyname(tk, sym, name);

  csi->c0[ctrl].sym = sym;
  csi->c0[ctrl].modifier_set = modifier_set;
  csi->c0[ctrl].modifier_mask = modifier_mask;

  return sym;
}

termkey_keysym termkey_register_csi_ss3(termkey_t *tk, termkey_type type, termkey_keysym sym, unsigned char cmd, const char *name)
{
  return termkey_register_csi_ss3_full(tk, type, sym, 0, 0, cmd, name);
}

termkey_keysym termkey_register_csi_ss3_full(termkey_t *tk, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char cmd, const char *name)
{
  termkey_csi *csi = tk->driver_info;

  if(cmd < 0x40 || cmd >= 0x80) {
    fprintf(stderr, "Cannot register CSI/SS3 key at cmd 0x%02x - out of bounds\n", cmd);
    return -1;
  }

  if(name)
    sym = termkey_register_keyname(tk, sym, name);

  csi->csi_ss3s[cmd - 0x40].type = type;
  csi->csi_ss3s[cmd - 0x40].sym = sym;
  csi->csi_ss3s[cmd - 0x40].modifier_set = modifier_set;
  csi->csi_ss3s[cmd - 0x40].modifier_mask = modifier_mask;

  return sym;
}

termkey_keysym termkey_register_ss3kpalt(termkey_t *tk, termkey_type type, termkey_keysym sym, unsigned char cmd, const char *name, char kpalt)
{
  return termkey_register_ss3kpalt_full(tk, type, sym, 0, 0, cmd, name, kpalt);
}

termkey_keysym termkey_register_ss3kpalt_full(termkey_t *tk, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, unsigned char cmd, const char *name, char kpalt)
{
  termkey_csi *csi = tk->driver_info;

  if(cmd < 0x40 || cmd >= 0x80) {
    fprintf(stderr, "Cannot register SS3 key at cmd 0x%02x - out of bounds\n", cmd);
    return -1;
  }

  if(name)
    sym = termkey_register_keyname(tk, sym, name);

  csi->ss3s[cmd - 0x40].type = type;
  csi->ss3s[cmd - 0x40].sym = sym;
  csi->ss3s[cmd - 0x40].modifier_set = modifier_set;
  csi->ss3s[cmd - 0x40].modifier_mask = modifier_mask;
  csi->ss3_kpalts[cmd - 0x40] = kpalt;

  return sym;
}

termkey_keysym termkey_register_csifunc(termkey_t *tk, termkey_type type, termkey_keysym sym, int number, const char *name)
{
  return termkey_register_csifunc_full(tk, type, sym, 0, 0, number, name);
}

termkey_keysym termkey_register_csifunc_full(termkey_t *tk, termkey_type type, termkey_keysym sym, int modifier_set, int modifier_mask, int number, const char *name)
{
  termkey_csi *csi = tk->driver_info;

  if(name)
    sym = termkey_register_keyname(tk, sym, name);

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
