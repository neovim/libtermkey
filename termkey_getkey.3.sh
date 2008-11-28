# vim:ft=nroff
cat <<EOF
.TH TERMKEY_GETKEY 3
.SH NAME
termkey_getkey, termkey_getkey_force \- retrieve the next key event
.SH SYNOPSIS
.nf
.B #include <termkey.h>
.sp
.BI "termkey_result termkey_getkey(termkey_t *" tk ", termkey_key *" key );
.br
.BI "termkey_result termkey_getkey_force(termkey_t *" tk ", termkey_key *" key );
.fi
.sp
Link with \fI-ltermkey\fP.
.SH DESCRIPTION
\fBtermkey_getkey\fP attempts to retrieve a single keypress event from the buffer, and put it in the structure referred to by \fIkey\fP. If successful it will return \fBTERMKEY_RES_KEY\fP to indicate that the structure now contains a new keypress event. If nothing is in the buffer it will return \fBTERMKEY_RES_NONE\fP. If the buffer contains a partial keypress event which does not yet contain all the bytes required, it will return \fBTERMKEY_RES_AGAIN\fP. If no events are ready and the input stream is now closed, will return \fBTERMKEY_RES_EOF\fP.
.PP
\fBtermkey_getkey_force\fP is similar to \fBtermkey_getkey\fP but will not return \fBTERMKEY_RES_AGAIN\fP if a partial match is found. Instead, it will force an interpretation of the bytes, even if this means interpreting the start of an Escape-prefixed multi-byte sequence as a literal "Escape" key followed by normal letters.
.PP
Neither of these functions will block or perform any IO operations on the underlying filehandle. To use the instance in an asynchronous program, see \fBtermkey_advisereadable\fP(3). For a blocking call suitable for use in a synchronous program, use \fBtermkey_waitkey\fP(3) instead of \fBtermkey_getkey\fP().
.PP
The \fItermkey_key\fP structure is defined as follows:
.PP
.in +4n
.nf
typedef struct {
    termkey_type type;
    union {
        long           codepoint; /* TERMKEY_TYPE_UNICODE  */
        int            number;    /* TERMKEY_TYPE_FUNCTION */
        termkey_keysym sym;       /* TERMKEY_TYPE_KEYSYM   */
    } code;
    int modifiers;
    char utf8[7];
} termkey_key;
.fi
.in
.PP
The \fItype\fP field indicates the type of event, and determines which of the members of the \fIcode\fP union is valid. It will be one of the following constants:
.TP
.B TERMKEY_TYPE_UNICODE
a Unicode codepoint
.TP
.B TERMKEY_TYPE_FUNCTION
a numbered function key
.TP
.B TERMKEY_TYPE_KEYSYM
a symbolic key
.PP
The \fImodifiers\fP bitmask is composed of a bitwise-or of the constants \fBTERMKEY_KEYMOD_SHIFT\fP, \fBTERMKEY_KEYMOD_CTRL\fP and \fBTERMKEY_KEYMOD_ALT\fP.
.PP
The \fIutf8\fP field is only set on events whose \fItype\fP is \fBTERMKEY_TYPE_UNICODE\fP. It should not be read for other events.
.PP
To convert the \fIsym\fP to a symbolic name, see \fBtermkey_get_keyname\fP(3) function. It may instead be easier to convert the entire key event structure to a string, using \fBtermkey_snprint_key\fP(3).
.SH "RETURN VALUE"
\fBtermkey_getkey\fP() and \fBtermkey_getkey_force\fP() return one of the following constants:
.TP
.B TERMKEY_RES_NONE
No key event is ready.
.TP
.B TERMKEY_RES_KEY
A key event as been provided.
.TP
.B TERMKEY_RES_EOF
No key events are ready and the terminal has been closed, so no more will arrive.
.TP
.B TERMKEY_RES_AGAIN
No key event is ready yet, but a partial one has been found. This is only returned by \fBtermkey_getkey\fP(). To obtain the partial result even if it never completes, use \fBtermkey_getkey_force\fP().
.SH EXAMPLE
The following example program prints details of every keypress until the user presses "Ctrl-C". It demonstrates how to use the termkey instance in a typical \fBpoll\fP()-driven asynchronous program, which may include mixed IO with other file handles.
.PP
.in +4n
`while read LINE; do echo ".br"; echo "$LINE"; done <demo-async.c`
.in
.nf
.fi
.SH "SEE ALSO"
.BR termkey_new (3),
.BR termkey_advisereadable (3),
.BR termkey_waitkey (3),
.BR termkey_set_waittime (3),
.BR termkey_get_keyname (3),
.BR termkey_snprint_key (3)
EOF
