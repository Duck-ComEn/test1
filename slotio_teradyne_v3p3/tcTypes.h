#ifndef _TCTYPES_H_
#define _TCTYPES_H_


typedef unsigned long Dword;
typedef unsigned short Word;
typedef unsigned char Byte;

typedef struct cell_card_inventory_block {
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


#ifdef __cplusplus
extern "C"
{
#endif

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

#ifdef __cplusplus
}
#endif


#endif
