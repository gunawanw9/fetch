#!/bin/sh
# $Id$

PROGNAME=testfetch
FILES='	aclocal.m4
	config.cache
	config.cross.cache
	config.log
	config.status
	configure
	Makefile
	Makefile.in
	*.o
	Makefile
	Makefile.in
	$PROGNAME
	$PROGNAME.exe
	std*.txt'

test -w configure && (if (./configure) then make distclean; fi)
for file in $FILES; do test -w $file && rm -f $file; done
test -w .deps && rm -rf .deps
