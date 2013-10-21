#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>

#include "../testcase/tcTypes.h"
#include "common.h"
#include "commonEnums.h"
#include "slibu3.h"
#include "libsi.h"
#include "../uart3io/ccb.h"

/**
 * <pre>
 * Description:
 * 	Execute SCSI Command as CCB block on UART3 protocol.
 * 	Issue a SCSI command and data transfer with Query Request.
 * Arguments:
 *	CCB cb			: INPUT - Cell command block
 * 	void data		: INPUT - The data to be sent to the drive
 * 	Byte bCellNo	: INPUT - Cell number
 * Return:
 * </pre>
 ******************************************************************************/
Byte siExecuteSCSICommand(CCB *cb, void *data, Byte bCellNo) {
	Byte rAck = success;
	struct timeval startTime, endTime;
	Dword dwTimeoutInMillSec = 0;
	Dword Timeout_mS_remaining = dwTimeoutInMillSec;
	const Dword Timeout_mS_issue_command = 10 * 1000; // fixed 10 sec
	Byte bCommandBehaviorFlag = NOT_DEFINED;

	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s, bCellNo=%d\n",
			__FUNCTION__, bCellNo);

	// Validate arguments

	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	if (cb == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Null argument. CCB argument is Null\n",
				__FUNCTION__);
		rAck = nullArgumentError;
		goto FUNCTIONEND;
	}
	dwTimeoutInMillSec = cb->timeout;

	Common::Instance.WriteToLog(
			LogTypeTrace,
			"%s- CCB Data: cdblength=%d, senselength=%d, read=%d, write=%d, timeout=%d msec\n",
			__FUNCTION__, cb->cdblength, cb->senselength, cb->read, cb->write,
			cb->timeout);

	if ((cb->cdblength < MINCDBLENGTH) || (MAXCDBLENGTH < cb->cdblength)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. CDB size %d is out of range\n",
				__FUNCTION__, cb->cdblength);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}

	if (MAXSENSELENGTH < cb->senselength) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Invalid argument. Sense data size %d is out of range\n",
				__FUNCTION__, cb->senselength);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}

	if ((0 < cb->read) && (0 < cb->write)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Read and Write size are invalid\n",
				__FUNCTION__);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	if (cb->timeout < 1) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Timeout %d is invalid\n",
				__FUNCTION__, cb->timeout);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	// Set command behavior
	if ((cb->read == 0) && (cb->write == 0) && (data == NULL)) {
		bCommandBehaviorFlag = withoutDataTransfer;
	} else if ((cb->write > 0) && (data != NULL)) {
		bCommandBehaviorFlag = writeDataTransfer;
	} else if ((cb->read > 0) && (data != NULL)) {
		bCommandBehaviorFlag = readDataTransfer;
	} else {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Invalid argument. Invalid command behavior has been specified\n",
				__FUNCTION__);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	// initialize timer
	gettimeofday(&startTime, NULL);

	// send SCSI commad block (Transfer CDB & Execute CDB)
	rAck = siIssueSCSICommand(bCellNo, cb->cdblength, &cb->cdb[0],
			Timeout_mS_issue_command);
	if (rAck != uart3DriveBusyWithDataReceived) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- siIssueSCSICommand failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	// Timeout Check
	gettimeofday(&endTime, NULL);
	if (dwTimeoutInMillSec
			< Common::Instance.GetElapsedTimeInMillSec(startTime, endTime)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Timed out while executing the command\n", __FUNCTION__);
		rAck = commandTimeoutError;
		goto FUNCTIONEND;
	}
	Timeout_mS_remaining = (dwTimeoutInMillSec
			- Common::Instance.GetElapsedTimeInMillSec(startTime, endTime));

	// Data Transfer
	switch (bCommandBehaviorFlag) {

	case withoutDataTransfer:
		Common::Instance.WriteToLog(LogTypeTrace,
				"%s: No data transfer required\n", __FUNCTION__);
		break;
	case writeDataTransfer:
		rAck = siWriteDataTransfer(bCellNo, (Dword) cb->write, (Byte*)data,
				Timeout_mS_remaining);
		switch (rAck) {
		case uart3AckSenseAvailable:
		case uart3AckSCSICommandComplete:
			goto FUNCTIONEND;
			break;
		case uart3DriveBusyWithDataReceived:
			break;
		default:
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: %s- siReadDataTransfer failed with error code 0x%x\n",
					__FUNCTION__, rAck);
			goto FUNCTIONEND;
			break;
		}
		break;
	case readDataTransfer:
		rAck = siReadDataTransfer(bCellNo, (Dword) cb->read, (Byte*)data,
				Timeout_mS_remaining);
		switch (rAck) {
		case uart3AckSenseAvailable:
		case uart3AckSCSICommandComplete:
			goto FUNCTIONEND;
			break;
		case uart3AckBusy:
			break;
		default:
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: %s- siReadDataTransfer failed with error code 0x%x\n",
					__FUNCTION__, rAck);
			goto FUNCTIONEND;
			break;
		}
		break;
	default:
		break;
	}

	// Timeout Check
	gettimeofday(&endTime, NULL);
	if (dwTimeoutInMillSec
			< Common::Instance.GetElapsedTimeInMillSec(startTime, endTime)) {
		rAck = commandTimeoutError;
		goto FUNCTIONEND;
	}
	Timeout_mS_remaining = (dwTimeoutInMillSec
			- Common::Instance.GetElapsedTimeInMillSec(startTime, endTime));

	rAck = siQueryRequestWaitForCommandComplete(bCellNo, Timeout_mS_remaining);

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}

/**
 * <pre>
 * Description:
 *	Execute the SCSI command
 * Arguments:
 *   Byte bCellNo 				: INPUT		- Cell number
 *   Word wSize   				: INPUT 	- Transfer data length
 *   Byte *bData  				: OUTPUT 	- Data buffer to save the data from the drive
 *   Dword dwTimeoutInMillSec	: INPUT 	- Timeout in millisecond
 * Return:
 *
 * </pre>
 *****************************************************************************/
Byte siIssueSCSICommand(Byte bCellNo, Word wSize, Byte *bData,
		Dword dwTimeoutInMillSec) {
	Byte rAck = success;
	const Word Timeout_mS_max_call = 10 * 1000;
	struct timeval startTime, endTime;
	Dword Timeout_mS_remaining = dwTimeoutInMillSec;
	Dword Timeout_mS_this_call = 0;
	Word wRetry = 0;

	Common::Instance.WriteToLog(LogTypeTrace,
			"Entry: %s, bCellNo=%d, wSize=%d, T/O=%d msec\n", __FUNCTION__,
			bCellNo, wSize, dwTimeoutInMillSec);

	// Validate Arguments

	// Check the slot id
	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	// Size range check
	if ((wSize < MINCDBLENGTH) || (MAXCDBLENGTH < wSize)) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Command size %d is out of range\n",
				__FUNCTION__, wSize);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}

	// Null argument check
	if (bData == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Null argument has been passed\n", __FUNCTION__);
		rAck = nullArgumentError;
		goto FUNCTIONEND;
	}

	if (dwTimeoutInMillSec < 1) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Timeout %d is invalid\n",
				__FUNCTION__, dwTimeoutInMillSec);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	// initialize timer
	gettimeofday(&startTime, NULL);

	for (;;) {
		// Timeout Check
		gettimeofday(&endTime, NULL);
		if (dwTimeoutInMillSec
				< Common::Instance.GetElapsedTimeInMillSec(startTime, endTime)) {
			rAck = commandTimeoutError;
			goto FUNCTIONEND;
		}

		Timeout_mS_remaining = (dwTimeoutInMillSec
				- Common::Instance.GetElapsedTimeInMillSec(startTime, endTime));
		Timeout_mS_this_call = MIN(Timeout_mS_remaining, Timeout_mS_max_call);

		rAck = siSCSICmdRequest(bCellNo, wSize, bData, Timeout_mS_this_call);
		if ((rAck == commandTimeoutError) || (rAck == checksumMismatchError)) {
			wRetry++;
		} else if (rAck == uart3DriveBusyWithDataReceived) {
			goto FUNCTIONEND;
		} else {
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: %s- Received unexpected ack 0x%x from siSCSICmdRequest\n",
					__FUNCTION__, rAck);
			goto FUNCTIONEND;
		}

		usleep(1 * 1000 * 1000);
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}

/**
 * <pre>
 * Description:
 *	Execute the Query command
 * Arguments:
 *   Byte bCellNo 				: INPUT - cell number
 *   Dword dwTimeoutInMillSec	: INPUT - Timeout in millisecond
 * Return:
 *
 * </pre>
 *****************************************************************************/
Byte siQueryRequestWaitForCommandComplete(Byte bCellNo,
		Dword dwTimeoutInMillSec) {
	Byte rAck = success;
	const Word Timeout_mS_max_call = 10 * 1000;
	struct timeval startTime, endTime;
	Dword Timeout_mS_remaining = dwTimeoutInMillSec;
	Dword Timeout_mS_this_call = 0;
	Word wRetry = 0;
	Byte bFlagByte = 0;

	Common::Instance.WriteToLog(LogTypeTrace,
			"Entry: %s, bCellNo=%d, T/O=%d msec\n", __FUNCTION__, bCellNo,
			dwTimeoutInMillSec);

	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	if (dwTimeoutInMillSec < 1) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Timeout %d is invalid\n",
				__FUNCTION__, dwTimeoutInMillSec);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	// initialize timer
	gettimeofday(&startTime, NULL);

	for (;;) {
		// Timeout Check
		gettimeofday(&endTime, NULL);
		if (dwTimeoutInMillSec < Common::Instance.GetElapsedTimeInMillSec(startTime, endTime)) {
			rAck = commandTimeoutError;
			goto FUNCTIONEND;
		}

		Timeout_mS_remaining = (dwTimeoutInMillSec
				- Common::Instance.GetElapsedTimeInMillSec(startTime, endTime));
		Timeout_mS_this_call = MIN(Timeout_mS_remaining, Timeout_mS_max_call);

		rAck = siQueryRequest(bCellNo, bFlagByte, Timeout_mS_this_call);
		if ((rAck == uart3AckBusy) || (rAck == commandTimeoutError)
				|| (rAck == checksumMismatchError)) {
			wRetry++;
		} else if (rAck == uart3AckSCSICommandComplete) {
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Return command complete\n", __FUNCTION__);
			break;
		} else if (rAck == uart3AckSenseAvailable) {
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Return check completed\n", __FUNCTION__);
			break;
		} else {
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: %s- Received unexpected ack 0x%x from siQueryRequest\n",
					__FUNCTION__, rAck);
			goto FUNCTIONEND;
		}

		usleep(1 * 1000 * 1000);
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}

/**
 * <pre>
 * Description:
 *	Read the data from the drive
 * Arguments:
 *   Byte bCellNo				: INPUT		- Cell Number
 *   Dword dwSize				: INPUT 	- Data length to be read from the drive
 *   Byte *bData  				: OUTPUT 	- Data buffer which will have the data from the drive
 *   Dword dwTimeoutInMillSec	: INPUT 	- Timeout in millisecond
 * Return:
 *
 * </pre>
 *****************************************************************************/
Byte siReadDataTransfer(Byte bCellNo, Dword dwSize, Byte *bData,
		Dword dwTimeoutInMillSec) {

	Byte rAck = success;
	struct timeval startTime, endTime;
	const Word Timeout_mS_max_call = 30 * 1000;
	Dword Timeout_mS_remaining = dwTimeoutInMillSec;
	Dword Timeout_mS_this_call = 0;

	Word wChunkSize = 0;
	Word wDataSequence = 1;
	Dword dwDataPtr = 0;
	Dword dwRetry = 0;
	Byte bFlagByte = 0x01;

	Common::Instance.WriteToLog(LogTypeTrace,
			"Entry: %s, bCellNo=%d, wSize=%ld, T/O=%ld msec\n", __FUNCTION__,
			bCellNo, dwSize, dwTimeoutInMillSec);

	// Validate Arguments

	// Check cell number
	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	// Null argument check
	if (bData == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Null argument has been passed. bData=%p\n",
				__FUNCTION__, bData);
		rAck = nullArgumentError;
		goto FUNCTIONEND;
	}

	// Data size range check
	if ((dwSize < 1) || (0x0FFFFFFF < dwSize)) { // 256 Mbytes
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Data size %ld is out of range\n",
				__FUNCTION__, dwSize);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}

	if (dwTimeoutInMillSec < 1) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Timeout %ld is invalid\n",
				__FUNCTION__, dwTimeoutInMillSec);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	// initialize timer
	gettimeofday(&startTime, NULL);

	// Data Transfer Sequence
	for (;;) {
		// Timeout Check
		gettimeofday(&endTime, NULL);
		if (dwTimeoutInMillSec < Common::Instance.GetElapsedTimeInMillSec(startTime, endTime)) {
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: %s- Timed out while executing the Read Data command\n",
					__FUNCTION__);
			rAck = commandTimeoutError;
			goto FUNCTIONEND;
		}
		Timeout_mS_remaining = (dwTimeoutInMillSec
				- Common::Instance.GetElapsedTimeInMillSec(startTime, endTime));
		Timeout_mS_this_call = MIN(Timeout_mS_remaining, Timeout_mS_max_call);

		// data size check
		if (dwDataPtr > dwSize) {
			Common::Instance.WriteToLog(LogTypeError,
					"ERROR: %s- Received more data %ld than expected %ld\n",
					__FUNCTION__, dwDataPtr, dwSize);
			rAck = invalidDataLengthError;
			goto FUNCTIONEND;
		} else if (dwDataPtr == dwSize) {
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Data transfer completed\n", __FUNCTION__);
			break;
		}

		// update chunk size
		wChunkSize =
				(dwSize - dwDataPtr) > UART3_MAX_CHUNK_SIZE ?
						UART3_MAX_CHUNK_SIZE : (dwSize - dwDataPtr);

		// data transfer
		rAck = siReadDataRequest(bCellNo, &wChunkSize, wDataSequence,
				&bData[dwDataPtr], Timeout_mS_this_call);

		// ack evaluation
		if ((rAck == uart3AckBusy) || (rAck == checksumMismatchError)
				|| (rAck == commandTimeoutError)
				|| (rAck == uart3AckInvalidSyncWord)
				|| (rAck == uart3AckFlowControlError)) {
			dwRetry++;
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Received ack 0x%x. Retrying ...\n", __FUNCTION__,
					rAck);
			continue;
		} else if (rAck == uart3AckSenseAvailable) {
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Sense available. Return immediately\n", __FUNCTION__);
			goto FUNCTIONEND;
		} else if (rAck == uart3AckInvalidState) {
			Common::Instance.WriteToLog(LogTypeTrace, "%s: Restart sequence\n", __FUNCTION__);
			wChunkSize = 0;
			wDataSequence = 1;
			dwDataPtr = 0;
			continue;
		} else if (rAck == uart3AckDataAvailable_DriveRetainsControl) {
			dwDataPtr += wChunkSize;
			wDataSequence++;
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Read request sequence success! %d, %d", __FUNCTION__,
					wDataSequence, dwDataPtr);
		} else {
			Common::Instance.WriteToLog(LogTypeError,
					"ERROR: %s- Received unexpected ack 0x%x from siReadDataRequest\n", __FUNCTION__,
					rAck);
			goto FUNCTIONEND;
		}
	}

	// send Query Request with flag for report finished last sequence.
	for (;;) {
		// Timeout Check
		gettimeofday(&endTime, NULL);
		if (dwTimeoutInMillSec
				< Common::Instance.GetElapsedTimeInMillSec(startTime, endTime)) {
			Common::Instance.WriteToLog(LogTypeError,
					"ERROR: %s- Timed out while executing the command\n",
					__FUNCTION__);
			rAck = commandTimeoutError;
			goto FUNCTIONEND;
		}
		Timeout_mS_remaining = (dwTimeoutInMillSec
				- Common::Instance.GetElapsedTimeInMillSec(startTime, endTime));
		Timeout_mS_this_call = MIN(Timeout_mS_remaining, Timeout_mS_max_call);

		// query request
		rAck = siQueryRequest(bCellNo, bFlagByte, Timeout_mS_this_call);

		// ack evaluation
		if (rAck == commandTimeoutError) {
			/* nop */
		} else if (rAck == uart3AckBusy) {
			break;
		} else {
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: %s- Received unexpected ack 0x%x from siQueryRequest\n",
					__FUNCTION__, rAck);
			goto FUNCTIONEND;
		}
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}

/**
 * <pre>
 * Description:
 *	Write the data to the drive
 * Arguments:
 *   Byte bCellNo				: INPUT - Cell number
 *   Dword dwSize				: INPUT - Data length.
 *   Byte *bData  				: INPUT - Pointer of send data buffer.
 *   Dword dwTimeoutInMillSec 	: INPUT - Timeout in millisecond
 * Return:
 *
 * </pre>
 *****************************************************************************/
Byte siWriteDataTransfer(Byte bCellNo, Dword dwSize, Byte *bData,
		Dword dwTimeoutInMillSec) {

	Byte rAck = success;
	struct timeval startTime, endTime;

	Word Timeout_mS_max_call = 30 * 1000;
	Dword Timeout_mS_remaining = dwTimeoutInMillSec;
	Dword Timeout_mS_this_call = 0;

	Word wChunkSize = 0;
	Word wDataSequence = 1;
	Dword dwDataPtr = 0;
	Dword dwRetry = 0;

	Common::Instance.WriteToLog(LogTypeTrace,
			"Entry: %s, bCellNo=%d, dwSize=%l, T/O=%l msec\n", __FUNCTION__,
			bCellNo, dwSize, dwTimeoutInMillSec);

	// Validate arguments.
	// Cell number check
	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}
	// Null argument check
	if (bData == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Null argument has been passed\n", __FUNCTION__);
		rAck = nullArgumentError;
		goto FUNCTIONEND;
	}
	// Data length range check
	if ((dwSize < 1) || (0x0FFFFFFF < dwSize)) { // 256 Mbytes
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Data size %l is out of range\n",
				__FUNCTION__, dwSize);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}
	// Timeout value check
	if (dwTimeoutInMillSec < 1) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Timeout %l is invalid\n",
				__FUNCTION__, dwTimeoutInMillSec);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	// initialize timer
	gettimeofday(&startTime, NULL);

	// Data Transfer Sequence
	for (;;) {
		// Timeout Check
		gettimeofday(&endTime, NULL);
		if (dwTimeoutInMillSec < Common::Instance.GetElapsedTimeInMillSec(startTime, endTime)) {
			Common::Instance.WriteToLog(LogTypeError, "ERROR: %s- Timed out while executing the command\n", __FUNCTION__);
			rAck = commandTimeoutError;
			goto FUNCTIONEND;
		}

		Timeout_mS_remaining = (dwTimeoutInMillSec
				- Common::Instance.GetElapsedTimeInMillSec(startTime, endTime));
		Timeout_mS_this_call = MIN(Timeout_mS_remaining, Timeout_mS_max_call);

		// data size check
		if (dwDataPtr > dwSize) {
			Common::Instance.WriteToLog(LogTypeError,
					"ERROR: %s- Received more data %d than expected %d\n",
					__FUNCTION__, dwDataPtr, dwSize);
			rAck = invalidDataLengthError;
			goto FUNCTIONEND;
		} else if (dwDataPtr == dwSize) {
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Data transfer completed\n", __FUNCTION__);
			break;
		}

		// update chunk size
		wChunkSize =
				(dwSize - dwDataPtr) > UART3_MAX_CHUNK_SIZE ?
						UART3_MAX_CHUNK_SIZE : (dwSize - dwDataPtr);

		// data transfer
		rAck = siWriteDataRequest(bCellNo, wChunkSize, wDataSequence,
				&bData[dwDataPtr], Timeout_mS_this_call);

		// ack evaluation
		if ((rAck == uart3AckBusy) || (rAck == checksumMismatchError)
				|| (rAck == commandTimeoutError)
				|| (rAck == uart3AckInvalidSyncWord)
				|| (rAck == uart3AckFlowControlError)) {
			dwRetry++;
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Received ack 0x%x. Retrying ...\n", __FUNCTION__,
					rAck);
			continue;
		} else if (rAck == uart3AckSenseAvailable) {
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Sense available. Return immediately\n", __FUNCTION__);
			goto FUNCTIONEND;
		} else if (rAck == uart3AckInvalidState) {
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Restarting sequence\n", __FUNCTION__);
			wChunkSize = 0;
			wDataSequence = 1;
			dwDataPtr = 0;
			continue;
		} else if (rAck == uart3DriveBusyWithDataReceived) {
			dwDataPtr += wChunkSize;
			wDataSequence++;
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Write data request success! %d, %d\n", __FUNCTION__,
					wDataSequence, dwDataPtr);
			continue;
		} else {
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: %s- Received unexpected ack 0x%x from siWriteDataRequest\n",
					__FUNCTION__, rAck);
			goto FUNCTIONEND;
		}
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}

/**
 * <pre>
 * Description:
 *	Set the drive response delay time for the acknowledgment
 * Arguments:
 *   Byte bCellNo				- INPUT - Cell number
 *   Word wDelayTimeInMicroSec 	- INPUT - Delay time in micro second
 * Return:
 *
 * </pre>
 *****************************************************************************/
Byte siSetDriveResponseDelayTimeForAck(Byte bCellNo,
		Word wDelayTimeInMicroSec) {

	Common::Instance.WriteToLog(LogTypeTrace,
			"Entry: %s, bCellNo=%d, wDelayTimeInMicroSec=%d\n", __FUNCTION__,
			bCellNo, wDelayTimeInMicroSec);

	Byte rAck = success;
	Word wTimeout = 12000;
	GETSETU3PARAM paramU3;

	// Validate arguments
	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	rAck = siGetUARTParametersRequest(bCellNo, &paramU3, wTimeout);
	if (rAck != uart3AckDataAvailable_DriveRetainsControl) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siGetUARTParametersRequest\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	paramU3.bAckDelayTimeInMicroSecondsH = (wDelayTimeInMicroSec & 0xFF00) >> 8;
	paramU3.bAckDelayTimeInMicroSecondsL = (wDelayTimeInMicroSec & 0x00FF);

	rAck = siSetUARTParametersRequest(bCellNo, &paramU3, wTimeout);
	if (rAck != uart3AckReady) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siSetUARTParametersRequest\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}

/**
 * <pre>
 * Description:
 *	Set the maximum read block size
 * Arguments:
 *   Byte bCellNo 				- INPUT - Cell number
 *   Word wMaxBlockSizeForRead	- INPUT - Max read block size
 * Return:
 *
 * </pre>
 *****************************************************************************/
Byte siSetMaxReadBlockSize(Byte bCellNo, Word wMaxBlockSizeForRead) {
	Byte rAck = 1;
	Word wTimeout = 12000;
	GETSETU3PARAM paramU3;

	Common::Instance.WriteToLog(LogTypeTrace,
			"Entry: %s, bCellNo=%d, wMaxBlockSizeForRead=%d\n", __FUNCTION__,
			bCellNo, wMaxBlockSizeForRead);

	// Validate arguments
	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	rAck = siGetUARTParametersRequest(bCellNo, &paramU3, wTimeout);
	if (rAck != uart3AckDataAvailable_DriveRetainsControl) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siGetUARTParametersRequest\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	paramU3.bMaxBlockSizeForReadLH = (wMaxBlockSizeForRead & 0xFF00) >> 8;
	paramU3.bMaxBlockSizeForReadLL = (wMaxBlockSizeForRead & 0x00FF);

	rAck = siSetUARTParametersRequest(bCellNo, &paramU3, wTimeout);
	if (rAck != uart3AckReady) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siSetUARTParametersRequest\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

        rAck = 0;  //katayama 20120710
FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}

/**
 * <pre>
 * Description:
 *	Set the UART line speed
 * Arguments:
 *   Byte bCellNo 			- INPUT - Cell number
 *   Dword dwLineSpeedInBps	- INPUT - UART line speed
 * Return:
 *
 * </pre>
 *****************************************************************************/
Byte siSetUartLineSpeed(Byte bCellNo, Dword dwLineSpeedInBps) {
	Byte rAck = success;
	Byte bFlagByte = 0;
	Word wTimeout = 12000;
	Byte bLineSpeedNumber = 0;
	GETSETU3PARAM u3p;

	Common::Instance.WriteToLog(LogTypeTrace,
			"Entry: %s, bCellNo=%d, dwLineSpeeInBps=%ld\n", __FUNCTION__,
			bCellNo, dwLineSpeedInBps);

	// Validate arguments
	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	switch (dwLineSpeedInBps) {

	case (115200):
		bLineSpeedNumber = lineSpeed115200;
		break;
	case (1843200):
		bLineSpeedNumber = lineSpeed1843200;
		break;
	case (2083000):
		bLineSpeedNumber = lineSpeed2083000;
		break;
	case (2380000):
		bLineSpeedNumber = lineSpeed2380000;
		break;
	case (2778000):
		bLineSpeedNumber = lineSpeed2777000;
		break;
	case (3333000):
		bLineSpeedNumber = lineSpeed3333000;
		break;
	case (4167000):
		bLineSpeedNumber = lineSpeed4166000;
		break;
	case (5556000):
		bLineSpeedNumber = lineSpeed5555000;
		break;
	case (8333000):
		bLineSpeedNumber = lineSpeed8333000;
		break;
	case (11111000):
		bLineSpeedNumber = lineSpeed11111000;
		break;
	case (16666000):
		bLineSpeedNumber = lineSpeed16666000;
		break;
	default:
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Invalid argument. Line speed %l is invalid\n",
				__FUNCTION__, dwLineSpeedInBps);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
		break;
	}

	rAck = siGetUARTParametersRequest(bCellNo, &u3p, wTimeout);
	if (rAck != uart3AckDataAvailable_DriveRetainsControl) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siGetUARTParametersRequest\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	u3p.bUartLineSpeedL = bLineSpeedNumber;

	rAck = siSetUARTParametersRequest(bCellNo, &u3p, wTimeout);
	if (rAck != uart3AckReady) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siSetUARTParametersRequest\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	rAck = siSetUartBaudrate(bCellNo, dwLineSpeedInBps);
	if (rAck != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- siSetUartBaudrate failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}
	usleep(100 * 1000); // Sleep for 100 milli seconds
//	task_twait(100);

	rAck = siQueryRequest(bCellNo, bFlagByte, wTimeout);
	if (rAck != uart3AckReady) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siQueryRequest. Expected ack = 0x%x\n",
				__FUNCTION__, rAck, uart3AckReady);
		goto FUNCTIONEND;
	}

	rAck = success;
FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}

/**
 * <pre>
 * Description:
 *	Set Inter segment delay
 * Arguments:
 *   Byte bCellNo 				- INPUT - Cell number
 *   Word wInterSegmentDelay	- INPUT - Inter segment delay
 * Return:
 *
 * </pre>
 *****************************************************************************/
Byte siSetUartInterSegmentDelay(Byte bCellNo, Word wInterSegmentDelay) {
	Byte rAck = success;
	Word wTimeout = 12000;
	GETSETU3PARAM paramU3;

	Common::Instance.WriteToLog(LogTypeTrace,
			"Entry: %s, bCellNo=%d, wInterSegmentDelay=%d\n", __FUNCTION__,
			bCellNo, wInterSegmentDelay);

	// Validate arguments
	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	rAck = siGetUARTParametersRequest(bCellNo, &paramU3, wTimeout);
	if (rAck != uart3AckDataAvailable_DriveRetainsControl) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siGetUARTParametersRequest. Expected ack = 0x%x\n",
				__FUNCTION__, rAck, uart3AckDataAvailable_DriveRetainsControl);
		goto FUNCTIONEND;
	}

	paramU3.bInterSegmentDelayInMicroSecondsH = (Byte) ((wInterSegmentDelay
			& 0xFF00) >> 8);
	paramU3.bInterSegmentDelayInMicroSecondsL = (Byte) (wInterSegmentDelay
			& 0x00FF);

	rAck = siSetUARTParametersRequest(bCellNo, &paramU3, wTimeout);
	if (rAck != uart3AckReady) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siGetUARTParametersRequest. Expected ack = 0x%x\n",
				__FUNCTION__, rAck, uart3AckDataAvailable_DriveRetainsControl);
		goto FUNCTIONEND;
	}
	rAck = success;

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}

/**
 * <pre>
 * Description:
 *	Change the settings to start using UART3 protocol
 * Arguments:
 *   Byte bCellNo 			- INPUT - Cell number
 *   Word dwUartLineSpeed	- INPUT - Baud rate
 *   Word wTimeoutInMillSec	- INPUT - Timeout in millisecond
 * Return:
 *
 * </pre>
 *****************************************************************************/
Byte siChangeToUart3_debug(Byte bCellNo, Dword dwUartLineSpeed,
		Word wTimeoutInMillSec) {
	Byte rAck = success;
	Byte bEndianFlag = 1; /* always big endian. */

	Common::Instance.WriteToLog(LogTypeTrace,
			"Entry: %s, bCellNo=%d, dwUartLineSpeed=%ld, T/O=%d\n", __FUNCTION__,
			bCellNo, dwUartLineSpeed, wTimeoutInMillSec);

	// Validate arguments
	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	if (wTimeoutInMillSec < 1) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Invalid argument. Timeout %d is less than 1 millisecond\n",
				__FUNCTION__, wTimeoutInMillSec);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	rAck = siSetUartProtocol(bCellNo, bEndianFlag);
	if (rAck != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- siSetUartProtocol failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	rAck = siSetUartLineSpeed(bCellNo, dwUartLineSpeed);
	if (rAck != uart3AckReady) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- siSetUartLineSpeed failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

FUNCTIONEND:
	Common::Instance.WriteToLog(LogTypeTrace,
			"Exit : %s, exit code=0x%x\n", __FUNCTION__, rAck);
	RET(rAck);
}
