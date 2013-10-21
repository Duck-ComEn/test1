#!/bin/bash -x

echo "------------------------------------------------------------------" >/dev/null
APP_NAME_XCAL=wrapperSRST_nothreads_xcal
echo " building ${APP_NAME_XCAL}"                                         >/dev/null
echo "------------------------------------------------------------------" >/dev/null

LIB_NAME_XCAL=../libsi_xcal.a

SO_XCAL="-L /usr/xcal/run/lib       -lCellEnv_20-2.6.18-164.el5             -lCellRIMHGSTSerial3DP_1-2.6.18-164.el5"

INCLUDE_COMMON=../
INCLUDE_XCAL=/usr/xcal/dev/include

CPPFLAGS_XCAL="   -Wall -fPIC -D_REENTRANT  -I$INCLUDE_COMMON -I$INCLUDE_XCAL    -DXCALIBRE"

##OUTPUT_DIR=../../../bin-2.6.18-164.el5


#---xcalibre actions--------------------------------------
rm -f $APP_NAME_XCAL

g++ -c $CPPFLAGS_XCAL *.cpp
g++ -o $APP_NAME_XCAL *.o  $LIB_NAME_XCAL $SO_XCAL

##cp $APP_NAME_XCAL $OUTPUT_DIR 
rm -f *.o




