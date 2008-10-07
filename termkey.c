#include "termkey.h"
#include "termkey-internal.h"

#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>

static struct termkey_driver *drivers[] = {
  &termkey_driver_csi,
  NULL,
};

// Forwards for the "protected" methods
static void eat_bytes(termkey_t *tk, size_t count);

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

  tk->method.eat_bytes = &eat_bytes;

  for(i = 0; drivers[i]; i++) {
    void *driver_info = (*drivers[i]->new_driver)(tk);
    if(!driver_info)
      continue;

    tk->driver = *(drivers[i]);
    tk->driver_info = driver_info;
  }

  if(!tk->driver_info) {
    fprintf(stderr, "Unable to find a terminal driver\n");
    return NULL;
  }

  // Special built-in names
  termkey_register_keyname(tk, TERMKEY_SYM_NONE, "NONE");
  termkey_register_keyname(tk, TERMKEY_SYM_SPACE, "Space");
  termkey_register_keyname(tk, TERMKEY_SYM_DEL,   "DEL");

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

  (*tk->driver.free_driver)(tk->driver_info);
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

static void eat_bytes(termkey_t *tk, size_t count)
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

termkey_result termkey_getkey(termkey_t *tk, termkey_key *key)
{
  return (*tk->driver.getkey)(tk, key);
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
