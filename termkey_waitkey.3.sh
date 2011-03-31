# vim:ft=nroff
cat <<EOF
.TH TERMKEY_WAITKEY 3
.SH NAME
termkey_waitkey \- wait for and retrieve the next key event
.SH SYNOPSIS
.nf
.B #include <termkey.h>
.sp
.BI "TermKeyResult termkey_waitkey(TermKey *" tk ", TermKeyKey *" key );
.fi
.sp
Link with \fI-ltermkey\fP.
.SH DESCRIPTION
\fBtermkey_waitkey\fP attempts to retrieve a single keypress event from the buffer, and put it in the structure referred to by \fIkey\fP. If successful it will return \fBTERMKEY_RES_KEY\fP to indicate that the structure now contains a new keypress event. If nothing is in the buffer it will block until one is available. If no events are ready and the input stream is now closed, will return \fBTERMKEY_RES_EOF\fP.
.PP
For details of the \fBTermKeyKey\fP structure, see \fBtermkey_getkey\fP(3).
.PP
Some keypresses generate multiple bytes from the terminal. Because there may be network or other delays between the terminal and an application using termkey, \fBtermkey_waitkey\fP() will attempt to wait for the remaining bytes to arrive if it detects the start of a multibyte sequence. If no more bytes arrive within a certain time, then the bytes will be reported as they stand, even if this results in interpreting a partially-complete Escape sequence as a literal Escape key followed by some normal letters or other symbols. The amount of time to wait can be set by \fBtermkey_set_waittime\fP(3).
.SH "RETURN VALUE"
\fBtermkey_waitkey\fP() returns one of the following constants:
.TP
.B TERMKEY_RES_KEY
A key event as been provided.
.TP
.B TERMKEY_RES_EOF
No key events are ready and the terminal has been closed, so no more will arrive.
.SH EXAMPLE
The following example program prints details of every keypress until the user presses "Ctrl-C".
.PP
.in +4n
EOF
sed i.br demo.c
cat <<EOF
.in
.nf
.fi
.SH "SEE ALSO"
.BR termkey_new (3),
.BR termkey_getkey (3),
.BR termkey_set_waittime (3),
.BR termkey_get_keyname (3),
.BR termkey_strfkey (3)
EOF
