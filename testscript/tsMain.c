/*
[019KG]  2011.03.30 Furukawa Bug fix for grading() in tsStat.c.
[019KP3] 2011.07.21 Furukawa sorting skip causeof reported pass1  reportHostGradingId() in tsHost.c.
                             H/W Library version dump tsMain.c.
[019KP5] 2011.07.28 Furukawa Initialize temperature.
[019KPA] 2012.01.31 Furukawa Bug fix for E/C5715 / 5910 / addition SRST immediate stop request.
[019KPB] 2012.02.23 Furukawa God9 of PCPGM improvement.
[019KPC] 2012.09.05 N.Saito  Bug fix for Timeout fail at re-final test.(GOD9)
[019KPD] 2012.11.21 Ekkarach 1. EC6110 no rawdata support.
                             2. TestScript version change.
         2012.11.28 Ekkarach 1. Change KEY_PES_DATA to sizeof(KEY_PES_DATA).                                     
[019KPE] 2013.1.29 N.Saito   Support control code at reading config file.
[019KPI] 2013.6.04 Y.Katayama Support clearFPGABuffer command   .
*/

#include "ts.h"
/**
 * <pre>
 * Description:
 *   run test script main routine.
 *
 *   Ref) Manual Tester Source Code
 *     JPT,JPK satoh@pc101:/libUartAt064c
 *     PTB,PTC satoh@pc101:/libUartMobile058b
 *     GMK simofuji@pc101:/serial/libUartAt028S12n
 *     GMN simofuji@pc101:/serial/libUartAt040
 *     STN simofuji@pc101:/serial/libUartAt040u
 *
 * </pre>
 * 018EG based 018B  for EB7(2STEP)
 *****************************************************************************/
void runTestScriptMain(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  //  Byte *bData = malloc(1024 * 1024);
  //  ts_printf(__FILE__, __LINE__, "[DEBUG]");
  //  downloadFile(tscb, "hotrun.sh", bData, 395);
  //  putBinaryDataDump(bData, 0, 395);
  //  return;

  //  int a = 0;
  //  ts_sleep_slumber(5);
  //  a = 100 / a;
  //  for (;;) ts_sleep_slumber(5);
  //  return;

  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;

  switch (tscb->isTestMode) {
  case NORMAL_TEST_SCRIPT:
    runTestScriptSrst(tscb);
    break;
  case HOST_COM_LOOPBACK_TEST:
    runTestScriptHostComLoopbackTest(tscb);
    break;
  case HOST_COM_DOWNLOAD_TEST:
    runTestScriptHostComDownloadTest(tscb);
    break;
  case HOST_COM_MESSAGE_TEST:
    runTestScriptHostComMessageTest(tscb);
    break;
  case HOST_COM_UPLOAD_TEST:
    runTestScriptHostComUploadTest(tscb);
    break;
  case DRIVE_COM_READ_TEST:
    runTestScriptDriveComReadTest(tscb);
    break;
  case DRIVE_COM_READ_THEN_HOST_COM_UPLOAD_TEST:
    runTestScriptDriveComReadThenHostComUploadTest(tscb);
    break;
  case DRIVE_COM_ECHO_TEST:
    runTestScriptDriveComEchoTest(tscb);
    break;
  case DRIVE_COM_ECHO_TEST_WITH_POWER_CYCLE:
    runTestScriptDriveComEchoTestWithPowerCycle(tscb);
    break;
  case POWER_ON_ONLY_TEST:
    runTestScriptPowerOnOnly(tscb);
    break;
  case DRIVE_POLL_ONLY_TEST:
    runTestScriptDrivePollOnly(tscb);
    break;
  case DRIVE_TEMPERATURE_TEST:
    runTestScriptTemperatureTest(tscb);
    break;
  case DRIVE_VOLTAGE_TEST:
    runTestScriptVoltageTest(tscb);
    break;
  default:
    ts_printf(__FILE__, __LINE__, "invalid parameter isTestMode=%d", tscb->isTestMode);
    tseb->isFatalError = 1;
    break;
  }
}

/**
 * <pre>
 * Description:
 *   run test script main routine.
 * </pre>
 *****************************************************************************/
void runTestScriptDestructor(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  requestDriveUnload(tscb);
  turnDrivePowerSupplyOff(tscb);
  createErrorCode(tscb);
  tscb->dwTimeStampAtTestEnd = getTestScriptTimeInSec();
  reportHostTesterLog(tscb);
  reportHostTestComplete(tscb, REPORT_COMP);
}

/**
 * <pre>
 * Description:
 *   run SRST test script.
 * </pre>
 *****************************************************************************/
void runTestScriptSrst(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc = 1;
  Dword dwLength = 0;
  Byte *bString = NULL;
  Dword dwTime = 0;
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;

  /************ SRST PART************/
  /* power off */
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOff");
  rc = turnDrivePowerSupplyOff(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }


  reportHostTestMessage(tscb, TESTCODE_VERSION);       /*  [019KPD]  */
  ts_printf(__FILE__, __LINE__, TESTCODE_VERSION);     /*  [019KPD]  */
  tscb->bSwtablePickupf = 0;
  rc = vcGetVersion(&bString, &dwLength);

  if (sizeof(tscb->bLibraryVersion) < dwLength) {   /*  [019KP3] */
    ts_printf(__FILE__, __LINE__, "Library Version data Size Error: %d < %d (length)", sizeof(tscb->bLibraryVersion), dwLength);
    goto FINALIZE_PROCESS;
  }
  memcpy(&tscb->bLibraryVersion, (Byte *)&bString[0], dwLength);
  tscb->dwVersionLength = dwLength;
  if (!rc) {
    ts_printf(__FILE__, __LINE__, "Library Version : %s", bString);
  } else {
    ts_printf(__FILE__, __LINE__, "Library Version getting fail rc=%d", rc);
    goto FINALIZE_PROCESS;
  }

  if (tscb->isNoDriveFinalized) {
    reportHostTestMessage(tscb, "No Finalize Mode");
  }
  ts_sleep_slumber(1);
  tscb->dwTimeStampAtTurnDrivePowerSupplyOn = getTestScriptTimeInSec();
  /* power on */
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
  rc = turnDrivePowerSupplyOn(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* identify */
  tscb->bPartFlag = SRST_PART;
  tscb->dwTimeStampAtIdentifyDrive = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "identifyDrive");
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  ts_printf(__FILE__, __LINE__, "**********************  identifyDrive  **********************");
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  rc = identifyDrive(tscb);

  if (rc == FINAL_MODE) { /* rc = FINAL_MODE(2) means SRST has finished already from signature*/
    reportHostTestMessage(tscb, "Signature means finished SRST part already,jump to Final part");
    ts_printf(__FILE__, __LINE__, "Final test timeout calculation is skipped here."); /* [019KPC] */
    goto FINAL_SRST;  /* Jump to Final from judgement signature */
  }
  if (rc >= 10) {
    reportHostTestMessage(tscb, "Identify Error. rc = %d", rc);
    goto FINALIZE_PROCESS;
  }

  if (tscb->dwDriveTestElapsedTimeSec) {
    ts_printf(__FILE__, __LINE__, "Resume!! TestTime will change.  elapsed %d sec", tscb->dwDriveTestElapsedTimeSec);

    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToSRSTEndCutoff %ld sec", tscb->dwTimeoutSecFromStartToSRSTEndCutoff);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToSRSTEnd       %ld sec", tscb->dwTimeoutSecFromStartToSRSTEnd);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToFinalEndCutoff %ld sec", tscb->dwTimeoutSecFromStartToFinalEndCutoff);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToFinalEnd       %ld sec", tscb->dwTimeoutSecFromStartToFinalEnd);
    tscb->dwTimeoutSecFromStartToSRSTEndCutoff -= tscb->dwDriveTestElapsedTimeSec;  /* resuem at final will be minus */
    tscb->dwTimeoutSecFromStartToSRSTEnd -= tscb->dwDriveTestElapsedTimeSec;         /* resuem at final will be minus */
    tscb->dwTimeoutSecFromStartToFinalEndCutoff -= tscb->dwDriveTestElapsedTimeSec;  /* resuem at final will be minus */
    tscb->dwTimeoutSecFromStartToFinalEnd -= tscb->dwDriveTestElapsedTimeSec;         /* resuem at final will be minus */
    /* Final test is no need change testtime.*/
    ts_printf(__FILE__, __LINE__, "Time Changed dwTimeoutSecFromStartToSRSTEndCutoff %ld sec", tscb->dwTimeoutSecFromStartToSRSTEndCutoff);
    ts_printf(__FILE__, __LINE__, "Time Changed dwTimeoutSecFromStartToSRSTEnd       %ld sec", tscb->dwTimeoutSecFromStartToSRSTEnd);
    ts_printf(__FILE__, __LINE__, "Time Changed dwTimeoutSecFromStartToFinalEndCutoff %ld sec", tscb->dwTimeoutSecFromStartToFinalEndCutoff);
    ts_printf(__FILE__, __LINE__, "Time Changed dwTimeoutSecFromStartToFinalEnd       %ld sec", tscb->dwTimeoutSecFromStartToFinalEnd);
  } else {
    ts_printf(__FILE__, __LINE__, "No Resume!! This is initial Test.elapsed %ld sec", tscb->dwDriveTestElapsedTimeSec);
  }


  /* status polling */
  tscb->dwTimeStampAtWaitDriveTestCompleted = getTestScriptTimeInSec();
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  ts_printf(__FILE__, __LINE__, "**********************  waitDriveTempCompleted SRST part  **********************");
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  reportHostTestMessage(tscb, "waitDriveTestCompleted SRST part");
  rc = waitDriveTestCompleted(tscb);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "waitDriveTestCompleted Error rc=%d", rc);
    goto FINALIZE_PROCESS;
  }


  /* get rawdata */
  tscb->dwTimeStampAtGetDriveRawdata = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "getDriveRawdata");
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  ts_printf(__FILE__, __LINE__, "**********************  getDriveRawdata  **********************");
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  rc = getDriveRawdata(tscb);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "getDriveRawdata Error rc=%d", rc);
    goto FINALIZE_PROCESS;
  }
  reportHostTestMessage(tscb, "Finished SRST part,jump to Final part");

  createErrorCode(tscb);
  tscb->dwTimeStampAtSRSTTestEnd = getTestScriptTimeInSec();

  if (tscb->dwDriveExtendedErrorCode) {
    goto FINALIZE_PROCESS;
  }

  reportHostTesterLog(tscb);
  reportHostTestComplete(tscb, REPORT_PRECOMP);



FINAL_SRST:
//  y.katayama 2010.11.04 (example: jump to label A)
  /**********************************************************************************/
  /********************************    JUMP   ***************************************/
  /**********************************************************************************/
  /* power off */
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOff");
  rc = turnDrivePowerSupplyOff(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }
  ts_sleep_slumber(1);

  TCPRINTF("dwDriveTestElapsedTimeSec = %d", (int)tscb->dwDriveTestElapsedTimeSec);

  rc = JumpProcess("A");
  if (rc) {
    reportHostTestMessage(tscb, "JUMP REQ ERROR");
    tseb->isFatalError = 1;
    goto FINALIZE_PROCESS;
  }

  reportHostTestMessage(tscb, "waiting NT condition");

  rc = WaitNT();
  if (rc) {
    tseb->isFatalError = 1;
    goto FINALIZE_PROCESS;
  }


  /**********************************************************************************/
  /******************************** JUMP END ***************************************/
  /**********************************************************************************/

  /***********************************/
  /************ FINAL PART************/
  /***********************************/
  if (tscb->dwTimeoutSecFromStartToSRSTEnd > getTestScriptTimeInSec()) {
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToSRSTEndCutoff %ld sec", tscb->dwTimeoutSecFromStartToSRSTEndCutoff);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToSRSTEnd       %ld sec", tscb->dwTimeoutSecFromStartToSRSTEnd);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToFinalEndCutoff = %ld sec", tscb->dwTimeoutSecFromStartToFinalEndCutoff);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToFinalEnd = %ld sec", tscb->dwTimeoutSecFromStartToFinalEnd);
    ts_printf(__FILE__, __LINE__, "getTestScriptTimeInSec()  %ld sec", getTestScriptTimeInSec());

    dwTime = tscb->dwTimeoutSecFromStartToSRSTEnd - getTestScriptTimeInSec(); /* saving time */
    tscb->dwTimeoutSecFromStartToFinalEnd -= dwTime;
    tscb->dwTimeoutSecFromStartToFinalEndCutoff -= dwTime;

    ts_printf(__FILE__, __LINE__, "saving time  %ld sec", dwTime);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToFinalEndCutoff = %ld sec(CHANGED)", tscb->dwTimeoutSecFromStartToFinalEndCutoff);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToFinalEnd = %ld sec(CHANGED)", tscb->dwTimeoutSecFromStartToFinalEnd);
  }
  /* reportHostUpdateStartTime(tscb); *//* Report Final Start date to PCPGM due to Final Test time is longer than manual for 40 mins of cooling. 019KP8 2011.12.08 */
  reportHostTestMessage(tscb, "Comp NT condition");
  reportHostTestMessage(tscb, "Start Final Part");

  optTime(tscb); /* optimize SRST time out based on reduction time */

  tscb->dwTimeStampAtTurnDrivePowerSupplyOn = getTestScriptTimeInSec();
  /* power on */
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
  rc = turnDrivePowerSupplyOn(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* identify */
  tscb->bPartFlag = FINAL_PART;
  tscb->dwTimeStampAtIdentifyDrive = getTestScriptTimeInSec();
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  ts_printf(__FILE__, __LINE__, "**********************  identifyDrive Final part  **********************");
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  reportHostTestMessage(tscb, "identifyDrive Final part");
  rc = identifyDrive(tscb);
  if (rc) {
    reportHostTestMessage(tscb, "FINAL PART Identify Error. rc = %d", rc);
    goto FINALIZE_PROCESS;
  }

  /* status polling */
  tscb->dwTimeStampAtWaitDriveTestCompleted = getTestScriptTimeInSec();
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  ts_printf(__FILE__, __LINE__, "**********************  waitFinalTestCompleted Final part  **********************");
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  reportHostTestMessage(tscb, "waitFinalTestCompleted Final part");
  rc = waitDriveTestCompleted(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* get rawdata */
  tscb->dwTimeStampAtGetDriveRawdata = getTestScriptTimeInSec();
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  ts_printf(__FILE__, __LINE__, "**********************  getDriveFinalRawdata  **********************");
  ts_printf(__FILE__, __LINE__, "************************************************************************************************");
  reportHostTestMessage(tscb, "getDriveFinalRawdata");
  rc = getDriveRawdata(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }


  /*************************/
  /* send finilize message */
  /*************************/


  /* finalize */
FINALIZE_PROCESS:
  /* tscb->bPartFlag = FINAL_PART;*/  /* this sentense needs separate for preComp SRST or Comp */
  freeMemoryForSaveData(tscb);
  tscb->dwTimeStampAtFinalize = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "finalize");


  return;
}

/**
 * <pre>
 * Description:
 *   testTime Optimize
 * </pre>
 *****************************************************************************/
void optTime(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{

  /* need to get SRST reduction time   and   standard time based sequence file*/
  /* reduction time = standard time - elapsed time */
  ts_printf(__FILE__, __LINE__, "OptimizeTime dwTimeoutSecFromStartToFinalEndCutoff %d sec", tscb->dwTimeoutSecFromStartToFinalEndCutoff);
  ts_printf(__FILE__, __LINE__, "OptimizeTime dwTimeoutSecFromStartToFinalEnd       %d sec", tscb->dwTimeoutSecFromStartToFinalEnd);
  ts_printf(__FILE__, __LINE__, "Elapsed Time  %d sec", tscb->dwDriveTestElapsedTimeSec);
  ts_printf(__FILE__, __LINE__, "getTestScriptTimeInSec()  %d sec", getTestScriptTimeInSec());

  tscb->dwTimeStampAtFinalTestStart = getTestScriptTimeInSec(); /* Final Part Start Time */

}
/**
 * <pre>
 * Description:
 *   run communication test script.
 * </pre>
 *****************************************************************************/
void runTestScriptHostComLoopbackTest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  ZQUE *zque = NULL;
  Dword dwHugCpuCounter = 0;
  Dword i = 0;

  /* com test */
  reportHostTestMessage(tscb, "HOST_COM_LOOPBACK_TEST");
  for (i = 0 ; i < tscb->dwTestModeParam1 ; i++) {
    if (!ts_qis()) {
      ts_sleep_partial(1);
      continue;
    }
    zque = (ZQUE *)ts_qget();
    reportHostTestData(tscb, KEY_TESTER_DUMMY_DATA, &zque->bData[0], zque->wLgth);
    ts_qfree((void *)zque);

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }
  }

  return;
}

/**
 * <pre>
 * Description:
 *   run communication test script.
 * </pre>
 *****************************************************************************/
void runTestScriptHostComDownloadTest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  ZQUE *zque = NULL;
  Dword dwHugCpuCounter = 0;
  Dword i = 0;

  /* com test */
  reportHostTestMessage(tscb, "HOST_COM_DOWNLOAD_TEST");
  for (i = 0 ; i < tscb->dwTestModeParam1 ; i++) {
    if (!ts_qis()) {
      ts_sleep_partial(1);
      continue;
    }
    zque = (ZQUE *)ts_qget();
    reportHostTestData(tscb, KEY_TESTER_DUMMY_DATA, &zque->bData[0], zque->wLgth);
    ts_qfree((void *)zque);

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }
  }

  return;
}

/**
 * <pre>
 * Description:
 *   run communication test script.
 * </pre>
 *****************************************************************************/
void runTestScriptHostComMessageTest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Dword dwHugCpuCounter = 0;
  Dword i = 0;
  Dword dwStartTime = 0;
  Dword dwEndTime = 0;
  Dword dwElapsedTime = 0;
  Dword dwNumOfCommand = 0;
  Dword dwTotalDataSize = 0;
  Dword dwIops = 0;
  Dword dwThroughtput = 0;
  Byte bMessageBuffer[128];

  /* create buffer */
  if (tscb->dwTestModeParam2 >= sizeof(bMessageBuffer)) {
    tscb->dwTestModeParam2 = sizeof(bMessageBuffer) - 1;
  }

  memset(&bMessageBuffer[0], '?', tscb->dwTestModeParam2);
  bMessageBuffer[tscb->dwTestModeParam2] = '\0';

  /* com test */
  reportHostTestMessage(tscb, "HOST_COM_MESSAGE_TEST");
  dwStartTime = getTestScriptTimeInSec();
  for (i = 0 ; i < tscb->dwTestModeParam1 ; i++) {
    reportHostTestMessage(tscb, &bMessageBuffer[0]);

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }
  }
  dwEndTime = getTestScriptTimeInSec();

  dwElapsedTime = dwEndTime - dwStartTime;
  dwNumOfCommand = tscb->dwTestModeParam1;
  dwTotalDataSize = dwNumOfCommand * tscb->dwTestModeParam2;
  if (dwElapsedTime > 0) {
    dwIops = dwNumOfCommand / dwElapsedTime;
    dwThroughtput = dwTotalDataSize / dwElapsedTime;
  } else {
    dwIops = 0;
    dwThroughtput = 0;
  }

  reportHostTestMessage(tscb, "HostComMessageTest result = [%dcmd,%dbyte,%dsec,%diops,%dbyte/sec]",
                        dwNumOfCommand,
                        dwTotalDataSize,
                        dwElapsedTime,
                        dwIops,
                        dwThroughtput
                       );
  ts_printf(__FILE__, __LINE__, "HostComMessageTest result = [%dcmd,%dbyte,%dsec,%diops,%dbyte/sec]",
            dwNumOfCommand,
            dwTotalDataSize,
            dwElapsedTime,
            dwIops,
            dwThroughtput
           );

  return;
}

/**
 * <pre>
 * Description:
 *   run communication test script.
 * </pre>
 *****************************************************************************/
void runTestScriptHostComUploadTest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Dword dwHugCpuCounter = 0;
  Dword i = 0;
  Dword dwStartTime = 0;
  Dword dwEndTime = 0;
  Dword dwElapsedTime = 0;
  Dword dwNumOfCommand = 0;
  Dword dwTotalDataSize = 0;
  Dword dwIops = 0;
  Dword dwThroughtput = 0;
  Byte *bUploadData = NULL;

  reportHostTestMessage(tscb, "HOST_COM_UPLOAD_TEST");

  /* range check */
  if (tscb->dwTestModeParam2 < HOST_COM_UPLOAD_TEST_MIN_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > HOST_COM_UPLOAD_TEST_MAX_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > sizeof(tscb->bSectorBuffer)) {
    tseb->isFatalError = 1;
    return;
  }

  /* initialze sending data (5 bytes header + byte incremental data)
   *
   * Header#1 : Cell ID (0x01, 0x02, ...)
   * Header#2 : Loop Count MSB (0x00, 0x00, 0x00, ...)
   * Header#3 : Loop Count  |  (0x00, 0x00, 0x00, ...)
   * Header#4 : Loop Count  |  (0x00, 0x00, 0x00, ...)
   * Header#5 : Loop Count LSB (0x00, 0x01, 0x02, ...)
   * Data#1   : 0x05 (Incremantal Data)
   * Data#2   : 0x06 (Incremantal Data)
   * Data#3   : 0x07 (Incremantal Data)
   * ..
   * .
   * Data#256 : 0xff (Incremantal Data)
   * Data#257 : 0x00 (Incremantal Data)
   * Data#258 : 0x01 (Incremantal Data)
   * Data#259 : 0x02 (Incremantal Data)
   * ..
   * .
   */
  bUploadData = &tscb->bSectorBuffer[0];
  bUploadData[0] = MY_CELL_NO + 1;
  for (i = 5 ; i < tscb->dwTestModeParam2 ; i++) {
    bUploadData[i] = i;
  }

  /* com test */
  dwStartTime = getTestScriptTimeInSec();
  for (i = 0 ; i < tscb->dwTestModeParam1 ; i++) {
    /* add header info */
    bUploadData[1] = (Byte)(0xff & (i >> 24));
    bUploadData[2] = (Byte)(0xff & (i >> 16));
    bUploadData[3] = (Byte)(0xff & (i >> 8));
    bUploadData[4] = (Byte)(0xff & i);

    /* report data */
    reportHostTestData(tscb, KEY_TESTER_DUMMY_DATA, &bUploadData[0], tscb->dwTestModeParam2);

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }
  }
  dwEndTime = getTestScriptTimeInSec();

  dwElapsedTime = dwEndTime - dwStartTime;
  dwNumOfCommand = tscb->dwTestModeParam1;
  dwTotalDataSize = dwNumOfCommand * tscb->dwTestModeParam2;
  if (dwElapsedTime > 0) {
    dwIops = dwNumOfCommand / dwElapsedTime;
    dwThroughtput = dwTotalDataSize / dwElapsedTime;
  } else {
    dwIops = 0;
    dwThroughtput = 0;
  }

  reportHostTestMessage(tscb, "HostComUploadTest result = [%dcmd,%dbyte,%dsec,%diops,%dbyte/sec]",
                        dwNumOfCommand,
                        dwTotalDataSize,
                        dwElapsedTime,
                        dwIops,
                        dwThroughtput
                       );
  ts_printf(__FILE__, __LINE__, "HostComUploadTest result = [%dcmd,%dbyte,%dsec,%diops,%dbyte/sec]",
            dwNumOfCommand,
            dwTotalDataSize,
            dwElapsedTime,
            dwIops,
            dwThroughtput
           );

  return;
}

/**
 * <pre>
 * Description:
 *   run communication test script.
 * </pre>
 *****************************************************************************/
void runTestScriptDriveComReadTest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  UART_PERFORMANCE_LOG_BLOCK *uplb_current = &tscb->uartPerformanceLogBlock[READ_DRIVE_MEMORY_PROTOCOL - READ_DRIVE_MEMORY_PROTOCOL];
  UART_PERFORMANCE_LOG_BLOCK uplb_prev = *uplb_current;
  Dword i = 0;
  Byte rc = 0;
  Dword dwHugCpuCounter = 0;
  Dword dwStartTime = 0;
  Dword dwEndTime = 0;
  Dword dwElapsedTime = 0;
  Dword dwNumOfCommand = 0;
  Dword dwNumOfCommandSuccess = 0;
  Dword dwTotalDataSize = 0;
  Dword dwIops = 0;
  Dword dwThroughtput = 0;

  /* range check */
  if (tscb->dwTestModeParam2 < DRIVE_COM_READ_TEST_MIN_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > DRIVE_COM_READ_TEST_MAX_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > sizeof(tscb->bSectorBuffer)) {
    tseb->isFatalError = 1;
    return;
  }

  /* power on */
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
  rc = turnDrivePowerSupplyOn(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* identify */
  reportHostTestMessage(tscb, "identifyDrive");
  rc = identifyDrive(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* unload */
  if (tscb->isForceDriveUnload) {
    ts_printf(__FILE__, __LINE__, "force drive unload flag on");
    for (i = 0 ; i < 5 ; i++) {
      rc = requestDriveUnload(tscb);
      if (rc == 0) {
        break;
      }
    }
  }
  if (rc) {
    tseb->isDriveIdentifyTimeout = 1;
    goto FINALIZE_PROCESS;
  }

  /* read drive memory */
  reportHostTestMessage(tscb, "DRIVE_COM_READ_TEST");
  uplb_prev = *uplb_current;
  dwStartTime = getTestScriptTimeInSec();
  for (i = 0 ; i < tscb->dwTestModeParam1 ; i++) {
    driveIoCtrl(tscb->dwTestModeParam3, tscb->dwTestModeParam2, &tscb->bSectorBuffer[0], READ_DRIVE_MEMORY_PROTOCOL, uplb_current);

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }
  }
  dwEndTime = getTestScriptTimeInSec();

  /* result */
  dwNumOfCommand = uplb_current->dwNumOfCommand - uplb_prev.dwNumOfCommand;
  dwNumOfCommandSuccess = uplb_current->dwNumOfCommandSuccess - uplb_prev.dwNumOfCommandSuccess;
  dwTotalDataSize = uplb_current->dwTotalDataSize - uplb_prev.dwTotalDataSize;
  dwElapsedTime = dwEndTime - dwStartTime;
  if (dwElapsedTime > 0) {
    dwIops = dwNumOfCommandSuccess / dwElapsedTime;
    dwThroughtput = dwTotalDataSize / dwElapsedTime;
  } else {
    dwIops = 0;
    dwThroughtput = 0;
  }

FINALIZE_PROCESS:
  reportHostTestMessage(tscb, "DriveComReadTest result = [%dcmd,%dok,%dbyte,%dsec,%diops,%dbyte/sec]",
                        dwNumOfCommand,
                        dwNumOfCommandSuccess,
                        dwTotalDataSize,
                        dwElapsedTime,
                        dwIops,
                        dwThroughtput
                       );
  ts_printf(__FILE__, __LINE__, "DriveComReadTest result = [%dcmd,%dok,%dbyte,%dsec,%diops,%dbyte/sec]",
            dwNumOfCommand,
            dwNumOfCommandSuccess,
            dwTotalDataSize,
            dwElapsedTime,
            dwIops,
            dwThroughtput
           );

  /* finalize */
  reportHostTestMessage(tscb, "finalize");

  return;
}

/**
 * <pre>
 * Description:
 *   run communication test script.
 * </pre>
 *****************************************************************************/
void runTestScriptDriveComReadThenHostComUploadTest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  UART_PERFORMANCE_LOG_BLOCK *uplb_current = &tscb->uartPerformanceLogBlock[READ_DRIVE_MEMORY_PROTOCOL - READ_DRIVE_MEMORY_PROTOCOL];
  UART_PERFORMANCE_LOG_BLOCK uplb_prev = *uplb_current;
  Dword i = 0;
  Byte rc = 0;
  Dword dwHugCpuCounter = 0;
  Dword dwStartTime = 0;
  Dword dwEndTime = 0;
  Dword dwElapsedTime = 0;
  Dword dwNumOfCommand = 0;
  Dword dwNumOfCommandSuccess = 0;
  Dword dwTotalDataSize = 0;
  Dword dwIops = 0;
  Dword dwThroughtput = 0;

  /* range check */
  if (tscb->dwTestModeParam2 < DRIVE_COM_READ_TEST_MIN_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > DRIVE_COM_READ_TEST_MAX_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > sizeof(tscb->bSectorBuffer)) {
    tseb->isFatalError = 1;
    return;
  }

  /* power on */
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
  rc = turnDrivePowerSupplyOn(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* identify */
  reportHostTestMessage(tscb, "identifyDrive");
  rc = identifyDrive(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* unload */
  if (tscb->isForceDriveUnload) {
    ts_printf(__FILE__, __LINE__, "force drive unload flag on");
    for (i = 0 ; i < 5 ; i++) {
      rc = requestDriveUnload(tscb);
      if (rc == 0) {
        break;
      }
    }
  }
  if (rc) {
    tseb->isDriveIdentifyTimeout = 1;
    goto FINALIZE_PROCESS;
  }

  /* read drive memory */
  reportHostTestMessage(tscb, "DRIVE_COM_READ_THEN_HOST_COM_UPLOAD_TEST");
  uplb_prev = *uplb_current;
  dwStartTime = getTestScriptTimeInSec();
  for (i = 0 ; i < tscb->dwTestModeParam1 ; i++) {
    /* read memory in drive */
    driveIoCtrl(tscb->dwTestModeParam3, tscb->dwTestModeParam2, &tscb->bSectorBuffer[0], READ_DRIVE_MEMORY_PROTOCOL, uplb_current);

    /* report data */
    reportHostTestData(tscb, KEY_TESTER_DUMMY_DATA, &tscb->bSectorBuffer[0], tscb->dwTestModeParam2);

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }
  }
  dwEndTime = getTestScriptTimeInSec();

  /* result */
  dwNumOfCommand = uplb_current->dwNumOfCommand - uplb_prev.dwNumOfCommand;
  dwNumOfCommandSuccess = uplb_current->dwNumOfCommandSuccess - uplb_prev.dwNumOfCommandSuccess;
  dwTotalDataSize = uplb_current->dwTotalDataSize - uplb_prev.dwTotalDataSize;
  dwElapsedTime = dwEndTime - dwStartTime;
  if (dwElapsedTime > 0) {
    dwIops = dwNumOfCommandSuccess / dwElapsedTime;
    dwThroughtput = dwTotalDataSize / dwElapsedTime;
  } else {
    dwIops = 0;
    dwThroughtput = 0;
  }

FINALIZE_PROCESS:
  reportHostTestMessage(tscb, "DriveComReadTest result = [%dcmd,%dok,%dbyte,%dsec,%diops,%dbyte/sec]",
                        dwNumOfCommand,
                        dwNumOfCommandSuccess,
                        dwTotalDataSize,
                        dwElapsedTime,
                        dwIops,
                        dwThroughtput
                       );
  ts_printf(__FILE__, __LINE__, "DriveComReadTest result = [%dcmd,%dok,%dbyte,%dsec,%diops,%dbyte/sec]",
            dwNumOfCommand,
            dwNumOfCommandSuccess,
            dwTotalDataSize,
            dwElapsedTime,
            dwIops,
            dwThroughtput
           );

  /* finalize */
  reportHostTestMessage(tscb, "finalize");

  return;
}


/**
 * <pre>
 * Description:
 *   run communication test script.
 * </pre>
 *****************************************************************************/
void runTestScriptDriveComEchoTest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  UART_PERFORMANCE_LOG_BLOCK *uplb_current = &tscb->uartPerformanceLogBlock[IDENTIFY_DRIVE_PROTOCOL - READ_DRIVE_MEMORY_PROTOCOL];
  UART_PERFORMANCE_LOG_BLOCK uplb_prev = *uplb_current;
  Dword i = 0;
  Byte rc = 0;
  Dword dwHugCpuCounter = 0;
  Dword dwStartTime = 0;
  Dword dwEndTime = 0;
  Dword dwElapsedTime = 0;
  Dword dwNumOfCommand = 0;
  Dword dwNumOfCommandSuccess = 0;
  Dword dwTotalDataSize = 0;
  Dword dwIops = 0;
  Dword dwThroughtput = 0;

  reportHostTestMessage(tscb, "runTestScriptDriveComEchoTest");

  /* range check */
  if (tscb->dwTestModeParam2 < DRIVE_COM_READ_TEST_MIN_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > DRIVE_COM_READ_TEST_MAX_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > sizeof(tscb->bSectorBuffer)) {
    tseb->isFatalError = 1;
    return;
  }

  /* power on */
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
  rc = turnDrivePowerSupplyOn(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* echo drive */
  reportHostTestMessage(tscb, "DRIVE_COM_ECHO_TEST");
  uplb_prev = *uplb_current;
  dwStartTime = getTestScriptTimeInSec();
  for (i = 0 ; i < tscb->dwTestModeParam1 ; i++) {
    driveIoCtrl(0, tscb->dwTestModeParam2, &tscb->bSectorBuffer[0], IDENTIFY_DRIVE_PROTOCOL, &tscb->uartPerformanceLogBlock[0]);

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }
  }
  dwEndTime = getTestScriptTimeInSec();

  /* result */
  dwNumOfCommand = uplb_current->dwNumOfCommand - uplb_prev.dwNumOfCommand;
  dwNumOfCommandSuccess = uplb_current->dwNumOfCommandSuccess - uplb_prev.dwNumOfCommandSuccess;
  dwTotalDataSize = uplb_current->dwTotalDataSize - uplb_prev.dwTotalDataSize;
  dwElapsedTime = dwEndTime - dwStartTime;
  if (dwElapsedTime > 0) {
    dwIops = dwNumOfCommandSuccess / dwElapsedTime;
    dwThroughtput = dwTotalDataSize / dwElapsedTime;
  } else {
    dwIops = 0;
    dwThroughtput = 0;
  }

#define UART_CHECK_CRITERIA  (60.0)    // unit in percent
  float fCommandSuccessRatio = (100.0 * dwNumOfCommandSuccess) / (1.0 * dwNumOfCommand);

  ts_printf(__FILE__, __LINE__, "fCommandSuccessRatio,%f", fCommandSuccessRatio);
  if (fCommandSuccessRatio < UART_CHECK_CRITERIA) {
    tseb->isDriveIdentifyTimeout = 1;
  }

FINALIZE_PROCESS:
  reportHostTestMessage(tscb, "DriveComEchoTest result = [%dcmd,%dok,%dbyte,%dsec,%diops,%dbyte/sec]",
                        dwNumOfCommand,
                        dwNumOfCommandSuccess,
                        dwTotalDataSize,
                        dwElapsedTime,
                        dwIops,
                        dwThroughtput
                       );
  ts_printf(__FILE__, __LINE__, "DriveComEchoTest result = [%dcmd,%dok,%dbyte,%dsec,%diops,%dbyte/sec]",
            dwNumOfCommand,
            dwNumOfCommandSuccess,
            dwTotalDataSize,
            dwElapsedTime,
            dwIops,
            dwThroughtput
           );

  /* finalize */
  reportHostTestMessage(tscb, "finalize");
  /* manipulate error code
   *     note: this function judges pass or fail under communication failure rate only.
   *           it needs to ignore drive native error code and rawdata reporting complete flag.
   */
  tscb->wDriveNativeErrorCode = 0;
  tscb->isDriveRawdataComplete = 1;
  return;
}

/**
 * <pre>
 * Description:
 *   run communication test script.
 * </pre>
 *****************************************************************************/
void runTestScriptDriveComEchoTestWithPowerCycle(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  UART_PERFORMANCE_LOG_BLOCK *uplb_current = &tscb->uartPerformanceLogBlock[IDENTIFY_DRIVE_PROTOCOL - READ_DRIVE_MEMORY_PROTOCOL];
  UART_PERFORMANCE_LOG_BLOCK uplb_prev = *uplb_current;
  Dword i = 0;
  Byte rc = 0;
  Dword dwHugCpuCounter = 0;
  Dword dwStartTime = 0;
  Dword dwEndTime = 0;
  Dword dwElapsedTime = 0;
  Dword dwNumOfCommand = 0;
  Dword dwNumOfCommandSuccess = 0;
  Dword dwTotalDataSize = 0;
  Dword dwIops = 0;
  Dword dwThroughtput = 0;

  reportHostTestMessage(tscb, "runTestScriptDriveComEchoTestWithPowerCycle");

  /* range check */
  if (tscb->dwTestModeParam2 < DRIVE_COM_READ_TEST_MIN_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > DRIVE_COM_READ_TEST_MAX_DATA_SIZE) {
    tseb->isFatalError = 1;
    return;
  }
  if (tscb->dwTestModeParam2 > sizeof(tscb->bSectorBuffer)) {
    tseb->isFatalError = 1;
    return;
  }

  /* power on */
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
  rc = turnDrivePowerSupplyOn(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* echo drive */
  reportHostTestMessage(tscb, "DRIVE_COM_ECHO_TEST");
  uplb_prev = *uplb_current;
  dwStartTime = getTestScriptTimeInSec();
  for (i = 0 ; i < tscb->dwTestModeParam1 ; i++) {
    driveIoCtrl(0, tscb->dwTestModeParam2, &tscb->bSectorBuffer[0], IDENTIFY_DRIVE_PROTOCOL, &tscb->uartPerformanceLogBlock[0]);

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }

    reportHostTestMessage(tscb, "turnDrivePowerSupplyOff");
    rc = turnDrivePowerSupplyOff(tscb);
    if (rc) {
      goto FINALIZE_PROCESS;
    }
    ts_sleep_slumber(10);

    /* power on */
    reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
    rc = turnDrivePowerSupplyOn(tscb);
    if (rc) {
      goto FINALIZE_PROCESS;
    }
    rc = clearFPGABuffers( MY_CELL_NO + 1 , RECEIVE_BUFFER_ID );
    if (rc) {
    reportHostTestMessage(tscb, "clear FPGA Buffers Error. rc = %d", rc);
     goto FINALIZE_PROCESS;
    }
    ts_sleep_slumber(1);
  }
  dwEndTime = getTestScriptTimeInSec();

  /* result */
  dwNumOfCommand = uplb_current->dwNumOfCommand - uplb_prev.dwNumOfCommand;
  dwNumOfCommandSuccess = uplb_current->dwNumOfCommandSuccess - uplb_prev.dwNumOfCommandSuccess;
  dwTotalDataSize = uplb_current->dwTotalDataSize - uplb_prev.dwTotalDataSize;
  dwElapsedTime = dwEndTime - dwStartTime;
  if (dwElapsedTime > 0) {
    dwIops = dwNumOfCommandSuccess / dwElapsedTime;
    dwThroughtput = dwTotalDataSize / dwElapsedTime;
  } else {
    dwIops = 0;
    dwThroughtput = 0;
  }

#define UART_CHECK_CRITERIA  (60.0)    // unit in percent
  float fCommandSuccessRatio = (100.0 * dwNumOfCommandSuccess) / (1.0 * dwNumOfCommand);

  ts_printf(__FILE__, __LINE__, "fCommandSuccessRatio,%f", fCommandSuccessRatio);
  if (fCommandSuccessRatio < UART_CHECK_CRITERIA) {
    tseb->isDriveIdentifyTimeout = 1;
  }

FINALIZE_PROCESS:
  reportHostTestMessage(tscb, "DriveComEchoTest result = [%dcmd,%dok,%dbyte,%dsec,%diops,%dbyte/sec]",
                        dwNumOfCommand,
                        dwNumOfCommandSuccess,
                        dwTotalDataSize,
                        dwElapsedTime,
                        dwIops,
                        dwThroughtput
                       );
  ts_printf(__FILE__, __LINE__, "DriveComEchoTest result = [%dcmd,%dok,%dbyte,%dsec,%diops,%dbyte/sec]",
            dwNumOfCommand,
            dwNumOfCommandSuccess,
            dwTotalDataSize,
            dwElapsedTime,
            dwIops,
            dwThroughtput
           );

  /* finalize */
  reportHostTestMessage(tscb, "finalize");
  /* manipulate error code
   *     note: this function judges pass or fail under communication failure rate only.
   *           it needs to ignore drive native error code and rawdata reporting complete flag.
   */
  tscb->wDriveNativeErrorCode = 0;
  tscb->isDriveRawdataComplete = 1;
  return;
}


/**
 * <pre>
 * Description:
 *   run power on only test script.
 * </pre>
 *****************************************************************************/
void runTestScriptPowerOnOnly(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 1;

  reportHostTestMessage(tscb, "runTestScriptPowerOnOnly");

  /* power on */
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
  rc = turnDrivePowerSupplyOn(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* timeout manipulation */
  if (tscb->dwDriveTestElapsedTimeSec < tscb->dwTimeoutSecFromStartToSRSTEnd) {
    tscb->dwTimeoutSecFromStartToSRSTEnd -= tscb->dwDriveTestElapsedTimeSec;
  } else {
    tscb->dwTimeoutSecFromStartToSRSTEnd = 0;
  }
  ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToSRSTEnd = %d sec", tscb->dwTimeoutSecFromStartToSRSTEnd);

  /* loop */
  for (;;) {
    reportHostTestMessage(tscb, "loop %ld / %ld sec", getTestScriptTimeInSec(), tscb->dwTimeoutSecFromStartToSRSTEnd);
    ts_printf(__FILE__, __LINE__, "loop %ld / %ld sec", getTestScriptTimeInSec(), tscb->dwTimeoutSecFromStartToSRSTEnd);

    /* error check */
    if (tseb->isFatalError) {
      ts_printf(__FILE__, __LINE__, "error");
      rc = 1;
      break;
    }

    /* env check */
    rc = isEnvironmentError(tscb);
    if (rc) {
      rc = 1;
      break;
    }

    rc = getEnvironmentStatus(tscb);
    if (rc) {
      rc = 1;
      break;
    }

    /* timeout check */
    if (tscb->dwTimeoutSecFromStartToSRSTEnd < getTestScriptTimeInSec()) {
      ts_printf(__FILE__, __LINE__, "drive test timeout %dsec", tscb->dwTimeoutSecFromStartToSRSTEnd);
      rc = 0;
      break;
    }

    /* wait */
    ts_sleep_partial(tscb->dwDriveTestStatusPollingIntervalTimeSec);
  }

FINALIZE_PROCESS:
  /* finalize */
  reportHostTestMessage(tscb, "finalize");

  return;
}


/**
 * <pre>
 * Description:
 *   run SRST test script.
 * </pre>
 *****************************************************************************/
void runTestScriptDrivePollOnly(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc = 1;
  Byte skip = 1;

  reportHostTestMessage(tscb, "runTestScriptDrivePollOnly");

  /* power on */
  tscb->dwTimeStampAtTurnDrivePowerSupplyOn = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
  rc = turnDrivePowerSupplyOn(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* identify */
  tscb->dwTimeStampAtIdentifyDrive = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "identifyDrive");
  rc = identifyDrive(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* status polling */
  tscb->dwTimeStampAtWaitDriveTestCompleted = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "waitDriveTestCompleted3");
  rc = waitDriveTestCompleted(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  if (skip) {
    reportHostTestMessage(tscb, "skip rawdata dump");
  } else {
    /* get rawdata */
    tscb->dwTimeStampAtGetDriveRawdata = getTestScriptTimeInSec();
    reportHostTestMessage(tscb, "getDriveRawdata");
    rc = getDriveRawdata(tscb);
    if (rc) {
      goto FINALIZE_PROCESS;
    }
  }

  /* finalize */
FINALIZE_PROCESS:
  tscb->dwTimeStampAtFinalize = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "finalize");

  return;
}


/**
 * <pre>
 * Description:
 *   run SRST test script.
 *
 *   Testcase evaluate temperature for stabilazed phase shown in below figure P2
 *   with given parameters (*). Test time (dwTimeoutSecFromStartToSRSTEnd) should be more
 *   than 2 hours to secure enough P2 period at least 1 hour.
 *
 *   (*) dwTestModeParam1 for drive AE temperature criteria min
 *       dwTestModeParam2 for drive AE temperature criteria max
 *       dwTestModeParam3 for P1 period (ignore evaluating for tempearature ramp)
 *
 *   Obviously temperature sequence shall be described for this test.
 *    - single temperature target
 *    - more than test time (dwTimeoutSecFromStartToSRSTEnd)
 *
 *   Temp
 *    |
 *    |       --------------\
 *    |      /   |         | \
 *    |     /    |         |  \
 *    |    /     |         |   \
 *    |   /      |         |    \
 *    |  /       |         |     \
 *    | /        |         |      \
 *    +------------------------------> Time
 *     |   P1    |   P2    |  P3  |
 *     |<------->|<------->|<---->|
 *     |  N hr   |  M hrs  | 0.5hr|
 *
 *   Note) P3 phase is after test script finish. Testcase will set safe handling
 *   mode and wait temperature until particular safe temperature.
 *
 * </pre>
 *****************************************************************************/
void runTestScriptTemperatureTest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
#define MAX_CRITERIA      (100)
  Byte rc = 1;
  Byte skip = 1;
  Byte bDriveTemperatureCriteriaMax = 0;
  Byte bDriveTemperatureCriteriaMin = 0;
  Dword dwIgnoreEvaluatingTimeSec = 0;
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;

  reportHostTestMessage(tscb, "runTestScriptTemperatureTest");

  if (tscb->dwTestModeParam1 < MAX_CRITERIA) {
    bDriveTemperatureCriteriaMin = tscb->dwTestModeParam1;
  } else {
    ts_printf(__FILE__, __LINE__, "invalid param1 %ld, it should be < %d", tscb->dwTestModeParam1, MAX_CRITERIA);
    tseb->isFatalError = 1;
    goto FINALIZE_PROCESS;
  }

  if (tscb->dwTestModeParam2 < MAX_CRITERIA) {
    bDriveTemperatureCriteriaMax = tscb->dwTestModeParam2;
  } else {
    ts_printf(__FILE__, __LINE__, "invalid param2 %ld, it should be < %d", tscb->dwTestModeParam2, MAX_CRITERIA);
    tseb->isFatalError = 1;
    goto FINALIZE_PROCESS;
  }

  if (bDriveTemperatureCriteriaMin > bDriveTemperatureCriteriaMax) {
    ts_printf(__FILE__, __LINE__, "invalid param1 (%d) / param2 (%d) conbination. param1 should be < param2 ", bDriveTemperatureCriteriaMin, bDriveTemperatureCriteriaMax);
    tseb->isFatalError = 1;
    goto FINALIZE_PROCESS;
  }

  dwIgnoreEvaluatingTimeSec = tscb->dwTestModeParam3;

  /* power on */
  tscb->dwTimeStampAtTurnDrivePowerSupplyOn = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "turnDrivePowerSupplyOn");
  rc = turnDrivePowerSupplyOn(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }

  /* identify */
  tscb->dwTimeStampAtIdentifyDrive = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "identifyDrive");
  rc = identifyDrive(tscb);
  if (rc) {
    goto FINALIZE_PROCESS;
  }


  /* status polling */
  tscb->dwTimeStampAtWaitDriveTestCompleted = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "waitDriveTestCompleted2");

  ts_printf(__FILE__, __LINE__, "waitDriveTestCompleted2");

  if (tscb->dwDriveTestElapsedTimeSec < tscb->dwTimeoutSecFromStartToSRSTEnd) {
    tscb->dwTimeoutSecFromStartToSRSTEnd -= tscb->dwDriveTestElapsedTimeSec;
  } else {
    tscb->dwTimeoutSecFromStartToSRSTEnd = 0;
  }
  ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToSRSTEnd = %d sec", tscb->dwTimeoutSecFromStartToSRSTEnd);

  for (;;) {

    /* error check */
    if (tseb->isFatalError) {
      ts_printf(__FILE__, __LINE__, "error");
      rc = 1;
      break;
    }

    /* env check */
    rc = isEnvironmentError(tscb);
    if (rc) {
      break;
    }

    rc = getEnvironmentStatus(tscb);
    if (rc) {
      break;
    }

    /* drive status */
    rc = getDriveTemperature(tscb);
    if (!rc) {
      rc = getSRSTstep(tscb);
      if (!rc) {
        reportHostTestStatus(tscb);
        recordDriveAndEnvironmentStatus(tscb);
      }
    }

    /* timeout check */
    if (tscb->dwTimeoutSecFromStartToSRSTEnd < getTestScriptTimeInSec()) {
      ts_printf(__FILE__, __LINE__, "drive test timeout %dsec", tscb->dwTimeoutSecFromStartToSRSTEnd);
      ts_printf(__FILE__, __LINE__, "but this is not failure for this script");
      //      tseb->isDriveTestTimeout = 1;
      //      rc = 1;
      break;
    }

    /* debug flag */
    if (tscb->isForceDriveTestCompFlagOn) {
      ts_printf(__FILE__, __LINE__, "force drive test comp flag on");
      requestDriveTestAbort(tscb);
    }
    if (tscb->isForceDriveUnload) {
      ts_printf(__FILE__, __LINE__, "force drive unload flag on");
      requestDriveUnload(tscb);
    }

    /* wait */
    ts_sleep_partial(tscb->dwDriveTestStatusPollingIntervalTimeSec);
  }
  if (rc) {
    goto FINALIZE_PROCESS;
  }


  if (skip) {
    reportHostTestMessage(tscb, "skip rawdata dump");
  } else {
    /* get rawdata */
    tscb->dwTimeStampAtGetDriveRawdata = getTestScriptTimeInSec();
    reportHostTestMessage(tscb, "getDriveRawdata");
    rc = getDriveRawdata(tscb);
    if (rc) {
      goto FINALIZE_PROCESS;
    }
  }

  /* evaluate trace */
  Dword dwCounter = tscb->dwTestScriptRecorderBlockCounter;
  Dword i = 0;
  Dword dwPassCount = 0;
  Dword dwFailCount = 0;

  ts_printf(__FILE__, __LINE__, "evaluate trace %ld", dwCounter);
  for (i = 0 ; i < dwCounter ; i++) {
    if (tscb->testScriptRecorderBlock[i].dwDriveTestTimeSec < dwIgnoreEvaluatingTimeSec) {
      continue;
    }

    if (tscb->testScriptRecorderBlock[i].bDriveTemperature < bDriveTemperatureCriteriaMin) {
      dwFailCount++;
    } else if (bDriveTemperatureCriteriaMax < tscb->testScriptRecorderBlock[i].bDriveTemperature) {
      dwFailCount++;
    } else {
      dwPassCount++;
    }
  }
  ts_printf(__FILE__, __LINE__, "pass,%ld,fail,%ld", dwPassCount, dwFailCount);

#define TEMP_CHECK_CRITERIA  (98.0)    // unit in percent
  float fPassRatio = (100.0 * dwPassCount) / (1.0 * (dwPassCount + dwFailCount));

  ts_printf(__FILE__, __LINE__, "fPassRatio,%f", fPassRatio);
  if (fPassRatio < TEMP_CHECK_CRITERIA) {
    tseb->isCellTemperatureAbnormal = 1;
    ts_printf(__FILE__, __LINE__, "failed!!!");
  } else {
    ts_printf(__FILE__, __LINE__, "passed!!!");
  }

  /* finalize */
FINALIZE_PROCESS:
  tscb->dwTimeStampAtFinalize = getTestScriptTimeInSec();
  reportHostTestMessage(tscb, "finalize");

  return;
}
/**
 * <pre>
 * Description:
 *   run voltage test
 * </pre>
 *****************************************************************************/
void runTestScriptVoltageTest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 1;
  Dword dwPollTimeInSec = 1;
  Dword i = 0;

  reportHostTestMessage(tscb, "runTestScriptVoltageTest");

  /* loop */
  for (;;) {

    /* error check */
    if (tseb->isFatalError) {
      ts_printf(__FILE__, __LINE__, "error");
      rc = 1;
      goto FINALIZE_PROCESS;
    }

    /* un-plug check */
    for (;;) {
      rc = isDrivePlugged(MY_CELL_NO + 1);
      ts_sleep_slumber(dwPollTimeInSec);  // must check here to prevent chattering
      if (rc == 0) {
        break;
      }
    }

    /* plug check */
    for (;;) {
      rc = isDrivePlugged(MY_CELL_NO + 1);
      ts_sleep_slumber(dwPollTimeInSec);  // must check here to prevent chattering
      if (rc) {
        break;
      }
    }

    reportHostTestMessage(tscb, "find");
    ts_printf(__FILE__, __LINE__, "find");

    siSetLed(MY_CELL_NO + 1, 3);

    /* power on */
    rc = setTargetVoltage(MY_CELL_NO + 1, tscb->dwDrivePowerSupply5V, tscb->dwDrivePowerSupply12V);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      goto FINALIZE_PROCESS;
    }

    for (i = 0 ; i < tscb->dwDrivePowerOnWaitTime ; ) {
      rc = getCurrentVoltage(MY_CELL_NO + 1, &tscb->wCurrentV5InMilliVolts, &tscb->wCurrentV12InMilliVolts);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "error %d", rc);
        tseb->isFatalError = 1;
        goto FINALIZE_PROCESS;
      }
      ts_sleep_slumber(dwPollTimeInSec);
      i += dwPollTimeInSec;
    }

    /* power off */
    rc = setTargetVoltage(MY_CELL_NO + 1, 0, 0);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      goto FINALIZE_PROCESS;
    }

    siSetLed(MY_CELL_NO + 1, 1);
  }

FINALIZE_PROCESS:
  /* finalize */
  reportHostTestMessage(tscb, "finalize");

  return;
}

/**
 * <pre>
 * Description:
 *   Jump request to chamber sclip
 * </pre>
 *****************************************************************************/
Byte JumpProcess(Byte *string)
{
  Dword JumpTime;

  /*if (tccb.optionSyncManagerDebug) {*/
  SM_MESSAGE smMessage;
  smMessage.type   = SM_TS_SD_JMPREQ;
  Byte rc = 0;

  ts_printf(__FILE__, __LINE__, "JUMP to \"%s\"", string);

  strcpy(smMessage.string, string);
  rc = smSendSyncMessage(&smMessage, SM_TS_SD_TIMEOUT);  /* JUMP REQUEST */
  if (rc) {
    ts_printf(__FILE__, __LINE__, "JUMP REQ ERROR LABEL is %s", string);
    return 1;
  }
  rc = SM_MESSAGE_UNAVAILABLE;
  JumpTime = getTestScriptTimeInSec();
  ts_printf(__FILE__, __LINE__, "ACK wait JumpTime = %d", JumpTime);
  while (1) {
    rc = smQuerySyncMessage(SM_TS_RV_JMPACK);  /*SM_TS_RV_JMPACK defined 2 in syncmanager\libsm.h */
    if (rc == SM_MESSAGE_AVAILABLE) {
      break;
    }
    ts_sleep_partial(WAIT_TIME_SEC_FOR_CHAMBER_SCRIPT_RESPONSE_POLL);   //wait for other testscript(sync request timing would be severalminute to several hour)
    if ((getTestScriptTimeInSec() - JumpTime) > 1200) {
      ts_printf(__FILE__, __LINE__, "ACK wating time out !!!!!");
      return 2;
    }
  }
  TCPRINTF("MESSAGE AVAILABLE");

  rc = smReceiveSyncMessage(SM_TS_RV_TIMEOUT, &smMessage);
  if (rc != SM_NO_ERROR) {
    /*reportHostTestMessage(tscb, "JUMP ACK ERROR1");*/
    ts_printf(__FILE__, __LINE__, "JUMP ACK ERROR1");
    return 3;
  }
  if (smMessage.type != SM_TS_RV_JMPACK) {
    /*reportHostTestMessage(tscb, "JUMP ACK ERROR2");*/
    ts_printf(__FILE__, __LINE__, "JUMP ACK ERROR2");
    return 4;
  }
  /*     Removed 2011/02/02 no check jump ack at Manual tester and Hi speed uart --> should be same specification
     if(smMessage.param[0]!=0){
       TCPRINTF("jmp_ack_error!!!!!param[0]= %d",(int)smMessage.param[0]);
       reportHostTestMessage(tscb, "JUMP ACK ERROR3");
       return 5;
     }
  */
  ts_sleep_partial(10);   //wait for other testscript(sync request timing would be severalminute to several hour)
  /*}*/
  return 0;
}
/**
 * <pre>
 * Description:
 *   waiting NT condition ,
 * </pre>
 *****************************************************************************/
Byte WaitNT(void)
{
  Byte rc = 0;
  Dword JumpTime;
  SM_MESSAGE smMessage;

  JumpTime = getTestScriptTimeInSec();
  ts_printf(__FILE__, __LINE__, "NT wait JumpTime = %d", JumpTime);
  while (1) {
    rc = smQuerySyncMessage(SM_TS_RV_FINALSTART);  /*SM_TS_RV_JMPACK defined 2 in syncmanager\libsm.h */
    if (rc == SM_MESSAGE_AVAILABLE) {
      break;
    }
    ts_sleep_partial(WAIT_TIME_SEC_FOR_CHAMBER_SCRIPT_RESPONSE_POLL);   //wait for other testscript(sync request timing would be severalminute to several hour)
    if ((getTestScriptTimeInSec() - JumpTime) > 3600) {
      ts_printf(__FILE__, __LINE__, "NT wating time out !!!!! JumpTime:%d Now:%d", JumpTime, getTestScriptTimeInSec());
      return 1;
    }
  }

  rc = smReceiveSyncMessage(SM_TS_RV_TIMEOUT, &smMessage);
  if (rc != SM_NO_ERROR) {
    TCPRINTF("FINAL START MESSAGE ERROR !!!!!");
    ts_printf(__FILE__, __LINE__, "FINAL START MESSAGE ERROR !!!!!");
    return 2;
  }
  if (smMessage.type != SM_TS_RV_FINALSTART) {
    TCPRINTF("FINAL START MESSAGE error!!!!!param[0]= %d", (int)smMessage.param[0]);
    ts_printf(__FILE__, __LINE__, "FINAL START MESSAGE error!!!!!param[0]= %d", (int)smMessage.param[0]);
    return 3;
  }
  return 0;
}
