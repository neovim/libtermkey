# vim:ft=nroff
cat <<EOF
.TH TERMKEY_GETKEY 3
.SH NAME
termkey_getkey, termkey_getkey_force \- retrieve the next key event
.SH SYNOPSIS
.nf
.B #include <termkey.h>
.sp
.BI "TermKeyResult termkey_getkey(TermKey *" tk ", TermKeyKey *" key );
.BI "TermKeyResult termkey_getkey_force(TermKey *" tk ", TermKeyKey *" key );
.fi
.sp
Link with \fI-ltermkey\fP.
.SH DESCRIPTION
\fBtermkey_getkey\fP() attempts to retrieve a single keypress event from the buffer, and put it in the structure referred to by \fIkey\fP. It returns one of the following values:
.in
.TP
.B TERMKEY_RES_KEY
a complete keypress was removed from the buffer, and has been placed in the \fIkey\fP structure.
.TP
.B TERMKEY_RES_AGAIN
a partial keypress event was found in the buffer, but it does not yet contain all the bytes required. An indication of what \fBtermkey_getkey_force\fP(3) would return has been placed in the \fIkey\fP structure.
.TP
.B TERMKEY_RES_NONE
no bytes are waiting in the buffer.
.TP
.B TERMKEY_RES_EOF
 no bytes are ready and the input stream is now closed.
.PP
\fBtermkey_getkey_force\fP() is similar to \fBtermkey_getkey\fP() but will not return \fBTERMKEY_RES_AGAIN\fP if a partial match is found. Instead, it will force an interpretation of the bytes, even if this means interpreting the start of an Escape-prefixed multi-byte sequence as a literal "Escape" key followed by normal letters.
.PP
Neither of these functions will block or perform any IO operations on the underlying filehandle. To use the instance in an asynchronous program, see \fBtermkey_advisereadable\fP(3). For a blocking call suitable for use in a synchronous program, use \fBtermkey_waitkey\fP(3) instead of \fBtermkey_getkey\fP().
.PP
The \fITermKeyKey\fP structure is defined as follows:
.PP
.in +4n
.nf
typedef struct {
    TermKeyType type;
    union {
        long       codepoint; /* TERMKEY_TYPE_UNICODE  */
        int        number;    /* TERMKEY_TYPE_FUNCTION */
        TermKeySym sym;       /* TERMKEY_TYPE_KEYSYM   */
        char       mouse[4]   /* TERMKEY_TYPE_MOUSE    */
    } code;
    int modifiers;
    char utf8[7];
} TermKeyKey;
.fi
.in
.PP
The \fItype\fP field indicates the type of event, and determines which of the members of the \fIcode\fP union is valid. It will be one of the following constants:
.TP
.B TERMKEY_TYPE_UNICODE
a Unicode codepoint. This value indicates that \fIcode.codepoint\fP is valid, and will contain the codepoint number of the keypress. In Unicode mode (if the \fBTERMKEY_FLAG_UTF8\fP bit is set) this will be its Unicode character number. In raw byte mode, this will contain a single 8-bit byte.
.TP
.B TERMKEY_TYPE_FUNCTION
a numbered function key. This value indicates that \fIcode.number\fP is valid, and contains the number of the numbered function key.
.TP
.B TERMKEY_TYPE_KEYSYM
a symbolic key. This value indicates that \fIcode.sym\fP is valid, and contains the symbolic key value. This is an opaque value which may be passed to \fBtermkey_get_keyname\fP(3).
.TP
.B TERMKEY_TYPE_MOUSE
a mouse button press, release, or movement. The \fIcode.mouse\fP array should be considered opaque. Use \fBtermkey_interpret_mouse\fP(3) to interpret it.
.PP
The \fImodifiers\fP bitmask is composed of a bitwise-or of the constants \fBTERMKEY_KEYMOD_SHIFT\fP, \fBTERMKEY_KEYMOD_CTRL\fP and \fBTERMKEY_KEYMOD_ALT\fP.
.PP
The \fIutf8\fP field is only set on events whose \fItype\fP is \fBTERMKEY_TYPE_UNICODE\fP. It should not be read for other events.
.PP
To convert the \fIsym\fP to a symbolic name, see \fBtermkey_get_keyname\fP(3) function. It may instead be easier to convert the entire key event structure to a string, using \fBtermkey_strfkey\fP(3).
.SH "RETURN VALUE"
\fBtermkey_getkey\fP() returns an enumeration of one of \fBTERMKEY_RES_KEY\fP, \fBTEMRKEY_RES_AGAIN\fP, \fBTERMKEY_RES_NONE\fP or \fBTERMKEY_RES_EOF\fP. \fBtermkey_getkey_force\fP() returns one of the above, except for \fBTERMKEY_RES_AGAIN\fP.
.SH EXAMPLE
The following example program prints details of every keypress until the user presses "Ctrl-C". It demonstrates how to use the termkey instance in a typical \fBpoll\fP()-driven asynchronous program, which may include mixed IO with other file handles.
.PP
.in +4n
.nf
EOF
sed "s/\\\\/\\\\\\\\/g" demo-async.c
cat <<EOF
.in
.fi
.SH "SEE ALSO"
.BR termkey_new (3),
.BR termkey_advisereadable (3),
.BR termkey_waitkey (3),
.BR termkey_set_waittime (3),
.BR termkey_get_keyname (3),
.BR termkey_interpret_mouse (3),
.BR termkey_strfkey (3),
.BR termkey_keycmp (3)
EOF
