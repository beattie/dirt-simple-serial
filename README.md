dirt-simple-serial
==================
Dirt simple serial port program for Linux.  I really miss cu, but
it seems to be lostin th edust of history.  I rather liked gtkterm,
especially once it got logging, but lately I'e become annoyed with it's
lackadaisical handling (or lack of) window size.

Frankly with a windowing system and xterm or gnome-terminal or whatever
you use for command line the really is no need to include window and
display in the program, even logging can be handled by script.  With all
that, this program has almost all you coud want in a program to do serial
port conections.  At least it fills my needs.

If it works for you thats nice.  its one source file it supports Linux,
Probably not FreeBSD or Unix wiht out some tweaking.  No Autotools,
no configure script.  Like I said, dirt simple.

Documentation:
Takes one or two arguments, a serial port device name (for /dev/ttyS0
can be /dev/ttySO, or ttyS0 or S0, will try all three).  If no speed
specified it will leav the speed unchanged (probably not very useful).
Once connected commands a prefixed with Ctrl-A.  To send a Ctrl-A to
the serial port type two in a row.  f (lowercase f) disables hardware
flow control, F enables it.  T raises DTR, t drops it.  R raises RTS,
r drops it (if flow control is disabled).  ? prints a help message. any
of th efollowing close the port, restore the original state and exits:
q, Q, x, X, Ctrl-Q, Ctr-X.  Thats pretty much it.
