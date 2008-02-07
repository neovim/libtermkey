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

static int getkey_csi(termkey_t *tk, size_t introlen, termkey_key *key)
{
  size_t csi_end = introlen;

  while(csi_end < tk->buffvalid) {
    if(tk->buffer[csi_end] >= 0x40 && tk->buffer[csi_end] < 0x80)
      break;
    csi_end++;
  }

  if(csi_end >= tk->buffvalid) {
    key->code = TERMKEY_SYM_NONE;
    return key->code;
  }

  unsigned char cmd = tk->buffer[csi_end];
  int arg1 = -1;
  int arg2 = -1;

  size_t p = introlen;

  // Now attempt to parse out up to the first two 1;2; separated arguments
  if(tk->buffer[p] >= '0' && tk->buffer[p] <= '9') {
    arg1 = 0;
    while(p < csi_end && tk->buffer[p] != ';') {
      arg1 = (arg1 * 10) + tk->buffer[p] - '0';
      p++;
    }

    if(tk->buffer[p] == ';')
      p++;

    if(tk->buffer[p] >= '0' && tk->buffer[p] <= '9') {
      arg2 = 0;
      while(p < csi_end && tk->buffer[p] != ';') {
        arg2 = (arg2 * 10) + tk->buffer[p] - '0';
        p++;
      }
    }
  }

  key->code = TERMKEY_SYM_UNKNOWN;

  switch(cmd) {
    case 'A': key->code = TERMKEY_SYM_UP;    break;
    case 'B': key->code = TERMKEY_SYM_DOWN;  break;
    case 'C': key->code = TERMKEY_SYM_RIGHT; break;
    case 'D': key->code = TERMKEY_SYM_LEFT;  break;
    case 'E': key->code = TERMKEY_SYM_BEGIN; break;
    case 'F': key->code = TERMKEY_SYM_END;   break;
    case 'H': key->code = TERMKEY_SYM_HOME;  break;
    case 'P': key->code = TERMKEY_SYM_F1;    break;
    case 'Q': key->code = TERMKEY_SYM_F2;    break;
    case 'R': key->code = TERMKEY_SYM_F3;    break;
    case 'S': key->code = TERMKEY_SYM_F4;    break;
    case '~':
      switch(arg1) {
        case  1: key->code = TERMKEY_SYM_HOME;     break;
        case  2: key->code = TERMKEY_SYM_INSERT;   break;
        case  3: key->code = TERMKEY_SYM_DELETE;   break;
        case  4: key->code = TERMKEY_SYM_END;      break;
        case  5: key->code = TERMKEY_SYM_PAGEUP;   break;
        case  6: key->code = TERMKEY_SYM_PAGEDOWN; break;
        case 15: key->code = TERMKEY_SYM_F5;       break;
        case 17: key->code = TERMKEY_SYM_F6;       break;
        case 18: key->code = TERMKEY_SYM_F7;       break;
        case 19: key->code = TERMKEY_SYM_F8;       break;
        case 20: key->code = TERMKEY_SYM_F9;       break;
        case 21: key->code = TERMKEY_SYM_F10;      break;
        case 23: key->code = TERMKEY_SYM_F11;      break;
        case 24: key->code = TERMKEY_SYM_F12;      break;

        default:
          fprintf(stderr, "CSI function key %d\n", arg1);
      }
      break;

    default:
      fprintf(stderr, "CSI arg1=%d arg2=%d cmd=%c\n", arg1, arg2, cmd);
  }

  key->modifiers = arg2 == -1 ? 0 : arg2 - 1;
  key->flags = TERMKEY_KEYFLAG_SPECIAL;

  eatbytes(tk, csi_end + 1);

  return key->code;
}

static int getkey_ss3(termkey_t *tk, size_t introlen, termkey_key *key)
{
  if(introlen + 1 < tk->buffvalid) {
    key->code = TERMKEY_SYM_NONE;
    return key->code;
  }

  unsigned char cmd = tk->buffer[introlen];

  key->code = TERMKEY_SYM_UNKNOWN;

  switch(cmd) {
    case 'A': key->code = TERMKEY_SYM_UP;       break;
    case 'B': key->code = TERMKEY_SYM_DOWN;     break;
    case 'C': key->code = TERMKEY_SYM_RIGHT;    break;
    case 'D': key->code = TERMKEY_SYM_LEFT;     break;
    case 'F': key->code = TERMKEY_SYM_END;      break;
    case 'H': key->code = TERMKEY_SYM_HOME;     break;
    case 'M': key->code = TERMKEY_SYM_KPENTER;  break;
    case 'P': key->code = TERMKEY_SYM_F1;       break;
    case 'Q': key->code = TERMKEY_SYM_F2;       break;
    case 'R': key->code = TERMKEY_SYM_F3;       break;
    case 'S': key->code = TERMKEY_SYM_F4;       break;
    case 'X': key->code = TERMKEY_SYM_KPEQUALS; break;
    case 'j': key->code = TERMKEY_SYM_KPMULT;   break;
    case 'k': key->code = TERMKEY_SYM_KPPLUS;   break;
    case 'l': key->code = TERMKEY_SYM_KPCOMMA;  break;
    case 'm': key->code = TERMKEY_SYM_KPMINUS;  break;
    case 'n': key->code = TERMKEY_SYM_KPPERIOD; break;
    case 'o': key->code = TERMKEY_SYM_KPDIV;    break;
    case 'p': key->code = TERMKEY_SYM_KP0;      break;
    case 'q': key->code = TERMKEY_SYM_KP1;      break;
    case 'r': key->code = TERMKEY_SYM_KP2;      break;
    case 's': key->code = TERMKEY_SYM_KP3;      break;
    case 't': key->code = TERMKEY_SYM_KP4;      break;
    case 'u': key->code = TERMKEY_SYM_KP5;      break;
    case 'v': key->code = TERMKEY_SYM_KP6;      break;
    case 'w': key->code = TERMKEY_SYM_KP7;      break;
    case 'x': key->code = TERMKEY_SYM_KP8;      break;
    case 'y': key->code = TERMKEY_SYM_KP9;      break;

    default:
      fprintf(stderr, "SS3 %c (0x%02x)\n", cmd, cmd);
      break;
  }

  key->modifiers = 0;
  key->flags = TERMKEY_KEYFLAG_SPECIAL;

  if(tk->flags & TERMKEY_FLAG_CONVERTKP) {
    char is_kp = 1;
    switch(key->code) {
      case TERMKEY_SYM_KP0:      key->code = '0'; break;
      case TERMKEY_SYM_KP1:      key->code = '1'; break;
      case TERMKEY_SYM_KP2:      key->code = '2'; break;
      case TERMKEY_SYM_KP3:      key->code = '3'; break;
      case TERMKEY_SYM_KP4:      key->code = '4'; break;
      case TERMKEY_SYM_KP5:      key->code = '5'; break;
      case TERMKEY_SYM_KP6:      key->code = '6'; break;
      case TERMKEY_SYM_KP7:      key->code = '7'; break;
      case TERMKEY_SYM_KP8:      key->code = '8'; break;
      case TERMKEY_SYM_KP9:      key->code = '9'; break;
      case TERMKEY_SYM_KPPLUS:   key->code = '+'; break;
      case TERMKEY_SYM_KPMINUS:  key->code = '-'; break;
      case TERMKEY_SYM_KPMULT:   key->code = '*'; break;
      case TERMKEY_SYM_KPDIV:    key->code = '/'; break;
      case TERMKEY_SYM_KPCOMMA:  key->code = ','; break;
      case TERMKEY_SYM_KPPERIOD: key->code = '.'; break;
      case TERMKEY_SYM_KPEQUALS: key->code = '='; break;

      default:
        is_kp = 0;
        break;
    }

    if(is_kp) {
      key->flags = 0;

      key->utf8[0] = key->code;
      key->utf8[1] = 0;
    }
  }

  eatbytes(tk, introlen + 1);

  return key->code;
}

int termkey_getkey(termkey_t *tk, termkey_key *key)
{
  if(tk->buffvalid == 0) {
    key->code = tk->is_closed ? TERMKEY_SYM_EOF : TERMKEY_SYM_NONE;
    return key->code;
  }

  // Now we're sure at least 1 byte is valid
  unsigned char b0 = tk->buffer[0];

  if(b0 == 0x1b) {
    if(tk->buffvalid == 1) {
      key->code = TERMKEY_SYM_ESCAPE;
      key->modifiers = 0;
      key->flags = TERMKEY_KEYFLAG_SPECIAL;

      eatbytes(tk, 1);

      return key->code;
    }

    unsigned char b1 = tk->buffer[1];
    if(b1 == '[')
      return getkey_csi(tk, 2, key);

    if(b1 == 'O')
      return getkey_ss3(tk, 2, key);

    tk->buffer++;

    int metakey = termkey_getkey(tk, key);
    if(metakey != TERMKEY_SYM_NONE) {
      key->modifiers |= TERMKEY_KEYMOD_ALT;
      tk->buffer--;
      eatbytes(tk, 1);
    }

    return key->code;
  }
  else if(b0 < 0x20) {
    // Control key

    key->code = TERMKEY_SYM_UNKNOWN;

    if(!(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
      // Try to interpret C0 codes that have nice names
      switch(b0) {
        case 0x08: key->code = TERMKEY_SYM_BACKSPACE; break;
        case 0x09: key->code = TERMKEY_SYM_TAB;       break;
        case 0x0a: key->code = TERMKEY_SYM_ENTER;     break;
      }
    }

    if(key->code == TERMKEY_SYM_UNKNOWN) {
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

    return key->code;
  }
  else if(b0 == 0x20 && !(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
    key->code = TERMKEY_SYM_SPACE;
    key->modifiers = 0;
    key->flags = TERMKEY_KEYFLAG_SPECIAL;

    eatbytes(tk, 1);

    return key->code;
  }
  else if(b0 == 0x7f && !(tk->flags & TERMKEY_FLAG_NOINTERPRET)) {
    key->code = TERMKEY_SYM_DEL;
    key->modifiers = 0;
    key->flags = TERMKEY_KEYFLAG_SPECIAL;

    eatbytes(tk, 1);

    return key->code;
  }
  else if(b0 >= 0x20 && b0 < 0x80) {
    // ASCII lowbyte range
    key->code = b0;
    key->modifiers = 0;
    key->flags = 0;

    key->utf8[0] = key->code;
    key->utf8[1] = 0;

    eatbytes(tk, 1);

    return key->code;
  }

  fprintf(stderr, "TODO - tk->buffer[0] == 0x%02x\n", tk->buffer[0]);
  return TERMKEY_SYM_NONE;
}

int termkey_waitkey(termkey_t *tk, termkey_key *key)
{
  int ret;
  while((ret = termkey_getkey(tk, key)) == TERMKEY_SYM_NONE) {
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

// Must be kept synchronised with enum termkey_sym_e in termkey.h
const char *keynames[] = {
  "None",

  // Special names in C0
  "Backspace",
  "Tab",
  "Enter",
  "Escape",

  // Special names in G0
  "Space",
  "DEL",

  // CSI keys
  "Up",
  "Down",
  "Left",
  "Right",
  "Begin",

  // CSI function keys
  "Insert",
  "Delete",
  "Home",
  "End",
  "PageUp",
  "PageDown",

  "F1",
  "F2",
  "F3",
  "F4",
  "F5",
  "F6",
  "F7",
  "F8",
  "F9",
  "F10",
  "F11",
  "F12",

  // Numeric keypad special keys
  "KP0",
  "KP1",
  "KP2",
  "KP3",
  "KP4",
  "KP5",
  "KP6",
  "KP7",
  "KP8",
  "KP9",
  "KPEnter",
  "KPPlus",
  "KPMinus",
  "KPMult",
  "KPDiv",
  "KPComma",
  "KPPeriod",
  "KPEquals",
};

const char *termkey_describe_sym(int code)
{
  if(code == -1)
    return "EOF";

  if(code == -2)
    return "UNKNOWN";

  if(code < sizeof(keynames)/sizeof(keynames[0]))
    return keynames[code];

  return "UNKNOWN";
}
