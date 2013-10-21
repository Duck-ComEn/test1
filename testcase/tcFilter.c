#include "tc.h"
#include "../testscript/ts.h"

const unsigned long ts_crc32_table[] = {0x0, 0x77073096, 0xee0e612c, 0x990951ba, 0x76dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0xedb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x9b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x1db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x6b6b51f, 0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0xf00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x86d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x3b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x4db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0xd6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0xa00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x26d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x5005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0xcb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0xbdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};
/* ---------------------------------------------------------------------- */
unsigned long ts_crc32(const unsigned char *buffer, size_t byte_count)
/* ---------------------------------------------------------------------- */
{
  unsigned long result;

  result = 0xFFFFFFFFUL;
  do {
    result = (result >> CHAR_BIT) ^ ts_crc32_table[(unsigned char)result ^ *buffer++];
  } while (--byte_count > 0);
  result = result ^ 0xFFFFFFFFUL;

  return result;
}

const Byte bBinary2AsciiOfPrintf[] = "0123456789ABCDEF";
/* ---------------------------------------------------------------------- */
void putBinaryDataDumpOfPrintf(Byte *dwPhysicalAddress, Dword dwLogicalAddress, Dword dwSize)
/* ---------------------------------------------------------------------- */
{
  Byte bLineBuffer[64 + 1];
  Byte bByteBuffer;
  Dword dwOffsetToBinary = 0;
  Dword dwOffsetToAscii = 48;
  Dword i;

  memset(&bLineBuffer[0], ' ', sizeof(bLineBuffer));
  bLineBuffer[sizeof(bLineBuffer)] = '\0';

  for (i = 0 ; i < dwSize ; i++) {
    /* Get One Byte */
    bByteBuffer = *(Byte *)((Dword)dwPhysicalAddress + i);

    /* Set Binary Format */
    bLineBuffer[dwOffsetToBinary] = bBinary2AsciiOfPrintf[(bByteBuffer & 0xF0) >> 4];
    bLineBuffer[dwOffsetToBinary + 1] = bBinary2AsciiOfPrintf[bByteBuffer & 0x0F];
    bLineBuffer[dwOffsetToBinary + 2] = ' ';

    /* Set ASCII Format */
    if ((bByteBuffer < ' ') || ('~' < bByteBuffer)) {
      bLineBuffer[dwOffsetToAscii] = '.';
    } else {
      bLineBuffer[dwOffsetToAscii] = bByteBuffer;
    }
    bLineBuffer[dwOffsetToAscii + 1] = '\0';


    if ((dwOffsetToAscii + 1) == 64) {
      /* Sweep out buffer */
      dwOffsetToBinary = 0;
      dwOffsetToAscii = 48;
      printf("  [%08lx] %s\n", dwLogicalAddress + i - 15, &bLineBuffer[0]);
      memset(bLineBuffer, ' ', 64);
      bLineBuffer[sizeof(bLineBuffer)] = '\0';

    } else {
      /* Move foward */
      dwOffsetToBinary += 3;
      dwOffsetToAscii++;
    }
  }

  /* Sweep out buffer */
  if (dwOffsetToAscii != 48) {
    printf("  [%08lx] %s\n", dwLogicalAddress + i - (i % 16), &bLineBuffer[0]);
  }
}

/* ---------------------------------------------------------------------- */
void tcLogFilter(char *filename)
/* ---------------------------------------------------------------------- */
{
  FILE *fp = NULL;
  long filesize = 0;
  unsigned char *fileimage = NULL;
  TEST_SCRIPT_CONTROL_BLOCK __tscb__;
  TEST_SCRIPT_CONTROL_BLOCK *tscb = &__tscb__;
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  DRIVE_MEMORY_CONTROL_BLOCK *dmcb = &tscb->driveMemoryControlBlock[0];
  UART_PERFORMANCE_LOG_BLOCK *uplb = &tscb->uartPerformanceLogBlock[0];
  CELL_CARD_INVENTORY_BLOCK *ccib = &tscb->cellCardInventoryblock;
  Byte isLittleEndianRawdata = 0;
  int i = 0;

  /* open log file */
  fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("file open error\n");
    return;
  }
  fseek(fp, 0, SEEK_END);
  filesize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (filesize < 100) {
    fclose(fp);
    printf("file size too small\n");
    return;
  }

  /* copy data */
  fileimage = malloc(filesize + 32);
  if (fileimage == NULL) {
    fclose(fp);
    printf("file read error\n");
    return;
  }
  fread(fileimage, 1, filesize, fp);
  fclose(fp);

  /* parsing */
  printf("report-id,crc32,length\n");
  for (i = 300 ; i < filesize ; ) {
    Dword length = 0;
    Dword crc32 = 0;

    if (!ts_isprint_mem(&fileimage[i], 20)) {
      printf("invalid dir\n");
      break;
    }

    length = 0x000000ff & fileimage[i+20];
    length += 0x0000ff00 & (fileimage[i+21] << 8);
    length += 0x00ff0000 & (fileimage[i+22] << 16);
    length += 0xff000000 & (fileimage[i+23] << 24);
    crc32 = ts_crc32(&fileimage[i+24], length);

    printf("[%20.20s],0x%08x,%ld\n", &fileimage[i], (unsigned int)crc32, length);

    if (MATCH == memcmp(&fileimage[i], KEY_TESTER_UART_LOG, 20)) {
      Dword j = 0;
      memmove((unsigned char *)tscb, &fileimage[i+24], length);
      /* endian swap */
      if (tscb->dwSignature == TSCB_SIGNATURE) {
        /* little endian -> do nothing */
        isLittleEndianRawdata = 1;
      } else {
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwSignature, (Byte *)&tscb->dwSignature, sizeof(tscb->dwSignature));

        memcpyWith16bitBigEndianTo16bitConversion(&tscb->wHostID, (Byte *)&tscb->wHostID, sizeof(tscb->wHostID));
        memcpyWith16bitBigEndianTo16bitConversion(&tscb->wCurrentTempInHundredth, (Byte *)&tscb->wCurrentTempInHundredth, sizeof(tscb->wCurrentTempInHundredth));
        memcpyWith16bitBigEndianTo16bitConversion(&tscb->wCurrentV5InMilliVolts, (Byte *)&tscb->wCurrentV5InMilliVolts, sizeof(tscb->wCurrentV5InMilliVolts));
        memcpyWith16bitBigEndianTo16bitConversion(&tscb->wCurrentV12InMilliVolts, (Byte *)&tscb->wCurrentV12InMilliVolts, sizeof(tscb->wCurrentV12InMilliVolts));

        memcpyWith16bitBigEndianTo16bitConversion(&tscb->wDriveReSrstProhibit, (Byte *)&tscb->wDriveReSrstProhibit, sizeof(tscb->wDriveReSrstProhibit));

        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveResultTotalSizeByte, (Byte *)&tscb->dwDriveResultTotalSizeByte, sizeof(tscb->dwDriveResultTotalSizeByte));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveResultDumpSizeByte, (Byte *)&tscb->dwDriveResultDumpSizeByte, sizeof(tscb->dwDriveResultDumpSizeByte));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePesTotalSizeByte, (Byte *)&tscb->dwDrivePesTotalSizeByte, sizeof(tscb->dwDrivePesTotalSizeByte));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePesDumpSizeByte, (Byte *)&tscb->dwDrivePesDumpSizeByte, sizeof(tscb->dwDrivePesDumpSizeByte));
//        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveMcsTotalSizeByte, (Byte *)&tscb->dwDriveMcsTotalSizeByte, sizeof(tscb->dwDriveMcsTotalSizeByte));      2010.11.04 y.katayama
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveParametricSizeByte, (Byte *)&tscb->dwDriveParametricSizeByte, sizeof(tscb->dwDriveParametricSizeByte));
//        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveMcsDumpSizeByte, (Byte *)&tscb->dwDriveMcsDumpSizeByte, sizeof(tscb->dwDriveMcsDumpSizeByte));         2010.11.04 y.katayama
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveParametricDumpSizeByte, (Byte *)&tscb->dwDriveParametricDumpSizeByte, sizeof(tscb->dwDriveParametricDumpSizeByte));

        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePesCylinder, (Byte *)&tscb->dwDrivePesCylinder, sizeof(tscb->dwDrivePesCylinder));
        memcpyWith16bitBigEndianTo16bitConversion(&tscb->wDrivePesDataCount, (Byte *)&tscb->wDrivePesDataCount, sizeof(tscb->wDrivePesDataCount));
        memcpyWith16bitBigEndianTo16bitConversion(&tscb->wDrivePesSpindleSpeed, (Byte *)&tscb->wDrivePesSpindleSpeed, sizeof(tscb->wDrivePesSpindleSpeed));
        memcpyWith16bitBigEndianTo16bitConversion(&tscb->wDrivePesNumberOfServoSector, (Byte *)&tscb->wDrivePesNumberOfServoSector, sizeof(tscb->wDrivePesNumberOfServoSector));

        memcpyWith16bitBigEndianTo16bitConversion(&tscb->wDriveNativeErrorCode, (Byte *)&tscb->wDriveNativeErrorCode, sizeof(tscb->wDriveNativeErrorCode));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveExtendedErrorCode, (Byte *)&tscb->dwDriveExtendedErrorCode, sizeof(tscb->dwDriveExtendedErrorCode));

        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveTestElapsedTimeSec, (Byte *)&tscb->dwDriveTestElapsedTimeSec, sizeof(tscb->dwDriveTestElapsedTimeSec));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveTestIdentifyTimeoutSec, (Byte *)&tscb->dwDriveTestIdentifyTimeoutSec, sizeof(tscb->dwDriveTestIdentifyTimeoutSec));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveTestStatusPollingIntervalTimeSec, (Byte *)&tscb->dwDriveTestStatusPollingIntervalTimeSec, sizeof(tscb->dwDriveTestStatusPollingIntervalTimeSec));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTimeoutSecFromStartToSRSTEndCutoff, (Byte *)&tscb->dwTimeoutSecFromStartToSRSTEndCutoff, sizeof(tscb->dwTimeoutSecFromStartToSRSTEndCutoff));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTimeoutSecFromStartToSRSTEnd, (Byte *)&tscb->dwTimeoutSecFromStartToSRSTEnd, sizeof(tscb->dwTimeoutSecFromStartToSRSTEnd));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveTestRawdataDumpTimeoutSec, (Byte *)&tscb->dwDriveTestRawdataDumpTimeoutSec, sizeof(tscb->dwDriveTestRawdataDumpTimeoutSec));

        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveAmbientTemperatureTarget, (Byte *)&tscb->dwDriveAmbientTemperatureTarget, sizeof(tscb->dwDriveAmbientTemperatureTarget));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveAmbientTemperaturePositiveRampRate, (Byte *)&tscb->dwDriveAmbientTemperaturePositiveRampRate, sizeof(tscb->dwDriveAmbientTemperaturePositiveRampRate));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveAmbientTemperatureNegativeRampRate, (Byte *)&tscb->dwDriveAmbientTemperatureNegativeRampRate, sizeof(tscb->dwDriveAmbientTemperatureNegativeRampRate));

        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePowerSupply5V, (Byte *)&tscb->dwDrivePowerSupply5V, sizeof(tscb->dwDrivePowerSupply5V));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePowerSupply12V, (Byte *)&tscb->dwDrivePowerSupply12V, sizeof(tscb->dwDrivePowerSupply12V));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePowerRiseTime5V, (Byte *)&tscb->dwDrivePowerRiseTime5V, sizeof(tscb->dwDrivePowerRiseTime5V));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePowerRiseTime12V, (Byte *)&tscb->dwDrivePowerRiseTime12V, sizeof(tscb->dwDrivePowerRiseTime12V));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePowerIntervalFrom5VTo12V, (Byte *)&tscb->dwDrivePowerIntervalFrom5VTo12V, sizeof(tscb->dwDrivePowerIntervalFrom5VTo12V));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePowerOnWaitTime, (Byte *)&tscb->dwDrivePowerOnWaitTime, sizeof(tscb->dwDrivePowerOnWaitTime));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDrivePowerOffWaitTime, (Byte *)&tscb->dwDrivePowerOffWaitTime, sizeof(tscb->dwDrivePowerOffWaitTime));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwDriveUartPullup, (Byte *)&tscb->dwDriveUartPullup, sizeof(tscb->dwDriveUartPullup));

        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwFakeErrorCode, (Byte *)&tscb->dwFakeErrorCode, sizeof(tscb->dwFakeErrorCode));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTesterLogSizeKByte, (Byte *)&tscb->dwTesterLogSizeKByte, sizeof(tscb->dwTesterLogSizeKByte));

        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTestModeParam1, (Byte *)&tscb->dwTestModeParam1, sizeof(tscb->dwTestModeParam1));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTestModeParam2, (Byte *)&tscb->dwTestModeParam2, sizeof(tscb->dwTestModeParam2));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTestModeParam3, (Byte *)&tscb->dwTestModeParam3, sizeof(tscb->dwTestModeParam3));

        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTimeStampAtTestStart, (Byte *)&tscb->dwTimeStampAtTestStart, sizeof(tscb->dwTimeStampAtTestStart));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTimeStampAtTurnDrivePowerSupplyOn, (Byte *)&tscb->dwTimeStampAtTurnDrivePowerSupplyOn, sizeof(tscb->dwTimeStampAtTurnDrivePowerSupplyOn));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTimeStampAtIdentifyDrive, (Byte *)&tscb->dwTimeStampAtIdentifyDrive, sizeof(tscb->dwTimeStampAtIdentifyDrive));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTimeStampAtWaitDriveTestCompleted, (Byte *)&tscb->dwTimeStampAtWaitDriveTestCompleted, sizeof(tscb->dwTimeStampAtWaitDriveTestCompleted));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTimeStampAtGetDriveRawdata, (Byte *)&tscb->dwTimeStampAtGetDriveRawdata, sizeof(tscb->dwTimeStampAtGetDriveRawdata));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTimeStampAtFinalize, (Byte *)&tscb->dwTimeStampAtFinalize, sizeof(tscb->dwTimeStampAtFinalize));
        memcpyWith32bitBigEndianTo32bitConversion(&tscb->dwTimeStampAtTestEnd, (Byte *)&tscb->dwTimeStampAtTestEnd, sizeof(tscb->dwTimeStampAtTestEnd));

        for (j = 0 ; j < (sizeof(tscb->uartPerformanceLogBlock) / sizeof(tscb->uartPerformanceLogBlock[0])) ; j++) {
          memcpyWith32bitBigEndianTo32bitConversion(&uplb[j].dwNumOfCommand, (Byte *)&uplb[j].dwNumOfCommand, sizeof(uplb[j].dwNumOfCommand));
          memcpyWith32bitBigEndianTo32bitConversion(&uplb[j].dwNumOfCommandSuccess, (Byte *)&uplb[j].dwNumOfCommandSuccess, sizeof(uplb[j].dwNumOfCommandSuccess));
          memcpyWith32bitBigEndianTo32bitConversion(&uplb[j].dwTotalDataSize, (Byte *)&uplb[j].dwTotalDataSize, sizeof(uplb[j].dwTotalDataSize));
        }


        for (dmcb = &tscb->driveMemoryControlBlock[0], j = 0 ; dmcb->bName[0] != '\0' ; dmcb++, j++) {
          memcpyWith32bitBigEndianTo32bitConversion(&dmcb->dwAddress, (Byte *)&dmcb->dwAddress, sizeof(dmcb->dwAddress));
          memcpyWith32bitBigEndianTo32bitConversion(&dmcb->dwAddressOffset, (Byte *)&dmcb->dwAddressOffset, sizeof(dmcb->dwAddressOffset));
          memcpyWith16bitBigEndianTo16bitConversion(&dmcb->wSize, (Byte *)&dmcb->wSize, sizeof(dmcb->wSize));
        }
      }

      /* show tscb structure */
      printf("typedef struct test_script_control_block {\n");
      printf("  dwSignature = %lxh\n", tscb->dwSignature);
      printf("  bCellTestLibraryVersion[32] = %s\n", &tscb->bCellTestLibraryVersion[0]);
      printf("  wHostID = %xh\n", tscb->wHostID);
      printf("  bTestResultFileName[128+1] = %s\n", tscb->bTestResultFileName);
      printf("  bTestPfTableFileName[128+1] = %s\n", tscb->bTestPfTableFileName);

      printf("  wCurrentTempInHundredth = %d\n", tscb->wCurrentTempInHundredth);
      printf("  wCurrentV5InMilliVolts = %d\n", tscb->wCurrentV5InMilliVolts);
      printf("  wCurrentV12InMilliVolts = %d\n", tscb->wCurrentV12InMilliVolts);

      printf("  bTestMfgId[6+1] = %s\n", tscb->bTestMfgId);
      printf("  bTestLabelMfgIdOffset = %d\n", tscb->bTestLabelMfgIdOffset);
      printf("  bTestSerialNumber[8+1] = %s\n", tscb->bTestSerialNumber);
      printf("  bTestDefaultBadHeadTable[10] = %d %d %d %d %d %d %d %d %d %d\n", tscb->bTestDefaultBadHeadTable[0], tscb->bTestDefaultBadHeadTable[1], tscb->bTestDefaultBadHeadTable[2], tscb->bTestDefaultBadHeadTable[3], tscb->bTestDefaultBadHeadTable[4], tscb->bTestDefaultBadHeadTable[5], tscb->bTestDefaultBadHeadTable[6], tscb->bTestDefaultBadHeadTable[7], tscb->bTestDefaultBadHeadTable[8], tscb->bTestDefaultBadHeadTable[9]);
      printf("  bTestUartProtocol = %d\n", tscb->bTestUartProtocol);
      printf("  bTestUartBaudrate = %ld\n", tscb->dwTestUartBaudrate);

      printf("  isDriveIdentifyComplete = %d\n", tscb->isDriveIdentifyComplete);
      printf("  bDriveSerialNumber[8] = %8.8s\n", tscb->bDriveSerialNumber);
      printf("  bDriveMfgId[6] = %6.6s\n", tscb->bDriveMfgId);
      printf("  bDriveMfgIdOnLabel[6] = %6.6s\n", tscb->bDriveMfgIdOnLabel);
      printf("  bDriveTemperature = %d\n", tscb->bDriveTemperature);
      printf("  bDriveTemperatureMax = %d\n", tscb->bDriveTemperatureMax);
      printf("  bDriveTemperatureMin = %d\n", tscb->bDriveTemperatureMin);
      printf("  bDriveStepCount = %xh\n", tscb->bDriveStepCount);
//      printf("  bDriveStatusAndControlFlag = %xh\n", tscb->bDriveStatusAndControlFlag); 2010.11.04 y.katayama
      printf("  wDriveStatusAndControlFlag = %xh\n", tscb->wDriveStatusAndControlFlag);
      printf("  wDriveReSrstProhibit = %xh\n", tscb->wDriveReSrstProhibit);
      printf("  dwDriveResultTotalSizeByte = %ld\n", tscb->dwDriveResultTotalSizeByte);
      printf("  dwDriveResultDumpSizeByte = %ld\n", tscb->dwDriveResultDumpSizeByte);
      printf("  dwDrivePesTotalSizeByte = %ld\n", tscb->dwDrivePesTotalSizeByte);
      printf("  dwDrivePesDumpSizeByte = %ld\n", tscb->dwDrivePesDumpSizeByte);
      printf("  dwDriveMcsTotalSizeByte = %ld\n", tscb->dwDriveParametricSizeByte);
      printf("  dwDriveMcsDumpSizeByte = %ld\n", tscb->dwDriveParametricDumpSizeByte);
      printf("  dwDrivePesCylinder = %ld\n", tscb->dwDrivePesCylinder);
      printf("  bDrivePesHeadNumber = %d\n", tscb->bDrivePesHeadNumber);
      printf("  bDrivePesTemperature = %d\n", tscb->bDrivePesTemperature);
      printf("  wDrivePesDataCount = %d\n", tscb->wDrivePesDataCount);
      printf("  wDrivePesSpindleSpeed = %d\n", tscb->wDrivePesSpindleSpeed);
      printf("  wDrivePesNumberOfServoSector = %d\n", tscb->wDrivePesNumberOfServoSector);
      printf("  bDriveSignature[128] = %.128s\n", tscb->bDriveSignature);
      printf("  wDriveNativeErrorCode = %04xh\n", tscb->wDriveNativeErrorCode);
      printf("  dwDriveExtendedErrorCode = %08lxh\n", tscb->dwDriveExtendedErrorCode);

      printf("  dwDriveTestElapsedTimeSec = %ld\n", tscb->dwDriveTestElapsedTimeSec);
      printf("  dwDriveTestIdentifyTimeoutSec = %ld\n", tscb->dwDriveTestIdentifyTimeoutSec);
      printf("  dwDriveTestStatusPollingIntervalTimeSec = %ld\n", tscb->dwDriveTestStatusPollingIntervalTimeSec);
      printf("  dwTimeoutSecFromStartToSRSTEndCutoff = %ld\n", tscb->dwTimeoutSecFromStartToSRSTEndCutoff);
      printf("  dwTimeoutSecFromStartToSRSTEnd = %ld\n", tscb->dwTimeoutSecFromStartToSRSTEnd);
      printf("  dwDriveTestRawdataDumpTimeoutSec = %ld\n", tscb->dwDriveTestRawdataDumpTimeoutSec);

      printf("  dwDriveAmbientTemperatureTarget = %ld\n", tscb->dwDriveAmbientTemperatureTarget);
      printf("  dwDriveAmbientTemperaturePositiveRampRate = %ld\n", tscb->dwDriveAmbientTemperaturePositiveRampRate);
      printf("  dwDriveAmbientTemperatureNegativeRampRate = %ld\n", tscb->dwDriveAmbientTemperatureNegativeRampRate);

      printf("  dwDrivePowerSupply5V = %ld\n", tscb->dwDrivePowerSupply5V);
      printf("  dwDrivePowerSupply12V = %ld\n", tscb->dwDrivePowerSupply12V);
      printf("  dwDrivePowerRiseTime5V = %ld\n", tscb->dwDrivePowerRiseTime5V);
      printf("  dwDrivePowerRiseTime12V = %ld\n", tscb->dwDrivePowerRiseTime12V);
      printf("  dwDrivePowerIntervalFrom5VTo12V = %ld\n", tscb->dwDrivePowerIntervalFrom5VTo12V);
      printf("  dwDrivePowerOnWaitTime = %ld\n", tscb->dwDrivePowerOnWaitTime);
      printf("  dwDrivePowerOffWaitTime = %ld\n", tscb->dwDrivePowerOffWaitTime);
      printf("  dwDriveUartPullup = %ld\n", tscb->dwDriveUartPullup);

      printf("  isNoPlugCheck = %d\n", tscb->isNoPlugCheck);
      printf("  isNoPowerControl = %d\n", tscb->isNoPowerControl);
      printf("  isNoPowerOffAfterTest = %d\n", tscb->isNoPowerOffAfterTest);
      printf("  isNoDriveFinalized = %d\n", tscb->isNoDriveFinalized);
      printf("  isBadHeadReport = %d\n", tscb->isBadHeadReport);
      printf("  isMultiGrading = %d\n", tscb->isMultiGrading);
      printf("  isDriveVoltageDataReport = %d\n", tscb->isDriveVoltageDataReport);
      printf("  isReSrstProhibitControl = %d\n", tscb->isReSrstProhibitControl);

      printf("  isTpiSearch = %d\n", tscb->isTpiSearch);
      printf("  wTpiSearchOffset = %xh\n", tscb->wTpiSearchOffset);
      printf("  wTpiSearchSize = %d\n", tscb->wTpiSearchSize);

      printf("  isForceDriveTestCompFlagOn = %d\n", tscb->isForceDriveTestCompFlagOn);
      printf("  isForceDriveUnload = %d\n", tscb->isForceDriveUnload);
      printf("  isTesterLogReportEnable = %d\n", tscb->isTesterLogReportEnable);
      printf("  isTesterLogMirrorToStdout = %d\n", tscb->isTesterLogMirrorToStdout);

      printf("  isFakeErrorCode = %d\n", tscb->isFakeErrorCode);
      printf("  dwFakeErrorCode = %lxh\n", tscb->dwFakeErrorCode);

      printf("  dwTesterLogSizeKByte = %ld\n", tscb->dwTesterLogSizeKByte);
      printf("  isTestMode = %d\n", tscb->isTestMode);
      printf("  dwTestModeParam1 = %ld\n", tscb->dwTestModeParam1);
      printf("  dwTestModeParam2 = %ld\n", tscb->dwTestModeParam2);
      printf("  dwTestModeParam3 = %ld\n", tscb->dwTestModeParam3);

      printf("  dwTimeStampAtTestStart = %ld\n", tscb->dwTimeStampAtTestStart);
      printf("  dwTimeStampAtTurnDrivePowerSupplyOn = %ld\n", tscb->dwTimeStampAtTurnDrivePowerSupplyOn);
      printf("  dwTimeStampAtIdentifyDrive = %ld\n", tscb->dwTimeStampAtIdentifyDrive);
      printf("  dwTimeStampAtWaitDriveTestCompleted = %ld\n", tscb->dwTimeStampAtWaitDriveTestCompleted);
      printf("  dwTimeStampAtGetDriveRawdata = %ld\n", tscb->dwTimeStampAtGetDriveRawdata);
      printf("  dwTimeStampAtFinalize = %ld\n", tscb->dwTimeStampAtFinalize);
      printf("  dwTimeStampAtTestEnd = %ld\n", tscb->dwTimeStampAtTestEnd);

      for (j = 0 ; j < (sizeof(tscb->uartPerformanceLogBlock) / sizeof(tscb->uartPerformanceLogBlock[0])) ; j++) {
        printf("  uartPerformanceLogBlock[%ld].dwNumOfCommand = %ld\n", j, uplb[j].dwNumOfCommand);
        printf("  uartPerformanceLogBlock[%ld].dwNumOfCommandSuccess = %ld\n", j, uplb[j].dwNumOfCommandSuccess);
        printf("  uartPerformanceLogBlock[%ld].dwTotalDataSize = %ld\n", j, uplb[j].dwTotalDataSize);
      }

      printf("  testScriptErrorBlock.FatalError = %d\n", tseb->isFatalError);
      printf("  testScriptErrorBlock.AbortByUser = %d\n", tseb->isAbortByUser);
      printf("  testScriptErrorBlock.CellTemperatureAbnormal = %d\n", tseb->isCellTemperatureAbnormal);
      printf("  testScriptErrorBlock.DriveUnplugged = %d\n", tseb->isDriveUnplugged);
      printf("  testScriptErrorBlock.DrivePowerFail = %d\n", tseb->isDrivePowerFail);
      printf("  testScriptErrorBlock.DriveTestTimeout = %d\n", tseb->isDriveTestTimeout);
      printf("  testScriptErrorBlock.DriveIdentifyTimeout = %d\n", tseb->isDriveIdentifyTimeout);
      printf("  testScriptErrorBlock.DriveRawdataDumpTimeoutError = %d\n", tseb->isDriveRawdataDumpTimeoutError);
      printf("  testScriptErrorBlock.DriveRawdataDumpReportingFlagError = %d\n", tseb->isDriveRawdataDumpReportingFlagError);
      printf("  testScriptErrorBlock.DriveRawdataDumpDriveError = %d\n", tseb->isDriveRawdataDumpDriveError);
      printf("  testScriptErrorBlock.DriveRawdataDumpTotalPageError = %d\n", tseb->isDriveRawdataDumpTotalPageError);
      printf("  testScriptErrorBlock.DriveRawdataDumpPageIndexError = %d\n", tseb->isDriveRawdataDumpPageIndexError);
      printf("  testScriptErrorBlock.DriveRawdataDumpPesPageError = %d\n", tseb->isDriveRawdataDumpPesPageError);
      printf("  testScriptErrorBlock.DriveRawdataDumpOtherError = %d\n", tseb->isDriveRawdataDumpOtherError);


      for (dmcb = &tscb->driveMemoryControlBlock[0], j = 0 ; dmcb->bName[0] != '\0' ; dmcb++, j++) {
        printf("  driveMemoryControlBlock[%ld].dwAddress = %lxh\n", j, dmcb->dwAddress);
        printf("  driveMemoryControlBlock[%ld].dwAddressOffset = %ld\n", j, dmcb->dwAddressOffset);
        printf("  driveMemoryControlBlock[%ld].wSize = %d\n", j, dmcb->wSize);
        printf("  driveMemoryControlBlock[%ld].bAccessFlag = %d\n", j, dmcb->bAccessFlag);
        printf("  driveMemoryControlBlock[%ld].bEndianFlag = %d\n", j, dmcb->bEndianFlag);
        printf("  driveMemoryControlBlock[%ld].bReportFlag = %d\n", j, dmcb->bReportFlag);
        printf("  driveMemoryControlBlock[%ld].bName[20+1] = %s\n", j, dmcb->bName);
      }

      printf("  cellCardInventoryBlock.bCustomerInformation\n");
      putBinaryDataDumpOfPrintf(ccib->bCustomerInformation, 0, sizeof(ccib->bCustomerInformation));
      printf("  cellCardInventoryBlock.bManufactureCountry\n");
      putBinaryDataDumpOfPrintf(ccib->bManufactureCountry, 0, sizeof(ccib->bManufactureCountry));
      printf("  cellCardInventoryBlock.bReserved_1\n");
      putBinaryDataDumpOfPrintf(ccib->bReserved_1, 0, sizeof(ccib->bReserved_1));
      printf("  cellCardInventoryBlock.bPartsNumber\n");
      putBinaryDataDumpOfPrintf(ccib->bPartsNumber, 0, sizeof(ccib->bPartsNumber));
      printf("  cellCardInventoryBlock.bCardRev\n");
      putBinaryDataDumpOfPrintf(ccib->bCardRev, 0, sizeof(ccib->bCardRev));
      printf("  cellCardInventoryBlock.bReserved_2\n");
      putBinaryDataDumpOfPrintf(ccib->bReserved_2, 0, sizeof(ccib->bReserved_2));
      printf("  cellCardInventoryBlock.bCardType\n");
      putBinaryDataDumpOfPrintf(ccib->bCardType, 0, sizeof(ccib->bCardType));
      printf("  cellCardInventoryBlock.bHddType\n");
      putBinaryDataDumpOfPrintf(ccib->bHddType, 0, sizeof(ccib->bHddType));
      printf("  cellCardInventoryBlock.bReserved_3\n");
      putBinaryDataDumpOfPrintf(ccib->bReserved_3, 0, sizeof(ccib->bReserved_3));
      printf("  cellCardInventoryBlock.bSerialNumber\n");
      putBinaryDataDumpOfPrintf(ccib->bSerialNumber, 0, sizeof(ccib->bSerialNumber));
      printf("  cellCardInventoryBlock.bReserved_4\n");
      putBinaryDataDumpOfPrintf(ccib->bReserved_4, 0, sizeof(ccib->bReserved_4));
      printf("  cellCardInventoryBlock.bConnectorLifeCount\n");
      putBinaryDataDumpOfPrintf(ccib->bConnectorLifeCount, 0, sizeof(ccib->bConnectorLifeCount));
      printf("  cellCardInventoryBlock.bReserved_5\n");
      putBinaryDataDumpOfPrintf(ccib->bReserved_5, 0, sizeof(ccib->bReserved_5));
      printf("  cellCardInventoryBlock.bConnectorLifeCountBackup\n");
      putBinaryDataDumpOfPrintf(ccib->bConnectorLifeCountBackup, 0, sizeof(ccib->bConnectorLifeCountBackup));
      printf("  cellCardInventoryBlock.bReserved_6\n");
      putBinaryDataDumpOfPrintf(ccib->bReserved_6, 0, sizeof(ccib->bReserved_6));

      printf("}\n");

    } else if (MATCH == memcmp(&fileimage[i], KEY_TESTER_LOG, 20)) {
      Dword j = 0;
      printf("hhhh:mm:ss,cell,file,message\n");
      for (j = 0 ; j < length ; j++) {
        putc(fileimage[i+24+j], stdout);
      }
      putc('\n', stdout);

    } else if (MATCH == memcmp(&fileimage[i], KEY_TESTER_RECORD, 20)) {
      Dword j = 0;
      TEST_SCRIPT_RECORDER_BLOCK tsrb;
      printf("time[sec],temp[degC],xvolt[V],yvolt[V],HDDtemp[degC],HDDstep\n");
      for (j = 0 ; j < length ; j += sizeof(TEST_SCRIPT_RECORDER_BLOCK)) {
        memmove(&tsrb, &fileimage[i+24+j], sizeof(TEST_SCRIPT_RECORDER_BLOCK));
        if (!isLittleEndianRawdata) {
          memcpyWith32bitBigEndianTo32bitConversion(&tsrb.dwDriveTestTimeSec, (Byte *)&tsrb.dwDriveTestTimeSec, sizeof(tsrb.dwDriveTestTimeSec));
          memcpyWith16bitBigEndianTo16bitConversion(&tsrb.wTempInHundredth, (Byte *)&tsrb.wTempInHundredth, sizeof(tsrb.wTempInHundredth));
          memcpyWith16bitBigEndianTo16bitConversion(&tsrb.wV5InMilliVolts, (Byte *)&tsrb.wV5InMilliVolts, sizeof(tsrb.wV5InMilliVolts));
          memcpyWith16bitBigEndianTo16bitConversion(&tsrb.wV12InMilliVolts, (Byte *)&tsrb.wV12InMilliVolts, sizeof(tsrb.wV12InMilliVolts));
        }
        printf("%ld,%.2f,%.3f,%.3f,%d,%d\n",
               tsrb.dwDriveTestTimeSec,
               tsrb.wTempInHundredth / 100.0,
               tsrb.wV5InMilliVolts / 1000.0,
               tsrb.wV12InMilliVolts / 1000.0,
               tsrb.bDriveTemperature,
               tsrb.bDriveStepCount
              );
      }

    }

    i += 24 + length;
  }
  free(fileimage);
  return;
}
