#!/bin/bash

DEPS=($(pkg-config --print-requires-private ecore-con))
echo "${DEPS[@]}"
CFLAGS="$(pkg-config --static --cflags ${DEPS[@]} ecore-con)"
echo "$CFLAGS"

LIBS="$(pkg-config --static --libs ${DEPS[@]} ecore-con)"
echo "$LIBS"
if (echo "$LIBS" | grep gnutls &> /dev/null) ; then
	LIBS+=" $(pkg-config --static --libs gnutls)"
fi
g++ -c pugixml.cpp -o pugixml.o || exit 1
ar cru pugixml.a pugixml.o
ranlib pugixml.a

g++ -c xml.cpp -o xml.o $CFLAGS -O0 -pipe -Wall -Wextra -g || exit 1
gcc -c shotgun.c -o shotgun.o $CFLAGS -O0 -pipe -Wall -Wextra -g || exit 1

g++ xml.o shotgun.o -o shotgun -L/usr/lib -lc $LIBS pugixml.a
