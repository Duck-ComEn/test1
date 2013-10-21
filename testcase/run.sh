#!/bin/sh
#
# ----------------------------------------------------------------------
# initialize
# ----------------------------------------------------------------------
rm -f tclogger.*.core.* core.*
rm -f tclogger.*.testcase*
rm -f tclogger.*.log*
rm -f 0000????.000 0000????.txt
rm -f unittest.pdf
#
# ----------------------------------------------------------------------
# test run (check program for desktop)
# ----------------------------------------------------------------------
#
#./testcase config.chktmpjpt01 config.chktmpjpt02 -cschamber.chktmpjpt        -nh &
#./testcase config.chktmpjpt01 config.chktmpjpt02 -cschamber.chktmpjpt_resume -nh &
./testcase config.chkuatjpt01 config.chkuatjpt02 -cschamber.chknop           -nh &
#./testcase config.chkvoljpt01 config.chkvoljpt02 -cschamber.chknop           -nh &
#
# ----------------------------------------------------------------------
# test run (check program for mobile)
# ----------------------------------------------------------------------
#
#./testcase config.chktmpptb01 -cschamber.chktmpptb -nh &
#sleep 2
#./testcase config.chktmpptb02 -cschamber.chktmpptb_resume -nh &
#
#./testcase config.chkuatptb01 -cschamber.chknop -nh &
#sleep 2
#./testcase config.chkuatptb02 -cschamber.chknop -nh &
#
#./testcase config.chkvolptb01 -cschamber.chknop -nh &
#sleep 2
#./testcase config.chkvolptb02 -cschamber.chknop -nh &
#
# ----------------------------------------------------------------------
# test run (short srst)
# ----------------------------------------------------------------------
#
#./testcase config.jpt01 config.jpt02 -cssequence.jpt -nh &
#
#
# ./testcase config.ptb01 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb02 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb03 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb04 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb05 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb06 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb07 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb08 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb09 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb10 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb11 -nh -bd -cssequence.ptb &
# sleep 2
# ./testcase config.ptb12 -nh -bd -cssequence.ptb &
#
#
# ----------------------------------------------------------------------
# test run (UNIT_TEST)
# ----------------------------------------------------------------------
#./testcase unittest.cfg2 unittest.cfg3 -ps -csunittest.seq
#./testcase -lf 00000001.000 > 00000001.txt
#./testcase -lf 00000002.000 > 00000002.txt
#
# ----------------------------------------------------------------------
# memory profile
# ----------------------------------------------------------------------
#valgrind --tool=massif ./testcase unittest.cfg2 unittest.cfg3 -csunittest.seq -ns 1>tmp.txt 2>tmp.txt
#ps2pdf massif.*.ps unittest.pdf
#
# ----------------------------------------------------------------------
# memory check
# ----------------------------------------------------------------------
#valgrind --tool=memcheck --leak-check=full --gen-suppressions=all --suppressions=unittest.sup ./testcase unittest.cfg2 unittest.cfg3 -csunittest.seq -ns 1> tmp.txt
#
# ----------------------------------------------------------------------
# finalize
# ----------------------------------------------------------------------
#rm -f massif.*.ps
#rm -f massif.*.txt
#rm -f tmp.txt
#