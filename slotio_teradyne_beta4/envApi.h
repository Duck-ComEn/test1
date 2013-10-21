#ifndef _ENV_API_H_
#define _ENV_API_H_

/* ------------------------------------------------------------------
 *
 * THESE THINGS ARE NOW DEFINED IN TCTYPES.H
 *
//#include "..\equ.h"  Ilan - we didn't receive this file

// Hitachi is missing these definitions. Took them from ts.h 
// (which we can't and don't want to include here.) 
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


extern Byte isCellEnvironmentError(Byte bCellNo);
extern Byte getCurrentTemperature(Byte bCellNo, Word *wTempInHundredth);
extern Byte setTargetTemperature(Byte bCellNo, Word wTempInHundredth);
extern Byte getTargetTemperature(Byte bCellNo, Word *wTempInHundredth);
extern Byte getReferenceTemperature(Byte bCellNo, Word *wTempInHundredth);
extern Byte getHeaterOutput(Byte bCellNo, Word *wHeaterOutputInPercent);
extern Byte setHeaterOutput(Byte bCellNo, Word wHeaterOutputInPercent);
extern Byte getShutterPosition(Byte bCellNo, Word *wShutterPositionInPercent);
extern Byte setShutterPosition(Byte bCellNo, Word wShutterPositionInPercent);
extern Byte getTemperatureErrorStatus(Byte bCelNo, Word *wErrorStatus);
extern Byte getPositiveTemperatureRampRate(Byte bCellNo, Word *wTempInHundredthPerMinutes);
extern Byte setPositiveTemperatureRampRate(Byte bCellNo, Word wTempInHundredthPerMinutes);
extern Byte getNegativeTemperatureRampRate(Byte bCellNo, Word *wTempInHundredthPerMinutes);
extern Byte setNegativeTemperatureRampRate(Byte bCellNo, Word wTempInHundredthPerMinutes);
extern Byte setTemperatureSensorCarlibrationData(Byte bCellNo, Word wSoftwareTempInHundredth, Word wRealTempInHundredth);
extern Byte setTargetVoltage(Byte bCellNo,Word wV5InMilliVolts, Word wV12InMilliVolts);
extern Byte getTargetVoltage(Byte bCellNo,Word *wV5InMilliVolts, Word *wV12InMilliVolts);
extern Byte getCurrentVoltage(Byte bCellNo,Word *wV5InMilliVolts, Word *wV12InMilliVolts);
extern Byte setVoltageCalibration(Byte bCellNo, Word wV5LowInMilliVolts, Word wV5HighInMilliVolts, Word wV12LowInMilliVolts, Word wV12HighInMilliVolts);
extern Byte setVoltageRiseTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec);
extern Byte getVoltageRiseTime(Byte bCellNo, Word *wV5TimeInMsec, Word *wV12TimeInMsec);
extern Byte setVoltageFallTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec);
extern Byte getVoltageFallTime(Byte bCellNo, Word *wV5TimeInMsec, Word *wV12TimeInMsec);
extern Byte setVoltageInterval(Byte bCellNo, short wTimeFromV5ToV12InMsec);
extern Byte getVoltageInterval(Byte bCellNo, short *wTimeFromV5ToV12InMsec);
extern Byte getVoltageErrorStatus(Byte bCellNo, Word *wErrorStatus);sendHugeBuffer
extern Byte isDrivePlugged(Byte bCellNo);
extern Byte getCellInventory(Byte bCellNo, CELL_CARD_INVENTORY_BLOCK **stCellInventory);
extern int getCellComMode(Byte bCellNo, Byte *bComMode);
extern int setCellComMode(Byte bCellNo, Byte bComMode);
extern Byte setUartPullupVoltage(Byte bCellNo, Word wUartPullupVoltageInMilliVolts);
extern Byte getUartPullupVoltage(Byte bCellNo, Word *wUartPullupVoltageInMilliVolts);
-----------------------------------------------------------------------------------------------------------------*/
// ADDED BY TERADYNE TO SUPPORT HITACHI TEST CODE
/*!
 * sendHugeBuffer
   @param bCellNo (0 based) index number of the cell/HDD
   @param bufferLength: Length of buffer to hold data
   @param buffer: Pointer to buffer to hold data
   @param addrs: Address on HDD to read data from
   @param addrsCount: Number of addresses to read - Not used when sending data
   @return Status code (1 means error, 0 means no error)
 */
extern Byte sendHugeBuffer(Byte bCellNo, Dword bufferLength, Byte *buffer, Dword addrs, Dword addrs_cnt);

/*!
 * receiveHugeBuffer
   @param bCellNo (0 based) index number of the cell/HDD
   @param bufferLength: Filled in by this function with numbers of bytes received
   @param timeout: Duration to wait in milliseconds
   @param buffer: Pointer to buffer to hold data
   @param addrs: Address on HDD to read data from
   @param addrsCount: Number of addresses to read
   @return Status code (1 means error, 0 means no error)
 */
extern Byte receiveHugeBuffer(Byte bCellNo, Dword *dataLength, Dword timeout, Byte *buffer, Dword addrs, Dword addrs_cnt);


#endif
