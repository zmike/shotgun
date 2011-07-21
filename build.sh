#!/bin/bash

CF="-D_GNU_SOURCE=1 -O0 -pipe -Wall -Wextra -g"

DEPS=($(pkg-config --print-requires-private ecore-con))
echo "DEPENDENCIES: ${DEPS[@]}"
CFLAGS="$(pkg-config --static --cflags ${DEPS[@]} ecore-con)"
echo "DEPENDENCY CFLAGS: $CFLAGS"

LIBS="$(pkg-config --static --libs ${DEPS[@]} ecore-con)"
echo "DEPENDENCY LIBS: $LIBS"
echo

for x in *.c ; do
	[[ -f "${x/.c/.o}" && "$x" -ot "${x/.c/.o}" ]] && continue
	echo "gcc -c $x -o ${x/.c/.o} $CFLAGS $CF || exit 1"
	gcc -c $x -o "${x/.c/.o}" $CFLAGS $CF || exit 1
done

for x in *.cpp ; do
	[[ -f "${x/.cpp/.o}" && "$x" -ot "${x/.cpp/.o}" ]] && continue
	echo "gcc -c $x -o ${x/.cpp/.o} $CFLAGS $CF || exit 1"
	gcc -c $x -o "${x/.cpp/.o}" $CFLAGS $CF || exit 1
done

echo "g++ *.o -o shotgun -L/usr/lib -lc $LIBS" #pugixml.a
g++ *.o -o shotgun -L/usr/lib -lc $LIBS #pugixml.a
