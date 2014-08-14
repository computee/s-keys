#!/bin/sh

for i in test/*.c; do
    BNAME=$(basename $i .c)
	cc -Iinclude -O1 ./jump*.c $i -o ${BNAME} &&\
	./${BNAME}
done
