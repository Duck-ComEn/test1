#include "ts.h"

#define IGNORE_QGET_DEBUG

/**
 * <pre>
 * Description:
 *   Report drive id to host.
 * Example:
 *   S[09000000]LTSServerS[10000000]setSerial:S[12000000]XXXXXXOOOOOOOOL[CCCCCCCC]
 *   S[09000000]LTSServerS[15000000]setGradeSerialWithFlag:S[14000000] count until here
 *   [ ] is bynaly 00 count 1byte.
 *     XXXXXX    Grading ID
 *     OOOOOOOO  SN
 *     CCCCCCCC  Slot (always 01000000 except TRITON-u)
 * </pre>
 *****************************************************************************/
void reportHostId(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Dword dwLength = 0;
  Byte bBuf[256];
  Byte *bBufLastPointer = &bBuf[0];
  ZQUE *que = NULL;
  Byte *bDataLastPointer = NULL;
  Byte *bMessage = NULL;
  Dword ctrlPacketLength = 0;

  ts_printf(__FILE__, __LINE__, "reportHostId");

  dwLength = 0;
  dwLength += sprintf(&bBufLastPointer[dwLength], "%-6.6s%-8.8s",
                      &tscb->bDriveMfgIdOnLabel[0],
                      &tscb->bDriveSerialNumber[0]
                     );


  /* create que */
  ctrlPacketLength = 39;
  que = ts_qalloc(dwLength + ctrlPacketLength);

  bDataLastPointer = que->bData;

  bMessage = LOGIC_CARD_SERVER_STR;

  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);


  bMessage = MESSAGE_ID_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = &bBuf[0];
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));  /* ID + Serial */
  bDataLastPointer += strlen(bMessage);

  *bDataLastPointer++ = INTEGER_SEPARATOR;
  *bDataLastPointer++ = 0x01;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;

  /* set header info */
  que->wDID = tscb->wHostID;
  que->wSID = (Word)MY_TASK_ID;
  que->wLgth = (Byte *)bDataLastPointer - &que->bData[0];
  que->bType = ZCmd;
  que->bAdrs = MY_CELL_NO + 1;
  que->bP[0] = 0x00;
  que->bP[1] = 0x00;
  que->bP[2] = 0x00;
  que->bP[3] = 0x00;

  /* send queue */
  putBinaryDataDump(&que->bData[0], 0, que->wLgth);
  if (tccb.optionNoHostCommunication) {
    ts_qput((QUE *)que);
  } else {
    hi_qput((QUE *)que);
  }
}

/**
 * <pre>
 * Description:
 *   Report drive grading id to host.
 * Example:
 *   S[09000000]LTSServerS[15000000]setGradeSerialWithFlag:S[14000000]XXXXXXOOOOOOOOFFL[CCCCCCCC]
 *   S[09000000]LTSServerS[15000000]setGradeSerialWithFlag:S[14000000] count until here
 *   [ ] is bynaly 00 count 1byte.
 *     XXXXXX    Grading ID
 *     OOOOOOOO  SN
 *     CCCCCCCC  Slot (always 01000000 except TRITON-u)
 * </pre>
 *****************************************************************************/
void reportHostGradingId(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Dword dwLength = 0;
  Byte bBuf[256];
  Byte *bBufLastPointer = &bBuf[0];
  ZQUE *que = NULL;
  Byte *bDataLastPointer = NULL;
  Byte *bMessage = NULL;
  Dword ctrlPacketLength = 0;

  ts_printf(__FILE__, __LINE__, "reportHostGradingId");

  dwLength = 0;
  if (memcmp(tscb->bDriveMfgIdOnLabel, tscb->bDrivePass2Id, 6)) {
    dwLength += sprintf(&bBufLastPointer[dwLength], "%-6.6s%-8.8s01",
                        &tscb->bDrivePass2Id[0],
                        &tscb->bDriveSerialNumber[0]
                       );
  } else {
    dwLength += sprintf(&bBufLastPointer[dwLength], "%-6.6s%-8.8s00",
                        &tscb->bDriveMfgIdOnLabel[0],
                        &tscb->bDriveSerialNumber[0]
                       );
  }

  /* create que */
  ctrlPacketLength = 47;
  que = ts_qalloc(dwLength + ctrlPacketLength);

  bDataLastPointer = que->bData;


  bMessage = LOGIC_CARD_SERVER_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  /*  if(tscb->dwDriveGradingFlag){*/        /* This if sentense is no need, because this command send with flag. 2010.11.29 */
  bMessage = MESSAGE_GRADING_ID_STR_WITHFLAG;

  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = &bBuf[0];
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));  /* ID + Serial */
  bDataLastPointer += strlen(bMessage);

  *bDataLastPointer++ = INTEGER_SEPARATOR;
  *bDataLastPointer++ = 0x01;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;

  /* set header info */
  que->wDID = tscb->wHostID;
  que->wSID = (Word)MY_TASK_ID;
  que->wLgth = (Byte *)bDataLastPointer - &que->bData[0];
  que->bType = ZCmd;
  que->bAdrs = MY_CELL_NO + 1;
  que->bP[0] = 0x00;
  que->bP[1] = 0x00;
  que->bP[2] = 0x00;
  que->bP[3] = 0x00;


  /* send queue */
  putBinaryDataDump(&que->bData[0], 0, que->wLgth);
  if (tccb.optionNoHostCommunication) {
    ts_qput((QUE *)que);
  } else {
    hi_qput((QUE *)que);
  }
}
/**
 * <pre>
 * Description:
 *   Report drive TestTimeout to Host.
 * Example:
 *   S[09000000]LTSServerS[15000000]setTestTimeout:S[05000000]XXXXXX
 *   [ ] is bynaly 00 count 1byte.
 *     XXXXXX    Timeout (min , ASCII,decimal)
 * </pre>
 *****************************************************************************/
void reportHostTestTimeOut(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Dword dwLength = 0;
  Byte bBuf[256];
  Byte *bBufLastPointer = &bBuf[0];
  ZQUE *que = NULL;
  Byte *bDataLastPointer = NULL;
  Byte *bMessage = NULL;
  Dword ctrlPacketLength = 0;
  Dword dwTimeout;
  Byte bTimeout[6];

  ts_printf(__FILE__, __LINE__, "reportHostTestTimeOut");

  dwLength = 0;
  dwTimeout = tscb->dwTimeoutSecFromStartToFinalEnd / 60;
  sprintf(bTimeout, "%05ld min", dwTimeout);

  dwLength += sprintf(&bBufLastPointer[dwLength], "%05ld", dwTimeout);

  /* create que */
  ctrlPacketLength = 47;
  que = ts_qalloc(dwLength + ctrlPacketLength);

  bDataLastPointer = que->bData;


  bMessage = LOGIC_CARD_SERVER_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage); /* 09 */
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = MESSAGE_TEST_TIMEOUT;

  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage); /* 10 */
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = &bBuf[0];
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));  /* Timeout */
  bDataLastPointer += strlen(bMessage);

  /* set header info */
  que->wDID = tscb->wHostID;
  que->wSID = (Word)MY_TASK_ID;
  que->wLgth = (Byte *)bDataLastPointer - &que->bData[0];
  que->bType = ZCmd;
  que->bAdrs = MY_CELL_NO + 1;
  que->bP[0] = 0x00;
  que->bP[1] = 0x00;
  que->bP[2] = 0x00;
  que->bP[3] = 0x00;


  /* send queue */
  putBinaryDataDump(&que->bData[0], 0, que->wLgth);
  if (tccb.optionNoHostCommunication) {
    ts_qput((QUE *)que);
  } else {
    hi_qput((QUE *)que);
  }
}
/**
 * <pre>
 * Description:
 *   Report test script status to host.
 * Example:
 *   S[09000000]LTSServerS[0B000000]uartstatus:S[05000000]mmmmmmMMMMMMMMssssssssSSTTeeeeeeFFEEEEEEEEEE
 *   S[09000000]LTSServerS[0B000000]uartstatus:S[05000000]mmmmmmMMMMMMMMssssssssSSTTeeeeeeEEEEEEEEEEFF(for EB7 2Step)
 *   -> All of the following character should be ASCII.
 *     uartstatus:       - For UART Status Report Command
 *     mmmmmm:           - STD ID
 *     mmmmmm:           - STD ID (on label)
 *     ssssssss:         - Serial Number
 *     SS:               - SRST Step
 *     TT:               - AE Temperature
 *     eeeeee:           - Error Code
 *     EEEEEEEEEE        - elapsed time
 *     FF:               - Flag
 *     tttt:             - chamber temperature (hundredth)
 *     xxxxx:            - x voltage (mV)
 *     yyyyy:            - y voltage (mV)
 * </pre>
 *****************************************************************************/
void reportHostTestStatus(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Dword dwLength = 0;
  Byte bBuf[256];
  Byte *bBufLastPointer = &bBuf[0];
  ZQUE *que = NULL;
  Byte *bDataLastPointer = NULL;
  Byte *bMessage = NULL;
  Dword ctrlPacketLength = 0;
  Byte bTime[11];
  Byte bflag;

  ts_printf(__FILE__, __LINE__, "reportHostTestStatus");

  sec2hhhhmmssTimeFormat(getTestScriptTimeInSec(), &bTime[0]);

  if (tscb->bPartFlag == SRST_PART) {
    bflag = SRST_PART;
  } else {
    bflag = FINAL_PART;
  }

  dwLength = 0;
  dwLength += sprintf(&bBufLastPointer[dwLength], "%-6.6s%-6.6s%-8.8s%02x%02x%06x%02x%s%04d%05d%05d",
                      &tscb->bDriveMfgId[0],
                      &tscb->bDriveMfgIdOnLabel[0],
                      &tscb->bDriveSerialNumber[0],
                      tscb->bDriveStepCount,
                      tscb->bDriveTemperature,
                      (unsigned int)(tscb->dwDriveExtendedErrorCode & 0xffffff),
                      bflag,
                      &bTime[0],
                      tscb->wCurrentTempInHundredth,
                      tscb->wCurrentV5InMilliVolts,
                      tscb->wCurrentV12InMilliVolts
                     );

  /* create que */
  ctrlPacketLength = 36;
  que = ts_qalloc(dwLength + ctrlPacketLength);

  bDataLastPointer = que->bData;

  bMessage = LOGIC_CARD_SERVER_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = MESSAGE_STATUS_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = &bBuf[0];
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  /* set header info */
  que->wDID = tscb->wHostID;
  que->wSID = (Word)MY_TASK_ID;
  que->wLgth = (Byte *)bDataLastPointer - &que->bData[0];
  que->bType = ZCmd;
  que->bAdrs = MY_CELL_NO + 1;
  que->bP[0] = 0x00;
  que->bP[1] = 0x00;
  que->bP[2] = 0x00;
  que->bP[3] = 0x00;

  /* send queue */
  if (tccb.optionNoHostCommunication) {
    ts_qput((QUE *)que);
  } else {
    hi_qput((QUE *)que);
  }
}

/**
 * <pre>
 * Description:
 *   Report test script result data to host.
 * </pre>
 *****************************************************************************/
void reportHostTestDataChild(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, Byte *bData, Dword dwSize)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  ZDAT *zdat = NULL;
  ZQUE *zque = NULL;
  Dword dwIdNameLength = 0;
  Dword dwFileNameLength = 0;
  Dword i = 0;
  Dword dwSizeForDump = 0;

  ts_printf(__FILE__, __LINE__, "reportHostTestData %s %d", bName, dwSize);

  /* Dump Data to Console */
  dwSizeForDump = 16;
  if (dwSizeForDump > dwSize) {
    dwSizeForDump = dwSize;
  }
  putBinaryDataDump(&bData[0], 0, dwSizeForDump);

  /* Clear existing que & Put queue */
  /* ---------------------------------------------------------------------- */
#if defined(UNIT_TEST)
  /* ---------------------------------------------------------------------- */

  /* ---------------------------------------------------------------------- */
#elif defined(LINUX)
  /* ---------------------------------------------------------------------- */

  /* ---------------------------------------------------------------------- */
#else
  /* ---------------------------------------------------------------------- */
  if (ts_qis()) {
    void *invalid_que = NULL;
    invalid_que = (HQUE *)ts_qget();
    ts_qfree_then_abort(invalid_que);
    return;
  }
  /* ---------------------------------------------------------------------- */
#endif
  /* ---------------------------------------------------------------------- */

  tseb = &tscb->testScriptErrorBlock;

  if (tscb->bTestResultFileName[0] == '\0') {
    ts_printf(__FILE__, __LINE__, "no result file name");
    tseb->isFatalError = 1;
    return;
  }

  /* create que */
  dwIdNameLength = strlen(&bName[0]);
  dwFileNameLength = strlen(&tscb->bTestResultFileName[0]) + 1;

  zdat = (ZDAT *)ts_qalloc(sizeof(ZDAT) + dwFileNameLength + dwIdNameLength + sizeof(dwSize) + dwSize);
  zque = &(zdat->zque);

  /* set header info */
  zque->wDID = tscb->wHostID;
  zque->wSID = (Word)MY_TASK_ID;
  zque->wLgth = dwFileNameLength;
  zque->bType = ZUpload;
  zque->bAdrs = MY_CELL_NO + 1;
  zque->bP[0] = dwFileNameLength;
  zque->bP[1] = 0x00;
  zque->bP[2] = 0xFF;
  zque->bP[3] = 0x00;

  zdat->dwLgth = dwIdNameLength + sizeof(dwSize) + dwSize; /* --> File name length? */
  zdat->wAdrs[0] = zque->bAdrs;
  zdat->wAdrs[1] = 0x00;
  zdat->pName = zdat->bData;
  zdat->pData = &zdat->bData[dwFileNameLength];

  memmove(zdat->pName, &tscb->bTestResultFileName[0], dwFileNameLength);
  memmove(zdat->pData, &bName[0], dwIdNameLength);
  zdat->pData[dwIdNameLength + 0] = (dwSize) & 0xff;
  zdat->pData[dwIdNameLength + 1] = (dwSize >> 8) & 0xff;
  zdat->pData[dwIdNameLength + 2] = (dwSize >> 16) & 0xff;
  zdat->pData[dwIdNameLength + 3] = (dwSize >> 24) & 0xff;
  memmove(&zdat->pData[dwIdNameLength + 4], &bData[0], dwSize);

  /* send que */
  if (tccb.optionNoHostCommunication) {
    ts_qput((QUE *)zdat);
  } else {
    hi_qput((QUE *)zdat);
  }
  zdat = NULL;
  zque = NULL;

  /* Get response */
  for (i = 0 ; i < WAIT_TIME_SEC_FOR_HOST_RESPONSE ; i += WAIT_TIME_SEC_FOR_HOST_RESPONSE_POLL) {
    if (zque) {
      ts_qfree((Byte *)zque);
      zque = NULL;
    }
    if (ts_qis()) {
      if (tccb.optionNoHostCommunication) {
#if defined(IGNORE_QGET_DEBUG)
        zque = (ZQUE *)ts_qget();
        zque->wSID = tscb->wHostID;
        zque->wLgth = strlen(MESSAGE_HOST_RESPONSE_FOR_FILE);
        memcpy(&zque->bData[0], MESSAGE_HOST_RESPONSE_FOR_FILE, strlen(MESSAGE_HOST_RESPONSE_FOR_FILE));
#else
        zque = (ZQUE *)ts_qget();
#endif
      } else {
        zque = (ZQUE *)hi_qget(MY_CELL_NO + 1);
        ts_printf(__FILE__, __LINE__, "USE_HOSTIO_LIBRARY");
        putBinaryDataDump((Byte *)zque, 0, sizeof(ZQUE) + zque->wLgth + 32);
      }

      if ((zque->wSID != tscb->wHostID) ||
          (zque->wLgth != strlen(MESSAGE_HOST_RESPONSE_FOR_FILE)) ||
          (MATCH != memcmp(&zque->bData[0], MESSAGE_HOST_RESPONSE_FOR_FILE, strlen(MESSAGE_HOST_RESPONSE_FOR_FILE)))) {
        ts_qfree_then_abort(zque);
        return;
      } else {
        ts_printf(__FILE__, __LINE__, "Rcv Ctrl Pkt from Host [%s]", MESSAGE_HOST_RESPONSE_FOR_FILE);
        putBinaryDataDump((Byte *)&zque->bData[0], 0, strlen(MESSAGE_HOST_RESPONSE_FOR_FILE));
        break;
      }
    }
    ts_sleep_partial(WAIT_TIME_SEC_FOR_HOST_RESPONSE_POLL);
  }
  ts_printf(__FILE__, __LINE__, "reportHostTestDataque %s", bName);
  if (zque) {
    ts_qfree((Byte *)zque);
    zque = NULL;
  }
  ts_printf(__FILE__, __LINE__, "reportHostTestDatafinished %s", bName);

  /* Timeout */
  if (i >= WAIT_TIME_SEC_FOR_HOST_RESPONSE) {
    ts_printf(__FILE__, __LINE__, "fatal error - response que timeout");
    ts_abort();
    return;
  }
}

/**
 * <pre>
 * Description:
 *   Report test script result data to host.
 * </pre>
 *****************************************************************************/
void reportHostTestData(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, Byte *bData, Dword dwSize)
{
  Dword dwOffset = 0;
  Dword dwSendingDataSize = 0;

  if (dwSize > MAX_REPORT_DATA_SIZE_BYTE) {
    ts_printf(__FILE__, __LINE__, "reportHostTestData %s %d -> dividing...", bName, dwSize);
  }

  for (dwOffset = 0 ; dwOffset < dwSize ; ) {
    if ((dwSize - dwOffset) > MAX_REPORT_DATA_SIZE_BYTE) {  /*MAX_REPORT_DATA_SIZE_BYTE -> 63*1024) */
      dwSendingDataSize = MAX_REPORT_DATA_SIZE_BYTE;
    } else {
      dwSendingDataSize = (dwSize - dwOffset);
    }
    reportHostTestDataChild(tscb, bName, &bData[dwOffset], dwSendingDataSize);
    dwOffset += dwSendingDataSize;
  }
}

/**
 * <pre>
 * Description:
 *   Report test script bad head information to host.
 * Format:
 *   S[09000000]LTSServerS[08000000]badHead:L[xxxxxxxx]
 *     xxxxxxxx:               - Bad head (32 bits little enditian)
 * Example:
 *   S[09000000]LTSServerS[08000000]badHead:L[28000000]
 * </pre>
 *****************************************************************************/
void reportHostBadHead(TEST_SCRIPT_CONTROL_BLOCK *tscb, Word wBadHeadData)
{
  ZQUE *que = NULL;
  Byte *bDataLastPointer = NULL;
  Byte *bMessage = NULL;
  Dword ctrlPacketLength = 0;

  ts_printf(__FILE__, __LINE__, "reportHostBadHead");

  /* create que */
  ctrlPacketLength = 32;
  que = ts_qalloc(ctrlPacketLength);

  bDataLastPointer = que->bData;

  bMessage = LOGIC_CARD_SERVER_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = MESSAGE_BAD_HEAD_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  *bDataLastPointer++ = INTEGER_SEPARATOR;
  *bDataLastPointer++ = wBadHeadData & 0xff;
  *bDataLastPointer++ = (wBadHeadData >> 8) & 0xff;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;

  /* set header info */
  que->wDID = tscb->wHostID;
  que->wSID = (Word)MY_TASK_ID;
  que->wLgth = (Byte *)bDataLastPointer - &que->bData[0];
  que->bType = ZCmd;
  que->bAdrs = MY_CELL_NO + 1;
  que->bP[0] = 0x00;
  que->bP[1] = 0x00;
  que->bP[2] = 0x00;
  que->bP[3] = 0x00;

  /* send queue */
  ts_qput((QUE *)que);
}

/**
 * <pre>
 * Description:
 *   Report test script completion to host.
 * Example:
 *   S[09000000]LTSServerS[aaaaaaaa]srstComp:rc:resultFile:pfTable:S[bbbbbbbb]SELFTESTL[cccccccc]S[dddddddd]C:\!LTS\!SYSOUT\!BIN\RESULT.001S[eeeeeeee]c:\!lts\!system\!st\pf_final.cmn
 *   -> Report test competion status (in SRST mode)
 *     14 Byte: S[09000000]LTSServer                          - Server Type
 *     36 Byte: S[aaaaaaaa]srstComp:rc:resultFile:pfTable:    - Command Name
 *                                                            - Fixed string "srstComp:rc:resultFile:pfTable:"
 *     13 Byte: S[bbbbbbbb]SELFTEST                           - Function Name
 *      5 Byte: L[cccccccc]                                   - Return Code from Failed Function
 *                                                              Return SRST Error Code (5 or 6 digit ?)
 *     36 Byte: S[dddddddd]C:\!LTS\!SYSOUT\!BIN\RESULT.001    - Result Filename
 *     37 Byte: S[eeeeeeee]c:\!lts\!system\!st\pf_final.cmn   - PF Table Filename
 * Reference:
 *   S[09000000]LTSServerS[aaaaaaaa]testComp:rc:resultFile:pfTable:S[bbbbbbbb]powerOnResetSyncSCRL[cccccccc]S[dddddddd]C:\!LTS\!SYSOUT\!BIN\RESULT.001S[eeeeeeee]c:\!lts\!system\!st\pf_final.cmn
 *   -> Report test competion status (in I/F mode)
 *     S[09000000]LTSServer                        - Server Type
 *     S[aaaaaaaa]testComp:rc:resultFile:pfTable:  - Command Name
 *     S[bbbbbbbb]powerOnResetSyncSCR              - Failed Function Name
 *     L[cccccccc]                                 - Return Code from Failed Function
 *     S[dddddddd]C:\!LTS\!SYSOUT\!BIN\RESULT.001  - Result Filename
 *     S[eeeeeeee]c:\!lts\!system\!st\pf_final.cmn - PF Table Filename
 * </pre>
 *****************************************************************************/
void reportHostTestComplete(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte bComp)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  ZQUE *que = NULL;
  Byte *bDataLastPointer = NULL;
  Byte *bMessage = NULL;
  Dword ctrlPacketLength = 0;
  Dword i = 0;

  if (bComp == REPORT_PRECOMP) {
    ts_printf(__FILE__, __LINE__, "reportHostTestComplete PRECOMP %d");
  } else {
    ts_printf(__FILE__, __LINE__, "reportHostTestComplete COMP %d");
  }

  /* Clear existing que & Put queue */
  /* ---------------------------------------------------------------------- */
#if defined(UNIT_TEST)
  /* ---------------------------------------------------------------------- */

  /* ---------------------------------------------------------------------- */
#elif defined(LINUX)
  /* ---------------------------------------------------------------------- */

  /* ---------------------------------------------------------------------- */
#else
  /* ---------------------------------------------------------------------- */
  if (ts_qis()) {
    void *invalid_que = NULL;
    invalid_que = (HQUE *)ts_qget();
    ts_qfree_then_abort(invalid_que);
    return;
  }
  /* ---------------------------------------------------------------------- */
#endif
  /* ---------------------------------------------------------------------- */

  tseb = &tscb->testScriptErrorBlock;

  if (tscb->bTestResultFileName[0] == '\0') {
    ts_printf(__FILE__, __LINE__, "no result file name");
    tseb->isFatalError = 1;
    return;
  }

  /* create que */

  ctrlPacketLength = 100 + sizeof(tscb->bTestResultFileName) + sizeof(tscb->bTestPfTableFileName);
  que = ts_qalloc(ctrlPacketLength);

  bDataLastPointer = que->bData;

  bMessage = LOGIC_CARD_SERVER_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  if (bComp == REPORT_PRECOMP) {
    bMessage = MESSAGE_PRECOMP_COMMAND_STR;  /* for 2Step Command Name "srstPreComp:rc:resultFile:pfTable:"*/
  } else {
    bMessage = MESSAGE_COMP_COMMAND_STR;
  }

  /*bMessage = MESSAGE_COMP_COMMAND_STR;*/
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  if (bComp == REPORT_PRECOMP) {
    bMessage = MESSAGE_PRECOMP_FUNCTION_NAME_STR;            /* "SRSTFAIL" for 2Step */
  } else {
    bMessage = MESSAGE_FUNCTION_NAME_STR;
  }          /* "SELFTEST" */

  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  *bDataLastPointer++ = INTEGER_SEPARATOR;
  *bDataLastPointer++ = (tscb->dwDriveExtendedErrorCode) & 0xff;
  *bDataLastPointer++ = (tscb->dwDriveExtendedErrorCode >> 8) & 0xff;
  *bDataLastPointer++ = (tscb->dwDriveExtendedErrorCode >> 16) & 0xff;
  *bDataLastPointer++ = (tscb->dwDriveExtendedErrorCode >> 24) & 0xff;

  bMessage = &tscb->bTestResultFileName[0];
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = &tscb->bTestPfTableFileName[0];
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  /* set header info */
  que->wDID = tscb->wHostID;
  que->wSID = (Word)MY_TASK_ID;
  que->wLgth = (Byte *)bDataLastPointer - &que->bData[0];
  que->bType = ZCmd;
  que->bAdrs = MY_CELL_NO + 1;
  que->bP[0] = 0x00;
  que->bP[1] = 0x00;
  que->bP[2] = 0x00;
  que->bP[3] = 0x00;

  /* send que */
  if (tccb.optionNoHostCommunication) {
    ts_qput((QUE *)que);
  } else {
    hi_qput((QUE *)que);
  }
  que = NULL;

  /* Get response */
  for (i = 0 ; i < WAIT_TIME_SEC_FOR_HOST_RESPONSE ; i += WAIT_TIME_SEC_FOR_HOST_RESPONSE_POLL) {
    if (que) {
      ts_qfree((Byte *)que);
      que = NULL;
    }
    if (ts_qis()) {
      if (tccb.optionNoHostCommunication) {
#if defined(IGNORE_QGET_DEBUG)
        que = (ZQUE *)ts_qget();
        que->wSID = tscb->wHostID;
        que->wLgth = strlen(MESSAGE_HOST_RESPONSE_FOR_COMP);
        memcpy(&que->bData[0], MESSAGE_HOST_RESPONSE_FOR_COMP, strlen(MESSAGE_HOST_RESPONSE_FOR_COMP));
#else
        que = (ZQUE *)ts_qget();
#endif
      } else {
        que = (ZQUE *)hi_qget(MY_CELL_NO + 1);
        ts_printf(__FILE__, __LINE__, "USE_HOSTIO_LIBRARY");
        putBinaryDataDump((Byte *)que, 0, sizeof(ZQUE) + que->wLgth + 32);
      }

      if ((que->wSID != tscb->wHostID) ||
          (que->wLgth != strlen(MESSAGE_HOST_RESPONSE_FOR_COMP)) ||
          (MATCH != ts_memcmp_exp((Byte *)&que->bData[0], MESSAGE_HOST_RESPONSE_FOR_COMP, strlen(MESSAGE_HOST_RESPONSE_FOR_COMP)))) {
        ts_qfree_then_abort(que);
        return;
      } else {
        ts_printf(__FILE__, __LINE__, "Rcv Ctrl Pkt from Host [%s]", MESSAGE_HOST_RESPONSE_FOR_COMP);
        putBinaryDataDump((Byte *)&que->bData[0], 0, strlen(MESSAGE_HOST_RESPONSE_FOR_COMP));
        break;
      }
    }
    ts_sleep_partial(WAIT_TIME_SEC_FOR_HOST_RESPONSE_POLL);
  }
  if (que) {
    ts_qfree((Byte *)que);
    que = NULL;
  }

  /* Timeout */
  if (i >= WAIT_TIME_SEC_FOR_HOST_RESPONSE) {
    ts_printf(__FILE__, __LINE__, "fatal error - response que timeout");
    ts_abort();
    return;
  }
}

/**
 * <pre>
 * Description:
 *   Report test script log data to host.
 * </pre>
 *****************************************************************************/
void reportHostTesterLog(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte isReportEnable = 0;
  Byte *bLogData = NULL;
  Dword dwCurLogSizeInByte = 0;
  Dword dwMaxLogSizeInByte = 0;

  ts_printf(__FILE__, __LINE__, "reportHostTesterLog");

  if (tscb->isTesterLogReportEnable & IF_DRIVE_PASS) {
    if (tscb->dwDriveExtendedErrorCode == 0) {
      isReportEnable = 1;
    }
  }
  if (tscb->isTesterLogReportEnable & IF_DRIVE_FAIL) {
    if (tscb->wDriveNativeErrorCode != 0) {
      isReportEnable = 1;
    }
  }
  if (tscb->isTesterLogReportEnable & IF_DRIVE_GODx) {
    if ((tscb->wDriveNativeErrorCode == 0) && (tscb->dwDriveExtendedErrorCode != 0)) {
      isReportEnable = 1;
    }
  }

  /* tscb snapshot */
  reportHostTestData(tscb, KEY_TESTER_UART_LOG, (Byte *)tscb, sizeof(*tscb) - sizeof(tscb->bSectorBuffer) - sizeof(tscb->testScriptRecorderBlock));

  /* log data block */
  if (isReportEnable) {
    getTestScriptLog(&bLogData, &dwCurLogSizeInByte, &dwMaxLogSizeInByte);
    reportHostTestData(tscb, KEY_TESTER_LOG, bLogData, dwCurLogSizeInByte);
    reportHostTestData(tscb, KEY_TESTER_RECORD, (Byte *)tscb->testScriptRecorderBlock, tscb->dwTestScriptRecorderBlockCounter * sizeof(TEST_SCRIPT_RECORDER_BLOCK));
  } else {
    ts_printf(__FILE__, __LINE__, "no tester log report");
  }
}

/**
 * <pre>
 * Description:
 *   Report test script message to host.
 * Example:
 *   S[09000000]LTSServerS[04000000]msg:S[05000000]Hello
 *   -> Display message on Trace Window
 * </pre>
 *****************************************************************************/
void reportHostTestMessage(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *description, ...)
{
  Dword dwLength = 0;
  va_list ap;
  Byte bBuf[256];
  Byte *bBufLastPointer = &bBuf[0];
  ZQUE *que = NULL;
  Byte *bDataLastPointer = NULL;
  Byte *bMessage = NULL;
  Dword ctrlPacketLength = 0;

  ts_printf(__FILE__, __LINE__, "reportHostTestMessage");

  /* length check. it is some useful even if it is not 100% reliable. */
  dwLength = strlen(description);
  if (dwLength > (sizeof(bBuf) / 2)) {
    ts_printf(__FILE__, __LINE__, "Error - length overflow\r\n");
    return;
  }

  dwLength = 0;

  /* parsing */
  va_start(ap, description);
#if defined(UNIT_TEST) || defined(LINUX)
  /* ---------------------------------------------------------------------- */
  dwLength += vsprintf(&bBufLastPointer[dwLength], description, ap);
#else
  /* ---------------------------------------------------------------------- */
  dwLength += (sprintfdcom(&bBufLastPointer[dwLength], description, ap) - &bBufLastPointer[dwLength]);
#endif
  /* ---------------------------------------------------------------------- */
  va_end(ap);

  /* length check. it is some useful even if it is not 100% reliable. */
  if (dwLength > (sizeof(bBuf) - 16)) {
    ts_printf(__FILE__, __LINE__, "Error - length overflow\r\n");
    return;
  }

  /* create que */
  ctrlPacketLength = 28 + 256;
  que = ts_qalloc(dwLength + ctrlPacketLength);

  bDataLastPointer = que->bData;

  bMessage = LOGIC_CARD_SERVER_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = MESSAGE_COMMAND_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = &bBuf[0];
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  /* set header info */
  que->wDID = tscb->wHostID;
  que->wSID = (Word)MY_TASK_ID;
  que->wLgth = (Byte *)bDataLastPointer - &que->bData[0];
  que->bType = ZCmd;
  que->bAdrs = MY_CELL_NO + 1;
  que->bP[0] = 0x00;
  que->bP[1] = 0x00;
  que->bP[2] = 0x00;
  que->bP[3] = 0x00;

  /* send queue */
  if (tccb.optionNoHostCommunication) {
    ts_qput((QUE *)que);
  } else {
    hi_qput((QUE *)que);
  }
}

void reportHostErrorMessage(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *description, ...)
{
  Dword dwLength = 0;
  va_list ap;
  Byte bBuf[256];
  Byte *bBufLastPointer = &bBuf[0];
  ts_printf(__FILE__, __LINE__, "reportHostErrorMessage");

  /* length check. it is some useful even if it is not 100% reliable. */
  dwLength = strlen(description);
  if (dwLength > (sizeof(bBuf) / 2)) {
    ts_printf(__FILE__, __LINE__, "Error - length overflow\r\n");
    return;
  }

  dwLength = 0;

  /* parsing */
  va_start(ap, description);
#if defined(UNIT_TEST) || defined(LINUX)
  /* ---------------------------------------------------------------------- */
  dwLength += vsprintf(&bBufLastPointer[dwLength], description, ap);
#else
  /* ---------------------------------------------------------------------- */
  dwLength += (sprintfdcom(&bBufLastPointer[dwLength], description, ap) - &bBufLastPointer[dwLength]);
#endif
  /* ---------------------------------------------------------------------- */
  va_end(ap);

  /* length check. it is some useful even if it is not 100% reliable. */
  if (dwLength > (sizeof(bBuf) - 16)) {
    ts_printf(__FILE__, __LINE__, "Error - length overflow\r\n");
    return;
  }

  reportHostTestData(tscb, KEY_TESTER_ERROR_LOG, bBuf, dwLength);
}


/**
 * <pre>
 * Description:
 *   Download file from host
 * Argument:
 *   *tscb - pointer to test script control block
 *   *bName - file name (format is as same as I/F tester sequence file)
 *   *bData - pointer to data buffer prepared by caller
 *   dwSize - file size (== bData size) in byte
 * </pre>
 *****************************************************************************/
void downloadFile(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, Byte *bData, Dword dwSize) /* [HIGH_SPEED_UART] */
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;

  ts_printf(__FILE__, __LINE__, "downloadFile, %s, %d", bName, dwSize);

  /* arguments range check */
  if (strlen(bName) < 1) {
    ts_printf(__FILE__, __LINE__, "no file name");
    tseb->isFatalError = 1;
    return;
  }
  if (strlen(bName) > 1024) {
    ts_printf(__FILE__, __LINE__, "file name too large");
    tseb->isFatalError = 1;
    return;
  }
  if (dwSize < 1) {
    ts_printf(__FILE__, __LINE__, "no file size");
    tseb->isFatalError = 1;
    return;
  }
  if (dwSize > (100 * 1024 * 1024)) {
    ts_printf(__FILE__, __LINE__, "file size too large");
    tseb->isFatalError = 1;
    return;
  }

#if defined(UNIT_TEST) || defined(LINUX)
  /* ---------------------------------------------------------------------- */
  FILE *fp = NULL;
  Dword dwSizeForCheck = 0;

  fp = fopen(bName, "rb");
  if (fp == NULL) {
    ts_printf(__FILE__, __LINE__, "fatal error - to open file %s", bName);
    ts_abort();
    return;
  }
  fseek(fp, 0, SEEK_END);
  dwSizeForCheck = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (dwSizeForCheck != dwSize) {
    ts_printf(__FILE__, __LINE__, "fatal error - file size unmatch!!! %ld %ld", dwSizeForCheck, dwSize);
    fclose(fp);
    ts_abort();
    return;
  }

  dwSizeForCheck = fread(&bData[0], 1, dwSize, fp);

  if (dwSizeForCheck != dwSize) {
    ts_printf(__FILE__, __LINE__, "fatal error - file size unmatch!!! %ld %ld", dwSizeForCheck, dwSize);
    fclose(fp);
    ts_abort();
    return;
  }

  fclose(fp);
  ts_printf(__FILE__, __LINE__, "Grading File close");
  return ;
#else
  /* ---------------------------------------------------------------------- */
  Dword dwOffset = 0;
  Dword dwChunkSize = 0;
  Dword dwChunkSizeForVerify = 0;
  Byte isBroadcast = 0;
  Byte doCompare = 0;

  for (dwOffset = 0 ; ; dwOffset += DOWNLOAD_FILE_CHUNK_SIZE) {
    dwChunkSize = DOWNLOAD_FILE_CHUNK_SIZE;
    if ((dwOffset + dwChunkSize) >= dwSize) {
      dwChunkSize = dwSize - dwOffset;
      if (dwChunkSize < (DOWNLOAD_FILE_CHUNK_SIZE - DOWNLOAD_FILE_NAME_COMPARE_SIZE)) {
        doCompare = 1;
      }
    }
    dwChunkSizeForVerify = dwChunkSize;

    downloadFileChild(tscb, bName, &bData[dwOffset], &dwChunkSize, dwOffset, isBroadcast, doCompare);

    if (dwChunkSize != dwChunkSizeForVerify) {
      ts_printf(__FILE__, __LINE__, "fatal error - file size mismatch");
      tseb->isFatalError = 1;
      return;
    } else if ((dwOffset + dwChunkSize) > dwSize) {
      ts_printf(__FILE__, __LINE__, "fatal error - file size too large");
      tseb->isFatalError = 1;
      return;
    } else if ((dwOffset + dwChunkSize) == dwSize) {
      /* complete downloading */
      break;
    }
  }
#endif
  /* ---------------------------------------------------------------------- */
}


/**
 * <pre>
 * Description:
 *   Report test script result data to host.
 * </pre>
 *****************************************************************************/
void downloadFileInBroadcast(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, Byte *bData, Dword dwSize) /* [HIGH_SPEED_UART] */
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Dword dwChunkSize = 0;
  Byte isBroadcast = 1;
  Byte doCompare = 1;

  ts_printf(__FILE__, __LINE__, "downloadFileInBroadcast, %s, %d", bName, dwSize);

  /* arguments range check */
  if (strlen(bName) < 1) {
    ts_printf(__FILE__, __LINE__, "no file name");
    tseb->isFatalError = 1;
    return;
  }
  if (strlen(bName) > 1024) {
    ts_printf(__FILE__, __LINE__, "file name too large");
    tseb->isFatalError = 1;
    return;
  }
  if (dwSize < 1) {
    ts_printf(__FILE__, __LINE__, "no file size");
    tseb->isFatalError = 1;
    return;
  }
  if (dwSize > (100 * 1024 * 1024)) {
    ts_printf(__FILE__, __LINE__, "file size too large");
    tseb->isFatalError = 1;
    return;
  }

  dwChunkSize = dwSize;
  downloadFileChild(tscb, bName, &bData[0], &dwChunkSize, 0, isBroadcast, doCompare);

  if (dwChunkSize != dwSize) {
    /* complete downloading */
    ts_printf(__FILE__, __LINE__, "fatal error - file size unmatch!!! %d %d", dwChunkSize, dwSize);
    tseb->isFatalError = 1;
    return;
  }
}


/**
 * <pre>
 * Description:
 *   Download file from host.
 * Arguments:
 *   *tscb - Test script control block
 *   *bName - File name including path and file name
 *   *dwSize - Max file size (unit in bytes) provided by caller. This function sets actual receiving file size to it.
 *   *bData - File data buffer
 *   isBroadcast - Flag. If non-zero, broadcast. If zero, non broadcast.
 *   doCompare - Flag. If non-zero, file name is assumed to be appended at the last of data
 * Command Format:
 *   S[09000000]LTSServerS[19000000]downloadSyncCompare:S[20000000]C\!LTS\!SYSIN\!AAAAAL[xxxxxxxx]    (AAAAA : DL file name, xxxxxxxx  L : offset (15000 * N))
 *     14 Byte: S[09000000]LTSServer                          - Server Type
 *     25 Byte: S[13000000]downloadCMDCompare:                - Command Name - Fixed string "downloadSyncCompare:"
 *      N Byte: S[xxxxxxxx]'filepath+filename'                - The name say it all. (e.g. C\!LTS\!SYSIN\!MICROCODE.BIN)
 *      5 Byte: L[yyyyyyyy]                                   - Offset (15,000 x N) (e.g. 0:offset=0, 10:offset=150,000)
 * </pre>
 *****************************************************************************/
void downloadFileChild(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, Byte *bData, Dword *dwSize, Dword dwOffset, Byte isBroadcast, Byte doCompare)  /* [HIGH_SPEED_UART] */
{
#if defined(UNIT_TEST) || defined(LINUX)
  /* ---------------------------------------------------------------------- */
  ts_printf(__FILE__, __LINE__, "downloadFileChild,%s,%d,%d,%s,%s", bName, *dwSize, dwOffset, isBroadcast ? "Broadcast" : "Non Broadcast", doCompare ? "Do Compare" : "No Compare");

#else
  /* ---------------------------------------------------------------------- */
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  ZQUE *que = NULL;
  Byte *bDataLastPointer = NULL;
  Byte *bMessage = NULL;
  Dword ctrlPacketLength = 0;
  Dword i = 0;
  Byte bStringForFileNameCompare[DOWNLOAD_FILE_NAME_COMPARE_SIZE + 1];

  ts_printf(__FILE__, __LINE__, "downloadFileChild,%s,%d,%d,%s,%s", bName, *dwSize, dwOffset, isBroadcast ? "Broadcast" : "Non Broadcast", doCompare ? "Do Compare" : "No Compare");

  /* Clear existing que & Put queue */
  if (ts_qis()) {
    void *invalid_que = NULL;
    invalid_que = (HQUE *)ts_qget();
    ts_qfree_then_abort(invalid_que);
    return;
  }

  /* arguments range check */
  if (strlen(bName) < 1) {
    ts_printf(__FILE__, __LINE__, "no file name");
    tseb->isFatalError = 1;
    return;
  }
  if (strlen(bName) > 1024) {
    ts_printf(__FILE__, __LINE__, "file name too large");
    tseb->isFatalError = 1;
    return;
  }
  if ((bName[strlen(bName) - 1] == '\\') || (bName[strlen(bName) - 1] == '/')) {
    ts_printf(__FILE__, __LINE__, "file name last character slash");
    tseb->isFatalError = 1;
    return;
  }
  if (*dwSize < 1) {
    ts_printf(__FILE__, __LINE__, "no file size");
    tseb->isFatalError = 1;
    return;
  }
  if (*dwSize > (100 * 1024 * 1024)) {
    ts_printf(__FILE__, __LINE__, "file size too large");
    tseb->isFatalError = 1;
    return;
  }
  if ((dwOffset % DOWNLOAD_FILE_CHUNK_SIZE) != 0) {
    ts_printf(__FILE__, __LINE__, "offset size boundary (%d bytes) error", DOWNLOAD_FILE_CHUNK_SIZE);
    tseb->isFatalError = 1;
    return;
  }
  if (dwOffset > (100 * 1024 * 1024)) {
    ts_printf(__FILE__, __LINE__, "offset size too large");
    tseb->isFatalError = 1;
    return;
  }

  /* create string for file name compare */
  if (doCompare) {
    for (i = (strlen(bName) - 1) ; i > 0 ; i--) {
      if ((bName[i] == '\\') || (bName[i] == '/')) {
        i++;
        break;
      }
    }
    memset(&bStringForFileNameCompare[0], '\0', sizeof(bStringForFileNameCompare));
    if (strlen(&bName[i]) > DOWNLOAD_FILE_NAME_COMPARE_SIZE) {
      memcpy(&bStringForFileNameCompare[0], &bName[i], DOWNLOAD_FILE_NAME_COMPARE_SIZE);
    } else {
      memcpy(&bStringForFileNameCompare[0], &bName[i], strlen(&bName[i]));
    }
    for (i = 0 ; i < sizeof(bStringForFileNameCompare) ; i++) {
      bStringForFileNameCompare[i] = tolower(bStringForFileNameCompare[i]);
    }
  } else {
    memset(&bStringForFileNameCompare[0], 0x00, sizeof(bStringForFileNameCompare));
  }

  /* create que */
  ctrlPacketLength = 14 + 25 + 5 + strlen(bName) + 5 + 4; /* last 4 bytes are margin */
  que = ts_qalloc(ctrlPacketLength);

  bDataLastPointer = que->bData;

  bMessage = LOGIC_CARD_SERVER_STR;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  if (isBroadcast) {
    bMessage = MESSAGE_DOWNLOAD_SYNC_STR;
  } else {
    bMessage = MESSAGE_DOWNLOAD_CMD_STR;
  }
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  bMessage = bName;
  *bDataLastPointer++ = MESSAGE_SEPARATOR;
  *bDataLastPointer++ = strlen(bMessage);
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  *bDataLastPointer++ = 0x00;
  memmove(bDataLastPointer, bMessage, strlen(bMessage));
  bDataLastPointer += strlen(bMessage);

  if (isBroadcast) {
    /* no offset field */
  } else {
    *bDataLastPointer++ = INTEGER_SEPARATOR;
    *bDataLastPointer++ = (dwOffset) & 0xff;
    *bDataLastPointer++ = (dwOffset >> 8) & 0xff;
    *bDataLastPointer++ = (dwOffset >> 16) & 0xff;
    *bDataLastPointer++ = (dwOffset >> 24) & 0xff;
  }

  /* set header info */
  que->wDID = tscb->wHostID;
  que->wSID = (Word)MY_TASK_ID;
  que->wLgth = (Byte *)bDataLastPointer - &que->bData[0];
  que->bType = ZCmd;
  que->bAdrs = MY_CELL_NO + 1;
  que->bP[0] = 0x00;
  que->bP[1] = 0x00;
  que->bP[2] = 0x00;
  que->bP[3] = 0x00;

  /* [DEBUG] */
  ts_printf(__FILE__, __LINE__, "[DEBUG] - ts_qput");
  putBinaryDataDump((Byte *)que, 0, sizeof(ZQUE) + 64);

  /* send que */
  ts_qput((QUE *)que);
  que = NULL;

  /* Get response */
  for (i = 0 ; i < WAIT_TIME_SEC_FOR_HOST_RESPONSE ; i += WAIT_TIME_SEC_FOR_HOST_RESPONSE_POLL) {
    if (que) {
      ts_qfree((Byte *)que);
      que = NULL;
    }
    if (ts_qis()) {
      que = (ZQUE *)ts_qget();

      /* [DEBUG] */
      ts_printf(__FILE__, __LINE__, "[DEBUG] - ts_qget");
      putBinaryDataDump((Byte *)que, 0, sizeof(ZQUE) + 64);

      if (isBroadcast) {
        ZDAT *zdat = (ZDAT *)que;
        if (doCompare) {
          if ((que->wSID != tscb->wHostID) ||
              (que->bType != ZData) ||
              (zdat->dwLgth < (1 + DOWNLOAD_FILE_NAME_COMPARE_SIZE)) ||
              (zdat->dwLgth > (*dwSize + DOWNLOAD_FILE_NAME_COMPARE_SIZE)) ||
              (MATCH != memcmp(&zdat->pData[zdat->dwLgth - DOWNLOAD_FILE_NAME_COMPARE_SIZE], &bStringForFileNameCompare[0], DOWNLOAD_FILE_NAME_COMPARE_SIZE))
             ) {

            /* [DEBUG] */
            ts_printf(__FILE__, __LINE__, "que->wSID=%d", que->wSID);
            ts_printf(__FILE__, __LINE__, "tscb->wHostID=%d", tscb->wHostID);
            ts_printf(__FILE__, __LINE__, "que->bType=0x%x", que->bType);
            ts_printf(__FILE__, __LINE__, "zdat->dwLgth=%d", zdat->dwLgth);
            ts_printf(__FILE__, __LINE__, "DOWNLOAD_FILE_NAME_COMPARE_SIZE=%d", DOWNLOAD_FILE_NAME_COMPARE_SIZE);
            ts_printf(__FILE__, __LINE__, "*dwSize=%d", *dwSize);
            putBinaryDataDump((Byte *)&zdat->pData[que->wLgth - DOWNLOAD_FILE_NAME_COMPARE_SIZE], 0, DOWNLOAD_FILE_NAME_COMPARE_SIZE);
            putBinaryDataDump((Byte *)&bStringForFileNameCompare[0], 0, DOWNLOAD_FILE_NAME_COMPARE_SIZE);

            ts_qfree_then_abort(que);
            return;
          } else {
            *dwSize = zdat->dwLgth - DOWNLOAD_FILE_NAME_COMPARE_SIZE;
            ts_printf(__FILE__, __LINE__, "Rcv File from Host, %d", *dwSize);
            putBinaryDataDump((Byte *)&zdat->pData[0], 0, 16);
            memmove(bData, &zdat->pData[0], *dwSize);

            /* send START & FIN response to host */
            xchgID((QUE *)que);
            que->bType = ZStart;
            zdat = (ZDAT *)cpyQue((QUE *)que);
            zdat->zque.bType = ZFin;
            putBinaryDataDump((Byte *)que, 0, 64);
            ts_qput((QUE *)que);
            que = NULL;
            putBinaryDataDump((Byte *)zdat, 0, 64);
            ts_qput((QUE *)zdat);
            zdat = NULL;
            break;
          }
        } else {
          /* [DEBUG] TBD */
        }
      } else {
        if (doCompare) {
          if ((que->wSID != tscb->wHostID) ||
              (que->wLgth < (1 + DOWNLOAD_FILE_NAME_COMPARE_SIZE)) ||
              (que->wLgth > (*dwSize + DOWNLOAD_FILE_NAME_COMPARE_SIZE)) ||
              (MATCH != memcmp(&que->bData[que->wLgth - DOWNLOAD_FILE_NAME_COMPARE_SIZE], &bStringForFileNameCompare[0], DOWNLOAD_FILE_NAME_COMPARE_SIZE))
             ) {

            /* [DEBUG] need que->bType check */
            ts_printf(__FILE__, __LINE__, "que->wSID=%d", que->wSID);
            ts_printf(__FILE__, __LINE__, "tscb->wHostID=%d", tscb->wHostID);
            ts_printf(__FILE__, __LINE__, "que->wLgth=%d", que->wLgth);
            ts_printf(__FILE__, __LINE__, "DOWNLOAD_FILE_NAME_COMPARE_SIZE=%d", DOWNLOAD_FILE_NAME_COMPARE_SIZE);
            ts_printf(__FILE__, __LINE__, "*dwSize=%d", *dwSize);
            putBinaryDataDump((Byte *)&que->bData[que->wLgth - DOWNLOAD_FILE_NAME_COMPARE_SIZE], 0, DOWNLOAD_FILE_NAME_COMPARE_SIZE);
            putBinaryDataDump((Byte *)&bStringForFileNameCompare[0], 0, DOWNLOAD_FILE_NAME_COMPARE_SIZE);

            ts_qfree_then_abort(que);
            return;
          } else {
            *dwSize = que->wLgth - DOWNLOAD_FILE_NAME_COMPARE_SIZE;
            ts_printf(__FILE__, __LINE__, "Rcv File from Host, %d", *dwSize);
            putBinaryDataDump((Byte *)&que->bData[0], 0, 16);
            memmove(bData, &que->bData[0], *dwSize);
            break;
          }
        } else {
          if ((que->wSID != tscb->wHostID) ||
              (que->wLgth < 1) ||
              (que->wLgth > (*dwSize + DOWNLOAD_FILE_NAME_COMPARE_SIZE))
             ) {

            /* [DEBUG] */
            ts_printf(__FILE__, __LINE__, "que->wSID=%d", que->wSID);
            ts_printf(__FILE__, __LINE__, "tscb->wHostID=%d", tscb->wHostID);
            ts_printf(__FILE__, __LINE__, "que->wLgth=%d", que->wLgth);
            ts_printf(__FILE__, __LINE__, "*dwSize=%d", *dwSize);

            ts_qfree_then_abort(que);
            return;
          } else {
            *dwSize = que->wLgth;
            ts_printf(__FILE__, __LINE__, "Rcv File from Host, %d", *dwSize);
            putBinaryDataDump((Byte *)&que->bData[0], 0, 16);
            memmove(bData, &que->bData[0], *dwSize);
            break;
          }
        }
      }
    }
    ts_sleep_partial(WAIT_TIME_SEC_FOR_HOST_RESPONSE_POLL);
  }
  if (que) {
    ts_qfree((Byte *)que);
    que = NULL;
  }

  /* Timeout */
  if (i >= WAIT_TIME_SEC_FOR_HOST_RESPONSE) {
    ts_printf(__FILE__, __LINE__, "fatal error - response que timeout");
    ts_abort();
    return;
  }

#endif
  /* ---------------------------------------------------------------------- */
}

