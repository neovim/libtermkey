#include "termkey.h"
#include "termkey-internal.h"

#include <term.h>
#include <curses.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct ti_keyinfo {
  const char *seq;
  size_t seqlen; // cached strlen of seq since we'll use it lots
  struct keyinfo key;
};

typedef struct {
  termkey_t *tk;

  int nseqs;
  int alloced_seqs;
  struct ti_keyinfo *seqs;
} termkey_ti;

static int funcname2keysym(const char *funcname, termkey_type *typep, termkey_keysym *symp, int *modmask, int *modsetp);
static int register_seq(termkey_ti *ti, const char *seq, termkey_type type, termkey_keysym sym, int modmask, int modset);

static void *new_driver(termkey_t *tk, const char *term)
{
  int err;

  if(setupterm(term, 1, &err) != OK)
    return NULL;

  termkey_ti *ti = malloc(sizeof *ti);
  if(!ti)
    return NULL;

  ti->tk = tk;

  ti->alloced_seqs = 32; // We'll allocate more space if we need
  ti->nseqs = 0;

  ti->seqs = malloc(ti->alloced_seqs * sizeof(ti->seqs[0]));
  if(!ti->seqs)
    goto abort_free_ti;

  int i;
  for(i = 0; strfnames[i]; i++) {
    // Only care about the key_* constants
    const char *name = strfnames[i];
    if(strncmp(name, "key_", 4) != 0)
      continue;

    const char *value = tigetstr(strnames[i]);
    if(!value || value == (char*)-1)
      continue;

    termkey_type type;
    termkey_keysym sym;
    int mask = 0;
    int set  = 0;

    if(!funcname2keysym(strfnames[i] + 4, &type, &sym, &mask, &set))
      continue;

    if(sym != TERMKEY_SYM_NONE)
      if(!register_seq(ti, value, type, sym, mask, set))
        goto abort_free_seqs;
  }

  return ti;

abort_free_seqs:
  free(ti->seqs);

abort_free_ti:
  free(ti);

  return NULL;
}

static void start_driver(termkey_t *tk, void *info)
{
  /* The terminfo database will contain keys in application cursor key mode.
   * We may need to enable that mode
   */
  if(keypad_xmit) {
    // Can't call putp or tputs because they suck and don't give us fd control
    write(tk->fd, keypad_xmit, strlen(keypad_xmit));
  }
}

static void stop_driver(termkey_t *tk, void *info)
{
  if(keypad_local) {
    // Can't call putp or tputs because they suck and don't give us fd control
    write(tk->fd, keypad_local, strlen(keypad_local));
  }
}

static void free_driver(void *info)
{
  termkey_ti *ti = info;

  free(ti->seqs);

  free(ti);
}

#define CHARAT(i) (tk->buffer[tk->buffstart + (i)])

static termkey_result getkey(termkey_t *tk, void *info, termkey_key *key, int force)
{
  termkey_ti *ti = info;

  if(tk->buffcount == 0)
    return tk->is_closed ? TERMKEY_RES_EOF : TERMKEY_RES_NONE;

  // Now we're sure at least 1 byte is valid
  unsigned char b0 = CHARAT(0);

  int i;
  for(i = 0; i < ti->nseqs; i++) {
    struct ti_keyinfo *s = &(ti->seqs[i]);

    if(s->seq[0] != b0)
      continue;

    if(tk->buffcount >= s->seqlen) {
      if(strncmp(s->seq, (const char*)tk->buffer + tk->buffstart, s->seqlen) == 0) {
        key->type      = s->key.type;
        key->code.sym  = s->key.sym;
        key->modifiers = s->key.modifier_set;
        (*tk->method.eat_bytes)(tk, s->seqlen);
        return TERMKEY_RES_KEY;
      }
    }
    else if(!force) {
      // Maybe we'd get a partial match
      if(strncmp(s->seq, (const char*)tk->buffer + tk->buffstart, tk->buffcount) == 0)
        return TERMKEY_RES_AGAIN;
    }
  }

  return TERMKEY_RES_NONE;
}

static struct {
  const char *funcname;
  termkey_type type;
  termkey_keysym sym;
  int mods;
} funcs[] =
{
  /* THIS LIST MUST REMAIN SORTED! */
  { "backspace", TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BACKSPACE, 0 },
  { "begin",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BEGIN,     0 },
  { "beg",       TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BEGIN,     0 },
  { "btab",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_TAB,       TERMKEY_KEYMOD_SHIFT },
  { "cancel",    TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_CANCEL,    0 },
  { "clear",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_CLEAR,     0 },
  { "close",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_CLOSE,     0 },
  { "command",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_COMMAND,   0 },
  { "copy",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_COPY,      0 },
  { "dc",        TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DELETE,    0 },
  { "down",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DOWN,      0 },
  { "end",       TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,       0 },
  { "enter",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_ENTER,     0 },
  { "exit",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_EXIT,      0 },
  { "find",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_FIND,      0 },
  { "help",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HELP,      0 },
  { "home",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,      0 },
  { "ic",        TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_INSERT,    0 },
  { "left",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_LEFT,      0 },
  { "mark",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_MARK,      0 },
  { "message",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_MESSAGE,   0 },
  { "mouse",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_NONE,      0 },
  { "move",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_MOVE,      0 },
  { "next",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEDOWN,  0 }, // Not quite, but it's the best we can do
  { "npage",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEDOWN,  0 },
  { "open",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_OPEN,      0 },
  { "options",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_OPTIONS,   0 },
  { "ppage",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEUP,    0 },
  { "previous",  TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEUP,    0 }, // Not quite, but it's the best we can do
  { "print",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PRINT,     0 },
  { "redo",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_REDO,      0 },
  { "reference", TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_REFERENCE, 0 },
  { "refresh",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_REFRESH,   0 },
  { "replace",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_REPLACE,   0 },
  { "restart",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RESTART,   0 },
  { "resume",    TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RESUME,    0 },
  { "right",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RIGHT,     0 },
  { "save",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SAVE,      0 },
  { "select",    TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SELECT,    0 },
  { "suspend",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SUSPEND,   0 },
  { "undo",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_UNDO,      0 },
  { "up",        TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_UP,        0 },
  { NULL },
};

static int funcname2keysym(const char *funcname, termkey_type *typep, termkey_keysym *symp, int *modmaskp, int *modsetp)
{
  // Binary search

  int start = 0;
  int end   = sizeof(funcs)/sizeof(funcs[0]); // is "one past" the end of the range

  while(1) {
    int i = (start+end) / 2;
    int cmp = strcmp(funcname, funcs[i].funcname);

    if(cmp == 0) {
      *typep    = funcs[i].type;
      *symp     = funcs[i].sym;
      *modmaskp = funcs[i].mods;
      *modsetp  = funcs[i].mods;
      return 1;
    }
    else if(end == start + 1)
      // That was our last choice and it wasn't it - not found
      break;
    else if(cmp > 0)
      start = i;
    else
      end = i;
  }

  if(funcname[0] == 'f' && isdigit(funcname[1])) {
    *typep = TERMKEY_TYPE_FUNCTION;
    *symp  = atoi(funcname + 1);
    return 1;
  }

  // Last-ditch attempt; maybe it's a shift key?
  if(funcname[0] == 's' && funcname2keysym(funcname + 1, typep, symp, modmaskp, modsetp)) {
    *modmaskp |= TERMKEY_KEYMOD_SHIFT;
    *modsetp  |= TERMKEY_KEYMOD_SHIFT;
    return 1;
  }

  printf("TODO: Need to convert funcname %s to a type/sym\n", funcname);
  return 0;
}

static int register_seq(termkey_ti *ti, const char *seq, termkey_type type, termkey_keysym sym, int modmask, int modset)
{
  if(ti->nseqs == ti->alloced_seqs) {
    ti->alloced_seqs *= 2;
    void *newseqs = realloc(ti->seqs, ti->alloced_seqs * sizeof(ti->seqs[9]));
    if(!newseqs)
      return 0;
    ti->seqs = newseqs;
  }

  int i = ti->nseqs++;
  ti->seqs[i].seq = seq;
  ti->seqs[i].seqlen = strlen(seq);
  ti->seqs[i].key.type = type;
  ti->seqs[i].key.sym = sym;
  ti->seqs[i].key.modifier_mask = modmask;
  ti->seqs[i].key.modifier_set  = modset;

  return 1;
}

struct termkey_driver termkey_driver_ti = {
  .name        = "terminfo",

  .new_driver  = new_driver,
  .free_driver = free_driver,

  .start_driver = start_driver,
  .stop_driver  = stop_driver,

  .getkey = getkey,
};
