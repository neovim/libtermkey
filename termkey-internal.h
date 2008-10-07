#ifndef GUARD_TERMKEY_INTERNAL_H_
#define GUARD_TERMKEY_INTERNAL_H_

#include "termkey.h"

#include <stdint.h>
#include <termios.h>

struct termkey_driver
{
  void          *(*new_driver)(termkey_t *tk);
  void           (*free_driver)(void *);
  termkey_result (*getkey)(termkey_t *tk, termkey_key *key);
};

struct keyinfo {
  termkey_type type;
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

  struct termios restore_termios;
  char restore_termios_valid;

  int waittime; // msec

  char   is_closed;

  int  nkeynames;
  const char **keynames;

  // There are 32 C0 codes
  struct keyinfo c0[32];

  struct termkey_driver driver;
  void *driver_info;

  // Now some "protected" methods for the driver to call but which we don't
  // want exported as real symbols in the library
  struct {
    void (*eat_bytes)(termkey_t *tk, size_t count);
    void (*emit_codepoint)(termkey_t *tk, int codepoint, termkey_key *key);
  } method;
};

extern struct termkey_driver termkey_driver_csi;

// Keep this here for now since it's tiny
static inline int utf8_seqlen(int codepoint)
{
  if(codepoint < 0x0000080) return 1;
  if(codepoint < 0x0000800) return 2;
  if(codepoint < 0x0010000) return 3;
  if(codepoint < 0x0200000) return 4;
  if(codepoint < 0x4000000) return 5;
  return 6;
}

#endif
