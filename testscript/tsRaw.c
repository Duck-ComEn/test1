#include "ts.h"

/**
 * <pre>
 * Description:
 *   Return non-zero value if drive reporting data ready
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************
 2010/05/26 Support ReportFlag = 3 Get before Rawdata Dump & Report after Rawdata Dump Y.Enomoto
 *****************************************************************************/
Byte isDriveReportingDataReady(TEST_SCRIPT_CONTROL_BLOCK *tscb, DRIVE_MEMORY_CONTROL_BLOCK *dmcb, Byte bStateNow)
{
  Byte *p = NULL;
  Byte bTargetStep = 0;

  /* check condition #1 (no report required) */
  if (dmcb->bReportFlag == IS_REPORT_NOT_REQUIRED) {
    return 0;
  }

  /* check condition #2 (report before rawdata dump) or (get before and report after rawdata dump) */
  /***** Need ReportFlag is 1 or 3 ******/
  if ((dmcb->bReportFlag == IS_REPORT_BEFORE_RAWDATA_DUMP) || /*1*/
      (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP) || /*3*/
      (dmcb->bReportFlag == IS_REPORT_BEFORE_RAWDATA_DUMP_ONLY_SRST) ||  /* 4 same report with 1 */
      (dmcb->bReportFlag == IS_REPORT_BEFORE_RAWDATA_DUMP_ONLY_FINAL) ||  /* 5 same report with 1 */
      (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_SRST) ||   /* same report with 3 */
      (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_FINAL)) {   /* same report with 3 */
    if (bStateNow == GET_DRIVE_RAWDATA_FSM_get_drive_data_before_rawdata_dump) {
      if (isDriveRawdataReady(tscb)) {
        return 1;
      } else {
        return 0;
      }
    }
  }

  /* check condition #3 (report after rawdata dump) */
  if ((dmcb->bReportFlag == IS_REPORT_AFTER_RAWDATA_DUMP /*2*/) ||
      (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP/*3*/) ||
      (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_SRST) ||   /* 6 same report with 3 */
      (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_FINAL)) {   /* 7 same report with 3 */
    if (bStateNow == GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump) {
      return 1;
    }
  }

  /* check condition #4 (report after given signature finished) */
  if (isgraph(dmcb->bReportFlag)) {
    /* if current step is at END point, then always true */
    if (tscb->bDriveSignature[tscb->bDriveStepCount] == 'E') {
      return 1;
    }

    /* search target step */
    p = memchr(&tscb->bDriveSignature[0], dmcb->bReportFlag, sizeof(tscb->bDriveSignature));
    if (p == NULL) {
      ts_printf(__FILE__, __LINE__, "Error - signature not found %xh", dmcb->bReportFlag);
      putBinaryDataDump((Byte *)&tscb->bDriveSignature[0], (Dword)&tscb->bDriveSignature[0], sizeof(tscb->bDriveSignature));
      return 0;
    }
    bTargetStep = (Byte)(p - &tscb->bDriveSignature[0]);

    /* if current is larger than target, then true*/
    if (tscb->bDriveStepCount > bTargetStep) {
      return 1;
    } else {
      return 0;
    }
  }

  return 0;
}

/**
 * <pre>
 * Description:
 *   Get drive data
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************
 2010/05/28 Support JPT or later Rawdata Dump Y.Enomoto
 2010/06/15 Support PTB,PTC EC:6350 Dump Y.Enomoto
 *****************************************************************************/
Byte getDriveReportingData(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte bStateNow)
{
  TEST_RESULT_SAVE_BLOCK *trsb = &tscb->testResultSaveBlock[0];
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Dword i = 0;
  Dword j = 0;
  Dword dwsavedatanumber = 0;
  Byte rc = 0;
  DRIVE_MEMORY_CONTROL_BLOCK *dmcb = NULL;
  DRIVE_MEMORY_CONTROL_BLOCK *dmcbtpi = NULL;
  Dword dwHugCpuCounter = 0;
  Byte bBuf = ' ';
  Byte isReportIdDuplicationSupported = 0; /* replacing last byte of bName with dummy word '$' to identify duplicated report id. */
  Byte *bData_address;
  Word scode;

  ts_printf(__FILE__, __LINE__, "getDriveReportingData");

  for (i = 0 ; i < (sizeof(tscb->driveMemoryControlBlock) / sizeof(tscb->driveMemoryControlBlock[0])) ; i++) {
    dmcb = &tscb->driveMemoryControlBlock[i];

    /* check if end of dmcb entry */
    if (dmcb->bName[0] == '\0') {
      break;
    }

    /* check if reporting data is ready */
    if (!isDriveReportingDataReady(tscb, dmcb, bStateNow)) {
      continue;
    }

    /* read data */
    ts_printf(__FILE__, __LINE__, "report item %s", &dmcb->bName[0]);
    if (isReportIdDuplicationSupported) {
      bBuf = dmcb->bName[sizeof(dmcb->bName) - 2];
      dmcb->bName[sizeof(dmcb->bName) - 2] = '$';
    }
    if ((dmcb->bReportFlag != IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP/*3*/ &&
         dmcb->bReportFlag != IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_SRST/*6*/ &&
         dmcb->bReportFlag != IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_FINAL/*7*/) || (bStateNow != GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump)) {
      if (dmcb->bAccessFlag != IS_IDENTIFY_ACCESS) {
        if (MATCH == strcmp(&dmcb->bName[0], KEY_TPI_INFORMATION)) {
          memmove(&tscb->bSectorBuffer[0], &tscb->bDriveBPITPITable[0], dmcb->wSize);
        } else {
          rc = driveIoCtrlByKeywordWithRetry(tscb, &dmcb->bName[0], 0, READ_FROM_DRIVE);
        }
      }
    }
    if (isReportIdDuplicationSupported) {
      dmcb->bName[sizeof(dmcb->bName) - 2] = bBuf;
    }
    if (rc) {
      break;
    } else {
      if ((dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP/*3*/ ||
           dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_SRST/*6*/ ||
           dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_FINAL/*7*/) && (bStateNow == GET_DRIVE_RAWDATA_FSM_get_drive_data_before_rawdata_dump)) {
        if (dmcb->bAccessFlag != IS_IDENTIFY_ACCESS) {
          if (dwsavedatanumber < 64) {
            trsb = (TEST_RESULT_SAVE_BLOCK *) & tscb->testResultSaveBlock[dwsavedatanumber++];
            memmove(trsb->bSaveaddressTop, &tscb->bSectorBuffer[0], dmcb->wSize);
          } else {
            rc = 1;
            ts_printf(__FILE__, __LINE__, "Error - No space for save data number %d", dwsavedatanumber);
            break;
          }
        }
      }
    }

    /* tpi search PTB,PTC only */
    if (tscb->isTpiSearch) {
      if (MATCH == strcmp(&dmcb->bName[0], KEY_GEOPARAMETER)) {
        rc = queryDriveMemoryControlBlock(tscb, KEY_TPI_INFORMATION, &dmcbtpi);
        if (!rc) {
          for (j = 0 ; j < tscb->wTpiSearchSize ; j++) {
            tscb->bDriveBPITPITable[j] = tscb->bSectorBuffer[dmcbtpi->dwAddressOffset + (j * tscb->wTpiSearchOffset)];
          }
          dmcbtpi->wSize = tscb->wTpiSearchSize;
        }
      }
    }

    /* report to host ( Get before Rawdata Dump & Report after Rawdata Dump) */
    if ((dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP/*3*/ ||
         dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_SRST/*6*/ ||
         dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_FINAL/*7*/ ) && (bStateNow == GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump)) {
      if (dmcb->bAccessFlag != IS_IDENTIFY_ACCESS) {
        if (dwsavedatanumber < 64) {
          trsb = (TEST_RESULT_SAVE_BLOCK *) & tscb->testResultSaveBlock[dwsavedatanumber++];
          memmove(&tscb->bSectorBuffer[0], trsb->bSaveaddressTop,  dmcb->wSize);
        } else {
          rc = 1;
          ts_printf(__FILE__, __LINE__, "Error - No space for save data number %d", dwsavedatanumber);
          break;
        }
      }
    }

    if ((dmcb->bReportFlag != IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP/*!3*/ &&
         dmcb->bReportFlag != IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_SRST/*6*/ &&
         dmcb->bReportFlag != IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_FINAL/*7*/) || (bStateNow != GET_DRIVE_RAWDATA_FSM_get_drive_data_before_rawdata_dump)) {
      if (dmcb->bAccessFlag == IS_IDENTIFY_ACCESS) {
        bData_address = queryDriveDataAddress(tscb, dmcb);
        reportHostTestData(tscb, &dmcb->bName[0], bData_address, dmcb->wSize);
      } else {
        if (tscb->bPartFlag == SRST_PART) {
          if ((dmcb->bReportFlag == IS_REPORT_BEFORE_RAWDATA_DUMP_ONLY_FINAL/*5*/) || (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_FINAL/*7*/)) {
            ts_printf(__FILE__, __LINE__, "No report in SRST part %s", &dmcb->bName[0]);
          } else {
            reportHostTestData(tscb, &dmcb->bName[0], &tscb->bSectorBuffer[0], dmcb->wSize);
            ts_printf(__FILE__, __LINE__, "reported in SRST part %s", &dmcb->bName[0]);
          }
        }
        if (tscb->bPartFlag == FINAL_PART) {
          if ((dmcb->bReportFlag == IS_REPORT_BEFORE_RAWDATA_DUMP_ONLY_SRST /*4*/) || (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_SRST/*6*/)) {
            ts_printf(__FILE__, __LINE__, "No report in FINAL part %s", &dmcb->bName[0]);
          } else {
            reportHostTestData(tscb, &dmcb->bName[0], &tscb->bSectorBuffer[0], dmcb->wSize);
            ts_printf(__FILE__, __LINE__, "reported in FINAL part %s", &dmcb->bName[0]);
          }
        }
      }
    }

    if (bStateNow == GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump) {
      if ((MATCH == strcmp(&dmcb->bName[0], KEY_SRST_SEQUENCE)) || (MATCH == strcmp(&dmcb->bName[0], KEY_SRST_SEQUENCE2))) {
        if ((MATCH != memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SRST_ABORT, strlen(SIGNATURE_SRST_ABORT))) &&
            (MATCH != memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SRST_COMPLETE, strlen(SIGNATURE_SRST_COMPLETE))) &&
            (MATCH != memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SRST_PENDING, strlen(SIGNATURE_SRST_PENDING)))
           ) {
          tseb->isDriveSinetureError = 1;
        } else {
          tseb->isDriveSinetureError = 0;
          if (tscb->wDriveEchoVersion >= 3) {
            /* for MNR or after  ( for grading )INTERLOCK If GARADING LABEL cannot wirte or unknown,Drive cannot define PASS2 lable. */
            if (MATCH == memcmp(&tscb->bSectorBuffer[0], SIGNATURE_SRST_PENDING, strlen(SIGNATURE_SRST_PENDING))) {
              tseb->isGradingError = PENDING_ERROR;
            }
          }
        }
        if (tscb->wDriveEchoVersion == 0) { /* PTB,PTC */
          if (tscb->wDriveNativeErrorCode == 0x28) {
            scode = tscb->bSectorBuffer[0x94] + (tscb->bSectorBuffer[0x94+1] << 8);
            if (scode == 0x0031) {
              tscb->wDriveNativeErrorCode = 0xFF28; /* EC:6350 */
            }
          }
          tseb->isDriveSinetureError = 0;
        }
      }
    }

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }
  }
  return rc;
}

/**
 * <pre>
 * Description:
 *   Get drive parameters for rawdata dump
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************
 2010/05/28 Support JPT or later Rawdata Dump Y.Enomoto
 *****************************************************************************/
unsigned char getDriveParametersForResultDump(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc = 1;
  Word wNumOfPage = 0;
  Word wSizeOfLastPage = 0;

  ts_printf(__FILE__, __LINE__, "getDriveParametersForResultDump");

  /* num of page */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_LAST_PAGE, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  memmove(&wNumOfPage, &tscb->bSectorBuffer[0], sizeof(wNumOfPage));

  /* pointer in last page */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_LAST_POINTER, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  memmove(&wSizeOfLastPage, &tscb->bSectorBuffer[0], sizeof(wSizeOfLastPage));

  /* size calculation */
  if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
    tscb->dwDriveResultTotalSizeByte = wNumOfPage * DRIVE_SECTOR_SIZE_BYTE;
  } else {
    tscb->dwDriveResultTotalSizeByte = ((wNumOfPage - 1) * DRIVE_SECTOR_SIZE_BYTE) + wSizeOfLastPage;
  }
  ts_printf(__FILE__, __LINE__, "srst result size %d byte", tscb->dwDriveResultTotalSizeByte);
  tscb->dwDriveResultDumpSizeByte = 0;

  /* get Srst UART flag */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_UART_FLAG, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  memmove(&tscb->bDriveSrstuartflag, &tscb->bSectorBuffer[0], sizeof(tscb->bDriveSrstuartflag));

  return 0;
}

/**
 * <pre>
 * Description:
 *   Get drive parameters for pes dump
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************
 2010/05/28 Support JPT or later PES Dump Y.Enomoto
 *****************************************************************************/
unsigned char getDriveParametersForPesDump(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 1;
  Word wNumOfPage = 0;

  ts_printf(__FILE__, __LINE__, "getDriveParametersForPesDump");

  if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
    rc = GetdrivePespage(tscb);
    if (rc) {
      return 1;
    }
  } else {
    /* Get TotalNumberOfPagePES */
    rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_PES_TOTAL_NUMBER_OF_PAGE, 0, READ_FROM_DRIVE);
    if (rc) {
      return 1;
    }
    memmove((Byte *)&wNumOfPage, &tscb->bSectorBuffer[0], sizeof(wNumOfPage));

    /* Total Number of Page PES must be even, becase 1 PES unit is 1024 bytes (2 sector) */
    if (wNumOfPage % 2) {
      ts_printf(__FILE__, __LINE__, "pes page invalid %xh", wNumOfPage);
      tseb->isDriveRawdataDumpPesPageError = 1;
      return 1;
    }
    tscb->dwDrivePesTotalSizeByte = (wNumOfPage / 2) * (DRIVE_SECTOR_SIZE_BYTE * 2 + DRIVE_PES_HEADER_SIZE);
    ts_printf(__FILE__, __LINE__, "pes size %d byte", tscb->dwDrivePesTotalSizeByte);
  }
  tscb->dwDrivePesDumpSizeByte = 0;

  return 0;
}

/**
 * <pre>
 * Description:
 *   Get drive parameters for pes header
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
unsigned char getDriveParametersForPesHeader(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc = 1;

  ts_printf(__FILE__, __LINE__, "getDriveParametersForPesHeader");

  /* Get PesCylinder */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_PES_CYLINDER, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  memmove((Byte *)&tscb->dwDrivePesCylinder, &tscb->bSectorBuffer[0], sizeof(tscb->dwDrivePesCylinder));
  ts_printf(__FILE__, __LINE__, "pes cylinder %d", tscb->dwDrivePesCylinder);

  /* Get PesHeadNumber */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_PES_HEAD, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  } else {
    tscb->bDrivePesHeadNumber = tscb->bSectorBuffer[0];
  }
  ts_printf(__FILE__, __LINE__, "pes head %d", tscb->bDrivePesHeadNumber);

  /* Get PesTemperature */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_PES_TEMPERATURE, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  } else {
    tscb->bDrivePesTemperature = tscb->bSectorBuffer[0];
  }
  ts_printf(__FILE__, __LINE__, "pes temperature %d", tscb->bDrivePesTemperature);

  /* Get PesDataCount */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_PES_DATA_COUNT, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  memmove((Byte *)&tscb->wDrivePesDataCount, &tscb->bSectorBuffer[0], sizeof(tscb->wDrivePesDataCount));
  ts_printf(__FILE__, __LINE__, "pes data count %d", tscb->wDrivePesDataCount);

  /* Get PesSpindleSpeed */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_SPINDLE_RPM, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  memmove((Byte *)&tscb->wDrivePesSpindleSpeed, &tscb->bSectorBuffer[0], sizeof(tscb->wDrivePesSpindleSpeed));
  ts_printf(__FILE__, __LINE__, "pes spindle speed %d", tscb->wDrivePesSpindleSpeed);

  /* Get PesNumberOfServoSector */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_NUMBER_OF_SERVO_SECTOR, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  memmove((Byte *)&tscb->wDrivePesNumberOfServoSector, &tscb->bSectorBuffer[0], sizeof(tscb->wDrivePesNumberOfServoSector));
  ts_printf(__FILE__, __LINE__, "pes number of servo sector %d", tscb->wDrivePesNumberOfServoSector);

  return 0;
}

/**
 * <pre>
 * Description:
 *   Remove ECC data from sector buffer data
 * Arguments:
 *   Dword dwAddress Read Buffer Address
 *   Dword length    Read Data length
 * </pre>
 *****************************************************************************/
Dword removeECCbytes(Byte *BufferAddress, Dword dwlength, Dword dwECClength)
{
  Dword dwSourceOffset = DRIVE_SECTOR_SIZE_BYTE + dwECClength; /* JPT (512 + 16 = 528) */
  Dword dwDatalength = DRIVE_SECTOR_SIZE_BYTE;
  Byte *bsource = BufferAddress + dwSourceOffset;
  Byte *bdestination = BufferAddress + dwDatalength;

  if (dwlength < dwDatalength) {
    dwDatalength = dwlength;
  }
  dwlength -= dwSourceOffset;
  while ( dwlength > 0) {
    memmove(bdestination, bsource, DRIVE_SECTOR_SIZE_BYTE);
    bdestination += DRIVE_SECTOR_SIZE_BYTE;
    bsource += dwSourceOffset;
    dwDatalength += DRIVE_SECTOR_SIZE_BYTE;
    dwlength -= dwSourceOffset;
  }

  return dwDatalength;
}

/**
 * <pre>
 * Description:
 *   Get drive Buffer data for SRST Result,Pes, Parametric data
 *   Return 0:Sucess 1:Error 99: Wait Page number sync(for PTB,PTC)
 * Arguments:
 *   Dword dwAddress Drive Address
 *   word  wPage     Number of Read pages
 *   word  wExpectedPageIndex  Expect Page Number (for PTB,PTC)
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte  readRawdataFromDrive(Dword dwAddress, Word wPage, Word wExpectedPageIndex, TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte *bWriteDataPointer = &tscb->bSectorBuffer[0];
  UART_PERFORMANCE_LOG_BLOCK *uplb = &tscb->uartPerformanceLogBlock[0];
  Dword dwAllReadbyte = 0;
  Dword dwRemainReadbyte = 0;
  Dword dwlimitedReadbyte = 0;
  Dword dwdatabyte = 0;
  Word  wPageNumber = 0;
  Word  wRemainPage = wPage;
  Byte bPageIndex[6];
  Byte rc = 0;


  ts_printf(__FILE__, __LINE__, "read Rawdata from drive");
  if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
    dwAllReadbyte = dwRemainReadbyte = (DRIVE_SECTOR_SIZE_BYTE + tscb->wDriveECClength) * wPage; /* input toal data byte*/
    if (wPage > 64) {
      dwlimitedReadbyte = (DRIVE_SECTOR_SIZE_BYTE + tscb->wDriveECClength) * 64;
      rc = driveIoCtrl(dwAddress, dwlimitedReadbyte, bWriteDataPointer, READ_DRIVE_MEMORY_PROTOCOL, uplb);
      dwAddress += dwlimitedReadbyte;
      dwRemainReadbyte -= dwlimitedReadbyte;
      bWriteDataPointer += dwlimitedReadbyte;
      wRemainPage -= 64;
    }
    if (!rc) { /* 1st read success */
      while (wRemainPage > 0) {  /* There is remain page */
        if (wRemainPage > 64) {
          dwlimitedReadbyte = (DRIVE_SECTOR_SIZE_BYTE + tscb->wDriveECClength) * 64;
        } else {
          dwlimitedReadbyte = (DRIVE_SECTOR_SIZE_BYTE + tscb->wDriveECClength) * wRemainPage;
        }
        rc = driveIoCtrl(dwAddress, dwlimitedReadbyte, bWriteDataPointer, READ_DRIVE_MEMORY_PROTOCOL, uplb); /* Read remain page*/
        if (!rc) {
          dwAddress += dwlimitedReadbyte;
          dwRemainReadbyte -= dwlimitedReadbyte;
          bWriteDataPointer += dwlimitedReadbyte;
          if (wRemainPage > 64) {
            wRemainPage -= 64;
          } else {
            wRemainPage = 0;
          }
        } else {
          break;
        }
      }
      if (!rc) {
        dwdatabyte = removeECCbytes(&tscb->bSectorBuffer[0], dwAllReadbyte , tscb->wDriveECClength);
      }
    }
    /* nothing when reading fail */
    if (dwdatabyte != (Dword)(wPage * DRIVE_SECTOR_SIZE_BYTE)) {
      rc = 1;
      ts_printf(__FILE__, __LINE__, "Error - drive rawdata number of page %d get bytes %d", wPage, dwdatabyte);
    }
  } else { /* PTB,PTC */
    if (tscb->bDrivePORrequestflag == 1) {
      while (wExpectedPageIndex > wPageNumber) {
        rc = driveIoCtrl(dwAddress, 6, &bPageIndex[0], READ_DRIVE_MEMORY_PROTOCOL, uplb);
        if (rc == 0) {
          wPageNumber = bPageIndex[4] + (bPageIndex[5] << 8);
          if (wExpectedPageIndex == wPageNumber) {
            tscb->bDrivePORrequestflag++;
            break;
          } else if (wExpectedPageIndex < (wPageNumber + wPage)) {
            dwAddress += 512;
            wRemainPage--;
            if (wRemainPage != 0)
              continue;
            rc = 1;
            ts_printf(__FILE__, __LINE__, "Error - drive rawdata Expect page %d Get page %d", wExpectedPageIndex, wPageNumber);
            break;
          }
          rc = 99;
          ts_printf(__FILE__, __LINE__, "Read continue rawdata Expect page %d Get page %d", wExpectedPageIndex, wPageNumber);
        }
        break;
      }
    }
    if (rc == 0) {
      rc = driveIoCtrl(dwAddress, (DRIVE_SECTOR_SIZE_BYTE * wPage), bWriteDataPointer, READ_DRIVE_MEMORY_PROTOCOL, uplb);
    }
  }
  return rc;
}


/**
 * <pre>
 * Description:
 *   Get drive rawdata.
 * Arguments:
 *   *tscb - Test script control block
 * Sequence:
 *  - Operation Flow for rawdata reporting
 *
 *        Bit0     On  by SRST at the end of test (N=0)
 *                        or immediate flag Bit 0 on.
 *        Bit4     On  by SRST if FFT data was available.
 *        Bit5     On  by SRST if MCS data was available.
 *
 *   LP1  Bit6     On  by SRST before rawdata or FFT data read from disk.
 *        Bit6     Off by SRST after rawdata or FFT data read from disk.
 *
 *        Bit1     On  by SRST if setup rawdata or FFT data complete with successful.
 *                 -> Set number of page to be read to Offset 0x140.
 *         or
 *        Bit7     On  by SRST when any errors occurred at setup.
 *
 *        Bit0     Checked by Tester ( On -> Off reporting complete )
 *
 *   LP2  Bit7     Checked by Tester ( On -> FAIL )
 *
 *        Bit1     Checked by Tester ( On -> get rawdata or FFT data 512 x page number byte )
 *
 *        Bit2     Off by Tester when finish to get whole 512 x page number byte w/o any errors
 *                 On  by Tester when any error occurred , goto LP2
 *
 *        Bit1     Off by Tester
 *
 *        Bit1     Checked by SRST ( Polling )
 *
 *        Bit2     Checked by SRST  Bit1 Off
 *                                  Bit2 Off -> Next result to be read
 *                                  Bit2 On  -> if retry count > 16 -> Fail
 *                                                             < 16 retry goto LP1
 *                 Check page number by SRST ( N <= rawdata max or FFT data max goto LP1 )
 *
 *        Bit4     Checked by Tester after get the all of rawdata
 *                  On  -> Next FFT data
 *                          Set PES data count to Offset 0x264 by SRST
 *                          Set Spindle speed to  Offset 0x266 by SRST
 *                          Set Total PES page to Offset 0x268 by SRST
 *                          Set Head number to    Offset 0x12C by SRST
 *                          Set Temperature to    Offset 0x1E2 by SRST
 *
 *                          Get PES data count from Offset 0x264 by Tester
 *                          Get Spindle speed  from Offset 0x266 by Tester
 *                          Get Total PES page from Offset 0x268 by Tester
 *                          Get Head number    from Offset 0x12C by Tester
 *                          Get Temperature    from Offset 0x1E2 by Tester
 *
 *                          Goto LP1
 *
 *        Bit5     Checked by Tester after get all of rawdata & FFT data
 *                  On  -> Next MCS data  ( Just 512Byte , Page number = 1 )
 *                          Goto LP1
 *                  Off -> Goto END
 *
 *
 *   END  Bit0     Off by SRST when finish to report all data
 *
 *        (*) If power shutdown occurred during rawdata reporting , it resume from
 *            first page.
 *
 *  - Operation Flow for memory reporting
 *
 *        Bit3     On  by SRST for memory dump
 *                 Address is set to Offset 170H (double word 4 byte)
 *                 Lenght is set to  Offset 174H (word 2 byte)
 *        Bit3     Off by Tester after get all of memeory data
 *
 *    SRST -> Final process change
 *
 *  =================================================================
 *     Rawdata Reporting Error Recovery
 *  =================================================================
 *    If any error occures in rawdata reporting, tester will retry as following.
 *     1) Tester finds any errors in rawdata reporting mode
 *     2) Keep resume parameter#1 "Total Rawdata Page"    => R1
 *     3) Keep resume paremeter#2 "Reported Rawdata Page" => R2
 *     4) Tester shuts off drive power
 *     5) Tester waits 5 seconds
 *     6) Tester turns on drive power
 *     7) Tester waits 5 seconds / Drive waits 10 seconds
 *     8) Tester sets (R1) to drive "Total Rawdata Page"
 *     9) Tester sets (R2+1) to drive "Restart Rawdata Page"
 *    10) Tester sets "Test time out" bit in "Immediate flag"
 *    11) Drive starts rawdata reporting mode with resume parameters
 *    12) Tester re-starts rawdata reporting mode if "Under reporting rawdata" bit in "Status and control" is set
 *
 *  =================================================================
 *     Mobile Only
 *  =================================================================
 *     prohibit_reSRST
 *         => all mobile product when rawdata dump complete
 *         if (timeover[ rc ] || srstEC[ rc ] == 0x31) {
 *                 extEC [ rc ] = 4;
 *                 srstEC [ rc ] = prohibit_reSRST [ rc ];
 *                 sendMessage("61xx series EC:%x ", srstEC[rc]);
 *         }
 *         ==> default mode is prohibit re-srst.
 *
 *     (not required) voltage change request => BRK only
 *     (not required) EC:6350 (getparameters)
 *     (not required) porRecovery() & resumeGetRawdata() => all product, but under construction by Enomoto-san
 *     (not required) SrstTimeCriteria, TotalSrstTime => not in use?
 *     (not required) grading => FLC only
 *
 * </pre>
 *****************************************************************************
 2010/05/26 Support ReportFlag = 3 Get before Rawdata Dump & Report after Rawdata Dump Y.Enomoto
 *****************************************************************************/
Byte getDriveRawdata(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  DRIVE_MEMORY_CONTROL_BLOCK *dmcb = NULL;
  UART_PERFORMANCE_LOG_BLOCK *uplb = &tscb->uartPerformanceLogBlock[0];
  Byte bStatePrev = GET_DRIVE_RAWDATA_FSM_before_initialize;
  Byte bStateNow = GET_DRIVE_RAWDATA_FSM_initialize;
  Word wReportHead = 0;
  Byte rc = 1;
  Dword dwHugCpuCounter = 0;
  Byte bcheck6100[4];
  Dword dwTimeoutSec = 0;

  Word wSetupPageNumber = 0;
  Dword dwSetupPageSizeByte = 0;
  Dword dwSetupPageAddress = 0;

  Word wExpectedPageIndex = 0;
  Word wActualPageIndex = 0;
  Word wCheckEndPageIndex = 0;
  Word wCheckPageOffset = 0;
  Byte brawdataErrFlag = 0;
  Byte bRetryCount = 0;
  Byte bporRecoveryCount = 0;
  Byte bassignedEC = 0;
  Byte bLabel[sizeof(KEY_PES_DATA)];
  Byte gradingcheckf;
  Byte i;
  Dword dwReportTime, dwReportEndTime;

  ts_printf(__FILE__, __LINE__, "getDriveRawdata(%s)", tscb->bDriveSerialNumber);
  dwTimeoutSec = getTestScriptTimeInSec() + tscb->dwDriveTestRawdataDumpTimeoutSec;
  ts_printf(__FILE__, __LINE__, "timeout(Remain) = %d sec", dwTimeoutSec);

  rc = getDriveRawdataStatusAndControl(tscb);

  if (tscb->bPartFlag == FINAL_PART) {
    gradingcheckf = 0;
    if (tscb->wDriveStatusAndControlFlag & IS_GRADING_MODEL) {
      gradingcheckf = 1;
    }
    ts_printf(__FILE__, __LINE__, "Final gradingcheckf = %d Status %04x", gradingcheckf, tscb->wDriveStatusAndControlFlag);
  }

  /* Dump temperature & Step count [019KPD] */
  reportHostTestData(tscb, KEY_TEMPERATURE, &tscb->bDriveTemperature, 1); 
  reportHostTestData(tscb, KEY_STEP_COUNT, &tscb->bDriveStepCount, 1);

  /* When cut off timeout happened, dump SRST status page and finish rawdata [019KPD] */
  if(tseb->isDriveTestTimeout){
    ts_printf(__FILE__, __LINE__, "Cut off timeout happened!");
    for (i = 0 ; i < (sizeof(tscb->driveMemoryControlBlock) / sizeof(tscb->driveMemoryControlBlock[0])) ; i++) {
      dmcb = &tscb->driveMemoryControlBlock[i];
      if(MATCH == strcmp(&dmcb->bName[0], KEY_SRST_PARAMETER)){
        rc = driveIoCtrlByKeywordWithRetry(tscb, &dmcb->bName[0], 0, READ_FROM_DRIVE);
        reportHostTestData(tscb, &dmcb->bName[0], &tscb->bSectorBuffer[0], dmcb->wSize);
        break;
      }
    }
    tscb->isDriveRawdataComplete = 1;
    return 0;
  }


  for (;;) {

    /* error check */
    if (tseb->isFatalError) {
      ts_printf(__FILE__, __LINE__, "Fatal error first");
      return 1;
    }
    if (brawdataErrFlag) {
      rc = setDriveRawdataRetryRequest(tscb);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Rawdata Retry Request Write Error (GOD3) brawdataErrFlag=%d", brawdataErrFlag);
        tseb->isDriveWriteRetryRequestError = 1;
      } else {
        tseb->isDriveWriteRetryRequestError = 0;
      }
      if (bRetryCount++ > RAWDATA_RETRY_CONT) {
        if (tseb->isDriveRawdataDumpPageIndexError == 0) {
          ts_printf(__FILE__, __LINE__, "Retry:%d tseb->isDriveRawdataDumpDriveError = %d (GOD3)", bRetryCount, tseb->isDriveRawdataDumpDriveError);
          tseb->isDriveRawdataDumpDriveError = 1;
        }
        ts_printf(__FILE__, __LINE__, "Rawdata Retry Out");
        bStateNow = GET_DRIVE_RAWDATA_FSM_finalize;
        bassignedEC = 1;
      }
    }


    /* env check */
    rc = isEnvironmentError(tscb);
    if (rc) { /* set tseb->isDriveUnplugged = 1 in isEnvironmentError */
      return 1;
    }

    /* clear reporting timer on srst */
    rc = clearReportingTimerOnSsrst(tscb);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "clear repoting timer flag error");
    }

    /* timeout check */
    if (dwTimeoutSec < getTestScriptTimeInSec()) {
      ts_printf(__FILE__, __LINE__, "timeout Error = %d < %d", dwTimeoutSec, getTestScriptTimeInSec());
      tseb->isDriveRawdataDumpTimeoutError = 1;
      return 1;
    }

    /* state change check */
    if (bStateNow != bStatePrev) {
      ts_printf(__FILE__, __LINE__, "change state %d -> %d", bStatePrev, bStateNow);
    } else {
      ts_sleep_partial(WAIT_TIME_SEC_FOR_DRIVE_RAWDATA_FLAG_POLL);
    }

    bStatePrev = bStateNow;

    /* release cpu resource */
    if (dwHugCpuCounter++ >= RELEASE_CPU_TRIGGER) {
      dwHugCpuCounter = 0;
      ts_sleep_partial(WAIT_TIME_SEC_FOR_RELEASE_CPU);
    }

    /* run state machine */
    switch (bStateNow) {
      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_initialize:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "1  GET_DRIVE_RAWDATA_FSM_initialize");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");

      memmove(&tscb->bSectorBuffer[0], (Byte *)&tscb->bLibraryVersion[0], tscb->dwVersionLength);     /* [019KP3] */
      reportHostTestData(tscb, KEY_LIBRARY_VERSION, &tscb->bSectorBuffer[0], tscb->dwVersionLength);  /* [019KP3] */

      rc = getDriveRawdataStatusAndControl(tscb);
      if (!rc) {
        if (tscb->bDriveSkip_SRST_result) {
          bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_before_rawdata_dump;
          break;
        }
        rc = isDriveRawdataReady(tscb);
        if (!rc) {
          if (tscb->bDrivePORrequestflag) {
            ts_printf(__FILE__, __LINE__, "Wait drive rawdata ready after POR");
            break;
          }
          ts_printf(__FILE__, __LINE__, "Error - drive rawdata not ready (GOD3).tscb->bDrivePORrequextflag = %d", tscb->bDrivePORrequestflag);
          tseb->isDriveRawdataDumpOtherError = 1;
          return 1;
        }
      } else {
        break;
      }

      bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_error_code;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_get_drive_error_code:
      /* -------------------------------------------------------------- */
      /* Error Code */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "2  GET_DRIVE_RAWDATA_FSM_get_drive_error_code");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      rc = Getdriveerrorcode(tscb);
      if (rc) {
        break; /* Error */
      }
      bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_before_rawdata_dump;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_get_drive_data_before_rawdata_dump:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "3  GET_DRIVE_RAWDATA_FSM_get_drive_data_before_rawdata_dump");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      rc = getDriveReportingData(tscb, bStateNow);
      dwReportTime = getTestScriptTimeInSec();
      ts_printf(__FILE__, __LINE__, "Report Start Time %d", dwReportTime);
      if (rc) {
        break;
      }
      rc = getDriveRawdataStatusAndControl(tscb);
      if (tscb->bPartFlag == FINAL_PART) {
        if (gradingcheckf) {                                       /* refer -> if(tscb->wDriveStatusAndControlFlag & IS_GRADING_MODEL){ gradingcheckf = 1; }*/
          if (getGradingflag(tscb)) {  /*Get Flag from SRST memory*/
            ts_printf(__FILE__, __LINE__, "Grading flag getting error");
            tseb->isGradingError = NO_SW_TBL;
          } else {
            if ((tscb->dwDriveGradingFlag == 0 && tscb->wDriveEchoVersion < 4) || (tscb->dwDriveGradingWaterFallFlag == 0 && tscb->wDriveEchoVersion >= 4 )) {
              ts_printf(__FILE__, __LINE__, "Grading no request");
            } else {
              ts_printf(__FILE__, __LINE__, "GRADING FLAG ON");
              rc = grading(tscb);
              if (rc) {
                tseb->isGradingError = rc;
              }
            }
          }
        }
      }
      reportHostId(tscb);
      reportHostGradingId(tscb);

      if (tscb->bDriveSkip_SRST_result) {
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump;
        break;
      }
      bStateNow = GET_DRIVE_RAWDATA_FSM_get_parameters_for_srst_result;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_get_parameters_for_srst_result:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "4  GET_DRIVE_RAWDATA_FSM_get_parameters_for_srst_result");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      rc = getDriveParametersForResultDump(tscb); /* get SRST result page number and address which saved SRST result */
      if (rc) {
        break;
      }
      rc = driveIoCtrl((Dword)(tscb->dwDriveSrstTopAddress + TEST_TIME), 4, &bcheck6100[0], READ_DRIVE_MEMORY_PROTOCOL, uplb);  /* #define TEST_TIME (0x5CE) in ts.h */
      if (rc) {
        break;
      } else { /* Function test write testtime in TEST_TIME address, it will be E/C 6100. */
        tscb->bCheckflag = 0;
        ts_printf(__FILE__, __LINE__, "bcheck6100 : %s", &bcheck6100[0]);
        for (i = 0;i < 4;i++) {
          if (bcheck6100[i]) tscb->bCheckflag = 1;
        }
      }
      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_SRST_RESULT, 0, READ_FROM_DRIVE);
        if (!rc) {
          memmove((Byte *)&dwSetupPageAddress, &tscb->bSectorBuffer[0], sizeof(dwSetupPageAddress));
          ts_printf(__FILE__, __LINE__, "SRSTRESULT dwSetupPageAddress memmove : %0xh", dwSetupPageAddress);
        } else {
          ts_printf(__FILE__, __LINE__, "SRST RESULT address getting Error dwSetupPageAddress : %0xh", dwSetupPageAddress);
          break;
        }
      } else {
        rc = queryDriveMemoryControlBlock(tscb, KEY_SRST_RESULT, &dmcb);
        if (rc) {
          ts_printf(__FILE__, __LINE__, "Error - query dmcb");
          tseb->isFatalError = 1;
          return 1;
        }
        dwSetupPageAddress = dmcb->dwAddress;
      }

      if (tscb->bDrivePORrequestflag == 0) {
        wExpectedPageIndex = 1;
      }
      bStateNow = GET_DRIVE_RAWDATA_FSM_wait_till_srst_result_ready_on_drive_memory;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_wait_till_srst_result_ready_on_drive_memory:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "5  GET_DRIVE_RAWDATA_FSM_wait_till_srst_result_ready_on_drive_memory");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      rc = getDriveRawdataStatusAndControl(tscb);
      if (rc) {
        break;
      }
      if (tscb->bDriveSkip_SRST_result) {
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump;
        break;
      }

      if (!isDriveRawdataReady(tscb)) {
        ts_printf(__FILE__, __LINE__, "Error3 - reporting flag turned off %02xh", tscb->wDriveStatusAndControlFlag);
        tseb->isDriveRawdataDumpReportingFlagError = 1;
        return 1;
      } else if (isDriveRawdataSetupError(tscb)) {
        if ((tscb->wDriveEchoVersion == 0) && (bporRecoveryCount == 0)) { /* PTB,PTC and Por count == 0 */
          tscb->bDrivePORrequestflag = 1;
          rc = turnDrivePowerSupplyOff(tscb);
          if (!rc) {
            rc = turnDrivePowerSupplyOn(tscb);
          }
          if (rc) {
            ts_printf(__FILE__, __LINE__, "POR Error!!");
            tseb->isFatalError = 1;
            return rc;
          } else {
            bporRecoveryCount++;
            break;
          }
        }
        ts_printf(__FILE__, __LINE__, "Error - rawdata setup %02xh (GET_DRIVE_RAWDATA_FSM_wait_till_srst_result_ready_on_drive_memory)(GOD3)", tscb->wDriveStatusAndControlFlag);
        tseb->isDriveRawdataDumpDriveError = 1;
        bStateNow = GET_DRIVE_RAWDATA_FSM_finalize;
        bassignedEC = 1;
      } else if (!isDriveRawdataSetupReady(tscb)) {
        break;
      }
      bStateNow = GET_DRIVE_RAWDATA_FSM_dump_srst_result;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_dump_srst_result:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "6  GET_DRIVE_RAWDATA_FSM_dump_srst_result");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      if (tscb->bDriveSrstuartflag & IS_UARTFLAG_BURSTMODE) { /* Burst mode transfer */
        if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
          /* Determine Reporting page number, if total page number is over maxbuffer(128 defined config) , reporting page number is 128(MAX) */
          /* if total page number is under max(128),reporting page number is total page */
          if ((tscb->dwDriveResultTotalSizeByte - tscb->dwDriveResultDumpSizeByte) > (tscb->wDriveMaxbufferpagesize * DRIVE_SECTOR_SIZE_BYTE)) {
            wSetupPageNumber = tscb->wDriveMaxbufferpagesize;  /* tscb->wDriveMaxbufferpagesize is input from config file (it is 128) */
          } else {
            wSetupPageNumber = (tscb->dwDriveResultTotalSizeByte - tscb->dwDriveResultDumpSizeByte) / DRIVE_SECTOR_SIZE_BYTE;  /* prepare read result size except dumped data size */
          }
          dwSetupPageSizeByte = wSetupPageNumber * DRIVE_SECTOR_SIZE_BYTE;
        } else {
          /* get size of Legacy burst page */
          rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_NUMBER_OF_SETUP_PAGE, 0, READ_FROM_DRIVE);
          if (rc) {
            break;
          }
          memmove((Byte *)&wSetupPageNumber, &tscb->bSectorBuffer[0], sizeof(wSetupPageNumber));
          dwSetupPageSizeByte = wSetupPageNumber * DRIVE_SECTOR_SIZE_BYTE;
          /* total page size check */
          if ((tscb->dwDriveResultDumpSizeByte + dwSetupPageSizeByte) > tscb->dwDriveResultTotalSizeByte) {
            dwSetupPageSizeByte = tscb->dwDriveResultTotalSizeByte - tscb->dwDriveResultDumpSizeByte;
          }
        }
      } else {
        dwSetupPageSizeByte = DRIVE_SECTOR_SIZE_BYTE; /* Non burst mode */
        wSetupPageNumber = 1;
      }

      /* dump data */
      rc = readRawdataFromDrive(dwSetupPageAddress, wSetupPageNumber, wExpectedPageIndex, tscb); /* wExpectedPageIndex is for PTB/PTC only. */
      if ((tscb->wDriveEchoVersion == 0) && (rc == 99)) { /* Skip reading Rawdata to Expect page PTB,PTC */
        bStateNow = GET_DRIVE_RAWDATA_FSM_check_drive_error_for_srst_result;
        break;
      }
      brawdataErrFlag = rc; /* if read rawdata success , this errflag will be 0 */
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Rawdata Read Error");
        break;  /* read again */
      }

      /* check page index */
      wCheckEndPageIndex = wExpectedPageIndex + wSetupPageNumber;
      wCheckPageOffset = 0;
      while (wExpectedPageIndex < wCheckEndPageIndex) {
        tseb->isDriveRawdataDumpPageIndexError = 0;
        memcpyWith16bitLittleEndianTo16bitConversion(&wActualPageIndex, &tscb->bSectorBuffer[4 + wCheckPageOffset], sizeof(wActualPageIndex));
        if (wExpectedPageIndex != wActualPageIndex) {
          ts_printf(__FILE__, __LINE__, "page index mismatch!!!! expect=%xh actual=%xh (GOD3)", wExpectedPageIndex, wActualPageIndex);
          tseb->isDriveRawdataDumpPageIndexError = 1;
          brawdataErrFlag = 1;
          break;
        }
        wExpectedPageIndex++;
        wCheckPageOffset += DRIVE_SECTOR_SIZE_BYTE;
      }
      if (brawdataErrFlag) {
        break;
      } else if (tscb->wDriveStatusAndControlFlag & IS_RETRY_REQUEST) {
        brawdataErrFlag = 0;
        bRetryCount = 0;
        rc = clearDriveRawdataRetryRequest(tscb);
        if (rc) {
          tseb->isDriveWriteRetryRequestError = 1;
          ts_printf(__FILE__, __LINE__, "Clear Rawdata Retry Request Write Error1 (GOD3)");
          break;
        }
        tseb->isDriveWriteRetryRequestError = 0;
        break;
      }
      /* report to host */
      tscb->dwDriveResultDumpSizeByte += dwSetupPageSizeByte;
      ts_printf(__FILE__, __LINE__, "dumpedSrstResultSizeByte=%d, totalSrstResultSizeByte=%d", tscb->dwDriveResultDumpSizeByte, tscb->dwDriveResultTotalSizeByte);
      reportHostTestData(tscb, KEY_SRST_RESULT, &tscb->bSectorBuffer[0], dwSetupPageSizeByte);

      bStateNow = GET_DRIVE_RAWDATA_FSM_check_drive_error_for_srst_result;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_check_drive_error_for_srst_result:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "7  GET_DRIVE_RAWDATA_FSM_check_drive_error_for_srst_result");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      bStateNow = GET_DRIVE_RAWDATA_FSM_request_next_page_for_srst_result;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_request_next_page_for_srst_result:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "8  GET_DRIVE_RAWDATA_FSM_request_next_page_for_srst_result");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "*Check5- reporting flag %02xh", tscb->wDriveStatusAndControlFlag);
      rc = getDriveRawdataStatusAndControl(tscb);
      if (rc) {
        break;
      }
      if (tscb->bDriveSkip_SRST_result) {
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump;
        break;
      }

      /* branch next state for rawdata/pes/mcs */
      if (tscb->dwDriveResultDumpSizeByte < tscb->dwDriveResultTotalSizeByte) {
        rc =  setDriveRawdataTransferComplete(tscb);
        if (rc) {
          break;
        }
        bStateNow = GET_DRIVE_RAWDATA_FSM_wait_till_srst_result_ready_on_drive_memory; /* state 5 */
      } else if (isDrivePesReady(tscb)) {
        rc =  setDriveRawdataTransferComplete(tscb);
        if (rc) {
          break;
        }
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_parameters_for_pes;
      } else if (isDriveMcsReady(tscb)) {
        rc =  setDriveRawdataTransferComplete(tscb);
        if (rc) {
          break;
        }
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_parameters_for_mcs;
      } else {
        bStateNow = GET_DRIVE_RAWDATA_FSM_finalize;
      }
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_get_parameters_for_pes:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "9  GET_DRIVE_RAWDATA_FSM_get_parameters_for_pes");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      rc = getDriveRawdataStatusAndControl(tscb);  /* flag check */
      if (rc) {
        break;
      }
      if (tscb->bDriveSkip_SRST_result) {
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump;
        break;
      }

      if (!isDriveRawdataReady(tscb)) {
        ts_printf(__FILE__, __LINE__, "Error2 - reporting flag turned off %02xh", tscb->wDriveStatusAndControlFlag);
        tseb->isDriveRawdataDumpReportingFlagError = 1;
        return 1;
      } else if (isDriveRawdataSetupError(tscb)) {
        ts_printf(__FILE__, __LINE__, "Error - rawdata setup %02xh (GET_DRIVE_RAWDATA_FSM_get_parameters_for_pes) (GOD3)", tscb->wDriveStatusAndControlFlag);
        tseb->isDriveRawdataDumpDriveError = 1;
        return 1;
      } else if (!isDriveRawdataSetupReady(tscb)) {
        break;
      }

      rc = getDriveParametersForPesDump(tscb);  /* get Page numger of PES  MAX 255Page */
      if (rc) {
        break;
      }

      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_PES_DATA, 0, READ_FROM_DRIVE); /* "PESDATAUART        " */
        if (!rc) {
          memmove((Byte *)&dwSetupPageAddress, &tscb->bSectorBuffer[0], sizeof(dwSetupPageAddress));
          ts_printf(__FILE__, __LINE__, "PESDATAUART dwSetupPageAddress memmove : %x", dwSetupPageAddress);
        } else {
          ts_printf(__FILE__, __LINE__, "PESDATAUART address getting Error dwSetupPageAddress : %0xh", dwSetupPageAddress);
          break;
        }
      } else {
        rc = queryDriveMemoryControlBlock(tscb, KEY_PES_DATA, &dmcb);
        if (rc) {
          ts_printf(__FILE__, __LINE__, "Error - query dmcb");
          tseb->isFatalError = 1;
          return 1;
        }
        dwSetupPageAddress = dmcb->dwAddress;
      }

      wReportHead = 0;
      if (tscb->wDriveEchoVersion >= 2) { /* JPT,MRN,MRK,MRS or later */
        if (tscb->wDrivePesPage[wReportHead] > 255) {
          ts_printf(__FILE__, __LINE__, "Error - head %d Pes page is too large %d.Pes page is 255(MAX) (GOD3)", wReportHead, tscb->wDrivePesPage[wReportHead]);
          tseb->isDriveRawdataDumpDriveError = 1;
          return 1;
        }

        tscb->wDrivePesDataCount = tscb->wDrivePesPage[wReportHead];
      }
      bStateNow = GET_DRIVE_RAWDATA_FSM_wait_till_pes_ready_on_drive_memory;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_wait_till_pes_ready_on_drive_memory:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "10  GET_DRIVE_RAWDATA_FSM_wait_till_pes_ready_on_drive_memory");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      rc = getDriveRawdataStatusAndControl(tscb);
      if (rc) {
        break;
      }
      if (tscb->bDriveSkip_SRST_result) {
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump;
        break;
      }

      if (!isDriveRawdataReady(tscb)) { /* check under reporting mode */
        ts_printf(__FILE__, __LINE__, "Error1 - reporting flag turned off %02xh", tscb->wDriveStatusAndControlFlag);
        tseb->isDriveRawdataDumpReportingFlagError = 1;
        return 1;
      } else if (isDriveRawdataSetupError(tscb)) {
        ts_printf(__FILE__, __LINE__, "Error - rawdata setup %02xh (GET_DRIVE_RAWDATA_FSM_wait_till_pes_ready_on_drive_memory)(GOD3)", tscb->wDriveStatusAndControlFlag);
        tseb->isDriveRawdataDumpDriveError = 1;
        return 1;
      } else if (!isDriveRawdataSetupReady(tscb)) {
        break;
      }
      bStateNow = GET_DRIVE_RAWDATA_FSM_dump_pes;
      break;


      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_dump_pes:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "11  GET_DRIVE_RAWDATA_FSM_dump_pes");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later *//*   2010.10.19 chaged for MAXPAGE is 128->256
        if (tscb->wDrivePesDataCount > tscb->wDriveMaxbufferpagesize) {
          wSetupPageNumber = tscb->wDriveMaxbufferpagesize;
        } else {
          wSetupPageNumber = tscb->wDrivePesDataCount;
        }
        dwSetupPageSizeByte = wSetupPageNumber * DRIVE_SECTOR_SIZE_BYTE;
*/
        wSetupPageNumber = tscb->wDrivePesDataCount; /* changed here */
        dwSetupPageSizeByte = wSetupPageNumber * DRIVE_SECTOR_SIZE_BYTE; /* changed here */
      } else {
        /* Legacy PES report procedure */
        /* get header info */
        rc = getDriveParametersForPesHeader(tscb);
        if (rc) {
          break;
        }

        /* get size */
        rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_NUMBER_OF_SETUP_PAGE, 0, READ_FROM_DRIVE); /* No use? this KEY is from gaia.cfg */
        if (rc) {
          break;
        }
        memmove((Byte *)&wSetupPageNumber, &tscb->bSectorBuffer[0], sizeof(wSetupPageNumber));
        dwSetupPageSizeByte = wSetupPageNumber * DRIVE_SECTOR_SIZE_BYTE + DRIVE_PES_HEADER_SIZE;

        /* total page size check */
        if (((tscb->dwDrivePesDumpSizeByte + dwSetupPageSizeByte) > tscb->dwDrivePesTotalSizeByte)) {
          ts_printf(__FILE__, __LINE__, "pes page mismatch!!!! total=%d byte, dumped=%d, next=%d (GOD3)", tscb->dwDrivePesTotalSizeByte, tscb->dwDrivePesDumpSizeByte, dwSetupPageSizeByte);
          tseb->isDriveRawdataDumpTotalPageError = 1;
          return 1;
        }
      }

      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        rc = readRawdataFromDrive(dwSetupPageAddress, wSetupPageNumber, 0, tscb);
        ts_printf(__FILE__, __LINE__, "PESDATAUART dwSetupPageAddress : %x", dwSetupPageAddress);
      } else {
        rc = driveIoCtrl(dwSetupPageAddress, (dwSetupPageSizeByte - DRIVE_PES_HEADER_SIZE), &tscb->bSectorBuffer[DRIVE_PES_HEADER_SIZE], READ_DRIVE_MEMORY_PROTOCOL, uplb);
      }
      brawdataErrFlag = rc;
      if (rc) {
        break;
      } else if (tscb->wDriveStatusAndControlFlag & IS_RETRY_REQUEST) {
        bRetryCount = 0;
        rc = clearDriveRawdataRetryRequest(tscb);
        if (rc) {
          tseb->isDriveWriteRetryRequestError = 1;
          ts_printf(__FILE__, __LINE__, "Clear Rawdata Retry Request Write Error2 (GOD3)");
          break;
        }
        tseb->isDriveWriteRetryRequestError = 0;
        break;
      }

      if (tscb->wDriveEchoVersion < 2) { /*  Legacy PES report procedure */
        /* set pes header (hard coding to prevend memory leak by unexpected data size change) */
        memmove(&tscb->bSectorBuffer[0], (Byte *)&tscb->dwDrivePesCylinder, 4);
        memmove(&tscb->bSectorBuffer[4], (Byte *)&tscb->bDrivePesHeadNumber, 1);
        memmove(&tscb->bSectorBuffer[5], (Byte *)&tscb->bDrivePesTemperature, 1);
        memmove(&tscb->bSectorBuffer[6], (Byte *)&tscb->wDrivePesDataCount, 2);
        memmove(&tscb->bSectorBuffer[8], (Byte *)&tscb->wDrivePesSpindleSpeed, 2);
        memmove(&tscb->bSectorBuffer[10], (Byte *)&tscb->wDrivePesNumberOfServoSector, 2);
        memset(&tscb->bSectorBuffer[12], 0x00, 4);
      }

      /* report to host */
      tscb->dwDrivePesDumpSizeByte += dwSetupPageSizeByte;  /* sent data size */
      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        ts_printf(__FILE__, __LINE__, "head %d dumpedPesSizeByte=%d", wReportHead, tscb->dwDrivePesDumpSizeByte);
        memcpy(bLabel, KEY_PES_DATA, sizeof(KEY_PES_DATA));
        if (wReportHead < 10) {
          bLabel[13] = wReportHead + 0x30;  /* 0 - 9 */
        } else if (wReportHead < 16) {
          bLabel[13] = wReportHead + 0x37;  /* A - F */
        } else {
          bLabel[13] = '?';
        }
        reportHostTestData(tscb, bLabel, &tscb->bSectorBuffer[0], dwSetupPageSizeByte);
      } else {
        ts_printf(__FILE__, __LINE__, "dumpedPesSizeByte=%d, totalPesSizeByte=%d", tscb->dwDrivePesDumpSizeByte, tscb->dwDrivePesTotalSizeByte);
        reportHostTestData(tscb, KEY_PES_DATA, &tscb->bSectorBuffer[0], dwSetupPageSizeByte);
      }

      bStateNow = GET_DRIVE_RAWDATA_FSM_check_drive_error_for_pes;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_check_drive_error_for_pes:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "12  GET_DRIVE_RAWDATA_FSM_check_drive_error_for_pes");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      bStateNow = GET_DRIVE_RAWDATA_FSM_request_next_page_for_pes;
      break;


      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_request_next_page_for_pes:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "13  GET_DRIVE_RAWDATA_FSM_request_next_page_for_pes");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      rc = getDriveRawdataStatusAndControl(tscb);
      if (rc) {
        break;
      }
      if (tscb->bDriveSkip_SRST_result) {
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump;
        break;
      }

      /* branch next state for rawdata/pes/mcs */
      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        /*        if((wReportHead < (tscb->wDriveHeadNumber - 1)) || (tscb->wDrivePesDataCount > wSetupPageNumber)) {*/
        if ((wReportHead < (tscb->wDriveHeadNumber - 1))) {
          /* rc =  setDriveRawdataTransferComplete(tscb);*/  /* flag control */
          /*          if (rc) {
                      break;
                    }
          */
          /*          if(tscb->wDrivePesDataCount > wSetupPageNumber){
                      tscb->wDrivePesDataCount -= wSetupPageNumber;
                    }else{
          */
          rc =  setDriveRawdataTransferComplete(tscb);  /* change */
          wReportHead++; /* next head data */
          tscb->wDrivePesDataCount = tscb->wDrivePesPage[wReportHead];
          /*          }
          */
          bStateNow = GET_DRIVE_RAWDATA_FSM_wait_till_pes_ready_on_drive_memory;
          break;
        }
      } else { /*  Legacy PES report procedure */
        if (tscb->dwDrivePesDumpSizeByte < tscb->dwDrivePesTotalSizeByte) {
          rc =  setDriveRawdataTransferComplete(tscb);
          if (rc) {
            break;
          }
          bStateNow = GET_DRIVE_RAWDATA_FSM_wait_till_pes_ready_on_drive_memory;
          break;
        }
      }
      if (isDriveMcsReady(tscb)) {
        rc =  setDriveRawdataTransferComplete(tscb);
        if (rc) {
          break;
        }
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_parameters_for_mcs;
      } else {
        bStateNow = GET_DRIVE_RAWDATA_FSM_finalize;
      }
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_get_parameters_for_mcs:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "14  GET_DRIVE_RAWDATA_FSM_get_parameters_for_mcs");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      if (tscb->wDriveEchoVersion >= 3) { /* MRN or later */
        rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_DRIVE_PARAMETER_BYTE, 0, READ_FROM_DRIVE);   /* 2010.11.9 for MNR */
        if (!rc) {
          memmove((Byte *)&tscb->dwDriveParametricSizeByte, &tscb->bSectorBuffer[0], sizeof(wSetupPageNumber));
          ts_printf(__FILE__, __LINE__, "DriveParametric Byte %d", tscb->dwDriveParametricSizeByte);
        } else {
          ts_printf(__FILE__, __LINE__, "DriveParametric Byte read error");
          break;
        }
      }
      /*wSetupPageNumber = (tscb->dwDriveParametricSizeByte/512)+1;    */
      wSetupPageNumber = (tscb->dwDriveParametricSizeByte / 512) + wParamPageNumber;    /** Size -> Page Head is 1Sector -> 2Sector(only Mobile) DT is 1Sector 2010.12.07add*/
      /*wSetupPageNumber = (tscb->dwDriveParametricSizeByte/512)+tscb->bParametricHeaderPage;*/
      if (tscb->dwDriveParametricSizeByte % 512 != 0) {
        wSetupPageNumber++;
      }
      tscb->dwDriveParametricDumpSizeByte = 0;
      ts_printf(__FILE__, __LINE__, "DriveParametric size %d byte", tscb->dwDriveParametricSizeByte);

      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        /* get address putting rawdata(Drive parametric data) */
        rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_DRIVE_PARAMETER, 0, READ_FROM_DRIVE);

        if (!rc) { /* set address */
          memmove((Byte *)&dwSetupPageAddress, &tscb->bSectorBuffer[0], sizeof(dwSetupPageAddress));
          ts_printf(__FILE__, __LINE__, "DRIVEPARAMETER dwSetupPageAddress memmove : %0xh", dwSetupPageAddress);
        } else {
          ts_printf(__FILE__, __LINE__, "DRIVEPARAMETER address getting Error dwSetupPageAddress : %0xh", dwSetupPageAddress);
          break;
        }
      } else {
        rc = queryDriveMemoryControlBlock(tscb, KEY_DRIVE_PARAMETER, &dmcb);
        if (rc) {
          ts_printf(__FILE__, __LINE__, "Error - query dmcb");
          tseb->isFatalError = 1;
          return 1;
        }
        dwSetupPageAddress = dmcb->dwAddress;
      }

      bStateNow = GET_DRIVE_RAWDATA_FSM_wait_till_mcs_ready_on_drive_memory;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_wait_till_mcs_ready_on_drive_memory:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "15  GET_DRIVE_RAWDATA_FSM_wait_till_mcs_ready_on_drive_memory");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      rc = getDriveRawdataStatusAndControl(tscb);
      if (rc) {
        break;
      }
      if (tscb->bDriveSkip_SRST_result) {
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump;
        break;
      }

      if (!isDriveRawdataReady(tscb)) {
        ts_printf(__FILE__, __LINE__, "Error4 - reporting flag turned off %02xh", tscb->wDriveStatusAndControlFlag);
        tseb->isDriveRawdataDumpReportingFlagError = 1;
        return 1;
      } else if (isDriveRawdataSetupError(tscb)) {
        ts_printf(__FILE__, __LINE__, "Error - rawdata setup %02xh (GET_DRIVE_RAWDATA_FSM_wait_till_mcs_ready_on_drive_memory)(GOD3)", tscb->wDriveStatusAndControlFlag);
        tseb->isDriveRawdataDumpDriveError = 1;
        return 1;
      } else if (!isDriveRawdataSetupReady(tscb)) {
        break;
      }

      bStateNow = GET_DRIVE_RAWDATA_FSM_dump_mcs;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_dump_mcs:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "16  GET_DRIVE_RAWDATA_FSM_dump_mcs");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      /* get size */
      /*tscb->dwDriveParametricSizeByte --> total byte number without header */
      /*wSetupPageNumber = 1;*/
      dwSetupPageSizeByte = wSetupPageNumber * DRIVE_SECTOR_SIZE_BYTE;  /* Byte Number */

      /* total page size check */

      if (wSetupPageNumber > 255) {
        ts_printf(__FILE__, __LINE__, "DriveParametric page over flow!!!! %d page.MAX is 255. (GOD3)", wSetupPageNumber);
        tseb->isDriveRawdataDumpTotalPageError = 1;
        return 1;
      }

      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        rc = readRawdataFromDrive(dwSetupPageAddress, wSetupPageNumber, 0, tscb);  /* read page block */
        ts_printf(__FILE__, __LINE__, "DRIVEPARAMETER dwSetupPageAddress : %x", dwSetupPageAddress);
      } else {
        rc = driveIoCtrl(dwSetupPageAddress, dwSetupPageSizeByte, &tscb->bSectorBuffer[0], READ_DRIVE_MEMORY_PROTOCOL, uplb);
      }
      brawdataErrFlag = rc;
      if (rc) {
        break;
      } else if (tscb->wDriveStatusAndControlFlag & IS_RETRY_REQUEST) {
        bRetryCount = 0;
        rc = clearDriveRawdataRetryRequest(tscb);
        if (rc) {
          tseb->isDriveWriteRetryRequestError = 1;
          ts_printf(__FILE__, __LINE__, "Clear Rawdata Retry Reqest Write Error (GOD3)");
          break;
        }
        tseb->isDriveWriteRetryRequestError = 0;
        break;
      }

      /* report to host */
      tscb->dwDriveParametricDumpSizeByte += dwSetupPageSizeByte;
      ts_printf(__FILE__, __LINE__, "dumpedMcsSizeByte=%d, ParametricSizeByte=%d", tscb->dwDriveParametricDumpSizeByte, tscb->dwDriveParametricSizeByte);
      reportHostTestData(tscb, KEY_DRIVE_PARAMETER, &tscb->bSectorBuffer[0], dwSetupPageSizeByte);

      bStateNow = GET_DRIVE_RAWDATA_FSM_check_drive_error_for_mcs;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_check_drive_error_for_mcs:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "17  GET_DRIVE_RAWDATA_FSM_check_drive_error_for_mcs");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      bStateNow = GET_DRIVE_RAWDATA_FSM_request_next_page_for_mcs;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_request_next_page_for_mcs:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "18  GET_DRIVE_RAWDATA_FSM_request_next_page_for_mcs");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      rc = getDriveRawdataStatusAndControl(tscb);
      if (rc) {
        break;
      }
      if (tscb->bDriveSkip_SRST_result) {
        bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump;
        break;
      }

      bStateNow = GET_DRIVE_RAWDATA_FSM_finalize;

      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_finalize:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "19  GET_DRIVE_RAWDATA_FSM_finalize");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      brawdataErrFlag = 0;
      bRetryCount = 0;
      if (tscb->isNoDriveFinalized) {
        ts_printf(__FILE__, __LINE__, "no drive finalized ");

      } else {
        if (tscb->isReSrstProhibitControl) {
          rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_RE_SRST_PROHIBIT, 0, READ_FROM_DRIVE);
          if (rc) {
            break;
          }
          memmove((Byte *)&tscb->wDriveReSrstProhibit, &tscb->bSectorBuffer[0], sizeof(tscb->wDriveReSrstProhibit));
          ts_printf(__FILE__, __LINE__, "re-srst prohibit %04xh", tscb->wDriveReSrstProhibit);
        } else {
          ts_printf(__FILE__, __LINE__, "no re-srst prohibit control (GOD3)");
        }

        if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
          rc =  setDriveRawdataTransferComplete(tscb);
        }
      }
      ts_printf(__FILE__, __LINE__, "complete get rawdata!!!!!!!!!! (%s)", tscb->bDriveSerialNumber);

      bStateNow = GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump;
      break;

      /* -------------------------------------------------------------- */
    case GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "20  GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      if (tscb->wDriveEchoVersion < 3) {  //for JPT
        rc = getDriveReportingData(tscb, bStateNow);
        if (rc) {
          break;
        }
      }

      if (tscb->wDriveEchoVersion == 0) {/* PTB */
        rc =  setDriveRawdataTransferComplete(tscb);
      }
      for (bRetryCount = 0;bRetryCount < RAWDATA_RETRY_CONT;bRetryCount++) {
        ts_sleep_slumber(1);
        rc = getDriveRawdataStatusAndControl(tscb);
        if (rc) {
          continue;
        }
        ts_printf(__FILE__, __LINE__, "tscb->wDriveStatusAndControlFlag %x, return value %x", tscb->wDriveStatusAndControlFlag, rc);
        if (tscb->wDriveStatusAndControlFlag & (IS_UNDER_REPORTING_MODE | IS_RAWDATA_READY)) {
          continue;
        }
        ts_printf(__FILE__, __LINE__, "Success rawdata Flag clear !(%s)", tscb->bDriveSerialNumber);
        bassignedEC = 1;
        break;
      }
      if (bRetryCount >= RAWDATA_RETRY_CONT) {
        if (bassignedEC == 0) {
          tseb->isDriveRawdataFinalizeError = 1;
          return 1;
        }
      }



      if (tscb->wDriveEchoVersion >= 2) { /* JPT or later */
        if (tscb->wDriveEchoVersion >= 3) { /* JPT or later */
          rc = getDriveReportingData(tscb, bStateNow);
          if (rc) {
            break;
          }
        }

        dwReportEndTime = getTestScriptTimeInSec();
        ts_printf(__FILE__, __LINE__, "Report End Time %d", dwReportEndTime);
        dwReportTime = dwReportEndTime - dwReportTime;
        if (tscb->bPartFlag == FINAL_PART) {
          memmove(&tscb->bSectorBuffer[0], "FINAL ", 6);
        } else {
          memmove(&tscb->bSectorBuffer[0], "SRST  ", 6);
        }
        memmove(&tscb->bSectorBuffer[6], (Byte *)&dwReportTime, 4);
        reportHostTestData(tscb, KEY_TIME_LOG, &tscb->bSectorBuffer[0], 10);

        ts_sleep_slumber(60);
        tscb->isDriveRawdataComplete = 1;
        return 0;
      }

      ts_sleep_slumber(60);
      tscb->isDriveRawdataComplete = 1;
      return 0;

      /* -------------------------------------------------------------- */
    default:
      /* -------------------------------------------------------------- */
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "21  default");
      ts_printf(__FILE__, __LINE__, "*********************************************************************************");
      ts_printf(__FILE__, __LINE__, "invalid state!!");
      tseb->isFatalError = 1;
      return 1;
    }
  }

  ts_printf(__FILE__, __LINE__, "shall not be here!!");
  tseb->isFatalError = 1;
  return 1;
}
