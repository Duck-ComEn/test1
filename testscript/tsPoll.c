#include "ts.h"



/**
 * <pre>
 * Description:
 *   Wait drive test completed.  FOR FINAL PART * FOR FINAL PART * FOR FINAL PART * FOR FINAL PART
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte waitDriveTestCompleted(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Dword i = 0;
  Dword dwtime;
  Byte rc = 1;
  Byte bStatePrev = IS_GET_STATUS_AND_CONTROL;
  Byte bStateNow = IS_GET_TEMPERATURE;

  if (tscb->bPartFlag == FINAL_PART) {
    ts_printf(__FILE__, __LINE__, "waitDriveFinalTestCompleted");
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToFinalEndCutoff = %ld sec", tscb->dwTimeoutSecFromStartToFinalEndCutoff);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToFinalEnd = %ld sec", tscb->dwTimeoutSecFromStartToFinalEnd);
    ts_printf(__FILE__, __LINE__, "FINAL PART getTestScriptTimeInSec() = %ld sec", getTestScriptTimeInSec());
    ts_printf(__FILE__, __LINE__, "dwDriveTestElapsedTimeSec = %ld sec", tscb->dwDriveTestElapsedTimeSec);


  } else {
    ts_printf(__FILE__, __LINE__, "waitDriveTestCompleted");
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToSRSTEndCutoff = %ld sec", tscb->dwTimeoutSecFromStartToSRSTEndCutoff);
    ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToSRSTEnd = %ld sec", tscb->dwTimeoutSecFromStartToSRSTEnd);
    ts_printf(__FILE__, __LINE__, "SRST PART getTestScriptTimeInSec() = %ld sec", getTestScriptTimeInSec());
    ts_printf(__FILE__, __LINE__, "dwDriveTestElapsedTimeSec = %ld sec", tscb->dwDriveTestElapsedTimeSec);
  }
  /*
  if (tscb->dwDriveFinalTestElapsedTimeSec < tscb->dwTimeoutSecFromStartToFinalEndCutoff) {  For resume use
    tscb->dwTimeoutSecFromStartToFinalEndCutoff -= tscb->dwDriveFinalTestElapsedTimeSec;
  } else {
    tscb->dwTimeoutSecFromStartToFinalEndCutoff = 0;
  }

  ts_printf(__FILE__, __LINE__, "dwTimeoutSecFromStartToFinalEndCutoff = %d sec", tscb->dwTimeoutSecFromStartToFinalEndCutoff);
  ts_printf(__FILE__, __LINE__, "FINAL PART getTestScriptTimeInSec() = %d sec",getTestScriptTimeInSec());
  ts_printf(__FILE__, __LINE__, "dwDriveFinalTestElapsedTimeSec = %d sec", tscb->dwDriveFinalTestElapsedTimeSec);
  */


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

    bStatePrev = bStateNow;
    /* run state machine */
    switch (bStateNow) {

    case IS_GET_TEMPERATURE: /* Get temperature with report status to PC */
      rc = getDriveTemperature(tscb);
      if (!rc) {
        reportHostTestStatus(tscb);
        recordDriveAndEnvironmentStatus(tscb);
      }
      bStateNow = IS_GET_SRST_STEP;
      break;

    case IS_GET_SRST_STEP: /* Get SRST step */
      rc = getSRSTstep(tscb);
      ts_printf(__FILE__, __LINE__, "SRST step %xh  temperature %d", tscb->bDriveStepCount, tscb->bDriveTemperature);
      bStateNow = IS_GET_STATUS_AND_CONTROL;
      break;

    case IS_GET_STATUS_AND_CONTROL: /* Get temperature with report status to PC */
      rc = getDriveRawdataStatusAndControl(tscb); /* Getting SRST status */
      if (!rc) {
        rc = isDriveRawdataReady(tscb); /* If reporting flag ON, return code = 1 It means finshed SRST */
        if (rc) {
          ts_printf(__FILE__, __LINE__, "drive rawdata ready");
          rc = 0;
          bStateNow = IS_RAWDATA_READ_STATUS;  /* SRST test comp */
          break; /* finished this loop */
        } else {
          if (tscb->bDriveSkip_SRST_result) {
            rc = Getdriveerrorcode(tscb);
            if (!rc) {
              rc = 0;
              bStateNow = IS_RAWDATA_READ_STATUS;  /* Only Memory Report */
              break;
            }
          }
          rc = Getdriveerrorcode(tscb);
          if (!rc) {
            for (i = 0 ; i < MAX_NUMBER_OF_FORCE_STOP_PF_CODE  ; i++) {
              if (tscb->wDriveForceStopPFcode[i] == 0) {
                break;
              } else if (tscb->wDriveForceStopPFcode[i] == tscb->wDriveNativeErrorCode) {
                ts_printf(__FILE__, __LINE__, "force stop test by SRST EC %dsec", tscb->wDriveNativeErrorCode);
                tseb->isForceStopBySrst = 1;
                rc = 1;
                break;
              }
            }
          }
        }
      }
      bStateNow = IS_GET_TEMPERATURE;
      break;
    default:
      ts_printf(__FILE__, __LINE__, "invalid state!!");
      tseb->isFatalError = 1;
      rc = 1;
      break;
    } /* switch close */
    if (tseb->isForceStopBySrst) {
      ts_printf(__FILE__, __LINE__, "Force Stop error");
      break;
    }
    /* state change check */
    if (bStateNow == IS_RAWDATA_READ_STATUS) {
      break;
    } else if (bStateNow != bStatePrev) {
      ts_printf(__FILE__, __LINE__, "change state %d -> %d", bStatePrev, bStateNow);
    }

    if (tscb->bPartFlag == FINAL_PART) {
      dwtime = tscb->dwTimeoutSecFromStartToFinalEndCutoff;
    } else {
      dwtime = tscb->dwTimeoutSecFromStartToSRSTEndCutoff;
    }
    if (dwtime < getTestScriptTimeInSec()) {
      if (tscb->bPartFlag == FINAL_PART) {
        ts_printf(__FILE__, __LINE__, "drive Final test cutoff time %dlsec", tscb->dwTimeoutSecFromStartToFinalEndCutoff);
		tseb->isDriveTestTimeout = 2;  /* E/C 5910 */
      } else {
        ts_printf(__FILE__, __LINE__, "drive SRST  test cutoff time %dlsec", tscb->dwTimeoutSecFromStartToSRSTEndCutoff);
		tseb->isDriveTestTimeout = 1;  /* E/C 6110 */
      }
      rc = requestDriveTestAbort(tscb);   /* Issue PREPDATA */
      if ((rc) && (tscb->wDriveEchoVersion == 0)) {
        rc = turnDrivePowerSupplyOff(tscb);
        if (!rc) {
          rc = turnDrivePowerSupplyOn(tscb);
        }
        if (rc) {
          ts_printf(__FILE__, __LINE__, "POR Error!!");
          tseb->isFatalError = 1;
          return rc;
        }
        rc = requestDriveTestAbort(tscb);
      }

      if (rc) {
        ts_printf(__FILE__, __LINE__, "Abort Fail (GOD3)");
        tseb->isDriveWriteImmidiateflagError = 1;
      }
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
  }  /* for loop */

  return rc;
}

/**
 * <pre>
 * Description:
 *   Record drive & env status
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
void recordDriveAndEnvironmentStatus(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Dword dwCounter = tscb->dwTestScriptRecorderBlockCounter;
  TEST_SCRIPT_RECORDER_BLOCK *tsrb = &tscb->testScriptRecorderBlock[dwCounter];

  if (dwCounter >= (sizeof(tscb->testScriptRecorderBlock) / sizeof(tscb->testScriptRecorderBlock[0]))) {
    /* memory full */
    return;
  }

  tsrb->dwDriveTestTimeSec = getTestScriptTimeInSec();
  tsrb->wTempInHundredth = tscb->wCurrentTempInHundredth;
  tsrb->wV5InMilliVolts = tscb->wCurrentV5InMilliVolts;
  tsrb->wV12InMilliVolts = tscb->wCurrentV12InMilliVolts;
  tsrb->bDriveTemperature = tscb->bDriveTemperature;
  tsrb->bDriveStepCount = tscb->bDriveStepCount;

  tscb->dwTestScriptRecorderBlockCounter++;
  return;
}
