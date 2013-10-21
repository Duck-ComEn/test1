#include "ts.h"

/**
 * <pre>
 * Description:
 *   Return pointer to top of given field in given text file. If given field
 *   is not found, return NULL. Field separator is ' ' (space).
 *
 *   For example, if fieldNumber is 3, it returns pointer to top of "5000".
 *
 *   0           1                 2                3       4        5
 *   |           |                 |                |       |        |
 *   V           V                 V                V       V        V
 *   SeqStep0    330               041:00:00        5000    12000    0
 *
 * Arguments:
 *   *textFile - Test file in ASCII text format
 *   fieldNumber - Your desired field number
 * </pre>
 *****************************************************************************/
Byte *getFieldInScriptFile(Byte *textFile, Dword desiredNumOfField)
{
  Dword i = 0;
  Dword currentNumOfField = 0;
  Byte isQuotationMark = 0;

  for (currentNumOfField = 0, i = 0 ; ; currentNumOfField++) {
    /* search a letter (A to Z or a to z) or a digit (0 to 9)

           |
           V
           SeqMode                  1

           |
           V
           "MFGID               "   0x000001D0
    */
    for ( ; ; i++) {
      if (0 == isprint(textFile[i])) {
        return NULL;
      } else if (textFile[i] == '\r') {
        return NULL;
      } else if (textFile[i] == '\a') {
        return NULL;
      } else if ((textFile[i] == '/') && (textFile[i+1] == '/')) {
        return NULL;
      } else if (isgraph(textFile[i])) {
        break;
      }
    }
    if (textFile[i] == '\"') {
      isQuotationMark = 1;
    } else {
      isQuotationMark = 0;
    }

    /* if it is desired field, break */
    if (currentNumOfField == desiredNumOfField) {
      break;
    }

    /* skip to this field

                  |
                  V
           SeqMode                1

                                 |
                                 V
           "MFGID               "   0x000001D0
    */
    for (i++ ; ; i++) {
      if (0 == isprint(textFile[i])) {
        return NULL;
      } else if (textFile[i] == '\r') {
        return NULL;
      } else if (textFile[i] == '\a') {
        return NULL;
      } else if ((textFile[i] == '/') && (textFile[i+1] == '/')) {
        return NULL;
      } else if (isQuotationMark && (textFile[i] == '\"')) {
        i++;
        break;
      } else if (!isQuotationMark && (0 == isgraph(textFile[i]))) {
        break;
      }
    }
  }

  return &textFile[i];
}

/**
 * <pre>
 * Description:
 *   Return pointer to top of given line number in given script file.
 * Arguments:
 *   *scriptFile - Test script file in ASCII text format
 *   lineNumber - Your desired line number
 * </pre>
 *****************************************************************************/
Byte *searchLineTopInScriptFile(Byte *scriptFile, Dword lineNumber)
{
  Dword i = 0;
  Byte *lineTop = scriptFile;
  Byte *p = NULL;

  for (i = 0 ; ; i++) {
    for (;;) {
      /* Search first signature */
      p = getFieldInScriptFile(lineTop, 0);
      if (p != NULL) {
        lineTop = p;
        break;
      }

      /* Move next line */
      p = strchr(lineTop, (Dword)'\n');
      if (p == NULL) {
        return NULL;
      } else {
        lineTop = p + 1;
      }
    }

    if (i == lineNumber) {
      /* Found */
      break;
    } else {
      /* Move next line */
      p = strchr(lineTop, (Dword)'\n');
      if (p == NULL) {
        return NULL;
      } else {
        lineTop = p + 1;
      }
    }
  }

  return lineTop;
}

/**
 * <pre>
 * Description:
 *   Return pointer to top of given line number in given script file.
 * Arguments:
 *   *scriptFile - Test script file in ASCII text format
 *   lineNumber - Your desired line number
 * </pre>
 *****************************************************************************
  2010/05/12 Support dwTestUartBaudrate,wDriveEchoVersion and wDriveECClength Y.Enomoto
  2010/05/26 Support ReportFlag = 3 Get before Rawdata Dump & Report after Rawdata Dump Y.Enomoto
 *****************************************************************************/
Byte setParameterIntoTestScriptControlBlock(Byte *textFile, TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_RESULT_SAVE_BLOCK *trsb = &tscb->testResultSaveBlock[tscb->wTestResultSaveBlockCounter];

  Byte rc;
  Byte *arg1 = NULL;
  Byte *arg2 = NULL;
  Byte *arg3 = NULL;
  Byte *arg4 = NULL;
  Byte *arg5 = NULL;
  Byte *arg6 = NULL;
  Byte *arg7 = NULL;
  Byte *arg8 = NULL;
  Dword i = 0;

  arg1 = getFieldInScriptFile(&textFile[0], 0);
  arg2 = getFieldInScriptFile(&textFile[0], 1);
  arg3 = getFieldInScriptFile(&textFile[0], 2);
  arg4 = getFieldInScriptFile(&textFile[0], 3);
  arg5 = getFieldInScriptFile(&textFile[0], 4);
  arg6 = getFieldInScriptFile(&textFile[0], 5);
  arg7 = getFieldInScriptFile(&textFile[0], 6);
  arg8 = getFieldInScriptFile(&textFile[0], 7);

  if ((arg1 == NULL) || (arg2 == NULL)) {
    ts_printf(__FILE__, __LINE__, "error");
    return 1;
  }

  if (MATCH == memcmp("TEST_PARAM", arg1, strlen("TEST_PARAM"))) {

    if ((arg1 == NULL) || (arg2 == NULL) || (arg3 == NULL)) {
      ts_printf(__FILE__, __LINE__, "Error - invalid field");
      return 1;

    } else if (MATCH == memcmp("bTestResultFileName", arg2, strlen("bTestResultFileName"))) {
      rc = textInQuotationMark2String(arg3, &tscb->bTestResultFileName[0], sizeof(tscb->bTestResultFileName) - 1);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid field");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "bTestResultFileName,\"%s\"", tscb->bTestResultFileName);

    } else if (MATCH == memcmp("bTestPfTableFileName", arg2, strlen("bTestPfTableFileName"))) {
      rc = textInQuotationMark2String(arg3, &tscb->bTestPfTableFileName[0], sizeof(tscb->bTestPfTableFileName) - 1);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid field");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "bTestPfTableFileName,\"%s\"", tscb->bTestPfTableFileName);

    } else if (MATCH == memcmp("bTestMfgId", arg2, strlen("bTestMfgId"))) {
      rc = textInQuotationMark2String(arg3, &tscb->bTestMfgId[0], sizeof(tscb->bTestMfgId) - 1);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid field");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "bTestMfgId,\"%s\"", tscb->bTestMfgId);

    } else if (MATCH == memcmp("bTestLabelMfgIdOffset", arg2, strlen("bTestLabelMfgIdOffset"))) {
      Dword dw = ts_strtoul(arg3, NULL, 0);
      if ((sizeof(tscb->bTestMfgId) - 1 - 1) < dw) { /* 2 = maximum offset + string termination '\0' */
        ts_printf(__FILE__, __LINE__, "Error - invalid field");
        return 1;
      }
      if (UCHAR_MAX < dw) {
        ts_printf(__FILE__, __LINE__, "Error - invalid field");
        return 1;
      }
      tscb->bTestLabelMfgIdOffset = dw;
      ts_printf(__FILE__, __LINE__, "bTestLabelMfgIdOffset, %d", tscb->bTestLabelMfgIdOffset);

    } else if (MATCH == memcmp("bTestSerialNumber", arg2, strlen("bTestSerialNumber"))) {
      rc = textInQuotationMark2String(arg3, &tscb->bTestSerialNumber[0], sizeof(tscb->bTestSerialNumber) - 1);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid field");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "bTestSerialNumber,\"%s\"", tscb->bTestSerialNumber);

    } else if (MATCH == memcmp("bTestDefaultBadHeadTable", arg2, strlen("bTestDefaultBadHeadTable"))) {
      for (i = 0 ; ; i++) {
        arg4 = getFieldInScriptFile(&arg3[0], i);
        if (arg4 == NULL) {
          break;
        }
        if (i >= sizeof(tscb->bTestDefaultBadHeadTable)) {
          ts_printf(__FILE__, __LINE__, "field too long");
          return 1;
        }
        tscb->bTestDefaultBadHeadTable[i] = 0xff & ts_strtoul(arg4, NULL, 0);
      }
      ts_printf(__FILE__, __LINE__, "bTestDefaultBadHeadTable");
      putBinaryDataDump((Byte *)&tscb->bTestDefaultBadHeadTable, 0, sizeof(tscb->bTestDefaultBadHeadTable));

    } else if (MATCH == memcmp("bTestUartProtocol", arg2, strlen("bTestUartProtocol"))) {
      Dword dw = ts_strtoul(arg3, NULL, 0);
      if (UART_PROTOCOL_IS_ARM < dw) {
        ts_printf(__FILE__, __LINE__, "Error - invalid field");
        return 1;
      }
      if (UCHAR_MAX < dw) {
        ts_printf(__FILE__, __LINE__, "Error - invalid field");
        return 1;
      }
      tscb->bTestUartProtocol = dw;
      ts_printf(__FILE__, __LINE__, "bTestUartProtocol, %d", tscb->bTestUartProtocol);

    } else if (MATCH == memcmp("dwTestUartBaudrate", arg2, strlen("dwTestUartBaudrate"))) {
      tscb->dwTestUartBaudrate = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwTestUartBaudrate, %ld", tscb->dwTestUartBaudrate);

    } else if (MATCH == memcmp("dwDriveTestElapsedTimeSec", arg2, strlen("dwDriveTestElapsedTimeSec"))) {
      rc = hhhhmmssTimeFormat2Sec(arg3, &tscb->dwDriveTestElapsedTimeSec);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid format");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwDriveTestElapsedTimeSec, %d", tscb->dwDriveTestElapsedTimeSec);

    } else if (MATCH == memcmp("dwDriveFinalTestElapsedTimeSec", arg2, strlen("dwDriveFinalTestElapsedTimeSec"))) {  /* For 2Step Final */
      rc = hhhhmmssTimeFormat2Sec(arg3, &tscb->dwDriveFinalTestElapsedTimeSec);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid format");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwDriveTestElapsedTimeSec, %d", tscb->dwDriveTestElapsedTimeSec);

    } else if (MATCH == memcmp("dwDriveTestIdentifyTimeoutSec", arg2, strlen("dwDriveTestIdentifyTimeoutSec"))) {
      rc = hhhhmmssTimeFormat2Sec(arg3, &tscb->dwDriveTestIdentifyTimeoutSec);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid format");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwDriveTestIdentifyTimeoutSec, %d", tscb->dwDriveTestIdentifyTimeoutSec);

    } else if (MATCH == memcmp("dwDriveTestStatusPollingIntervalTimeSec", arg2, strlen("dwDriveTestStatusPollingIntervalTimeSec"))) {
      rc = hhhhmmssTimeFormat2Sec(arg3, &tscb->dwDriveTestStatusPollingIntervalTimeSec);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid format");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwDriveTestStatusPollingIntervalTimeSec, %d", tscb->dwDriveTestStatusPollingIntervalTimeSec);

    } else if (MATCH == memcmp("dwDriveTestCutoffTimeoutSec", arg2, strlen("dwDriveTestCutoffTimeoutSec"))) {
      rc = hhhhmmssTimeFormat2Sec(arg3, &tscb->dwTimeoutSecFromStartToSRSTEndCutoff);  /* E/C6110 Timer */
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid format");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwDriveTestCutoffTimeoutSec, %d", tscb->dwTimeoutSecFromStartToSRSTEndCutoff);
    } else if (MATCH == memcmp("dwDriveTestTimeoutSec", arg2, strlen("dwDriveTestTimeoutSec"))) {
      rc = hhhhmmssTimeFormat2Sec(arg3, &tscb->dwTimeoutSecFromStartToSRSTEnd);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid format");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwDriveTestTimeoutSec, %d", tscb->dwTimeoutSecFromStartToSRSTEnd);
    } else if (MATCH == memcmp("dwDriveTestRawdataDumpTimeoutSec", arg2, strlen("dwDriveTestRawdataDumpTimeoutSec"))) {
      rc = hhhhmmssTimeFormat2Sec(arg3, &tscb->dwDriveTestRawdataDumpTimeoutSec);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid format");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwDriveTestRawdataDumpTimeoutSec, %d", tscb->dwDriveTestRawdataDumpTimeoutSec);
    } else if (MATCH == memcmp("dwDriveFinalTestCutoffTimeoutSec", arg2, strlen("dwDriveFinalTestCutoffTimeoutSec"))) {  /* 2 STEP for FINAL */
      rc = hhhhmmssTimeFormat2Sec(arg3, &tscb->dwTimeoutSecFromStartToFinalEndCutoff);  /* E/C6110 Timer */
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid format");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwDriveFinalTestCutoffTimeoutSec, %d", tscb->dwTimeoutSecFromStartToFinalEndCutoff);
    } else if (MATCH == memcmp("dwDriveFinalTestTimeoutSec", arg2, strlen("dwDriveFinalTestTimeoutSec"))) {    /* 2 STEP for FINAL */
      rc = hhhhmmssTimeFormat2Sec(arg3, &tscb->dwTimeoutSecFromStartToFinalEnd);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "Error - invalid format");
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwDriveFinalTestTimeoutSec, %d", tscb->dwTimeoutSecFromStartToFinalEnd);

    } else if (MATCH == memcmp("dwDriveAmbientTemperatureTarget", arg2, strlen("dwDriveAmbientTemperatureTarget"))) {
      tscb->dwDriveAmbientTemperatureTarget = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDriveAmbientTemperatureTarget, %d", tscb->dwDriveAmbientTemperatureTarget);

    } else if (MATCH == memcmp("dwDriveAmbientTemperaturePositiveRampRate", arg2, strlen("dwDriveAmbientTemperaturePositiveRampRate"))) {
      tscb->dwDriveAmbientTemperaturePositiveRampRate = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDriveAmbientTemperaturePositiveRampRate, %d", tscb->dwDriveAmbientTemperaturePositiveRampRate);

    } else if (MATCH == memcmp("dwDriveAmbientTemperatureNegativeRampRate", arg2, strlen("dwDriveAmbientTemperatureNegativeRampRate"))) {
      tscb->dwDriveAmbientTemperatureNegativeRampRate = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDriveAmbientTemperatureNegativeRampRate, %d", tscb->dwDriveAmbientTemperatureNegativeRampRate);

    } else if (MATCH == memcmp("dwDrivePowerSupply5V", arg2, strlen("dwDrivePowerSupply5V"))) {
      tscb->dwDrivePowerSupply5V = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDrivePowerSupply5V, %d", tscb->dwDrivePowerSupply5V);

    } else if (MATCH == memcmp("dwDrivePowerSupply12V", arg2, strlen("dwDrivePowerSupply12V"))) {
      tscb->dwDrivePowerSupply12V = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDrivePowerSupply12V, %d", tscb->dwDrivePowerSupply12V);

    } else if (MATCH == memcmp("dwDrivePowerRiseTime5V", arg2, strlen("dwDrivePowerRiseTime5V"))) {
      tscb->dwDrivePowerRiseTime5V = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDrivePowerRiseTime5V, %d", tscb->dwDrivePowerRiseTime5V);

    } else if (MATCH == memcmp("dwDrivePowerRiseTime12V", arg2, strlen("dwDrivePowerRiseTime12V"))) {
      tscb->dwDrivePowerRiseTime12V = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDrivePowerRiseTime12V, %d", tscb->dwDrivePowerRiseTime12V);

    } else if (MATCH == memcmp("dwDrivePowerIntervalFrom5VTo12V", arg2, strlen("dwDrivePowerIntervalFrom5VTo12V"))) {
      tscb->dwDrivePowerIntervalFrom5VTo12V = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDrivePowerIntervalFrom5VTo12V, %d", tscb->dwDrivePowerIntervalFrom5VTo12V);

    } else if (MATCH == memcmp("dwDrivePowerOnWaitTime", arg2, strlen("dwDrivePowerOnWaitTime"))) {
      tscb->dwDrivePowerOnWaitTime = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDrivePowerOnWaitTime, %d", tscb->dwDrivePowerOnWaitTime);

    } else if (MATCH == memcmp("dwDrivePowerOffWaitTime", arg2, strlen("dwDrivePowerOffWaitTime"))) {
      tscb->dwDrivePowerOffWaitTime = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDrivePowerOffWaitTime, %d", tscb->dwDrivePowerOffWaitTime);

    } else if (MATCH == memcmp("dwDriveUartPullup", arg2, strlen("dwDriveUartPullup"))) {
      tscb->dwDriveUartPullup = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "dwDriveUartPullup, %d", tscb->dwDriveUartPullup);

    } else if (MATCH == memcmp("isNoPlugCheck", arg2, strlen("isNoPlugCheck"))) {
      tscb->isNoPlugCheck = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isNoPlugCheck, %d", tscb->isNoPlugCheck);

    } else if (MATCH == memcmp("isNoPowerControl", arg2, strlen("isNoPowerControl"))) {
      tscb->isNoPowerControl = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isNoPowerControl, %d", tscb->isNoPowerControl);

    } else if (MATCH == memcmp("isNoPowerOffAfterTest", arg2, strlen("isNoPowerOffAfterTest"))) {
      tscb->isNoPowerOffAfterTest = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isNoPowerOffAfterTest, %d", tscb->isNoPowerOffAfterTest);

    } else if (MATCH == memcmp("isNoDriveFinalized", arg2, strlen("isNoDriveFinalized"))) {
      tscb->isNoDriveFinalized = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isNoDriveFinalized, %d", tscb->isNoDriveFinalized);

    } else if (MATCH == memcmp("isBadHeadReport", arg2, strlen("isBadHeadReport"))) {
      tscb->isBadHeadReport = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isBadHeadReport, %d", tscb->isBadHeadReport);

    } else if (MATCH == memcmp("isMultiGrading", arg2, strlen("isMultiGrading"))) {
      tscb->isMultiGrading = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isMultiGrading, %d", tscb->isMultiGrading);

    } else if (MATCH == memcmp("isDriveVoltageDataReport", arg2, strlen("isDriveVoltageDataReport"))) {
      tscb->isDriveVoltageDataReport = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isDriveVoltageDataReport, %d", tscb->isDriveVoltageDataReport);

    } else if (MATCH == memcmp("isReSrstProhibitControl", arg2, strlen("isReSrstProhibitControl"))) {
      tscb->isReSrstProhibitControl = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isReSrstProhibitControl, %d", tscb->isReSrstProhibitControl);

    } else if (MATCH == memcmp("isTpiSearch", arg2, strlen("isTpiSearch"))) {
      tscb->isTpiSearch = (Byte)ts_strtoul(arg3, NULL, 0);
      tscb->wTpiSearchOffset = (Word)ts_strtoul(arg4, NULL, 0);
      tscb->wTpiSearchSize = (Word)ts_strtoul(arg5, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isTpiSearch, %d, %xh, %d", tscb->isTpiSearch, tscb->wTpiSearchOffset, tscb->wTpiSearchSize);

    } else if (MATCH == memcmp("isForceDriveTestCompFlagOn", arg2, strlen("isForceDriveTestCompFlagOn"))) {
      tscb->isForceDriveTestCompFlagOn = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isForceDriveTestCompFlagOn, %d", tscb->isForceDriveTestCompFlagOn);

    } else if (MATCH == memcmp("isForceDriveUnload", arg2, strlen("isForceDriveUnload"))) {
      tscb->isForceDriveUnload = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isForceDriveUnload, %d", tscb->isForceDriveUnload);

    } else if (MATCH == memcmp("isTesterLogReportEnable", arg2, strlen("isTesterLogReportEnable"))) {
      tscb->isTesterLogReportEnable = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isTesterLogReportEnable, %d", tscb->isTesterLogReportEnable);

    } else if (MATCH == memcmp("isTesterLogMirrorToStdout", arg2, strlen("isTesterLogMirrorToStdout"))) {
      tscb->isTesterLogMirrorToStdout = (Byte)ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isTesterLogMirrorToStdout, %d", tscb->isTesterLogMirrorToStdout);

    } else if (MATCH == memcmp("dwTesterLogSizeKByte", arg2, strlen("dwTesterLogSizeKByte"))) {
      tscb->dwTesterLogSizeKByte = ts_strtoul(arg3, NULL, 0);
      if (tscb->dwTesterLogSizeKByte > MAX_LOG_SIZE_IN_KBYTE) {
        ts_printf(__FILE__, __LINE__, "Error - too large value", tscb->dwTesterLogSizeKByte);
        return 1;
      }
      ts_printf(__FILE__, __LINE__, "dwTesterLogSizeKByte, %d", tscb->dwTesterLogSizeKByte);

    } else if (MATCH == memcmp("isFakeErrorCode", arg2, strlen("isFakeErrorCode"))) {
      tscb->isFakeErrorCode = (Byte)ts_strtoul(arg3, NULL, 0);
      tscb->dwFakeErrorCode = ts_strtoul(arg4, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isFakeErrorCode,%d,%xh", tscb->isFakeErrorCode, tscb->dwFakeErrorCode);

    } else if (MATCH == memcmp("isTestMode", arg2, strlen("isTestMode"))) {
      tscb->isTestMode = ts_strtoul(arg3, NULL, 0);
      tscb->dwTestModeParam1 = ts_strtoul(arg4, NULL, 0);
      tscb->dwTestModeParam2 = ts_strtoul(arg5, NULL, 0);
      tscb->dwTestModeParam3 = ts_strtoul(arg6, NULL, 0);
      ts_printf(__FILE__, __LINE__, "isTestMode,%d,%d,%d,%d", tscb->isTestMode, tscb->dwTestModeParam1, tscb->dwTestModeParam2, tscb->dwTestModeParam3);

    } else if (MATCH == memcmp("wDriveEchoVersion", arg2, strlen("wDriveEchoVersion"))) {
      tscb->wDriveEchoVersion = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "wDriveEchoVersion, %d", tscb->wDriveEchoVersion);

    } else if (MATCH == memcmp("wDriveECClength", arg2, strlen("wDriveECClength"))) {
      tscb->wDriveECClength = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "wDriveECClength, %d", tscb->wDriveECClength);

    } else if (MATCH == memcmp("wDriveMaxbufferpagesize", arg2, strlen("wDriveMaxbufferpagesize"))) {
      tscb->wDriveMaxbufferpagesize = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "wDriveMaxbufferpagesize, %d", tscb->wDriveMaxbufferpagesize);

    } else if (MATCH == memcmp("wDriveForceStopPFcode", arg2, strlen("wDriveForceStopPFcode"))) {
      for (i = 0 ; i < MAX_NUMBER_OF_FORCE_STOP_PF_CODE  ; i++) {
        arg4 = getFieldInScriptFile(&arg3[0], i);
        if (arg4 == NULL) {
          break;
        }
        tscb->wDriveForceStopPFcode[i] = 0xffff & ts_strtoul(arg4, NULL, 0);
      }
      ts_printf(__FILE__, __LINE__, "wDriveForceStopPFcode");
      putBinaryDataDump((Byte *)&tscb->wDriveForceStopPFcode, 0, MAX_NUMBER_OF_FORCE_STOP_PF_CODE*2);

    } else if (MATCH == memcmp("wReadBlocksize", arg2, strlen("wReadBlocksize"))) {
      wReadBlocksize = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "wReadBlocksize, 0x%x", wReadBlocksize);
    } else if (MATCH == memcmp("bTesterflag", arg2, strlen("bTesterflag"))) {
      bTesterflag = ts_strtoul(arg3, NULL, 0);
      if (bTesterflag == OPTIMUS) {/* 1:Optimus 0:Neptune*/
        ts_printf(__FILE__, __LINE__, "bTesterflag, %d OPTIMUS TESTER", bTesterflag);
      } else {
        ts_printf(__FILE__, __LINE__, "bTesterflag, %d NEPTUNE TESTER", bTesterflag);
      }
    } else if (MATCH == memcmp("wParamPageNumber", arg2, strlen("wParamPageNumber"))) {
      wParamPageNumber = ts_strtoul(arg3, NULL, 0);
      ts_printf(__FILE__, __LINE__, "wParamPageNumber, %d", wParamPageNumber);

    } else {
      ts_printf(__FILE__, __LINE__, "Error - unknown TESTPARAM keyword, Check Configfile.not defined Keyword in config file");
      reportHostTestMessage(tscb, "TEST PARAM undefined");
      return 1;

    }
  } else if (MATCH == memcmp("DRIVE_INFO", arg1, strlen("DRIVE_INFO"))) {
    DRIVE_MEMORY_CONTROL_BLOCK *dmcb = NULL;

    if ((arg1 == NULL) || (arg2 == NULL) || (arg3 == NULL) || (arg4 == NULL) || (arg5 == NULL) || (arg6 == NULL) || (arg7 == NULL) || (arg8 == NULL)) {
      ts_printf(__FILE__, __LINE__, "Error - invalid field");
      return 1;
    }

    for (i = 0 ; i < (sizeof(tscb->driveMemoryControlBlock) / sizeof(tscb->driveMemoryControlBlock[0])) ; i++) {
      if (tscb->driveMemoryControlBlock[i].bName[0] == '\0') {
        dmcb = &tscb->driveMemoryControlBlock[i];
        break;
      }
    }
    if (NULL == dmcb) {
      ts_printf(__FILE__, __LINE__, "Error - drive info table overflow");
      return 1;
    }

    rc = textInQuotationMark2String(arg2, &dmcb->bName[0], sizeof(dmcb->bName) - 1);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "Error - invalid field");
      return 1;
    }
    if (strlen(&dmcb->bName[0]) != (sizeof(dmcb->bName) - 1)) {
      ts_printf(__FILE__, __LINE__, "Error - id name invalid");
      return 1;
    }
    dmcb->dwAddress = ts_strtoul(arg3, NULL, 0);
    dmcb->dwAddressOffset = ts_strtoul(arg4, NULL, 0);
    dmcb->wSize = (Word)ts_strtoul(arg5, NULL, 0);
    dmcb->bAccessFlag = (Byte)ts_strtoul(arg6, NULL, 0);
    dmcb->bEndianFlag = (Byte)ts_strtoul(arg7, NULL, 0);
    if ((arg8[0] == '\'') && isgraph(arg8[1]) && (arg8[2] == '\'')) {
      dmcb->bReportFlag = arg8[1];
      ts_printf(__FILE__, __LINE__, "\"%s\",%08xh,%08xh,%dbytes,%d,%d,\'%c\'", &dmcb->bName[0], dmcb->dwAddress, dmcb->dwAddressOffset, dmcb->wSize, dmcb->bAccessFlag, dmcb->bEndianFlag, dmcb->bReportFlag);
    } else {
      dmcb->bReportFlag = (Byte)ts_strtoul(arg8, NULL, 0);
      ts_printf(__FILE__, __LINE__, "\"%s\",%08xh,%08xh,%dbytes,%d,%d,%d", &dmcb->bName[0], dmcb->dwAddress, dmcb->dwAddressOffset, dmcb->wSize, dmcb->bAccessFlag, dmcb->bEndianFlag, dmcb->bReportFlag);
      if ((dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP) || (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_SRST) || (dmcb->bReportFlag == IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_FINAL)) {
        trsb->bHeaderNameaddress = &dmcb->bName[0];
        trsb->bSaveaddressTop = ts_qalloc(dmcb->wSize);
        trsb->dwSavelengthInByte++;
        tscb->wTestResultSaveBlockCounter++;
        ts_printf(__FILE__, __LINE__, "allocNo %d Name %s  SaveAddressTop %08xh" , tscb->wTestResultSaveBlockCounter, &dmcb->bName[0], trsb->bSaveaddressTop);
      }
    }
    if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_SRST_TOP_ADDRESS)) {
      tscb->dwDriveSrstTopAddress = dmcb->dwAddress;
    }

  } else {
    ts_printf(__FILE__, __LINE__, "Error - unknown DRIVE PARAM keyword,Check Configfile.not defined keyword in config. file.");
    reportHostTestMessage(tscb, "DRIVE INFO undefined");
    putBinaryDataDump((Byte *)arg1, 0, 128);
    return 1;

  }

  return 0;
}

/**
 * <pre>
 * Description:
 *   Parses given script file and set it into test script control block.
 *   Return value is 0 if no error. Otherwise, non-zero value.
 * Arguments:
 *   *tscb - The name say it all
 *   *scriptFile - Test script file in ASCII text format
 *   Byte bFileSize - Size of config file [019KPE]
 * </pre>
 *****************************************************************************/
Byte parseTestScriptFile(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *scriptFile, Word wFileSize)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 0;
  Byte *lineTop = NULL;
  Byte isFirstLoop = 1;
  Word wCount = 0;

  ts_printf(__FILE__, __LINE__, "parseTestScriptFile");

  /* set default value */
  tscb->dwTesterLogSizeKByte = DEFAULT_LOG_SIZE_IN_KBYTE;
  tscb->dwTesterLogSizeKByte = DEFAULT_LOG_SIZE_IN_KBYTE;
  tscb->isTesterLogMirrorToStdout = DEFAULT_LOG_MIRROR_TO_STDOUT;
  tscb->wDriveReSrstProhibit = IS_RE_SRST_NG;
  tscb->wTestResultSaveBlockCounter = 0;
  tscb->dwDriveUartPullup = UART_PULLUP_VOLTAGE_DEFAULT;   //add for F/W 9000 2013.06.12 y.katayama

  /* Find control code [019KPE] */
  for(wCount = 0; wCount < wFileSize; wCount++) {
    /* Change EOF or CR to LF */
    if( (scriptFile[wCount] == 0x0D) || (scriptFile[wCount] == 0x1A) ) {
      scriptFile[wCount] = 0x0A;
    /* If there is Tab in file, finish as fail */
    } else if (scriptFile[wCount] == 0x09) {
      ts_printf(__FILE__, __LINE__, "Read config file fail. There is TAB in file.");
      tseb->isFatalError = 1;
      return 1;
    }
    /* In order free memory, sleep 1sec every 4KB data read */
    if((wCount % 4096) == 0){
      ts_sleep_partial(1);
    }
  }

  for (lineTop = &scriptFile[0], isFirstLoop = 1 ; ; ) {
    if (isFirstLoop) {
      isFirstLoop = 0;
      lineTop = searchLineTopInScriptFile(lineTop, 0);
    } else {
      lineTop = searchLineTopInScriptFile(lineTop, 1);
    }

    if (lineTop == NULL) {
      break;
    }

    rc = setParameterIntoTestScriptControlBlock(lineTop, tscb);
    if (rc) {
      tseb->isFatalError = 1;
      break;
    }
  }

  resetTestScriptLog(tscb->dwTesterLogSizeKByte * 1024, tscb->isTesterLogMirrorToStdout);

  if (rc) {
    ts_printf(__FILE__, __LINE__, "parseTestScriptFile error %50.50s", lineTop);
  }

  if (0 == strlen(&tscb->bTestResultFileName[0])) {
    ts_printf(__FILE__, __LINE__, "TestResultFileName error %s", &tscb->bTestResultFileName);
    tseb->isFatalError = 1;
    rc = 1;
  }
  if (0 == strlen(&tscb->bTestPfTableFileName[0])) {
    ts_printf(__FILE__, __LINE__, "TestPfTableFileName error %s", &tscb->bTestPfTableFileName);
    tseb->isFatalError = 1;
    rc = 1;
  }

  ts_printf(__FILE__, __LINE__, "parseTestScriptFile rc=%d", rc);
  return rc;
}

/**
 * <pre>
 * Description:
 *   Query pointer to drive memory control block in given test script control
 *   block.  Return value is 0 if success. Otherwise, non-zero value.
 * Arguments:
 *   *tscb - The name say it all
 *   *bKeyword - Search keyword
 *   **tmcb - The name say it all
 * </pre>
 *****************************************************************************/
Byte queryDriveMemoryControlBlock(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bKeyword, DRIVE_MEMORY_CONTROL_BLOCK **dmcb)
{
  Byte rc = 1;
  Dword i;

  for (i = 0 ; i < (sizeof(tscb->driveMemoryControlBlock) / sizeof(tscb->driveMemoryControlBlock[0])) ; i++) {
    if (MATCH == strcmp(&bKeyword[0], &tscb->driveMemoryControlBlock[i].bName[0])) {
      *dmcb = &tscb->driveMemoryControlBlock[i];
      rc = 0;
      break;
    }
  }

  return rc;
}

/**
 * <pre>
 * Description:
 *   Query pointer to drive data address from dmcd->bName (Keyword)
 *   block.  Return value is data pointer if Return = NULL No data store address.
 * Arguments:
 *   *tscb - The name say it all
 *   *dmcb - DRIVE_MEMORY_CONTROL_BLOCK
 * </pre>
 *****************************************************************************/
Byte *queryDriveDataAddress(TEST_SCRIPT_CONTROL_BLOCK *tscb, DRIVE_MEMORY_CONTROL_BLOCK *dmcb)
{
  Byte *bData_address;

  bData_address = (Byte *)NULL;
  if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_SRST_TOP_ADDRESS)) {
    bData_address = (Byte *) & tscb->dwDriveSrstTopAddress;
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_SERIAL_NUMBER)) {
    bData_address = (Byte *) & tscb->bDriveSerialNumber[0];
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_MFG_ID)) {
    bData_address = (Byte *) & tscb->bDriveMfgId[0];
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_MICRO_REVISION_LEVEL)) {
    bData_address = (Byte *) & tscb->bDriveMicroLevel[0];
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_NUMBER_OF_SERVO_SECTOR)) {
    bData_address = (Byte *) & tscb->wDrivePesNumberOfServoSector;
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_HEAD_TABLE)) {
    bData_address = (Byte *) & tscb->bDriveHeadTable;
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_TPI_INFORMATION)) {
    bData_address = (Byte *) & tscb->bDriveBPITPITable;
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_HEAD_NUMBER)) {
    bData_address = (Byte *) & tscb->wDriveHeadNumber;
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_SPINDLE_RPM)) {
    bData_address = (Byte *) & tscb->wDrivePesSpindleSpeed;
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_GRADING_FLAG)) { /* for Grading */
    bData_address = (Byte *) & tscb->dwDriveGradingFlag;
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_GRADING_FLAG_WF)) { /* for Grading of water fall*/
    bData_address = (Byte *) & tscb->dwDriveGradingWaterFallFlag;
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_OFFSET_HEAD)) { /* Offse Head 0x414 */
    bData_address = (Byte *) & tscb->bOffsetHead;
  } else if (MATCH == strcmp(&dmcb->bName[0], (Byte *)KEY_STATUS_AND_CONTROL_B)) { /* Offse Head 0x465 */
    bData_address = (Byte *) & tscb->bStatusControl;
  }

  return bData_address;
}

/**
 * <pre>
 * Description:
 *   Free mamory for reporting save data
 * Arguments:
 *   *tscb - The name say it all
 *   *dmcb - DRIVE_MEMORY_CONTROL_BLOCK
 * </pre>
 *****************************************************************************/
void freeMemoryForSaveData(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_RESULT_SAVE_BLOCK *trsb = &tscb->testResultSaveBlock[tscb->wTestResultSaveBlockCounter];

  Word wfreeCount = tscb->wTestResultSaveBlockCounter;

  ts_printf(__FILE__, __LINE__, "wfreeCount %d", wfreeCount);
  while (wfreeCount > 0) {
    ts_printf(__FILE__, __LINE__, "allocNo %d SaveAddressTop %08xh" , wfreeCount, trsb->bSaveaddressTop);
    ts_qfree((Byte *)trsb->bSaveaddressTop);
    wfreeCount--;
    trsb = &tscb->testResultSaveBlock[wfreeCount];
  }

}

/**
 * <pre>
 * Description:
*   get SWITCH TABLE for grading process (2010.10.08 for grading process)
 * Arguments:
 *   *tscb - The name say it all
 * </pre>
 *****************************************************************************/
void getSWtable(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte bcell[5];
  Byte FileName[12];
  /*Byte buffer[256];*/

  sprintf(bcell, "%04d", MY_CELL_NO + 1);
  strcpy(FileName, SWITCH_TABLE_NAME); /* making file name "gdinfo.0001"  0001 means cell number*/
  strcat(FileName, bcell);            /* This file locate testpc/uartcode/urMB019?/testcase/gdinfo.0109 , on debug without host.*/
  /* Actual production , testpc/mnt/copc/ */
  ts_printf(__FILE__, __LINE__, "FileName is %s ", FileName);
  downloadFile(tscb, FileName, &tscb->bSwitchTable[0], 256); /* gdinfo file will be supply from PC */
  /*memcpy(&buffer[0], &tscb->bSwitchTable[0],256);*/
  ts_printf(__FILE__, __LINE__, "get grading flag ");

  /* ts_printf(__FILE__, __LINE__, "down load success Grading Table = %s",&buffer[0]); */ /* hung potential */
  if (tseb->isFatalError == 1) {
    tseb->isGradingError = NO_SW_TBL;
    tseb->isFatalError = 0;
    ts_printf(__FILE__, __LINE__, "Switching Table filedownload error");
  }
  /*ts_printf(__FILE__, __LINE__, "Grading Table = %s",tscb->bSwitchTable);*/

  return;
}

