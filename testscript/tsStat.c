#include "ts.h"

/**
 * <pre>
 * Description:
 *   Return non-zero value if drive rawdata ready.
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte isDriveRawdataReady(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  if (tscb->wDriveStatusAndControlFlag & IS_STOP_REQ_FROM_SRST) {
    return 1;
  }
  return (tscb->wDriveStatusAndControlFlag & IS_UNDER_REPORTING_MODE);
}

/**
 * <pre>
 * Description:
 *   Return non-zero value if drive pes ready.
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte isDrivePesReady(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  return (tscb->wDriveStatusAndControlFlag & IS_FFT_DATA_AVAILABLE);
}

/**
 * <pre>
 * Description:
 *   Return non-zero value if drive mcs ready.
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte isDriveMcsReady(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  return (tscb->wDriveStatusAndControlFlag & IS_MCS_DATA_AVAILABLE);
}

/**
 * <pre>
 * Description:
 *   Return non-zero value if drive rawdata setup error
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte isDriveRawdataSetupError(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  return (tscb->wDriveStatusAndControlFlag & IS_ERROR_AT_SETUP);
}

/**
 * <pre>
 * Description:
 *   Return non-zero value if drive rawdata setup ready
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte isDriveRawdataSetupReady(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  return (tscb->wDriveStatusAndControlFlag & IS_RAWDATA_READY);
}


/**
 * <pre>
 * Description:
 *   Set rawdata transfer complete flag. It is usually called after every
 *   rawdata buffer dump.
 *   Return zero if success. Otherwise, non-zero value.
 * </pre>
 *****************************************************************************/
Byte setDriveRawdataTransferComplete(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Word wtmp;
  Byte btmp;
  Byte rc;

  if (tscb->wDriveEchoVersion >= 2) { /* JPT or later Word flag */

    wtmp = tscb->wDriveStatusAndControlFlag & (IS_RAWDATA_TRANSFER_COMP & IS_CLEAR_REPORTING_TIMER);
    memmove(&tscb->bSectorBuffer[0], (Byte *)&wtmp, sizeof(wtmp));
  } else { /* Legacy Product PTB,PTC Byte flag */
    btmp = (Byte)(tscb->wDriveStatusAndControlFlag & IS_RAWDATA_TRANSFER_COMP);
    memmove(&tscb->bSectorBuffer[0], (Byte *)&btmp, sizeof(btmp));
  }
  rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_STATUS_AND_CONTROL, 0, WRITE_TO_DRIVE);
  return rc;
}

/**
 * <pre>
 * Description:
 *   Set RAWDATA Retry Request to SRST
 *   Return zero if success. Otherwise, non-zero value.
 * </pre>
 *****************************************************************************/
Byte setDriveRawdataRetryRequest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Word wtmp;
  Byte btmp;
  Byte rc;

  if (tscb->wDriveEchoVersion >= 2) { /* JPT or later Word flag */
    wtmp = tscb->wDriveStatusAndControlFlag | IS_RETRY_REQUEST;
    memmove(&tscb->bSectorBuffer[0], (Byte *)&wtmp, sizeof(wtmp));
  } else { /* Legacy Product PTB,PTC Byte flag */
    btmp = (Byte)(tscb->wDriveStatusAndControlFlag | IS_RETRY_REQUEST);
    memmove(&tscb->bSectorBuffer[0], (Byte *)&btmp, sizeof(btmp));
  }
  rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_STATUS_AND_CONTROL, 0, WRITE_TO_DRIVE);
  return rc;
}

/**
 * <pre>
 * Description:
 *   Clear RAWDATA Retry Request to SRST
 *   Return zero if success. Otherwise, non-zero value.
 * </pre>
 *****************************************************************************/
Byte clearDriveRawdataRetryRequest(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Word wtmp;
  Byte btmp;
  Byte rc;

  if (tscb->wDriveEchoVersion >= 2) { /* JPT or later Word flag */
    wtmp = tscb->wDriveStatusAndControlFlag & IS_CLEAR_RETRY_REQUEST;
    memmove(&tscb->bSectorBuffer[0], (Byte *)&wtmp, sizeof(wtmp));
  } else { /* Legacy Product PTB,PTC Byte flag */
    btmp = (Byte)(tscb->wDriveStatusAndControlFlag & IS_CLEAR_RETRY_REQUEST);
    memmove(&tscb->bSectorBuffer[0], (Byte *)&btmp, sizeof(btmp));
  }
  rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_STATUS_AND_CONTROL, 0, WRITE_TO_DRIVE);
  return rc;
}

/**
 * <pre>
 * Description:
 *   Get drive rawdata status and control flag.
 *   Return zero if success. Otherwise, non-zero value.
 *   Validate reading value != 0xff that is unexpected status.
 * </pre>
 *****************************************************************************/
Byte getDriveRawdataStatusAndControl(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Word wtmp;
  Byte btmp;
  Byte rc;

  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_STATUS_AND_CONTROL, 0, READ_FROM_DRIVE);
  if (!rc) {
    if (tscb->wDriveEchoVersion >= 2) { /* JPT or later Word flag */
      memmove((Byte *)&wtmp, &tscb->bSectorBuffer[0], sizeof(wtmp));
      if (wtmp != 0xffff) {
        tscb->wDriveStatusAndControlFlag = wtmp;
        if (tscb->wDriveStatusAndControlFlag & IS_STOP_REQ_FROM_SRST) {
          tscb->bDriveSkip_SRST_result = 1;
        }
      } else {
        rc = 1;
      }
    } else { /* Legacy Product PTB,PTC Byte flag */
      memmove((Byte *)&btmp, &tscb->bSectorBuffer[0], sizeof(btmp));
      if (btmp != 0xff) {
        tscb->wDriveStatusAndControlFlag = (Word)btmp;
      } else {
        rc = 1;
      }
    }
    if (rc == 1) {
      ts_printf(__FILE__, __LINE__, "stat_ctrl validation fail");
      btmp = driveIoCtrlByKeywordWithRetry(tscb, KEY_SRST_SEQUENCE, 0, READ_FROM_DRIVE);
    }
  }
  return rc;
}

/**
 * <pre>
 * Description:
 *   clear reporting timer on srst
 *   rawdata buffer dump.
 *   Return zero if success. Otherwise, non-zero value.
 * </pre>
 *****************************************************************************/
Byte clearReportingTimerOnSsrst(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte btmp;
  Byte rc = 0;

  if (tscb->wDriveEchoVersion >= 2) { /* JPT or later Word flag */
    rc = getDriveRawdataStatusAndControl(tscb);
    if (!rc) {
      btmp = (Byte)(tscb->wDriveStatusAndControlFlag >> 8);
      ts_printf(__FILE__, __LINE__, "tscb->wDriveStatusAndControlFlag %04x btmp %04x", tscb->wDriveStatusAndControlFlag, btmp);
      btmp = btmp & ~CLEAR_TIMER_BYTE;    /* and by 0x80 */
      ts_printf(__FILE__, __LINE__, "tscb->wDriveStatusAndControlFlag %04x btmp %04x", tscb->wDriveStatusAndControlFlag, btmp);
      memmove(&tscb->bSectorBuffer[0], (Byte *)&btmp, sizeof(btmp));
      ts_printf(__FILE__, __LINE__, "bSectorBuffer[0][1] %02x%02x", tscb->bSectorBuffer[0], tscb->bSectorBuffer[1]);
      rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_STATUS_AND_CONTROL_B, 10, WRITE_TO_DRIVE); /* write only 1byte -> Endian Flag is 6 at configfile */
    }
    if (rc) {
      ts_printf(__FILE__, __LINE__, "Error - read memory(clearReportingTimer)");
    }
  }

  return rc;
}

/**
 * <pre>
 * Description:
 *   Get Drive Temperature.
 *   Return zero if success. Otherwise, non-zero value.
 *   Validate reading value != 0xff that is unexpected status.
 * </pre>
 *****************************************************************************/
Byte getDriveTemperature(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc;

  /* Temperature */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_TEMPERATURE, 0, READ_FROM_DRIVE);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Error - read Temperature");
    return rc;
  }
  tscb->bDriveTemperature = tscb->bSectorBuffer[0];
  if (tscb->bDriveTemperatureMax < tscb->bDriveTemperature) {
    tscb->bDriveTemperatureMax = tscb->bDriveTemperature;
  }
  if (tscb->bDriveTemperatureMin == 0) {
    tscb->bDriveTemperatureMin = tscb->bDriveTemperature;
  } else if (tscb->bDriveTemperature < tscb->bDriveTemperatureMin) {
    tscb->bDriveTemperatureMin = tscb->bDriveTemperature;
  }
  rc = adjustTemperatureControlByDriveTemperature(MY_CELL_NO + 1, (tscb->bDriveTemperature) * 100);  // 20110511 Htmp adjust control
  return rc;
}

/**
 * <pre>
 * Description:
 *   Get SRST step.
 *   Return zero if success. Otherwise, non-zero value.
 *   Validate reading value != 0xff that is unexpected status.
 * </pre>
 *****************************************************************************/
Byte getSRSTstep(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc;

  /* SRST Step */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_STEP_COUNT, 0, READ_FROM_DRIVE);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Error - read SRST step");
  } else {
    tscb->bDriveStepCount = tscb->bSectorBuffer[0];
  }
  return rc;
}
/**
 * <pre>
 * Description:
 *   Get Grading Flag.
 *   Return zero if success. Otherwise, non-zero value.
 *   Validate reading value != 0xff that is unexpected status.
 * </pre>
 *****************************************************************************/
Byte getGradingflag(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Dword dwtmp = 0;
  Byte rc, bsize;
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;

  bsize = 2;
  if (tscb->wDriveEchoVersion >= 4) { /* Water Fall Grading */
    bsize = 4;
  }

  if (tscb->wDriveEchoVersion <= 3) {
    rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_GRADING_FLAG, 0, READ_FROM_DRIVE);
  } else {
    rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_GRADING_FLAG_WF, 0, READ_FROM_DRIVE);  /* water fall grading EB7 */
  }

  if (rc) {
    ts_printf(__FILE__, __LINE__, "Error - read Grading Flag");
    tseb->isGradingError = NO_SW_TBL;  /*ERROR_CODE_grading_noswtb*/
  } else {
    memmove((Byte *)&dwtmp, &tscb->bSectorBuffer[0], bsize);
  }
  if (tscb->wDriveEchoVersion <= 3) {
    if (((dwtmp != 0xffff) && (bsize == 2)) || (dwtmp != 0xffffffff)) {
      tscb->dwDriveGradingFlag = dwtmp;
      ts_printf(__FILE__, __LINE__, "Grading flag %08x", tscb->dwDriveGradingFlag);
    } else {
      rc = NO_SW_TBL;
      tseb->isGradingError = NO_SW_TBL;  /*ERROR_CODE_grading_noswtb*/
      ts_printf(__FILE__, __LINE__, "Grading flag getting error");
    }
  }
  if (tscb->wDriveEchoVersion >= 4) {
    if (((dwtmp != 0xffff) && (bsize == 4)) || (dwtmp != 0xffffffff)) {
      tscb->dwDriveGradingWaterFallFlag = dwtmp;
      ts_printf(__FILE__, __LINE__, "Water Fall Grading flag %08x", tscb->dwDriveGradingWaterFallFlag);
    } else {
      rc = NO_SW_TBL;
      tseb->isGradingError = NO_SW_TBL;  /*ERROR_CODE_grading_noswtb*/
      ts_printf(__FILE__, __LINE__, "Water Fall Grading flag getting error");
    }
  }
  return rc;
}
/**
 * <pre>
 * Description:
 *   Get Grading Flag.
 *   Return zero if success. Otherwise, non-zero value.
 *   Validate reading value != 0xff that is unexpected status.
 * </pre>
 *****************************************************************************/
Byte getHead(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte wtmp = 0;
  Byte rc, bsize;
  /* TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock; */

  bsize = 2;
  if (tscb->wDriveEchoVersion >= 4) { /* Water Fall Grading */
    bsize = 4;
  }
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_OFFSET_HEAD, 0, READ_FROM_DRIVE);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Error - read offset head Flag");
    /* tseb->isGradingError = NO_SW_TBL;  *//*ERROR_CODE_grading_noswtb*/
  } else {
    memmove((Byte *)&wtmp, &tscb->bSectorBuffer[0], bsize);
    if (((wtmp != 0xff) && (bsize == 2)) || (wtmp != 0xff)) {
      tscb->bOffsetHead = wtmp;
    } else {
      /*rc = NO_SW_TBL;*/
      /* tseb->isGradingError = 0; */   /*ERROR_CODE_grading_noswtb*/
    }
  }
  return tscb->bOffsetHead;
}

/**
 * <pre>
 * Description:
*   Write Pass2ID to Drive (MN1A0W -> Write lower 4 bytes only "1A0W")
 *   Return zero if success. Otherwise, non-zero value.
 * </pre>
 *****************************************************************************/
Byte setDrivePass2ID(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc;

  memcpy(&tscb->bSectorBuffer[0], (Byte *)&tscb->bDrivePass2Id[2], 4);
  rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_PASS2ID, 0, WRITE_TO_DRIVE);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Pass2ID writing Error");
    /* tseb->isAbortByUser = ERROR_CODE_cell_writefail_PASS2ID;*/
  }
  return rc;
}
/**
 * <pre>
 * Description:
*   Write Pass2ID to Drive (MN1A0W -> Write lower 4 bytes only "1A0W")
 *   Return zero if success. Otherwise, non-zero value.
 * </pre>
 *****************************************************************************/
Byte clearGradingFlag(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc;

  tscb->wDriveStatusAndControlFlag &= IS_GRADING_MODEL_CLEAR;
  memmove(&tscb->bSectorBuffer[0], (Byte *)&tscb->wDriveStatusAndControlFlag, 2);
  rc =  driveIoCtrlByKeywordWithRetry(tscb, KEY_STATUS_AND_CONTROL, 0, WRITE_TO_DRIVE);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Grading Flag Clear writing Error");
    /* tseb->isAbortByUser = ERROR_CODE_cell_writefail_PASS2ID;*/
  }
  return rc;
}
/***********************************************************************
SWTABLE Example
        InputID,SW_bit,bit0ID,bit1ID,bit2ID,bit3ID,....
        6800,GE1830,0A,XXXXXX,GE1830,XXXXXX,GE1810,XXXXXX,XXXXXX,XXXXXX,XXXXXX
           Switching Table for Graging
   Byte bSwitchTable[75];
   Word wGradingFlag;
   return 0;OK

************************************************************************/
Byte grading(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{

  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte i;
  Byte tempID[8][6];
  Byte OriginID[6];
  Byte tempflag[2];
  /*Byte flag;*/
  Byte find_f = 0;
  Byte rc;
  Byte *pass2ID_ptr;
  Dword dwSwitching_bit;
  Dword dwAddress;
  Dword dwselectIDbit;  /* for serching bit */

  /*Byte buf[100] = "6800,E3J100,A0,XXXXXX,E3J100,XXXXXX,E3AAAA,XXXXXX,XXXXXX,XXXXXX,XXXXXX";
  memcpy(&tscb->bSwitchTable,buf,72);
  */

  if (tscb->wDriveEchoVersion <= 3) { /* MRN,MRK,MRS Grading */
    for (i = 0;i < 8;i++) {
      memcpy(tempID[i], &tscb->bSwitchTable[15+i*7], 6);
    }
    memcpy(tempflag, &tscb->bSwitchTable[12], 2);
    memcpy(OriginID, &tscb->bSwitchTable[5], 6);
    for (i = 2;i < 8;i++) {
      if (!strncmp(tempID[i], "XXXXXX", 6)) {
        continue;
      } else {
        memcpy(&tscb->bDrivePass2Id, &tempID[i][0], 6);
        ts_printf(__FILE__, __LINE__, "Selected Grading ID is %s(1)", &tempID[i][0]);
        find_f = 1;
        break;
      }
    }
    if (!find_f) {
      tseb->isGradingError = NO_PASS2ID;  /*ERROR_CODE_grading_nopass2ID*/
      ts_printf(__FILE__, __LINE__, "Grading ID is nothing");
      memcpy(&tscb->bDrivePass2Id[0], &tscb->bDriveMfgId[0], 6); /* better to write MFGID */
    }
  }
  /********************************************************
  000 53454C4653525354   20D001562E005600   010 0000000000000000   1F00000000000000
  020 0100000018020400   1802020001000000   030 73001A00FC011400   61001300A56F1A00
  040 00008BB9CB000600   0000000100000100   050 AC5B2600A4CFA105   0000000000003132
  060 3231030013028000   0100000000000000   070 0000000000005C01   0018000000000000 <-- grading request flag
  080 000001008000003A   01F003000000F184   090 E2E8000000000000   201C14491E2DC500
  0A0 A55B26000E0E6406   0C003F00963B0B40   0B0 6BD4020000000300   00000C0000000001
  0C0 FF01010000000000   0000000000000000   0D0 0000000000000000   0000000000000000
  0E0 0000000000000000   0000000000000000   0F0 1414141414141414   40003F0000007E00 <-- grading switch bit(4bytes)
                                 A0000030   <-Input like this,actual table is 3000000A
  *********************************************************/
  else { /* Water Fall Grading , Switch bit is 4byte(MAX) */

    memmove(&tscb->bSectorBuffer[0], (Byte *)&tscb->bSwitchTable[0], 256);
    reportHostTestData(tscb, KEY_SWITCH_TABLE, &tscb->bSectorBuffer[0], 256);
    dwSwitching_bit = cutout_sw_bit((Byte *) & tscb->bSwitchTable[0], (unsigned long *) & dwAddress);

    ts_printf(__FILE__, __LINE__, "sw_bit:%x ", dwSwitching_bit);
    if (tscb->dwDriveGradingWaterFallFlag != 0) {
      if (dwSwitching_bit == 0) { /* requested grading,but no sw bit */
        tseb->isGradingError = NO_PASS2ID;  /*ERROR_CODE_grading_nopass2ID*/
        /*memcpy(&tscb->bDrivePass2Id,&tscb->bDriveMfgId, 6);*/
      } else {
        pass2ID_ptr = (Byte *)dwAddress;
        dwselectIDbit = 0x01;
        find_f = 0;
        ts_printf(__FILE__, __LINE__, "WATER FALL GRADING SwitchingBit:%02x   WaterFallFlag:%08x", dwSwitching_bit, tscb->dwDriveGradingWaterFallFlag);
        for (i = 0;i < 32;i++) {
          if (dwselectIDbit & dwSwitching_bit & tscb->dwDriveGradingWaterFallFlag) {
            find_f = 1;
            break;
          }
          dwselectIDbit <<= 1;
          pass2ID_ptr += next_position((unsigned char *)pass2ID_ptr, ',');
        }
        if (!find_f) {
          tseb->isGradingError = NO_PASS2ID;  /*ERROR_CODE_grading_nopass2ID*/
          ts_printf(__FILE__, __LINE__, "cannot find Pass2ID from grading table");
          return NO_PASS2ID;
        }
        memcpy(&tscb->bDrivePass2Id[0], (unsigned char *)pass2ID_ptr, 6);
        if (!strncmp(&tscb->bDrivePass2Id[0], "XXXXXX", 6)) {
          ts_printf(__FILE__, __LINE__, "Pass2ID is invalid %s", tscb->bDrivePass2Id);
          memcpy(&tscb->bDrivePass2Id[0], &tscb->bDriveMfgId[0], 6); /* [019KG] bug fix   better to write MFGID */
          return NO_PASS2ID;
        }
        rc = checkAlpha(&tscb->bDrivePass2Id[0], 6); /* check A-Z or 0-9 or $ */
        if (rc) {
          ts_printf(__FILE__, __LINE__, "i=%d Pass2[%d]=%c", rc, rc, tscb->bDrivePass2Id[rc]);
          ts_printf(__FILE__, __LINE__, "Pass2ID has invalid charactar %s", tscb->bDrivePass2Id);
          memcpy(&tscb->bDrivePass2Id[0], &tscb->bDriveMfgId[0], 6); /* [019KG] bug fix   better to write MFGID */
          return NO_PASS2ID;
        }
        ts_printf(__FILE__, __LINE__, "Selected Grading ID is %s(2)", tscb->bDrivePass2Id);
      }
    }
  }


  memmove(&tscb->bSectorBuffer[0], (Byte *)&tscb->bDrivePass2Id[0], 6);
  memmove(&tscb->bSectorBuffer[6], (Byte *)&tscb->dwDriveGradingWaterFallFlag, 4);
  memmove(&tscb->bSectorBuffer[10], (Byte *)&dwSwitching_bit, 4);
  memmove(&tscb->bSectorBuffer[14], (Byte *)&dwselectIDbit, 4);
  memmove(&tscb->bSectorBuffer[18], (Byte *)&tscb->bDriveMfgIdOnLabel[0], 6);
  memmove(&tscb->bSectorBuffer[24], (Byte *)&tscb->bDrivePass2Id[0], 6);
  memmove(&tscb->bSectorBuffer[30], (Byte *)&find_f, 1);

  reportHostTestData(tscb, KEY_GRADING_REPORT, &tscb->bSectorBuffer[0], 31);


  rc = setDrivePass2ID(tscb);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Grading Pass2ID write error");
    tseb->isGradingError = SET_PASS2ID_ERROR;    /* ERROR_CODE_grading_wrpass2ID*/
    return SET_PASS2ID_ERROR;
  } else {
    rc = clearGradingFlag(tscb);    /*     */
  }

  return rc;
}

/**
 * <pre>
 * Description:
 *   Cutout switching bit from Switching Table
 *   Return switching bit
 * </pre>
 *****************************************************************************/
Dword cutout_sw_bit(Byte *bStart_pointer, Dword *dwRet_address)
{
  Dword i, ret, count;
  Dword size;

  ret = count = 0;
  for (i = 0;i < 2;i++) {
    while (*(bStart_pointer++) != ',') {
      if (count++ > 255)
        break;
    }
    if (count > 255)
      break;
  }
  if (count <= 255) {
    size = 0;
    while (*(bStart_pointer + size) != ',') {
      size++;
    }
    *dwRet_address = (Dword)bStart_pointer + size + 1;
    ret = calcAtoUL( (Byte *)bStart_pointer, size );
  }

  return ret;
}

/**
 * <pre>
 * Description:
 *   Found next Delimiter position
 *   Return next position offset
 * </pre>
 *****************************************************************************/
Dword next_position(Byte *bStart_point, Byte bDelimiter)
{
  Dword count;

  count = 0;
  while (*bStart_point++ != bDelimiter) {
    if (count++ > 255)
      break;
  }

  return count + 1;
}

/***************************************************************************************************************
 prepare buffer 2007/08/29 Y.Enomoto
 int mode -> 0:1st Message and data
   char *msgptr -> 2st message address
   char *dataptr -> data address
   long len -> data length

 int mode -> 1:additional data
   char *msgptr -> buffer address
   char *dataptr -> data address
   long len -> data length

 return next buffer pointer

******************************************************************************************************************/
unsigned char *prepare_buf(char *msgptr, unsigned char *dataptr, Byte len)
{
  unsigned char  *buf_adr;


  buf_adr = (unsigned char *)msgptr;
  memcpy(buf_adr, dataptr, len);
  return (buf_adr + len);
}
