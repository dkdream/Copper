#!/bin/sh

TEST=$$
CC=$1

echo '#include <stdio.h>' > $TEST.c
echo >> $TEST.c
./ascii2hex.x -lxxx ascii2hex.c >> $TEST.c
echo >> $TEST.c
echo 'int main(int argc, char **argv) { printf("%s", xxx); return 0; }' >> $TEST.c
$CC -o $TEST.x $TEST.c
./$TEST.x > $TEST.out
rm -f $TEST.x $TEST.c
if cmp ascii2hex.c $TEST.out ; then
    rm $TEST.out
    exit 0
else
    rm $TEST.out ./ascii2hex.x
    exit 1
fi


