#include "ts.h"

/**
 * <pre>
 * Description:
 *   Create error code following test status.
 * Arguments:
 *   *tscb - Test script control block
 * Note:
 *   E/C Definition On DB
 *     GOD1 - Power sense check fail
 *     GOD2 - Too many serial communication error
 *     GOD3 - Unit reason / reserved area write/read fail
 *     GOD5 - Communication refuse
 *     GOD6 - Reporting flag check fail
 *     GOD8 - Others
 *     GOD9 - Report time is not enough
 *     GODL - Remodeling required
 *
 *   E/C Definition On ec2pf.tbl
 *     0x10001 : GOD2 : cannot communicate with drive
 *     0x15001 : GODL : remodeling for VNC
 *     0x15002 : GODL : remodeling for VNC
 *     0x20002 : 6110 : 6110 for desktop
 *     0x20003 : 6010 :
 *     0x30000 : GOD3 : unit reason
 *     0x40000 : 6110 : test time is not enough
 *     0x40001 : 6114 : room temp test time is not enough
 *     0x40002 : 6100 : too bad or over criteria
 *     0x40003 : 6104 : too bad or over criteria
 *     0x50000 : GOD5 : cannot get S/N
 *     0x60000 : GOD6 : cannot finalize rawdata reporting mode
 *     0x70000 : GOD7 : 2nd time SRST
 *     0x80002 : 5910 : 6110 for desktop (final)
 *
 *   GODx related error definition by Shimofuji (25th April 2008)
 *     EC depends on test phases.
 *     If communication-error is occured, the ECs below will be assigned.
 *
 *     Start            Rawdata ready         Report Comp.
 *     |-------------------|--------------------|
 *     |<--GOD2----------->|<----  GOD3 ------->| <- GOD9
 *     GOD5
 *
 *     GOD2 Before Rawdata Ready
 *     GOD3 After Rawdata Ready
 *     GOD5 cannot get S/N from drive within 10 minutes after power-on.
 *     GOD9 No EC is reported from logic. ( logic hung or reporting time is not enough )
 *
 *     Where, "communication-error" includes error between drive and logic , reserved R/W fail, logical fail in logic.
 *
 *   6100/6110 error definition by Shimofuji (25th April 2008)
 *     3.5 product
 *       6110   When a drive receives PREPDATA, Drive starts reporting rawdata as usual.
 *              Even if GOD3 is occured during reporting,EC6110 will be assigned to skip DL station process.
 *       6100   Not assigned
 *
 *     2.5 product
 *       6110/6100
 *         When a drive receives PREPDATA, Drive starts reporting rawdata as usual.
 *         SRST-code itself judges 6110or6100.
 *         EC6110 will not assigned when GOD3 is occured.
 *       i.e. GOD3 is not rescued even if the drive is 61x0.
 *
 *   Re SRST Prohibition
 *        prohibit = (prohibit >> 2) & 0x03;
 *        if (timeover[ rc ] || srstEC[ rc ] == 0x31) {
 *                extEC [ rc ] = 4;
 *                srstEC [ rc ] = prohibit [ rc ];
 *        }
 *  * </pre>
 *****************************************************************************/
Byte createErrorCode(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Dword dwReportErrorCode = 0xFFFF;
  Byte rc = 0;
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte bi;
  Byte *bcheck;

  ts_printf(__FILE__, __LINE__, "checkError Flag");

  bcheck = (Byte *)tseb;
  for (bi = 0;bi < sizeof(*tseb);bi++) {
    if (*bcheck++ != 0) {
      tscb->dwDriveExtendedErrorCode = 0xFFFFF;
    }
  }

  if ((tscb->isDriveRawdataComplete != 1) && (tscb->wDriveNativeErrorCode == 0)) {
    tscb->dwDriveExtendedErrorCode = 0xFFFF0;
  }

  ts_printf(__FILE__, __LINE__, "createErrorCode initial%05x", tscb->dwDriveExtendedErrorCode);

  /* convert failSymptoms to errorCode */
  if (tseb->isFatalError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_cell_abort_by_test_script;
    ts_printf(__FILE__, __LINE__, "Error1");
  } else if (tseb->isAbortByUser) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_cell_abort_by_user;
    ts_printf(__FILE__, __LINE__, "Error2");
  } else if (tseb->isDriveUnplugged) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_cell_insertion_check_error;
    ts_printf(__FILE__, __LINE__, "Error3");
  } else if (tseb->isDrivePowerFail) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_cell_voltage_error;
    ts_printf(__FILE__, __LINE__, "Error4");
  } else if (tseb->isCellTemperatureAbnormal) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_cell_temperature_error;
    ts_printf(__FILE__, __LINE__, "Error5");
  } else if (tseb->isDriveIdentifyTimeout) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_cannot_get_sn; /*GOD5*/
    ts_printf(__FILE__, __LINE__, "Error6");
  } else if (tscb->bDriveSkip_SRST_result) { /*[COM_22] */
    if (tscb->wDriveNativeErrorCode == 0) {
      tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_no_error_code;
    } else {
      tscb->dwDriveExtendedErrorCode = tscb->wDriveNativeErrorCode;
    }
  } else if (tseb->isForceStopBySrst) {
    tscb->dwDriveExtendedErrorCode = tscb->wDriveNativeErrorCode;
    ts_printf(__FILE__, __LINE__, "Error7");
  } else if (tseb->isGradingError) {
    ts_printf(__FILE__, __LINE__, "Grading Error");
    switch (tseb->isGradingError) {
    case 0x01:
      tscb->dwDriveExtendedErrorCode = ERROR_CODE_grading_nopass2ID;
      break;/* NO pass2ID in SW tbale [2STEP_02] */
    case 0x02:
      tscb->dwDriveExtendedErrorCode = ERROR_CODE_grading_noswtb;
      break;/* Nothing SW tbale with Grading flag [2STEP_02] */
    case 0x04:
      tscb->dwDriveExtendedErrorCode = ERROR_CODE_grading_wrpass2ID;
      break;/* Write pass2 ID fail [2STEP_02] */
    case 0x08:
      tscb->dwDriveExtendedErrorCode = ERROR_CODE_grading_pending;
      break;/* Detect "PEND" Signeture fail [2STEP_02] */
    default:
      tscb->dwDriveExtendedErrorCode = 0xFFFFFFFF;
      break;
    }
  } else if (tseb->isDriveRawdataDumpPageIndexError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_unit_reason;  /*GOD3*/
    ts_printf(__FILE__, __LINE__, "Error8");
    if ((tscb->wDriveEchoVersion >= 2) && (tseb->isDriveSinetureError)) { /* JPT or later */
      tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_cannot_finalizereporting;
      ts_printf(__FILE__, __LINE__, "Error9");
    }

  } else if (tseb->isDriveRawdataFinalizeError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_detect_abnormalreporting; /*GOD6*/
    ts_printf(__FILE__, __LINE__, "Error10");
  } else if (tseb->isDriveRawdataDumpTimeoutError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_reporting_timeout;
    ts_printf(__FILE__, __LINE__, "Error11");
  } else if (tseb->isDriveRawdataDumpReportingFlagError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_reporting_timeout;
    ts_printf(__FILE__, __LINE__, "Error12");
  } else if (tseb->isDriveRawdataDumpDriveError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_unit_reason;  /*GOD3*/
    ts_printf(__FILE__, __LINE__, "Error13");
    if ((tscb->wDriveEchoVersion >= 2) && (tseb->isDriveSinetureError)) { /* JPT or later */
      tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_cannot_finalizereporting;
      ts_printf(__FILE__, __LINE__, "Error14");
    }

  } else if (tseb->isDriveRawdataDumpTotalPageError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_unit_reason;  /*GOD3*/
    ts_printf(__FILE__, __LINE__, "Error15");
  } else if (tseb->isDriveRawdataDumpPesPageError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_unit_reason;  /*GOD3*/
    ts_printf(__FILE__, __LINE__, "Error16");
  } else if (tseb->isDriveRawdataDumpOtherError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_unit_reason;  /*GOD3*/
    ts_printf(__FILE__, __LINE__, "Error17");
  } else if (tseb->isDriveWriteImmidiateflagError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_unit_reason;  /*GOD3*/
    ts_printf(__FILE__, __LINE__, "Error18");
  } else if (tseb->isDriveWriteRetryRequestError) {
    tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_unit_reason;  /*GOD3*/
    ts_printf(__FILE__, __LINE__, "Error19");
  } else if (tseb->isDriveTestTimeout) {
    if (tscb->isDriveRawdataComplete) {
      if (tscb->bCheckflag == 0) { /* for 6100 operation of JTB Jan/24/2011 Furukawa*/
        tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_re_srst_ng;  /* E/C6100 Re-SRST is NG */
        ts_printf(__FILE__, __LINE__, "1 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
        return rc;
      }
      if (tscb->isReSrstProhibitControl) {
        if (tscb->wDriveReSrstProhibit & IS_RE_SRST_NG) {  /* IS_RE_SRST_NG is signal from SRST */
          tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_re_srst_ng;  /* E/C6100 Re-SRST is NG */
          ts_printf(__FILE__, __LINE__, "2 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
        } else {
          if (tscb->bPartFlag == SRST_PART) {
            tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_re_srst_ok; /* ERROR_CODE_EXT_unit_reason; changed GOD3 -> 6110*/
            ts_printf(__FILE__, __LINE__, "3 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
          } else if (tscb->bPartFlag == FINAL_PART) {
            tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_enough_time_final;
            ts_printf(__FILE__, __LINE__, "4 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
          }
        }
      } else {
        if (tscb->bPartFlag == SRST_PART) {
          tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_re_srst_ok; /* ERROR_CODE_EXT_unit_reason; changed GOD3 -> 6110*/
          ts_printf(__FILE__, __LINE__, "5 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
        } else if (tscb->bPartFlag == FINAL_PART) {
          tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_enough_time_final;
          ts_printf(__FILE__, __LINE__, "6 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
        }
      }
    } else {
      if (tscb->bPartFlag == SRST_PART) {
        tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_re_srst_ok; /* ERROR_CODE_EXT_unit_reason; changed GOD3 -> 6110*/
        ts_printf(__FILE__, __LINE__, "7 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
      } else if (tscb->bPartFlag == FINAL_PART) {
        tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_enough_time_final;
        ts_printf(__FILE__, __LINE__, "8 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
      }
    }
  } else if (tseb->isDriveSinetureError) {
    if (tscb->bPartFlag == SRST_PART) {
      tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_re_srst_ok; /* ERROR_CODE_EXT_unit_reason; changed GOD3 -> 6110*/
      ts_printf(__FILE__, __LINE__, "9 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
    } else if (tscb->bPartFlag == FINAL_PART) {
      tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_enough_time_final;
      ts_printf(__FILE__, __LINE__, "10 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
    }
  } else if (tscb->wDriveNativeErrorCode == ERROR_CODE_srst_incomplete_re_srst_ok) {
    if (tscb->isReSrstProhibitControl) {
      if (tscb->wDriveReSrstProhibit & IS_RE_SRST_NG) {
        tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_re_srst_ng;
        ts_printf(__FILE__, __LINE__, "11 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
      } else {
        if (tscb->bPartFlag == SRST_PART) {
          tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_re_srst_ok; /* ERROR_CODE_EXT_unit_reason; changed GOD3 -> 6110*/
          ts_printf(__FILE__, __LINE__, "12 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
        } else if (tscb->bPartFlag == FINAL_PART) {
          tscb->dwDriveExtendedErrorCode = ERROR_CODE_EXT_enough_time_final;
          ts_printf(__FILE__, __LINE__, "13 Part=%d,isDriveTestTimeout = %d : isDriveRawdataComp = %d : isReSrstProhibitControl = %d ", tscb->bPartFlag, tseb->isDriveTestTimeout, tscb->isDriveRawdataComplete, tscb->isReSrstProhibitControl);
        }
      }
    } else {
      tscb->dwDriveExtendedErrorCode = tscb->wDriveNativeErrorCode;
      ts_printf(__FILE__, __LINE__, "possible GOD3");
    }

  } /*else {
    tscb->dwDriveExtendedErrorCode = tscb->wDriveNativeErrorCode;
  }*/

  ts_printf(__FILE__, __LINE__, "Before check Native E/C %04xh -> Extend E/C %08xh", tscb->wDriveNativeErrorCode, tscb->dwDriveExtendedErrorCode);

  if (tscb->dwDriveExtendedErrorCode == 0) {
    dwReportErrorCode = tscb->wDriveNativeErrorCode;
    ts_printf(__FILE__, __LINE__, "Error20");
  } else {
    dwReportErrorCode = tscb->dwDriveExtendedErrorCode;
    ts_printf(__FILE__, __LINE__, "Error21");
  }
  tscb->dwDriveExtendedErrorCode = dwReportErrorCode;

  /* Error Code */
  ts_printf(__FILE__, __LINE__, "After check Native E/C %04xh -> Extend E/C %08xh", tscb->wDriveNativeErrorCode, tscb->dwDriveExtendedErrorCode);

  /* Error Flags */
  putBinaryDataDump((Byte *)tseb, 0, sizeof(*tseb));

  /* Fake Error Code */
  if (tscb->isFakeErrorCode) {
    tscb->dwDriveExtendedErrorCode = tscb->dwFakeErrorCode;
    ts_printf(__FILE__, __LINE__, "overwriting Fake E/C -> %08xh", tscb->dwDriveExtendedErrorCode);
  }

  return rc;
}
