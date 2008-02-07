#include "termkey.h"

#include <unistd.h>
#include <string.h>

// Use GLib to implement for now because I am lazy. Rewrite sometime
#include <glib.h>
#include <stdio.h>

struct termkey {
  int    fd;
  int    flags;
  unsigned char *buffer;
  size_t buffvalid;
  size_t buffsize;

  char   is_closed;

  int  nkeynames;
  const char **keynames;

  int ncsi_ss3s;
  int *csi_ss3s;

  int nss3s;
  int *ss3s;
  char *ss3_kpalts;

  int ncsifuncs;
  int *csifuncs;
};

termkey_t *termkey_new_full(int fd, int flags, size_t buffsize)
{
  termkey_t *tk = g_new0(struct termkey, 1);

  tk->fd    = fd;
  tk->flags = flags;

  tk->buffer    = g_malloc0(buffsize);
  tk->buffvalid = 0;
  tk->buffsize  = buffsize;

  tk->is_closed = 0;

  // Special built-in names
  termkey_register_keyname(tk, TERMKEY_SYM_NONE, "NONE");

  // C0
  termkey_register_keyname(tk, TERMKEY_SYM_BACKSPACE, "Backspace");
  termkey_register_keyname(tk, TERMKEY_SYM_TAB,       "Tab");
  termkey_register_keyname(tk, TERMKEY_SYM_ENTER,     "Enter");
  termkey_register_keyname(tk, TERMKEY_SYM_ESCAPE,    "Escape");

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
  return termkey_new_full(fd, flags, 256);
}

static inline void eatbytes(termkey_t *tk, size_t count)
{
  memmove(tk->buffer, tk->buffer + count, tk->buffvalid - count);
  tk->buffvalid -= count;
}

static termkey_result getkey_csi(termkey_t *tk, size_t introlen, termkey_key *key)
{
  size_t csi_end = introlen;

  while(csi_end < tk->buffvalid) {
    if(tk->buffer[csi_end] >= 0x40 && tk->buffer[csi_end] < 0x80)
      break;
    csi_end++;
  }

  if(csi_end >= tk->buffvalid)
    return TERMKEY_RES_NONE;

  unsigned char cmd = tk->buffer[csi_end];
  int arg[16];
  char present = 0;
  int args = 0;

  size_t p = introlen;

  // Now attempt to parse out up number;number;... separated values
  while(p < csi_end) {
    unsigned char c = tk->buffer[p];

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

  if(cmd == '~') {
    if(arg[0] >= 0 && arg[0] < tk->ncsifuncs)
      key->code = tk->csifuncs[arg[0]];
    else
      key->code = TERMKEY_SYM_UNKNOWN;

    if(key->code == TERMKEY_SYM_UNKNOWN)
      fprintf(stderr, "CSI function key %d\n", arg[0]);
  }
  else {
    if(cmd < tk->ncsi_ss3s)
      key->code = tk->csi_ss3s[cmd];
    else
      key->code = TERMKEY_SYM_UNKNOWN;

    if(key->code == TERMKEY_SYM_UNKNOWN)
      fprintf(stderr, "CSI arg1=%d arg2=%d cmd=%c\n", arg[0], arg[1], cmd);
  }

  key->modifiers = (args > 1 && arg[1] != -1) ? arg[1] - 1 : 0;
  key->flags = TERMKEY_KEYFLAG_SPECIAL;

  return TERMKEY_RES_KEY;
}

static termkey_result getkey_ss3(termkey_t *tk, size_t introlen, termkey_key *key)
{
  if(introlen + 1 < tk->buffvalid)
    return TERMKEY_RES_NONE;

  unsigned char cmd = tk->buffer[introlen];

  eatbytes(tk, introlen + 1);

  key->code = TERMKEY_SYM_UNKNOWN;

  if(cmd < tk->ncsi_ss3s)
    key->code = tk->csi_ss3s[cmd];

  if(key->code == TERMKEY_SYM_UNKNOWN && cmd < tk->nss3s) {
    if(tk->flags & TERMKEY_FLAG_CONVERTKP && tk->ss3_kpalts[cmd]) {
      key->code = tk->ss3_kpalts[cmd];
      key->modifiers = 0;
      key->flags = 0;

      key->utf8[0] = key->code;
      key->utf8[1] = 0;

      return TERMKEY_RES_KEY;
    }

    key->code = tk->ss3s[cmd];
  }

  if(key->code == TERMKEY_SYM_UNKNOWN)
    fprintf(stderr, "SS3 %c (0x%02x)\n", cmd, cmd);

  key->modifiers = 0;
  key->flags = TERMKEY_KEYFLAG_SPECIAL;

  return TERMKEY_RES_KEY;
}

termkey_result termkey_getkey(termkey_t *tk, termkey_key *key)
{
  if(tk->buffvalid == 0)
    return tk->is_closed ? TERMKEY_RES_EOF : TERMKEY_RES_NONE;

  // Now we're sure at least 1 byte is valid
  unsigned char b0 = tk->buffer[0];

  if(b0 == 0x1b) {
    if(tk->buffvalid == 1) {
      key->code = TERMKEY_SYM_ESCAPE;
      key->modifiers = 0;
      key->flags = TERMKEY_KEYFLAG_SPECIAL;

      eatbytes(tk, 1);

      return TERMKEY_RES_KEY;
    }

    unsigned char b1 = tk->buffer[1];
    if(b1 == '[')
      return getkey_csi(tk, 2, key);

    if(b1 == 'O')
      return getkey_ss3(tk, 2, key);

    tk->buffer++;

    termkey_result metakey_result = termkey_getkey(tk, key);

    switch(metakey_result) {
      case TERMKEY_RES_KEY:
        key->modifiers |= TERMKEY_KEYMOD_ALT;
        tk->buffer--;
        eatbytes(tk, 1);
        break;

      case TERMKEY_RES_NONE:
      case TERMKEY_RES_EOF:
        break;
    }

    return metakey_result;
  }
  else if(b0 < 0x20) {
    // Control key

    key->code = 0;

    if(!(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
      // Try to interpret C0 codes that have nice names
      switch(b0) {
        case 0x08: key->code = TERMKEY_SYM_BACKSPACE; break;
        case 0x09: key->code = TERMKEY_SYM_TAB;       break;
        case 0x0a: key->code = TERMKEY_SYM_ENTER;     break;
      }
    }

    if(!key->code) {
      key->code = b0 + 0x40;
      key->modifiers = TERMKEY_KEYMOD_CTRL;
      key->flags = 0;

      key->utf8[0] = key->code;
      key->utf8[1] = 0;
    }
    else {
      key->modifiers = 0;
      key->flags = TERMKEY_KEYFLAG_SPECIAL;
    }

    eatbytes(tk, 1);

    return TERMKEY_RES_KEY;
  }
  else if(b0 == 0x20 && !(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
    key->code = TERMKEY_SYM_SPACE;
    key->modifiers = 0;
    key->flags = TERMKEY_KEYFLAG_SPECIAL;

    eatbytes(tk, 1);

    return TERMKEY_RES_KEY;
  }
  else if(b0 == 0x7f && !(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
    key->code = TERMKEY_SYM_DEL;
    key->modifiers = 0;
    key->flags = TERMKEY_KEYFLAG_SPECIAL;

    eatbytes(tk, 1);

    return TERMKEY_RES_KEY;
  }
  else if(b0 >= 0x20 && b0 < 0x80) {
    // ASCII lowbyte range
    key->code = b0;
    key->modifiers = 0;
    key->flags = 0;

    key->utf8[0] = key->code;
    key->utf8[1] = 0;

    eatbytes(tk, 1);

    return TERMKEY_RES_KEY;
  }

  fprintf(stderr, "TODO - tk->buffer[0] == 0x%02x\n", tk->buffer[0]);
  return TERMKEY_SYM_NONE;
}

termkey_result termkey_waitkey(termkey_t *tk, termkey_key *key)
{
  termkey_result ret;
  while((ret = termkey_getkey(tk, key)) == TERMKEY_RES_NONE) {
    termkey_advisereadable(tk);
  }

  return ret;
}

void termkey_pushinput(termkey_t *tk, unsigned char *input, size_t inputlen)
{
  if(tk->buffvalid + inputlen > tk->buffsize) {
    fprintf(stderr, "TODO! Extend input buffer!\n");
    exit(0);
  }

  // Not strcpy just in case of NUL bytes
  memcpy(tk->buffer + tk->buffvalid, input, inputlen);
  tk->buffvalid += inputlen;
}

void termkey_advisereadable(termkey_t *tk)
{
  unsigned char buffer[64]; // Smaller than the default size
  size_t len = read(tk->fd, buffer, sizeof buffer);

  if(len == -1)
    tk->is_closed = 1;
  else
    termkey_pushinput(tk, buffer, len);
}

const char *termkey_describe_sym(termkey_t *tk, int code)
{
  if(code == TERMKEY_SYM_UNKNOWN)
    return "UNKNOWN";

  if(code < tk->nkeynames)
    return tk->keynames[code];

  return "UNKNOWN";
}

int termkey_register_keyname(termkey_t *tk, int code, const char *name)
{
  if(!code)
    code = tk->nkeynames;

  if(code >= tk->nkeynames) {
    tk->keynames = g_renew(const char*, tk->keynames, code + 1);

    // Fill in the hole
    for(int i = tk->nkeynames; i < code; i++)
      tk->keynames[i] = NULL;

    tk->nkeynames = code + 1;
  }

  tk->keynames[code] = name;

  return code;
}

int termkey_register_csi_ss3(termkey_t *tk, int code, unsigned char cmd, const char *name)
{
  if(name)
    code = termkey_register_keyname(tk, code, name);

  if(cmd >= tk->ncsi_ss3s) {
    tk->csi_ss3s = g_renew(int, tk->csi_ss3s, cmd + 1);

    // Fill in the hole
    for(int i = tk->ncsi_ss3s; i < cmd; i++)
      tk->csi_ss3s[i] = TERMKEY_SYM_UNKNOWN;

    tk->ncsi_ss3s = cmd + 1;
  }

  tk->csi_ss3s[cmd] = code;

  return code;
}

int termkey_register_ss3kpalt(termkey_t *tk, int code, unsigned char cmd, const char *name, char kpalt)
{
  if(name)
    code = termkey_register_keyname(tk, code, name);

  if(cmd >= tk->nss3s) {
    tk->ss3s       = g_renew(int,  tk->ss3s,       cmd + 1);
    tk->ss3_kpalts = g_renew(char, tk->ss3_kpalts, cmd + 1);

    // Fill in the hole
    for(int i = tk->nss3s; i < cmd; i++) {
      tk->ss3s[i]       = TERMKEY_SYM_UNKNOWN;
      tk->ss3_kpalts[i] = 0;
    }

    tk->nss3s = cmd + 1;
  }

  tk->ss3s[cmd]       = code;
  tk->ss3_kpalts[cmd] = kpalt;

  return code;
}

int termkey_register_csifunc(termkey_t *tk, int code, int number, const char *name)
{
  if(name)
    code = termkey_register_keyname(tk, code, name);

  if(number >= tk->ncsifuncs) {
    tk->csifuncs = g_renew(int, tk->csifuncs, number + 1);
    tk->ncsifuncs = number + 1;

    // Fill in the hole
    for(int i = tk->ncsifuncs; i < number; i++)
      tk->csifuncs[i] = TERMKEY_SYM_UNKNOWN;
  }

  tk->csifuncs[number] = code;

  return code;
}
