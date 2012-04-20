#!/bin/sh

temp=/tmp/data.$$

left=$temp/left
right=$temp/right

if [ $# -ne 2 ] ; then
    [ $# -gt 2 ] || echo too few
    [ $# -lt 2 ] || echo too many
    exit 1
fi

mkdir $temp

./$1 --name copper_graph --file copper.cu | sed -e 's,_[0-9a-f][0-9a-f]*,,g' > $left
shift
./$1 --name copper_graph --file copper.cu | sed -e 's,_[0-9a-f][0-9a-f]*,,g' > $right

if diff $left $right ; then
    echo $1 Passed '(Same Graph)'
    rm -rf $temp
    exit 0
else
    echo $1 Failed '(Different Graph)'
    rm -rf $temp
    exit 1
fi