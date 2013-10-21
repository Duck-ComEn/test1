#!/bin/bash -x

echo "------------------------------------------------------------------" >/dev/null
APP_NAME_OPTIMUS=wrapperSRST_withthreads_optimus
echo " building ${APP_NAME_OPTIMUS}"                                      >/dev/null
echo "------------------------------------------------------------------" >/dev/null

LIB_NAME_OPTIMUS=../libsi_optimus.a

SO_OPTIMUS="-L /usr/optimus/run/lib -lCellEnvHGSTOptimus_20-2.6.18-164.el5  -lCellRIMHGSTSerialOptimus_1-2.6.18-164.el5"

INCLUDE_COMMON=../
INCLUDE_OPTIMUS=/usr/optimus/dev/include

CPPFLAGS_OPTIMUS="   -Wall -fPIC -D_REENTRANT  -I$INCLUDE_COMMON -I$INCLUDE_OPTIMUS    -DOPTIMUS"

##OUTPUT_DIR=../../../bin-2.6.18-164.el5


#--- actions--------------------------------------
rm -f $APP_NAME_OPTIMUS

g++ -c $CPPFLAGS_OPTIMUS *.cpp
g++ -o $APP_NAME_OPTIMUS *.o  $LIB_NAME_OPTIMUS $SO_OPTIMUS

##cp $APP_NAME_OPTIMUS $OUTPUT_DIR 
rm -f *.o



