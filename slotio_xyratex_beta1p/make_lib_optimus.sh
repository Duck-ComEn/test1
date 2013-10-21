#!/bin/bash -x

echo "------------------------------------------------------------------" >/dev/null
LIB_NAME_OPTIMUS=libsi_optimus.a
echo " make_lib_xcal.sh: building ${LIB_NAME_OPTIMUS}"                    >/dev/null
echo "------------------------------------------------------------------" >/dev/null


INCLUDE_COMMON=./
INCLUDE_OPTIMUS=/usr/optimus/dev/include

CPPFLAGS_OPTIMUS="   -g -Werror -Wall -fPIC -D_REENTRANT -I$INCLUDE_COMMON -I$INCLUDE_OPTIMUS    -DOPTIMUS"

##OUTPUT_DIR=..


rm -f $LIB_NAME_OPTIMUS
g++ -c $CPPFLAGS_OPTIMUS *.cpp
ar -rc $LIB_NAME_OPTIMUS *.o
##cp $LIB_NAME_OPTIMUS $OUTPUT_DIR 
rm -f *.o
