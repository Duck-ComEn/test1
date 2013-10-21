/*!
 * @file
 * This file contains function prototypes for Hitachi Interface
 * 
 *  Nov-29-2010, MG: 
 *   1) Fixed DUT powerup. Echo command does not require <slot> 1 pup from console anymore.
 *   2) Added siGetInterCharacterDelay and siSetInterCharacterDelay. 
 *   3) isSlotThere is fixed. Returns correct status code.
 *   4) Implemented siSetUartProtocol. Depending on the prototype, Echo and Read command
 *      will perform endian swap.        
 *  Dec-03-2010, MG:
 *   1) Incorporated patch from HGST 
 *   2) Multiple baud rate support
 *   3) Voltage margining is working. See setTargetVoltage() for info.
 *   4) UART level is fixed at 2.5V. However, setting to 0 will enable staggered spin.
 *   5) Added siBulkWriteToDrive(). This function disables UART2 before sending and
 *      re-enables after done.
 *   6) Added code to print serial no/ver. info in siInitialize 
 *   7) Added SIO IP address support. use export SIOIPADDRESS="xx.xx.xx.xx". If this env is
 *      not defined, ip address will default to 10.101.1.2
 *   8) Added siGetLastErrors for debugging SIO errors. This will be called in TERERROR(). 
 *  Dec-13-2010, MG:
 *   1) Beta2 API Support:
 *  		 getTargetTemperature
 *  		 getTemperatureErrorStatus
 *  		 getPositiveTemperatureRampRate
 *  		 getNegativeTemperatureRampRate
 *  		 getTargetVoltage
 *  		 getUartPullupVoltage
 *  		 getCurrentLimit
 *  		 getVoltageErrorStatus
 *  		 getVoltageLimit
 *  		 isOnTemp
 *  		 setCurrentLimit
 *  		 getTemperatureEnvelope
 *  		 setTemperatureEnvelope
 *  		 setTemperatureLimit
 *  		 getTemperatureLimit
 *  		 setVoltageLimit
 *  		 siGetUartBaudrate
 *  		 siSetLed
 *   2) Preliminary error logging support. Added siSetDebugLogLevel(Byte bCellNo, Byte bLogLevel)
 *      to enable/disable logging (bLogLevel=0=disable). Logs are written to /var/log/slotN.log, 
 *      where N=slot number
 *   3) Changed some of the hard-coded values to #defines.
 *  Dec-20-2010, MG:
 *   1) Added API for bulk read - siBulkReadFromDrive().
 *   2) FPGA screeching halt - With FPGA 4.11, if an error is encountered during transmission to drive,
 *      it will halt transmission and sets a status bit. Wrapper API is responsible for detecting and
 *      clearing this condition and pushing the error code to Testcase for error handling. Wrapper 
 *      API return values has changed [0=success, 1=Teradyne Error, 2=Drive Ack Timeout, 0x8X from drive].
 *   3) Preliminary coding changes for journaling [SIO level log].
 *   4) Code change and restructuring around logging and tracing..
 * 
 */

/****************************************************************************************
Teradyne Error Codes
--------------------
TER_Status_none                   
TER_Status_dio_timeout            
TER_Status_socket_not_open        
TER_Status_slotid_error           
TER_Status_Invalid_Mode - DUT power off or serial port not enabled.
TER_Status_Error_Argument_Invalid - bufferLength < 0 or > buffer size 
TER_Status_receive_packet_len_err 
TER_Status_invalid_reset_type     
TER_Status_data_length_error                 
TER_Status_send_buffer_already_in_progress 
TER_Status_not_safe_to_transmit   
TER_Status_no_room_in_output_silo 
TER_Status_file_not_found         
TER_Status_cant_open_file         
TER_Status_cant_read_file         
TER_Status_ucode_update_errors    
TER_Status_unknown_status         
TER_Status_Buffer_Overflow                 
TER_Status_drive_power_off        
TER_Status_dio_sequence_error     
TER_Status_rpc_fail               
TER_Status_Timeout                
TER_Status_NotImplemented         
TER_Status_Read_Only_Value           
TER_Status_Invalid_Baud_Rate      
TER_Status_Invalid_Stop_Bits      
TER_Status_Not_Supported          
TER_Status_No_Target_Temp         
TER_Status_ucode_upgrade_busy     
TER_Status_dio_invalid_request    
TER_Status_ucode_file_too_big     
TER_Status_no_client_connection   
TER_Status_high_ucode_ugrade_cnt 
****************************************************************************************/

 

#ifndef _LIBSI_H_
#define _LIBSI_H_

#include <pthread.h>
#include "tcTypes.h"


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
 *   Clear FPGA Buffers
 * Arguments:
 *   integer indicating whether to clear the FPGA's transmit or receive buffer
 *
 *   0:		TER_ResetType_Reset_SendBuffer		= Reset the FPGA transmit buffer
 * 	 1:		TER_ResetType_Reset_ReceiveBuffer	= Reset the FPGA receive buffer
 *
 * Return:
 *   zero if no error. otherwise, error
 * </pre>
 *****************************************************************************/
Byte clearFPGABuffers(Byte bCellNo, int bufferId);

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
   *    zero if success. otherwise, non-zero value.
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
   * Return:
   *   zero if no error, Otherwise, error.
   * Note:
   *   wErrorStatus bit assign is determined by each hardware - supplier should provide bit field definition.
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
  Byte getSafeHandlingTemperature(Byte bCellNo, Word * wTempInHundredth); // cjb added * 22Jun10

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
   *                                    range is 0 -to- 127 (0-127msec)
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
  Byte setVoltageInterval(Byte bCellNo, int wTimeFromV5ToV12InMsec);

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
   *   set slot temperature envelope to make sure the temperature in control
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wEnvelopeTempInTenth - INPUT - can be set to any value you desire (though 0 would be silly)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   slot firmware will monitor temperature in given envelope range.
   *   if the temperature is out, then firmware set 'warning' flag (not 'error') to notify 
   *   testcase that should make appropriate action. The action could be terminate testing
   *   (temperature sensitive test), or just ignore (temperature non-sensitive test). 
   *   the warning flag shall be 'latched' in the firmware. 
   *
   *   Power ON default EnvelopeTemp chosen quite high to avoid any confusing the warning to 
   *   testcase that does not really care temperature (eg 100 (10 degrees))
   * </pre>
   ***************************************************************************************/
  Byte setTemperatureEnvelope( Byte bCellNo, Word wEnvelopeTempInTenth);

  /**
   * <pre>
   * Description:
   *   get slot temperature envelope
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   *wEnvelopeTempInTenth - OUTPUT - envelop temperature in tenth
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getTemperatureEnvelope( Byte bCellNo, Word *wEnvelopeTempInTenth);

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
   *
   *   setting zero value at drive power supply on to achieve 'disable staggered spin up' 
   *   in SATA standard specification. 
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


  /**
   * <pre>
   * Description:
   *   Gets the on-temperature status
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   On-temperature criteria is target temperature +/- 0.5 degreeC
   * </pre>
   *****************************************************************************/
  Byte isOnTemp(Byte bCellNo);

  /**
   * <pre>
   * Description:
   *   Set drive fan speed 
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   DriveFanRPM - INPUT - fan speed in RPM
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   *****************************************************************************/
  Byte siSetDriveFanRPM(Byte bCellNo, int DriveFanRPM);

  /**
   * <pre>
   * Description:
   *   Set slot electronics fan speed 
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   ElectronicsFanRPM - INPUT - fan speed in RPM
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   *****************************************************************************/
  Byte siSetElectronicsFanRPM(Byte bCellNo, int ElectronicsFanRPM);

  /**
   * <pre>
   * Description:
   *   Set drive/slot electronics fan speed at default
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   changing fan speed affects drive vibration and temperature condition so much, 
   *   and hardly detect it in software. therefore it is dangerous.
   *   normal user should call this function instead of siSetDriveFanRPM or 
   *   siSetElectronicsFanRPM that can set wrong parameters. 
   * </pre>
   *****************************************************************************/
  Byte siSetFanRPMDefault(Byte bCellNo);

  /**
   * <pre>
   * Description:
   *   Set drive power supply over voltage protection level
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5LimitInMilliVolts - INPUT - voltage limites in milli voltage
   *   wV12LimitInMilliVolts - INPUT - voltage limites in milli voltage
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   this value applies slot hardwware logic to set over voltage safety interlock. 
   * </pre>
   *****************************************************************************/
  Byte siSetSupplyOverVoltageProtectionLevel(Byte bCellNo, Word wV5LimitInMilliVolts, Word wV12LimitInMilliVolts);

  /**
   * <pre>
   * Description:
   *   Set slot LED
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   bMode - INPUT - LED color (0:Off, 1:On)
   * Return:
   *   zero if success. otherwise, non-zero value.
   * Note:
   *   testcase usually turn on LED at the beginning of the test, then off at the end
   * </pre>
   *****************************************************************************/
  Byte siSetLed(Byte bCellNo, Byte bMode);

  /**
   * <pre>
   * Description:
   *   Used to make sure that the slot is functioning properly and running the coorect firmware
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte isSlotThere(Byte bCellNo);

  /**
   * <pre>
   * Description:
   *   Used to write to FPGA register WARNING: You can crash the SIO2_Application if 
   *   you use an incorrect address.
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
   
  /**
   * <pre>
   * Description:
   *   Used to set inter character delay in bit times.Programmable from 0 to 4095 bit intervals 
   *   For example, at a baud rate of 1.8423 Mbps, the bit interval is 542 nanoseconds.
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wValInBitTimes - INPUT - inter character delay in bit times
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siSetInterCharacterDelay ( Byte bCellNo, Dword wValInBitTimes );

  /**
   * <pre>
   * Description:
   *   Used to read inter character delay in bit times. 
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wValInBitTimes - OUTPUT - inter character delay in bit times
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siGetInterCharacterDelay ( Byte bCellNo, Dword *wValInBitTimes );

  /**
   * <pre>
   * Description:
   *   Used to enable trace level
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   bLogLevel - INPUT - 0 - 4,
   *      Log Level 0: Disabled. 
   *      Log Level 1 (Default): Only Wrapper Error Messages (TLOG_ERROR). 
   *      Log Level 2: All Wrapper Messages (TLOG_ERROR , TLOG_ENTRY, TLOG_EXIT, TLOG_TRACE)
   *      Log Level 3: a) All Wrapper Messages (TLOG_ERROR , TLOG_ENTRY, TLOG_EXIT, TLOG_TRACE) b) SIO Journal on error (100 SIO Journal entries)
   *      Log Level 4: a) All Wrapper Messages (TLOG_ERROR , TLOG_ENTRY, TLOG_EXIT, TLOG_TRACE) b) SIO Journal on error (100 SIO Journal entries) c) SIO Journal in siFinalize()
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siSetDebugLogLevel(Byte bCellNo, Byte bLogLevel);


  /**
   * <pre>
   * Description:
   *   Retrives and prints last error messages from SIO
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siGetLastErrors(Byte bCellNo);

  /**
   * <pre>
   * Description:
   *   Clears halt on error condition
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siClearHaltOnError(Byte bCellNo);

  /**
	 * <pre>
	 * Description:
	 *   Bypasses UART2 and and read from drive UART.
	 * Arguments:
	 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
	 *   *wSize - OUTPUT - number of bytes read
	 *   *bData - INPUT - pointer to data
	 *   wTimeoutInMillSec - INPUT - time out value in mill-seconds
	 * Return:
	 *   zero if success. otherwise, non-zero value.
	 * </pre>
	 *****************************************************************************/
  Byte siBulkReadFromDrive(Byte bCellNo, Word *wSize, Byte *bData, Word wTimeoutInMillSec);
  
  
  /**
   * <pre>
   * Description:
   *   Bypasses UART2 and write given data to drive UART.
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wSize - INPUT - number of bytes to be written
   *   *bData - INPUT - pointer to data
   *   wTimeoutInMillSec - INPUT - time out value in mill-seconds
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   *****************************************************************************/
  Byte siBulkWriteToDrive(Byte bCellNo, Word wSize, Byte *bData, Word wTimeoutInMillSec);

  /**
   * <pre>
   * Description:
   *   Used to set inter character delay in bit times.
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wValue - INPUT - Ack timeout [0=100ms, 1=500ms, 2=1000ms, 3=2000ms]
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siSetAckTimeout ( Byte bCellNo, Dword wValue );
  
   /**
   * <pre>
   * Description:
   *   Used to read inter character delay in bit times. 
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wValue - OUTPUT - Ack timeout [0=100ms, 1=500ms, 2=1000ms, 3=2000ms]
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siGetAckTimeout ( Byte bCellNo, Dword *wValue ); 
  
   /**
   * <pre>
   * Description:
   * Sets the delay between packets from 0 to 2550 usec in 10 usec increments.  
   * The default is 30 (300 usec).  This is the time from when the acknowledge 
   * is received from the drive until when the UART sends the next character.   
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wValue - OUTPUT - Inter packet delay in us [0..2550]. 10 us usec increments
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte siSetInterPacketDelay ( Byte bCellNo, Dword wValue ); 

  /**
   * <pre>
   * Description:
   *   Gets the current readings 
   * Arguments:
   *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
   *   wV5InMilliAmps - OUTPUT - 
   *   wV12InMilliAmps - OUTPUT - 
   * Return:
   *   zero if success. otherwise, non-zero value.
   * </pre>
   ***************************************************************************************/
  Byte getActualCurrent(Byte bCellNo,Word *wV5InMilliAmps, Word *wV12InMilliAmps);

 /**
  * <pre>
  * Description:
  *   Change To UART3 command
  * Arguments:
  *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
  *   wLineSpeed - INPUT - Line Speed (See Table 34 in UART2 Spec. for values)
  *   wTimeoutInMillSec - INPUT - time out value in mill-seconds
  * Return:
  *   zero if success. otherwise, non-zero value.
  * Note:
  * </pre>
  *****************************************************************************/
  Byte siChangeToUart3(Byte bCellNo, Word wLineSpeed, Word wTimeoutInMillSec);
  
 /**
  * <pre>
  * Description:
  *   Testcase calls this function periodically during the test to correct
  *   any deviation of  slot temperature from drive's internal temperature.
  *    
  * Arguments:
  *   bCellNo - INPUT - cell id
  *   wTempInHundredth - INPUT -  drive's temperature in hundredth of a degree C
  * Return:
  *   zero if success. otherwise, non-zero value.
  * </pre>
  ***************************************************************************************/
 Byte adjustTemperatureControlByDriveTemperature(Byte bCellNo, Word wTempInHundredth);

#ifdef __cplusplus
}
#endif

#endif
