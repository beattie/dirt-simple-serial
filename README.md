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
