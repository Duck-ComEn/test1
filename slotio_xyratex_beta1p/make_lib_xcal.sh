#!/bin/bash -x

echo "------------------------------------------------------------------" >/dev/null
LIB_NAME_XCAL=libsi_xcal.a
echo " make_lib_xcal.sh: building ${LIB_NAME_XCAL}"                       >/dev/null
echo "------------------------------------------------------------------" >/dev/null


INCLUDE_COMMON=./
INCLUDE_XCAL=/usr/xcal/dev/include

CPPFLAGS_XCAL="   -g -Werror -Wall -fPIC -D_REENTRANT -I$INCLUDE_COMMON -I$INCLUDE_XCAL    -DXCALIBRE"

##OUTPUT_DIR=..


rm -f $LIB_NAME_XCAL
g++ -c $CPPFLAGS_XCAL *.cpp
ar -rc $LIB_NAME_XCAL *.o
##cp $LIB_NAME_XCAL $OUTPUT_DIR 
rm -f *.o



