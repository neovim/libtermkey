#ifndef GUARD_TERMKEY_INTERNAL_H_
#define GUARD_TERMKEY_INTERNAL_H_

#include "termkey.h"

#include <stdint.h>
#include <termios.h>

struct TermKeyDriver
{
  const char      *name;
  void          *(*new_driver)(TermKey *tk, const char *term);
  void           (*free_driver)(void *info);
  void           (*start_driver)(TermKey *tk, void *info);
  void           (*stop_driver)(TermKey *tk, void *info);
  TermKeyResult (*peekkey)(TermKey *tk, void *info, TermKeyKey *key, int force, size_t *nbytes);
};

struct keyinfo {
  TermKeyType type;
  TermKeySym sym;
  int modifier_mask;
  int modifier_set;
};

struct TermKeyDriverNode;
struct TermKeyDriverNode {
  struct TermKeyDriver     *driver;
  void                      *info;
  struct TermKeyDriverNode *next;
};

struct _TermKey {
  int    fd;
  int    flags;
  unsigned char *buffer;
  size_t buffstart; // First offset in buffer
  size_t buffcount; // NUMBER of entires valid in buffer
  size_t buffsize; // Total malloc'ed size

  struct termios restore_termios;
  char restore_termios_valid;

  int waittime; // msec

  char   is_closed;

  int  nkeynames;
  const char **keynames;

  // There are 32 C0 codes
  struct keyinfo c0[32];

  struct TermKeyDriverNode *drivers;

  // Now some "protected" methods for the driver to call but which we don't
  // want exported as real symbols in the library
  struct {
    void (*emit_codepoint)(TermKey *tk, long codepoint, TermKeyKey *key);
    TermKeyResult (*peekkey_simple)(TermKey *tk, TermKeyKey *key, int force, size_t *nbytes);
    TermKeyResult (*peekkey_mouse)(TermKey *tk, TermKeyKey *key, size_t *nbytes);
  } method;
};

extern struct TermKeyDriver termkey_driver_csi;
extern struct TermKeyDriver termkey_driver_ti;

#endif
