/*!
 * @file
 * The file contains the implementation of the Hitachi Interface
 * functions in terms of Teradyne TER_ API functions.
 * 
 * The functions required by Hitachi are prototyped in libsi.h.
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
 *   8) Added siGetLastErrors for debugging SIO errors. This will be called in terError(__FUNCTION__, bCellNo, ns2, status)(). 
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
 */


/*!
 * System Include Files
 */
#include <alloca.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>


/*
 * Teradyne Specific Include Files
 */

#include "neptune_sio2.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "ver.h"

#include "../testcase/tcTypes.h"
//#include "tcTypes.h"
#include "libsi.h"
#include "neptune_hitachi_interface.h"
#include "neptune_hitachi_comm.h"


#define UART_PULLUP_VOLTAGE_DEFAULT 	2700
#define WAPI_DEBUG_TRACE_DISABLE 		0
#define WAPI_DEBUG_TRACE_ENABLE 		1
#define WAPI_V5_LIMIT_IN_MILLI_AMPS 	3000
#define WAPI_V12_LIMIT_IN_MILLI_AMPS 	0
#define WAPI_V5_LOWER_LIMIT_MV 			4500
#define WAPI_V5_UPPER_LIMIT_MV 			5500
#define WAPI_V12_LOWER_LIMIT_MV 		0
#define WAPI_V12_UPPER_LIMIT_MV 		0
#define WAPI_OVER_VOLTAGE_PROT_LIMIT_MV 5000
#define WAPI_SEND_RECV_BUF_SIZE 		2048
#define WAPI_SEND_RECV_BIG_BUF_SIZE     (65536 + 32)
#define WAPI_CMD_RESP_BUF_SIZE 		    128 
#define WAPI_TX_HALT_ON_ERROR_MASK 		(1 << 18) 
#define WAPI_ACK_TIMEOUT_MASK 			(1 << 11)
#define WAPI_HITACHI_SPECIAL_ENABLE      1

#define wapi2sio(slot)                  (slot-1)
#define sio2wapi(slot)                  (slot+1)


/* Hitachi global variables */
pthread_mutex_t  mutex_slotio;

/*
 * Teradyne static function prototypes
 */
static Byte sanityCheck(Byte);
static Byte validateReceivedData(Byte *sendRecvBuf, Word cmdLength, unsigned int respLength, 
                                 unsigned int expLength, Word *pBuffer, unsigned int status1);
static void setDriveSlotIndex(int idx);
static Byte terSio2Connect(void);
static Byte terSio2Disconnect(void);
static Byte terSio2Reconnect(Byte bCellNo);
static void createNeptuneIso2(void);
static void destroyNeptuneIso2(void);
static void printBuffer(char *txt, Byte *buf, Word len);
static void terPrintf(char *description, ...);
static void printJournal(Byte bCellNo);
static void terError(const char *funcName, Byte bCellNo, Neptune_sio2 *ns2, TER_Status status);
static void terFlushReceiveBuffer(Byte bCellNo, Byte *sendRecvBuf, unsigned int *status1);
/*
 * Teradyne global variables
 */
static int 	siSlotIdx = -1;
static int 	siSioConnected = 0;  // 0:Ethernet NOT connected, Non-zero: Ethernet Connected
static int 	siTargetVoltage5VinMilliVolts = 0;
static int 	siTargetVoltage12VinMilliVolts = 0;
static int 	siUartPullupVoltageInMilliVolts = 0;

static Word siV5LowerLimitInMilliVolts = 0; 
static Word siV12LowerLimitInMilliVolts = 0; 
static Word siV5UpperLimitInMilliVolts = 0; 
static Word siV12UpperLimitInMilliVolts = 0;

static Word siSetTempLowerInHundredth = 0; 
static Word siSetTempUpperInHundredth = 0; 
static Word siSensorTempLowerInHundredth = 0; 
static Word siSensorTempUpperInHundredth = 0;

static int 	siUartProtocol = 1;
static class Neptune_sio2 *ns2 = NULL;
static int  siLogLevel = 1;
static char logFileName[128] = {'\0'};

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
Byte siInitialize(Byte bCellNo)
{
	Byte rc = Success; /* No error */
	TER_Status status;
	TER_SlotInfo pSlotInfo;
	int mask = 0;
	int attemptCnt = 2;
        DP  dp;
        siSetDebugLogLevel(bCellNo, 1);

	/* Instantiate an Neptune_Sio2 object */
	createNeptuneIso2();


	if( bCellNo < 1 || bCellNo > 140 ) {
		TLOG_ERROR("ERROR: siInitialize failed. Invalid slot number \n");
		return TerError;
	}
	
	setDriveSlotIndex(bCellNo);
		
	/* Establish connection with SIO */
	for(attemptCnt = 1; attemptCnt <= 2; ++attemptCnt) {
		rc = terSio2Connect();
		if (rc == 0) {
			break;
		}
	}
	
	if( rc != TER_Status_none ) {
		if (rc == TER_Status_socket_not_open) {
			TLOG_ERROR("ERROR: siInitialize failed [rc=SIO_Socket_Error]. SIO is not accepting Socket connection \n");
			return SIO_Socket_Error;
		}
		if (rc == TER_Status_dio_timeout ) {
			TLOG_ERROR("ERROR: siInitialize failed [rc=DIO_Non_Responsive]. Cannot talk to DIO \n");
			return DIO_Non_Responsive;
		}
		else {
			TLOG_ERROR("ERROR: siInitialize failed [rc=SIO_Not_Reachable]. Error %d \n", rc);
			return SIO_Not_Reachable;
		}
	}

    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_IsDioPresent ( wapi2sio(bCellNo), &dp ));
    if ( status != TER_Status_none ) {
		TLOG_ERROR("ERROR: siInitialize failed [rc=DIO_Non_Responsive]. TER_IsDioPresent failed with status = %d. \n", status);
		return DIO_Non_Responsive;
	}
		
    if ( DP_FLAGS_in_boot_loader & dp.dp_flags ) {

		sleep(20); /* wait 20 secs. DIO may be still booting */

    	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_IsDioPresent ( wapi2sio(bCellNo), &dp )); /* check again */
		if ( status != TER_Status_none ) {
			TLOG_ERROR("ERROR: siInitialize failed [rc=DIO_Non_Responsive]. TER_IsDioPresent failed with status = %d. \n", status);
			return DIO_Non_Responsive;
		}
		else {
			if ( DP_FLAGS_in_boot_loader & dp.dp_flags ) {
				TLOG_ERROR("ERROR: DIO is in Bootloader [0x%08x]. Trying DIO firmware update.. \n", dp.dp_flags);
				WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_UpgradeMicrocode (wapi2sio(bCellNo)));
				if ( status != TER_Status_none ) {
					TLOG_ERROR("ERROR: siInitialize failed [rc=DIO_App_Error]. Updating DIO firmware failed [%d] \n", status);
					return DIO_App_Error;
				}
			}
		}
    }
    else if ( DP_FLAGS_main_app_xsum_not_valid & dp.dp_flags || DP_FLAGS_upgrade_recommended & dp.dp_flags ) {
    	if ( DP_FLAGS_upgrade_recommended & dp.dp_flags ) {
			TLOG_ERROR("ERROR: Upgrade Recommended [0x%08x]. Updating DIO firmware.. \n", dp.dp_flags);
		}
		else {
			TLOG_ERROR("ERROR: DIO App checksum failed [0x%08x]. Updating DIO firmware.. \n", dp.dp_flags);
		}

		WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_UpgradeMicrocode (wapi2sio(bCellNo)));
		if ( status != TER_Status_none ) {
			TLOG_ERROR("ERROR: siInitialize failed [rc=DIO_Firmware_Update_Error]. Updating DIO firmware failed [%d] \n", status);
			return DIO_Firmware_Update_Error;
		}
    }
    else if ( DP_FLAGS_timeout & dp.dp_flags ) {
		TLOG_ERROR("ERROR: siInitialize failed [rc=DIO_Non_Responsive]. Cannot talk to DIO [0x%08x] \n", dp.dp_flags);
		return DIO_Non_Responsive;
    }

#if WAPI_HITACHI_SPECIAL_ENABLE
	TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);
#endif

    /* clear slot errors if any. This also does DIO soft reset (slot reset) */
	if( 0 == terGetWapiTestMode(bCellNo) ) {  /* Certain wapitest command line will not work if slot reset is called and hence this code */
    	rc = clearCellEnvironmentError(bCellNo); 
		if( rc ) {
			TLOG_ERROR("ERROR: clearCellEnvironmentError \n");
		}
	}
	else {
		terSetWapiTestMode(bCellNo, 0); /* Do not call clearCellEnvironmentError, clear wapi test mode */
	}
	
	
	/* clear screeching halt */
    rc = siClearHaltOnError(bCellNo);
	if( rc ) {
		TLOG_ERROR("ERROR: siClearHaltOnError \n");
	}

#if WAPI_HITACHI_SPECIAL_ENABLE
	rc = siSetInterCharacterDelay(bCellNo, 546);	    
	if( rc ) {
		TLOG_ERROR("ERROR: siSetInterCharacterDelay failed \n");
	}
	rc = siSetAckTimeout ( bCellNo, 3);
	if( rc ) {
		TLOG_ERROR("ERROR: siSetAckTimeout failed \n");
	}
#endif


    /* Init global vars, log, check slot errors, print DIO/SIO/FPGA Rev, Library rev */
	siTargetVoltage5VinMilliVolts = 0;
	siTargetVoltage12VinMilliVolts = 0;
	siUartPullupVoltageInMilliVolts = 0;
	siUartProtocol = 1;

	/* Print serial no/version */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotInfo(wapi2sio(bCellNo),  &pSlotInfo));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    } 
	else {
    	TLOG_TRACE("DioFirmwareVersion: <%s>\n", pSlotInfo.DioFirmwareVersion);
    	TLOG_TRACE("DioPcbSerialNumber: <%s>\n", pSlotInfo.DioPcbSerialNumber);
    	TLOG_TRACE("SioFpgaVersion: <%s>\n", pSlotInfo.SioFpgaVersion);
    	TLOG_TRACE("SioPcbSerialNumber: <%s>\n", pSlotInfo.SioPcbSerialNumber);
    	TLOG_TRACE("SioPcbPartNum: <%s>\n", pSlotInfo.SioPcbPartNum);
    	TLOG_TRACE("SlotPcbPartNum: <%s>\n", pSlotInfo.SioPcbPartNum);
		TLOG_TRACE("Neptune SIO2 Lib (neptune_sio2.a): %d\n", NEPTUNE_SIO2_LIB_VER);
		TLOG_TRACE("SIO2 Super App (SIO2_SuperApplication): %d\n", SIO2_SUPER_APP_VER);
		TLOG_TRACE("WAPI Lib (libsi.a): %s\n", terGetWapiLibVersion());

	}	

    /* Set jornaling mask */
	mask = siLogLevel ? (Neptune_sio2::NS2_Journal_states) : (Neptune_sio2::NS2_Journal_none);
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetJournalMask ( wapi2sio(bCellNo), (Neptune_sio2::NS2_JOURNAL_MASK) mask ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

	
	/* Clear SIO Journal for this slot */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_ClearCurrentJournal ( wapi2sio(bCellNo) ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
		
	return rc;
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
Byte siFinalize(Byte bCellNo)
{
	TER_Status status;

    TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

/* SIO journal is called only upon error */
#if 0	
	/* Log SIO journal if enabled */
	if( siLogLevel) {
		printJournal(bCellNo);
	}
#endif

    /* clear journaling mask */
    status = ns2->TER_SetJournalMask ( wapi2sio(bCellNo), (Neptune_sio2::NS2_Journal_none) );
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
    }

   	terSio2Disconnect();
   	destroyNeptuneIso2();

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
   	return (Byte)0;
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
Byte isCellEnvironmentError(Byte bCellNo)
{
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotStatus(wapi2sio(bCellNo), &slotStatus));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		if (slotStatus.slot_errors) {
			return TerError;
		}
	}

    TLOG_EXIT("Exit : %s, rc=%s \n", __FUNCTION__, rc ? "Yes":"No");
	return rc;
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
Byte clearCellEnvironmentError(Byte bCellNo)
{
	TER_Status status;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_ResetSlot(wapi2sio(bCellNo), TER_ResetType_Reset_DioSoftReset));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

	TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte getCurrentTemperature(Byte bCellNo, Word *wTempInHundredth)
{
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d *) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotStatus(wapi2sio(bCellNo), &slotStatus));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		/* TER returns in units of 0.1 degree C. While Hitachi
		 * expects unit of 0.01 degree C. Multiply by 10. */
		*wTempInHundredth = (Word)(slotStatus.temp_drive_x10 * 10);
	}

    TLOG_EXIT("Exit : %s, rc=%d, %d \n", __FUNCTION__, rc, *wTempInHundredth);
    if(*wTempInHundredth > 7000){
        tcExit(EXIT_FAILURE);
    }
    if(*wTempInHundredth < 1000){
        tcExit(EXIT_FAILURE);
    }
	return rc;
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
Byte setTargetTemperature(Byte bCellNo, Word wTempInHundredth)
{
    targetTemperature tt;
    int tempTarget_x10, coolRampRate_x10_perMinute, heatRampRate_x10_perMinute;
    TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wTempInHundredth);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	/* Enable temperature control */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetTempControlEnable(wapi2sio(bCellNo), 1));
    if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
    	return TerError;
    }
	
	/* Read current ramp rates from SIO */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	
	heatRampRate_x10_perMinute = (Word)(slotSettings.temp_ramp_rate_heat_x10);
	coolRampRate_x10_perMinute = (Word)(slotSettings.temp_ramp_rate_cool_x10);
    tempTarget_x10 = (wTempInHundredth +5) / 10;

    tt.temp_target_x10    = tempTarget_x10;
    tt.cool_ramp_rate_x10 = coolRampRate_x10_perMinute;
    tt.heat_ramp_rate_x10 = heatRampRate_x10_perMinute;
    tt.cooler_enable      = 1;
    tt.heater_enable      = 1;

    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetTargetTemperature(wapi2sio(bCellNo), &tt));
    if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

	/* Read back and validate */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	
	if( (tt.temp_target_x10 != tempTarget_x10) ||
	    (tt.cool_ramp_rate_x10 != coolRampRate_x10_perMinute) ||
		(tt.heat_ramp_rate_x10 != heatRampRate_x10_perMinute) )
	{
	    TLOG_ERROR("ERROR: %s failed validation \n", __FUNCTION__ );
		return TerError;
	}
	
    TLOG_EXIT("Exit : %s, rc=%d\n", __FUNCTION__, rc);
	return rc;
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
Byte getTargetTemperature(Byte bCellNo, Word *wTempInHundredth)
{
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		/* TER returns in units of 0.1 degree C. While Hitachi
		 * expects unit of 0.01 degree C. Multiply by 10. */
		*wTempInHundredth = (Word)(slotSettings.temperature_target_x10 * 10);
	}

    TLOG_EXIT("Exit : %s, rc=%d, %d \n", __FUNCTION__, rc, *wTempInHundredth);
	return rc;
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
Byte getReferenceTemperature(Byte bCellNo, Word *wTempInHundredth)
{
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
Byte getHeaterOutput(Byte bCellNo, Word *wHeaterOutputInPercent)
{
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotStatus(wapi2sio(bCellNo), &slotStatus));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		*wHeaterOutputInPercent = (Word)(slotStatus.heat_demand_percent);
	}

    TLOG_EXIT("Exit : %s, rc=%d, %d \n", __FUNCTION__, rc, *wHeaterOutputInPercent);
	return rc;
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
Byte setHeaterOutput(Byte bCellNo, Word wHeaterOutputInPercent)
{
	Byte rc = Success; /* No error */
	
	/* This function is not supported */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wHeaterOutputInPercent);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte getShutterPosition(Byte bCellNo, Word *wShutterPositionInPercent)
{
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = Success; /* No error */

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	/* return cooling demand instead!! */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotStatus(wapi2sio(bCellNo), &slotStatus));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		*wShutterPositionInPercent = (Word)(slotStatus.cool_demand_percent);
	}

	return rc;
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
Byte setShutterPosition(Byte bCellNo, Word wShutterPositionInPercent)
{
	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wShutterPositionInPercent);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return (Byte)0;
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
Byte getTemperatureErrorStatus(Byte bCellNo, Word *wErrorStatus)
{
	TER_Status status;
	TER_SlotStatus slotStatus;
	int slotErrors;
	int tempErrors;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	/* Initialize to an error condition */
	*wErrorStatus = 1;

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotStatus(wapi2sio(bCellNo), &slotStatus));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		slotErrors = slotStatus.slot_errors;
		tempErrors = 0;
		tempErrors |= (slotErrors & TER_Notifier_FanFault);
		tempErrors |= (slotErrors & TER_Notifier_SledOverTemp);
		tempErrors |= (slotErrors & TER_Notifier_SlotCircuitOverTemp);
		tempErrors |= (slotErrors & TER_Notifier_SledTempSensorFault);
		tempErrors |= (slotErrors & TER_Notifier_SledHeaterFault);
		tempErrors |= (slotErrors & TER_Notifier_TempEnvelopeFault);
		tempErrors |= (slotErrors & TER_Notifier_TempRampNotCompleted);
		*wErrorStatus = (Word)tempErrors;

		/* There's a problem here since Hitachi allocates just 16 bits to
		 * the error status and TER may have error info in the upper 16 bits.
		 * Need to return some true value if the upper 16 bits are true. */
		if (*wErrorStatus == 0 && tempErrors != 0) {
			*wErrorStatus = 0xffff;
		}
	}

    TLOG_EXIT("Exit : %s, rc=%d, 0x%0x \n", __FUNCTION__, rc, tempErrors);
	return rc;
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
Byte getPositiveTemperatureRampRate(Byte bCellNo, Word *wTempInHundredthPerMinutes)
{
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		/* TER returns in units of 0.1 deg/min. While Hitachi
		 * expects unit of 0.01 deg/min. Multiply by 10. */
		*wTempInHundredthPerMinutes = (Word)(slotSettings.temp_ramp_rate_heat_x10 * 10);
	}


    TLOG_EXIT("Exit : %s, rc=%d, %d \n", __FUNCTION__, rc, *wTempInHundredthPerMinutes);
	return rc;
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
Byte setPositiveTemperatureRampRate(Byte bCellNo, Word wTempInHundredthPerMinutes)
{
    targetTemperature tt;
    int tempTarget_x10, coolRampRate_x10_perMinute, heatRampRate_x10_perMinute;
    TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wTempInHundredthPerMinutes);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	/* Read current ramp & target temp from SIO */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	
	heatRampRate_x10_perMinute = (wTempInHundredthPerMinutes+5) / 10;
	coolRampRate_x10_perMinute = (Word)(slotSettings.temp_ramp_rate_cool_x10);
    tempTarget_x10             = (Word)(slotSettings.temperature_target_x10);

    tt.temp_target_x10    = tempTarget_x10;
    tt.cool_ramp_rate_x10 = coolRampRate_x10_perMinute;
    tt.heat_ramp_rate_x10 = heatRampRate_x10_perMinute;
    tt.cooler_enable      = 1;
    tt.heater_enable      = 1;

    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetTargetTemperature(wapi2sio(bCellNo), &tt));
    if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

	/* Read back and validate */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	
	if( (tt.temp_target_x10 != tempTarget_x10) ||
	    (tt.cool_ramp_rate_x10 != coolRampRate_x10_perMinute) ||
		(tt.heat_ramp_rate_x10 != heatRampRate_x10_perMinute) )
	{
	    TLOG_ERROR("ERROR: %s failed validation \n", __FUNCTION__ );
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte getNegativeTemperatureRampRate(Byte bCellNo, Word *wTempInHundredthPerMinutes)
{
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		/* TER returns in units of 0.1 deg/min. While Hitachi
		 * expects unit of 0.01 deg/min. Multiply by 10. */
		*wTempInHundredthPerMinutes = (Word)(slotSettings.temp_ramp_rate_cool_x10 * 10);
	}

    TLOG_EXIT("Exit : %s, rc=%d, %d \n", __FUNCTION__, rc, *wTempInHundredthPerMinutes);
	return rc;
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
Byte setNegativeTemperatureRampRate(Byte bCellNo, Word wTempInHundredthPerMinutes)
{
    targetTemperature tt;
    int tempTarget_x10, coolRampRate_x10_perMinute, heatRampRate_x10_perMinute;
    TER_Status status;
    TER_SlotSettings slotSettings;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wTempInHundredthPerMinutes);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    /* Convert from Hitachi units (0.01 deg/min) to TER (0.1 deg/min). */
    coolRampRate_x10_perMinute = (wTempInHundredthPerMinutes+5) / 10;

	/* Read current ramp & target temp from SIO */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	
    coolRampRate_x10_perMinute = wTempInHundredthPerMinutes / 10;
	heatRampRate_x10_perMinute = (Word)(slotSettings.temp_ramp_rate_heat_x10);
    tempTarget_x10             = (Word)(slotSettings.temperature_target_x10);

    tt.temp_target_x10    = tempTarget_x10;
    tt.cool_ramp_rate_x10 = coolRampRate_x10_perMinute;
    tt.heat_ramp_rate_x10 = heatRampRate_x10_perMinute;
    tt.cooler_enable      = 1;
    tt.heater_enable      = 1;

    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetTargetTemperature(wapi2sio(bCellNo), &tt));
    if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

	/* Read back and validate */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	
	if( (tt.temp_target_x10 != tempTarget_x10) ||
	    (tt.cool_ramp_rate_x10 != coolRampRate_x10_perMinute) ||
		(tt.heat_ramp_rate_x10 != heatRampRate_x10_perMinute) )
	{
	    TLOG_ERROR("ERROR: %s failed validation \n", __FUNCTION__ );
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte setTemperatureSensorCarlibrationData(Byte bCellNo, Word wSoftwareTempInHundredth, Word wRealTempInHundredth)
{
	if (sanityCheck(bCellNo)) {
		return TerError; 
	}

	return (Byte)0;
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
 Byte setTemperatureLimit(Byte bCellNo, Word wSetTempLowerInHundredth, Word wSetTempUpperInHundredth, Word wSensorTempLowerInHundredth, Word wSensorTempUpperInHundredth)
 {

    TLOG_ENTRY("Entry: %s(%d,%d,%d,%d,%d) \n", __FUNCTION__, bCellNo, wSetTempLowerInHundredth, wSetTempUpperInHundredth, wSensorTempLowerInHundredth, wSensorTempUpperInHundredth);

	if (sanityCheck(bCellNo)) {
		return TerError; 
	}

	siSetTempLowerInHundredth = wSetTempLowerInHundredth; 
	siSetTempUpperInHundredth = wSetTempUpperInHundredth; 
	siSensorTempLowerInHundredth = wSensorTempLowerInHundredth; 
	siSensorTempUpperInHundredth = wSensorTempUpperInHundredth;

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return (Byte)0;
 
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
 Byte getTemperatureLimit(Byte bCellNo, Word *wSetTempLowerInHundredth, Word *wSetTempUpperInHundredth, Word *wSensorTempLowerInHundredth, Word *wSensorTempUpperInHundredth)
 {

    TLOG_ENTRY("Entry: %s(%d,*,*,*,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError; 
	}

	*wSetTempLowerInHundredth = siSetTempLowerInHundredth; 
	*wSetTempUpperInHundredth = siSetTempUpperInHundredth; 
	*wSensorTempLowerInHundredth = siSensorTempLowerInHundredth; 
	*wSensorTempUpperInHundredth = siSensorTempUpperInHundredth;

    TLOG_EXIT("Exit : %s, rc=0, %d, %d, %d, %d \n", __FUNCTION__, *wSetTempLowerInHundredth, *wSetTempUpperInHundredth, *wSensorTempLowerInHundredth, *wSensorTempUpperInHundredth);
	return (Byte)0;

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
Byte setSafeHandlingTemperature(Byte bCellNo, Word wTempInHundredth)
{
	Byte rc = Success; /* No error */
#if 0
    TER_Status status;
#endif

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wTempInHundredth);

	if (sanityCheck(bCellNo)) {
		return TerError; 
	}

#if 0    
    /* Go to safe handling temperature */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GoToSafeTemp(wapi2sio(bCellNo), ((wTempInHundredth+5)/10)));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
#endif 
    rc = setTargetTemperature(bCellNo, wTempInHundredth);

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
}


/**
 * <pre>
 * Description:
 *   Gets the safe handling temperature values of the cell or the chamber
 * Arguments:
 *   rCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wTempInHundredth - OUTPUT - in hundredth of a degree Celsius
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte getSafeHandlingTemperature(Byte bCellNo, Word * wTempInHundredth)
{
	Byte rc = Success; /* No error */

	return rc;
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
Byte setTargetVoltage(Byte bCellNo, Word wV5InMilliVolts, Word wV12InMilliVolts)
{
	TER_Status status;
	int mask;
	int OnOff;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d,%d) \n", __FUNCTION__, bCellNo, wV5InMilliVolts, wV12InMilliVolts);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}
	
	/* Need to enable setting the power to the 5V and 12V supplies */
	/* Mask: FID Power = 0x1, 12V Power = 0x2, 5V Power = 0x4, 3V Power = 0x8 */
	
	mask = 0x4;  /* 5V mask */
	if( wV5InMilliVolts ) {
		OnOff = mask;
	
		WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetPowerEnabled(wapi2sio(bCellNo), mask, OnOff));
		if (status) {
			terError(__FUNCTION__, bCellNo, ns2, status);
			return TerError;
		}

		WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetPowerVoltage(wapi2sio(bCellNo), mask, (int)wV5InMilliVolts));
		if (status) {
			terError(__FUNCTION__, bCellNo, ns2, status);
			return TerError;
		}
	}
	else {
		OnOff = 0;
	
		WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetPowerEnabled(wapi2sio(bCellNo), mask, OnOff));
		if (status) {
			terError(__FUNCTION__, bCellNo, ns2, status);
			return TerError;
		}
	}
	
	/* 12V */
	mask = 0x2;  /* 12V mask */
	if( wV12InMilliVolts ) {
		OnOff = mask;
	
		WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetPowerEnabled(wapi2sio(bCellNo), mask, OnOff));
		if (status) {
			terError(__FUNCTION__, bCellNo, ns2, status);
			return TerError;
		}

		WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetPowerVoltage(wapi2sio(bCellNo), mask, (int)wV12InMilliVolts));
		if (status) {
			terError(__FUNCTION__, bCellNo, ns2, status);
			return TerError;
		}
	}
	else {
		OnOff = 0;
	
		WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetPowerEnabled(wapi2sio(bCellNo), mask, OnOff));
		if (status) {
			terError(__FUNCTION__, bCellNo, ns2, status);
			return TerError;
		}
	}
	
	/* Need to cache the target voltage, since we don't have a Get function */
	siTargetVoltage5VinMilliVolts = wV5InMilliVolts;

	/* Need to cache the target voltage, since we don't have a Get function */
	siTargetVoltage12VinMilliVolts = wV12InMilliVolts;

	/* Enable UART and set up UART pull up voltage */
	setUartPullupVoltage(bCellNo, UART_PULLUP_VOLTAGE_DEFAULT);


    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;

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
Byte getTargetVoltage(Byte bCellNo,Word *wV5InMilliVolts, Word *wV12InMilliVolts)
{
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	*wV5InMilliVolts = (Word)siTargetVoltage5VinMilliVolts;
	*wV12InMilliVolts = (Word)siTargetVoltage12VinMilliVolts;

    TLOG_EXIT("Exit : %s, rc=%d, %d, %d \n", __FUNCTION__, rc, *wV5InMilliVolts, *wV12InMilliVolts);
	return rc;
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
Byte getCurrentVoltage(Byte bCellNo,Word *wV5InMilliVolts, Word *wV12InMilliVolts)
{
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = Success; /* No error */
        Word wV5Current;
        Word wV12Current;
        Byte crc = Success;

	TLOG_ENTRY("Entry: %s(%d,*,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotStatus(wapi2sio(bCellNo), &slotStatus));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}


	else {
		*wV5InMilliVolts = (Word)(slotStatus.voltage_actual_5v_mv);
		*wV12InMilliVolts = (Word)(slotStatus.voltage_actual_12v_mv);
	}

        crc = getActualCurrent(bCellNo, &wV5Current, &wV12Current);
	TCPRINTF("rc = %d,5V current = %d, 12V current = %d\n",crc,wV5Current,wV12Current);

    TLOG_EXIT("Exit : %s, rc=%d, %d, %d \n", __FUNCTION__, rc, *wV5InMilliVolts, *wV12InMilliVolts);
	return rc;
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
Byte getActualCurrent(Byte bCellNo,Word *wV5InMilliAmps, Word *wV12InMilliAmps)
{
	TER_Status status;
	TER_SlotStatus slotStatus;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotStatus(wapi2sio(bCellNo), &slotStatus));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}


	else {
		*wV5InMilliAmps = (Word)(slotStatus.current_actual_5a_mv);
		*wV12InMilliAmps = (Word)(slotStatus.current_actual_12a_mv);
	}

    TLOG_EXIT("Exit : %s, rc=%d, %d, %d \n", __FUNCTION__, rc, *wV5InMilliAmps, *wV12InMilliAmps);
	return rc;
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
Byte setVoltageCalibration(Byte bCellNo, Word wV5LowInMilliVolts, Word wV5HighInMilliVolts,
		                                 Word wV12LowInMilliVolts, Word wV12HighInMilliVolts)
{
    TLOG_ENTRY("Entry: %s(%d,%d,%d,%d,%d) \n", __FUNCTION__, bCellNo, wV5LowInMilliVolts, wV5HighInMilliVolts, wV12LowInMilliVolts, wV12HighInMilliVolts);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return (Byte)0;
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
Byte setVoltageLimit(Byte bCellNo, Word wV5LowerLimitInMilliVolts, Word wV12LowerLimitInMilliVolts, Word wV5UpperLimitInMilliVolts, Word wV12UpperLimitInMilliVolts)
{
	Byte rc = Success; /* No error */

    TLOG_ENTRY("Entry: %s(%d,%d,%d,%d,%d) \n", __FUNCTION__, bCellNo, wV5LowerLimitInMilliVolts, wV12LowerLimitInMilliVolts, wV5UpperLimitInMilliVolts, wV12UpperLimitInMilliVolts);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}
	
	if( (wV5LowerLimitInMilliVolts < WAPI_V5_LOWER_LIMIT_MV) || 
	    (wV5UpperLimitInMilliVolts > WAPI_V5_UPPER_LIMIT_MV) ||
	    (wV12LowerLimitInMilliVolts != WAPI_V12_LOWER_LIMIT_MV) || 
	    (wV12UpperLimitInMilliVolts != WAPI_V12_UPPER_LIMIT_MV) )  
	{
	    TLOG_EXIT("Exit : %s, rc=1 \n", __FUNCTION__);
		return TerError;
	}

	siV5LowerLimitInMilliVolts = wV5LowerLimitInMilliVolts;
	siV5UpperLimitInMilliVolts = wV5UpperLimitInMilliVolts;
	siV12LowerLimitInMilliVolts = wV12LowerLimitInMilliVolts;
	siV12UpperLimitInMilliVolts = wV12UpperLimitInMilliVolts;

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte getVoltageLimit(Byte bCellNo, Word *wV5LowerLimitInMilliVolts, Word *wV12LowerLimitInMilliVolts, Word *wV5UpperLimitInMilliVolts, Word *wV12UpperLimitInMilliVolts)
{
    TLOG_ENTRY("Entry: %s(%d,*,*,*,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	*wV5LowerLimitInMilliVolts = siV5LowerLimitInMilliVolts;
	*wV5UpperLimitInMilliVolts = siV5UpperLimitInMilliVolts;
	*wV12LowerLimitInMilliVolts = siV12LowerLimitInMilliVolts;
	*wV12UpperLimitInMilliVolts = siV12UpperLimitInMilliVolts;

    TLOG_EXIT("Exit : %s, rc=0, %d, %d, %d \n", __FUNCTION__, *wV5LowerLimitInMilliVolts, *wV12LowerLimitInMilliVolts, *wV5UpperLimitInMilliVolts, *wV12UpperLimitInMilliVolts);
	return (Byte)0;
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
Byte setCurrentLimit(Byte bCellNo, Word wV5LimitInMilliAmpere, Word wV12LimitInMilliAmpere)
{
    TLOG_ENTRY("Entry: %s(%d, %d, %d) \n", __FUNCTION__, bCellNo, wV5LimitInMilliAmpere, wV12LimitInMilliAmpere);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	if (wV5LimitInMilliAmpere != WAPI_V5_LIMIT_IN_MILLI_AMPS) {
	    TLOG_EXIT("Exit : %s, rc=1 \n", __FUNCTION__);
		return TerError;
	}

	if (wV12LimitInMilliAmpere != WAPI_V12_LIMIT_IN_MILLI_AMPS) {
	    TLOG_EXIT("Exit : %s, rc=1 \n", __FUNCTION__);
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return (Byte)0;
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
Byte getCurrentLimit(Byte bCellNo, Word *wV5LimitInMilliAmpere, Word *wV12LimitInMilliAmpere)
{
    TLOG_ENTRY("Entry: %s(%d,*,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}
	
	*wV5LimitInMilliAmpere = WAPI_V5_LIMIT_IN_MILLI_AMPS;
	*wV12LimitInMilliAmpere = WAPI_V12_LIMIT_IN_MILLI_AMPS;

    TLOG_EXIT("Exit : %s, rc=0, %d, %d \n", __FUNCTION__, *wV5LimitInMilliAmpere, *wV12LimitInMilliAmpere);
	return (Byte)0;
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
Byte setVoltageRiseTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec)
{
	Byte rc = Success; /* No error */
	TER_Status status;
	int t5VRiseTimeInMsec, t5VFallTimeInMsec;

	TLOG_ENTRY("Entry: %s(%d,%d,%d) \n", __FUNCTION__, bCellNo, wV5TimeInMsec, wV12TimeInMsec);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    /* Get 5V rise/fall time */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_Get5VRiseAndFallTimes ( wapi2sio(bCellNo), &t5VRiseTimeInMsec, &t5VFallTimeInMsec ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }
	
    /* Set 5V rise/fall time */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_Set5VRiseAndFallTimes ( wapi2sio(bCellNo), (int)wV5TimeInMsec, t5VFallTimeInMsec ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

	/* 12V Not supported */
	
    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte getVoltageRiseTime(Byte bCellNo, Word *wV5TimeInMsec, Word *wV12TimeInMsec)
{
	Byte rc = Success; /* No error */
	TER_Status status;
	int t5VRiseTimeInMsec, t5VFallTimeInMsec;

	TLOG_ENTRY("Entry: %s(%d,*,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}
	
    /* 5V rise/fall time */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_Get5VRiseAndFallTimes ( wapi2sio(bCellNo), &t5VRiseTimeInMsec, &t5VFallTimeInMsec ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

    *wV5TimeInMsec = (Word)t5VRiseTimeInMsec;
	*wV12TimeInMsec = 0; /* Not supported yet */
	
    TLOG_EXIT("Exit : %s, rc=%d, %d, %d \n", __FUNCTION__, rc, *wV5TimeInMsec, *wV12TimeInMsec);
	return rc;
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
Byte setVoltageFallTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec)
{
	Byte rc = Success; /* No error */
	TER_Status status;
	int t5VRiseTimeInMsec, t5VFallTimeInMsec;

	TLOG_ENTRY("Entry: %s(%d,%d,%d) \n", __FUNCTION__, bCellNo, wV5TimeInMsec, wV12TimeInMsec);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    /* Get 5V rise/fall time */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_Get5VRiseAndFallTimes ( wapi2sio(bCellNo), &t5VRiseTimeInMsec, &t5VFallTimeInMsec ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }
	
    /* Set 5V rise/fall time */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_Set5VRiseAndFallTimes ( wapi2sio(bCellNo), t5VRiseTimeInMsec, (int)wV5TimeInMsec));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

	/* 12V Not supported */
	
    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte getVoltageFallTime(Byte bCellNo, Word *wV5TimeInMsec, Word *wV12TimeInMsec)
{
	Byte rc = Success; /* No error */
	TER_Status status;
	int t5VRiseTimeInMsec, t5VFallTimeInMsec;

	TLOG_ENTRY("Entry: %s(%d,*,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}
	
    /* 5V rise/fall time */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_Get5VRiseAndFallTimes ( wapi2sio(bCellNo), &t5VRiseTimeInMsec, &t5VFallTimeInMsec ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

    *wV5TimeInMsec = (Word)t5VFallTimeInMsec;
	*wV12TimeInMsec = 0; /* Not supported yet */
	
    TLOG_EXIT("Exit : %s, rc=%d, %d, %d \n", __FUNCTION__, rc, *wV5TimeInMsec, *wV12TimeInMsec);
	return rc;
}


/**
 * <pre>
 * Description:
 *   Sets the voltatge rise interval between 5V and 12V
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wTimeFromV5ToV12InMsec - INPUT - interval time from 5V on to 12V on.
 *  								  range is 0 -to- 255 (0-255msec)
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
Byte setVoltageInterval(Byte bCellNo, Word wTimeFromV5ToV12InMsec)
{
	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	return (Byte)0;
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
Byte getVoltageInterval(Byte bCellNo, Word *wTimeFromV5ToV12InMsec)
{
	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	return (Byte)0;
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
Byte getVoltageErrorStatus(Byte bCellNo, Word *wErrorStatus)
{
	TER_Status status;
	TER_SlotStatus slotStatus;
	int slotErrors;
	int voltageError;
	Byte rc = Success; /* No error */

	/* Set initial value to error */
	*wErrorStatus = 1;

	TLOG_ENTRY("Entry: %s(%d,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotStatus(wapi2sio(bCellNo), &slotStatus));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		slotErrors = slotStatus.slot_errors;
		voltageError = 0;
		voltageError |= (slotErrors & TER_Notifier_SlotCircuitFault);
		voltageError |= (slotErrors & TER_Notifier_DriveOverCurrent);

		*wErrorStatus = (Word)voltageError;
	}

    TLOG_EXIT("Exit : %s, rc=%d, 0x%x \n", __FUNCTION__, rc, *wErrorStatus);
	return rc;
}

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
Byte isDrivePlugged(Byte bCellNo)
{
	TER_Status status;
	TER_SlotSettings slotSettings;
    Byte drivePlugged = 0; /* Drive not plugged */
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return drivePlugged;
	}
	else {
		if (slotSettings.slot_status_bits & TER_IsDrivePresent) {
			drivePlugged = 1;
		}
	}

    TLOG_EXIT("Exit : %s, rc=%s \n", __FUNCTION__, drivePlugged ? "Yes":"No");
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
Byte getCellInventory(Byte bCellNo, CELL_CARD_INVENTORY_BLOCK **stCellInventory)
{
	Byte rc = Success; /* No error */
	TER_Status status;
	TER_SlotInfo pSlotInfo;

	static CELL_CARD_INVENTORY_BLOCK cellCardInventoryBlock;
	CELL_CARD_INVENTORY_BLOCK *p = &cellCardInventoryBlock;

	TLOG_ENTRY("Entry: %s(%d,**) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotInfo(wapi2sio(bCellNo),  &pSlotInfo));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		memset((void*)p, 0, sizeof(cellCardInventoryBlock));

		/* The Hitachi Test code (that we have) cares only about the following fields */
		memcpy((void*)cellCardInventoryBlock.bPartsNumber,
				(const void*)pSlotInfo.SioPcbSerialNumber,
				sizeof(cellCardInventoryBlock.bPartsNumber));
		memcpy((void*)cellCardInventoryBlock.bCardRev,
				(const void*)pSlotInfo.DioFirmwareVersion,
				sizeof(cellCardInventoryBlock.bCardRev));
		memcpy((void*)cellCardInventoryBlock.bCardType,
				(const void*)"DIO",
				sizeof(cellCardInventoryBlock.bCardType));
		memcpy((void*)cellCardInventoryBlock.bHddType,
				(const void*)"1",
				sizeof(cellCardInventoryBlock.bHddType));
		memcpy((void*)cellCardInventoryBlock.bSerialNumber,
				(const void*)pSlotInfo.DioPcbSerialNumber,
				sizeof(cellCardInventoryBlock.bSerialNumber));
		memcpy((void*)cellCardInventoryBlock.bConnectorLifeCount,
				(const void*)"10000",
				sizeof(cellCardInventoryBlock.bConnectorLifeCount));
	}

	//stCellInventory = &p;
	*stCellInventory = p;
    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte setTemperatureEnvelope( Byte bCellNo, Word wEnvelopeTempInTenth)
{

	Byte rc = Success; /* No error */
	TER_Status status;

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wEnvelopeTempInTenth);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}
	
	/* Range check */

    /* Issue the call */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetTemperatureEnvelope ( wapi2sio(bCellNo), (unsigned int)wEnvelopeTempInTenth ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte getTemperatureEnvelope( Byte bCellNo, Word *wEnvelopeTempInTenth)
{
 	TER_Status status;
	unsigned int tempInTenth;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*)", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    /* Issue the call */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetTemperatureEnvelope (wapi2sio(bCellNo),&tempInTenth));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }
	
	*wEnvelopeTempInTenth = (Word)tempInTenth;

    TLOG_EXIT("Exit : %s, rc=%d, %d \n", __FUNCTION__, rc, *wEnvelopeTempInTenth);
	return rc;
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
Byte setUartPullupVoltageDefault(Byte bCellNo)
{
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	rc = setUartPullupVoltage(bCellNo, UART_PULLUP_VOLTAGE_DEFAULT);
	
    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
}

Byte setUartPullupVoltage(Byte bCellNo, Word wUartPullupVoltageInMilliVolts)
{
	TER_Status status;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wUartPullupVoltageInMilliVolts);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    if( wUartPullupVoltageInMilliVolts != 0 ) {    /* 0 = Enable staggred spin-up */
		if ( wUartPullupVoltageInMilliVolts < 1200 ||    /* Valid range 1.2V-3.3V */
	     	wUartPullupVoltageInMilliVolts > 3300 ) {
			return TerError;
		}
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetSerialLevels(wapi2sio(bCellNo), (int)wUartPullupVoltageInMilliVolts));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetSerialSelect(wapi2sio(bCellNo), 0));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}


	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetSerialEnable(wapi2sio(bCellNo), 1));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

	/* Need to cache the UART pullup Voltage for the Get function */
	siUartPullupVoltageInMilliVolts = (int)wUartPullupVoltageInMilliVolts;

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte getUartPullupVoltage(Byte bCellNo, Word *wUartPullupVoltageInMilliVolts)
{
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	/* Return cached value */
	*wUartPullupVoltageInMilliVolts = siUartPullupVoltageInMilliVolts;

    TLOG_EXIT("Exit : %s, rc=%d, %d \n", __FUNCTION__, rc, *wUartPullupVoltageInMilliVolts);
	return rc;
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
Byte siSetUartBaudrate(Byte bCellNo, Dword dwBaudrate)
{
	TER_Status status;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, dwBaudrate);

	if (sanityCheck(bCellNo)) { 
		return TerError;
	}

    if(!dwBaudrate) dwBaudrate = 1843200; 

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetSerialParameters(wapi2sio(bCellNo), dwBaudrate, 1));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte siGetUartBaudrate(Byte bCellNo, Dword *dwBaudrate)
{
	TER_Status status;
	TER_SlotSettings slotSettings;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {  
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	else {
		/* Baud rate in bps */
		*dwBaudrate = (Dword)slotSettings.baud_rate;
	}

    TLOG_EXIT("Exit : %s, rc=%d, %d \n", __FUNCTION__, rc, *dwBaudrate);
	return rc;
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
Byte siBulkReadFromDrive(Byte bCellNo, Word *wSize, Byte *bData, Word wTimeoutInMillSec)
{
                           
    Byte sendRecvBuf[WAPI_SEND_RECV_BUF_SIZE]; 
	TER_Status status;
	Byte rc = Success; /* No error */
	Word i;
    unsigned int respLength;
	unsigned int status1;

	TLOG_ENTRY("Entry: %s(%d,*,*,%d) \n", __FUNCTION__, bCellNo, wTimeoutInMillSec);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    respLength = WAPI_SEND_RECV_BUF_SIZE; 
	
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioReceiveBuffer(wapi2sio(bCellNo), &respLength, wTimeoutInMillSec, sendRecvBuf, &status1)); 
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
    else
    {
        for (i = 0; i < respLength; ++i) {
            bData[i] = sendRecvBuf[i];
 		}	
		
		*wSize = respLength;
    }
    
    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte siBulkWriteToDrive(Byte bCellNo, Word wSize, Byte *bData, Word wTimeoutInMillSec)
{
                           
    Byte sendRecvBuf[WAPI_SEND_RECV_BUF_SIZE]; 
	TER_Status status;
	Byte rc = Success; /* No error */
	Word cmdLength, i;

	TLOG_ENTRY("Entry: %s(%d,%d,*,%d) \n", __FUNCTION__, bCellNo, wSize, wTimeoutInMillSec);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	/* Disable UART2 state machine */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_TwoByteOneByte(wapi2sio(bCellNo), 0));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

    for(i=0; i < wSize; ++i) {    /* Data */
        sendRecvBuf [i] = bData[i];
    }
    cmdLength = wSize; 
	
 	TLOG_BUFFER("Req Pkt: ", sendRecvBuf, wSize); 

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioSendBuffer(wapi2sio(bCellNo), cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

	/* Enable UART2 state machine */
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_TwoByteOneByte(wapi2sio(bCellNo), 1));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		TLOG_ERROR("ERROR: UART2 is still disabled \n");
		return TerError;
	}
	
    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;

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
Byte siWriteDriveMemory(Byte bCellNo, Dword dwAddress, Word wSize, Byte *bData, Word wTimeoutInMillSec)
{
                           
    Byte sendRecvBuf[WAPI_SEND_RECV_BUF_SIZE]; 
	TER_Status status;
	Byte rc = Success; /* No error */
	Word cmdLength, i, j, checkSum, cmdLen, tmpWSize;
    unsigned int respLength;
	unsigned int status1;

	TLOG_ENTRY("Entry: %s(%d,0x%x,%d,*,%d) \n", __FUNCTION__, bCellNo, dwAddress, wSize, wTimeoutInMillSec);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	/* Flush sio receive buffer */
	terFlushReceiveBuffer(bCellNo, sendRecvBuf, &status1);

    tmpWSize = wSize;
    if (wSize & 1) {   
      	wSize++;	
    }			

    i = 0;
    cmdLen = (wSize/2)+5;
    sendRecvBuf [ i++ ] = 0xaa;   /* Sync Pkt */
    sendRecvBuf [ i++ ] = 0x55;

    sendRecvBuf [ i++ ] = (cmdLen)&0xFF;   /* Length */
    sendRecvBuf [ i++ ] = (cmdLen)&0xFF;

    sendRecvBuf [ i++ ] = 0x00;   /* Opcode */
    sendRecvBuf [ i++ ] = 0x02;

    sendRecvBuf [ i++ ] = (dwAddress>>24)&0xFF;   /* Address */
    sendRecvBuf [ i++ ] = (dwAddress>>16)&0xFF; 
    sendRecvBuf [ i++ ] = (dwAddress>>8)&0xFF; 
    sendRecvBuf [ i++ ] = (dwAddress)&0xFF; 

    sendRecvBuf [ i++ ] = (tmpWSize>>8)&0xFF;        /* Data Length */
    sendRecvBuf [ i++ ] = (tmpWSize)&0xFF; 


    for(j=0; j < wSize; ++j) {                /* Data */
		if(j%2==0){                                // for big endian
        	sendRecvBuf [ i++ ] = bData[j+1];   
		}
		else {                                     
        	sendRecvBuf [ i++ ] = bData[j-1];
		}
    }

    /* Calculate checksum */
	checkSum = computeChecksum(sendRecvBuf, i);
    
    sendRecvBuf [ i++ ] = (checkSum>>8)&0xFF;     /* Checksum */
    sendRecvBuf [ i++ ] = (checkSum)&0xFF;  

    TLOG_BUFFER("Req Pkt: ", sendRecvBuf, i); 
               
    cmdLength = i;
	
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioSendBuffer(wapi2sio(bCellNo), cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError; /* Some error exists */
	}
    
    respLength = WAPI_SEND_RECV_BUF_SIZE; 
    wTimeoutInMillSec = 500;
	
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioReceiveBuffer(wapi2sio(bCellNo), &respLength, wTimeoutInMillSec, sendRecvBuf, &status1));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
    else
	{
        TLOG_BUFFER("Res Pkt: ", sendRecvBuf, respLength);
    }

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte siReadDriveMemory(Byte bCellNo, Dword dwAddress, Word wSize, Byte *bData, Word wTimeoutInMillSec)
{
    Byte 			*recvBuf = NULL;
	Byte 			sendBuf[WAPI_CMD_RESP_BUF_SIZE];
	TER_Status 		status;
	Byte 			rc = Success; /* No error */
	Word 			cmdLength, i, j, checkSum;
    unsigned int	respLength, expLength;
	unsigned int 	status1;

	TLOG_ENTRY("Entry: %s(%d,0x%x,%d,*,%d) \n", __FUNCTION__, bCellNo, dwAddress, wSize, wTimeoutInMillSec);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}


 	if(dwAddress & 1){
		TLOG_ERROR("!!ERROR set odd address!!\n");
		return TerError;
	}
	
   /* [DEBUG] simple but dirty patch for odd wSize case It is problem if bData buffer size is equal to wSize
       but bData is usually allocated 64KBytes so it would be OK for short term solution.
	*/
	
    if (wSize & 1) {
      	wSize++;
    }

	/* Flush sio receive buffer */
	terFlushReceiveBuffer(bCellNo, sendBuf, &status1);

    i = 0;
    sendBuf [ i++ ] = 0xaa;   /* Sync Pkt */
    sendBuf [ i++ ] = 0x55;

    sendBuf [ i++ ] = 0x05;   /* Length */
    sendBuf [ i++ ] = 0x05;

    sendBuf [ i++ ] = 0x00;   /* Opcode */
    sendBuf [ i++ ] = 0x01;

    sendBuf [ i++ ] = (dwAddress>>24)&0xFF;   /* Address */
    sendBuf [ i++ ] = (dwAddress>>16)&0xFF; 
    sendBuf [ i++ ] = (dwAddress>>8)&0xFF; 
    sendBuf [ i++ ] = (dwAddress)&0xFF; 

    sendBuf [ i++ ] = (wSize>>8)&0xFF;        /* Data Length */
    sendBuf [ i++ ] = (wSize)&0xFF; 
 
    /* Calculate checksum */
	checkSum = computeChecksum(sendBuf, i);
    
    sendBuf [ i++ ] = (checkSum>>8)&0xFF;     /* Checksum */
    sendBuf [ i++ ] = (checkSum)&0xFF;
       
    TLOG_BUFFER("Req Pkt: ", sendBuf, i);  
               
    cmdLength = i;
	expLength = (cmdLength/2)+8+wSize;
    respLength = expLength;
	wTimeoutInMillSec = (wTimeoutInMillSec > 10000) ? 10000:wTimeoutInMillSec;
	if(wSize < WAPI_SEND_RECV_BUF_SIZE) {
		WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioSendBuffer(wapi2sio(bCellNo), cmdLength, sendBuf));
		if (status != TER_Status_none) {
			terError(__FUNCTION__, bCellNo, ns2, status);
			return TerError; /* Some error exists */
		}

		recvBuf = (Byte *)malloc(WAPI_SEND_RECV_BUF_SIZE);
		WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioReceiveBuffer(wapi2sio(bCellNo), &respLength, wTimeoutInMillSec, recvBuf, &status1)); 
	}
	else {
		recvBuf = (Byte *)malloc(WAPI_SEND_RECV_BIG_BUF_SIZE);
 	    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioHugeBufferWriteRead (wapi2sio(bCellNo), cmdLength, sendBuf,
					  (int *)&respLength, recvBuf, wTimeoutInMillSec/1000, &status1, -1));
	}
	
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		free(recvBuf);
		return TerError;
	}
    else
    {
        /* Validate the received packet */
        rc = validateReceivedData(recvBuf, cmdLength, respLength, expLength, &i, status1);
        if( rc == Success) {
            for (j = 0, i += 6; j < wSize; i++,j++) {
                bData[j] = recvBuf[i];
 				if(dwAddress&1){
					bData[j] = recvBuf[i];
				}
				else {
					if(j%2==0) {                                // for big endian
	               		bData[j] = recvBuf[i+1];       // y.katayama
					}
					else {                                     // 2010.12.03 
                		bData[j] = recvBuf[i-1];       //
					}
				}
			}	
        }
    }
    
	if(recvBuf) {
	    free(recvBuf);
	}

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte siEchoDrive(Byte bCellNo, Word wSize, Byte *bData, Word wTimeoutInMillSec)
{
    Byte sendRecvBuf[WAPI_SEND_RECV_BUF_SIZE]; 
	TER_Status status;
	Byte rc = Success; /* No error */
	Word cmdLength, i, j, checkSum;
    unsigned int respLength, expLength;
	unsigned int status1;

	TLOG_ENTRY("Entry: %s(%d,%d,*,%d) \n", __FUNCTION__, bCellNo, wSize, wTimeoutInMillSec);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}


	/* Flush sio receive buffer */
	terFlushReceiveBuffer(bCellNo, sendRecvBuf, &status1);
	
    i = 0;
    sendRecvBuf [ i++ ] = 0xaa;   /* Sync Pkt */
    sendRecvBuf [ i++ ] = 0x55;

    sendRecvBuf [ i++ ] = 0x04;   /* Length */
    sendRecvBuf [ i++ ] = 0x04;

    sendRecvBuf [ i++ ] = 0x00;   /* Opcode */
    sendRecvBuf [ i++ ] = 0x00;

    sendRecvBuf [ i++ ] = 0x00;   /* Command Flags */
    sendRecvBuf [ i++ ] = 0x00;
    sendRecvBuf [ i++ ] = (wSize>>8)&0xFF;        /* Length of enquiry data*/
    sendRecvBuf [ i++ ] = (wSize)&0xFF; 
 
 	/* Calculate checksum */
	checkSum = computeChecksum(sendRecvBuf, i);
    
    sendRecvBuf [ i++ ] = (checkSum>>8)&0xFF;     /* Checksum */
    sendRecvBuf [ i++ ] = (checkSum)&0xFF;  
       
    TLOG_BUFFER("Req Pkt: ", sendRecvBuf, i);  

    cmdLength = i;
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioSendBuffer(wapi2sio(bCellNo), cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError; /* Some error exists */
	}    

    expLength = (cmdLength/2)+8+wSize;
    respLength = expLength;
	wTimeoutInMillSec = (wTimeoutInMillSec > 10000) ? 10000:wTimeoutInMillSec;

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioReceiveBuffer(wapi2sio(bCellNo), &respLength, wTimeoutInMillSec, sendRecvBuf, &status1)); 
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
    else
    {
        rc = validateReceivedData(sendRecvBuf, cmdLength, respLength, expLength, &i, status1);
        if( rc == Success )
        {
	        for (j = 0, i += 6; j < wSize; i++,j++) {
				if(j%2==0){                           
                	bData[j] = sendRecvBuf[i+1];
				}
				else {                                 
        	    	bData[j] = sendRecvBuf[i-1];
				}
            }
       }
    }                             


    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte siGetDriveUartVersion(Byte bCellNo, Word *wVersion)
{
    Byte sendRecvBuf[WAPI_CMD_RESP_BUF_SIZE];
	TER_Status status;
	Byte rc = Success; /* No error */
	Word i, cmdLength;
    unsigned int respLength,expLength;
	unsigned int status1;

	TLOG_ENTRY("Entry: %s(%d,*) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	/* Flush sio receive buffer */
	terFlushReceiveBuffer(bCellNo, sendRecvBuf, &status1);

    i = 0;
    sendRecvBuf [ i++ ] = 0xaa;   /* Sync Pkt */
    sendRecvBuf [ i++ ] = 0x55;

    sendRecvBuf [ i++ ] = 0x02;   /* Length */
    sendRecvBuf [ i++ ] = 0x02;

    sendRecvBuf [ i++ ] = 0x00;   /* Opcode */
    sendRecvBuf [ i++ ] = 0x04;

    sendRecvBuf [ i++ ] = 0xFF;   /* Checksum */
    sendRecvBuf [ i++ ] = 0xFC; 
    
    TLOG_BUFFER("Req Pkt: ", sendRecvBuf, i);
   
    cmdLength = i;
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioSendBuffer(wapi2sio(bCellNo), cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

    expLength = ((cmdLength/2)+10);
    respLength = expLength; 
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioReceiveBuffer(wapi2sio(bCellNo), &respLength, 500, sendRecvBuf, &status1));  
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
    else
    {
        /* Validate the received packet */
        rc = validateReceivedData(sendRecvBuf, cmdLength, respLength, expLength, &i, status1);
        
        if(rc == Success)
            *wVersion = (sendRecvBuf[i+7] << 8)|sendRecvBuf[i+6];  
              
    }

    TLOG_EXIT("Exit : %s, rc=%d, %d \n", __FUNCTION__, rc, *wVersion);
	return rc;
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
Byte siSetDriveDelayTime(Byte bCellNo, Word wDelayTimeInMicroSec, Word wTimeoutInMillSec)
{
    Byte sendRecvBuf[WAPI_CMD_RESP_BUF_SIZE];
	TER_Status status;
	Byte rc = Success; /* No error */
	Word i, cmdLength, checkSum;
    unsigned int respLength;
	unsigned int status1;

	TLOG_ENTRY("Entry: %s(%d,%d,%d) \n", __FUNCTION__, bCellNo, wDelayTimeInMicroSec, wTimeoutInMillSec);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	/* Flush sio receive buffer */
	terFlushReceiveBuffer(bCellNo, sendRecvBuf, &status1);

    i = 0;
    sendRecvBuf [ i++ ] = 0xaa;   /* Sync Pkt */
    sendRecvBuf [ i++ ] = 0x55;

    sendRecvBuf [ i++ ] = 0x03;   /* Length */
    sendRecvBuf [ i++ ] = 0x03;

    sendRecvBuf [ i++ ] = 0x00;   /* Opcode */
    sendRecvBuf [ i++ ] = 0x0b;

    sendRecvBuf [ i++ ] = (wDelayTimeInMicroSec>>8)&0xFF;        /* Delay value */
    sendRecvBuf [ i++ ] = (wDelayTimeInMicroSec)&0xFF; 
 
    /* Calculate checksum */
	checkSum = computeChecksum(sendRecvBuf, i);

    sendRecvBuf [ i++ ] = (checkSum>>8)&0xFF;     /* Checksum */
    sendRecvBuf [ i++ ] = (checkSum)&0xFF;  
    
    TLOG_BUFFER("Req Pkt: ", sendRecvBuf, i);
   
    cmdLength = i;
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioSendBuffer(wapi2sio(bCellNo), cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

    respLength = WAPI_CMD_RESP_BUF_SIZE; 
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioReceiveBuffer(wapi2sio(bCellNo), &respLength, wTimeoutInMillSec, sendRecvBuf, &status1));  
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
    else
    {
        /* Validate the received packet */
        rc = validateReceivedData(sendRecvBuf, cmdLength, respLength,((cmdLength/2)+8), &i, status1);
    }

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte siResetDrive(Byte bCellNo, Word wType, Word wResetFactor, Word wTimeoutInMillSec)
{
                          
    Byte sendRecvBuf[WAPI_CMD_RESP_BUF_SIZE]; 
	TER_Status status;
	Byte rc = Success; /* No error */
	Word cmdLength, i, j, checkSum;

	TLOG_ENTRY("Entry: %s(%d,%d,%d,%d) \n", __FUNCTION__, bCellNo, wType, wResetFactor, wTimeoutInMillSec);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    i = 0;
    sendRecvBuf [ i++ ] = 0xaa;   /* Sync Pkt */
    sendRecvBuf [ i++ ] = 0x55;

    sendRecvBuf [ i++ ] = 0x04;   /* Length */
    sendRecvBuf [ i++ ] = 0x04;

    sendRecvBuf [ i++ ] = 0x00;   /* Opcode */
    sendRecvBuf [ i++ ] = 0x20;

    sendRecvBuf [ i++ ] = (wType>>8)&0xFF;               /* Drive type */
    sendRecvBuf [ i++ ] = (wType)&0xFF;
    
    sendRecvBuf [ i++ ] = (wResetFactor>>8)&0xFF;        /* Command Flags */
    sendRecvBuf [ i++ ] = (wResetFactor)&0xFF;
     
 
    /* Calculate checksum */
	checkSum = computeChecksum(sendRecvBuf, i);
    
    sendRecvBuf [ i++ ] = (checkSum>>8)&0xFF;     /* Checksum */
    sendRecvBuf [ i++ ] = (checkSum)&0xFF;  
  
          
    for(j=0; j<i; ++j)
        TLOG_TRACE("%02x ", sendRecvBuf[j]);
               
    cmdLength = i;
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SioSendBuffer(wapi2sio(bCellNo), cmdLength, sendRecvBuf));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}
	
    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte siSetUartProtocol(Byte bCellNo, Byte bProtocol)
{
    Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, bProtocol);
    
    /* Cache protocol type. This value will be used to determine endian swap */
    siUartProtocol = bProtocol; 
    
    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
    return rc;
}

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
Byte isOnTemp(Byte bCellNo)
{
	TER_Status status;
	TER_SlotSettings slotSettings;
    Byte isTempReached = 0; 
    Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetSlotSettings(wapi2sio(bCellNo), &slotSettings));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return isTempReached;
	}
	else {
		if (slotSettings.slot_status_bits & IsTempTargetReached) {
			isTempReached = 1;
		}
	}

    TLOG_EXIT("Exit : %s, rc=%s \n", __FUNCTION__, isTempReached ? "Yes":"No");
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
Byte siSetDriveFanRPM(Byte bCellNo, int DriveFanRPM)
{
    Byte rc = Success; /* No error */
	TER_Status status;

    TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, DriveFanRPM);

	/* Disable temp control */
 	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetTempControlEnable (wapi2sio(bCellNo), 0));
	if ( status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
	  	return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetFanDutyCycle(wapi2sio(bCellNo), DriveFanRPM));
	if ( status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
	  	return TerError;
	}
    
    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
    return rc;
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
Byte siSetElectronicsFanRPM(Byte bCellNo, int ElectronicsFanRPM)
{

	TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return (Byte)0;
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
Byte siSetFanRPMDefault(Byte bCellNo)
{
	TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return (Byte)0;
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
Byte siSetSupplyOverVoltageProtectionLevel(Byte bCellNo, Word wV5LimitInMilliVolts, Word wV12LimitInMilliVolts)
{
    TLOG_ENTRY("Entry: %s(%d,%d,%d) \n", __FUNCTION__, bCellNo, wV5LimitInMilliVolts, wV12LimitInMilliVolts);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	if (wV5LimitInMilliVolts > WAPI_OVER_VOLTAGE_PROT_LIMIT_MV) {
	    TLOG_EXIT("Exit : %s, rc=1 \n", __FUNCTION__);
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return (Byte)0;
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
Byte siSetLed(Byte bCellNo, Byte bMode)
{
#if 0 
	TER_Status status;

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

 	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetDioLeds (wapi2sio(bCellNo), (bMode?1:0)));
	if ( status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
	  	return TerError;
	}
#endif	
    TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);
    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return (Byte)0;
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
Byte isSlotThere(Byte bCellNo)
{
	TER_Status status;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}
	
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_IsSlotThere(wapi2sio(bCellNo)));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return status;
    }

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
}

/**
 * <pre>
 * Description:
 *   Used to enable trace level
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   bLogLevel - INPUT - 0=Trace disabled, 1=Trace enabled
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte siSetDebugLogLevel(Byte bCellNo, Byte bLogLevel)
{
	Byte rc = Success; /* No error */
	FILE *fp = NULL;
    time_t timep;
    struct tm tm;
	char  strDateTime[128];
    char filePath[128];

 
    /* Make a date & time string */
    time(&timep);
    localtime_r(&timep, &tm);
   
	sprintf(strDateTime, "%04d.%02d.%02d.%02d.%02d.%02d",
         tm.tm_year + 1900,
         tm.tm_mon + 1,
         tm.tm_mday,
         tm.tm_hour,
         tm.tm_min,
         tm.tm_sec
        );

	sprintf(logFileName, "slot%d_%s.log", bCellNo, strDateTime);
	sprintf(filePath, "/var/tmp/%s", logFileName);
	
	if(bLogLevel) {
    	fp = fopen(filePath,"r+"); /* if the log file does not exists, create one */
		if (fp == NULL) {
			fp = fopen(filePath,"w"); /* create a new log file */
		}
		fclose(fp);
	}
	siLogLevel = bLogLevel;

	return rc;
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
Byte siSetInterCharacterDelay ( Byte bCellNo, Dword wValInBitTimes )
{
	TER_Status status;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wValInBitTimes);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetInterCharacterDelay(wapi2sio(bCellNo), wValInBitTimes));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte siGetInterCharacterDelay ( Byte bCellNo, Dword *wValInBitTimes )
{
	TER_Status status;
	Byte rc = Success; /* No error */

    TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}
 
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetInterCharacterDelay ( wapi2sio(bCellNo), (int *)wValInBitTimes ));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=0, %d \n", __FUNCTION__, *wValInBitTimes);
	return rc;
}

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
Byte siGetLastErrors(Byte bCellNo)
{
	Byte rc = Success; /* No error */
	TER_Status status;
	char wbuf [ ERRLOG_LOG_SIZE ];

    TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

    memset(wbuf,0,ERRLOG_LOG_SIZE);
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetLastErrors ( wapi2sio(bCellNo), wbuf ));
    if ( TER_Status_none != status ) {
      	ns2->TER_ConvertStatusToString(-1, status, wbuf );
      	TLOG_ERROR("ERROR: TER_GetLastErrors: %s \n",wbuf);
    } 
	else {
      	TLOG_ERROR("%s\n", wbuf);
    }

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return rc;
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
Byte siClearHaltOnError(Byte bCellNo)
{
	TER_Status status;
	Byte rc = Success; /* No error */

    TLOG_ENTRY("Entry: %s(%d) \n", __FUNCTION__, bCellNo);
	
	/* Reset Tx FIFO */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_ResetSlot ( wapi2sio(bCellNo), TER_ResetType_Reset_SendBuffer ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

	/* Reset Rx FIFO */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_ResetSlot ( wapi2sio(bCellNo), TER_ResetType_Reset_ReceiveBuffer ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

	/* Clear halt condition */
    WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_ClearTxHalt ( wapi2sio(bCellNo) ));
    if ( TER_Status_none != status ) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
    }

    TLOG_EXIT("Exit : %s, rc=0 \n", __FUNCTION__);
	return rc;
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
Byte siSetAckTimeout ( Byte bCellNo, Dword wValue )
{
	TER_Status status;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wValue);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetACKTimeout(wapi2sio(bCellNo), wValue));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
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
Byte siGetAckTimeout ( Byte bCellNo, Dword *wValue )
{
	TER_Status status;
	Byte rc = Success; /* No error */

	if (sanityCheck(bCellNo)) {
		return TerError;
	}
 
	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_GetACKTimeout ( wapi2sio(bCellNo), (int *)wValue ));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

	return rc;
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
 *   wValue - OUTPUT - Inter packet delay in us [0..2550]. 10 us usec increments
 * Return:
 *   zero if success. otherwise, non-zero value.
 * </pre>
 ***************************************************************************************/
Byte siSetInterPacketDelay ( Byte bCellNo, Dword wValue )
{
	TER_Status status;
	Byte rc = Success; /* No error */

	TLOG_ENTRY("Entry: %s(%d,%d) \n", __FUNCTION__, bCellNo, wValue);

	if (sanityCheck(bCellNo)) {
		return TerError;
	}

	WAPI_CALL_RPC_WITh_RETRY(status = ns2->TER_SetInterPacketDelay(wapi2sio(bCellNo), wValue));
	if (status != TER_Status_none) {
		terError(__FUNCTION__, bCellNo, ns2, status);
		return TerError;
	}

    TLOG_EXIT("Exit : %s, rc=%d \n", __FUNCTION__, rc);
	return rc;
}


/** ===========================================================
 *        Auxiliary/helper functions.
 * ===========================================================*/
/*!
 * Just a sanity check to make sure it's OK to call the various
 * Hitachi and Teradyne functions.
 */
static Byte sanityCheck(Byte bCellNo)
{
	Byte rc = Success;

	if (ns2 == NULL || siSioConnected == 0 || siSlotIdx != bCellNo || bCellNo <= 0) {
		return TerError;
	}
	return rc;
}

#define  MAX_LINE_LENGTH   80    
/*!
 * Prints a buffer to error log for debugging purpose
 */
static void printBuffer(char *txt, Byte *buf, Word len)
{
    int 	i,j;
  	char 	lineBuf[MAX_LINE_LENGTH+2];

	if(len <=0) {
	    TLOG_TRACE("%s<none>\n", txt);
		return;
	}
		
  	if(len >= (MAX_LINE_LENGTH/2)) {
		len = MAX_LINE_LENGTH/2; 
	}
	
	for(i=0, j=0; i < len; i++, j+=2) {
		sprintf(&lineBuf[j],"%02x", buf[i]);
	}
	
	TLOG_TRACE("%s%s\n", txt,lineBuf);
	return;
}

/*!
 * Creates the Neptune_sio2 object.
 */
static void createNeptuneIso2(void)
{
	if (ns2 == NULL) {
		ns2 = new Neptune_sio2();
	}

	if (ns2 == NULL) {
		TLOG_ERROR("ERROR: Cannot create NeptuneIso2 Object \n");
	}
}

/*!
 * Destroys the Neptune_sio2 object.
 */
void destroyNeptuneIso2(void)
{
	if (ns2 != NULL) {
		delete ns2;
	}
	ns2 = NULL;
}


/*!
 * Connect to the SIO.
 * The SIO IP address and port number are hardcoded.
 * This function creates the socket and connects to the SIO server.
   \return Status code
 */
Byte terSio2Connect(void)
{
	TER_Status status;
	char *sioIpAddress;
	static char wbuf [256];

	sioIpAddress = getenv("SIOIPADDRESS");
	if(sioIpAddress != NULL) {
		TLOG_TRACE("SIO IP Address = %s\n", sioIpAddress);
		status = ns2->TER_Connect(sioIpAddress);
	}
	else
		status = ns2->TER_Connect((char *)"10.101.1.2");
	

	if (status != TER_Status_none) {
		TLOG_ERROR("ERROR: terSio2Connect: %s \n",wbuf);
	    return status;
	}

	siSioConnected = 1;

	return 0;
}

/*!
 * Disconnect from the SIO.
   \return Status code
 */
Byte terSio2Disconnect(void)
{
	TER_Status status;
	static char wbuf [256];

	status = ns2->TER_Disconnect();
	if (status != TER_Status_none) {
		TLOG_ERROR("ERROR: terSio2Disconnect: %s \n",wbuf);
	    return TerError;
	}
	siSioConnected = 0;

	return 0;
}


/*!
 * Disconnect and reconnect to SIO.
   \return Status code
 */
static Byte terSio2Reconnect(Byte bCellNo)
{
	Byte rc = Success; /* No error */
	static char wbuf [256];

	terSio2Disconnect();
   	destroyNeptuneIso2();
				
	createNeptuneIso2();
	setDriveSlotIndex(bCellNo);
	rc = terSio2Connect();
	if( rc != TER_Status_none ) {
		TLOG_ERROR("ERROR: terSio2Reconnect failed: %s \n",wbuf);
	    return(TerError);
	}
    return(rc);
}

/*!
 * Sets the Device Under Test slot index number
 * The value is from 1 to 140.
 */
void setDriveSlotIndex(int idx)
{
	siSlotIdx = idx;
}

static Byte validateReceivedData(Byte *sendRecvBuf, Word cmdLength, unsigned int respLength, 
       unsigned int expLength, Word *pBuffer, unsigned int status1)
{
    Byte rc1 = Success, rc = Success, temp, bCellNo = siSlotIdx; 
    Word i, j, k, checksum;

    TLOG_BUFFER("Res Pkt: ", sendRecvBuf, respLength);

	/* If transmit is halted in FPGA due to error, log error, clear halt condition and get out */
	if(WAPI_TX_HALT_ON_ERROR_MASK & status1) {
	    TLOG_ERROR("ERROR: Transmit halted in FPGA due to error \n");
    	for (i=0; i < respLength; i++)  {
        	if(sendRecvBuf[i] != DriveAckSuccess) 
        	{
        		TLOG_ERROR("ERROR: Did not receive good response (0x80) from the drive %02x \n", sendRecvBuf[i]);
        		rc1 = sendRecvBuf[i];
				break;
        	}
    	}	
		
		if( rc1 == Success ) {  /* Looks like FPGA timed out waiting for response from drive */
			if( WAPI_ACK_TIMEOUT_MASK & status1 )
			{
	    		TLOG_ERROR("ERROR: Drive ACK timeout \n");
				rc1 = DriveError;
			}
		}
		
		rc = siClearHaltOnError(bCellNo); /* Clear screeching halt condition */
		if(rc != Success) {
      		TLOG_ERROR("ERROR:siClearHaltOnError: \n");
		}
		return rc1;
	}
	else {
		TLOG_TRACE("----> TX Success. No screeching halt \n");
	}
	
         
    /* Check if we received the expected number of bytes */
    if( respLength != expLength )  {
        TLOG_ERROR("ERROR: Did not receive the expected number of bytes: Received %d, Expected %d \n", respLength, expLength);
        return TerError;
    }
	else {
		TLOG_TRACE("----> Received expected number of bytes \n");
	}
	
        
    for (i=0; i < (cmdLength/2); i++)  {
        if(sendRecvBuf[i] != DriveAckSuccess) 
        {
            TLOG_ERROR("ERROR: Did not receive the expected number of Acks %02x \n", sendRecvBuf[i]);
            return TerError;
        }
    }    
	TLOG_TRACE("----> Received expected number of ACKs \n");
	
	*pBuffer = i;  
	
	if( siUartProtocol == 1 ) /* ARM Processor */
	{
		/* Perform endian swap depending on protocol type */
		for(j=i, k=0; k<((respLength-i)/2); j+=2,k++) {
			temp = sendRecvBuf[j];
			sendRecvBuf[j] = sendRecvBuf[j+1];
			sendRecvBuf[j+1] = temp;
		}
	}

    /* Check sync packet - 0xAA55 */
    if( (sendRecvBuf[i] == 0x55) && (sendRecvBuf[i+1] == 0xAA) ) {
        //OK
    }else if( (sendRecvBuf[i] == 0xAA) && (sendRecvBuf[i+1] == 0x55) ) {
        //OK
    }else{
        TLOG_ERROR("Did not get sync packet - 0x%02x 0x%02x \n", sendRecvBuf[i], sendRecvBuf[i+1]);
        return TerError;
    }

	TLOG_TRACE("----> SYNC Packet is OK \n");
        
    /* Check return code - 0x0011 */
    if( sendRecvBuf[i+2]!= 0x00 || sendRecvBuf[i+3] != 0x00 ) {
        /* TLOG_ERROR("ERROR: Invalid return code \n"); */
        rc = 0;
    }
	
//	TLOG_TRACE("%d \n", respLength-i-6);
	checksum = computeChecksum(&sendRecvBuf[i], respLength-i-6);
//	TLOG_TRACE("checkSum %04x", checksum);
    return rc;   
}

/*!
 * teradyne printf
 */
static void terPrintf(char *description, ...)
{
  char lineBuf[1024];
  char filePath[100];
  int lineSizeNow = 0;
  int lineSizeMax = sizeof(lineBuf) - 6;  // 1 byte for addtional '\n'
  // 1 byte for '\0'
  // 4 byte for margin
  int ret = 0;
  int size = 0;
  static FILE *logFilePtr = NULL;
  // get time
  time_t timep;
  struct tm tm;

  time(&timep);
  localtime_r(&timep, &tm);

  sprintf(filePath, "/var/tmp/%s", logFileName);
  logFilePtr = fopen(filePath, "a+b");
 
  size = lineSizeNow < lineSizeMax ? lineSizeMax - lineSizeNow : 0;
  ret = snprintf(&lineBuf[lineSizeNow], size, "%04d/%02d/%02d,%02d:%02d:%02d,",
                 tm.tm_year + 1900,
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec
                );

  if (ret < size) {
    lineSizeNow += ret;
  } else {
    lineSizeNow += size - 1;
  }

  // description
  va_list ap;
  va_start(ap, description);
  size = lineSizeNow < lineSizeMax ? lineSizeMax - lineSizeNow : 0;
  ret = vsnprintf(&lineBuf[lineSizeNow], size, description, ap);
  if (ret < size) {
    lineSizeNow += ret;
  } else {
    lineSizeNow += size - 1;
  }
  va_end(ap);

#if 0
  // add a newline only if not one already
  if (lineBuf[lineSizeNow - 1] != '\n') {
    ret = snprintf(&lineBuf[lineSizeNow], 2, "\n");  // 2 = '\n' + '\0'
    lineSizeNow += 1;
    lineSizeMax += 1;
  }
#endif

  // Sweep out to Console
  if (logFilePtr != NULL) {
    fwrite(&lineBuf[0], 1, lineSizeNow, logFilePtr);
    fflush(logFilePtr);
  }

  // closing
  if (logFilePtr != NULL) {
    fclose(logFilePtr);
  }
}

/*!
 * Take journal snapshot
 */
static void printJournal(Byte bCellNo)
{
   	Neptune_sio2::NS2_JOURNAL_BUFFER *jb = NULL;
	TER_Status 	status;
	int         maxLines;

    /* Take journal snapshot */
    status = ns2->TER_SnapshotCurrentJournal ( wapi2sio(bCellNo), &jb );
    if ( TER_Status_none != status ) {
		terPrintf("ERROR: TER_SnapshotCurrentJournal \n");
    } 
	else {
	
		maxLines = (jb->cnt > 100) ? 100:jb->cnt;
     	for ( int i = jb->cnt-1, j = 0; j < maxLines; i--, j++) {
			Neptune_sio2::STAMPER *s = &jb->stamper[i];
			char strbuf [200];
			int strbuf_size = sizeof(strbuf);

			status = ns2->TER_StamperToString ( s, &strbuf_size, strbuf );
			if ( status ) {
				terPrintf("ERROR: TER_StamperToString \n");
	  			break;
			}
			else {
				terPrintf("%s",strbuf);
			}
      	}
	}
	/* Cleanup */
    if ( jb ) ns2->TER_FreeJournalBuffer(jb);

}


/*!
 * Print teradyne error (used by wapitest)
 */
void terPrintError(Byte bCellNo, TER_Status status)
{
	static char wbuf [256];

    ns2->TER_ConvertStatusToString(wapi2sio(bCellNo), status, wbuf);
	printf("%s \n", wbuf);
	return ;
}

/* return version */
char *terGetWapiLibVersion ( void )
{
  	static char verbuf [ 128 ];

  	sprintf(verbuf, "Build: %d,  Date: %s\n",
	  	BUILD_NO, BUILD_DATE);

  	return verbuf;
}


static Byte bWapiTestMode = 0;

/*!
 * set wapi test mode (used by wapitest)
 */
void terSetWapiTestMode(Byte bCellNo, Byte bMode)
{
	bWapiTestMode = bMode;
}

/*!
 * get wapi test mode (used by wapitest)
 */
Byte terGetWapiTestMode(Byte bCellNo)
{
	return(bWapiTestMode);
}


/*!
 * Print slot info (used by wapitest)
 */
void terPrintSlotInfo(Byte bCellNo)
{
	TER_Status status;
	TER_SlotInfo pSlotInfo;

	/* Print serial no/version etc.. */
    status = ns2->TER_GetSlotInfo(wapi2sio(bCellNo),  &pSlotInfo);
    if ( TER_Status_none != status ) {
		printf("ERROR: terPrintSlotInfo: Failed \n");
    } 
	else {
    	printf("DioFirmwareVersion: <%s>\n", pSlotInfo.DioFirmwareVersion);
    	printf("DioPcbSerialNumber: <%s>\n", pSlotInfo.DioPcbSerialNumber);
    	printf("SioFpgaVersion: <%s>\n", pSlotInfo.SioFpgaVersion);
    	printf("SioPcbSerialNumber: <%s>\n", pSlotInfo.SioPcbSerialNumber);
    	printf("SioPcbPartNum: <%s>\n", pSlotInfo.SioPcbPartNum);
    	printf("SlotPcbPartNum: <%s>\n", pSlotInfo.SioPcbPartNum);
		printf("Neptune SIO2 Lib (neptune_sio2.a): %d\n", NEPTUNE_SIO2_LIB_VER);
		printf("SIO2 Super App (SIO2_SuperApplication): %d\n", SIO2_SUPER_APP_VER);
		printf("WAPI Lib (libsi.a): %s\n", terGetWapiLibVersion());
	}			
	return;
}

/*!
 * log error
 */
static void terError(const char *funcName, Byte bCellNo, Neptune_sio2 *ns2, TER_Status status)
{
	static char wbuf [256];

    ns2->TER_ConvertStatusToString(wapi2sio(bCellNo), status, wbuf);
    TLOG_ERROR("ERROR: TER_GetLastErrors: %s \n", wbuf);
	siGetLastErrors (wapi2sio(bCellNo));
	
	return;
}


/*!
 * flush
 */
static void terFlushReceiveBuffer(Byte bCellNo, Byte *sendRecvBuf, unsigned int *status1)
{
	return; /* No need to do anything, just return. FIFOs are already flushed.  Remove the call later */
}

#ifdef __cplusplus
};
#endif
