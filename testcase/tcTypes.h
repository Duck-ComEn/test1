// Extended by Chris_Barnett@xyratex:
//--------------------------------------
// 1)  enhanced prototype of tc_printf to get compiler args checking
// 2)  add a protocol statistics structure etc...
// 3)  added micros-secs resolution ot pstats
//
// Modified by Satoshi_Takahashi@hitachigst:
//--------------------------------------
// 3)  add SAFE_HANDLING_TEMP macro
// 4)  add tcQuitTestcaseGently

#ifndef _TCTYPES_H_
#define _TCTYPES_H_

typedef unsigned long Dword;
typedef unsigned short Word;
typedef unsigned char Byte;

typedef struct stZque {    // Owner,   Description
  void *more, *less;       // Library  link list pointer
  Word wDID, wSID;         // User     destination, source ID
  Word wLgth;              // User     bData size in bytes. < 64KB.
  Byte bType;              // User     enum ZCmd (= 0x0a) fixed value
  Byte bAdrs;              // User     cell/slot address
  Byte bP[4];              // User     4 bytes parameter (transferred to Host)
  Word wDID0, wSID0;       // User     reserved
  Byte bData[0];           // User     data instance (transferred to Host)
} ZQUE, BQUE, QUE;

typedef struct stZData {    // Owner,   Description
  ZQUE  zque;               // User     see ZQUE definition. 'bType' shall be enum ZData (= 0x04)
  Dword dwLgth;             // User     *pData size in bytes. < 64KB
  Byte  *pName;             // User     pointer to file name string (data instance is allocated in bData[])
  Byte  *pData;             // User     pointer to file data (data instance is allocated in bData[])
  ZQUE  *que;               // User     reserved
  Byte  bReturn;            // User     reserved
  Word  wAdrs[2];           // User     reserved
  Byte  bData[0];           // User     data instance
} ZDAT;

// typedef struct stHddProtocolQueFormat {
typedef struct stHque {     // Owner,         Description
  // ZQUE header
  BQUE  *more, *less;       // Library        link list pointer
  Word  wDID, wSID;         // User           Destination, Source ID
  Word  wLgth;              // User           size from dwAddress to bData[] in bytes. < 64KB.
  Byte  bType;              // User           enum UART_READ(=0x41) or UART_WRITE(=0x42) or UART_IDENTIFY(=0x43)
  Byte  bAdrs;              // User           cell/slot Address
  Byte  bP[4];              // User           reserved
  Word  wDID0, wSID0;       // User           reserved
  // HDD Communication Data
  Dword dwAddress;          // User           HDD memory address
  Byte  bAck[2];            // Library        HDD ack/nack
  Word  wSync;              // Library        HDD packet data
  Word  wReturnCode;        // Library        communication error code
  Word  wLength;            // User/Library   send/receive data length
  Byte  bData[0];           // User/Library   send/receive data instance
} HQUE;


typedef struct cell_card_inventory_block {
  /* adapter card information */
  Byte bCustomerInformation[3072];
  Byte bManufactureCountry[3];
  Byte bReserved_1[13];
  Byte bPartsNumber[11];
  Byte bCardRev[1];
  Byte bReserved_2[4];
  Byte bCardType[8];
  Byte bHddType[2];
  Byte bReserved_3[214];
  Byte bSerialNumber[11];
  Byte bReserved_4[5];
  Byte bConnectorLifeCount[6];
  Byte bReserved_5[10];
  Byte bConnectorLifeCountBackup[6];
  Byte bReserved_6[506];
} CELL_CARD_INVENTORY_BLOCK;


#define MAX_NUM_OF_TEST_SCRIPT_THREAD      (24)


typedef struct sm_message {
  Dword type;
  Dword param[8];
  Byte  string[64];
} SM_MESSAGE;


typedef struct sm_buffer {
  Dword readPointer;
  Dword writePointer;
  Dword numOfMessage;
  SM_MESSAGE message[4];
} SM_BUFFER;


typedef struct stGenericThreadControlBlock {
  pthread_t thread;
  pthread_attr_t attr;
  void *(*start_routine)(void *);
  void *arg;
  void *thread_return;
  SM_BUFFER smSendBuffer;
  SM_BUFFER smReceiveBuffer;
  Word wHGSTCellNo;    // for only test script thread
  Word wCellIndex;     // for only test script thread
  Word wTrayIndex;     // for only test script thread
  Byte isFinished;      // for only test script thread
  Byte requestLabel[64];       // for only test script thread
} GTCB;


typedef struct stTestcaseControlBlock {
  GTCB tc;
  GTCB sh;
  GTCB cs;
  GTCB sm;
  GTCB ts[MAX_NUM_OF_TEST_SCRIPT_THREAD];
  int optionNumOfTestScriptThread;
  int optionPrintLogMirrorToStdout;
  int optionNoSignalHandlerThread;
  int optionNoHostCommunication;
  int optionGoToDie;
  int optionBenchDebug;
  int optionDevelopmentStation;
  int optionIgnoreHtempAdjust;
  int workaroundForAckMissingInOptimus;
  int optionSyncManagerDebug;
  int deleteLogFiles;
  int isCellOffline;
  int isChamberScriptReadyToStart;
  int argc;
  char **argv;
} TCCB;


extern TCCB tccb;


#ifdef __cplusplus
extern "C"
{
#endif

  //void tc_printf(char *bFileName, unsigned long dwNumOfLine, char *description, ...);

  // ChrisB - this gives benefits of compiler arg-type-match checking !!!!

#define PRINTF_STYLE(fmt_paramnum,firstarg_paramnum) __attribute__ ((format (printf,fmt_paramnum,firstarg_paramnum)))

  //
  // use gcc extension
  //
#define TCPRINTF(fmt, ...)  tcPrintf(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)

  //
  // use C99
  //
  //#define TCPRINTF(...)           TCPRINTF_TMP(__VA_ARGS__, "")
  //#define TCPRINTF_TMP(fmt, ...)  tcPrintf(__FILE__, __LINE__, __func__, fmt"%s", __VA_ARGS__)

  /**
   * <pre>
   * Description:
   *   Puts given strings into system log file. This function inserts current time
   *   in secondes and my cell ID automatically at top of line. Also, it append
   *   'line-feed(0x0A)' at last of description if it is not in description.
   * Arguments:
   *   Variable arguments like 'printf'
   * </pre>
   *****************************************************************************/
  void tcPrintf(char *bFileName, unsigned long dwNumOfLine, const char *bFuncName, char *description, ...)  PRINTF_STYLE(4, 5);



  /**
   * <pre>
   * Description:
   *   Quit this testcase gently.
   *   Turn off drive power and go to safe handling temperature mode.
   *   No error check for slot I/O control because we can do nothing even though error occurs
   * Arguments:
   *   None
   * Note:
   *   someone may want to use atexit() instead of this function.
   * </pre>
   *****************************************************************************/
  void tcExit(int exitcode);

  /**
   * <pre>
   * Description:
   *   Dump core file by using 'gcore'
   * Arguments:
   *   none
   * Return:
   *   none
   * </pre>
   *****************************************************************************/
  void tcDumpCore(void);

  /**
   * <pre>
   * Description:
   *  Display back trace by using tcPrintf() and check recursive call
   * Arguments:
   *   recursiveCheckFunc - INPUT - function name to be checked recursive call
   * Return:
   *   0: no recursive call. 1:recursive call 2:back trace is empty
   * Note:
   * </pre>
   *****************************************************************************/
  Byte tcDumpBacktrace(const char *recursiveCheckFunc );

  /**
   * <pre>
   * Description:
   *   Display binary data dump by using tcPrintf()
   * Arguments:
   *   *bData - pointer to data being shown
   *   dwSize - data size
   *   dwAddress - top address being shown in dump. e.g. (Dword)bData
   * Return:
   *   none
   * </pre>
   *****************************************************************************/
  void tcDumpData(Byte *bData, Dword dwSize, Dword dwAddress);

  unsigned long ts_crc32(const unsigned char *buffer, size_t byte_count);

#ifdef __cplusplus
}
#endif








#endif //ndef _TCTYPES_H_
