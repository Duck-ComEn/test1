#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "../ts/ts.h"
#include "common.h"
//#include "commonEnums.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "../uart3io/ccb.h"
#include "../uart3io/libu3.h"
#include "../testcase/tcTypes.h"

/*
 * Name			:	siSCSICmdRequest
 * Description	:	Send SCSI command request to drive
 * Inputs		:	bCellNo				: HGST cell id
 *   				wLength 			: Length of the SCSI command
 *   				*bCmd				: The command
 *   				dwTimeoutInMillSec 	: Timeout in msec
 * Returns		:
 */
//Byte siSCSICmdRequest(Byte bCellNo, Word wLength, Byte *bCmd, Dword dwTimeoutInMillSec) {

//	Byte rAck = success;
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo=%d, wLength=%d, T/O=%d\n", __FUNCTION__,
//			bCellNo, wLength, dwTimeoutInMillSec);

	// Sanity checks
	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Data length is valid
//	if (wLength < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Invalid argument: Data length %d is invalid",
//				__FUNCTION__, wLength);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if buffer is NULL
//	if (bCmd == NULL) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - NULL argument has been passed\n", __FUNCTION__);
//		rAck = nullArgumentError;
//		goto FUNCTIONEND;
//	}

	// 4) Check if timeout value is invalid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

//	rAck = Common::Instance.Uart3LinkControl(bCellNo, /* Slot Id */
//	uart3OpcodeSCSICmd, /* Opcode = 0x40 */
//	0x00, /* Flag Byte = 0 */
//	&wLength, /* Data Lenght != 0 */
//	0x00, /* Data Sequence = 0 */
//	0x0000, /* wReserved=0 */
//	bCmd, /* Data Buffer != NULL */
//	dwTimeoutInMillSec /* Command timeout */
//	);

	// Check if the SCSI command request failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)
//			&& (rAck != uart3DriveBusyWithDataReceived)) {
//		Common::Instance.WriteToLog(
//				LogTypeError,
//				"ERROR: %s - Uart3LinkControl method failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code = 0x%02x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siQueryRequest
 * Description	:	Send Query Request command on UART 3 protocol
 * Inputs		:	bCellNo				: HGST cell id
 *   				bFlagByte 			:
 *   				dwTimeoutInMillSec 	: Timeout in msec
 * Returns		:
 */
//Byte siQueryRequest(Byte bCellNo, Byte bFlagByte, Dword dwTimeoutInMillSec) {

//	Byte rAck = success;
//	Word wLength = 0;

//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo=%d, bFlagByte=%d, T/O=%d\n",
//			__FUNCTION__, bCellNo, bFlagByte, dwTimeoutInMillSec);

	// Sanity Checks
	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if the timeout value is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout is less than 1 msec\n",
//				__FUNCTION__);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute the Query command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,				/* Slot Id */
//			uart3OpcodeQueryCmd, 	/* bReqRespOpcode = 0x41 */
//			bFlagByte, 				/* Flag Byte */
//			&wLength, 				/* Data Length = 0 */
//			0x0000, 				/* DataSequence = 0 */
//	                0x0000, /* wReserved=0 */
//			NULL, 					/* Data Buffer = NULL */
//			dwTimeoutInMillSec		/* Command Time Out */
///			);

	// Check whether the command failed for any internal wrapper errors
//	if ((rAck > success) && (rAck < maxTerError)) {
		// There is an internal wrapper error
//		Common::Instance.WriteToLog(LogTypeError,
//				"Uart3LinkControl method failed with error code 0x%x\n", rAck);
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code 0x%02x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siReadDataRequest
 * Description	:	This method reads the requested amount of the data from the drive
 * Inputs		:	bCellNo				: Slot ID
 * 					*wLength			: The requested data size (Input/Output)
 * 					wSeqNum				: Sequence number
 * 					*bData				: data from the drive (Output)
 * 					dwTimeoutInMillSec	: Timeout in Milli Seconds
 * Return		:
 */
//Byte siReadDataRequest(Byte bCellNo, Word *wLength, Word wSeqNum, Byte *bData,
//		Dword dwTimeoutInMillSec) {
//
//	Byte rAck = success;
//
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo=%d, wSeqNum=%d, wSize=%d, T/O=%d\n",
//			__FUNCTION__, bCellNo, wSeqNum, *wLength, dwTimeoutInMillSec);
//
	// Sanity checks

	// 1a) Check if the slot id is correct
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if NULL pointers are passed
//	if ((wLength == NULL) || (bData == NULL)) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Null argument has been passed\n", __FUNCTION__);
//		rAck = nullArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if sequence number is valid
//	if (wSeqNum < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Sequence number %d is invalid\n",
//				__FUNCTION__, wSeqNum);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// 4) Check if data length is valid
//	if (*wLength < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Data length %d is invalid\n",
//				__FUNCTION__, *wLength);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// 5) Check if timeout is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(
//				LogTypeError,
//				"ERROR %s - Invalid argument: Timeout is %d invalid has been passed\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute the Read command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,					/* Slot Id */
//			uart3OpcodeReadDataCmd,		/* Opcode = 0x42 */
//			0x00,						/* Flag Byte = 0 */
//			wLength,						/* Data Length != 0 */
//			wSeqNum,					/* Data Sequence != 0 */
//	                0x0000, /* wReserved=0 */
//			bData,						/* Data Buffer != NULL */
//			dwTimeoutInMillSec			/* Command time out */
//			);

	// Check if Read command failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError) &&
//			(rAck != uart3DriveBusyWithDataReceived)) {
//		Common::Instance.WriteToLog(
//				LogTypeError,
//				"ERROR %s - Uart3LinkControl method failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siWriteDataRequest
 * Description	:	This method writes the requested amount of data to the drive
 * Inputs		:	bCellNo				: Slot ID
 * 					wLength				: Data length (bytes)
 * 					wSeqNum				: Sequence number
 * 					*bData				: data to the drive
 * 					dwTimeoutInMillSec	: Timeout in Milli Seconds
 * Return		:
 */
//Byte siWriteDataRequest(Byte bCellNo, Word wLength, Word wSeqNum, Byte *bData,
//		Dword dwTimeoutInMillSec) {
//	Byte rAck = success;
//
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo=%d, wSeqNum=%d, wSize=%d, T/O=%d\n",
//			__FUNCTION__, bCellNo, wSeqNum, wLength, dwTimeoutInMillSec);

	// Sanity checks

	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if NULL pointers are passed
//	if (bData == NULL) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Null argument has been passed\n", __FUNCTION__);
//		rAck = nullArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if data length is valid
//	if (wLength < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Data length %d is invalid\n",
//				__FUNCTION__, wLength);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// 4) Check if sequence number is valid
//	if (wSeqNum < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Sequence number %d is invalid\n",
//				__FUNCTION__, wSeqNum);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// 5) Check if timeout is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute the Write command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,					/* Slot Id */
//			uart3OpcodeWriteDataCmd,	/* Opcode = 0x43 */
//			0x00,						/* Flag Byte = 0 */
//			&wLength,					/* Data Length != 0 */
//			wSeqNum,					/* Data Sequence != 0 */
//	                0x0000, /* wReserved=0 */
//			bData,						/* Data Buffer != NULL */
//			dwTimeoutInMillSec			/* Command time out */
//			);

	// Check if Write command failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError) &&
//			(rAck != uart3DriveBusyWithDataReceived)) {
//		Common::Instance.WriteToLog(
//				LogTypeError,
//				"ERROR %s - Uart3LinkControl method failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siAbortSCSICmdRequest
 * Description	:	This method aborts the current scsi command
 * Inputs		:	bCellNo				: Slot ID
 * 					dwTimeoutInMillSec	: Timeout in Milli Seconds
 * Return		:
 */
//Byte siAbortSCSICmdRequest(Byte bCellNo, Dword dwTimeoutInMillSec) {
//
//	Byte rAck = success;
//	Word wLength = 0;
//	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s, bCellNo=%d, T/O=%d\n",
//			__FUNCTION__, bCellNo, dwTimeoutInMillSec);

	// Sanity checks

	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if timeout value is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute AbortSCSI command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo, 					/* Slot Id */
//			uart3OpcodeAbortSCSICmd, 	/* Opcode = 0x44 */
//			0x00, 						/* Flag Byte = 0 */
//			&wLength, 					/* Data Length = 0 */
//			0x00, 						/* Data Sequence = 0 */
///	                0x0000, /* wReserved=0 */
//			NULL, 						/* Data Buffer = NULL */
//			dwTimeoutInMillSec			/* Command timeout */
//			);

	// Check if AbortSCSI command failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)) {
//		Common::Instance.WriteToLog(
//				LogTypeError,
//				"ERROR %s - Uart3LinkControl method failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code=0x%02x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siGetUARTParametersRequest
 * Description	:	This method gets the UART parameters from the drive
 * Inputs		:	bCellNo				: Slot ID
 * 					*paramData			: UART parameters data from the drive (Output)
 * 					dwTimeoutInMillSec	: Timeout in Milli Seconds
 * Return		:
 */
//Byte siGetUARTParametersRequest(Byte bCellNo, GETSETU3_PARAM *paramData, Dword dwTimeoutInMillSec) {

//	Byte rAck = success;
//	Word wSize = sizeof(GETSETU3_PARAM);
//	Byte *bData = new Byte[wSize];

//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo=%d, T/0=%d\n", __FUNCTION__,
//			bCellNo, dwTimeoutInMillSec);

	// Sanity checks

	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}
//
//	// 2) Check if NULL pointers are passed
//	if (paramData == NULL) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
//		rAck = nullArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if timeout value is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute the GetUARTParameters command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,							/* Slot Id */
//			uart3OpcodeGetUARTParametersCmd, 	/* Opcode = 0x45 */
//			0x00, 								/* Flag Byte = 0 */
//			&wSize, 							/* Data Length != 0 */
//			0x00,								/* Data Sequence = 0 */
//	                0x0000, /* wReserved=0 */
//			bData,								/* Data Buffer != NULL */
//			dwTimeoutInMillSec					/* Command Timeout */
//			);

	// Check if GetUARTParameters comamnd failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Uart3LinkControl failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// Copy the data into the output buffer
//	memcpy(paramData, bData, wSize);

//FUNCTIONEND:
//	delete[] bData;
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siSetUARTParametersRequest
 * Description	:	This method sets the UART parameters from the drive
 * Inputs		:	bCellNo				: Slot ID
 * 					*paramData			: UART parameters data to the drive (Output)
 * 					wTimeoutInMillSec	: Timeout in Milli Seconds
 * Return		:
 */
//Byte siSetUARTParametersRequest(Byte bCellNo, GETSETU3_PARAM *paramData, Dword dwTimeoutInMillSec) {

//	Byte rAck = success;
//	Word wSize = sizeof(GETSETU3_PARAM);
//	Byte *bData = new Byte[wSize];

//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo=%d, T/O=%d\n",
//			__FUNCTION__, bCellNo, dwTimeoutInMillSec);

	// Sanity checks

	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if NULL pointers are passed
//	if (paramData == NULL) {
//		Common::Instance.WriteToLog(LogTypeError, "ERROR: %s - Null argument has been passed\n", __FUNCTION__);
//		rAck = nullArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if timeout value is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Copy the input data into a byte array and execute the SetUARTParameters command
//	memcpy(bData, paramData, wSize);
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,							/* Slot Id */
//			uart3OpcodeSetUARTParametersCmd,	/* Opcode = 0x46 */
//			0x00,								/* Flag Byte = 0 */
//			&wSize,								/* Data Size != 0 */
//			0x00,								/* Data Sequence = 0 */
//	                0x0000, /* wReserved=0 */
//			bData,								/* Data Buffer != NULL */
//			dwTimeoutInMillSec					/* Command Timeout */
//			);

	// Check if SetUARTParameters command failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Uart3LinkControl failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	delete[] bData;
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siSetUART2ModeRequest
 * Description	:	This method sets the drive to UART2 mode
 * Inputs		:	bCellNo			: Slot ID
 * 					dwTimeoutInMillSec	: Timeout in Milli Seconds
 * Return		:
 */
//Byte siSetUART2ModeRequest(Byte bCellNo, Dword dwTimeoutInMillSec) {
//
//	Byte rAck = success;
//	Word wDataLength = 0;

//	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s, bCellNo=%d, T/O=%d\n",
//			__FUNCTION__, bCellNo, dwTimeoutInMillSec);

	// Sanity checks

	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if timeout value is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute the SetUART2Mode command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,					/* Slot ID */
//			uart3OpcodeSetUART2ModeCmd, /* Opcode = 0x47 */
//			0x00,						/* Flag Byte = 0 */
//			&wDataLength,				/* Data Length = 0 */
//			0x00, 						/* Data Sequence = 0 */
//	                0x0000, /* wReserved=0 */
//			NULL,						/* Data Buffer = NULL */
//			dwTimeoutInMillSec			/* Command Time out */
//			);

	// Check if SetUART2Mode command failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)) {
//		Common::Instance.WriteToLog(
//				LogTypeError,
//				"ERROR: %s, Uart3LinkControl method failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// If the drive responded with Ready ack then enable UART2 mode in the FPGA
//	if (rAck == uart3AckReady) {
//		// The drive is ready and has changed to UART2 mode.
		// Change the FPGA to enable the UART2 mode
//		Byte rc = Common::Instance.TerEnableFpgaUart2Mode(bCellNo, 1);
//		if (rc != success) {
//			Common::Instance.WriteToLog(
//					LogTypeError,
//					"ERROR: %s - TerEnableFpgaUart2Mode failed with error code 0x%x\n",
//					__FUNCTION__, rc);
//			rAck = rc;
//		}
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code = 0x%02x\n",
//			__FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siMemoryReadRequest
 * Description	:	This method reads data from the specified memory location
 * Inputs		:	bCellNo - Slot ID
 * 					wLength - Requested data length (bytes)
 * 					dwAddress - The memory address
 * 					*bData - The data buffer (Output)
 *   				dwTimeoutInMillSec - Timeout in milli seconds
 * Return		:
 */
//Byte siMemoryReadRequest(Byte bCellNo, Word wLength, Dword dwAddress, Byte *bData, Dword dwTimeoutInMillSec) {
//
//	Byte rAck = success;
//
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo=%d, wLength=%d, dwAddress=0x%x, T/O=%d\n",
//			__FUNCTION__, bCellNo, wLength, dwAddress, dwTimeoutInMillSec);
//
	// Sanity checks

	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if NULL pointers are passed
//	if (bData == NULL) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Null argument has been passed\n", __FUNCTION__);
//		rAck = nullArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if data length is valid
//	if (wLength < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Data length %d is invalid\n",
//				__FUNCTION__, wLength);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// 4) Check if timeout value is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute the MemoryRead command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,					/* Slot Id */
//			uart3OpcodeMemoryReadCmd,	/* Opcode = 0x48 */
//			0x00,						/* Flag Byte = 0x00 */
//			&wLength,					/* Data Length != 0 */
//			0x00,						/* Data Sequence = 0 */
//	                0x0000, /* wReserved=0 */
//			bData,						/* Data Buffer != NULL */
//			dwTimeoutInMillSec			/* Command timeout */
//			);

	// Check if the MemoryRead command failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)) {
//		Common::Instance.WriteToLog(
//				LogTypeError,
//				"ERROR %s - Uart3LinkControl method failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code=0x%02x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siMemoryWriteRequest
 * Description	:	This method writes data to the specified memory location
 * Inputs		:	bCellNo - Slot ID
 * 					wLength - data length (bytes)
 * 					dwAddress - The memory address
 * 					*bData - The data buffer (Output)
 *   				dwTimeoutInMillSec - Timeout in milli seconds
 * Return		:
 */
//Byte siMemoryWriteRequest(Byte bCellNo, Word wLength, Dword dwAddress, Byte *bData, Dword dwTimeoutInMillSec) {
//
//	Byte rAck = success;
//
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo=%d, wLength=%d, dwAddress=0x%x, T/O=%d\n",
//			__FUNCTION__, bCellNo, wLength, dwAddress, dwTimeoutInMillSec);

	// Sanity checks

	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if NULL pointers are passed
//	if (bData == NULL) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Null argument has been passed\n", __FUNCTION__);
//		rAck = nullArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if data length is valid
//	if (wLength < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Data length %d is invalid\n",
//				__FUNCTION__, wLength);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// 4) Check if timeout value is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute the MemoryWrite command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,					/* Slot Id */
//			uart3OpcodeMemoryWriteCmd,	/* Opcode = 0x49 */
//			0x00,						/* Flag Byte = 0x00 */
//			&wLength,					/* Data Length != 0 */
//			0x00,						/* Data Sequence = 0 */
//	                0x0000, /* wReserved=0 */
//			bData,						/* Data Buffer != NULL */
//			dwTimeoutInMillSec			/* Command timeout */
//			);

	// Check if the MemoryWrite command failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)) {
//		Common::Instance.WriteToLog(
//				LogTypeError,
//				"ERROR %s - Uart3LinkControl method failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code=0x%02x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siEchoRequest
 * Description	:	This method sends Echo command to the drive
 * Inputs		:	bCellNo				: Slot ID
 * 					wLength				: The size of data requested from drive
 * 					*bData				: Echo data from the drive (Output)
 * 					dwTimeoutInMillSec	: Timeout in Milli Seconds
 * Return		:
 */
//Byte siEchoRequest(Byte bCellNo, Word wLength, Byte *bData, Dword dwTimeoutInMillSec) {
//Byte siEchoRequest(Byte bCellNo, Byte *bData, Dword dwTimeoutInMillSec) {
//
//	Byte rAck = success;
  //      Word wLength = 0x0038;
//
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo = %d, T/0 = %d\n", __FUNCTION__, bCellNo,
//			dwTimeoutInMillSec);

	// Sanity checks

	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if NULL pointers are passed
//	if (bData == NULL) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
//		rAck = nullArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if data length is valid
//	if (wLength < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Invalid argument: Data length %d is invalid\n",
//				__FUNCTION__, wLength);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// 4) Check if timeout value is invalid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute Echo command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,			/* Slot Id */
//			uart3OpcodeEchoCmd, /* Opcode = 0x4A */
//			0x00,				/* Flag Byte = 0 */
//			&wLength,				/* Data Length != 0 */
//			0x00,				/* Data Sequence = 0 */
//	                0x0000, /* wReserved=0 */
//			bData,				/* Data Buffer != NULL */
//			dwTimeoutInMillSec	/* Command Timeout */
//			);

	// Check if Echo command failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Uart3LinkControl failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siGetDriveStateRequest
 * Description	:	This method gets the current drive state during a scsi command
 * Inputs		:	bCellNo				: Slot ID
 * 					*driveStateData		: Drive state data from the drive (Output)
 * 					dwTimeoutInMillSec	: Timeout in Milli Seconds
 * Return		:
 */
//Byte siGetDriveStateRequest(Byte bCellNo, GET_DRIVE_STATE_DATA *driveStateData,
//		Dword dwTimeoutInMillSec) {
//
//	Byte rAck = success;
//	Word wSize = sizeof(GET_DRIVE_STATE_DATA);
//	Byte *bData = new Byte[wSize];
//
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Entry: %s, bCellNo=%d, T/O=%d\n",
//			__FUNCTION__, bCellNo, dwTimeoutInMillSec);

	// Sanity checks

	// 1a) Check if slot id is valid
	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if NULL pointers are passed
//	if (driveStateData == NULL) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Null argument has been passed\n", __FUNCTION__);
//		rAck = nullArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if timeout value is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// Execute the GetDriveState command
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,						/* Slot Id */
//			uart3OpcodeGetDriveStateCmd, 	/* Opcode = 0x4E */
//			0x00, 							/* Flag Byte = 0 */
//			&wSize, 						/* Data Length != 0 */
//			0x00, 							/* Data Offset */
//	                0x0000, /* wReserved=0 */
//			bData,							/* Data Buffer != NULL */
//			dwTimeoutInMillSec				/* Command Timeout */
//			);

	// Check if GetDriveState command failed for internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Uart3LinkControl failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

//	// Copy the byte array into the output buffer
//	memcpy(driveStateData, bData, wSize);
//	driveStateData->ShowstopValue = ntohl(driveStateData->ShowstopValue);
//	driveStateData->wBrokenReason = ntohs(driveStateData->wBrokenReason);
//	driveStateData->wDegradedReason = ntohs(driveStateData->wDegradedReason);
//	driveStateData->DriveStateVersion = ntohs(driveStateData->DriveStateVersion);
//	driveStateData->wFunctionalMode = ntohs(driveStateData->wFunctionalMode);
//	driveStateData->wOperatingState = ntohs(driveStateData->wOperatingState);
//	driveStateData->wResetCause = ntohs(driveStateData->wResetCause);
//	driveStateData->ShowstopReason = ntohs(driveStateData->ShowstopReason);
//	driveStateData->ShowstopState = ntohs(driveStateData->ShowstopState);
//
//FUNCTIONEND:
//	delete[] bData;
//	Common::Instance.WriteToLog(LogTypeTrace,
//			"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rAck);
//	RET(rAck);
//}

/*
 * Name			:	siDriveResetRequest
 * Description	:	This method initiates a hard reset of the drive
 * Inputs		:	bCellNo - Slot ID
 *   				dwTimeoutInMillSec - Timeout in milli seconds
 * Return		:
 */
//Byte siDriveResetRequest(Byte bCellNo, DRIVE_RESET_REQUEST_MODE mode, Dword dwTimeoutInMillSec) {
//	Byte rAck = success;
//	Word wDataLength = 0;
//
//	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s, bCellNo=%d, mode = %d, T/O=%d\n",
//			__FUNCTION__, bCellNo, mode, dwTimeoutInMillSec);
//
//	// Sanity checks
//
//	// 1a) Check if slot id is valid
//	// 1b) Check if SIO is connected
//	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR: %s - Sanity check failed with error code 0x%x\n",
//				__FUNCTION__, rAck);
//		goto FUNCTIONEND;
//	}

	// 2) Check if timeout value is valid
//	if (dwTimeoutInMillSec < 1) {
//		Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: Timeout %d is invalid\n",
//				__FUNCTION__, dwTimeoutInMillSec);
//		rAck = invalidArgumentError;
//		goto FUNCTIONEND;
//	}

	// 3) Check if mode value is valid
//        switch (mode) {
//         case RESET_TYPE_NORMAL:      /* FALLTHROUGH */
//          case RESET_TYPE_SHOWSTOP:    /* FALLTHROUGH */
//          case RESET_TYPE_POR_RESET:   /* FALLTHROUGH */
//          case RESET_TYPE_HARD_RESET:  /* FALLTHROUGH */
//          case RESET_TYPE_SOFT_RESET:
//            break;
//          default:
//	    Common::Instance.WriteToLog(LogTypeError,
//				"ERROR %s - Invalid argument: mode %d is invalid\n",
//				__FUNCTION__,mode);
//            rAck = invalidArgumentError;
//	    goto FUNCTIONEND;
//        }

//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,					/* Slot Id */
//			uart3OpcodeDriveResetCmd,	/* Opcode = 0x4F */
//			0x00,						/* Flag Byte = 0 */
//			&wDataLength,				/* Data Length = 0 */
//			0x00,						/* Data Sequence = 0 */
//                       mode,
//			NULL,						/* Data Buffer = NULL */
//			dwTimeoutInMillSec			/* Command Time out */
//			);

	// Check if DriveReset command failed because of internal wrapper error
//	if ((rAck > success) && (rAck < maxTerError)) {
		// There is an internal wrapper error
//		Common::Instance.WriteToLog(LogTypeError,
//				"Uart3LinkControl method failed with error code 0x%x\n", rAck);
//		goto FUNCTIONEND;
//	}

	// If the drive is ready and has changed to UART2 mode.
	// Change the FPGA to enable the UART2 mode
//	if (rAck == uart3AckReady) {
//		Byte rc = Common::Instance.TerEnableFpgaUart2Mode(bCellNo, 1);
//		if (rc != success) {
//			Common::Instance.WriteToLog(
//					LogTypeError,
//					"ERROR: %s - TerEnableFpgaUart2Mode failed with error code 0x%x\n",
//					__FUNCTION__, rc);
//			rAck = rc;
//		}
//		goto FUNCTIONEND;
//	}

//FUNCTIONEND:
//	Common::Instance.WriteToLog(LogTypeTrace, "Exit: %s, exit code=0x%02x\n",
//		__FUNCTION__, rAck);
//	RET(rAck);
//}
//----------------------------------------------------------------------
Word getMaxChunkSize()
//----------------------------------------------------------------------
{
    return 16*1024;
}

//----------------------------------------------------------------------
//Byte siUart3LinkControl(Byte bCellNo, Byte bChannelId, Byte bReqRespOpcode, Byte bFlagByte, Word *wPtrDataLength, Word wDataSequence, Word wReserved, Byte *DataBuffer, Byte *ec, Word Timeout_mS)
//----------------------------------------------------------------------
//{
//	Byte rAck = success;
//	rAck = Common::Instance.Uart3LinkControl(
//			bCellNo,					/* Slot Id */
//			bReqRespOpcode,                         	/* Opcode = 0x4F */
//			bFlagByte,						/* Flag Byte = 0 */
//			wPtrDataLength,				/* Data Length = 0 */
//			wDataSequence,						/* Data Sequence = 0 */
//  //                      wReserved,
//			DataBuffer,						/* Data Buffer = NULL */
//			Timeout_mS			/* Command Time out */
//			);
//       *ec = rAck;
 //      return rAck;
//}

Byte siSerialSend(Byte bCellNo, Byte *bData, Word wLength){
        Byte rc  = success;
        Byte ack = success;
        TER_Status status = TER_Status_none;

        Common::Instance.LogSendReceiveBuffer("Req Header: ", bData,wLength);

        WAPI_CALL_RPC_WITh_RETRY(
                status = (Common::Instance.GetSIOHandle())->TER_SioSendBuffer(bCellNo, wLength, bData));

        if (status != TER_Status_none) {
                Common::Instance.LogError(__FUNCTION__, bCellNo, status);
                rc =  Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
                return rc;
        }
        return ack;
}

Byte siSerialReceive(Byte bCellNo, Word wTimeoutInms, Byte *bData, Word *wLength, Byte *isHardwareError)
{
        Byte ack = success;
	unsigned int respLength = 0;
	TER_Status status = TER_Status_none;
        Common::Instance.WriteToLog(LogTypeTrace, "receiving data...\n");

        respLength = *wLength;
	WAPI_CALL_RPC_WITh_RETRY(
                status = (Common::Instance.GetSIOHandle())->TER_SioReceiveBuffer(
                                        (int)bCellNo, &respLength,(unsigned int)wTimeoutInms,(unsigned char *)bData, (unsigned int *)&status));

        if (status != TER_Status_none) {
                Common::Instance.LogError(__FUNCTION__, bCellNo, status);
                ack = Common::Instance.GetWrapperErrorCode(status, TER_Notifier_Cleared);
                return ack;
        }

        if (respLength != *wLength) {
                Common::Instance.WriteToLog(LogTypeError, "ERROR: Receive data length mismatch\n");
                ack = invalidDataLengthError;
                return ack;
        }

        Common::Instance.LogSendReceiveBuffer("Resp Data: ", bData, *wLength);

        return ack;
}
void siFlushRxUart(Byte bCellNo){
	Common::Instance.TerFlushReceiveBuffer(bCellNo);
//  if (local_saves.CellSupportsAutoRxFlush == false) {
//    FlushRxUART(bCellNo, psda[bCellNo].UARTChannelId); // maybe a good idea to flush the UART receive buffer first (but quite in-efficient)
//  } else {
//    TCPRINTF("skipping FlushRxUART (since CellSupportsAutoRxFlush)");
//  }
}

Byte siSerialSendReceiveComp(Byte bCellNo, Word wTimeoutInMillSec){
  Byte ack = success;
  return ack;
}

#ifdef __cplusplus
}
#endif
