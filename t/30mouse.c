#include "../termkey.h"
#include "taplib.h"

int main(int argc, char *argv[])
{
  TermKey   *tk;
  TermKeyKey key;
  TermKeyMouseEvent ev;
  int        button, line, col;
  char       buffer[32];
  size_t     len;

  plan_tests(31);

  /* vt100 doesn't have a mouse, we need xterm */
  tk = termkey_new_abstract("xterm", 0);

  termkey_push_bytes(tk, "\e[M !!", 6);

  is_int(termkey_getkey(tk, &key), TERMKEY_RES_KEY, "getkey yields RES_KEY for mouse press");

  is_int(key.type, TERMKEY_TYPE_MOUSE, "key.type for mouse press");

  is_int(termkey_interpret_mouse(tk, &key, &ev, &button, &line, &col), TERMKEY_RES_KEY, "interpret_mouse yields RES_KEY");

  is_int(ev,     TERMKEY_MOUSE_PRESS, "mouse event for press");
  is_int(button, 1,                   "mouse button for press");
  is_int(line,   1,                   "mouse line for press");
  is_int(col,    1,                   "mouse column for press");
  is_int(key.modifiers, 0,            "modifiers for press");

  len = termkey_strfkey(tk, buffer, sizeof buffer, &key, 0);
  is_int(len, 13, "string length for press");
  is_str(buffer, "MousePress(1)", "string buffer for press");

  len = termkey_strfkey(tk, buffer, sizeof buffer, &key, TERMKEY_FORMAT_MOUSE_POS);
  is_int(len, 21, "string length for press");
  is_str(buffer, "MousePress(1) @ (1,1)", "string buffer for press");

  termkey_push_bytes(tk, "\e[M@\"!", 6);

  termkey_getkey(tk, &key);
  is_int(termkey_interpret_mouse(tk, &key, &ev, &button, &line, &col), TERMKEY_RES_KEY, "interpret_mouse yields RES_KEY");

  is_int(ev,     TERMKEY_MOUSE_DRAG, "mouse event for drag");
  is_int(button, 1,                   "mouse button for drag");
  is_int(line,   1,                   "mouse line for drag");
  is_int(col,    2,                   "mouse column for drag");
  is_int(key.modifiers, 0,            "modifiers for press");

  termkey_push_bytes(tk, "\e[M##!", 6);

  termkey_getkey(tk, &key);
  is_int(termkey_interpret_mouse(tk, &key, &ev, &button, &line, &col), TERMKEY_RES_KEY, "interpret_mouse yields RES_KEY");

  is_int(ev,     TERMKEY_MOUSE_RELEASE, "mouse event for release");
  is_int(line,   1,                     "mouse line for release");
  is_int(col,    3,                     "mouse column for release");
  is_int(key.modifiers, 0,            "modifiers for press");

  termkey_push_bytes(tk, "\e[M0++", 6);

  termkey_getkey(tk, &key);
  is_int(termkey_interpret_mouse(tk, &key, &ev, &button, &line, &col), TERMKEY_RES_KEY, "interpret_mouse yields RES_KEY");

  is_int(ev,     TERMKEY_MOUSE_PRESS, "mouse event for Ctrl-press");
  is_int(button, 1,                   "mouse button for Ctrl-press");
  is_int(line,   11,                  "mouse line for Ctrl-press");
  is_int(col,    11,                  "mouse column for Ctrl-press");
  is_int(key.modifiers, TERMKEY_KEYMOD_CTRL, "modifiers for Ctrl-press");

  len = termkey_strfkey(tk, buffer, sizeof buffer, &key, 0);
  is_int(len, 15, "string length for Ctrl-press");
  is_str(buffer, "C-MousePress(1)", "string buffer for Ctrl-press");

  termkey_destroy(tk);

  return exit_status();
}
