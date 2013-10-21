#include "ts.h"

/**
 * <pre>
 * Description:
 *   Identify drive with "ECHO" protocol in UART. It contains drive S/N.
 *
 * Arguments:
 *   *tscb - Test script control block
 *
 * Querying Data in Echo Command
 *   Function is getSN_Echo() in comUart.c
 *   wDriveEchoVersion ----> Echo data format version
 *   Drive CPU A00
 *     0:PTB,PTC Couldn't use Echo Command
 *     -:GMN  srstMfgID, srstSN
 *     -:STN  srstMfgID, srstSN, srstMicroLevel ,srstNumberOfServoSector, srstHeadTable, tpiinfo, srstSpindleSpeed
 *
 *   Drive CPU ARM
 *     2:JPK,JPT      srstMfgID, srstSN, srstMicroLevel ,srstNumberOfServoSector, srstHeadTable, tpiinfo, srstSpindleSpeed
 *     4:EB7,JET,JGB  srstMfgID, srstSN, srstMicroLevel ,srstNumberOfServoSector, srstHeadTable, tpiinfo, srstSpindleSpeed,srst,addressSRSTTop
 *     4:MRK,MNR,MRS  srstMfgID, srstSN, srstMicroLevel ,srstNumberOfServoSector, srstHeadTable, tpiinfo, srstSpindleSpeed,srst,addressSRSTTop
 *
 * Multi Grading at Function
 *   MFG-ID in echo command and MFG-ID on label would be different.
 *   MFG-ID on label is in SRST sequence.
 *   Report both S/N to PC
 *
 * Drive Voltage Report Format
 *   Dwrod  Contents
 *   ------------------------
 *   0      5V reading value (e.g 5.05V ==> 500) : Big endian
 *   1      Reserved
 *   2      12V reading value (e.g 11.96V ==> 1196) : Big endian
 *   3      Reserved
 *   4      Reserved
 *   5      Reserved
 *
 * </pre>
 ***************************************************************************************
   2010/05/13 Support JPT or later Echo command procedure Y.Enomoto
 *****************************************************************************/
Byte identifyDrive(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 1;
  Dword dwTimeoutSec = 0;
  Byte bStatePrev = IDENTIFY_DRIVE_FSM_before_initialize;
//  Byte bStateNow = IDENTIFY_DRIVE_FSM_srst_sequence;
  Byte bStateNow = IDENTIFY_DRIVE_FSM_data_from_Echo_before_sequence;
  Dword dwHugCpuCounter = 0;


  ts_printf(__FILE__, __LINE__, "identifyDrive");
  dwTimeoutSec = tscb->dwDriveTestIdentifyTimeoutSec;
  ts_printf(__FILE__, __LINE__, "timeout = %d sec", dwTimeoutSec);

  if (tscb->bPartFlag == FINAL_PART) {
    ts_printf(__FILE__, __LINE__, "dwTimeoutSec = %d", dwTimeoutSec);
    dwTimeoutSec = dwTimeoutSec + tscb->dwTimeStampAtFinalTestStart;
    ts_printf(__FILE__, __LINE__, "dwTimeoutSec for Final = %d", dwTimeoutSec);
  }

  for (;;) {

    /* error check */
    if (tseb->isFatalError) {
      ts_printf(__FILE__, __LINE__, "fatal error in Identify");
      return 10;
    }

    /* timeout check */
    if (dwTimeoutSec < getTestScriptTimeInSec()) {
      ts_printf(__FILE__, __LINE__, "Error - identify timeout");
      tseb->isDriveIdentifyTimeout = 1;
      return 11;
    }

    /* state change check */
    if (bStateNow != bStatePrev) {
      ts_printf(__FILE__, __LINE__, "change state %d -> %d", bStatePrev, bStateNow);
    } else {
      ts_sleep_partial(WAIT_TIME_SEC_FOR_DRIVE_IDENTIFY_POLL);
    }
    bStatePrev = bStateNow;

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }

    /* run state machine */
    switch (bStateNow) {

    case IDENTIFY_DRIVE_FSM_data_from_Echo_before_sequence: /* srst sequence (check if srst sequence valid) */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_data_from_Echo_before_sequence****");
      rc = 0;
      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        rc = getEchoData(tscb);
      }
      if (!rc) {
        bStateNow = IDENTIFY_DRIVE_FSM_serial_number;
      }
      break;

    case IDENTIFY_DRIVE_FSM_serial_number: /* drive serial number */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_serial_number****");
      rc = 0;
      if (tscb->wDriveEchoVersion < 2) { /* Legacy product (PTB,PTC) */
        rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_MFG_ID, 0, READ_FROM_DRIVE);
        if (!rc) {
          memmove(&tscb->bDriveMfgId[0], (Word *)&tscb->bSectorBuffer[0], sizeof(tscb->bDriveMfgId));
          memmove(&tscb->bDrivePass2Id[0], (Word *)&tscb->bSectorBuffer[0], sizeof(tscb->bDriveMfgId));  /* temporaly */
          memmove(&tscb->bDriveSerialNumber[0], &tscb->bSectorBuffer[sizeof(tscb->bDriveMfgId)], sizeof(tscb->bDriveSerialNumber));
        }
      }
      if (!rc) {
        if (!ts_isalnum_mem(&tscb->bDriveMfgId[0], sizeof(tscb->bDriveMfgId))) {
          ts_printf(__FILE__, __LINE__, "Error - data contents invalid (MfgID %s)", tscb->bDriveMfgId);
          tseb->isFatalError = 1;
          return 12;
        }
        if (!ts_isalnum_mem(&tscb->bDriveSerialNumber[0], sizeof(tscb->bDriveSerialNumber))) {
          ts_printf(__FILE__, __LINE__, "Error - data contents invalid (Serial %s)", tscb->bDriveSerialNumber);
          tseb->isFatalError = 1;
          return 13;
        }
        if (MATCH != ts_memcmp_exp(&tscb->bDriveSerialNumber[0], &tscb->bTestSerialNumber[0], sizeof(tscb->bDriveSerialNumber))) {
          ts_printf(__FILE__, __LINE__, "Error - s/n invalid");
          tseb->isFatalError = 1;
          return 14;
        }
        bStateNow = IDENTIFY_DRIVE_FSM_mfg_id;
      }
      ts_printf(__FILE__, __LINE__, "IDENTIFY(%s)", tscb->bDriveSerialNumber);
      break;

    case IDENTIFY_DRIVE_FSM_mfg_id: /* drive mfg id in identify */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_mfg_id****");
      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        bStateNow = IDENTIFY_DRIVE_FSM_srst_sequence;
      } else {
        bStateNow = IDENTIFY_DRIVE_FSM_mfg_id_on_label;
      }
      break;

    case IDENTIFY_DRIVE_FSM_srst_sequence: /* srst sequence (check if srst sequence valid) */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_srst_sequence****");
      rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_SRST_SEQUENCE, 0, READ_FROM_DRIVE);
      if (!rc) {

        if ((MATCH != memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SRST_RUNNING, strlen(SIGNATURE_SRST_RUNNING))) &&   /* SELF */
            (MATCH != memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SRST_ABORT, strlen(SIGNATURE_SRST_ABORT))) &&       /* ABOR */
            (MATCH != memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SRST_COMPLETE, strlen(SIGNATURE_SRST_COMPLETE))) && /* COMP */
            (MATCH != memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SRST_PENDING, strlen(SIGNATURE_SRST_PENDING)))  /* PEND */  ) {
          ts_printf(__FILE__, __LINE__, "Error - srst seq invalid - retry");
          break;
        }
        if (tscb->bPartFlag == SRST_PART) {  /* SRST part */
          if ((MATCH == memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SELFFIN1, strlen(SIGNATURE_SELFFIN1)) ||
               (MATCH == memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SELFFIN0, strlen(SIGNATURE_SELFFIN0))))) { /* finished SRST , need to jump at final part */
            ts_printf(__FILE__, __LINE__, "****SRST has finished already(%s),need to final part ****", tscb->bSectorBuffer);
            return FINAL_MODE;
          }
          if (MATCH == memcmp(&tscb->bSectorBuffer[0], SIGNATURE_ABORSRST, strlen(SIGNATURE_ABORSRST))) {  /* aborted SRST , need to ReSRST as of now ,this pattern is nothing. */
            ts_printf(__FILE__, __LINE__, "****SRST Part has aborted. ****");
            tseb->isFatalError = 1;
            return 15;
          }
          if (MATCH == memcmp(&tscb->bSectorBuffer[0], SIGNATURE_PENDSRST, strlen(SIGNATURE_PENDSRST))) {  /* aborted SRST , need to ReSRST as of now ,this pattern is nothing. */
            ts_printf(__FILE__, __LINE__, "****SRST Part has PENDed. ****");
            tseb->isFatalError = 1;
            return 16;
          }
        }
        if (tscb->bPartFlag == FINAL_PART) {  /* FINAL part */
          if (MATCH == memcmp(&tscb->bSectorBuffer[0], SIGNATURE_ABORFIN1, strlen(SIGNATURE_ABORFIN1))) { /* aborted FINAL , need finishing test */
            ts_printf(__FILE__, __LINE__, "****Final Part has aborted. ****");  /*[019KPD] */
            tseb->isFatalError = 1;
            return 17;
          }
          if (MATCH == memcmp(&tscb->bSectorBuffer[0], SIGNATURE_ABORFIN0, strlen(SIGNATURE_ABORFIN0))) { /* aborted FINAL , need to Refianl test */
            ts_printf(__FILE__, __LINE__, "****Final Part has aborted. ****");  /*[019KPD] */
            tseb->isFatalError = 1;
            return 18;
          }
          if (MATCH == memcmp(&tscb->bSectorBuffer[0], SIGNATURE_PENDFIN0, strlen(SIGNATURE_PENDFIN0))) { /* aborted FINAL , need to Refianl test */
            ts_printf(__FILE__, __LINE__, "****Final Part has pended. ****");  /*[019KPD] */
            tseb->isFatalError = 1;
            return 19;
          }
          if (MATCH == memcmp(&tscb->bSectorBuffer[0], SIGNATURE_PENDFIN1, strlen(SIGNATURE_PENDFIN1))) { /* aborted FINAL , need to Refianl test */
            ts_printf(__FILE__, __LINE__, "****Final Part has pended. ****");  /*[019KPD] */
            tseb->isFatalError = 1;
            return 20;
          }
          if (MATCH == memcmp(&tscb->bSectorBuffer[4], "SRST", 4)) {
            ts_printf(__FILE__, __LINE__, "**** still SRST Part ****");
            tseb->isFatalError = 1;
            return 21;
          }
        }

//        bStateNow = IDENTIFY_DRIVE_FSM_serial_number;
        bStateNow = IDENTIFY_DRIVE_FSM_data_from_Echo_after_sequence;
      }
      break;

    case IDENTIFY_DRIVE_FSM_data_from_Echo_after_sequence: /* srst sequence (check if srst sequence valid) */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_data_from_Echo_after_sequence****");
      rc = getEchoData(tscb);
      if (!rc) {
        bStateNow = IDENTIFY_DRIVE_FSM_mfg_id_on_label;
      }
      break;

    case IDENTIFY_DRIVE_FSM_mfg_id_on_label: /* mfg id on label */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_mfg_id_on_label****");
      if (tscb->isMultiGrading) { /* isMultiGrading is from config file. always 1 */
        rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_MFGID_ON_LABEL, 0, READ_FROM_DRIVE);
        if (!rc) {
          if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
            memmove(&tscb->bDriveMfgIdOnLabel[0], &tscb->bDriveMfgId[0], sizeof(tscb->bDriveMfgIdOnLabel));
            Dword dwOffset = tscb->bTestLabelMfgIdOffset;
            memmove(&tscb->bDriveMfgIdOnLabel[dwOffset], &tscb->bSectorBuffer[0], sizeof(tscb->bDriveMfgIdOnLabel) - dwOffset);
          } else {
            memmove(&tscb->bDriveMfgIdOnLabel[0], &tscb->bSectorBuffer[0], sizeof(tscb->bDriveMfgIdOnLabel));


          }
          if (!ts_isalnum_mem(&tscb->bDriveMfgIdOnLabel[0], sizeof(tscb->bDriveMfgIdOnLabel))) {
            ts_printf(__FILE__, __LINE__, "Error - data contents invalid (LabelID %s)", tscb->bDriveMfgIdOnLabel);
            tseb->isFatalError = 1;
            return 22;
          }
          if (MATCH != ts_memcmp_exp(&tscb->bDriveMfgIdOnLabel[0], &tscb->bTestMfgId[0], sizeof(tscb->bDriveMfgIdOnLabel))) {
            ts_printf(__FILE__, __LINE__, "Error - mfg-id invalid");
            tseb->isFatalError = 1;
            return 23;
          }
          memcpy(&tscb->bDrivePass2Id[0], &tscb->bDriveMfgId[0], sizeof(tscb->bDriveMfgId));     /* for function grading reportHost function */
          reportHostId(tscb);
          reportHostGradingId(tscb);
          bStateNow = IDENTIFY_DRIVE_FSM_status;
        }
      } else {
        ts_printf(__FILE__, __LINE__, "no Fuction grading");
        memmove(&tscb->bDriveMfgIdOnLabel[0], &tscb->bDriveMfgId[0], sizeof(tscb->bDriveMfgIdOnLabel));
        reportHostId(tscb);
        bStateNow = IDENTIFY_DRIVE_FSM_status;
      }

      break;

    case IDENTIFY_DRIVE_FSM_status: /* status */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_status****");
      rc = getDriveTemperature(tscb);
      if (!rc) {
        rc = getSRSTstep(tscb);
        if (!rc) {
          bStateNow = IDENTIFY_DRIVE_FSM_environment;
        }
      }
      break;

    case IDENTIFY_DRIVE_FSM_signature: /* srst signature */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_signature****");
      rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_SRST_SIGNATURE, 0, READ_FROM_DRIVE);
      if (!rc) {
        Dword i = 0;
        for (i = 0 ; i < (sizeof(tscb->bDriveSignature) / sizeof(tscb->bDriveSignature[0])) ; i++) {
          tscb->bDriveSignature[i] = tscb->bSectorBuffer[8 * i];
        }
        bStateNow = IDENTIFY_DRIVE_FSM_environment;
      }
      break;

    case IDENTIFY_DRIVE_FSM_environment: /* environment data */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_environment****");
      rc = getEnvironmentStatus(tscb);
      if (!rc) {
        bStateNow = IDENTIFY_DRIVE_FSM_report_to_host;
      }
      break;

    case IDENTIFY_DRIVE_FSM_report_to_host: /* report to host */
      ts_printf(__FILE__, __LINE__, "****IDENTIFY_DRIVE_FSM_report_to_host****");
      tscb->isDriveIdentifyComplete = 1;
      reportHostTestStatus(tscb);
      reportHostTestTimeOut(tscb);

      if (tscb->isDriveVoltageDataReport) {
        memset(&tscb->bSectorBuffer[0], 0x00, 24);
        tscb->bSectorBuffer[2] = 0xff & ((tscb->wCurrentV5InMilliVolts / 10) >> 8);
        tscb->bSectorBuffer[3] = 0xff & ((tscb->wCurrentV5InMilliVolts / 10) >> 0);
        tscb->bSectorBuffer[10] = 0xff & ((tscb->wCurrentV12InMilliVolts / 10) >> 8);
        tscb->bSectorBuffer[11] = 0xff & ((tscb->wCurrentV12InMilliVolts / 10) >> 0);
        reportHostTestData(tscb, KEY_TESTER_VOLTAGE_DATA, &tscb->bSectorBuffer[0], 24);
      } else {
        ts_printf(__FILE__, __LINE__, "no drive volt report");
      }

      /* 3Step is no grading *//* MNR,MRK,MRNT or EB7,JET,JGR or later */
      if (tscb->bSwtablePickupf == 0) {
        ts_sleep_partial(30);
        if (tscb->wDriveEchoVersion >= 3) { /* MNR,MRK,MRNT or EB7,JET,JGR or later */
          reportHostTestMessage(tscb, "getting switching table");
          getSWtable(tscb);
          reportHostTestMessage(tscb, "finished switching table");
          tscb->bSwtablePickupf = 1;
        }
      }
      /********************************************************************/
      return 0;
      break;

    default:
      ts_printf(__FILE__, __LINE__, "invalid state!!");
      tseb->isFatalError = 1;
      return 24;
      break;
    }
  }

  ts_printf(__FILE__, __LINE__, "shall not be here!!");
  tseb->isFatalError = 1;
  return 25;
}

/**
 * <pre>
 * Description:
 *   Identify drive from "ECHO" protocol in UART. It contains all of Identification Data.
 *   These Data access procedure came from Config file.
 *
 * Arguments:
 *   *tscb - Test script control block
 *
 * Querying Data in Echo Command
 *   Function is getSN_Echo() in comUart.c
 *     2:JPK,JPT      srstMfgID, srstSN, srstMicroLevel ,srstNumberOfServoSector, srstHeadTable, tpiinfo, srstSpindleSpeed
 *     4:EB7,JET,JGB  srstMfgID, srstSN, srstMicroLevel ,srstNumberOfServoSector, srstHeadTable, tpiinfo, srstSpindleSpeed,srst,addressSRSTTop
 *     4:MRK,MNR,MRS  srstMfgID, srstSN, srstMicroLevel ,srstNumberOfServoSector, srstHeadTable, tpiinfo, srstSpindleSpeed,srst,addressSRSTTop
 *
 * </pre>
 ***************************************************************************************
   2010/05/13 Support JPT or later Echo command procedure Y.Enomoto
 *****************************************************************************/

Byte getEchoData(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  DRIVE_MEMORY_CONTROL_BLOCK *dmcb;
  Byte rc = 1;
  Byte *bData_address;

  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_SERIAL_NUMBER, 0, READ_FROM_DRIVE); /* Read 512 bytes Echo Data */
  if (!rc) {
    for (dmcb = &tscb->driveMemoryControlBlock[0] ; dmcb < &tscb->driveMemoryControlBlock[MAX_SIZE_OF_DMCB] ; dmcb++) {
      if (dmcb->bAccessFlag == IS_IDENTIFY_ACCESS) {
        bData_address = queryDriveDataAddress(tscb, dmcb);
        if (bData_address == NULL) {
          continue;
        }
      } else {
        continue;
      }

      if (dmcb->bEndianFlag == IS_ENDIAN_8BIT) {
        memcpy(bData_address, &tscb->bSectorBuffer[dmcb->dwAddress], dmcb->wSize);
      } else if (dmcb->bEndianFlag == IS_ENDIAN_16BIT_LITTLE_TO_8BIT) {
        memcpyWith16bitLittleEndianTo8bitConversion(bData_address, &tscb->bSectorBuffer[dmcb->dwAddress], dmcb->wSize);
      } else if (dmcb->bEndianFlag == IS_ENDIAN_32BIT_LITTLE_TO_8BIT) {
        memcpyWith32bitLittleEndianTo8bitConversion(bData_address, &tscb->bSectorBuffer[dmcb->dwAddress], dmcb->wSize);
      } else if (dmcb->bEndianFlag == IS_ENDIAN_16BIT_LITTLE_TO_16BIT) {
        memcpyWith16bitLittleEndianTo16bitConversion((Word *)bData_address, &tscb->bSectorBuffer[dmcb->dwAddress], dmcb->wSize);
      } else if (dmcb->bEndianFlag == IS_ENDIAN_32BIT_LITTLE_TO_32BIT) {
        memcpyWith32bitLittleEndianTo32bitConversion((Dword *)bData_address, &tscb->bSectorBuffer[dmcb->dwAddress], dmcb->wSize);
      } else if (dmcb->bEndianFlag == IS_ENDIAN_16BIT_BIG_TO_16BIT) {
        memcpyWith16bitBigEndianTo16bitConversion((Word *)bData_address, &tscb->bSectorBuffer[dmcb->dwAddress], dmcb->wSize);
      } else {
        ts_printf(__FILE__, __LINE__, "error - unknown endian flag value %d", dmcb->bEndianFlag);
        tseb->isFatalError = 1;
        return 1;
      }
    }
  }
  return rc;
}

