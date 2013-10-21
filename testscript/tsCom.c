#include "ts.h"


/**
 * <pre>
 * Description:
 *   Parses given script file and set it into test script control block.
 *   Return value is 0 if no error. Otherwise, non-zero value.
 * Arguments:
 *   dwAddress - read/write drive memory address
 *   wSize - read/write drive memory size
 *   *bData - pointer to read/write drive memory data
 *   bProtocol - flag to read/write/identify
 * </pre>
 *****************************************************************************/
Byte driveIoCtrl(Dword dwAddress, Word wSize, Byte *bData, Byte bProtocol, UART_PERFORMANCE_LOG_BLOCK *uplb)
{
  Byte rc = 1;
  Byte *msg[] = {"Read", "Write", "Identify"};
  Byte i;
  Byte bSizeloop;
  Word wSizebuf;
  Dword dwRemainReadbyte;
  Dword dwlimitedReadbyte;
  Byte bReadErCount = 0;

  /* range check */
  if ((wSize > MAX_UART_DATA_SIZE_BYTE) || ((bProtocol - READ_DRIVE_MEMORY_PROTOCOL) > IS_IDENTIFY_ACCESS)) {
    ts_printf(__FILE__, __LINE__, "driveIoCtrl range check failure %08xh %d %d", dwAddress, wSize, bProtocol);
    return 1;
  }

  ts_printf(__FILE__, __LINE__, "driveIoCtrl %08xh %d %s", dwAddress, wSize, msg[bProtocol-READ_DRIVE_MEMORY_PROTOCOL]);

  /* uart performance log */
  uplb[bProtocol-READ_DRIVE_MEMORY_PROTOCOL].dwNumOfCommand++;

  /* ---------------------------------------------------------------------- */
#if defined(LINUX)
  /* ---------------------------------------------------------------------- */

  /* Access Drive */
  if (bProtocol == READ_DRIVE_MEMORY_PROTOCOL) {
    /*if(bTesterflag == OPTIMUS){*/
    if (wSize > wReadBlocksize) {
      ts_printf(__FILE__, __LINE__, "Read Size = %d BlockSize %d", wSize, wReadBlocksize);
      wSizebuf = wSize;
      wSize = wReadBlocksize;
      bSizeloop = wSizebuf / (Dword)wSize;
      if (wSizebuf % (Dword)wSize) bSizeloop++;
      for (i = 0;i < bSizeloop;i++) {
        rc = siReadDriveMemory(MY_CELL_NO + 1, dwAddress, wSize, bData, WAIT_TIME_SEC_FOR_DRIVE_RESPONSE * 1000);
        /*dwAddress += dwlimitedReadbyte;*/
        if (!rc) {
          dwAddress += wReadBlocksize;
          dwRemainReadbyte -= dwlimitedReadbyte;
          bData += wReadBlocksize;
        } else { /* read error */
          i--;
          bReadErCount++;
          ts_printf(__FILE__, __LINE__, "drive I/O error loop = %d", i);
          if (bReadErCount > 20) {
            return rc;
          }
          ts_sleep_partial(DRIVE_ACCESS_RETRY_INTERVAL_SEC);
          continue;
        }
      }
    } else {
      rc = siReadDriveMemory(MY_CELL_NO + 1, dwAddress, wSize, bData, WAIT_TIME_SEC_FOR_DRIVE_RESPONSE * 1000);
    }
    /*}else{
         rc = siReadDriveMemory(MY_CELL_NO + 1, dwAddress, wSize, bData, WAIT_TIME_SEC_FOR_DRIVE_RESPONSE * 1000);
    }*/
  } else if (bProtocol == WRITE_DRIVE_MEMORY_PROTOCOL) {
    rc = siWriteDriveMemory(MY_CELL_NO + 1, dwAddress, wSize, bData, WAIT_TIME_SEC_FOR_DRIVE_RESPONSE * 1000);
  } else if (bProtocol == IDENTIFY_DRIVE_PROTOCOL) {
    rc = siEchoDrive(MY_CELL_NO + 1, wSize, bData, WAIT_TIME_SEC_FOR_DRIVE_RESPONSE * 1000);
  } else {
    return 1;
  }

  /* ---------------------------------------------------------------------- */
#if defined(UNIT_TEST)
  /* ---------------------------------------------------------------------- */
  HQUE *que = NULL;

  que = (HQUE *)ts_qget();
  /* Copy Data */
  if (!rc) {
    if ((READ_DRIVE_MEMORY_PROTOCOL == bProtocol) || (IDENTIFY_DRIVE_PROTOCOL == bProtocol)) {
      memmove(&bData[0], &que->bData[0], wSize);
    }
  }

  /* Free memory */
  if (que) {
    ts_qfree((Byte *)que);
  }


  /* ---------------------------------------------------------------------- */
#endif   // end of UNIT_TEST
  /* ---------------------------------------------------------------------- */

  /* ---------------------------------------------------------------------- */
#else    // else of LINUX
  /* ---------------------------------------------------------------------- */

  HQUE *que = NULL;
  Dword i = 0;
  Word wHeaderSize = 0;

  /* Clear existing que & Put queue */
  /* ---------------------------------------------------------------------- */
#ifndef UNIT_TEST
  /* ---------------------------------------------------------------------- */
  if (ts_qis()) {
    que = (HQUE *)ts_qget();
    ts_qfree_then_abort(que);
    return 1;
  }
  /* ---------------------------------------------------------------------- */
#endif   // end of UNIT_TEST
  /* ---------------------------------------------------------------------- */

  /* Alloc memory */
  if (bProtocol == WRITE_DRIVE_MEMORY_PROTOCOL) {
    que = ts_qalloc(sizeof(HQUE) + wSize);
  } else {
    que = ts_qalloc(sizeof(HQUE));
  }

  /* Set parameters into queue */
  wHeaderSize = sizeof(que->dwAddress) + sizeof(que->bAck) + sizeof(que->wSync) + sizeof(que->wReturnCode) + sizeof(que->wLength);
  que->wDID = getCellComModuleTaskNo(MY_CELL_NO + 1);
  que->bAdrs = MY_CELL_NO + 1;
  if (bProtocol == WRITE_DRIVE_MEMORY_PROTOCOL) {
    que->wLgth = wHeaderSize + wSize;
    memmove(&que->bData[0], &bData[0], wSize);
  } else {
    que->wLgth = wHeaderSize;
  }
  que->wLength = wSize;
  que->dwAddress = dwAddress;
  que->bType = bProtocol;

  ts_qput((QUE*)que);
  que = NULL;

  /* Get response */
  for (i = 0 ; i < WAIT_TIME_SEC_FOR_DRIVE_RESPONSE ; i += WAIT_TIME_SEC_FOR_DRIVE_RESPONSE_POLL) {
    if (que) {
      ts_qfree((Byte *)que);
      que = NULL;
    }
    if (ts_qis()) {
      que = (HQUE *)ts_qget();
      if ((que->wLength != wSize) ||
          (que->dwAddress != dwAddress) ||
          (que->bType != bProtocol)) {
        ts_qfree_then_abort(que);
        return 1;
      } else {
        if (que->wReturnCode) {
          ts_printf(__FILE__, __LINE__, "Error - drive ioctrl rc=%d", que->wReturnCode);
          rc = 1;
        } else {
          rc = 0;
        }
      }
      break;
    }
    ts_sleep_partial(WAIT_TIME_SEC_FOR_DRIVE_RESPONSE_POLL);
  }

  /* Timeout */
  if (i >= WAIT_TIME_SEC_FOR_DRIVE_RESPONSE) {
    ts_printf(__FILE__, __LINE__, "fatal error - response que timeout");
    ts_abort();
    return 1;
  }

  /* Copy Data */
  if (!rc) {
    if ((READ_DRIVE_MEMORY_PROTOCOL == bProtocol) || (IDENTIFY_DRIVE_PROTOCOL == bProtocol)) {
      memmove(&bData[0], &que->bData[0], wSize);
    }
  }

  /* Free memory */
  if (que) {
    ts_qfree((Byte *)que);
  }

  /* ---------------------------------------------------------------------- */
#endif    // end of LINUX
  /* ---------------------------------------------------------------------- */

  /* uart performance log */
  if (!rc) {
    uplb[bProtocol-READ_DRIVE_MEMORY_PROTOCOL].dwNumOfCommandSuccess++;
    uplb[bProtocol-READ_DRIVE_MEMORY_PROTOCOL].dwTotalDataSize += wSize;
  }

  return rc;
}

/**
 * <pre>
 * Description:
 *   Endian Swap by dmcb->bEndianFlag .
 * Arguments:
 *   *tscb - Test script control block
 *   *dmcb - Test script dive information block
 * </pre>
 *****************************************************************************/
Byte EndianSwaponSectorBuffer(TEST_SCRIPT_CONTROL_BLOCK *tscb,  DRIVE_MEMORY_CONTROL_BLOCK *dmcb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 0;


  if (dmcb->bEndianFlag == IS_ENDIAN_8BIT) {
    ;
  } else if (dmcb->bEndianFlag == IS_ENDIAN_16BIT_LITTLE_TO_8BIT) {
    memcpyWith16bitLittleEndianTo8bitConversion(&tscb->bSectorBuffer[0], &tscb->bSectorBuffer[0], dmcb->wSize);
  } else if (dmcb->bEndianFlag == IS_ENDIAN_32BIT_LITTLE_TO_8BIT) {
    memcpyWith32bitLittleEndianTo8bitConversion(&tscb->bSectorBuffer[0], &tscb->bSectorBuffer[0], dmcb->wSize);
  } else if (dmcb->bEndianFlag == IS_ENDIAN_16BIT_LITTLE_TO_16BIT) {
    memcpyWith16bitLittleEndianTo16bitConversion((Word *)&tscb->bSectorBuffer[0], &tscb->bSectorBuffer[0], dmcb->wSize);
  } else if (dmcb->bEndianFlag == IS_ENDIAN_32BIT_LITTLE_TO_32BIT) {
    memcpyWith32bitLittleEndianTo32bitConversion((Dword *)&tscb->bSectorBuffer[0], &tscb->bSectorBuffer[0], dmcb->wSize);
  } else {
    ts_printf(__FILE__, __LINE__, "error - unknown endian flag value %d", dmcb->bEndianFlag);
    tseb->isFatalError = 1;
    rc = 1;
  }
  return rc;
}

/**
 * <pre>
 * Description:
 *   Access drive memory by given keyword with retry in given timeout in sec.
 * Arguments:
 *   *tscb - Test script control block
 *   *bKeyword - The name say it all.
 *    dwTimeoutSec - The name say it all.
 *    isRead - 1:Read from Drive 0:Write to Drive.
 * </pre>
 *****************************************************************************
 2010/05/27 support IS_DIRECT_ACCESS_WITH_OFFSET(from SRST top address) and IS_INDIRECT_ACCESS_WITH_OFFSET(from SRST top address)
 *****************************************************************************/
Byte driveIoCtrlByKeywordWithRetry(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bKeyword, Dword dwTimeoutSec, Byte isRead)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  DRIVE_MEMORY_CONTROL_BLOCK *dmcb = NULL;
  UART_PERFORMANCE_LOG_BLOCK *uplb = &tscb->uartPerformanceLogBlock[0];
  Dword dwAddress = 0;
  Dword dwTempAddr = 0;
  Dword dwAddressOffset = 0;
  Word wSize = 0;
  Byte bProtocol = READ_DRIVE_MEMORY_PROTOCOL;
  Byte rc = 1;
  Byte bSaveData[4]; /* 4byte data save area for INDIRECT mode write */
  Byte i;
  Byte bTemp[20];

  ts_printf(__FILE__, __LINE__, "driveIoCtrlByKeywordWithRetry %s %d %d", &bKeyword[0], dwTimeoutSec, isRead);
  if (!isRead) {
    putBinaryDataDump(&tscb->bSectorBuffer[0], 0, 1);
  }
  dwTimeoutSec += getTestScriptTimeInSec();

  /* Get Drive Memory Control Block */
  rc = queryDriveMemoryControlBlock(tscb, bKeyword, &dmcb);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Error - query dmcb");
    tseb->isFatalError = 1;
    return rc;
  }

  /* Set Size, Address, Protocol */
  if ((dmcb->bAccessFlag == IS_DIRECT_ACCESS) || (dmcb->bAccessFlag == IS_DIRECT_ACCESS_WITH_OFFSET)) {
    if (dmcb->bAccessFlag == IS_DIRECT_ACCESS_WITH_OFFSET) {
      dwAddress = tscb->dwDriveSrstTopAddress;
      dmcb->bAccessFlag = IS_DIRECT_ACCESS;
      dwAddressOffset = dmcb->dwAddressOffset;
    } else {
      dwAddress = dmcb->dwAddress;
    }
    wSize = dmcb->wSize;
    if (isRead) {
      bProtocol = READ_DRIVE_MEMORY_PROTOCOL;
    } else {
      bProtocol = WRITE_DRIVE_MEMORY_PROTOCOL;
    }

  } else if ((dmcb->bAccessFlag == IS_INDIRECT_ACCESS) || (dmcb->bAccessFlag == IS_INDIRECT_ACCESS_WITH_OFFSET)) {
    if (dmcb->bAccessFlag == IS_INDIRECT_ACCESS_WITH_OFFSET) {
      dwAddress = tscb->dwDriveSrstTopAddress + dmcb->dwAddressOffset;
    } else {
      dwAddress = dmcb->dwAddress;
      dwAddressOffset = dmcb->dwAddressOffset;
    }
    wSize = 4;
    bProtocol = READ_DRIVE_MEMORY_PROTOCOL;

  } else if (dmcb->bAccessFlag == IS_IDENTIFY_ACCESS) {
    dwAddress = 0;
//    wSize = dmcb->dwAddress + dmcb->wSize;
    wSize = DRIVE_ECHO_DATA_SIZE;
    bProtocol = IDENTIFY_DRIVE_PROTOCOL;

  } else {
    ts_printf(__FILE__, __LINE__, "Error - Unknown access flag value %d", dmcb->bAccessFlag);
    tseb->isFatalError = 1;
    return 1;
  }

  /* Execute Read Indirect Address */
  if ((dmcb->bAccessFlag == IS_INDIRECT_ACCESS) || (dmcb->bAccessFlag == IS_INDIRECT_ACCESS_WITH_OFFSET))  {
    memmove(&bSaveData[0], &tscb->bSectorBuffer[0], sizeof(dwAddress)); /* save data */
    do {
      rc = driveIoCtrl(dwAddress, wSize, &tscb->bSectorBuffer[0], bProtocol, uplb);
      if (!rc) {
        memcpyWith32bitLittleEndianTo32bitConversion(&dwAddress, &tscb->bSectorBuffer[0], sizeof(dwAddress));
        ts_printf(__FILE__, __LINE__, "Update Indirect address %08xh -> Direct address %08xh", dmcb->dwAddress, dwAddress);
        dmcb->dwAddress = dwAddress;
        dmcb->dwAddressOffset = 0;
        if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_MFG_ID)) {
          wSize = dmcb->wSize = sizeof(tscb->bDriveSerialNumber) + sizeof(tscb->bDriveMfgId);
        } else {
          wSize = dmcb->wSize;
        }
        dmcb->bAccessFlag = IS_DIRECT_ACCESS;
        if (isRead) {
          bProtocol = READ_DRIVE_MEMORY_PROTOCOL;
        } else {
          bProtocol = WRITE_DRIVE_MEMORY_PROTOCOL;
        }
        break;
      }
      ts_sleep_partial(DRIVE_ACCESS_RETRY_INTERVAL_SEC);
    } while (getTestScriptTimeInSec() < dwTimeoutSec);

    if (rc) {
      ts_printf(__FILE__, __LINE__, "Warning - timeout");
      return rc;
    }
    memmove(&tscb->bSectorBuffer[0], &bSaveData[0], sizeof(dwAddress)); /* restore save data */
  }

  /* Execute Identify or Read or Write Direct Address */
  if (dwAddressOffset) { /* with Offset */
    dwAddress += dwAddressOffset;
    ts_printf(__FILE__, __LINE__, "Update address offset %08xh(%d) -> %08xh", dmcb->dwAddress, dmcb->dwAddressOffset, dwAddress);
    dmcb->dwAddress = dwAddress;
    dmcb->dwAddressOffset = 0;
  }





  if (!isRead) {  /* write mode */
    if (dmcb->bEndianFlag == EACH_WORD_WRITE) { /* Write 1 word only */
      memcpy(&bTemp[0], &tscb->bSectorBuffer[0], wSize);
      dwTempAddr = dwAddress;
      for (i = 0;i < wSize;i++) {
        do {
          rc = driveIoCtrl(dwTempAddr, 1, &bTemp[i], bProtocol, uplb);
          dwTempAddr++;
          if (!rc) {
            break;
          }
          ts_sleep_partial(DRIVE_ACCESS_RETRY_INTERVAL_SEC);
        } while (getTestScriptTimeInSec() < dwTimeoutSec);
      }
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Each Word Writing Error");
      } else {
        bTemp[wSize+1] = '\0';
        ts_printf(__FILE__, __LINE__, "Each Word Write %s", bTemp);
      }

      return rc;
    } else {
      /* endian swap if need */
      rc = EndianSwaponSectorBuffer(tscb, dmcb);
      if (rc) {
        return 1;
      }
    }
  }


  do {
    rc = driveIoCtrl(dwAddress, wSize, &tscb->bSectorBuffer[0], bProtocol, uplb);
    if (!rc) {
      break;
    }
    ts_sleep_partial(DRIVE_ACCESS_RETRY_INTERVAL_SEC);
  } while (getTestScriptTimeInSec() < dwTimeoutSec);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Warning - timeout");
  } else {
    /* Get Data in Identify Image */
    if (dmcb->bAccessFlag == IS_IDENTIFY_ACCESS) {
      putBinaryDataDump(&tscb->bSectorBuffer[0], dwAddress, wSize);
    } else {
      if (isRead) {
        /* endian swap if need */
        rc = EndianSwaponSectorBuffer(tscb, dmcb);
        if (rc) {
          return 1;
        }
        /* Dump Data to Console */
        wSize = 16;
        if (wSize > dmcb->wSize) {
          wSize = dmcb->wSize;
        }
        putBinaryDataDump(&tscb->bSectorBuffer[0], dmcb->dwAddress, wSize);
      }
    }
  }

  return rc;
}

/**
 * <pre>
 * Description:
 *   Get drive Error code .
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte Getdriveerrorcode(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc = 1;
  /* Error Code */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_ERROR_CODE, 0, READ_FROM_DRIVE);
  if (!rc) {
    memmove((Byte *)&tscb->wDriveNativeErrorCode, &tscb->bSectorBuffer[0], sizeof(tscb->wDriveNativeErrorCode));
  }

  return rc;
}

/**
 * <pre>
 * Description:
 *   Get drive Each head nuber of PES pages .
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************
 2010/05/28 Support JPT or later PES Dump Y.Enomoto
 *****************************************************************************/
Byte GetdrivePespage(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Word *wsource = (Word *) & tscb->bSectorBuffer[0];
  Word head;
  Byte rc = 1;

  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_PES_HEAD, 0, READ_FROM_DRIVE);
  if (!rc) {
    for (head = 0;head < tscb->wDriveHeadNumber;head++) {
      memmove((Byte *)&tscb->wDrivePesPage[head], wsource++, sizeof(tscb->wDriveNativeErrorCode));
      ts_printf(__FILE__, __LINE__, "Head %d pes size %d page", head, tscb->wDrivePesPage[head]);
    }
  }

  return rc;
}

