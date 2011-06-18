#!/bin/bash

CF="-O0 -pipe -Wall -Wextra -g"

DEPS=($(pkg-config --print-requires-private ecore-con))
echo "DEPENDENCIES: ${DEPS[@]}"
CFLAGS="$(pkg-config --static --cflags ${DEPS[@]} ecore-con)"
#CFLAGS+=" -I/usr/include/sasl"
echo "DEPENDENCY CFLAGS: $CFLAGS"

LIBS="$(pkg-config --static --libs ${DEPS[@]} ecore-con)"
if (echo "$LIBS" | grep gnutls &> /dev/null) ; then
	LIBS+=" $(pkg-config --static --libs gnutls)"
fi
#LIBS+=" -lsasl2"
echo "DEPENDENCY LIBS: $LIBS"
echo

echo "g++ -c pugixml.cpp -o pugixml.o || exit 1"
g++ -c pugixml.cpp -o pugixml.o || exit 1

echo "gcc -c cdecode.c -o cdecode.o || exit 1"
gcc -c cdecode.c -o cdecode.o || exit 1
echo "gcc -c cencode.c -o cencode.o || exit 1"
gcc -c cencode.c -o cencode.o || exit 1
echo "gcc -c shotgun_utils.c -o shotgun_utils.o || exit 1"
gcc -c shotgun_utils.c -o shotgun_utils.o || exit 1


echo "g++ -c xml.cpp -o xml.o $CFLAGS $CF || exit 1"
g++ -c xml.cpp -o xml.o $CFLAGS $CF || exit 1
echo "gcc -c getpass_x.c -o getpass_x.o $CF || exit 1"
gcc -c getpass_x.c -o getpass_x.o $CF || exit 1
echo "gcc -c shotgun.c -o shotgun.o $CFLAGS $CF || exit 1"
gcc -c shotgun.c -o shotgun.o $CFLAGS $CF || exit 1
echo "gcc -c login.c -o login.o $CFLAGS $CF || exit 1"
gcc -c login.c -o login.o $CFLAGS $CF || exit 1
echo "gcc -c messaging.c -o messaging.o $CFLAGS $CF || exit 1"
gcc -c messaging.c -o messaging.o $CFLAGS $CF || exit 1
echo "gcc -c iq.c -o iq.o $CFLAGS $CF || exit 1"
gcc -c iq.c -o iq.o $CFLAGS $CF || exit 1

echo "g++ *.o -o shotgun -L/usr/lib -lc $LIBS" #pugixml.a
g++ *.o -o shotgun -L/usr/lib -lc $LIBS #pugixml.a
