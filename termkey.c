#include "termkey.h"

#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>

struct keyinfo {
  termkey_keysym sym;
  int modifier_mask;
  int modifier_set;
};

struct termkey {
  int    fd;
  int    flags;
  unsigned char *buffer;
  size_t buffstart; // First offset in buffer
  size_t buffcount; // NUMBER of entires valid in buffer
  size_t buffsize; // Total malloc'ed size

  int waittime; // msec

  char   is_closed;

  int  nkeynames;
  const char **keynames;

  // There are 32 C0 codes
  struct keyinfo c0[32];

  // There are 64 codes 0x40 - 0x7F
  struct keyinfo csi_ss3s[64];
  struct keyinfo ss3s[64];
  char ss3_kpalts[64];

  int ncsifuncs;
  struct keyinfo *csifuncs;
};

termkey_t *termkey_new_full(int fd, int flags, size_t buffsize, int waittime)
{
  termkey_t *tk = malloc(sizeof(*tk));
  if(!tk)
    return NULL;

  if(!(flags & (TERMKEY_FLAG_RAW|TERMKEY_FLAG_UTF8))) {
    int locale_is_utf8 = 0;
    char *e;

    if((e = getenv("LANG")) && strstr(e, "UTF-8"))
      locale_is_utf8 = 1;

    if(!locale_is_utf8 && (e = getenv("LC_MESSAGES")) && strstr(e, "UTF-8"))
      locale_is_utf8 = 1;

    if(!locale_is_utf8 && (e = getenv("LC_ALL")) && strstr(e, "UTF-8"))
      locale_is_utf8 = 1;

    if(locale_is_utf8)
      flags |= TERMKEY_FLAG_UTF8;
    else
      flags |= TERMKEY_FLAG_RAW;
  }

  tk->fd    = fd;
  tk->flags = flags;

  tk->buffer = malloc(buffsize);
  if(!tk->buffer) {
    free(tk);
    return NULL;
  }

  tk->buffstart = 0;
  tk->buffcount = 0;
  tk->buffsize  = buffsize;

  tk->waittime = waittime;

  tk->is_closed = 0;

  int i;

  for(i = 0; i < 32; i++) {
    tk->c0[i].sym = TERMKEY_SYM_UNKNOWN;
  }

  for(i = 0; i < 64; i++) {
    tk->csi_ss3s[i].sym = TERMKEY_SYM_UNKNOWN;
    tk->ss3s[i].sym     = TERMKEY_SYM_UNKNOWN;
    tk->ss3_kpalts[i] = 0;
  }

  // These numbers aren't too important; buffers will be grown if insufficient
  tk->nkeynames = 64;
  tk->ncsifuncs = 32;

  tk->keynames = malloc(sizeof(tk->keynames[0]) * tk->nkeynames);
  tk->csifuncs = malloc(sizeof(tk->csifuncs[0]) * tk->ncsifuncs);

  for(i = 0; i < tk->nkeynames; i++)
    tk->keynames[i] = NULL;

  for(i = 0; i < tk->ncsifuncs; i++)
    tk->csifuncs[i].sym = TERMKEY_SYM_NONE;

  // Special built-in names
  termkey_register_keyname(tk, TERMKEY_SYM_NONE, "NONE");

  termkey_register_c0(tk, TERMKEY_SYM_BACKSPACE, 0x08, "Backspace");
  termkey_register_c0(tk, TERMKEY_SYM_TAB,       0x09, "Tab");
  termkey_register_c0(tk, TERMKEY_SYM_ENTER,     0x0a, "Enter");
  termkey_register_c0(tk, TERMKEY_SYM_ENTER,     0x0d, NULL);
  termkey_register_c0(tk, TERMKEY_SYM_ESCAPE,    0x1b, "Escape");

  // G1
  termkey_register_keyname(tk, TERMKEY_SYM_SPACE, "Space");
  termkey_register_keyname(tk, TERMKEY_SYM_DEL,   "DEL");

  termkey_register_csi_ss3(tk, TERMKEY_SYM_UP,    'A', "Up");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_DOWN,  'B', "Down");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_RIGHT, 'C', "Right");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_LEFT,  'D', "Left");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_BEGIN, 'E', "Begin");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_END,   'F', "End");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_HOME,  'H', "Home");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_F1,    'P', "F1");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_F2,    'Q', "F2");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_F3,    'R', "F3");
  termkey_register_csi_ss3(tk, TERMKEY_SYM_F4,    'S', "F4");

  termkey_register_csi_ss3_full(tk, TERMKEY_SYM_TAB, TERMKEY_KEYMOD_SHIFT, TERMKEY_KEYMOD_SHIFT, 'Z', NULL);

  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KPENTER,  'M', "KPEnter",  0);
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KPEQUALS, 'X', "KPEquals", '=');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KPMULT,   'j', "KPMult",   '*');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KPPLUS,   'k', "KPPlus",   '+');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KPCOMMA,  'l', "KPComma",  ',');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KPMINUS,  'm', "KPMinus",  '-');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KPPERIOD, 'n', "KPPeriod", '.');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KPDIV,    'o', "KPDiv",    '/');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP0,      'p', "KP0",      '0');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP1,      'q', "KP1",      '1');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP2,      'r', "KP2",      '2');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP3,      's', "KP3",      '3');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP4,      't', "KP4",      '4');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP5,      'u', "KP5",      '5');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP6,      'v', "KP6",      '6');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP7,      'w', "KP7",      '7');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP8,      'x', "KP8",      '8');
  termkey_register_ss3kpalt(tk, TERMKEY_SYM_KP9,      'y', "KP9",      '9');

  termkey_register_csifunc(tk, TERMKEY_SYM_HOME,      1, "Home");
  termkey_register_csifunc(tk, TERMKEY_SYM_INSERT,    2, "Insert");
  termkey_register_csifunc(tk, TERMKEY_SYM_DELETE,    3, "Delete");
  termkey_register_csifunc(tk, TERMKEY_SYM_END,       4, "End");
  termkey_register_csifunc(tk, TERMKEY_SYM_PAGEUP,    5, "PageUp");
  termkey_register_csifunc(tk, TERMKEY_SYM_PAGEDOWN,  6, "PageDown");
  termkey_register_csifunc(tk, TERMKEY_SYM_F5,       15, "F5");
  termkey_register_csifunc(tk, TERMKEY_SYM_F6,       17, "F6");
  termkey_register_csifunc(tk, TERMKEY_SYM_F7,       18, "F7");
  termkey_register_csifunc(tk, TERMKEY_SYM_F8,       19, "F8");
  termkey_register_csifunc(tk, TERMKEY_SYM_F9,       20, "F9");
  termkey_register_csifunc(tk, TERMKEY_SYM_F10,      21, "F10");
  termkey_register_csifunc(tk, TERMKEY_SYM_F11,      23, "F11");
  termkey_register_csifunc(tk, TERMKEY_SYM_F12,      24, "F12");

  return tk;
}

termkey_t *termkey_new(int fd, int flags)
{
  return termkey_new_full(fd, flags, 256, 50);
}

void termkey_setwaittime(termkey_t *tk, int msec)
{
  tk->waittime = msec;
}

int termkey_getwaittime(termkey_t *tk)
{
  return tk->waittime;
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
  int codepoint = key->code;
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
  if(codepoint < 0x20) {
    // C0 range
    key->code = 0;
    key->modifiers = 0;

    if(!(tk->flags & TERMKEY_FLAG_NOINTERPRET) && tk->c0[codepoint].sym != TERMKEY_SYM_UNKNOWN) {
      key->code = tk->c0[codepoint].sym;
      key->modifiers |= tk->c0[codepoint].modifier_set;
    }

    if(!key->code) {
      key->code = codepoint + 0x40;
      key->modifiers = TERMKEY_KEYMOD_CTRL;
      key->flags = 0;
    }
    else {
      key->flags = TERMKEY_KEYFLAG_SPECIAL;
    }
  }
  else if(codepoint == 0x20 && !(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
    // ASCII space
    key->code = TERMKEY_SYM_SPACE;
    key->modifiers = 0;
    key->flags = TERMKEY_KEYFLAG_SPECIAL;
  }
  else if(codepoint == 0x7f && !(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
    // ASCII DEL
    key->code = TERMKEY_SYM_DEL;
    key->modifiers = 0;
    key->flags = TERMKEY_KEYFLAG_SPECIAL;
  }
  else if(codepoint >= 0x20 && codepoint < 0x80) {
    // ASCII lowbyte range
    key->code = codepoint;
    key->modifiers = 0;
    key->flags = 0;
  }
  else if(codepoint >= 0x80 && codepoint < 0xa0) {
    // UTF-8 never starts with a C1 byte. So we can be sure of these
    key->code = codepoint - 0x40;
    key->modifiers = TERMKEY_KEYMOD_CTRL|TERMKEY_KEYMOD_ALT;
    key->flags = 0;
  }
  else {
    // UTF-8 codepoint
    key->code = codepoint;
    key->flags = 0;
    key->modifiers = 0;
  }

  if(!(key->flags & TERMKEY_KEYFLAG_SPECIAL))
    fill_utf8(key);
}

#define CHARAT(i) (tk->buffer[tk->buffstart + (i)])

static termkey_result getkey_csi(termkey_t *tk, size_t introlen, termkey_key *key)
{
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

  key->flags = TERMKEY_KEYFLAG_SPECIAL;

  if(cmd == '~') {
    if(arg[0] == 27) {
      do_codepoint(tk, arg[2], key);
    }
    else if(arg[0] >= 0 && arg[0] < tk->ncsifuncs) {
      key->code = tk->csifuncs[arg[0]].sym;
      key->modifiers &= ~(tk->csifuncs[arg[0]].modifier_mask);
      key->modifiers |= tk->csifuncs[arg[0]].modifier_set;
    }
    else
      key->code = TERMKEY_SYM_UNKNOWN;

    if(key->code == TERMKEY_SYM_UNKNOWN)
      fprintf(stderr, "CSI function key %d\n", arg[0]);
  }
  else {
    // We know from the logic above that cmd must be >= 0x40 and < 0x80
    key->code = tk->csi_ss3s[cmd - 0x40].sym;
    key->modifiers &= ~(tk->csi_ss3s[cmd - 0x40].modifier_mask);
    key->modifiers |= tk->csi_ss3s[cmd - 0x40].modifier_set;

    if(key->code == TERMKEY_SYM_UNKNOWN)
      fprintf(stderr, "CSI arg1=%d arg2=%d cmd=%c\n", arg[0], arg[1], cmd);
  }

  return TERMKEY_RES_KEY;
}

static termkey_result getkey_ss3(termkey_t *tk, size_t introlen, termkey_key *key)
{
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

  key->code = tk->csi_ss3s[cmd - 0x40].sym;
  key->modifiers = tk->csi_ss3s[cmd - 0x40].modifier_set;

  if(key->code == TERMKEY_SYM_UNKNOWN) {
    if(tk->flags & TERMKEY_FLAG_CONVERTKP && tk->ss3_kpalts[cmd - 0x40]) {
      key->code = tk->ss3_kpalts[cmd - 0x40];
      key->modifiers = 0;
      key->flags = 0;

      key->utf8[0] = key->code;
      key->utf8[1] = 0;

      return TERMKEY_RES_KEY;
    }

    key->code = tk->ss3s[cmd - 0x40].sym;
    key->modifiers = tk->ss3s[cmd - 0x40].modifier_set;
  }

  if(key->code == TERMKEY_SYM_UNKNOWN)
    fprintf(stderr, "SS3 %c (0x%02x)\n", cmd, cmd);

  key->flags = TERMKEY_KEYFLAG_SPECIAL;

  return TERMKEY_RES_KEY;
}

termkey_result termkey_getkey(termkey_t *tk, termkey_key *key)
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

    key->flags = 0;
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
    key->code = b0;
    key->modifiers = 0;
    key->flags = 0;

    key->utf8[0] = key->code;
    key->utf8[1] = 0;

    eatbytes(tk, 1);

    return TERMKEY_RES_KEY;
  }

  return TERMKEY_SYM_NONE;
}

termkey_result termkey_getkey_force(termkey_t *tk, termkey_key *key)
{
  int old_waittime = tk->waittime;
  tk->waittime = 0;

  termkey_result ret = termkey_getkey(tk, key);

  tk->waittime = old_waittime;

  return ret;
}

termkey_result termkey_waitkey(termkey_t *tk, termkey_key *key)
{
  while(1) {
    termkey_result ret = termkey_getkey(tk, key);

    switch(ret) {
      case TERMKEY_RES_KEY:
      case TERMKEY_RES_EOF:
        return ret;

      case TERMKEY_RES_NONE:
        termkey_advisereadable(tk);
        break;

      case TERMKEY_RES_AGAIN:
        {
          struct pollfd fd;

          fd.fd = tk->fd;
          fd.events = POLLIN;

          int pollres = poll(&fd, 1, tk->waittime);

          if(pollres == 0)
            return termkey_getkey_force(tk, key);

          termkey_advisereadable(tk);
        }
        break;
    }
  }

  /* UNREACHABLE */
}

void termkey_pushinput(termkey_t *tk, unsigned char *input, size_t inputlen)
{
  if(tk->buffstart + tk->buffcount + inputlen > tk->buffsize) {
    fprintf(stderr, "TODO! Extend input buffer!\n");
    exit(0);
  }

  // Not strcpy just in case of NUL bytes
  memcpy(tk->buffer + tk->buffstart + tk->buffcount, input, inputlen);
  tk->buffcount += inputlen;
}

termkey_result termkey_advisereadable(termkey_t *tk)
{
  unsigned char buffer[64]; // Smaller than the default size
  size_t len = read(tk->fd, buffer, sizeof buffer);

  if(len == -1 && errno == EAGAIN)
    return TERMKEY_RES_NONE;
  else if(len < 1) {
    tk->is_closed = 1;
    return TERMKEY_RES_NONE;
  }
  else {
    termkey_pushinput(tk, buffer, len);
    return TERMKEY_RES_AGAIN;
  }
}

const char *termkey_describe_sym(termkey_t *tk, termkey_keysym code)
{
  if(code == TERMKEY_SYM_UNKNOWN)
    return "UNKNOWN";

  if(code < tk->nkeynames)
    return tk->keynames[code];

  return "UNKNOWN";
}

termkey_keysym termkey_register_keyname(termkey_t *tk, termkey_keysym code, const char *name)
{
  if(!code)
    code = tk->nkeynames;

  if(code >= tk->nkeynames) {
    const char **new_keynames = realloc(tk->keynames, sizeof(new_keynames[0]) * (code + 1));
    tk->keynames = new_keynames;

    // Fill in the hole
    for(int i = tk->nkeynames; i < code; i++)
      tk->keynames[i] = NULL;

    tk->nkeynames = code + 1;
  }

  tk->keynames[code] = name;

  return code;
}

termkey_keysym termkey_register_c0(termkey_t *tk, termkey_keysym code, unsigned char ctrl, const char *name)
{
  return termkey_register_c0_full(tk, code, 0, 0, ctrl, name);
}

termkey_keysym termkey_register_c0_full(termkey_t *tk, termkey_keysym code, int modifier_set, int modifier_mask, unsigned char ctrl, const char *name)
{
  if(ctrl >= 0x20) {
    fprintf(stderr, "Cannot register C0 key at ctrl 0x%02x - out of bounds\n", ctrl);
    return -1;
  }

  if(name)
    code = termkey_register_keyname(tk, code, name);

  tk->c0[ctrl].sym = code;
  tk->c0[ctrl].modifier_set = modifier_set;
  tk->c0[ctrl].modifier_mask = modifier_mask;

  return code;
}

termkey_keysym termkey_register_csi_ss3(termkey_t *tk, termkey_keysym code, unsigned char cmd, const char *name)
{
  return termkey_register_csi_ss3_full(tk, code, 0, 0, cmd, name);
}

termkey_keysym termkey_register_csi_ss3_full(termkey_t *tk, termkey_keysym code, int modifier_set, int modifier_mask, unsigned char cmd, const char *name)
{
  if(cmd < 0x40 || cmd >= 0x80) {
    fprintf(stderr, "Cannot register CSI/SS3 key at cmd 0x%02x - out of bounds\n", cmd);
    return -1;
  }

  if(name)
    code = termkey_register_keyname(tk, code, name);

  tk->csi_ss3s[cmd - 0x40].sym = code;
  tk->csi_ss3s[cmd - 0x40].modifier_set = modifier_set;
  tk->csi_ss3s[cmd - 0x40].modifier_mask = modifier_mask;

  return code;
}

termkey_keysym termkey_register_ss3kpalt(termkey_t *tk, termkey_keysym code, unsigned char cmd, const char *name, char kpalt)
{
  return termkey_register_ss3kpalt_full(tk, code, 0, 0, cmd, name, kpalt);
}

termkey_keysym termkey_register_ss3kpalt_full(termkey_t *tk, termkey_keysym code, int modifier_set, int modifier_mask, unsigned char cmd, const char *name, char kpalt)
{
  if(cmd < 0x40 || cmd >= 0x80) {
    fprintf(stderr, "Cannot register SS3 key at cmd 0x%02x - out of bounds\n", cmd);
    return -1;
  }

  if(name)
    code = termkey_register_keyname(tk, code, name);

  tk->ss3s[cmd - 0x40].sym = code;
  tk->ss3s[cmd - 0x40].modifier_set = modifier_set;
  tk->ss3s[cmd - 0x40].modifier_mask = modifier_mask;
  tk->ss3_kpalts[cmd - 0x40] = kpalt;

  return code;
}

termkey_keysym termkey_register_csifunc(termkey_t *tk, termkey_keysym code, int number, const char *name)
{
  return termkey_register_csifunc_full(tk, code, 0, 0, number, name);
}

termkey_keysym termkey_register_csifunc_full(termkey_t *tk, termkey_keysym code, int modifier_set, int modifier_mask, int number, const char *name)
{
  if(name)
    code = termkey_register_keyname(tk, code, name);

  if(number >= tk->ncsifuncs) {
    struct keyinfo *new_csifuncs = realloc(tk->csifuncs, sizeof(new_csifuncs[0]) * (number + 1));
    tk->csifuncs = new_csifuncs;

    // Fill in the hole
    for(int i = tk->ncsifuncs; i < number; i++)
      tk->csifuncs[i].sym = TERMKEY_SYM_UNKNOWN;

    tk->ncsifuncs = number + 1;
  }

  tk->csifuncs[number].sym = code;
  tk->csifuncs[number].modifier_set = modifier_set;
  tk->csifuncs[number].modifier_mask = modifier_mask;

  return code;
}
