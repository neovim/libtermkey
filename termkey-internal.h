#ifndef GUARD_TERMKEY_INTERNAL_H_
#define GUARD_TERMKEY_INTERNAL_H_

#include "termkey.h"

#include <stdint.h>
#include <termios.h>

struct termkey_driver
{
  const char      *name;
  void          *(*new_driver)(termkey_t *tk, const char *term);
  void           (*free_driver)(void *info);
  void           (*start_driver)(termkey_t *tk, void *info);
  void           (*stop_driver)(termkey_t *tk, void *info);
  termkey_result (*getkey)(termkey_t *tk, void *info, termkey_key *key, int force);
};

struct keyinfo {
  termkey_type type;
  termkey_keysym sym;
  int modifier_mask;
  int modifier_set;
};

struct termkey_drivernode;
struct termkey_drivernode {
  struct termkey_driver     *driver;
  void                      *info;
  struct termkey_drivernode *next;
};

struct termkey {
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

  struct termkey_drivernode *drivers;

  // Now some "protected" methods for the driver to call but which we don't
  // want exported as real symbols in the library
  struct {
    void (*eat_bytes)(termkey_t *tk, size_t count);
    void (*emit_codepoint)(termkey_t *tk, long codepoint, termkey_key *key);
    termkey_result (*getkey_simple)(termkey_t *tk, termkey_key *key, int force);
  } method;
};

extern struct termkey_driver termkey_driver_csi;
extern struct termkey_driver termkey_driver_ti;

#endif
