#include "termkey.h"
#include "termkey-internal.h"

#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>

// TODO: Move these into t->driver
void *termkeycsi_new_driver(termkey_t *tk);
void termkeycsi_free_driver(void *private);
termkey_result termkeycsi_getkey(termkey_t *tk, termkey_key *key);
// END TODO

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

  tk->restore_termios_valid = 0;

  tk->waittime = waittime;

  tk->is_closed = 0;

  tk->nkeynames = 64;
  tk->keynames = malloc(sizeof(tk->keynames[0]) * tk->nkeynames);

  int i;
  for(i = 0; i < tk->nkeynames; i++)
    tk->keynames[i] = NULL;

  tk->driver_info = termkeycsi_new_driver(tk);

  // Special built-in names
  termkey_register_keyname(tk, TERMKEY_SYM_NONE, "NONE");

  termkey_register_c0(tk, TERMKEY_SYM_BACKSPACE, 0x08, "Backspace");
  termkey_register_c0(tk, TERMKEY_SYM_TAB,       0x09, "Tab");
  termkey_register_c0(tk, TERMKEY_SYM_ENTER,     0x0d, "Enter");
  termkey_register_c0(tk, TERMKEY_SYM_ESCAPE,    0x1b, "Escape");

  // G1
  termkey_register_keyname(tk, TERMKEY_SYM_SPACE, "Space");
  termkey_register_keyname(tk, TERMKEY_SYM_DEL,   "DEL");

  termkey_register_csi_ss3(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_UP,    'A', "Up");
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DOWN,  'B', "Down");
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RIGHT, 'C', "Right");
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_LEFT,  'D', "Left");
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BEGIN, 'E', "Begin");
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,   'F', "End");
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,  'H', "Home");
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_FUNCTION, 1, 'P', NULL);
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_FUNCTION, 2, 'Q', NULL);
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_FUNCTION, 3, 'R', NULL);
  termkey_register_csi_ss3(tk, TERMKEY_TYPE_FUNCTION, 4, 'S', NULL);

  termkey_register_csi_ss3_full(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_TAB, TERMKEY_KEYMOD_SHIFT, TERMKEY_KEYMOD_SHIFT, 'Z', NULL);

  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPENTER,  'M', "KPEnter",  0);
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPEQUALS, 'X', "KPEquals", '=');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMULT,   'j', "KPMult",   '*');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPLUS,   'k', "KPPlus",   '+');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPCOMMA,  'l', "KPComma",  ',');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMINUS,  'm', "KPMinus",  '-');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPERIOD, 'n', "KPPeriod", '.');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPDIV,    'o', "KPDiv",    '/');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP0,      'p', "KP0",      '0');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP1,      'q', "KP1",      '1');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP2,      'r', "KP2",      '2');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP3,      's', "KP3",      '3');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP4,      't', "KP4",      '4');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP5,      'u', "KP5",      '5');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP6,      'v', "KP6",      '6');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP7,      'w', "KP7",      '7');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP8,      'x', "KP8",      '8');
  termkey_register_ss3kpalt(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP9,      'y', "KP9",      '9');

  termkey_register_csifunc(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_FIND,      1, "Find");
  termkey_register_csifunc(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_INSERT,    2, "Insert");
  termkey_register_csifunc(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DELETE,    3, "Delete");
  termkey_register_csifunc(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SELECT,    4, "Select");
  termkey_register_csifunc(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEUP,    5, "PageUp");
  termkey_register_csifunc(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEDOWN,  6, "PageDown");
  termkey_register_csifunc(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,      7, "Home");
  termkey_register_csifunc(tk, TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,       8, "End");

  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 1,  11, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 2,  12, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 3,  13, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 4,  14, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 5,  15, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 6,  17, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 7,  18, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 8,  19, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 9,  20, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 10, 21, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 11, 23, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 12, 24, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 13, 25, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 14, 26, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 15, 28, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 16, 29, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 17, 31, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 18, 32, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 19, 33, NULL);
  termkey_register_csifunc(tk, TERMKEY_TYPE_FUNCTION, 20, 34, NULL);

  if(!(flags & TERMKEY_FLAG_NOTERMIOS)) {
    struct termios termios;
    if(tcgetattr(fd, &termios) == 0) {
      tk->restore_termios = termios;
      tk->restore_termios_valid = 1;

      termios.c_iflag &= ~(IXON|INLCR|ICRNL);
      termios.c_lflag &= ~(ICANON|ECHO|ISIG);

      tcsetattr(fd, TCSANOW, &termios);
    }
  }

  return tk;
}

termkey_t *termkey_new(int fd, int flags)
{
  return termkey_new_full(fd, flags, 256, 50);
}

void termkey_free(termkey_t *tk)
{
  free(tk->buffer); tk->buffer = NULL;
  free(tk->keynames); tk->keynames = NULL;

  termkeycsi_free_driver(tk->driver_info);
  tk->driver_info = NULL; /* Be nice to GC'ers, etc */

  free(tk);
}

void termkey_destroy(termkey_t *tk)
{
  if(tk->restore_termios_valid)
    tcsetattr(tk->fd, TCSANOW, &tk->restore_termios);

  termkey_free(tk);
}

void termkey_setwaittime(termkey_t *tk, int msec)
{
  tk->waittime = msec;
}

int termkey_getwaittime(termkey_t *tk)
{
  return tk->waittime;
}

termkey_result termkey_getkey(termkey_t *tk, termkey_key *key)
{
  return termkeycsi_getkey(tk, key);
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
    while(tk->buffstart + tk->buffcount + inputlen > tk->buffsize)
      tk->buffsize *= 2;

    unsigned char *newbuffer = realloc(tk->buffer, tk->buffsize);
    tk->buffer = newbuffer;
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

termkey_keysym termkey_register_keyname(termkey_t *tk, termkey_keysym sym, const char *name)
{
  if(!sym)
    sym = tk->nkeynames;

  if(sym >= tk->nkeynames) {
    const char **new_keynames = realloc(tk->keynames, sizeof(new_keynames[0]) * (sym + 1));
    tk->keynames = new_keynames;

    // Fill in the hole
    for(int i = tk->nkeynames; i < sym; i++)
      tk->keynames[i] = NULL;

    tk->nkeynames = sym + 1;
  }

  tk->keynames[sym] = name;

  return sym;
}

const char *termkey_get_keyname(termkey_t *tk, termkey_keysym sym)
{
  if(sym == TERMKEY_SYM_UNKNOWN)
    return "UNKNOWN";

  if(sym < tk->nkeynames)
    return tk->keynames[sym];

  return "UNKNOWN";
}

size_t termkey_snprint_key(termkey_t *tk, char *buffer, size_t len, termkey_key *key, termkey_format format)
{
  size_t pos = 0;
  size_t l;

  int longmod = format & TERMKEY_FORMAT_LONGMOD;

  int wrapbracket = (format & TERMKEY_FORMAT_WRAPBRACKET) &&
                    (key->type != TERMKEY_TYPE_UNICODE || key->modifiers != 0);

  if(wrapbracket) {
    l = snprintf(buffer + pos, len - pos, "<");
    if(l <= 0) return pos;
    pos += l;
  }

  if(format & TERMKEY_FORMAT_CARETCTRL) {
    if(key->type == TERMKEY_TYPE_UNICODE &&
       key->modifiers == TERMKEY_KEYMOD_CTRL &&
       key->code.number >= '@' &&
       key->code.number <= '_') {
      l = snprintf(buffer + pos, len - pos, "^");
      if(l <= 0) return pos;
      pos += l;
      goto do_codepoint;
    }
  }

  if(key->modifiers & TERMKEY_KEYMOD_ALT) {
    int altismeta = format & TERMKEY_FORMAT_ALTISMETA;

    l = snprintf(buffer + pos, len - pos, longmod ? ( altismeta ? "Meta-" : "Alt-" )
                                                  : ( altismeta ? "M-"    : "A-" ));
    if(l <= 0) return pos;
    pos += l;
  }

  if(key->modifiers & TERMKEY_KEYMOD_CTRL) {
    l = snprintf(buffer + pos, len - pos, longmod ? "Ctrl-" : "C-");
    if(l <= 0) return pos;
    pos += l;
  }

  if(key->modifiers & TERMKEY_KEYMOD_SHIFT) {
    l = snprintf(buffer + pos, len - pos, longmod ? "Shift-" : "S-");
    if(l <= 0) return pos;
    pos += l;
  }

do_codepoint:

  switch(key->type) {
  case TERMKEY_TYPE_UNICODE:
    l = snprintf(buffer + pos, len - pos, "%s", key->utf8);
    break;
  case TERMKEY_TYPE_KEYSYM:
    l = snprintf(buffer + pos, len - pos, "%s", termkey_get_keyname(tk, key->code.sym));
    break;
  case TERMKEY_TYPE_FUNCTION:
    l = snprintf(buffer + pos, len - pos, "F%d", key->code.number);
    break;
  }

  if(l <= 0) return pos;
  pos += l;

  if(wrapbracket) {
    l = snprintf(buffer + pos, len - pos, ">");
    if(l <= 0) return pos;
    pos += l;
  }

  return pos;
}
