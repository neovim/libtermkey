#ifndef GUARD_TERMKEY_INTERNAL_H_
#define GUARD_TERMKEY_INTERNAL_H_

#include "termkey.h"

#include <stdint.h>
#include <termios.h>

struct termkey_driver
{
  void *(*new_driver)(void);
  void  (*free_driver)(void *);
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

  struct termkey_driver driver;
  void *driver_info;
};

void *termkeycsi_new_driver(termkey_t *t);

#endif
