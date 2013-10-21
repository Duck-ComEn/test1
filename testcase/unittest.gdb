# ---------------------------------------------------------------------------
define pass_if_match
# ---------------------------------------------------------------------------
if $arg0 != $arg1
failed$arg9
end
end

# ---------------------------------------------------------------------------
define pass_if_unmatch
# ---------------------------------------------------------------------------
if $arg0 == $arg1
failed$arg9
end
end

# ---------------------------------------------------------------------------
# initialize
# ---------------------------------------------------------------------------
set args config.uartcheck -ps
b main
run
call tcGetBuffer(1024 * 1024)
call tcGetTscb(1024 * 1024)
call set_task(0x101)
call resetTestScriptLog(1024 * 1024, 1)

# ---------------------------------------------------------------------------
# bTestResultFileName
# ---------------------------------------------------------------------------
call setParameterIntoTestScriptControlBlock("TEST_PARAM  bTestResultFileName                             \"/result/00000001.000\"", tcUnitTestTscb)
pass_if_match $ 0

call setParameterIntoTestScriptControlBlock("GUEST_PARAM  bTestResultFileName                            \"/result/00000001.000\"", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  bTestResultFileName        \b                   \"/result/00000001.000\"", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("   TEST_PARAM  bTestResultFileName                             \"/result/00000001.000\"", tcUnitTestTscb)
pass_if_match $ 0

call setParameterIntoTestScriptControlBlock("   ", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM   ", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  bTestResultFileName                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \"/result/00000001.000\"", tcUnitTestTscb)
pass_if_match $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM bTestResultFileName", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  bTestResultFileName                             \"\"", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  bTestResultFileName                             \"88888888888888888888888888888888888888888888888888888888888888888888888888888888888888\
888888888888888888888888888888888888888888\"", tcUnitTestTscb)
pass_if_match $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  bTestResultFileName                             \"99999999999999999999999999999999999999999999999999999999999999999999999999999999999999\
9999999999999999999999999999999999999999999\"", tcUnitTestTscb)
pass_if_unmatch $ 0

# ---------------------------------------------------------------------------
# bTestPfTableFileName
# ---------------------------------------------------------------------------
call setParameterIntoTestScriptControlBlock("TEST_PARAM  bTestPfTableFileName                             \"88888888888888888888888888888888888888888888888888888888888888888888888888888888888888\
888888888888888888888888888888888888888888\"", tcUnitTestTscb)
pass_if_match $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  bTestPfTableFileName                             \"99999999999999999999999999999999999999999999999999999999999999999999999999999999999999\
9999999999999999999999999999999999999999999\"", tcUnitTestTscb)
pass_if_unmatch $ 0

# ---------------------------------------------------------------------------
# dwDriveTestIdentifyTimeoutSec
# ---------------------------------------------------------------------------
call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  0000:00:00", tcUnitTestTscb)
pass_if_match $ 0
pass_if_match tcUnitTestTscb->dwDriveTestIdentifyTimeoutSec 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  0000:00:30", tcUnitTestTscb)
pass_if_match $ 0
pass_if_match tcUnitTestTscb->dwDriveTestIdentifyTimeoutSec 30

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  9999:99:99", tcUnitTestTscb)
pass_if_match $ 0
pass_if_match tcUnitTestTscb->dwDriveTestIdentifyTimeoutSec 36002439

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  0000:0:0", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  0000: 0: 0", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  10000:00:00", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  0000:100:00", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  0000:00:100", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  0000:-5:00", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  0x00:00:00", tcUnitTestTscb)
pass_if_unmatch $ 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwDriveTestIdentifyTimeoutSec                  0000:0?:00", tcUnitTestTscb)
pass_if_unmatch $ 0

# ---------------------------------------------------------------------------
# dwTesterLogSizeKByte
# ---------------------------------------------------------------------------
call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwTesterLogSizeKByte                           64", tcUnitTestTscb)
pass_if_match $ 0
pass_if_match tcUnitTestTscb->dwTesterLogSizeKByte 64

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwTesterLogSizeKByte                           0", tcUnitTestTscb)
pass_if_match $ 0
pass_if_match tcUnitTestTscb->dwTesterLogSizeKByte 0

call setParameterIntoTestScriptControlBlock("TEST_PARAM  dwTesterLogSizeKByte                           16000", tcUnitTestTscb)
pass_if_unmatch $ 0

# ---------------------------------------------------------------------------
# ts_isalnum_mem
# ---------------------------------------------------------------------------
call ts_isalnum_mem("12345678", 8)
pass_if_unmatch $ 0

call ts_isalnum_mem("", 0)
pass_if_unmatch $ 0

call ts_isalnum_mem("AB\nD", 4)
pass_if_match $ 0

call ts_isalnum_mem("AB D", 4)
pass_if_match $ 0

# ---------------------------------------------------------------------------
# sec2hhhhmmssTimeFormat
# ---------------------------------------------------------------------------
call sec2hhhhmmssTimeFormat(0, tcUnitTestBuffer)
call strcmp(tcUnitTestBuffer, "0000:00:00")
pass_if_match $ 0

call sec2hhhhmmssTimeFormat(59, tcUnitTestBuffer)
call strcmp(tcUnitTestBuffer, "0000:00:59")
pass_if_match $ 0

call sec2hhhhmmssTimeFormat(60, tcUnitTestBuffer)
call strcmp(tcUnitTestBuffer, "0000:01:00")
pass_if_match $ 0

call sec2hhhhmmssTimeFormat(3599, tcUnitTestBuffer)
call strcmp(tcUnitTestBuffer, "0000:59:59")
pass_if_match $ 0

call sec2hhhhmmssTimeFormat(3600, tcUnitTestBuffer)
call strcmp(tcUnitTestBuffer, "0001:00:00")
pass_if_match $ 0

call sec2hhhhmmssTimeFormat(0x7fffffff, tcUnitTestBuffer)
call strcmp(tcUnitTestBuffer, "9999:59:59")
pass_if_match $ 0

call sec2hhhhmmssTimeFormat(0x80000000, tcUnitTestBuffer)
call strcmp(tcUnitTestBuffer, "9999:59:59")
pass_if_match $ 0

call sec2hhhhmmssTimeFormat(0xffffffff, tcUnitTestBuffer)
call strcmp(tcUnitTestBuffer, "9999:59:59")
pass_if_match $ 0

# ---------------------------------------------------------------------------
# memcpy
# ---------------------------------------------------------------------------
call strcpy(tcUnitTestBuffer, "01234567")
call memcpyWith16bitLittleEndianTo8bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 8)
call strcmp(tcUnitTestBuffer, "10325476")
pass_if_match $ 0
pass_if_unmatch tcUnitTestBuffer 0

call memcpyWith16bitLittleEndianTo8bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 1)
pass_if_match $ 0


call strcpy(tcUnitTestBuffer, "01234567")
call memcpyWith32bitLittleEndianTo8bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 8)
call strcmp(tcUnitTestBuffer, "32107654")
pass_if_match $ 0
pass_if_unmatch tcUnitTestBuffer 0

call memcpyWith32bitLittleEndianTo8bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 1)
pass_if_match $ 0


call strcpy(tcUnitTestBuffer, "01234567")
call memcpyWith16bitLittleEndianTo16bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 8)
call strcmp(tcUnitTestBuffer, "01234567")
pass_if_match $ 0
pass_if_unmatch tcUnitTestBuffer 0

call memcpyWith16bitLittleEndianTo16bitConversion(&tcUnitTestBuffer[1], tcUnitTestBuffer, 8)
pass_if_match $ 0

call memcpyWith16bitLittleEndianTo16bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 1)
pass_if_match $ 0


call strcpy(tcUnitTestBuffer, "01234567")
call memcpyWith16bitBigEndianTo16bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 8)
call strcmp(tcUnitTestBuffer, "10325476")
pass_if_match $ 0
pass_if_unmatch tcUnitTestBuffer 0

call memcpyWith16bitBigEndianTo16bitConversion(&tcUnitTestBuffer[1], tcUnitTestBuffer, 8)
pass_if_match $ 0

call memcpyWith16bitBigEndianTo16bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 1)
pass_if_match $ 0


call strcpy(tcUnitTestBuffer, "01234567")
call memcpyWith32bitLittleEndianTo32bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 8)
call strcmp(tcUnitTestBuffer, "01234567")
pass_if_match $ 0
pass_if_unmatch tcUnitTestBuffer 0

call memcpyWith32bitLittleEndianTo32bitConversion(&tcUnitTestBuffer[1], tcUnitTestBuffer, 8)
pass_if_match $ 0

call memcpyWith32bitLittleEndianTo32bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 1)
pass_if_match $ 0


call strcpy(tcUnitTestBuffer, "01234567")
call memcpyWith32bitBigEndianTo32bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 8)
call strcmp(tcUnitTestBuffer, "32107654")
pass_if_match $ 0
pass_if_unmatch tcUnitTestBuffer 0

call memcpyWith32bitBigEndianTo32bitConversion(&tcUnitTestBuffer[1], tcUnitTestBuffer, 8)
pass_if_match $ 0

call memcpyWith32bitBigEndianTo32bitConversion(tcUnitTestBuffer, tcUnitTestBuffer, 1)
pass_if_match $ 0

call tcBadHeadUnitTest()
pass_if_match $ 0

