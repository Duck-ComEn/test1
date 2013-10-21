#define WRAPPER_INTERFACE_VERSION "20100604a-ChrisB"
	// added siSetDriveFanRPM
	// other comment clean-up
	
//#define WRAPPER_INTERFACE_VERSION "20100514a-ChrisB"
//
// 1) section at end containing:
//   -extra (optional) functions eg:
//     siGetReasonStringForReturnCode
//     etc
//
//   -externs to variables available in library eg:
//     control of logging
//     reading of statistics 
//     control of NEW/OLD PROTOCOL
// 
// 2) (possibly) minor enhancements/clarifications to text in comment sections
//

#ifndef _LIBSI_H_
#define _LIBSI_H_

#include <pthread.h>
//#include "../testcase/tcTypes.h"
#include "tcTypes.h" // (relative path not suitable for suppliers)



//======================================================================================================================================
//======================================================================================================================================
// Additions offered by Xyratex/ChrisBarnett  (Moved from ../testcase/tcTypes.h 2011.06.14 Satoshi Takahashi
//======================================================================================================================================
//======================================================================================================================================

typedef enum _protocol_rc {
  prcOK = 0,
  prcXYRC,  // (see global protocol_last_xyrc for details)
  prcTimeoutWaitingForAck,
  prcBadAckValue,
  prcExcessivePacketReTransmits,
  prcRxBufferOverrun,
  prcTimeoutWaitingForCompleteResponse,
  prcNotImplementedYet,

  // insert new values immediately ABOVE here...
  prcEnd
} protocol_rc;



struct _protocol_stats {
  int     TotalCommands;
  double TotalDataBytesReceived;
  double TotalTimeTransferring_mS;
  double TotalTimeReceiving_mS;

  double TotalTimeTransferring_uS; // 25May10 - not sure if/when will roll-over ?
  double TotalTimeReceiving_uS;    // 25May10 - not sure if/when will roll-over ?

  int     TotalBadCommands; // includes timeout, bad acks etc.
  int     TotalBadResponses; // includes timeout, overruns etc.
  int     TotalResponseOverruns; // RxBufferOverrun only

  int HighestReceivesForOneAck;
  int AvgReceivesForOneAck;
  double TotalNumberOfAcksReceived;

  int HighestReceivesForOneResponse;
  int AvgReceivesForOneResponse;
  double TotalNumberOfResponsesReceived;

  Byte  last_bad_xyrc;
  int   TotalPacketRetransmits;
  Byte u8RetransmitsLastCommand;
  Byte u8RetransmitsThisPacket;
  Byte  u8BadAckValue;   // when retransmit not successful
  Byte  u8RetriedAckValue; // // when retransmit was successful
  Byte  u8PacketNumberWithinCmd; // within last single BulkSerialSendCmd
};

// Note: see xyratex-enhanced libsi.h for 'extern' to instances of this structure


/**
 * for safe multi threading, each thread should be running with this mutex locked.
 * this mutex is unlocked when each thread enters sleep function,
 * and locked when each thread exits sleep function.
 * it guarantees that only one thread accesss slot I/O API at same time.
 *
 * note) this might be achived by thread attribue - schedpolicy=SCHED_RR in super user mode.
 ***************************************************************************************/
extern pthread_mutex_t mutex_slotio;

#ifdef __cplusplus
extern "C"
{
#endif


  /**
   * <pre>
   * Description:
   *   Initialize cell hardware
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if no error. otherwise, error
   * Note:
   *   it usually creates instance of slot environment control c++ class.
   *   it does nothing for GAIA
   * </pre>
   *****************************************************************************/
  Byte siInitialize(Byte bCellNo);

  /**
   * <pre>
   * Description:
   *   Finalize cell hardware
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if no error. otherwise, error
   * Note:
   *   it usually turns off drive power, sets safe handling temperature,
   *   make sure everything clean up to destory instance of slot environment.
   *   it does nothing for GAIA
   * </pre>
   *****************************************************************************/
  Byte siFinalize(Byte bCellNo);

  /**
   * return code to string conversion functoin will be defined
   * some rc value is dupulicate definition over each functions,
   * so it may need not only rc value but also function name
   *****************************************************************************/
  /* Byte *libsiRcToStr(Byte rc, void *func) */

  /**
   * <pre>
   * Description:
   *   Get a quick check of cell environment error status
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if no error. otherwise, error
   * Note:
   *   detail information can be queried in getVoltageErrorStatus(),
   *   getTemperatureErrorStatus(), etc
   * </pre>
   *****************************************************************************/
  Byte isCellEnvironmentError(Byte bCellNo);

  /**
   * <pre>
   * Description:
   *   Clear cell environment error flag
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if no error. otherwise, error
   * Note:
   *   It does NOT clear flags of dangerous error which needs hardware reset or
   *   physical repair process.
   * </pre>
   *****************************************************************************/
  Byte clearCellEnvironmentError(Byte bCellNo);

  /**
   * <pre>
   * Description:
   *   Gets the latest *measured* temperature level of the slot environment
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempInHundredth - OUTPUT - in hundredth of a degree Celsius
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getCurrentTemperature(Byte bCellNo, Word *wTempInHundredth);

  /**
   * <pre>
   * Description:
   *   Sets the target temperature values of the cell or the chamber & start ramping
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempInHundredth - INPUT - in hundredth of a degree Celsius
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte setTargetTemperature(Byte bCellNo, Word wTempInHundredth);

  /**
   * <pre>
   * Description:
   *   Gets the target temperature values of the cell or the chamber
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempInHundredth - OUTPUT - in hundredth of a degree Celsius
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getTargetTemperature(Byte bCellNo, Word *wTempInHundredth);

  /**
   * <pre>
   * Description:
   *   Gets the reference temperature values of the cell or the chamber
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempInHundredth - INPUT - in hundredth of a degree Celsius
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   this temperature is latest reference temperature determined by PID controller.
   *   this temperature is as same as target temperature
   * </pre>
   ***************************************************************************************/
  Byte getReferenceTemperature(Byte bCellNo, Word *wTempInHundredth);

  /**
   * <pre>
   * Description:
   *   Gets the current heater PWM output values of the cell or the chamber
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wHeaterOutputInPercent - OUTPUT - range is 0 -to- 100 in percent
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   0 means heater off, 100 means heater always on, 50 means 50% duty cycle of heater on/off
   * </pre>
   ***************************************************************************************/
  Byte getHeaterOutput(Byte bCellNo, Word *wHeaterOutputInPercent);

  /**
   * <pre>
   * Description:
   *   Sets the current heater PWM output values of the cell or the chamber in percent
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wHeaterOutputInPercent - INPUT - range is 0 -to- 100 in percent
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   DANGEROUS to set this by testcase!!!!!!
   *   see example at getHeaterOutput()
   * </pre>
   ***************************************************************************************/
  Byte setHeaterOutput(Byte bCellNo, Word wHeaterOutputInPercent);

  /**
   * <pre>
   * Description:
   *   Gets the current shutter position of the cell or the chamber
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wShutterPositionInPercent - INPUT - range is 0 -to- 100 in percent
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   0 means shutter full close to stop supplying cooling air,
   *   100 means shutter full open to supply all cooling air
   *   50 means shutter half open to supply half cooling air
   * </pre>
   ***************************************************************************************/
  Byte getShutterPosition(Byte bCellNo, Word *wShutterPositionInPercent);

  /**
   * <pre>
   * Description:
   *   Sets the current shutter position of the cell or the chamber
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wShutterPositionInPercent - OUTPUT - range is 0 -to- 100 in percent
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   see example at getShutterPosition()
   * </pre>
   ***************************************************************************************/
  Byte setShutterPosition(Byte bCellNo, Word wShutterPositionInPercent);

  /**
   * <pre>
   * Description:
   *   Gets the temperature error status
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTemperaturerrorStatus - OUTPUT - Various error assigned in each bit field (Drop, Over, etc).
   *                  (currently Supplier X returns HeaterInterlockTrip Reason bit values "hit*" in CellTypes.h)
   * Return:
   *   zero if no error, Otherwise, error.
   * Note:
   *   wErrorStatus bit assign is determined by each hardware - supplier should provide bit field definition.
   *   wErrorStatus is cleared zero when voltgate????  supplies state is changed from OFF to ON ?????????????????.
   * </pre>
   ***************************************************************************************/
  Byte getTemperatureErrorStatus(Byte bCellNo, Word *wErrorStatus);

  /**
   * <pre>
   * Description:
   *   Gets the current positive temperature ramp rate
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempInHundredthPerMinutes - OUTPUT - range is 1 -to- 999 (+0.01C/min -to- +9.99C/min)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getPositiveTemperatureRampRate(Byte bCellNo, Word *wTempInHundredthPerMinutes);

  /**
   * <pre>
   * Description:
   *   Sets the current positive temperature ramp rate
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempRampRate - INPUT - range is 1 -to- 999 (+0.01C/min -to- +9.99C/min)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   support range depends on real hardware system. supplier should provide
   *   the range to be able to set, and return error if testcase set over-range value.
   * </pre>
   ***************************************************************************************/
  Byte setPositiveTemperatureRampRate(Byte bCellNo, Word wTempInHundredthPerMinutes);

  /**
   * <pre>
   * Description:
   *   Gets the current negative temperature ramp rate
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempRampRate - OUTPUT - range is 1 -to- 999 (-0.01C/min -to- -9.99C/min)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getNegativeTemperatureRampRate(Byte bCellNo, Word *wTempInHundredthPerMinutes);

  /**
   * <pre>
   * Description:
   *   Sets the current negative temperature ramp rate
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempRampRate - INPUT - range is 1 -to- 999 (-0.01C/min -to- -9.99C/min)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note
   *   see the Note in setPositiveTemperatureRampRate()
   * </pre>
   ***************************************************************************************/
  Byte setNegativeTemperatureRampRate(Byte bCellNo, Word wTempInHundredthPerMinutes);

  /**
   * <pre>
   * Description:
   *   Sets the temperature sensor calibration data
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wRefTemp - INPUT - Reference temperature in hundredth of degree celsius. 30.00C and 60.00C are recommended.
   *   wAdjustTemp - INPUT - Real temperature value by calibrated reliable accurate temperature sensor
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   DANGEROUS to set this by testcase!!!!!!
   *   PID controller determine real temperature value from calculating temperature sensor value
   *   and correlation between temperature sensor value and real temperature value.
   * </pre>
   ***************************************************************************************/
  Byte setTemperatureSensorCarlibrationData(Byte bCellNo, Word wSoftwareTempInHundredth, Word wRealTempInHundredth);

  /**
   * <pre>
   * Description:
   *   Sets the temperature limit value for setting temperature and sensor reading temperature.
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wSetTempLowerInHundredth - INPUT - set temperature lower limit in hundredth of a degree Celsius
   *   wSetTempUpperInHundredth - INPUT - set temperature upper limit in hundredth of a degree Celsius
   *   wSensorTempLowerInHundredth - INPUT - sensor reading temperature lower limit
   *                                         in hundredth of a degree Celsius
   *   wSensorTempUpperInHundredth - INPUT - sensor reading  temperature upper limit
   *                                         in hundredth of a degree Celsius
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   default value is 25C to 60C for set temperature, 15C to 70C for sensor reading temperature
   * </pre>
   ***************************************************************************************/
  Byte setTemperatureLimit(Byte bCellNo, Word wSetTempLowerInHundredth, Word wSetTempUpperInHundredth, Word wSensorTempLowerInHundredth, Word wSensorTempUpperInHundredth);

  /**
   * <pre>
   * Description:
   *   Gets the temperature limit value for setting temperature and sensor reading temperature.
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wSetTempLowerInHundredth - OUTPUT - set temperature lower limit in hundredth of a degree Celsius
   *   wSetTempUpperInHundredth - OUTPUT - set temperature upper limit in hundredth of a degree Celsius
   *   wSensorTempLowerInHundredth - OUTPUT - sensor reading temperature lower limit
   *                                          in hundredth of a degree Celsius
   *   wSensorTempUpperInHundredth - OUTPUT - sensor reading  temperature upper limit
   *                                          in hundredth of a degree Celsius
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getTemperatureLimit(Byte bCellNo, Word *wSetTempLowerInHundredth, Word *wSetTempUpperInHundredth, Word *wSensorTempLowerInHundredth, Word *wSensorTempUpperInHundredth);

  /**
   * <pre>
   * Description:
   *   Requests the cell environment to never exceed the 'SafeHandling' Temperature
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempInHundredth - INPUT - safe handling temperature in hundredth of a degree Celsius
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   This is the best way to 'cool' a slot environment after a test
   *   (better than requesting a ramp down to 'ambient plus a bit')
   *
   *   Heat will NEVER be applied,
   *   Cooling will be used to achieve controlled ramp own to Safehandling Temp as necessary
   *   and then the minimum amount of cooling necessary to remain at or below SafeHandling temp
   *
   *   This is also the best mode to select when you don't really care about temperature,
   * </pre>
   ***************************************************************************************/
  Byte setSafeHandlingTemperature(Byte bCellNo, Word wTempInHundredth);

  /**
   * <pre>
   * Description:
   *   Gets the safe handling temperature values of the cell or the chamber
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTempInHundredth - OUTPUT - in hundredth of a degree Celsius
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getSafeHandlingTemperature(Byte bCellNo, Word wTempInHundredth);

  /**
   * <pre>
   * Description:
   *   Sets the target voltatge values supplying to the drive
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5InMilliVolts - INPUT - range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
   *   wV12InMilliVolts - INPUT - range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
    * Return:
   *   zero if success. otherwise, non-zero value.
   *  Note:
   *   power supply is turned off if 0 is set, otherwise turn on.
   *   support range depends on real hardware system. supplier should provide
   *   the range to be able to set, and return error if testcase set over-range value.
   * </pre>
   ***************************************************************************************/
  Byte setTargetVoltage(Byte bCellNo, Word wV5InMilliVolts, Word wV12InMilliVolts);

  /**
   * <pre>
   * Description:
   *   Gets the target voltatge values supplying to the drive
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5InMilliVolts - OUTPUT - range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
   *   wV12InMilliVolts - OUTPUT - range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
    * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getTargetVoltage(Byte bCellNo, Word *wV5InMilliVolts, Word *wV12InMilliVolts);

  /**
   * <pre>
   * Description:
   *   Gets the latest *measured* voltage levels
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5InMilliVolts - OUTPUT - in millivolts of measured voltage level
   *   wV12InMilliVolts - OUTPUT - in millivolts of measured voltage level
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   If a supply is NOT switched on - then you see the ACTUAL (zero or residual) value
   *   NOT the set value
   *   500~700 msec takes to update value in firmware. 1 sec is enough to get new value by testcase.
   *   15-16 bit ADC Xcalibre (16 bit available in hardware)
   *   15-16 bit ADC Optimus (16 bit available in hardware)
   *   accuracy depends on register and reference voltage
   * </pre>
   ***************************************************************************************/
  Byte getCurrentVoltage(Byte bCellNo, Word *wV5InMilliVolts, Word *wV12InMilliVolts);

  /**
   * <pre>
   * Description:
   *   Sets the voltage output and voltage reading calibration data into cell card EEPROM.
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5LowInMilliVolts - OUTPUT - HDD+5V real-voltage in milli-volts value measured by high accuracy digital multi meter
   *   wV5HighInMilliVolts - OUTPUT - HDD+5V real-voltage in milli-volts value measured by high accuracy digital multi meter
   *   wV12LowInMilliVolts - OUTPUT - HDD+12V real-voltage in milli-volts value measured by high accuracy digital multi meter
   *   wV12HighInMilliVolts - OUTPUT - HDD+12V real-voltage in milli-volts value measured by high accuracy digital multi meter
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   DANGEROUS to set this by testcase!!!!!!
   *
   *   Voltage Adjustment is done in the following method.
   *
   *   (1) Real output voltage values are stored when low value (DL) and high value (DH)
   *       is set to D/A converter
   *
   *   (2) The measured voltage is registered in the flash memory
   *
   *   (3) D/A Converter set value is calculated as following formula
   *
   *       Y = (X - VL) x (DH - DL) / (VH - VL) + DL
   *
   *       Y : D/A Converter set value
   *       X : Output voltage
   *       DL : D/A Converter set value (Low)
   *       DH : D/A Converter set value (High)
   *       VL : Output voltage (When D/A Converter set value is DL)
   *       VH : Output voltage (When D/A Converter set value is DH)
   * </pre>
   ***************************************************************************************/
  Byte setVoltageCalibration(Byte bCellNo, Word wV5LowInMilliVolts, Word wV5HighInMilliVolts, Word wV12LowInMilliVolts, Word wV12HighInMilliVolts);

  /**
   * <pre>
   * Description:
   *   Sets the voltage limit values supplying to the drive
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5LowerLimitInMilliVolts - INPUT - HDD+5V lower limit in mill-volts
   *                                       range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
   *   wV12LowerLimitInMilliVolts - INPUT - HDD+12V lower limit in mill-volts
   *                                        range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
   *   wV5UpperLimitInMilliVolts - INPUT - HDD+5V lower upper in mill-volts
   *                                       range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
   *   wV12UpperLimitInMilliVolts - INPUT - HDD+12V upper limit in mill-volts
   *                                        range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   Subsequent calls to setTargetVoltage must then be within these limits.
   *   Will reject any future setTargetVoltage calls to those supplies, if outside these limits.
   *   Limits default to the absolute LOWER and UPPER limits are HDD+5V +/- 10%, HDD+12 +/- 15%.
   *   This call would be useful in allowing you to code your own 'safety net' to protect against
   *   the lower levels of a testcase requesting a voltage which could fry a particular
   *   drive/interface type
   *
   *   exceed this limit to shutdown power supply & set error flag in getVoltageErrorStatus()
   *   exception is that a testcase can always set 0V without any errors to turn off power supply
   *   even if it is under low voltage limit.
   *
   *   support range depends on real hardware system. supplier should provide
   *   the range to be able to set, and return error if testcase set over-range value.
   * </pre>
   ***************************************************************************************/
  Byte setVoltageLimit(Byte bCellNo, Word wV5LowerLimitInMilliVolts, Word wV12LowerLimitInMilliVolts, Word wV5UpperLimitInMilliVolts, Word wV12UpperLimitInMilliVolts);

  /**
   * <pre>
   * Description:
   *   Gets the voltage limit values supplying to the drive
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5LowerLimitInMilliVolts - OUTPUT - HDD+5V lower limit in mill-volts
   *                                       range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
   *   wV12LowerLimitInMilliVolts - OUTPUT - HDD+12V lower limit in mill-volts
   *                                        range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
   *   wV5UpperLimitInMilliVolts - OUTPUT - HDD+5V lower upper in mill-volts
   *                                       range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
   *   wV12UpperLimitInMilliVolts - OUTPUT - HDD+12V upper limit in mill-volts
   *                                        range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getVoltageLimit(Byte bCellNo, Word *wV5LowerLimitInMilliVolts, Word *wV12LowerLimitInMilliVolts, Word *wV5UpperLimitInMilliVolts, Word *wV12UpperLimitInMilliVolts);

  /**
   * <pre>
   * Description:
   *   Sets the current limit values supplying to the drive
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5LimitInMilliAmpere - INPUT - HDD+5V current limit in mill-Ampere
   *                                    range is 0 -to- 5000 in milli-Ampere (0-5A)
   *   wV12LimitInMilliAmpere - INPUT - HDD+12V current limit in mill-Ampere
   *                                     range is 0 -to- 5000 in milli-Ampere (0-5A)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   default is no limit.
   *
   *   exceed this limit to shutdown power supply & set error flag in getCurrentErrorStatus()
   *   exception is that a testcase can always set 0V without any errors to turn off power supply
   *   even if it is under low current limit.
   *
   *   support range depends on real hardware system. supplier should provide
   *   the range to be able to set, and return error if testcase set over-range value.
   * </pre>
   ***************************************************************************************/
  Byte setCurrentLimit(Byte bCellNo, Word wV5LimitInMilliAmpere, Word wV12LimitInMilliAmpere);

  /**
   * <pre>
   * Description:
   *   Gets the current limit values supplying to the drive
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5LimitInMilliAmpere - OUTPUT - HDD+5V current limit in mill-Ampere
   *                                     range is 0 -to- 5000 in milli-Ampere (0-5A)
   *   wV12LimitInMilliAmpere - OUTPUT - HDD+12V current limit in mill-Ampere
   *                                      range is 0 -to- 5000 in milli-Ampere (0-5A)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getCurrentLimit(Byte bCellNo, Word *wV5LimitInMilliAmpere, Word *wV12LimitInMilliAmpere);

  /**
   * <pre>
   * Description:
   *   Sets the voltatge rise up time
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5TimeInMsec - INPUT - time in mill-seconds for HDD+5V. range is 0 -to- 255 (0-255msec)
   *   wV12TimeInMsec - INPUT - time in mill-seconds for HDD+12V. range is 0 -to- 255 (0-255msec)
    * Return:
   *   zero if success. otherwise, non-zero value.
   *  Note:
   *   support range depends on real hardware system. supplier should provide
   *   the range to be able to set, and return error if testcase set over-range value.
   * </pre>
   ***************************************************************************************/
  Byte setVoltageRiseTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec);

  /**
   * <pre>
   * Description:
   *   Gets the voltatge rise up time
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5TimeInMsec - OUTPUT - time in mill-seconds for HDD+5V
   *   wV12TimeInMsec - OUTPUT - time in mill-seconds for HDD+12V
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getVoltageRiseTime(Byte bCellNo, Word *wV5TimeInMsec, Word *wV12TimeInMsec);

  /**
   * <pre>
   * Description:
   *   Sets the voltatge fall down time
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5TimeInMsec - INPUT - time in mill-seconds for HDD+5V. range is 0 -to- 255 (0-255msec)
   *   wV12TimeInMsec - INPUT - time in mill-seconds for HDD+12V. range is 0 -to- 255 (0-255msec)
    * Return:
   *   zero if success. otherwise, non-zero value.
   *  Note:
   *   support range depends on real hardware system. supplier should provide
   *   the range to be able to set, and return error if testcase set over-range value.
   * </pre>
   ***************************************************************************************/
  Byte setVoltageFallTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec);

  /**
   * <pre>
   * Description:
   *   Gets the voltatge fall down time
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5TimeInMsec - OUTPUT - time in mill-seconds for HDD+5V
   *   wV12TimeInMsec - OUTPUT - time in mill-seconds for HDD+12V
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getVoltageFallTime(Byte bCellNo, Word *wV5TimeInMsec, Word *wV12TimeInMsec);

  /**
   * <pre>
   * Description:
   *   Sets the voltatge rise interval between 5V and 12V
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTimeFromV5ToV12InMsec - INPUT - interval time from 5V on to 12V on.
   *                                    range is 0 -to- 255 (0-255msec)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   positive value is 5V first then 12V.
   *   negative value is reverse order
   *   example
   *      100 : 5V-On -> 100msec wait -> 12V-On
   *      -20 : 12V-On -> 20msec wait -> 5V-On
   *
   *   support range depends on real hardware system. supplier should provide
   *   the range to be able to set, and return error if testcase set over-range value.
   * </pre>
   ***************************************************************************************/
  Byte setVoltageInterval(Byte bCellNo, Word wTimeFromV5ToV12InMsec);

  /**
   * <pre>
   * Description:
   *   Gets the voltatge rise interval between 5V and 12V
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wTimeFromV5ToV12InMsec - OUTPUT - interval time from 5V on to 12V on.
   *                                     range is 0 -to- 255 (0-255msec)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getVoltageInterval(Byte bCellNo, Word *wTimeFromV5ToV12InMsec);

  /**
   * <pre>
   * Description:
   *   Gets the voltage error status
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wVoltagErrorStatus - Various error assigned in each bit field (Drop, Over, etc).
   * Return:
   *   zero if no error, Otherwise, error.
   * Note:
   *   wErrorStatus bit assign is determined by each hardware - supplier should provide bit field definition.
   *   wErrorStatus is cleared zero when voltgate supplies state is changed from OFF to ON.
   * </pre>
   ***************************************************************************************/
  Byte getVoltageErrorStatus(Byte bCellNo, Word *wErrorStatus);

  /**
   * <pre>
   * Description:
   *   Gets the drive plug status
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if not plugged. Otherwise, drive plugged.
   * </pre>
   ***************************************************************************************/
  Byte isDrivePlugged(Byte bCellNo);

  /**
   * <pre>
   * Description:
   *   Gets cell card inventory information.
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   stCellInventory - OUTPUT - The structure of cell inventory information.
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   CELL_CARD_INVENTORY_BLOCK definition depends on suppliers.
   *   supplier should provide the struct definition.
   * </pre>
   ***************************************************************************************/
  Byte getCellInventory(Byte bCellNo, CELL_CARD_INVENTORY_BLOCK **stCellInventory);

  /**
   * <pre>
   * Description:
   *   Sets the target voltatge values supplying to UART I/O and UART pull-up
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wUartPullupVoltageInMilliVolts - INPUT - UART pull-up voltage value
   *                                            range is 0 -to- 3300 in milli-volts (0.0V - 3.3V)
    * Return:
   *   zero if success. otherwise, non-zero value.
   *  Note:
   *   IMPORTANT!!!!! - even the name is wUartPullupVoltageInMilliVolts, this applies not
   *   only UART pull-up but also UART I/O voltage (Vcc for UART Buffer). UART pull-up
   *   shall be supplied during HDD+5V turned on. That means UART pull-up and HDD+5V
   *   shall be synchronized.
   *
   *   support range depends on real hardware system. supplier should provide
   *   the range to be able to set, and return error if testcase set over-range value.
   * </pre>
   ***************************************************************************************/
  Byte setUartPullupVoltage(Byte bCellNo, Word wUartPullupVoltageInMilliVolts);

  Byte setUartPullupVoltageDefault(Byte bCellNo);

  /**
   * <pre>
   * Description:
   *   Gets the target voltatge values supplying to UART I/O and UART pull-up
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wUartPullupVoltageInMilliVolts - OUTPUT - UART pull-up voltage value
    * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getUartPullupVoltage(Byte bCellNo, Word *wUartPullupVoltageInMilliVolts);

  /**
   * <pre>
   * Description:
   *   Sets UART baudrate setting
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   dwBaudrate - INPUT - baudrate in bps
    * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siSetUartBaudrate(Byte bCellNo, Dword dwBaudrate);

  /**
   * <pre>
   * Description:
   *   Gets UART baudrate setting
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   dwBaudrate - OUTPUT - baudrate in bps
    * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siGetUartBaudrate(Byte bCellNo, Dword *dwBaudrate);

  /**
   * <pre>
   * Description:
   *   Write given data to drive memory via UART
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   dwAddress - INPUT - drive memory address
   *   wSize - INPUT - drive memory size
   *   *bData - INPUT - pointer to data
   *   wTimeoutInMillSec - INPUT - time out value in mill-seconds
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   *****************************************************************************/
  Byte siWriteDriveMemory(Byte bCellNo, Dword dwAddress, Word wSize, Byte *bData, Word wTimeoutInMillSec);

  /**
   * <pre>
   * Description:
   *   Read data from drive memory via UART
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   dwAddress - INPUT - drive memory address
   *   wSize - INPUT - drive memory size
   *   *bData - OUTPUT - pointer to data. caller function shall alloc memory.
   *   wTimeoutInMillSec - INPUT - time out value in mill-seconds
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   *****************************************************************************/
  Byte siReadDriveMemory(Byte bCellNo, Dword dwAddress, Word wSize, Byte *bData, Word wTimeoutInMillSec);

  /**
   * <pre>
   * Description:
   *   Query drive identify data via UART
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wSize - INPUT - drive memory size
   *   *bData - OUTPUT - pointer to data. caller function shall alloc memory.
   *   wTimeoutInMillSec - INPUT - time out value in mill-seconds
   * Return:
   *   zero if success. otherwise, non-zero value.
   *   echo command does not have 'address' parameter in their command block.
   *   so 'dwAddress' does not work exactly same as read command, but this function
   *   handle 'dwAddress' as if echo command has address parameter.
   *   example:- if dwAddress = 0x100 and wSize = 0x60 are set,
   *             this function issues echo command with length = 0x160 (0x100 + 0x60),
   *             then copy data in offset 0x100-0x15F into *bData.
   * </pre>
   *****************************************************************************/
  Byte siEchoDrive(Byte bCellNo, Word wSize, Byte *bData, Word wTimeoutInMillSec);
  // [DEBUG] remove address in argument  2010.05.17

  /**
   * <pre>
   * Description:
   *   Get drive UART version via UART
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wVersion - OUTPUT - version in 16 bits
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   This function requests that the drive report back to the host the version of the UART
   *   interface it supports. A drive that supports the UART 2 interface will return the Version response block
   *   with the version field set to 2, while a drive that does not will either return an error on the transmission
   *   (legacy versions) or return the response block with a different value (future versions).
   * </pre>
   *****************************************************************************/
  Byte siGetDriveUartVersion(Byte bCellNo, Word *wVersion);

  /**
   * <pre>
   * Description:
   *   Change drive delay time via UART
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wDelayTimeInMicroSec - INPUT - delay time in micro seconds
   *   wTimeoutInMillSec - INPUT - time out value in mill-seconds
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   This function is used to set the UART timing delay from host sending packet
   *   to drive responsing ack.
   * </pre>
   *****************************************************************************/
  Byte siSetDriveDelayTime(Byte bCellNo, Word wDelayTimeInMicroSec, Word wTimeoutInMillSec);

  /**
   * <pre>
   * Description:
   *   Reset drive via UART
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wType - INPUT - reset type
   *   wResetFactor - INPUT - reset factor
   *   wTimeoutInMillSec - INPUT - time out value in mill-seconds
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   The command initiates a reset on the drive. The drive will send a response block to the
   *   host indicating that the command is acknowledged and then call the appropriate reset function
   *
   *   Current implementation supports only type = 0x00 0x01 (0xHH 0xLL)
   *
   *   reset factor( 0xHH 0xLL )
   *     0x00 0x01 Jump to ShowStop function. Note that if drive receives this command, after sending response UART communication stops due to ShowStop
   *     0x00 0x02 Jump to Power On Reset Path.
   *     0x00 0x04 Request HardReset to RTOS ( RealTime Operating System )
   *     0x00 0x08 Request SoftReset to RTOS
   *     0x00 0x10 reserved for server products
   *     0x00 0x20 reserved for server products
   * </pre>
   *****************************************************************************/
  Byte siResetDrive(Byte bCellNo, Word wType, Word wResetFactor, Word wTimeoutInMillSec);

  /**
   * <pre>
   * Description:
   *   Change uart protocol
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   bProtocol - INPUT - protocol type
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   bProtocol 0: A00 protocol (PTB), 1: ARM protocol (JPK)
   * </pre>
   *****************************************************************************/
  Byte siSetUartProtocol(Byte bCellNo, Byte bProtocol);
  // [DEBUG] add this function  2010.05.17





	//======================================================================================================================================
	//======================================================================================================================================
	// Additions offered by Xyratex/ChrisBarnett
	//======================================================================================================================================
	//======================================================================================================================================
	
	Byte siGetReasonStringForReturnCode( Byte brc, char* pCallersBuffer, Word wBufferSize);

	Byte isOnTemp( Byte bCellNo); // using bit in CellStatusFlags (much easier than getCurrentTemp, particularly for SafeHandlingTemp)

	Byte SetFPGATestMode( Byte bCellNo);

	Byte siSetDriveFanRPM( Byte bCellNo, int DriveFanRPM); // ChrisB: 04Jun10 
		// allowable range varies by product, and firmware version 
		// returns xyrcParameterOutOfRange if request is outside allowed range for target product/version

	
	
	// To discuss/consider:
	//   LED control (direct from testcase)
	//   Firmware Error text strings (currently just logged if detected on status queries)



	// ChrisB 04Jun10
	// Note: I wanted to extern the Trace control bools, protocol statistics arrays, etc. here
	//      - but there is a conflict with use of 'bool' casuing compile errors in testcase folder 
	//      (suspect its a .cpp versus .c issue - possibly can resolve by using #ifdef __cplusplus, and defining to ints otherwise ?)
	
	// So, for now, I have created a seperate new header
	//  "libsi_xyexterns.h" <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	// to carry these for now
	// (I think moving them to  siMain.h was not good practice
	//   - since that is internal to the library and contains things that lib users/testcases should not see)


	Byte siSetElectronicsFanRPM( Byte bCellNo, int ElectronicsFanRPM);  // SatoshiT: 09Jun10
	Byte siSetFanRPMDefault( Byte bCellNo);  // SatoshiT: 07Jun10

	Byte siSetLed(Byte bCellNo, Byte bMode);  // SatoshiT: 10Jun10
        
        Byte setTemperatureEnvelope( Byte bCellNo, Word wEnvelopeTempInTenth); //Yukari Katayama: 14/June/2011 for commonality with teradyne  
	Byte adjustTemperatureControlByDriveTemperature(Byte bCellNo, Word wTempInHundredth);//Yukari Katayama: 14/June/2011 for commonality with teradyne

#ifdef __cplusplus
}
#endif

#endif
