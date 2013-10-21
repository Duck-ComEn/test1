/** @mainpage
 * @ref MFG
 */

/** @file uart2Interface.cpp
 * @n The file contains the implementation of the Hitachi Interface functions
 * in terms of Teradyne @c TER_ API functions.
 *
 * @n The functions required by Hitachi are prototyped in @ref libsi.h.
 * 
 * @cond
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
 *   8) Added siGetLastErrors for debugging SIO errors. This will be called in
 *      terError(__FUNCTION__, bCellNo, ns2, status)().
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
 *      API return values has changed [0=success, 1=Teradyne Error, 2=Drive Error].
 *   3) Preliminary coding changes for journaling [SIO level log].
 *   4) Code change and restructuring around logging and tracing..
 *  Dec-23-2010, MG:
 *   1) Incorporated changes from Katayama-san
 *   2) Changed the name of hitachi test utility to a more meaningful name (wapitest) from (hitachi2)
 *   3) Added preliminary coding for 64K FIFO support
 *   4) Added SIO journal support. SIO Journal should be enabled by siSetDebugLogLevel(). SIO Journal will
 *      be printed to a log at the end of a test (siFinalize)
 *  Jan-11-2011, MG:
 *  1) Supported Phase 3 APIs -  getHeaterOutput, getCellInventory, getReferenceTemperature, siGetDriveUartVersion, 
 *     siResetDrive,siSetDriveDelayTime, setHeaterOutput, getShutterPosition, setShutterPosition, 
 *     setTemperatureSensorCarlibrationData, setVoltageCalibration, siSetDriveFanRPM, siSetElectronicsFanRPM
 *  2) Comment out isSlotThere() from siInitialize()
 *  Feb-01-2011, MG:
 *  1) High UART2 failures were related to smaller turnaround delay (15us) after we receive good response (0x80) from the drive.
 *     This releases contains new FPGA (40.1) with fixed 300us (same as inter character delay for EB7 drives) turnaround delay.
 *  2) Added declaration of mutex_slotio as requested by Katayama-san.
 *  3) Added calls to set inter character delay, ack timeout and logging enable in siInitialize, as requested by Katayama-san
 *  4) Enabled isSlotThere() in siInitialize.
 *  5) Fixed setSafeHandlingTemperature(). This call was always failing before.
 *  6) respLength was not correctly set in TER_SioReceiveBuffer call. Because of this echo and read commands were taking longer
 *     time than expected. Fixed this in many APIs.
 *  7) Testcase calls many APIs with 30 secs as timeout value. SIO API supports timeouts only up to 10secs. Added code to truncate
 *     timeouts to 10sec.
 *  8) Initial code for SIO/DIO status and temp envelope. Code exists, but commented out.
 *  Feb-08-2011, MG:
 *  1) Convert 0 based slot indices to 1 based
 *  2) Implementation of SIO/DIO state detection.
 *  Feb-11-2011, MG:
 *  1) Temperature Envelope support.
 *  2) Enhanced logging - Log return values for function calls where return values are in function parameters.
 *  3) Validate setTargetTemperature, setPositiveTemperatureRampRate, setNegativeTemperatureRampRate by reading back from SIO/DIO
 *  Feb-28-2011
 *  1) setVoltageRiseTime & getVoltageRiseTime
 *  2) Added terPrintSlotInfo() print version info and serial numbers. 
 *  3) Re-implement siSetDriveFanRPM with real SIO API.
 *  4) TLOG_EXIT now prints function name.
 *  5) 12V control for Enterprise
 *  6) Fixed truncation errors in the case x100 numbers. Rounding up instead.
 *  7) In case of RPC errors, disconnect, reconnect and retry failed command X times.
 *  8) SIO Journal related changes a) Disable logging by default b) New API which is slot specific c) Print SIO Journal only on error.
 *  9) Implemented getActualCurrent() to read actual current for 5V and 12V
 * 10) Changed Logging - Log file for a slot is not one big file any more. A log file will be called siSetDebugLogLevel() is called.
 *     Date and time stamps are used in the log file names [Example: slot10_2011.03.09.10.55.56.log]
 * 11) Fixed ptential memory leak in Neptune SIO2 lib.
 * March-28-2011, MG
 * 1) Optimized 64K support.
 * 2) Multi-level logging.
 * 3) Display 924x version info. Fix DIO/SIO part number display. Added DIO board type.
 * 4) Slot Error handling. If any slot error, return code will be terError from siInitialize(), getVoltageErrorStatus() & getTemperatureErrorStatus().
 * 5) tcLogger integration - testcase will be required to compile switch WAPI_TCLOGGER_ENABLE switch in makefile.
 * 6) Implementation of setVoltageInterval().
 * April-21-2011, MG
 * 1) 12V rise/fall time support
 * April-25-2011, MG
 * 1) Add support for row based temp compensation [conditionally disabled in this release]
 * 2) Htemp correction algorithm
 * May-13-2011, MG
 * 1) Add range check for htemp algrithm as protection from wrong htemp readings.
 * July-3-2011, KRG
 * 1) Fixed HTemp related bugs. Moved implementation to neptune_sio2 library
 * July-7-2011, KRG
 * 1) Change the log file name to have slot id as three digit number and the extension to be text files
 * @endcond
 */

// --- System Library Interfaces
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>                     /*for neptune_sio2.h*/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>                  /*for neptune_sio2.h*/
#include <syslog.h>
#include <unistd.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Teradyne Library Interfaces
#include "common.h"
#include "commonEnums.h"
#include "libsi.h"
#include "terSioLib.h"
#include "tcTypes.h"
#include "ver.h"

#define WAPI_READ_CMD_OVERHEAD_BYTES    18                                         /* Actual 15 bytes */
#define WAPI_MAX_BYTES_FROM_SMALL_BUFF  (2048 - WAPI_READ_CMD_OVERHEAD_BYTES)
#define WAPI_SEND_RECV_BUF_SIZE 		(2048 + WAPI_READ_CMD_OVERHEAD_BYTES)
#define WAPI_SEND_RECV_BIG_BUF_SIZE     (65536 + WAPI_READ_CMD_OVERHEAD_BYTES)
#define WAPI_CMD_RESP_BUF_SIZE 		    1024
#define WAPI_HITACHI_SPECIAL_ENABLE      1
#define WAPI_64K_MAX_PENDING_PER_FPGA_DEFAULT    12
#define WAPI_TEMP_ADJ_TARGET_TEMP_MIN    4500   /* In deg C * 100. Temp adjustment is done only if target temp is greater than this value */
#define WAPI_TEMP_ADJ_OFFSET_MAX         500    /* In deg C * 100. maximum +/- offset read from csv file */
#define UART_PULLUP_VOLTAGE_DEFAULT       2700

/* Hitachi global variables */
pthread_mutex_t mutex_slotio;

/*
 * Teradyne global variables
 */
static int siTargetVoltage5VinMilliVolts = 0;
static int siTargetVoltage12VinMilliVolts = 0;
static int siUartPullupVoltageInMilliVolts = 0;
static int si64kMaxPendingPerFPGA = WAPI_64K_MAX_PENDING_PER_FPGA_DEFAULT;

static Word siV5LowerLimitInMilliVolts = 0;
static Word siV12LowerLimitInMilliVolts = 0;
static Word siV5UpperLimitInMilliVolts = 0;
static Word siV12UpperLimitInMilliVolts = 0;

static Word siSetTempLowerInHundredth = 0;
static Word siSetTempUpperInHundredth = 0;
static Word siSensorTempLowerInHundredth = 0;
static Word siSensorTempUpperInHundredth = 0;

static Word siTemperatureTarget = 0;

/* Debug Flag */
static Byte bDebugCellNoCCB = 0;
static Byte bDebugFlagCCB = 0;

Byte siGetCellNoDebugCCB(void)
{
  return bDebugCellNoCCB;
}

void siSetCellNoDebugCCB(Byte bCellNo)
{
  bDebugCellNoCCB = bCellNo;
  return;
}

void siEnableDebugFlagCCB(void)
{
  bDebugFlagCCB = 1;
}

void siDisableDebugFlagCCB(void)
{
  bDebugFlagCCB = 0;
}

Byte isDebugFlagCCB(void)
{
  return bDebugFlagCCB;
}


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
Byte siInitialize(Byte bCellNo) {
	Byte rc = success; /* No error */
	TER_Status status;
	TER_SlotInfo pSlotInfo;
	int attemptCnt = 2;
	char build_version[MAX_LIB_VERSION_SIZE];
	TER_SlotUpgradeStatus upgradeSlotStatus;

	// Check if the slot index is in valid range
	if (bCellNo < SLOT_INDEX_MIN || bCellNo > SLOT_INDEX_MAX) {
		RET(invalidSlotIndex);
	}

    siSetDebugLogLevel(bCellNo, 3);
        
	/* Establish connection with SIO */
	for (attemptCnt = 1; attemptCnt <= 2; ++attemptCnt) {
		rc = Common::Instance.TerSioConnect();
		if (rc == 0) {
			break;
		}
	}

	// Successfully connected to the SIO
	Common::Instance.SetSlotIndex(bCellNo);

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	// After all attempts if the connection is not made then throw an error
	if (rc != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: siInitialized failed with status 0x%x \n", rc);
		RET(rc);
	}

	// Check if the DIO is present
	WAPI_CALL_RPC_WITh_RETRY(
			status =
					(Common::Instance.GetSIOHandle())->TER_GetSlotUpgradeStatus(
							bCellNo, &upgradeSlotStatus));
	if (status != TER_Status_none) {
		char errorString[MAX_ERROR_STRING_LENGTH];
		(Common::Instance.GetSIOHandle())->TER_ConvertStatusToString(status,
				errorString);
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: siInitialize failed because call to TER_IsDioPresent failed with status = %d (%s) \n",
				status, errorString);
		RET(Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared));
	}

	// DIO is present. Check whether firmware upgrade is recommended
	if (DP_FLAGS_upgrade_recommended & upgradeSlotStatus.upgradeFlags) {
		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_UpgradeSlot(bCellNo));
		if (status != TER_Status_none) {
			char errorString[MAX_ERROR_STRING_LENGTH];
			(Common::Instance.GetSIOHandle())->TER_ConvertStatusToString(
					status, errorString);
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: siInitialize failed while updating DIO firmware failed with status = %d (%s) \n",
					status, errorString);
			RET(Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared));
		}
	} else if (DP_FLAGS_timeout & upgradeSlotStatus.upgradeFlags) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: siInitialize failed since DIO is not present \n");
		RET(dioNotResponsive);
	} else if (DP_FLAGS_invalid_dio_tpn & upgradeSlotStatus.upgradeFlags) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: siInitialize failed. DIO has invalid part number so failed to update the DIO\n");
		RET(dioInvalidTPN);
	}

	/* clear slot errors if any. This also does DIO soft reset (slot reset) */
	/* Certain wapitest command line will not work if slot reset is called and hence this code */
	if (Common::Instance.GetWapitestResetSlotDuringInitialize()) {
		rc = clearCellEnvironmentError(bCellNo);
		if (rc) {
			Common::Instance.WriteToLog(LogTypeError,
					"ERROR: clearCellEnvironmentError \n");
			RET(rc);
		}

		/* now check if any slot errors remain, if so stop here */
		rc = isCellEnvironmentError(bCellNo);
		if (rc) {
			Common::Instance.WriteToLog(LogTypeError,
					"ERROR: isCellEnvironmentError returned error 0x%x \n", rc);
			RET(rc);
		}
	} else {
		/* Do not call clearCellEnvironmentError, clear wapi test mode */
		Common::Instance.SetWapitestResetSlotDuringInitialize(1);
	}

	/* clear screeching halt */
	rc = siClearHaltOnError(bCellNo);
	if (rc) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: siClearHaltOnError \n");
		RET(rc);
	}

#if WAPI_HITACHI_SPECIAL_ENABLE
	rc = siSetInterCharacterDelay(bCellNo, 546);
	if (rc) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: siSetInterCharacterDelay failed \n");
		RET(rc);
	}
	rc = siSetAckTimeout(bCellNo, 3);
	if (rc) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: siSetAckTimeout failed \n");
		RET(rc);
	}
#endif

	/* Init global vars, log, check slot errors, print DIO/SIO/FPGA Rev, Library rev */
	siTargetVoltage5VinMilliVolts = 0;
	siTargetVoltage12VinMilliVolts = 0;
	siUartPullupVoltageInMilliVolts = 0;

	/* Print serial no/version */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotInfo(bCellNo, &pSlotInfo));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		RET(Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared));
	} else {
		Common::Instance.WriteToLog(
				LogTypeTrace,
				"DioFirmwareVersion: <DIO App:%s> <924x Init:%d.%d> <924x App:%d.%d>\n",
				pSlotInfo.DioFirmwareVersion, upgradeSlotStatus.powerInitMajorVer,
				upgradeSlotStatus.powerInitMinorVer, upgradeSlotStatus.powerFwMajorVer, upgradeSlotStatus.powerFwMinorVer);
		Common::Instance.WriteToLog(LogTypeTrace, "DioPcbSerialNumber: <%s>\n",
				pSlotInfo.DioPcbSerialNumber);
		Common::Instance.WriteToLog(LogTypeTrace, "SioFpgaVersion: <%s>\n",
				pSlotInfo.SioFpgaVersion);
		Common::Instance.WriteToLog(LogTypeTrace, "SioPcbSerialNumber: <%s>\n",
				pSlotInfo.SioPcbSerialNumber);
		Common::Instance.WriteToLog(LogTypeTrace, "SioTeradynePartNum: <%s>\n",
				pSlotInfo.SioPcbPartNum);
		Common::Instance.WriteToLog(LogTypeTrace, "DioTeradynePartNum: <%s>\n",
				pSlotInfo.SlotPcbPartNum);

		rc = (Common::Instance.GetSIOHandle())->TER_GetLibraryVersion(build_version);
		if (TER_Status_none != status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			RET(
					Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared));
		}
		Common::Instance.WriteToLog(LogTypeTrace,
				"Neptune SIO Lib: %s\n", build_version);
		Common::Instance.WriteToLog(LogTypeTrace,
				"SIO2 Super App (SIO2_SuperApplication): %s\n",
				pSlotInfo.SioAppVersion);
		Common::Instance.WriteToLog(LogTypeTrace, "WAPI Lib (libsi.a): %s\n",
				terGetWapiLibVersion());
	}

	char *str64kMaxPendingPerFPGA = NULL;

	si64kMaxPendingPerFPGA = WAPI_64K_MAX_PENDING_PER_FPGA_DEFAULT;

	str64kMaxPendingPerFPGA = getenv("WAPI_64K_MAX_PENDING_PER_FPGA");
	if (str64kMaxPendingPerFPGA != NULL)
	{
		if ((isdigit(str64kMaxPendingPerFPGA[0]))
				&& (isdigit(str64kMaxPendingPerFPGA[1])
						|| str64kMaxPendingPerFPGA[1] == '\0')) {
			si64kMaxPendingPerFPGA = atoi(str64kMaxPendingPerFPGA);
			si64kMaxPendingPerFPGA =
					si64kMaxPendingPerFPGA > 35 ? 35 : si64kMaxPendingPerFPGA;
		}
	}

	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, rc=%d \n",
			__FUNCTION__, rc);

	RET(rc);
}

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
Byte siFinalize(Byte bCellNo) {
	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	Byte rc = Common::Instance.SanityCheck(bCellNo);
	if (rc != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x",
				__FUNCTION__, rc);
		// If the SIO is not connected then we don't have to do anything
		if (rc == sioNotConnected) {
			goto FUNCTIONEND;
		}

		// If the cell no is invalid then just disconnect from SIO
		Common::Instance.TerSioDisconnect();
		goto FUNCTIONEND;
	}

	Common::Instance.TerSioDisconnect();

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, rc=0 \n",
			__FUNCTION__);
	return (Byte) 0;
}

/**
 * <pre>
 * Description:
 *   Get current notifier mask
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   mask - OUTPUT - Notifier mask
 * Return:
 *   zero if no error. otherwise, error
 * </pre>
 *****************************************************************************/
Byte siGetNotifierMask(Byte bCellNo, TER_Notifier* mask) {
	Byte rc = success;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d)\n", __FUNCTION__,
			bCellNo);

	if (mask == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*mask = Common::Instance.GetSlotErrorsMask();

FUNCTIONEND:
	Common::Instance.WriteToLog(
			LogTypeTrace,
			"Exit: %s(%d), mask = 0x%lx\n",	__FUNCTION__, bCellNo, *mask);
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Enable/Disable masking of a particular error
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   mask - INPUT - error to add to current mask
 *   enable - True:  Enble masking of the error, False: disable masking of the error
 * Return:
 *   zero if no error. otherwise, error
 * </pre>
 *****************************************************************************/
Byte siSetNotifierMask(Byte bCellNo, TER_Notifier mask, int enable) {
	Byte rc = success;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d)\n", __FUNCTION__,
			bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}
	if ((rc = Common::Instance.SetSlotErrorsMask(mask, enable)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Unable to set notifier mask x%lx, rc = 0x%x", __FUNCTION__, mask, rc);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(
			LogTypeTrace,
			"Exit: %s(%d), mask = 0x%lx\n",	__FUNCTION__, bCellNo, mask);
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   return last error cause
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   errorCause - OUTPUT - returns the description of the error
 * Return:
 *   zero if no error. otherwise, error
 * </pre>
 *****************************************************************************/
Byte siGetLastErrorCause(Byte bCellNo, char* lastErrorCause,
		Byte *lastErrorCode) {

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d)\n", __FUNCTION__,
			bCellNo);

	if ((lastErrorCause == NULL) || (lastErrorCode == NULL)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		return nullArgumentError;
	}

	Common::Instance.GetErrorString(bCellNo, lastErrorCause, lastErrorCode);

	Common::Instance.WriteToLog(
			LogTypeTrace,
			"Exit: %s(%d), Last error cause = %s, Last error code = 0x%x, exit code = 0x%x\n",
			__FUNCTION__, bCellNo, lastErrorCause, *lastErrorCode, success);
	RET(success);
}

/**
 * <pre>
 * Description:
 *   Clear FPGA Buffers
 * Arguments:
 *   bCellNo  -  INPUT - cell id
 *   bufferId -  indicating whether to clear the FPGA's transmit or receive buffer
 *
 *   This will call TER_ResetSlot with:
 *
 *   0:		TER_ResetType_Reset_SendBuffer		= Reset the FPGA transmit buffer
 * 	 1:		TER_ResetType_Reset_ReceiveBuffer	= Reset the FPGA receive buffer
 *
 * Return:
 *   zero if no error. otherwise, error
 * </pre>
 *****************************************************************************/

Byte clearFPGABuffers(Byte bCellNo, fpgaBufferType bufferId) {

	TER_ResetType resetType;
	TER_Status status;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/*
	 * check which buffer should be cleared
	 * any other value than the expected values will
	 * give an error code
	 *
	 */

	switch(bufferId){

	case fpgaBufferTypeSend:
		resetType = TER_ResetType_Reset_SendBuffer;
		break;

	case fpgaBufferTypeReceive:
		resetType = TER_ResetType_Reset_ReceiveBuffer;
		break;

	default:

		Common::Instance.WriteToLog(LogTypeError,
						"ERROR: %s - Invalid bufferId was passed, %d\n", __FUNCTION__, bufferId);
		rc = invalidArgumentError;
		goto FUNCTIONEND;
	}
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_ResetSlot(bCellNo, resetType));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
	}

	FUNCTIONEND:
		Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
				__FUNCTION__, rc);
	RET (rc);
}


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
Byte isCellEnvironmentError(Byte bCellNo) {
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotStatus(bCellNo, &slotStatus));
	if (status != TER_Status_none)
	{
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
	} else {
		Common::Instance.maskRequestedErrors (&slotStatus.slot_errors);
		if (slotStatus.slot_errors) {
			Common::Instance.WriteToLog(LogTypeError,
					"ERROR : slot_errors = 0x%0x\n", slotStatus.slot_errors);
			rc = Common::Instance.GetWrapperErrorCode(TER_Status_none,
					(TER_Notifier) slotStatus.slot_errors);
		}
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x, slot errors = 0x%0x\n", __FUNCTION__,
			rc, slotStatus.slot_errors);
	RET(rc);
}

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
Byte clearCellEnvironmentError(Byte bCellNo) {
	TER_Status status;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_ResetSlot(bCellNo, TER_ResetType_Reset_Slot));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte getCurrentTemperature(Byte bCellNo, Word *wTempInHundredth) {
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = success; /* No error */

    Word wTempInHundredthMaxLimit = 8000;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d *) \n",
			__FUNCTION__, bCellNo);

	if (wTempInHundredth == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed \n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wTempInHundredth = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotStatus(bCellNo, &slotStatus));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		/* TER returns in units of 0.1 degree C. While Hitachi
		 * expects unit of 0.01 degree C. Multiply by 10. */
		*wTempInHundredth = (Word) (slotStatus.temp_drive_x10 * 10);
	}

    if ( *wTempInHundredth >= wTempInHundredthMaxLimit){
    	 Common::Instance.WriteToLog(LogTypeError,"ERROR: %s failed because slot temperature over %d degreeC \n",__FUNCTION__,wTempInHundredthMaxLimit/100);
    	 rc = terError;
    }

FUNCTIONEND:
	if (wTempInHundredth == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Current Temperature = %d\n",
				__FUNCTION__, rc, *wTempInHundredth);
	}
	RET(rc);
}

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
Byte setTargetTemperature(Byte bCellNo, Word wTempInHundredth) {
	targetTemperature tt;
	int tempTarget_x10, coolRampRate_x10_perMinute, heatRampRate_x10_perMinute;
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d) \n",
			__FUNCTION__, bCellNo, wTempInHundredth);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Enable temperature control */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetTempControlEnable(bCellNo, 1));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	/* Read current ramp rates from SIO */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	heatRampRate_x10_perMinute = (Word) (slotSettings.temp_ramp_rate_heat_x10);
	coolRampRate_x10_perMinute = (Word) (slotSettings.temp_ramp_rate_cool_x10);
	tempTarget_x10 = (wTempInHundredth + 5) / 10;

	tt.temp_target_x10 = tempTarget_x10;
	tt.cool_ramp_rate_x10 = coolRampRate_x10_perMinute;
	tt.heat_ramp_rate_x10 = heatRampRate_x10_perMinute;
	tt.cooler_enable = 1;
	tt.heater_enable = 1;

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetTargetTemperature(bCellNo, tt));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	/* Read back and validate */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	if ((tt.temp_target_x10 != tempTarget_x10)
			|| (tt.cool_ramp_rate_x10 != coolRampRate_x10_perMinute)
			|| (tt.heat_ramp_rate_x10 != heatRampRate_x10_perMinute)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s failed validation \n", __FUNCTION__);
		rc = terError;
		goto FUNCTIONEND;
	}

	siTemperatureTarget = wTempInHundredth; /* Save this value for htemp calculation */

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte getTargetTemperature(Byte bCellNo, Word *wTempInHundredth) {
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*) \n",
			__FUNCTION__, bCellNo);

	if (wTempInHundredth == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wTempInHundredth = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none)
	{
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		/* TER returns in units of 0.1 degree C. While Hitachi
		 * expects unit of 0.01 degree C. Multiply by 10. */
		*wTempInHundredth = (Word) (slotSettings.temperature_target_x10 * 10);
	}

FUNCTIONEND:
	if (wTempInHundredth == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Target Temperature = %d\n",
				__FUNCTION__, rc, *wTempInHundredth);
	}
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Returns a trace of the last 4 events performed via the SIO in a chronological order.
 *   Each event indicates the beginning or the end of a transaction. The first transaction in the array is the oldest transaction
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   eventTrace[] - OUTPUT - An array of MAX_TRANSACTION_TRACE elements indicating the type and time stamp of each transaction
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte getTimeOfUartEvent(Byte bCellno, UART_EVENT (*event)[4]) {
	TER_Status status;
	TER_TransactionTrace transactionTrace[MAX_TRANSACTION_TRACE];
	Byte rc = success; /* no error */

	Common::Instance.WriteToLog(LogTypeTrace, "entry: %s(%d,*) \n",
			__FUNCTION__, bCellno);

	if (event == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"error: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
        }

	if ((rc = Common::Instance.SanityCheck(bCellno)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	if (MAX_TRANSACTION_TRACE > 4) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - MAX_TRANSACTION_TRACE is too big 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetTransactionTrace(bCellno, &(transactionTrace[0])));
	if (status != TER_Status_none)
	{
		Common::Instance.LogError(__FUNCTION__, bCellno, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		for (int ii = 0; ii < MAX_TRANSACTION_TRACE; ii++) {
			(*event)[ii].event_code = transactionTrace[ii].eventCode;
                        strcpy((char *)(*event)[ii].event_code_description, transactionTrace[ii].eventCodeDescription);
			(*event)[ii].error_code = transactionTrace[ii].errorCode;
			(*event)[ii].num_bytes_requested = transactionTrace[ii].bytesToTransfer;
			(*event)[ii].num_bytes = transactionTrace[ii].transferredBytes;
			(*event)[ii].tv_start_sec = transactionTrace[ii].tv_start_sec;
			(*event)[ii].tv_start_usec = transactionTrace[ii].tv_start_usec;
			(*event)[ii].tv_end_sec = transactionTrace[ii].tv_end_sec;
			(*event)[ii].tv_end_usec = transactionTrace[ii].tv_end_usec;
		}
 	}

FUNCTIONEND:
	if (event == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		for (int ii = 0; ii < MAX_TRANSACTION_TRACE; ii++) {
			int seconds = 0;
			int microsecs = 0;
			// calculate elapsed time since last transaction
			if (ii) {
				seconds = (*event)[ii].tv_start_sec - (*event)[ii-1].tv_end_sec;
				microsecs = (*event)[ii].tv_start_usec - (*event)[ii-1].tv_end_usec;
		        	if (microsecs <  0) {
		        		seconds --;
		        		microsecs += 1000000;
		        	}
			}
			char description [GET_DIAG_INFO_LEN_64];
			sprintf (description, "(%s),",(*event)[ii].event_code_description);
			Common::Instance.WriteToLog(LogTypeError, " 0x%0x %-18s %d bytes %d bytes,\ttimeStart: %d.%06d, timeEnd: %d.%06d, time elapsed since last op: %d.%06d sec\n",
					(*event)[ii].event_code, description, (*event)[ii].num_bytes_requested, (*event)[ii].num_bytes,
					(*event)[ii].tv_start_sec, (*event)[ii].tv_start_usec,
					(*event)[ii].tv_end_sec, (*event)[ii].tv_end_usec, seconds, microsecs);
		}
		Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	}
	RET(rc);
}

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
Byte getReferenceTemperature(Byte bCellNo, Word *wTempInHundredth) {
	return getTargetTemperature(bCellNo, wTempInHundredth);
}

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
Byte getHeaterOutput(Byte bCellNo, Word *wHeaterOutputInPercent) {
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*) \n",
			__FUNCTION__, bCellNo);

	if (wHeaterOutputInPercent == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wHeaterOutputInPercent = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotStatus(bCellNo, &slotStatus));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		*wHeaterOutputInPercent = (Word) (slotStatus.heat_demand_percent);
	}

FUNCTIONEND:
	if (wHeaterOutputInPercent == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Current Heater Output = %d\%\n",
				__FUNCTION__, rc, *wHeaterOutputInPercent);
	}
	RET(rc);
}

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
Byte setHeaterOutput(Byte bCellNo, Word wHeaterOutputInPercent) {

	/* This function is not supported */
	RET(success);
}

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
Byte getShutterPosition(Byte bCellNo, Word *wShutterPositionInPercent) {
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s (%d)\n", __FUNCTION__,
			bCellNo);

	if (wShutterPositionInPercent == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wShutterPositionInPercent = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* return cooling demand instead!! */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotStatus(bCellNo, &slotStatus));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		*wShutterPositionInPercent = (Word) (slotStatus.cool_demand_percent);
	}

FUNCTIONEND:
	if (wShutterPositionInPercent == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Shutter Position = %d\%\n",
				__FUNCTION__, rc, *wShutterPositionInPercent);
	}
	RET(rc);
}

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
Byte setShutterPosition(Byte bCellNo, Word wShutterPositionInPercent) {

	/* FUNCTION IS NOT SUPPORTED */
	RET(success);
}

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
Byte getTemperatureErrorStatus(Byte bCellNo, Word *wErrorStatus) {
	TER_Status status = TER_Status_none;
	TER_SlotStatus slotStatus;
	int slotErrors = 0;
	int tempErrors = 0;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n",
			__FUNCTION__, bCellNo);

	if (wErrorStatus == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	/* Initialize to no error condition */
	*wErrorStatus = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotStatus(bCellNo, &slotStatus));
	if (status != TER_Status_none)
	{
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		Common::Instance.maskRequestedErrors (&slotStatus.slot_errors);
		slotErrors = slotStatus.slot_errors;
		tempErrors = 0;
		tempErrors |= (slotErrors & TER_Notifier_BlowerFault);
		tempErrors |= (slotErrors & TER_Notifier_SledOverTemp);
		tempErrors |= (slotErrors & TER_Notifier_SlotCircuitOverTemp);
		tempErrors |= (slotErrors & TER_Notifier_SledDiodeFault);
		tempErrors |= (slotErrors & TER_Notifier_SledHeaterFaultOpen);
		tempErrors |= (slotErrors & TER_Notifier_SledHeaterFaultShort);
		tempErrors |= (slotErrors & TER_Notifier_TempEnvelopeFault);
		tempErrors |= (slotErrors & TER_Notifier_TempRampNotCompleted);
		*wErrorStatus = (Word) tempErrors;

		/* There's a problem here since Hitachi allocates just 16 bits to
		 * the error status and TER may have error info in the upper 16 bits.
		 * Need to return some true value if the upper 16 bits are true. */
		if (*wErrorStatus == 0 && tempErrors != 0) {
			*wErrorStatus = 0xffff;
			rc = Common::Instance.GetWrapperErrorCode(TER_Status_none,
					(TER_Notifier) slotStatus.slot_errors);
		}
	}

	/* if there are any slot errors, return an error */
	if (slotStatus.slot_errors) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR : slot_errors = 0x%0x\n", slotStatus.slot_errors);
		rc = Common::Instance.GetWrapperErrorCode(TER_Status_none,
				(TER_Notifier) slotStatus.slot_errors);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	if (wErrorStatus == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Slot Errors = 0x%x\n",
				__FUNCTION__, rc, slotStatus.slot_errors);
	}
	RET(rc);
}

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
Byte getPositiveTemperatureRampRate(Byte bCellNo,
		Word *wTempInHundredthPerMinutes) {
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*) \n",
			__FUNCTION__, bCellNo);

	if (wTempInHundredthPerMinutes == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wTempInHundredthPerMinutes = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none)
	{
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		/* TER returns in units of 0.1 deg/min. While Hitachi
		 * expects unit of 0.01 deg/min. Multiply by 10. */
		*wTempInHundredthPerMinutes =
				(Word) (slotSettings.temp_ramp_rate_heat_x10 * 10);
	}

FUNCTIONEND:
	if (wTempInHundredthPerMinutes == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Positive Ramp Rate = %d\n",
				__FUNCTION__, rc, *wTempInHundredthPerMinutes);
	}
	RET(rc);
}

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
Byte setPositiveTemperatureRampRate(Byte bCellNo,
		Word wTempInHundredthPerMinutes) {
	targetTemperature tt;
	int tempTarget_x10, coolRampRate_x10_perMinute, heatRampRate_x10_perMinute;
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d) \n",
			__FUNCTION__, bCellNo, wTempInHundredthPerMinutes);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Read current ramp & target temp from SIO */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	heatRampRate_x10_perMinute = (wTempInHundredthPerMinutes + 5) / 10;
	coolRampRate_x10_perMinute = (Word) (slotSettings.temp_ramp_rate_cool_x10);
	tempTarget_x10 = (Word) (slotSettings.temperature_target_x10);

	tt.temp_target_x10 = tempTarget_x10;
	tt.cool_ramp_rate_x10 = coolRampRate_x10_perMinute;
	tt.heat_ramp_rate_x10 = heatRampRate_x10_perMinute;
	tt.cooler_enable = 1;
	tt.heater_enable = 1;

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetTargetTemperature(bCellNo, tt));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	/* Read back and validate */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	if ((tt.temp_target_x10 != tempTarget_x10)
			|| (tt.cool_ramp_rate_x10 != coolRampRate_x10_perMinute)
			|| (tt.heat_ramp_rate_x10 != heatRampRate_x10_perMinute)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s failed validation \n", __FUNCTION__);
		rc = terError;
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte getNegativeTemperatureRampRate(Byte bCellNo,
		Word *wTempInHundredthPerMinutes) {
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*) \n",
			__FUNCTION__, bCellNo);

	if (wTempInHundredthPerMinutes == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wTempInHundredthPerMinutes = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		/* TER returns in units of 0.1 deg/min. While Hitachi
		 * expects unit of 0.01 deg/min. Multiply by 10. */
		*wTempInHundredthPerMinutes =
				(Word) (slotSettings.temp_ramp_rate_cool_x10 * 10);
	}

FUNCTIONEND:
	if (wTempInHundredthPerMinutes == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Negative Ramp Rate = %d\n",
				__FUNCTION__, rc, *wTempInHundredthPerMinutes);
	}
	RET(rc);
}

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
Byte setNegativeTemperatureRampRate(Byte bCellNo,
		Word wTempInHundredthPerMinutes) {
	targetTemperature tt;
	int tempTarget_x10, coolRampRate_x10_perMinute, heatRampRate_x10_perMinute;
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d) \n",
			__FUNCTION__, bCellNo, wTempInHundredthPerMinutes);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Convert from Hitachi units (0.01 deg/min) to TER (0.1 deg/min). */
	coolRampRate_x10_perMinute = (wTempInHundredthPerMinutes + 5) / 10;

	/* Read current ramp & target temp from SIO */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	coolRampRate_x10_perMinute = wTempInHundredthPerMinutes / 10;
	heatRampRate_x10_perMinute = (Word) (slotSettings.temp_ramp_rate_heat_x10);
	tempTarget_x10 = (Word) (slotSettings.temperature_target_x10);

	tt.temp_target_x10 = tempTarget_x10;
	tt.cool_ramp_rate_x10 = coolRampRate_x10_perMinute;
	tt.heat_ramp_rate_x10 = heatRampRate_x10_perMinute;
	tt.cooler_enable = 1;
	tt.heater_enable = 1;

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetTargetTemperature(bCellNo, tt));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	/* Read back and validate */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	if ((tt.temp_target_x10 != tempTarget_x10)
			|| (tt.cool_ramp_rate_x10 != coolRampRate_x10_perMinute)
			|| (tt.heat_ramp_rate_x10 != heatRampRate_x10_perMinute)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s failed validation \n", __FUNCTION__);
		rc = terError;
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte setTemperatureSensorCarlibrationData(Byte bCellNo,
		Word wSoftwareTempInHundredth, Word wRealTempInHundredth) {

	/* FUNCTION IS NOT SUPPORTED */
	RET(success);
}

/**
 * <pre>
 * Description:
 *   Sets the temperature limit value for setting temperature and sensor reading temperature.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wSetTempLowerInHundredth - INPUT - set temperature lower limit in hundredth of a degree Celsius
 *   wSetTempUpperInHundredth - INPUT - set temperature upper limit in hundredth of a degree Celsius
 *   wSensorTempLowerInHundredth - INPUT - sensor reading temperature lower limit
 * 										in hundredth of a degree Celsius
 *   wSensorTempUpperInHundredth - INPUT - sensor reading  temperature upper limit
 * 										in hundredth of a degree Celsius
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   default value is 25C to 60C for set temperature, 15C to 70C for sensor reading temperature
 * </pre>
 ***************************************************************************************/
Byte setTemperatureLimit(Byte bCellNo, Word wSetTempLowerInHundredth,
		Word wSetTempUpperInHundredth, Word wSensorTempLowerInHundredth,
		Word wSensorTempUpperInHundredth) {

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d,%d,%d,%d) \n",
			__FUNCTION__, bCellNo, wSetTempLowerInHundredth,
			wSetTempUpperInHundredth, wSensorTempLowerInHundredth,
			wSensorTempUpperInHundredth);

	Byte rc = success;
	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success){
		Common::Instance.WriteToLog(LogTypeTrace,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	siSetTempLowerInHundredth = wSetTempLowerInHundredth;
	siSetTempUpperInHundredth = wSetTempUpperInHundredth;
	siSensorTempLowerInHundredth = wSensorTempLowerInHundredth;
	siSensorTempUpperInHundredth = wSensorTempUpperInHundredth;

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Gets the temperature limit value for setting temperature and sensor reading temperature.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wSetTempLowerInHundredth - OUTPUT - set temperature lower limit in hundredth of a degree Celsius
 *   wSetTempUpperInHundredth - OUTPUT - set temperature upper limit in hundredth of a degree Celsius
 *   wSensorTempLowerInHundredth - OUTPUT - sensor reading temperature lower limit
 * 										 in hundredth of a degree Celsius
 *   wSensorTempUpperInHundredth - OUTPUT - sensor reading  temperature upper limit
 * 										 in hundredth of a degree Celsius
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte getTemperatureLimit(Byte bCellNo, Word *wSetTempLowerInHundredth,
		Word *wSetTempUpperInHundredth, Word *wSensorTempLowerInHundredth,
		Word *wSensorTempUpperInHundredth) {

	Byte rc = success;
	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n",
			__FUNCTION__, bCellNo);

	if ((wSetTempLowerInHundredth == NULL) || (wSetTempUpperInHundredth == NULL)
			|| (wSensorTempLowerInHundredth == NULL)
			|| (wSensorTempUpperInHundredth == NULL)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wSetTempLowerInHundredth = *wSetTempUpperInHundredth =
			*wSensorTempLowerInHundredth = *wSensorTempUpperInHundredth = 0;
	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success){
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	*wSetTempLowerInHundredth = siSetTempLowerInHundredth;
	*wSetTempUpperInHundredth = siSetTempUpperInHundredth;
	*wSensorTempLowerInHundredth = siSensorTempLowerInHundredth;
	*wSensorTempUpperInHundredth = siSensorTempUpperInHundredth;

FUNCTIONEND:
	if ((wSetTempLowerInHundredth == NULL)
			|| (wSetTempUpperInHundredth == NULL)
			|| (wSensorTempLowerInHundredth == NULL)
			|| (wSensorTempUpperInHundredth == NULL)) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, %d, %d, %d, %d \n", __FUNCTION__,
				rc, *wSetTempLowerInHundredth, *wSetTempUpperInHundredth,
				*wSensorTempLowerInHundredth, *wSensorTempUpperInHundredth);
	}
	RET(rc);
}

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
Byte setSafeHandlingTemperature(Byte bCellNo, Word wTempInHundredth) {
	Byte rc = success; /* No error */
	TER_Status status;
	targetTemperature tt;
	int tempTarget_x10, coolRampRate_x10_perMinute, heatRampRate_x10_perMinute;
	TER_SlotSettings slotSettings;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d) \n",
			__FUNCTION__, bCellNo, wTempInHundredth);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Read current ramp rates from SIO */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	heatRampRate_x10_perMinute = (Word) (slotSettings.temp_ramp_rate_heat_x10);
	coolRampRate_x10_perMinute = (Word) (slotSettings.temp_ramp_rate_cool_x10);
	tempTarget_x10 = (wTempInHundredth + 5) / 10;

	tt.temp_target_x10 = tempTarget_x10;
	tt.cool_ramp_rate_x10 = coolRampRate_x10_perMinute;
	tt.heat_ramp_rate_x10 = heatRampRate_x10_perMinute;
	tt.cooler_enable = 1;
	tt.heater_enable = 1;

	/* Go to safe handling temperature */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetSafeHandlingTemperature(bCellNo, tt));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte getSafeHandlingTemperature(Byte bCellNo, Word * wTempInHundredth) {

	/* FUNCTION IS NOT SUPPORTED */
	RET(success);
}

/**
 * <pre>
 * Description:
 *   Sets the target voltatge values supplying to the drive
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wV5InMilliVolts - INPUT - range is 4500 -to- 5500 in milli-volts (4.5V - 5.5V)
 *   wV12InMilliVolts - INPUT - 0 (Not Supported)
 * Return:
 *   zero if success. otherwise, non-zero value.
 *  Note:
 *   power supply is turned off if 0 is set, otherwise turn on.
 *   support range depends on real hardware system. supplier should provide
 *   the range to be able to set, and return error if testcase set over-range value.
 * </pre>
 ***************************************************************************************/
Byte setTargetVoltage(Byte bCellNo, Word wV5InMilliVolts,
		Word wV12InMilliVolts) {
	TER_Status status;
	int mask;
	int OnOff;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d,%d) \n",
			__FUNCTION__, bCellNo, wV5InMilliVolts, wV12InMilliVolts);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Need to enable setting the power to the 5V and 12V supplies */
	/* Mask: FID Power = 0x1, 12V Power = 0x2, 5V Power = 0x4, 3V Power = 0x8 */

	mask = 0x4; /* 5V mask */
	if (wV5InMilliVolts) {
		OnOff = mask;

		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_SetPowerVoltage(bCellNo, mask, (int)wV5InMilliVolts));
		if (status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status,
					TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}

		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_SetPowerEnabled(bCellNo, mask, OnOff));
		if (status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status,
					TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}
	} else {
		OnOff = 0;

		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_SetPowerEnabled(bCellNo, mask, OnOff));
		if (status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status,
					TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}
	}

	/* 12V */
	mask = 0x2; /* 12V mask */
	if (wV12InMilliVolts) {
		OnOff = mask;

		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_SetPowerVoltage(bCellNo, mask, (int)wV12InMilliVolts));
		if (status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status,
					TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}

		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_SetPowerEnabled(bCellNo, mask, OnOff));
		if (status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status,
					TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}
	} else {
		OnOff = 0;

		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_SetPowerEnabled(bCellNo, mask, OnOff));
		if (status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status,
					TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}
	}

	/* Need to cache the target voltage, since we don't have a Get function */
	siTargetVoltage5VinMilliVolts = wV5InMilliVolts;

	/* Need to cache the target voltage, since we don't have a Get function */
	siTargetVoltage12VinMilliVolts = wV12InMilliVolts;

	/* Enable UART pull up voltage */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetSerialEnable(bCellNo, 1));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte getTargetVoltage(Byte bCellNo, Word *wV5InMilliVolts,
		Word *wV12InMilliVolts) {
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*,*) \n",
			__FUNCTION__, bCellNo);

	if ((wV5InMilliVolts == NULL) || (wV12InMilliVolts == NULL)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	*wV5InMilliVolts = (Word) siTargetVoltage5VinMilliVolts;
	*wV12InMilliVolts = (Word) siTargetVoltage12VinMilliVolts;

FUNCTIONEND:
	if ((wV5InMilliVolts == NULL) || (wV12InMilliVolts == NULL)) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(
				LogTypeTrace,
				"Exit : %s, exit code = 0x%x, 5V Target = %d mVolts, 12V Target = %d mVolts \n",
				__FUNCTION__, rc, *wV5InMilliVolts, *wV12InMilliVolts);
	}
	RET(rc);
}

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
Byte getCurrentVoltage(Byte bCellNo, Word *wV5InMilliVolts,
		Word *wV12InMilliVolts) {
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*,*) \n",
			__FUNCTION__, bCellNo);

	if ((wV5InMilliVolts == NULL) || (wV12InMilliVolts == NULL)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wV5InMilliVolts = 0;
	*wV12InMilliVolts = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotStatus(bCellNo, &slotStatus));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		*wV5InMilliVolts = (Word) (slotStatus.voltage_actual_5v_mv);
		*wV12InMilliVolts = (Word) (slotStatus.voltage_actual_12v_mv);
	}

FUNCTIONEND:
	if ((wV5InMilliVolts == NULL) || (wV12InMilliVolts == NULL)) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(
				LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Current_5V = %d mVolts, Current_12V = %d mVolts\n",
				__FUNCTION__, rc, *wV5InMilliVolts, *wV12InMilliVolts);
	}
	RET(rc);
}

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
Byte getActualCurrent(Byte bCellNo, Word *wV5InMilliAmps,
		Word *wV12InMilliAmps) {
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*,*) \n",
			__FUNCTION__, bCellNo);

	if ((wV5InMilliAmps == NULL) || (wV12InMilliAmps == NULL)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wV5InMilliAmps = 0;
	*wV12InMilliAmps = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotStatus(bCellNo, &slotStatus));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		*wV5InMilliAmps = (Word) (slotStatus.current_actual_5v_ma);
		*wV12InMilliAmps = (Word) (slotStatus.current_actual_12v_ma);
	}

FUNCTIONEND:
	if ((wV5InMilliAmps == NULL) || (wV12InMilliAmps == NULL)) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, %d mAmps, %d mAmps\n",
				__FUNCTION__, rc, *wV5InMilliAmps, *wV12InMilliAmps);
	}
	RET(rc);
}

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
 *  	 is set to D/A converter
 *
 *   (2) The measured voltage is registered in the flash memory
 *
 *   (3) D/A Converter set value is calculated as following formula
 *
 *  	 Y = (X - VL) x (DH - DL) / (VH - VL) + DL
 *
 *  	 Y : D/A Converter set value
 *  	 X : Output voltage
 *  	 DL : D/A Converter set value (Low)
 *  	 DH : D/A Converter set value (High)
 *  	 VL : Output voltage (When D/A Converter set value is DL)
 *  	 VH : Output voltage (When D/A Converter set value is DH)
 * </pre>
 ***************************************************************************************/
Byte setVoltageCalibration(Byte bCellNo, Word wV5LowInMilliVolts,
		Word wV5HighInMilliVolts, Word wV12LowInMilliVolts,
		Word wV12HighInMilliVolts) {

	/* FUNCTION IS NOT SUPPORTED */
	RET(success);
}
/**
 * <pre>
 * Description:
 *   Sets the voltage limit values supplying to the drive
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wV5LowerLimitInMilliVolts - INPUT - HDD+5V lower limit in mill-volts
 * 									  range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
 *   wV12LowerLimitInMilliVolts - INPUT - HDD+12V lower limit in mill-volts
 * 									   range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
 *   wV5UpperLimitInMilliVolts - INPUT - HDD+5V lower upper in mill-volts
 * 									  range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
 *   wV12UpperLimitInMilliVolts - INPUT - HDD+12V upper limit in mill-volts
 * 									   range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
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
Byte setVoltageLimit(Byte bCellNo, Word wV5LowerLimitInMilliVolts,
		Word wV12LowerLimitInMilliVolts, Word wV5UpperLimitInMilliVolts,
		Word wV12UpperLimitInMilliVolts) {
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d,%d,%d,%d) \n",
			__FUNCTION__, bCellNo, wV5LowerLimitInMilliVolts,
			wV12LowerLimitInMilliVolts, wV5UpperLimitInMilliVolts,
			wV12UpperLimitInMilliVolts);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	if ((wV5LowerLimitInMilliVolts < V5_LOWER_LIMIT_MV)
			|| (wV5UpperLimitInMilliVolts > V5_UPPER_LIMIT_MV)
			|| (wV12LowerLimitInMilliVolts < V12_LOWER_LIMIT_MV)
			|| (wV12UpperLimitInMilliVolts > V12_UPPER_LIMIT_MV)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Limits are out of range\n", __FUNCTION__);
		rc = invalidArgumentError;
		goto FUNCTIONEND;
	}

	siV5LowerLimitInMilliVolts = wV5LowerLimitInMilliVolts;
	siV5UpperLimitInMilliVolts = wV5UpperLimitInMilliVolts;
	siV12LowerLimitInMilliVolts = wV12LowerLimitInMilliVolts;
	siV12UpperLimitInMilliVolts = wV12UpperLimitInMilliVolts;

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Gets the voltage limit values supplying to the drive
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wV5LowerLimitInMilliVolts - OUTPUT - HDD+5V lower limit in mill-volts
 * 									  range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
 *   wV12LowerLimitInMilliVolts - OUTPUT - HDD+12V lower limit in mill-volts
 * 									   range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
 *   wV5UpperLimitInMilliVolts - OUTPUT - HDD+5V lower upper in mill-volts
 * 									  range is 0 -to- 6000 in milli-volts (0.0V - 6.0V)
 *   wV12UpperLimitInMilliVolts - OUTPUT - HDD+12V upper limit in mill-volts
 * 									   range is 0 -to- 14400 in milli-volts (0.0V - 14.4V)
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte getVoltageLimit(Byte bCellNo, Word *wV5LowerLimitInMilliVolts,
		Word *wV12LowerLimitInMilliVolts, Word *wV5UpperLimitInMilliVolts,
		Word *wV12UpperLimitInMilliVolts) {

	Byte rc = success;
	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n",
			__FUNCTION__, bCellNo);

	if ((wV5LowerLimitInMilliVolts == NULL)
			|| (wV5UpperLimitInMilliVolts == NULL)
			|| (wV12LowerLimitInMilliVolts == NULL)
			|| (wV12UpperLimitInMilliVolts == NULL)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wV5LowerLimitInMilliVolts = *wV5UpperLimitInMilliVolts =
			*wV12LowerLimitInMilliVolts = *wV12UpperLimitInMilliVolts = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	*wV5LowerLimitInMilliVolts = siV5LowerLimitInMilliVolts;
	*wV5UpperLimitInMilliVolts = siV5UpperLimitInMilliVolts;
	*wV12LowerLimitInMilliVolts = siV12LowerLimitInMilliVolts;
	*wV12UpperLimitInMilliVolts = siV12UpperLimitInMilliVolts;

FUNCTIONEND:
	if ((wV5LowerLimitInMilliVolts == NULL)
		|| (wV5UpperLimitInMilliVolts == NULL)
		|| (wV12LowerLimitInMilliVolts == NULL)
		|| (wV12UpperLimitInMilliVolts == NULL)) {
		Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(
				LogTypeTrace,
				"Exit : %s, exit code = 0x%x, 5VLowerLimit = %d mVolts, 12VLowerLimit = %d mVolts, 5VUpperLimit = %d mVolts, 12VUpperLimit = %d mVolts\n",
				__FUNCTION__, rc, *wV5LowerLimitInMilliVolts,
				*wV12LowerLimitInMilliVolts, *wV5UpperLimitInMilliVolts,
				*wV12UpperLimitInMilliVolts);
	}
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Sets the current limit values supplying to the drive
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wV5LimitInMilliAmpere - INPUT - HDD+5V current limit in mill-Ampere
 *  								  range is 0 -to- 5000 in milli-Ampere (0-5A)
 *   wV12LimitInMilliAmpere - INPUT - HDD+12V current limit in mill-Ampere
 *  								   range is 0 -to- 5000 in milli-Ampere (0-5A)
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
Byte setCurrentLimit(Byte bCellNo, Word wV5LimitInMilliAmpere,
		Word wV12LimitInMilliAmpere) {
	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d, %d, %d) \n",
			__FUNCTION__, bCellNo, wV5LimitInMilliAmpere,
			wV12LimitInMilliAmpere);

	Byte rc = success;
	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	if ((wV5LimitInMilliAmpere != V5_LIMIT_IN_MILLI_AMPS)
			|| (wV12LimitInMilliAmpere != V12_LIMIT_IN_MILLI_AMPS)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Invalid limit have been passed\n", __FUNCTION__);
		rc = invalidArgumentError;
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Gets the current limit values supplying to the drive
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wV5LimitInMilliAmpere - OUTPUT - HDD+5V current limit in mill-Ampere
 *  								   3000 mA (Fixed Value)
 *   wV12LimitInMilliAmpere - OUTPUT - HDD+12V current limit in mill-Ampere
 *  								   0 (No 12V Support)
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte getCurrentLimit(Byte bCellNo, Word *wV5LimitInMilliAmpere,
		Word *wV12LimitInMilliAmpere) {

	Byte rc = success;
	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n",
			__FUNCTION__, bCellNo);

	if ((wV5LimitInMilliAmpere == NULL) || (wV12LimitInMilliAmpere == NULL)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wV5LimitInMilliAmpere = *wV12LimitInMilliAmpere = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	*wV5LimitInMilliAmpere = V5_LIMIT_IN_MILLI_AMPS;
	*wV12LimitInMilliAmpere = V12_LIMIT_IN_MILLI_AMPS;

FUNCTIONEND:
	if ((wV5LimitInMilliAmpere == NULL) || (wV12LimitInMilliAmpere == NULL)) {
		Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(
				LogTypeTrace,
				"Exit : %s, exit code = 0x%x, 5VLimit = %d mAmp, 12VLimit = %d mAmp\n",
				__FUNCTION__, rc, *wV5LimitInMilliAmpere,
				*wV12LimitInMilliAmpere);
	}
	RET(rc);
}

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
 *   5v: Valid values for rise time are 2-100ms; for fall time, 10-100ms.  Values below the minimum will 
 *   be internally adjusted to the minimum, and values above the maximum will be internally adjusted 
 *   to the maximum 
 *   12V: Valid values for rise time are 2-100ms; for fall time 10-100ms
 * </pre>
 ***************************************************************************************/
Byte setVoltageRiseTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec) {
	Byte rc = success; /* No error */
	TER_Status status;
	int values[2] = {-1, -1};

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d,%d) \n",
			__FUNCTION__, bCellNo, wV5TimeInMsec, wV12TimeInMsec);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	wV5TimeInMsec = wV5TimeInMsec < MIN_VOLTAGE_RISE_TIME_MS ? MIN_VOLTAGE_RISE_TIME_MS : wV5TimeInMsec;
	wV5TimeInMsec = wV5TimeInMsec > MAX_VOLTAGE_RISE_TIME_MS ? MAX_VOLTAGE_RISE_TIME_MS : wV5TimeInMsec;

	/* Get 5V rise/fall time */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terProperty5VRiseFallTimes, 2, values));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	/* Set 5V rise/fall time */
	values[0] = (int)wV5TimeInMsec;
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terProperty5VRiseFallTimes, 2, values));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	if (wV12TimeInMsec) { /* No need to set rise time if zero */

		wV12TimeInMsec = wV12TimeInMsec < MIN_VOLTAGE_RISE_TIME_MS ? MIN_VOLTAGE_RISE_TIME_MS : wV12TimeInMsec;
		wV12TimeInMsec = wV12TimeInMsec > MAX_VOLTAGE_RISE_TIME_MS ? MAX_VOLTAGE_RISE_TIME_MS : wV12TimeInMsec;

		/* Get 12V rise/fall time */
		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terProperty12VRiseFallTimes, 2, values));
		if (TER_Status_none != status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}

		/* Set 12V rise/fall time */
		values[0] = (int)wV12TimeInMsec;
		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terProperty12VRiseFallTimes, 2, values));
		if (TER_Status_none != status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte getVoltageRiseTime(Byte bCellNo, Word *wV5TimeInMsec,
		Word *wV12TimeInMsec) {
	Byte rc = success; /* No error */
	TER_Status status;
	int values[2] = {-1, -1};

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n",
			__FUNCTION__, bCellNo);

	if ((wV5TimeInMsec == NULL) || (wV12TimeInMsec == NULL)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wV5TimeInMsec = *wV12TimeInMsec = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s -Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* 5V rise/fall time */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terProperty5VRiseFallTimes, 2, values));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}
	*wV5TimeInMsec = (Word)values[0];

	/* 12V rise/fall time */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terProperty12VRiseFallTimes, 2, values));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}
	*wV12TimeInMsec = (Word)values[0];

FUNCTIONEND:
	if ((wV5TimeInMsec == NULL) || (wV12TimeInMsec == NULL)) {
		Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
	Common::Instance.WriteToLog(
			LogTypeTrace,
			"Exit : %s, exit code = 0x%x, 5VRiseTime = %d mSec, 12VRiseTime = %d mSec\n",
			__FUNCTION__, rc, *wV5TimeInMsec, *wV12TimeInMsec);
	}
	RET(rc);
}

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
Byte setVoltageFallTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec) {
	Byte rc = success; /* No error */
	TER_Status status;
	int values[2] = {-1, -1};

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d,%d) \n",
			__FUNCTION__, bCellNo, wV5TimeInMsec, wV12TimeInMsec);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	wV5TimeInMsec = wV5TimeInMsec < MIN_VOLTAGE_FALL_TIME_MS ? MIN_VOLTAGE_FALL_TIME_MS : wV5TimeInMsec;
	wV5TimeInMsec = wV5TimeInMsec > MAX_VOLTAGE_FALL_TIME_MS ? MAX_VOLTAGE_FALL_TIME_MS : wV5TimeInMsec;

	/* Get 5V rise/fall time */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terProperty5VRiseFallTimes, 2, values));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	/* Set 5V rise/fall time */
	values[1] = (int)wV5TimeInMsec;
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terProperty5VRiseFallTimes, 2, values));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	if (wV12TimeInMsec) { /* No need to set rise time if zero */
		wV12TimeInMsec = wV12TimeInMsec < MIN_VOLTAGE_FALL_TIME_MS ? MIN_VOLTAGE_FALL_TIME_MS : wV12TimeInMsec;
		wV12TimeInMsec = wV12TimeInMsec > MAX_VOLTAGE_FALL_TIME_MS ? MAX_VOLTAGE_FALL_TIME_MS : wV12TimeInMsec;

		/* Get 12V rise/fall time */
		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terProperty12VRiseFallTimes, 2, values));
		if (TER_Status_none != status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}

		/* Set 12V rise/fall time */
		values[1] = (int)wV12TimeInMsec;
		WAPI_CALL_RPC_WITh_RETRY(
				status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terProperty12VRiseFallTimes, 2, values));
		if (TER_Status_none != status) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status);
			rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte getVoltageFallTime(Byte bCellNo, Word *wV5TimeInMsec,
		Word *wV12TimeInMsec) {
	Byte rc = success; /* No error */
	TER_Status status;
	int values[2] = {-1, -1};

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*,*) \n",
			__FUNCTION__, bCellNo);

	if ((wV5TimeInMsec == NULL) ||
		(wV12TimeInMsec == NULL)) {
		Common::Instance.WriteToLog(LogTypeError, "ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wV5TimeInMsec = *wV12TimeInMsec = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* 5V rise/fall time */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terProperty5VRiseFallTimes, 2, values));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}
	*wV5TimeInMsec = (Word)values[1];

	/* 12V rise/fall time */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terProperty12VRiseFallTimes, 2, values));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}
	*wV12TimeInMsec = (Word)values[1];

FUNCTIONEND:
	if ((wV5TimeInMsec == NULL) || (wV12TimeInMsec == NULL)) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(
				LogTypeTrace,
				"Exit : %s, exit code = 0x%x, 5VFallTime = %d mSec, 12VFallTime = %d mSec\n",
				__FUNCTION__, rc, *wV5TimeInMsec, *wV12TimeInMsec);
	}
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Sets the voltatge rise interval between 5V and 12V
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wTimeFromV5ToV12InMsec - INPUT - interval time from 5V on to 12V on.
 *  								  range is 0 -to- 127 (0-127msec)
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   positive value is 5V first then 12V.
 *   negative value is reverse order
 *   example
 *  	100 : 5V-On -> 100msec wait -> 12V-On
 *  	-20 : 12V-On -> 20msec wait -> 5V-On
 *
 *   support range depends on real hardware system. supplier should provide
 *   the range to be able to set, and return error if testcase set over-range value.
 * </pre>
 ***************************************************************************************/
Byte setVoltageInterval(Byte bCellNo, int wTimeFromV5ToV12InMsec) {
	Byte rc = success; /* No error */
	TER_Status status;
	int values[1] = {-1};

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d) \n",
			__FUNCTION__, bCellNo, wTimeFromV5ToV12InMsec);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	wTimeFromV5ToV12InMsec = wTimeFromV5ToV12InMsec < MIN_5V12V_SEQ_TIME ? MIN_5V12V_SEQ_TIME : wTimeFromV5ToV12InMsec;
	wTimeFromV5ToV12InMsec = wTimeFromV5ToV12InMsec > MAX_5V12V_SEQ_TIME ? MAX_5V12V_SEQ_TIME : wTimeFromV5ToV12InMsec;

	/* Set 5V & 12V sequencing time */
	values[0] = wTimeFromV5ToV12InMsec;
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terProperty5V12VSeqTime, 1, values));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Gets the voltatge rise interval between 5V and 12V
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wTimeFromV5ToV12InMsec - OUTPUT - interval time from 5V on to 12V on.
 *  								   range is 0 -to- 255 (0-255msec)
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte getVoltageInterval(Byte bCellNo, Word *wTimeFromV5ToV12InMsec) {

	/* FUNCTION IS NOT SUPPORTED */
	RET(success);
}

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
Byte getVoltageErrorStatus(Byte bCellNo, Word *wErrorStatus) {
	TER_Status status;
	TER_SlotStatus slotStatus;
	int slotErrors;
	int voltageError;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*) \n",
			__FUNCTION__, bCellNo);

	if (wErrorStatus == NULL) {
		Common::Instance.WriteToLog(LogTypeError, "ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wErrorStatus = 0;
	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotStatus(bCellNo, &slotStatus));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		Common::Instance.maskRequestedErrors (&slotStatus.slot_errors);
		slotErrors = slotStatus.slot_errors;
		voltageError = 0;
		voltageError |= (slotErrors & TER_Notifier_SlotCircuitFault);
		voltageError |= (slotErrors & TER_Notifier_DriveOverCurrent);

		*wErrorStatus = (Word) voltageError;
		if (voltageError != 0)
			rc = Common::Instance.GetWrapperErrorCode(TER_Status_none,
					(TER_Notifier) slotStatus.slot_errors);
	}

	/* if there are any slot errors, return an error */
	if (slotStatus.slot_errors) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR : %s, rc=%d, 0x%x, 0x%x\n", __FUNCTION__, rc,
				*wErrorStatus, slotStatus.slot_errors);
		rc = Common::Instance.GetWrapperErrorCode(TER_Status_none,
				(TER_Notifier) slotStatus.slot_errors);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	if (wErrorStatus == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, 0x%x\n", __FUNCTION__, rc,
				slotStatus.slot_errors);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, 0x%x, 0x%x\n", __FUNCTION__, rc,
				*wErrorStatus, slotStatus.slot_errors);
	}
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Gets the drive plug status
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 * Return:
 *   0 - Drive is not plugged
 *   1 - Drive is plugged
 *   sioNotConnected (0x20) - SIO is not connected
 *   invalidSlotIndex (0x12) - Slot index is invalid
 * </pre>
 ***************************************************************************************/
Byte isDrivePlugged(Byte bCellNo) {
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte drivePlugged = 0; /* Drive not plugged */
	Byte rc = success;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed. exit code = 0x%02x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings( bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status,
				TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		if (slotSettings.slot_status_bits & IsDrivePresent) {
			drivePlugged = 1;
		}
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x, Drive Plugged = %s \n", __FUNCTION__,
			rc, (drivePlugged == 1) ? "Yes" : "No");
	// DO NOT use the macro RET because this method needs to return the drive plugged state
	return drivePlugged;
}

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
Byte getCellInventory(Byte bCellNo,
		CELL_CARD_INVENTORY_BLOCK **stCellInventory) {
	Byte rc = success; /* No error */
	TER_Status status;
	TER_SlotInfo pSlotInfo;
	CELL_CARD_INVENTORY_BLOCK cellCardInventoryBlock;
	CELL_CARD_INVENTORY_BLOCK* pcellCardInventoryBlock = &cellCardInventoryBlock;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,**) \n",
			__FUNCTION__, bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	if (stCellInventory == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
//		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotInfo(bCellNo, &pSlotInfo));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		memset((void*) pcellCardInventoryBlock, 0, sizeof(cellCardInventoryBlock));

		/* The Hitachi Test code (that we have) cares only about the following fields */
		memcpy((void*) cellCardInventoryBlock.bPartsNumber,
				(const void*) pSlotInfo.SioPcbSerialNumber,
				sizeof(cellCardInventoryBlock.bPartsNumber));
		memcpy((void*) cellCardInventoryBlock.bCardRev,
				(const void*) pSlotInfo.DioFirmwareVersion,
				sizeof(cellCardInventoryBlock.bCardRev));
		memcpy((void*) cellCardInventoryBlock.bCardType, (const void*) "DIO",
				sizeof(cellCardInventoryBlock.bCardType));
		memcpy((void*) cellCardInventoryBlock.bHddType, (const void*) "1",
				sizeof(cellCardInventoryBlock.bHddType));
		memcpy((void*) cellCardInventoryBlock.bSerialNumber,
				(const void*) pSlotInfo.DioPcbSerialNumber,
				sizeof(cellCardInventoryBlock.bSerialNumber));
		memcpy((void*) cellCardInventoryBlock.bConnectorLifeCount,
				(const void*) "10000",
				sizeof(cellCardInventoryBlock.bConnectorLifeCount));
	}

	//memcpy((void*)(*stCellInventory), (const void*) &cellCardInventoryBlock, sizeof(cellCardInventoryBlock));
	*stCellInventory = pcellCardInventoryBlock;

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte setTemperatureEnvelope(Byte bCellNo, Word wEnvelopeTempInTenth) {

	Byte rc = success; /* No error */
	TER_Status status;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d) \n",
			__FUNCTION__, bCellNo, wEnvelopeTempInTenth);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Issue the call */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetTemperatureEnvelope ( bCellNo, wEnvelopeTempInTenth ));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte getTemperatureEnvelope(Byte bCellNo, Word *wEnvelopeTempInTenth) {
	TER_Status status;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*) \n",
			__FUNCTION__, bCellNo);

	if (wEnvelopeTempInTenth == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wEnvelopeTempInTenth = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Issue the call */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetTemperatureEnvelope (bCellNo,wEnvelopeTempInTenth));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	if (wEnvelopeTempInTenth == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Temperature Envelope = %d \n",
				__FUNCTION__, rc, *wEnvelopeTempInTenth);
	}
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Sets the target voltatge values supplying to UART I/O and UART pull-up
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wUartPullupVoltageInMilliVolts - INPUT - UART pull-up voltage value
 *  	 range is 2700 or 3300 milli-volts for Development Station
 *                1200-to- 3300 milli-volts for real hardware
 *                0 will chieve 'disable staggered spin up'
 * Return:
 *   zero if success. otherwise, non-zero value.
 *  Note:
 *   IMPORTANT!!!!! - even the name is wUartPullupVoltageInMilliVolts, this applies not
 *   only UART pull-up but also UART I/O voltage (Vcc for UART Buffer). UART pull-up
 *   shall be supplied during HDD+5V turned on. That means UART pull-up and HDD+5V
 *   shall be synchronized.
 *
 *   setting zero value at drive power supply on to achieve 'disable staggered spin up' 
 *   in SATA standard specification. 
 * </pre>
 ***************************************************************************************/
Byte setUartPullupVoltageDefault(Byte bCellNo) {
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code = 0x%x",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	rc = setUartPullupVoltage(bCellNo, UART_PULLUP_VOLTAGE_DEFAULT);

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, rc=%d \n",
			__FUNCTION__, rc);
	RET(rc);
}

Byte setUartPullupVoltage(Byte bCellNo, Word wUartPullupVoltageInMilliVolts) {
	TER_Status status;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d) \n",
			__FUNCTION__, bCellNo, wUartPullupVoltageInMilliVolts);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code = 0x%x",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	if (wUartPullupVoltageInMilliVolts != 0) { /* 0 = Enable staggred spin-up */
		if (wUartPullupVoltageInMilliVolts < 1200 || /* Valid range 1.2V-3.3V */
		wUartPullupVoltageInMilliVolts > 3300) {
			Common::Instance.WriteToLog(LogTypeError,
					"ERROR: %s - Pullup volate is out of range\n",
					__FUNCTION__);
			rc = outOfRange;
			goto FUNCTIONEND;
		}
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetSerialLevels(bCellNo, (int)wUartPullupVoltageInMilliVolts));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	/* Need to cache the UART pullup Voltage for the Get function */
	siUartPullupVoltageInMilliVolts = (int) wUartPullupVoltageInMilliVolts;

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte getUartPullupVoltage(Byte bCellNo, Word *wUartPullupVoltageInMilliVolts) {
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*) \n",
			__FUNCTION__, bCellNo);

	if (wUartPullupVoltageInMilliVolts == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wUartPullupVoltageInMilliVolts = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code = 0x%x",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Return cached value */
	*wUartPullupVoltageInMilliVolts = siUartPullupVoltageInMilliVolts;

FUNCTIONEND:
	if (wUartPullupVoltageInMilliVolts == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(
				LogTypeTrace,
				"Exit : %s, exit code = 0x%x, UART Pullup Voltage = %d mVolts\n",
				__FUNCTION__, rc, *wUartPullupVoltageInMilliVolts);
	}
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Sets UART baudrate setting
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   dwBaudrate - INPUT - baudrate in bps (115200,460800,921600,1843200,2778000
 *                       3333000,4167000,5556000,8333000)
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte siSetUartBaudrate(Byte bCellNo, Dword dwBaudrate) {
	TER_Status status;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%ld) \n",
			__FUNCTION__, bCellNo, dwBaudrate);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	if (!dwBaudrate)
		dwBaudrate = 1843200;
  	switch(dwBaudrate) {
		case (115200):
			dwBaudrate = 115200;
			break;
		case (1843200):
			dwBaudrate = 1843200;
			break;
		case (2083333):
			dwBaudrate = 2083000;
			break;
		case (2380000):
			dwBaudrate = 2380000;
			break;
		case (2777777):
			dwBaudrate = 2778000;
			break;
		case (3333333):
			dwBaudrate = 3333000;
			break;
		case (4166666):
			dwBaudrate = 4167000;
			break;
		case (5555555):
			dwBaudrate = 5556000;
			break;
		case (8333333):
			dwBaudrate = 8333000;
			break;
		case (11111111):
			dwBaudrate = 11111000;
			break;
		case (16666666):
			dwBaudrate = 16666000;
			break;
		default:
			Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. baudrate %l is invalid\n",
				__FUNCTION__, dwBaudrate);
			rc = invalidArgumentError;
			goto FUNCTIONEND;
            break;
    }

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetSerialParameters(bCellNo, dwBaudrate, 1));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte siGetUartBaudrate(Byte bCellNo, Dword *dwBaudrate) {
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*) \n",
			__FUNCTION__, bCellNo);

	if (dwBaudrate == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*dwBaudrate = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings(bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		/* Baud rate in bps */
		*dwBaudrate = (Dword) slotSettings.baud_rate;
	}

FUNCTIONEND:
	if (dwBaudrate == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, rc=%d, %d \n",
				__FUNCTION__, rc, *dwBaudrate);
	}
	RET(rc);
}

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
Byte siBulkReadFromDrive(Byte bCellNo, Word *wSize, Byte *bData,
		Word wTimeoutInMillSec) {

	Byte sendRecvBuf[WAPI_SEND_RECV_BUF_SIZE];
	TER_Status status;
	Byte rc = success; /* No error */
	Word i;
	unsigned int respLength;
	unsigned int status1;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*,*,%d) \n",
			__FUNCTION__, bCellNo, wTimeoutInMillSec);

	if ((wSize == NULL) || (bData == NULL)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	wTimeoutInMillSec = (wTimeoutInMillSec > 10000) ? 10000 : wTimeoutInMillSec;
	respLength = *wSize; /* no 2 by 1 protocol */

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioReceiveBuffer(bCellNo, &respLength, wTimeoutInMillSec, sendRecvBuf, &status1));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		for (i = 0; i < respLength; ++i) {
			bData[i] = sendRecvBuf[i];
		}
		*wSize = respLength;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte siBulkWriteToDrive(Byte bCellNo, Word wSize, Byte *bData,
		Word wTimeoutInMillSec) {

	Byte sendRecvBuf[WAPI_SEND_RECV_BUF_SIZE];
	TER_Status status;
	Byte rc = success; /* No error */
	Word cmdLength, i;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d,*,%d) \n",
			__FUNCTION__, bCellNo, wSize, wTimeoutInMillSec);

	if (bData == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	Common::Instance.TerEnableFpgaUart2Mode(bCellNo, 0);

	for (i = 0; i < wSize; ++i) { /* Data */
		sendRecvBuf[i] = bData[i];
	}
	cmdLength = wSize;

	Common::Instance.LogSendReceiveBuffer("Req Pkt: ", sendRecvBuf, wSize);

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioSendBuffer(bCellNo, cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	Common::Instance.TerEnableFpgaUart2Mode(bCellNo, 1);

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);

}

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
Byte siWriteDriveMemory(Byte bCellNo, Dword dwAddress, Word wSize, Byte *bData,
		Word wTimeoutInMillSec) {

	Byte sendRecvBuf[WAPI_SEND_RECV_BUF_SIZE];
	TER_Status status;
	Byte rc = success; /* No error */
	Word cmdLength, i, j, checkSum, cmdLen, tmpWSize;
	unsigned int respLength, expLength;
	unsigned int status1;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,0x%lx,%d,*,%d) \n",
			__FUNCTION__, bCellNo, dwAddress, wSize, wTimeoutInMillSec);

	if (bData == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Flush sio receive buffer */
	Common::Instance.TerFlushReceiveBuffer(bCellNo);

	tmpWSize = wSize;
	if (wSize & 1) {
		wSize++;
	}

	i = 0;
	cmdLen = (wSize / 2) + 5;
	sendRecvBuf[i++] = 0xaa; /* Sync Pkt */
	sendRecvBuf[i++] = 0x55;

	sendRecvBuf[i++] = (cmdLen) & 0xFF; /* Length */
	sendRecvBuf[i++] = (cmdLen) & 0xFF;

	sendRecvBuf[i++] = 0x00; /* Opcode */
	sendRecvBuf[i++] = 0x02;

	sendRecvBuf[i++] = (dwAddress >> 24) & 0xFF; /* Address */
	sendRecvBuf[i++] = (dwAddress >> 16) & 0xFF;
	sendRecvBuf[i++] = (dwAddress >> 8) & 0xFF;
	sendRecvBuf[i++] = (dwAddress) & 0xFF;

	sendRecvBuf[i++] = (tmpWSize >> 8) & 0xFF; /* Data Length */
	sendRecvBuf[i++] = (tmpWSize) & 0xFF;

	for (j = 0; j < wSize; ++j) { /* Data */
		if (j % 2 == 0) { // for big endian
			sendRecvBuf[i++] = bData[j + 1];
		} else {
			sendRecvBuf[i++] = bData[j - 1];
		}
	}

	/* Calculate checksum */
	checkSum = Common::Instance.ComputeUart2Checksum(sendRecvBuf, i);

	sendRecvBuf[i++] = (checkSum >> 8) & 0xFF; /* Checksum */
	sendRecvBuf[i++] = (checkSum) & 0xFF;

	Common::Instance.LogSendReceiveBuffer("Req Pkt: ", sendRecvBuf, i);

	cmdLength = i;

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioSendBuffer(bCellNo, cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	expLength = (cmdLength / 2) + 8;
	respLength = expLength;
	wTimeoutInMillSec = (wTimeoutInMillSec > 10000) ? 10000 : wTimeoutInMillSec;

	memset(sendRecvBuf, 0, WAPI_SEND_RECV_BUF_SIZE);

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioReceiveBuffer(bCellNo, &respLength, wTimeoutInMillSec, sendRecvBuf, &status1));
	rc = Common::Instance.ValidateUart2ReceivedData(sendRecvBuf, cmdLength, respLength, expLength, &i, status, status1);
	if (rc == success) {
		Common::Instance.LogSendReceiveBuffer("Res Pkt: ", sendRecvBuf,
				respLength);
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Read data from drive [2K or Less]
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
Byte siReadDriveMemory2KAndLess(Byte bCellNo, Dword dwAddress, Word wSize,
		Byte *bData, Word wTimeoutInMillSec) {
	Byte *recvBuf = NULL;
	Byte sendBuf[WAPI_CMD_RESP_BUF_SIZE];
	TER_Status status;
	Byte rc = success; /* No error */
	Word cmdLength, i, j, checkSum;
	unsigned int respLength, expLength;
	unsigned int status1;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,0x%lx,%d,*,%d) \n",
			__FUNCTION__, bCellNo, dwAddress, wSize, wTimeoutInMillSec);

	if (bData == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	i = 0;
	sendBuf[i++] = 0xaa; /* Sync Pkt */
	sendBuf[i++] = 0x55;

	sendBuf[i++] = 0x05; /* Length */
	sendBuf[i++] = 0x05;

	sendBuf[i++] = 0x00; /* Opcode */
	sendBuf[i++] = 0x01;

	sendBuf[i++] = (dwAddress >> 24) & 0xFF; /* Address */
	sendBuf[i++] = (dwAddress >> 16) & 0xFF;
	sendBuf[i++] = (dwAddress >> 8) & 0xFF;
	sendBuf[i++] = (dwAddress) & 0xFF;

	sendBuf[i++] = (wSize >> 8) & 0xFF; /* Data Length */
	sendBuf[i++] = (wSize) & 0xFF;

	/* Calculate checksum */
	checkSum = Common::Instance.ComputeUart2Checksum(sendBuf, i);

	sendBuf[i++] = (checkSum >> 8) & 0xFF; /* Checksum */
	sendBuf[i++] = (checkSum) & 0xFF;

	Common::Instance.LogSendReceiveBuffer("Req Pkt: ", sendBuf, i);

	cmdLength = i;
	expLength = (cmdLength / 2) + 8 + wSize;
	respLength = expLength;

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioSendBuffer(bCellNo, cmdLength, sendBuf));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	recvBuf = (Byte *) malloc(WAPI_SEND_RECV_BUF_SIZE);
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioReceiveBuffer(bCellNo, &respLength, wTimeoutInMillSec, recvBuf, &status1));
	rc = Common::Instance.ValidateUart2ReceivedData(recvBuf, cmdLength, respLength, expLength, &i, status, status1);

	if (rc == success) {
		for (j = 0, i += 6; j < wSize; i++, j++) {
			bData[j] = recvBuf[i];
			if (dwAddress & 1) {
				bData[j] = recvBuf[i];
			} else {
				if (j % 2 == 0) {
					bData[j] = recvBuf[i + 1];
				} else {
					bData[j] = recvBuf[i - 1];
				}
			}
		}
	}

FUNCTIONEND:
	if (recvBuf) {
		free(recvBuf);
	}
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

/**
 * <pre>
 * Description:
 *   Read data from drive memory. if size > 2K, send multiple requests.
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
Byte siReadDriveMemoryMulti(Byte bCellNo, Dword dwAddress, Word wSize,
		Byte *bData, Word wTimeoutInMillSec) {
	Byte rc = 0;
	Word size, totalSize;
	Byte *ptrData;
	Dword rAddr;

	totalSize = wSize;
	ptrData = bData;
	rAddr = dwAddress;

	while (totalSize > 0) {
		if (totalSize > WAPI_MAX_BYTES_FROM_SMALL_BUFF) {
			size = WAPI_MAX_BYTES_FROM_SMALL_BUFF;
			totalSize -= size;
		} else {
			size = totalSize;
			totalSize = 0;
		}

		rc = siReadDriveMemory2KAndLess(bCellNo, rAddr, size, ptrData,
				wTimeoutInMillSec);

		if (rc == success) {
			Common::Instance.WriteToLog(
					LogTypeTrace,
					"Slot %d: siReadDriveMemory, Read %d bytes from Address 0x%08x \n",
					bCellNo, size, (int) rAddr);
			rAddr += (size / 2);
			ptrData += size;
		} else {
			Common::Instance.WriteToLog(LogTypeTrace,
					"Slot %d: siReadDriveMemory,failed \n", bCellNo);
			break;
		}
	}
	RET(rc);
}

/**
 * <pre>
 * Description:
 *    Read data from drive memory.
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
Byte siReadDriveMemory(Byte bCellNo, Dword dwAddress, Word wSize, Byte *bData,
		Word wTimeoutInMillSec) {
	Byte *recvBuf = NULL;
	Byte sendBuf[WAPI_CMD_RESP_BUF_SIZE];
	TER_Status status;
	Byte rc = success; /* No error */
	Word cmdLength, i, j, checkSum;
	unsigned int respLength, expLength;
	unsigned int status1;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,0x%lx,%d,*,%d) \n",
			__FUNCTION__, bCellNo, dwAddress, wSize, wTimeoutInMillSec);

	if (bData == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	if (dwAddress & 1) {
		Common::Instance.WriteToLog(LogTypeError,
				"!!ERROR set odd address!!\n");
		rc = terError;
		goto FUNCTIONEND;
	}

	/* Odd number of bytes can't be read ! */
	if (wSize & 1) {
		wSize++;
	}

	/* Flush sio receive buffer */
	Common::Instance.TerFlushReceiveBuffer(bCellNo);

	i = 0;
	sendBuf[i++] = 0xaa; /* Sync Pkt */
	sendBuf[i++] = 0x55;

	sendBuf[i++] = 0x05; /* Length */
	sendBuf[i++] = 0x05;

	sendBuf[i++] = 0x00; /* Opcode */
	sendBuf[i++] = 0x01;

	sendBuf[i++] = (dwAddress >> 24) & 0xFF; /* Address */
	sendBuf[i++] = (dwAddress >> 16) & 0xFF;
	sendBuf[i++] = (dwAddress >> 8) & 0xFF;
	sendBuf[i++] = (dwAddress) & 0xFF;

	sendBuf[i++] = (wSize >> 8) & 0xFF; /* Data Length */
	sendBuf[i++] = (wSize) & 0xFF;

	/* Calculate checksum */
	checkSum = Common::Instance.ComputeUart2Checksum(sendBuf, i);

	sendBuf[i++] = (checkSum >> 8) & 0xFF; /* Checksum */
	sendBuf[i++] = (checkSum) & 0xFF;

	Common::Instance.LogSendReceiveBuffer("Req Pkt: ", sendBuf, i);

	cmdLength = i;
	expLength = (cmdLength / 2) + 8 + wSize;
	respLength = expLength;
	wTimeoutInMillSec = (wTimeoutInMillSec > 10000) ? 10000 : wTimeoutInMillSec;

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioSendBuffer(bCellNo, cmdLength, sendBuf));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	recvBuf = (Byte *) malloc(WAPI_SEND_RECV_BIG_BUF_SIZE);
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioReceiveBuffer(bCellNo, &respLength, wTimeoutInMillSec, recvBuf, &status1));

	rc = Common::Instance.ValidateUart2ReceivedData(recvBuf, cmdLength,
			respLength, expLength, &i, status, status1);

		if (rc == success) {
		for (j = 0, i += 6; j < wSize; i++, j++) {
			bData[j] = recvBuf[i];
			if (dwAddress & 1) {
				bData[j] = recvBuf[i];
			} else {
				if (j % 2 == 0) {
					bData[j] = recvBuf[i + 1];
				} else {
					bData[j] = recvBuf[i - 1];
				}
			}
		}
	}

FUNCTIONEND:
	if (recvBuf) {
		free(recvBuf);
	}
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
 *  		   this function issues echo command with length = 0x160 (0x100 + 0x60)
 *  		   then copy data in offset 0x100-0x15F into *bData.
 * </pre>
 *****************************************************************************/
Byte siEchoDrive(Byte bCellNo, Word wSize, Byte *bData,
		Word wTimeoutInMillSec) {
	Byte sendRecvBuf[WAPI_SEND_RECV_BUF_SIZE];
	TER_Status status;
	Byte rc = success; /* No error */
	Word cmdLength, i, j, checkSum;
	unsigned int respLength, expLength;
	unsigned int status1;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d,*,%d) \n",
			__FUNCTION__, bCellNo, wSize, wTimeoutInMillSec);

	if (bData == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	if (Common::Instance.SanityCheck(bCellNo)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Flush sio receive buffer */
	Common::Instance.TerFlushReceiveBuffer(bCellNo);

	i = 0;
	sendRecvBuf[i++] = 0xaa; /* Sync Pkt */
	sendRecvBuf[i++] = 0x55;

	sendRecvBuf[i++] = 0x04; /* Length */
	sendRecvBuf[i++] = 0x04;

	sendRecvBuf[i++] = 0x00; /* Opcode */
	sendRecvBuf[i++] = 0x00;

	sendRecvBuf[i++] = 0x00; /* Command Flags */
	sendRecvBuf[i++] = 0x00;
	sendRecvBuf[i++] = (wSize >> 8) & 0xFF; /* Length of enquiry data*/
	sendRecvBuf[i++] = (wSize) & 0xFF;

	/* Calculate checksum */
	checkSum = Common::Instance.ComputeUart2Checksum(sendRecvBuf, i);

	sendRecvBuf[i++] = (checkSum >> 8) & 0xFF; /* Checksum */
	sendRecvBuf[i++] = (checkSum) & 0xFF;

	Common::Instance.LogSendReceiveBuffer("Req Pkt: ", sendRecvBuf, i);

	cmdLength = i;
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioSendBuffer(bCellNo, cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	expLength = (cmdLength / 2) + 8 + wSize;
	respLength = expLength;
	wTimeoutInMillSec = (wTimeoutInMillSec > 10000) ? 10000 : wTimeoutInMillSec;

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioReceiveBuffer(bCellNo, &respLength, wTimeoutInMillSec, sendRecvBuf, &status1));

	rc = Common::Instance.ValidateUart2ReceivedData(sendRecvBuf, cmdLength,
			respLength, expLength, &i, status, status1);
	if (rc == success) {
		for (j = 0, i += 6; j < wSize; i++, j++) {
			if (j % 2 == 0) {
				bData[j] = sendRecvBuf[i + 1];
			} else {
				bData[j] = sendRecvBuf[i - 1];
			}
		}
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte siGetDriveUartVersion(Byte bCellNo, Word *wVersion) {
	Byte sendRecvBuf[WAPI_CMD_RESP_BUF_SIZE];
	TER_Status status;
	Byte rc = success; /* No error */
	Word i, cmdLength;
	unsigned int respLength, expLength;
	unsigned int status1;
	*wVersion = 0;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,*) \n",
			__FUNCTION__, bCellNo);

	if (wVersion == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Flush sio receive buffer */
	Common::Instance.TerFlushReceiveBuffer(bCellNo);

	i = 0;
	sendRecvBuf[i++] = 0xaa; /* Sync Pkt */
	sendRecvBuf[i++] = 0x55;

	sendRecvBuf[i++] = 0x02; /* Length */
	sendRecvBuf[i++] = 0x02;

	sendRecvBuf[i++] = 0x00; /* Opcode */
	sendRecvBuf[i++] = 0x04;

	sendRecvBuf[i++] = 0xFF; /* Checksum */
	sendRecvBuf[i++] = 0xFC;

	Common::Instance.LogSendReceiveBuffer("Req Pkt: ", sendRecvBuf, i);

	cmdLength = i;
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioSendBuffer(bCellNo, cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	expLength = ((cmdLength / 2) + 10);
	respLength = expLength;
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioReceiveBuffer(bCellNo, &respLength, 500, sendRecvBuf, &status1));

	rc = Common::Instance.ValidateUart2ReceivedData(sendRecvBuf, cmdLength,
			respLength, expLength, &i, status, status1);

	if (rc == success)
		*wVersion = (sendRecvBuf[i + 7] << 8) | sendRecvBuf[i + 6];

FUNCTIONEND:
	if (wVersion == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Drive UART version = %d \n",
				__FUNCTION__, rc, *wVersion);
	}
	RET(rc);
}

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
Byte siSetDriveDelayTime(Byte bCellNo, Word wDelayTimeInMicroSec,
		Word wTimeoutInMillSec) {
//	Byte sendRecvBuf[WAPI_CMD_RESP_BUF_SIZE];
//	TER_Status status;
	Byte rc = success; /* No error */
//	Word i, cmdLength, checkSum;
//	unsigned int respLength, expLength;
//	unsigned int status1;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d,%d) \n",
			__FUNCTION__, bCellNo, wDelayTimeInMicroSec, wTimeoutInMillSec);


	rc = siSetInterCharacterDelay(bCellNo, (Dword)(wDelayTimeInMicroSec/1000));
	if (rc) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: siSetInterCharacterDelay failed \n");
		RET(rc);
	}
	rc = siSetAckTimeout(bCellNo, 3);
	if (rc) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: siSetAckTimeout failed \n");
		RET(rc);
	}

	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte siResetDrive(Byte bCellNo, Word wType, Word wResetFactor,
		Word wTimeoutInMillSec) {

	Byte sendRecvBuf[WAPI_CMD_RESP_BUF_SIZE];
	TER_Status status;
	Byte rc = success; /* No error */
	Word cmdLength, i, j, checkSum;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d,%d,%d) \n",
			__FUNCTION__, bCellNo, wType, wResetFactor, wTimeoutInMillSec);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	i = 0;
	sendRecvBuf[i++] = 0xaa; /* Sync Pkt */
	sendRecvBuf[i++] = 0x55;

	sendRecvBuf[i++] = 0x04; /* Length */
	sendRecvBuf[i++] = 0x04;

	sendRecvBuf[i++] = 0x00; /* Opcode */
	sendRecvBuf[i++] = 0x20;

	sendRecvBuf[i++] = (wType >> 8) & 0xFF; /* Drive type */
	sendRecvBuf[i++] = (wType) & 0xFF;

	sendRecvBuf[i++] = (wResetFactor >> 8) & 0xFF; /* Command Flags */
	sendRecvBuf[i++] = (wResetFactor) & 0xFF;

	/* Calculate checksum */
	checkSum = Common::Instance.ComputeUart2Checksum(sendRecvBuf, i);

	sendRecvBuf[i++] = (checkSum >> 8) & 0xFF; /* Checksum */
	sendRecvBuf[i++] = (checkSum) & 0xFF;

	for (j = 0; j < i; ++j)
		Common::Instance.WriteToLog(LogTypeTrace, "%02x ", sendRecvBuf[j]);

	cmdLength = i;
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioSendBuffer(bCellNo, cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, rc);
	RET(rc);
}

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
Byte siChangeToUART3DEBUG(Byte bCellNo, Word wLineSpeed, Word wTimeoutInMillSec) {

	Byte sendRecvBuf[WAPI_CMD_RESP_BUF_SIZE];
	TER_Status status;
	Byte rc = success; /* No error */
	Word i, cmdLength, checkSum;
	unsigned int respLength, expLength;
	unsigned int status1;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,0x%x) \n",
			__FUNCTION__, bCellNo, wLineSpeed);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Flush sio receive buffer */
	Common::Instance.TerFlushReceiveBuffer(bCellNo);

	i = 0;
	sendRecvBuf[i++] = 0xaa; /* Sync Pkt */
	sendRecvBuf[i++] = 0x55;

	sendRecvBuf[i++] = 0x03; /* Length */
	sendRecvBuf[i++] = 0x03;

	sendRecvBuf[i++] = 0x00; /* Opcode */
	sendRecvBuf[i++] = 0x30;

	sendRecvBuf[i++] = (wLineSpeed >> 8) & 0xFF; /* Delay value */
	sendRecvBuf[i++] = (wLineSpeed) & 0xFF;

	/* Calculate checksum */
	checkSum = Common::Instance.ComputeUart2Checksum(sendRecvBuf, i);

	sendRecvBuf[i++] = (checkSum >> 8) & 0xFF; /* Checksum */
	sendRecvBuf[i++] = (checkSum) & 0xFF;

	Common::Instance.LogSendReceiveBuffer("Req Pkt: ", sendRecvBuf, i);

	cmdLength = i;
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioSendBuffer(bCellNo, cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	expLength = (cmdLength / 2) + 8;
	respLength = expLength;
	wTimeoutInMillSec = (wTimeoutInMillSec > 10000) ? 10000 : wTimeoutInMillSec;

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SioReceiveBuffer(bCellNo, &respLength, wTimeoutInMillSec, sendRecvBuf, &status1));

	rc = Common::Instance.ValidateUart2ReceivedData(sendRecvBuf, cmdLength,
			respLength, ((cmdLength / 2) + 8), &i, status, status1);

	// Disable the FPGA UART2 mode
	Common::Instance.TerEnableFpgaUart2Mode(bCellNo, 0);

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte siSetUartProtocol(Byte bCellNo, Byte bProtocol) {
	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d) \n",
			__FUNCTION__, bCellNo, bProtocol);

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d, %d)\n",
			__FUNCTION__, bCellNo, bProtocol);

	/* Cache protocol type. This value will be used to determine endian swap */
	Common::Instance.SetUart2Protocol((Uart2ProtocolType) bProtocol);
	Common::Instance.TerEnableFpgaUart2Mode(bCellNo, 1);
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x \n",
			__FUNCTION__, success);
	RET(success);
}

/**
 * <pre>
 * Description:
 *   Gets the on-temperature status
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 * Return:
 *   0 - Not reached target temperature
 *   1 - Reached target temperature
 *   sioNotConnected (0x20) - SIO is not connected
 *   invalidSlotIndex (0x12) - Slot index is invalid
 * Note:
 *   On-temperature criteria is target temperature +/- 0.5 degreeC
 * </pre>
 *****************************************************************************/
Byte isOnTemp(Byte bCellNo) {
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte isTempReached = 0;
	Byte rc = success;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed. exit code = 0x%02x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetSlotSettings( bCellNo, &slotSettings));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status,
				TER_Notifier_Cleared);
		goto FUNCTIONEND;
	} else {
		if (slotSettings.slot_status_bits & IsTempTargetReached) {
			isTempReached = 1;
		}
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x, IsOnTemp = %s \n", __FUNCTION__, rc,
			(isTempReached == 1) ? "Yes" : "No");
	// DO NOT use the macro RET because this method needs to return the temperature reached state.
	return isTempReached;
}

/**
 * <pre>
 * Description:
 *   Set drive fan speed 
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   DriveFanRPM - INPUT - fan speed in duty_cycle [20 to 65]
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 *****************************************************************************/
Byte siSetDriveFanRPM(Byte bCellNo, int DriveFanRPM) {
	Byte rc = success; /* No error */
	TER_Status status;
	int values[1] = { -1 };

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%d) \n",
			__FUNCTION__, bCellNo, DriveFanRPM);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	DriveFanRPM = DriveFanRPM < MIN_FAN_DUTY_CYCLE ? MIN_FAN_DUTY_CYCLE : DriveFanRPM;
	DriveFanRPM = DriveFanRPM > MAX_FAN_DUTY_CYCLE ? MAX_FAN_DUTY_CYCLE : DriveFanRPM;

	/* Disable temp control */
	values[0] = DriveFanRPM;
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetTempControlEnable (bCellNo, 0));
	if (status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terPropertyFanDutyCycle, 1, values));
	if (status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte siSetElectronicsFanRPM(Byte bCellNo, int ElectronicsFanRPM) {

	/* FUNCTION IS NOT SUPPORTED */
	RET(success);
}

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
Byte siSetFanRPMDefault(Byte bCellNo) {

	/* FUNCTION IS NOT SUPPORTED */
	RET(success);
}

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
Byte siSetSupplyOverVoltageProtectionLevel(Byte bCellNo,
		Word wV5LimitInMilliVolts, Word wV12LimitInMilliVolts) {

	/* FUNCTION IS NOT SUPPORTED */
	RET(success);
}

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
Byte siSetLed(Byte bCellNo, Byte bMode) {

	/* FUNCTION IS NOT SUPPORTED */
	RET(success);
}

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
Byte isSlotThere(Byte bCellNo) {
	TER_Status status;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_IsSlotThere(bCellNo));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte siSetDebugLogLevel(Byte bCellNo, Byte bLogLevel) {
	Common::Instance.SetLogLevel(bLogLevel);
	RET(success);
}

/**
 * <pre>
 * Description:
 *   Used to set inter character delay in bit times.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wValInBitTimes - INPUT - inter character delay in bit times
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte siSetInterCharacterDelay(Byte bCellNo, Dword wValInBitTimes) {
	TER_Status status;
	Byte rc = success; /* No error */
	int values[1] = {wValInBitTimes};

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%ld) \n",
			__FUNCTION__, bCellNo, wValInBitTimes);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terPropertyInterCharacterDelay, 1, values));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte siGetInterCharacterDelay(Byte bCellNo, Dword *wValInBitTimes) {
	TER_Status status;
	Byte rc = success; /* No error */
	int values[1] = {-1};

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	if (wValInBitTimes == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wValInBitTimes = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terPropertyInterCharacterDelay, 1, values));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}
	*wValInBitTimes = values[0];

FUNCTIONEND:
	if (wValInBitTimes == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x, Inter-character delay = %d \n",
				__FUNCTION__, *wValInBitTimes);
	}
	RET(rc);
}

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
Byte siClearHaltOnError(Byte bCellNo) {
	TER_Status status;
	Byte rc = success; /* No error */

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d) \n", __FUNCTION__,
			bCellNo);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	/* Reset Tx FIFO */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_ResetSlot ( bCellNo, TER_ResetType_Reset_SendBuffer ));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	/* Reset Rx FIFO */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_ResetSlot ( bCellNo, TER_ResetType_Reset_ReceiveBuffer ));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	/* Clear halt condition */
	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_ResetSlot(bCellNo, TER_ResetType_Reset_TxHalt));
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte siSetAckTimeout(Byte bCellNo, Dword wValue) {
	TER_Status status;
	Byte rc = success; /* No error */
	int values[1] = { wValue };

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%ld) \n",
			__FUNCTION__, bCellNo, wValue);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terPropertyAckTimeOut, 1, values));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = %d \n", __FUNCTION__, rc);
	RET(rc);
}

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
Byte siGetAckTimeout(Byte bCellNo, Dword *wValue) {
	TER_Status status;
	Byte rc = success; /* No error */
	int values[1] = { -1 };

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s (%d)\n", __FUNCTION__,
			bCellNo);

	if (wValue == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - null argument has been passed\n", __FUNCTION__);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	*wValue = 0;

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_GetProperty(bCellNo, terPropertyAckTimeOut, 1, values));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	*wValue = values[0];

FUNCTIONEND:
	if (wValue == NULL) {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	} else {
		Common::Instance.WriteToLog(LogTypeTrace,
				"Exit: %s, exit code = 0x%x, ack timeout = %d\n", __FUNCTION__,
				rc, *wValue);
	}
	RET(rc);
}

/**
 * <pre>
 * Description:
 * Sets the delay between packets from 0 to 2550 usec in 10 usec increments.  
 * The default is 30 (300 usec).  This is the time from when the acknowledge 
 * is received from the drive until when the UART sends the next character. 
 *  
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wValue - INPUT - Inter packet delay in us [0..2550]. 10 us usec increments
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte siSetInterPacketDelay(Byte bCellNo, Dword wValue) {
	TER_Status status;
	Byte rc = success; /* No error */
	int values[1] = { wValue };

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d,%ld) \n",
			__FUNCTION__, bCellNo, wValue);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terPropertyInterPacketDelay, 1, values));
	if (status != TER_Status_none) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

/** ===========================================================
 *        Auxiliary/helper functions.
 * ===========================================================*/

/*!
 * Print teradyne error (used by wapitest)
 */
void terPrintError(Byte bCellNo, TER_Status status) {
	static char wbuf[256];

	(Common::Instance.GetSIOHandle())->TER_ConvertStatusToString(status, wbuf);
	printf("%s \n", wbuf);
	return;
}

/* return version */
char *terGetWapiLibVersion(void) {
	static char verbuf[128];

	sprintf(verbuf, "Build: %d\n", BUILD_NO);

	return verbuf;
}

/*!
 * Print slot info (used by wapitest)
 */
void terPrintSlotInfo(Byte bCellNo) {
	TER_Status status;
	TER_SlotInfo pSlotInfo;
	char build_version[MAX_LIB_VERSION_SIZE];
	TER_SlotUpgradeStatus slotUpgradeStatus;

	/* Print serial no/version etc.. */
	status = (Common::Instance.GetSIOHandle())->TER_GetSlotInfo(
			bCellNo, &pSlotInfo);
	if (TER_Status_none != status) {
		printf("ERROR: terPrintSlotInfo: Failed \n");
	} else {

		status = (Common::Instance.GetSIOHandle())->TER_GetSlotUpgradeStatus(bCellNo, &slotUpgradeStatus);
		if (status != TER_Status_none) {
			printf(
					"ERROR: terPrintSlotInfo failed. TER_IsDioPresent() failed with status = %d. \n",
					status);
		}

		printf(
				"DioFirmwareVersion: <DIO App:%s> <924x Init:%d.%d> <924x App:%d.%d>\n",
				pSlotInfo.DioFirmwareVersion, slotUpgradeStatus.powerInitMajorVer,
				slotUpgradeStatus.powerInitMinorVer, slotUpgradeStatus.powerFwMajorVer, slotUpgradeStatus.powerFwMajorVer);
		printf("DioPcbSerialNumber: <%s>\n", pSlotInfo.DioPcbSerialNumber);
		printf("SioFpgaVersion: <%s>\n", pSlotInfo.SioFpgaVersion);
		printf("SioPcbSerialNumber: <%s>\n", pSlotInfo.SioPcbSerialNumber);
		printf("SioTeradynePartNum: <%s>\n", pSlotInfo.SioPcbPartNum);
		printf("DioTeradynePartNum: <%s>\n", pSlotInfo.SlotPcbPartNum);

		status =
				(Common::Instance.GetSIOHandle())->TER_GetLibraryVersion(build_version);
		if (TER_Status_none != status) {
			printf(
					"ERROR: terPrintSlotInfo:  TER_GetNeptuneSio2BuildVersion() failed with status = %d. \n",
					status);
		}

		printf("Neptune SIO2 Lib (neptune_sio2.a): %s\n", build_version);
		printf("SIO2 Super App (SIO2_SuperApplication): %s\n",
				pSlotInfo.SioAppVersion);
		printf("WAPI Lib (libsi.a): %s\n", terGetWapiLibVersion());
	}
	return;
}

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
Byte adjustTemperatureControlByDriveTemperature(Byte bCellNo,
		Word wTempInHundredth) {
	Byte rc = success; /* No error */
	TER_Status status;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s(%d, %d) \n",
			__FUNCTION__, bCellNo, wTempInHundredth);

	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	status = (Common::Instance.GetSIOHandle())->TER_AdjustSlotTempBasedOnDriveTemp(bCellNo, wTempInHundredth);
	if (TER_Status_none != status) {
		Common::Instance.LogError(__FUNCTION__, bCellNo, status);
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x \n", __FUNCTION__, rc);
	RET(rc);
}

/** @page MFG Hitachi MFG
 * @n The Hitachi "One-Step" or "Forced Manufacturing Mode" sequence,
 * implemented on the Teradyne Neptune product, provides a means to trigger
 * DUTs (devices under test) to enter a manufacturing mode.
 *
 * @par Design
 * @n Sequence- The nominal sequence is expected to be:
 *    @li Power Application
 *       @n The voltages (nominally, +5 V and +12 V) are applied.
 *          @n - Concurrently (without stagger) (not configurable)
 *          @n - Standard ramping (not configurable)
 *    @li Delay
 *       @n The DUT (device under test) is "ignored" for a delay period.
 *          @n - [5, 100] ms; default is 5 ms (configurable)
 *    @li Impulse
 *       @n The DUT is stimulated by an impulse code.
 *       @n - The impulse period starts after the power delay has expired and
 *         consumes a varied period whose duration depends on the count, rate
 *         and size of the impulse transmissions.
 *       @n - @c 0x50 @c 'P' (not configurable)
 *       @n - Repeated [50, 255] times; default is 50 times (configurable)
 *       @n - Byte transmission consists of:
 *          {Start bit}
 *          {Byte/Character code point (8 bits)}
 *          {Stop bit}
 *          {Intercharacter delay (20 b; configurable)}
 *       @n - 115.2 kb/s rate (configurable)
 *       @n - At 115.2 kb/s and with a 20 b inter character delay, each
 *             byte's transmission consumes 260 microseconds
 *       @n - Therefore, 50 bytes consume 13+ ms, while 255 consume
 *             66+ ms
 *       @n - During this period, only the Field Programmable Gate
 *             Array (FPGA) interacts with the DUT.
 *       @n - The delay and impulse periods must not consume more than
 *             140 ms.
 *    @li Pause
 *       @n ...period after impulse stimulus and before beginning of response
 *       tracking...
 *          @n - 14 ms (not configurable)
 *          @n - During this period, only the FPGA interacts with the DUT.
 *          @n - The delay, impulse and pause periods must not consume
 *             more than 154 ms.
 *    @li Response
 *    @n The DUT's response to the impulse stimulus is to reply with a
 *    signature (sequence of bytes).
 *          @n - The response period is varied in that it begins after the
 *             pause (which must end on or before 154 ms after power
 *             application) and ends 300 ms after power application.
 *          @n - Signature expected to be @c 0x4D, @c 0x46 and then
 *             @c 0x47 (@c 'M', @c 'F' and then @c 'G')
 *          @n - Reference signature (the one that is expected) is
 *             configurable to any sequence of one (1) through 255 bytes;
 *             default is @c {0x4D, @c 0x46, @c 0x47} (three (3) bytes).
 *          @n - The response is compared with the expected reference
 *             signature to yield a status: {match : @c status,
 *             differ : fail}.
 *          @n - During this period, only the FPGA interacts with the DUT.
 *
 * @par Pattern
 * @n The software is designed around the preparation/execution pattern:
 *    @li Attributes are set (prepared)
 *       @n - Delay configured or defaulted: [5, 100] ms; default is 5 ms
 *       @n - Impulse count configured or defaulted: [50, 255] times;
 *          default is 50 times
 *       @n - Intercharacter delay configured or defaulted: [1, 50];
 *          default is 20 bits
 *       @n - Transmission rate configured: @ref siSetUartBaudrate
 *       @n - Response configured or defaulted: @c 0x4D, @c 0x46 and then
 *          @c 0x47 (@c 'M', @c 'F' and then @c 'G')
 *    @li Power Sequence is Executed: see @ref siSetPowerEnableMfgMode
 *       @n - Power is applied
 *       @n - Wrapper software quiesces (sleeps) until evaluation 500 ms
 *          (not configurable)
 *       @n - Response signature is compared against reference signature
 *          to yield a returned status {match : @c status, differ : fail}
 * @n The pattern is such that attribute preparation is optional if only the
 * defaults are desired. Thus, the only, required method is the power method:
 * @ref siSetPowerEnableMfgMode. The duration limit for power application
 * through response acquisition is 300 ms. Specifications of counts and/or
 * delays that would cause exceeding this limit result in a rejection when the
 * primary method (@ref siSetPowerEnableMfgMode) is invoked.
 *
 * @par Application Programming Interface
 * @n All the API's constants are declared as C-language symbolic constants.
 * The module (really, a submodule) includes the headers (@ref commonEnums.h,
 * @ref libsi.h and @ref tcTypes.h) that defines three types (listed, here, with
 * typical domains):
 * @verbatim
	Type    Minimum Value   Maximum Value   Size (bytes)
	Byte        0                  255       1
	DWord       0           4294967295       4
	Word        0                65535       2                     @endverbatim
 * @n Use of values outside of the @c Byte type's domain ([0, 255]) is
 * discouraged. Thus, many status and working values are derived from the
 * domain [0, 255].
 *
 * @li Most of the API's methods are restricted to accepting arguments whose
 * types are only @c Byte, @c DWord or @c Word or references (pointers) to
 * @c Byte, @c DWord or @c Word.
 *
 * @li Most of the API's methods return @c Byte type status codes declared in
 * the @c status enumeration in @ref commonEnums.h.
 *    @n - Success is indicated by zero (0) (@c status).
 *    @n - Status codes that detail anomaly detections are drawn from the
 *         @c status enumeration.
 *
 * @li All the API's methods are to be considered blocking: execution is
 * co-opted until the method completes. Some methods are implemented as macros
 * rather than functions.
 */

/* === Constants === */

#define /*Byte Maximum value*/ MFGByteMAX 255U
#define /*Sleep Interval (see siSetPowerEnableMfgMode)*/ MFGSlpIVL 500000UL

/* === Utility Methods (macros) === */

#define /*Maximum Value*/ MFGMax(a, b) ((a) >= (b)? (a) : (b))
#define /*Minimum Value*/ MFGMin(a, b) ((a) <= (b)? (a) : (b))
#define /*"NULL" for NULL*/	MFGN4N(ADR) ((ADR)? (ADR) : "NULL")

/* === Community Items (local but not extern) === */

/* --- Signature Length and Value hold the expected signature (see
 * siGet/SetSignatureMfgMode). */
static size_t /*Signature Length*/	mfgSigLen = MFGSignatureLengthDEFAULT;
static Byte /*Signature Value*/   	mfgSigVlu[MFGByteMAX] =
		MFGSignatureDEFAULT;

/* === Methods ===
 * An attempt is made to check status of every call in the benign execution
 * path. Sometimes (when returned values are other than anomaly reports), such
 * checking is useless and obscures logic; such bypasses are left as implicit
 * (void) calls. */

/** @details Entry performs a common set of validations...
 * @param [in] EnS (Entry String) describes the entry configuration...
 * @param [in] bCellNo (cELL nUMBER) specifies the Neptune slot identity, a
 * 	@c Byte value in the domain [@ref SLOT_INDEX_MIN (typically, 1),
 * 	@ref SLOT_INDEX_MAX (typically, 144)].
 * @returns @c Byte @c commonEnums.h @c status, suitable for direct returns...
 */
static Byte MFGEntry(const char EnS[], Byte bCellNo) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	Byte /*Result Code*/rc;
	rc = (Byte) success;

	/* --- Log, at tracing level, the entry string. Then, validate the SIO
	 * handle. */
	Common::Instance.WriteToLog(LogTypeTrace, "ENTRY: %s\n", MFGN4N(EnS));
	/* Sanity check... */
	rc = (Byte) Common::Instance.SanityCheck(bCellNo);
	if (/*anomaly?*/rc != (Byte) success) { /* Report... */
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s: SanityCheck(%u) == %u (0x%X)\n", MFGN4N(EnS),
				bCellNo, rc, rc);
	}
	return rc;
}

/** @details Exit performs a common set of exit services...
 * @param [in] EnS (Entry String) describes the entry configuration...
 * @param [in] RsC (Result Code) specifies the candidate, result code...
 * @returns result code (@c RsC)...
 */
static Byte MFGExit(const char EnS[], Byte RsC) {
	/* --- Provide anomaly triggered exit reporting, exit trace logging and
	 * return the specified result code. */
	Common::Instance.WriteToLog(
			(/*OK?*/ RsC == (Byte)success)? LogTypeTrace : LogTypeError,
			"EXIT %s: exit %u (0x%X)\n", MFGN4N(EnS), RsC, RsC);
	return RsC;
}

Byte siSetPowerEnableMfgMode(Byte bCellNo, Word wV5InMilliVolts,
		Word wV12InMilliVolts) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	int /*Duration (ms)*/               	dn;
	Word /*Power Delay (ms)*/           	dP;
	char /*Entry description*/          	en[4096];
	char /*Error String*/               	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/                	rc;
	size_t /*Received signature Length*/	rL;
	Byte /*Received Signature*/         	rS[MFGByteMAX + 1];
	TerSioLib* /*SIO Handle*/            	sH;
	unsigned int /*SIO Status*/             sS;
	TER_Status /*Teradyne Status*/      	status;
	int values[1] = { -1 };

	/* --- Prepare local(s), C-style. */
	dn = 0;
	dP = 0;
	sprintf(en, "%s(%u, %u, %u)", __FUNCTION__, bCellNo, wV5InMilliVolts,
			wV12InMilliVolts);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;
	rL = sizeof(rS);
	memset(rS, 0, sizeof(rS));
	sH = Common::Instance.GetSIOHandle();
	sS = 0;
	status = TER_Status_none;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Perform the duration check. If the total duration of the delays from
	 * recognition of power application through the end of the impulse stimulus
	 * is greater than the allowed maximum (MFGDelayLIMIT), reject the request
	 * (quit). */
	dn = siGetImpulseDurationMfgMode(bCellNo);
	if (/*anomaly?*/ dn < 0) {
		/* --- The delay calculator (siGetDelayMfgMode) detected O(and
		 * reported) an anomaly: the returned duration (dn) is, therefore,
		 * negative. The value is derived from an internal result code, assured
		 * to be in the domain [1, 255]. Therefore, duration (dn) is in the
		 * domain [-255, -1]: negate the duration (dn) to obtain the proper
		 * result code, and quit. */
		dn = -dn;
		assert(dn < 255L);
		rc = (Byte)dn;
		goto MethodEND;
	}
	assert(dn >= 0);
	if (/*anomaly?*/ siGetPowerDelayMfgMode(bCellNo, &dP) != (Byte)success) {
		/* --- The method (siGetPowerDelayMfgMode) has reported, already, the
		 * anomaly. */
		goto MethodEND;
	}
	dn += (long)dP;
	if (/*OK?*/ dn <= MFGDelayLIMIT) { /* OK: ensure OK status. */
		rc = (Byte)success;
		Common::Instance.WriteToLog(LogTypeTrace,
				"TRACE %s[%u]: delay (%d ms) OK\n", en, __LINE__, dn);
	} else /*too long?*/ { /* Set status, report & abort. */
		rc = (Byte)outOfRange;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: total, delay duration (%d ms) is greater than"
					" allowed (%d ms); action aborted\n", en, __LINE__, dn,
				MFGDelayLIMIT);
		goto MethodEND;
	}

	/* --- Enable forced manufacturing mode by calling TER_SetProperty to set
	 * terPropertyEnablePowerMfgMode to true / one (1). If OK, there must be
	 * corresponding call, near end of code, to reset property back to false
	 * zero (0); as a result, this is the last code paragraph that can use a
	 * goto. */
	assert(sH);
	values[0] = 1;
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_SetProperty(bCellNo,
			terPropertyPowerEnableMfgMode, 1, values));
	if (/*OK?*/ status == TER_Status_none) { /* OK: continue. */
		Common::Instance.WriteToLog(LogTypeTrace,
				"TRACE %s[%u]: TER_SetProperty(%d, "
					"terPropertyPowerEnableMfgMode(%d), 1) OK\n", en,
				__LINE__, bCellNo, terPropertyPowerEnableMfgMode);
	} else /*anomaly*/ { /* Set status, report & abort. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_SetProperty(%d, "
					"terPropertyPowerEnableMfgMode(%d), 1) == %d "
					"(%u, 0x%X, %s)\n", en, __LINE__, bCellNo,
				terPropertyPowerEnableMfgMode, status,
				(unsigned int)status, (unsigned int)status, eS);
		goto MethodEND;
	}

	/* --- Arm application of specified potentials (voltages) by calling
	 * setTargetVoltage. This validates, also, both parameters. There is no
	 * need to disarm them; subsequent procedures must set them, anew,
	 * anyway. */
	rc = (Byte)setTargetVoltage(bCellNo, wV5InMilliVolts, wV12InMilliVolts);
	if (/*OK?*/ rc == (Byte)success) { /* OK: continue. */
		Common::Instance.WriteToLog(LogTypeTrace,
				"TRACE %s[%u]: setTargetVoltage(%u, %u mV, %u mV) OK\n", en,
				__LINE__, bCellNo, wV5InMilliVolts, wV12InMilliVolts);
	} else /*anomaly*/ { /* Report & abort. */
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: setTargetVoltage(%u, %u mV, %u mV) == "
					"%u (0x%X)\n",
				en, __LINE__, bCellNo, wV5InMilliVolts, wV12InMilliVolts, rc,
				rc);
		goto MethodEND;
	}

	/* --- Sleep until the power delay, impulse stimulus and signature
	 * acquisition are complete. Since the Linux sleep method accepts intervals
	 * in seconds, only, use, instead, Linux usleep, which accepts intervals in
	 * microseconds. Note, however, that some implementations of usleep reject
	 * any interval greater than (and, maybe, also, equal to) 1000000 us (1 s).
	 * Anomalies are announced by non-zero return and diagnostic code in
	 * errno. */
	if (/*OK?*/ !usleep(MFGSlpIVL)) { /* OK: continue. */
		Common::Instance.WriteToLog(LogTypeTrace,
				"TRACE %s[%u]: usleep(%lu us) OK\n", en, __LINE__, MFGSlpIVL);
	} else /*anomaly*/ { /* Set status, report & abort. */
		rc = (Byte)terError;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: usleep(%lu us) anomaly; errno == %d\n", en,
				__LINE__, MFGSlpIVL, errno);
		goto MethodEND;
	}

	/* --- Acquire the DUT's signature and compare it with the expected
	 * signature. */
	rL = sizeof(rS);
	assert(sH);
	sS = 0;
	Common::Instance.WriteToLog(LogTypeTrace,
			"TRACE %s[%u]: TER_sioReceiveBuffer(%d, %p->%lu, 0, %p, %p->%u)\n",
			en, __LINE__,
			bCellNo, &rL, rL, rS, &sS, sS);
	WAPI_CALL_RPC_WITh_RETRY(status =
			sH->TER_SioReceiveBuffer(bCellNo, &rL, 0, rS, &sS));
	if (/*anomaly?*/ status != TER_Status_none) { /* Set status, report & abort. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_sioReceiveBuffer(%d, %p->%lu, 0, %p, %p->%u) "
					"== %d (%u, 0x%X, %s)\n   received %s[%u]\n",
				en, __LINE__,
				bCellNo, &rL, sizeof(rS), rS, &sS, sS,
				status, (unsigned int)status, (unsigned int)status, eS,
				Common::Sig2Str(rS, MFGMax(rL, mfgSigLen)), rL);
		goto MethodEND;
	}
	Common::Instance.WriteToLog(LogTypeTrace,
			"TRACE %s[%u]: TER_sioReceiveBuffer(%d, %p->%lu, 0, %p, %p->%u) "
				"== %d (%u, 0x%X)\n\n   received %s[%u]\n",
			en, __LINE__,
			bCellNo, &rL, rL, rS, &sS, sS,
			status, (unsigned int)status, (unsigned int)status,
			Common::Sig2Str(rS, MFGMax(rL, mfgSigLen)), rL);
	if (/*OK?*/ (rL == mfgSigLen) && !memcmp(rS, mfgSigVlu, rL)) { /*OK*/
		rc = (Byte)success;
		Common::Instance.WriteToLog(LogTypeError,
				"TRACE %s[%u]: signature match; OK\n", en, __LINE__);
	} else /*not OK: no match*/ { /* Set status, report & abort. */
		rc = (Byte)invalidAck;
		strcpy(eS, Common::Sig2Str(mfgSigVlu, mfgSigLen));
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: signature mismatch"
					" (expected %s[%u] != received %s[%u])\n",
				en, __LINE__, eS, mfgSigLen,
				Common::Sig2Str(rS, MFGMax(rL, mfgSigLen)), rL);
		goto MethodEND;
	}

	/* --- Tidy: disable force manufacturing mode by calling TER_SetProperty
	 * to reset terPropertyEnablePowerMfgMode to false zero (0). */
	assert(sH);
	values[0] = 0;
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_SetProperty(bCellNo,
			terPropertyPowerEnableMfgMode, 1, values));
	if (/*anomaly?*/ status != TER_Status_none ) {
		/* --- Set the result code (rc), report the anomaly and abort. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_SetProperty == %d (%u, 0x%X, %s)\n", en,
				__LINE__, status, (unsigned int)status, (unsigned int)status, eS);
	}

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

Byte siGetImpulseCodeMfgMode(Byte bCellNo, Byte* pbImpCode) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/    	en[2048];
	char /*Error String*/         	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/          	rc;
	TerSioLib* /*SIO Handle*/  		sH;
	TER_Status /*Teradyne Status*/	status;
	int values[1] = { -1 };

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, %p)", __FUNCTION__, bCellNo, (void*)pbImpCode);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;
	sH = Common::Instance.GetSIOHandle();
	status = TER_Status_none;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Validate target pointer parameter. */
	if (/*invalid?*/ !pbImpCode) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)nullArgumentError;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid target pointer\n", en, __LINE__);
		goto MethodEND;
	}

	/* --- Acquire the impulse code. Note that TER_GetProperty fills an
	 * unsigned int item, while this method fills a Byte item: fill a local,
	 * unsigned int item and then use its value to fill the item pointed to by
	 * *pbImpCode. */
	assert(sH);
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_GetProperty(bCellNo,
			terPropertyImpulseCodeMfgMode, 1, values));
	if (/*anomaly?*/ status != TER_Status_none ) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_GetProperty"
					"(terPropertyImpulseCodeMfgMode) == %d"
					"(%u, 0x%X)\n", en, __LINE__, status, (unsigned int)status,
				(unsigned int)status);
		goto MethodEND;
	}

	/* --- Apply the fetched value and ensure the success code.. */
	*pbImpCode = (Byte)(/*safety mask*/ 0xFFU & values[0]);
	assert(rc == (Byte)success);
	Common::Instance.WriteToLog(LogTypeError,
			"TRACE %s[%u]: impulse code (0x%02X) fetched\n", en, __LINE__,
			*pbImpCode);

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

Byte siSetImpulseCodeMfgMode(Byte bCellNo, Byte bImpCode) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/    	en[2048];
	char /*Error String*/         	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/          	rc;
	TerSioLib* /*SIO Handle*/  		sH;
	TER_Status /*Teradyne Status*/	status;
	int values[1] = { bImpCode };

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, 0x%02X)", __FUNCTION__, bCellNo, bImpCode);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;
	sH = Common::Instance.GetSIOHandle();
	status = TER_Status_none;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- No validation of the impulse code (bImpCode) is necessary: all,
	 * possible values are valid. Apply the code. */
	assert(sH);
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_SetProperty(bCellNo,
				terPropertyImpulseCodeMfgMode, 1, values));
		if (/*anomaly?*/status != TER_Status_none ) {
			/* --- Set the result code (rc), report the anomaly and quit.. */
			rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
			sH->TER_ConvertStatusToString(status, eS);
			Common::Instance.WriteToLog(LogTypeError,
					"ERROR %s[%u]: TER_SetProperty(terPropertyImpulseCodeMfgMode) =="
					" %d (%u, 0x%X)\n", en, __LINE__, status, (unsigned int)status,
					(unsigned int)status);
			goto MethodEND;
		}
		assert(rc == (Byte)success);
		Common::Instance.WriteToLog(LogTypeError,
				"TRACE %s[%u]: impulse code set to 0x%02X\n", en, __LINE__,
				bImpCode);

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

Byte siGetImpulseCountMfgMode(Byte bCellNo, Word* pwImpCount) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/    	en[2048];
	char /*Error String*/         	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/          	rc;
	TerSioLib* /*SIO Handle*/  		sH;
	TER_Status /*Teradyne Status*/	status;
	int values[1] = { -1 };

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, %p)", __FUNCTION__, bCellNo,
			(void*)pwImpCount);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;
	sH = Common::Instance.GetSIOHandle();
	status = TER_Status_none;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Validate target pointer parameter. */
	if (/*invalid?*/ !pwImpCount) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)nullArgumentError;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid target pointer\n", en, __LINE__);
		goto MethodEND;
	}

	/* --- Acquire the impulse count. Note that TER_GetProperty fills an
	 * unsigned int item with the fetched value, while this method fills a Word
	 * item: fill a local, unsigned int item and then use its value to fill
	 * item *pwImpCount. */
	assert(sH);
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_GetProperty(bCellNo,
			terPropertyImpulseCountMfgMode, 1, values));
	if (/*anomaly?*/ status != TER_Status_none ) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_GetProperty(terPropertyImpulseCountMfgMode) =="
					" %d (%u, 0x%X)\n", en, __LINE__, status, (unsigned int)status,
				(unsigned int)status);
		goto MethodEND;
	}
	assert(rc == (Byte)success);
	*pwImpCount = (Word)(/*safety mask*/ 0xFFFFU & values[0]);
	Common::Instance.WriteToLog(LogTypeError,
			"TRACE %s[%u]: impulse count (%u) fetched\n", en, __LINE__,
			*pwImpCount);

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

Byte siSetImpulseCountMfgMode(Byte bCellNo, Word wImpCount) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/    	en[2048];
	char /*Error String*/         	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/          	rc;
	TerSioLib* /*SIO Handle*/  	sH;
	TER_Status /*Teradyne Status*/	status;
	int values[1] = { wImpCount };

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, %u)", __FUNCTION__, bCellNo, wImpCount);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;
	sH = Common::Instance.GetSIOHandle();
	status = TER_Status_none;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Validate the candidate value parameter and apply the value. */
	if (/*invalid?*/ (wImpCount < MFGImpulseCountMINIMUM) ||
				(wImpCount > MFGImpulseCountMAXIMUM)) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)outOfRange;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid candidate value\n", en, __LINE__);
		goto MethodEND;
	}
	assert(sH);
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_SetProperty(bCellNo,
			terPropertyImpulseCountMfgMode,
			1, values));
	if (/*anomaly?*/ status != TER_Status_none ) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_SetProperty(terPropertyImpulseCountMfgMode) =="
					" %d (%u, 0x%X)\n", en, __LINE__, status, (unsigned int)status,
				(unsigned int)status);
		goto MethodEND;
	}
	assert(rc == (Byte)success);
	Common::Instance.WriteToLog(LogTypeError,
			"TRACE %s[%u]: impulse count set to %u\n", en, __LINE__, wImpCount);

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

#define MFGBaudotRateMINIMUM 38400U
int siGetImpulseDurationMfgMode(Byte bCellNo) {
	Dword /*Baudot Rate (b/s)*/       	bR;
	Word /*Impulse Count*/            	cI;
	Word /*Inter character Delay (b)*/	dI;
	int /*Duration (ms, returned)*/   	dn;

	/* --- Prepare local(s) by acquiring the data. Use the duration (dn) to
	 * hold the result code. Assuming that success is zero (0), any value other
	 * than zero (0) indicates an anomaly. */
	assert(outOfRange && !success);
	dn = -(int)siGetUartBaudrate(bCellNo, &bR);
	if (/*anomaly?*/ dn != (Byte)success) {goto MethodEND;}
	if (/*invalidity?*/ bR < MFGBaudotRateMINIMUM) {
		dn = -outOfRange;
		goto MethodEND;
	}
	dn = siGetImpulseCountMfgMode(bCellNo, &cI);
	if (/*anomaly?*/ dn != (Byte)success) {goto MethodEND;}
	if ((cI < MFGImpulseCountMINIMUM) || (cI > MFGImpulseCountMAXIMUM)) {
		dn = -outOfRange;
		goto MethodEND;
	}
	dn = siGetIntercharacterDelayMfgMode(bCellNo, &dI);
	if (/*anomaly?*/ dn != (Byte)success) {goto MethodEND;}
	if (/*(dI < MFGIntercharacterDelayMINIMUM) ||*/        /*zero (0)*/
			(dI > MFGIntercharacterDelayMAXIMUM)) {
		dn = -outOfRange;
		goto MethodEND;
	}

	/* --- Calculate the total, number of bits (impulse size) to send and then
	 * the time (ms) to send them. Note that the rate is in bits/second while
	 * the time is in milliseconds: multiply the impulse size (dn) by 1000 in
	 * order to compensate. The maximum impulse size is no more than 10455,
	 * given the maximum impulse count and inter character delay; 1000 times
	 * 10455 is 10455000, which is less than INT_MAX, assuming an int size of
	 * 32 bits. */
	assert((cI >= MFGImpulseCountMINIMUM) && (cI <= MFGImpulseCountMAXIMUM));
	assert(/*(dI >= MFGIntercharacterDelayMINIMUM) &&*/    /*zero (0)*/
			(dI <= MFGIntercharacterDelayMAXIMUM));
	dn = (/*[0, 31]*/ (int)dI) + 10;
	dn *= /*[1, 255]*/ (int)cI;
	assert(bR >= MFGBaudotRateMINIMUM);
	assert(dn <= 10455);
	dn *= 1000;
	assert(dn <= 10455000);
	dn /= (int)bR;
	Common::Instance.WriteToLog(LogTypeTrace,
			"TRACE %s(%u)[%u]: Baudot rate == %lu b/s; Impulse count == %u;"
				" Intercharacter delay == %u b; Duration == %d ms\n",
			__FUNCTION__, bCellNo, __LINE__, bR, cI, dI, dn);

MethodEND:
	return dn;
}
#undef MFGBaudotRateMINIMUM

Byte siGetIntercharacterDelayMfgMode(Byte bCellNo, Word* pwValInBitTimes) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/    	en[2048];
	char /*Error String*/         	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/          	rc;
	TerSioLib* /*SIO Handle*/  	sH;
	TER_Status /*Teradyne Status*/	status;
	int values[1] = { -1 };

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, %p)", __FUNCTION__, bCellNo, (void*)pwValInBitTimes);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;
	sH = Common::Instance.GetSIOHandle();
	status = TER_Status_none;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Validate target pointer parameter. */
	if (/*invalid?*/ !pwValInBitTimes) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)nullArgumentError;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid target pointer\n", en, __LINE__);
		goto MethodEND;
	}

	/* --- Acquire the inter character delay. Note that TER_GetProperty fills
	 * an unsigned int item with the fetched value, while this method fills a
	 * Word item: fill a local unsigned int item and then use its value to fill
	 * item *pwValInBitTimes. */
	assert(sH);
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_GetProperty(bCellNo,
			terPropertyInterCharacterDelayMfgMode, 1, values));
	if (/*anomaly?*/ status != TER_Status_none ) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_GetProperty"
					"(terPropertyIntercharacterDelayMfgMode) == %d"
					" (%u, 0x%X)\n", en, __LINE__, status, (unsigned int)status,
				(unsigned int)status);
		goto MethodEND;
	}
	assert(rc == (Byte)success);
	*pwValInBitTimes = (Word)(/*safety mask*/ 0xFFFFU & values[0]);
	Common::Instance.WriteToLog(LogTypeError,
			"TRACE %s[%u]: inter character delay (%u) fetched\n", en, __LINE__,
			*pwValInBitTimes);

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

Byte siSetIntercharacterDelayMfgMode(Byte bCellNo, Word wValInBitTimes) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/    	en[2048];
	char /*Error String*/         	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/          	rc;
	TerSioLib* /*SIO Handle*/  	sH;
	TER_Status /*Teradyne Status*/	status;
	int values[1] = { wValInBitTimes };

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, %u)", __FUNCTION__, bCellNo, wValInBitTimes);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;
	sH = Common::Instance.GetSIOHandle();
	status = TER_Status_none;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Validate the candidate value parameter and apply the value. */
	if (/*invalid?*/ wValInBitTimes > MFGIntercharacterDelayMAXIMUM) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)outOfRange;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid candidate value\n", en, __LINE__);
		goto MethodEND;
	}
	assert(sH);
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_SetProperty(bCellNo,
			terPropertyInterCharacterDelayMfgMode,
			1, values));
	if (/*anomaly?*/ status != TER_Status_none ) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_SetProperty"
					"(terPropertyInterCharacterDelayMfgMode) == %d"
					"(%u, 0x%X)\n", en, __LINE__, status, (unsigned int)status,
				(unsigned int)status);
		goto MethodEND;
	}
	assert(rc == (Byte)success);
	Common::Instance.WriteToLog(LogTypeError,
			"TRACE %s[%u]: inter character delay set to %u\n", en, __LINE__,
			wValInBitTimes);

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

Byte siGetPowerDelayMfgMode(Byte bCellNo, Word* pwDelay) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/    	en[2048];
	char /*Error String*/         	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/          	rc;
	TerSioLib* /*SIO Handle*/  	sH;
	TER_Status /*Teradyne Status*/	status;
	int values[1] = { -1 };

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, %p)", __FUNCTION__, bCellNo, (void*)pwDelay);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;
	sH = Common::Instance.GetSIOHandle();
	status = TER_Status_none;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Validate target pointer parameter. */
	if (/*invalid?*/ !pwDelay) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)nullArgumentError;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid target pointer\n", en, __LINE__);
		goto MethodEND;
	}

	/* --- Acquire the power delay. Note that TER_GetProperty fills an unsigned
	 * int item with the fetched value, while this method fills a Word item:
	 * fill a local unsigned int item and then use its value to fill item
	 * *pwDelay. */
	assert(sH);
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_GetProperty(bCellNo,
			terPropertyPowerOnDelayMfgMode, 1, values));
	if (/*anomaly?*/ status != TER_Status_none) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_GetProperty(terPropertyPowerOnDelayMfgMode)"
					" == %d (%u, 0x%X)\n", en, __LINE__, status, (unsigned int)status,
				(unsigned int)status);
		goto MethodEND;
	}
	assert(rc == (Byte)success);
	*pwDelay = (Word)(/*safety mask*/ 0xFFFFU & values[0]);
	Common::Instance.WriteToLog(LogTypeError,
			"TRACE %s[%u]: power delay (%u) fetched\n", en, __LINE__,
			*pwDelay);

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

Byte siSetPowerDelayMfgMode(Byte bCellNo, Word wDelay) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/    	en[2048];
	char /*Error String*/         	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/          	rc;
	TerSioLib* /*SIO Handle*/  	sH;
	TER_Status /*Teradyne Status*/	status;
	int values[1] = { wDelay };

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, %u)", __FUNCTION__, bCellNo, wDelay);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;
	sH = Common::Instance.GetSIOHandle();
	status = TER_Status_none;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Validate the candidate value parameter and apply the value. */
	if (/*invalid?*/ (wDelay < MFGPowerDelayMINIMUM) ||
				(wDelay > MFGPowerDelayMAXIMUM)) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)outOfRange;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid candidate value\n", en, __LINE__);
		goto MethodEND;
	}
	assert(sH);
	WAPI_CALL_RPC_WITh_RETRY(status = sH->TER_SetProperty(bCellNo,
			terPropertyPowerOnDelayMfgMode,
			1, values));
	if (/*anomaly?*/ status != TER_Status_none ) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
		sH->TER_ConvertStatusToString(status, eS);
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: TER_SetProperty(terPropertyPowerOnDelayMfgMode)"
					" == %d (%u, 0x%X)\n", en, __LINE__, status, (unsigned int)status,
				(unsigned int)status);
		goto MethodEND;
	}
	assert(rc == (Byte)success);
	Common::Instance.WriteToLog(LogTypeError,
			"TRACE %s[%u]: power delay set to %u\n", en, __LINE__, wDelay);

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

Byte siGetSignatureMfgMode(Byte bCellNo, void* pbSignature,
		Byte* pbSignatureLength) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/	en[2048];
	char /*Error String*/     	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/      	rc;

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, %p, %p)", __FUNCTION__, bCellNo, pbSignature,
			(void*)pbSignatureLength);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Validate parameters and use them to report signature's length and
	 * value. */
	if (/*invalid?*/ !pbSignature || !pbSignatureLength) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)nullArgumentError;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid target pointer\n", en, __LINE__);
		goto MethodEND;
	}
	*pbSignatureLength = mfgSigLen;
	memcpy(pbSignature, mfgSigVlu, mfgSigLen);
	assert(rc == (Byte)success);
	Common::Instance.WriteToLog(LogTypeTrace,
			"TRACE %s[%u]: signature (%s) fetched\n", en, __LINE__,
			Common::Sig2Str(pbSignature, *pbSignatureLength));

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

Byte siSetSignatureMfgMode(Byte bCellNo, const void* pbSignature,
		Byte bSignatureLength) {
	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char /*Entry description*/	en[2048];
	char /*Error String*/     	eS[MAX_ERROR_STRING_LENGTH];
	Byte /*Result Code*/      	rc;

	/* --- Prepare local(s), C-style. */
	sprintf(en, "%s(%u, %p, %u)", __FUNCTION__, bCellNo, pbSignature,
			bSignatureLength);
	memset(eS, 0, sizeof(eS));
	rc = (Byte)success;

	/* --- Perform the common, entry validation. */
	rc = MFGEntry(en, bCellNo);
	if (/*anomaly?*/ rc != (Byte)success) {
		/* --- The entry check method (MFGEntry) has reported, already, the
		 * anomaly: just quit.. */
		goto MethodEND;
	}

	/* --- Validate parameters and use them to apply new signature's length and
	 * value. */
	if (/*invalid?*/ !pbSignature) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)nullArgumentError;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid target pointer\n", en, __LINE__);
		goto MethodEND;
	}
	if (/*invalid?*/ !bSignatureLength) {
		/* --- Set the result code (rc), report the anomaly and quit.. */
		rc = (Byte)outOfRange;
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR %s[%u]: invalid length\n", en, __LINE__);
		goto MethodEND;
	}
	mfgSigLen = (size_t)bSignatureLength;
	memcpy(mfgSigVlu, pbSignature, bSignatureLength);
	assert(rc == (Byte)success);
	Common::Instance.WriteToLog(LogTypeTrace,
			"TRACE %s[%u]: signature set to %s\n", en, __LINE__,
			Common::Sig2Str(mfgSigVlu, mfgSigLen));

MethodEND:
	MFGExit(en, rc);
	RET(rc);
}

unsigned long ElapsedTimeIn_mS ( struct timeval *ptvStart, struct timeval *ptvEnd)
//---------------------------------------------------------------------------
{
        unsigned long ElapsedMs;
        unsigned long ElapsedWholeSecs;
        unsigned long Elapsed_us_part;
        if (ptvEnd->tv_sec > ptvStart->tv_sec){
                ElapsedWholeSecs = ptvEnd->tv_sec - ptvStart->tv_sec;
                if (ptvEnd->tv_usec >= ptvStart->tv_usec)
                {
                        Elapsed_us_part = ptvEnd->tv_usec - ptvStart->tv_usec;
                }
                else
                {
                        Elapsed_us_part = (ptvEnd->tv_usec + 1000000) - ptvStart->tv_usec;
                        ElapsedWholeSecs--;
                }
                if((4294968<=ElapsedWholeSecs)||((4294967==ElapsedWholeSecs)&&(296000<=Elapsed_us_part))){
                	ElapsedMs = 0xffffffff;
		}else{
                	ElapsedMs = (ElapsedWholeSecs * 1000) + (Elapsed_us_part/1000);
		}
        }else if(ptvEnd->tv_sec==ptvStart->tv_sec){
               if (ptvEnd->tv_usec >= ptvStart->tv_usec)
                {
                        ElapsedMs = (ptvEnd->tv_usec - ptvStart->tv_usec)/1000;
                }
                else
                {
                        ElapsedMs=0;
                }
        }else{
                ElapsedMs=0;
        }
//              LOG_TRACE "start:%ld:%ld end:%ld:%ld elapsed %ld ", ptvStart->tv_sec,ptvStart->tv_usec,ptvEnd->tv_sec,ptvEnd->tv_usec,ElapsedMs);
        return ElapsedMs;
}

Byte getCellCommunicationStatus(Byte bCellNo){
        return 0;
}


#ifdef __cplusplus
};
#endif
