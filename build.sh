#!/bin/bash

CF="-I$(readlink -f .) -D_GNU_SOURCE=1 -O0 -pipe -Wall -Wextra -g -I$(readlink -f src/include) -DHAVE_ECORE_X"

#DEPS=($(pkg-config --print-requires-private ecore-con ecore-x elementary))
#echo "DEPENDENCIES: ${DEPS[@]}"
CFLAGS="$(pkg-config --cflags ${DEPS[@]} ecore-con ecore-x elementary)"
#echo "DEPENDENCY CFLAGS: $CFLAGS"

LIBS="$(pkg-config --libs ${DEPS[@]} ecore-con ecore-x elementary)"
#echo "DEPENDENCY LIBS: $LIBS"
#echo

link=0
compile=0

if [[ -f ./shotgun ]] ; then
	for x in *.h src/{bin,include,lib}/*.h ; do
		if [[ "$x" -nt ./shotgun ]] ; then
			compile=1
			break;
		fi
	done
fi

for x in src/lib/*.c src/bin/*.c  ; do
	[[ $compile == 0 && -f "${x/.c/.o}" && "$x" -ot "${x/.c/.o}" ]] && continue
#	echo "gcc -c $x -o ${x/.c/.o} $CFLAGS $CF || exit 1"
	echo "gcc $x"
	(gcc -c $x -o "${x/.c/.o}" $CFLAGS $CF || exit 1)&
	link=1
done

for x in src/lib/*.cpp ; do
	[[ $compile == 0 && -f "${x/.cpp/.o}" && "$x" -ot "${x/.cpp/.o}" ]] && continue
#	echo "gcc -c $x -o ${x/.cpp/.o} $CFLAGS $CF || exit 1"
	echo "gcc $x"
	(gcc -c $x -o "${x/.cpp/.o}" $CFLAGS $CF || exit 1)&
	link=1
done

[[ $link == 0 ]] && exit 1
wait
#echo "g++ *.o -o shotgun -L/usr/lib -lc $LIBS" #pugixml.a
echo "g++ *.o -o shotgun" #pugixml.a
g++ src/lib/*.o src/bin/*.o -o shotgun -L/usr/lib -lc $LIBS #pugixml.a
