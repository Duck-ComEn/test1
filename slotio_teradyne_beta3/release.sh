#!/bin/bash

############################################################
# Release wrapper-api
############################################################
TARG="/mnt/hgfs/buildshare/Hitachi2/Release/wrapper-api"
chmod 777 ${TARG}/*
cp -f * ${TARG}/.
#
# cleanup release area
#
rm -f ${TARG}/*~
rm -f ${TARG}/*.o
rm -f ${TARG}/*.a
rm -f ${TARG}/t
rm -f ${TARG}/t[0-9].*

############################################################
# Release neptune_sio2
############################################################

cd ../neptune_sio2-1.0/

TARG="/mnt/hgfs/buildshare/Hitachi2/Release/neptune_sio2-1.0"
chmod 777 ${TARG}/*
cp -f * ${TARG}/.
#
# cleanup release area
#
rm -f ${TARG}/*~
rm -f ${TARG}/*.o
rm -f ${TARG}/*.a
rm -f ${TARG}/t
rm -f ${TARG}/t[0-9].*

############################################################
# Release sockTunnel
############################################################
cd ../sockTunnel-1.0

TARG="/mnt/hgfs/buildshare/Hitachi2/Release/Application"
#
chmod 777 ${TARG}/*
cp -f * ${TARG}/.
#
chmod 777 ${TARG}/marshal/*
cp -f marshal/* ${TARG}/marshal/.
#
# cleanup release area
#
rm -f ${TARG}/*~
rm -f ${TARG}/*.o
#rm -f ${TARG}/*.a
rm -f ${TARG}/t
rm -f ${TARG}/t[0-9].*
rm -f ${TARG}/sockTunnel.*

# Done
cd ../wrapper-api/
