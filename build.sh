#!/bin/bash

CF="-I$(readlink -f .) -D_GNU_SOURCE=1 -O0 -pipe -Wall -Wextra -g"

DEPS=($(pkg-config --print-requires-private ecore-con elementary))
#echo "DEPENDENCIES: ${DEPS[@]}"
CFLAGS="$(pkg-config --static --cflags ${DEPS[@]} ecore-con elementary)"
#echo "DEPENDENCY CFLAGS: $CFLAGS"

LIBS="$(pkg-config --static --libs ${DEPS[@]} ecore-con elementary)"
#echo "DEPENDENCY LIBS: $LIBS"
#echo

link=0
compile=0

if [[ -f ./shotgun ]] ; then
	for x in *.h ui/*.h ; do
		if [[ "$x" -nt ./shotgun ]] ; then
			compile=1
			break;
		fi
	done
fi

for x in *.c ui/*.c  ; do
	[[ $compile == 0 && -f "${x/.c/.o}" && "$x" -ot "${x/.c/.o}" ]] && continue
#	echo "gcc -c $x -o ${x/.c/.o} $CFLAGS $CF || exit 1"
	echo "gcc -c $x -o ${x/.c/.o} || exit 1"
	(gcc -c $x -o "${x/.c/.o}" $CFLAGS $CF || exit 1)
	link=1
done

for x in *.cpp ; do
	[[ $compile == 0 && -f "${x/.cpp/.o}" && "$x" -ot "${x/.cpp/.o}" ]] && continue
#	echo "gcc -c $x -o ${x/.cpp/.o} $CFLAGS $CF || exit 1"
	echo "gcc -c $x -o ${x/.cpp/.o} || exit 1"
	(gcc -c $x -o "${x/.cpp/.o}" $CFLAGS $CF || exit 1)
	link=1
done

[[ $link == 0 ]] && exit 1
wait
#echo "g++ *.o -o shotgun -L/usr/lib -lc $LIBS" #pugixml.a
echo "g++ *.o -o shotgun" #pugixml.a
g++ *.o ui/*.o -o shotgun -L/usr/lib -lc $LIBS #pugixml.a
