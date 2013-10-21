astyle -lcs2pn ../testscript/*.[ch] ../signalhandler/*.[ch] ../hostio/*.[ch] ../chamberscript/*.[ch] ../testcase/*.[ch]
echo "----------------------------------------------------------------------"
cd ../slotio
make clean
#make xcal
#make optimus
make $1
echo "----------------------------------------------------------------------"
cd ../testscript
make clean
make
echo "----------------------------------------------------------------------"
cd ../signalhandler
make clean
make
echo "----------------------------------------------------------------------"
cd ../chamberscript
make clean
make
echo "----------------------------------------------------------------------"
cd ../hostio
make clean
make
echo "----------------------------------------------------------------------"
cd ../testcase
make clean
make
