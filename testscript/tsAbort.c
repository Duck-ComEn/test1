#include "ts.h"
/*
name change 2010.10.01 By Enomoto
dwDriveMcsTotalSizeByte  --->  dwDriveParametricSizeByte
dwDriveMcsDumpSizeByte   --->  dwDriveParametricDumpSizeByte
*/
/**
 * <pre>
 * Description:
 *   Abort drive test and request rawdata preparation.
 * </pre>
 *****************************************************************************/
Byte requestDriveTestAbort(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 1;

  ts_printf(__FILE__, __LINE__, "abortDriveTest");

  /* error check */
  if (tseb->isFatalError) {
    ts_printf(__FILE__, __LINE__, "error");
    return 1;
  }

  /* check identify done */
  if (!tscb->isDriveIdentifyComplete) {
    ts_printf(__FILE__, __LINE__, "skip by identify incomplete");
    return 1;
  }

  /* SRST stop, and request test rawdata */
  /*  remove 2010.12.07 by Furukawa because it cannot read
    rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_IMMEDIATE_FLAG, 0, READ_FROM_DRIVE);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "Error - read memory");
      return 1;
    }
  */
  tscb->bSectorBuffer[0] = 0; /* 2010.12.07 add */
  tscb->bSectorBuffer[0] |= IS_TEST_TIME_OUT; /* or by 0x01 */
  rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_IMMEDIATE_FLAG, 0, WRITE_TO_DRIVE); /* abort SRST then reprot flag ON --> PREPDATA */
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Abort Error - write memory");
  }

  return rc;
}

/**
 * <pre>
 * Description:
 *   Unload drive head.
 * </pre>
 *****************************************************************************/
Byte requestDriveUnload(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte isFatalErrorCheck = 0;
  Byte rc = 1;

  ts_printf(__FILE__, __LINE__, "unloadDrive");

  /* error check */
  if (isFatalErrorCheck) {
    if (tseb->isFatalError) {
      ts_printf(__FILE__, __LINE__, "skip by fatal error flag");
      return 1;
    }
  }

  /* check identify done */
  if (!tscb->isDriveIdentifyComplete) {
    ts_printf(__FILE__, __LINE__, "skip by identify incomplete");
    return 1;
  }

  /* drive unload */
  /*
    rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_IMMEDIATE_FLAG, 0, READ_FROM_DRIVE);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "Error - write memory");
      return 1;
    }
  */
  tscb->bSectorBuffer[0] = 0; /* 2010.12.07 add */
  tscb->bSectorBuffer[0] |= IS_EMERGENCY_POWER_SHUTDOWN;
  rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_IMMEDIATE_FLAG, 0, WRITE_TO_DRIVE);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Warning - unload command failure -> no way, ignore");
  } else {
    ts_sleep_slumber(SLEEP_TIME_AFTER_DRIVE_UNLOAD_SEC);
  }

  return rc;
}
