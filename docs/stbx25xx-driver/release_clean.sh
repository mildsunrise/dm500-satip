#!/bin/sh
rm -f *.scc *.bak *.local
rm -f $(find -name *.scc)
rm -f $(find -name *.dsp)
rm -f $(find -name *.bak)


dos2unix $(find -name "*.c" -type f ) $(find -name "*.h" -type f )  $(find -type f -name "*.make") $(find -type f -name "Makefile")    

