#!/bin/sh

uncrustify -c ../uncrustify.cfg --replace --no-backup *.h *.cpp *.c
