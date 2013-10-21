#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>

#include "tcTypes.h"
#include "common.h"
#include "commonEnums.h"
#include "terSioLib.h"

/**
 * <pre>
 * Description:
 *   UART 3 protocol driver
 * Arguments:
 *   Byte bCellNo - INPUT - HGST cell id
 *   Byte bReqRespOpcode - INPUT - UART op code
 *   Word wDataLength - INPUT - Data length
 *   Word wDataSequence - INPUT - Data sequence
 *   Byte *DataBuffer - INPUT/OUTPUT - Data buffer
 *   Word Timeout_mS - INPUT - Timeout in msec
 * Return:
 * </pre>
 *****************************************************************************/
Byte Common::Uart3LinkControl(Byte bCellNo, Byte bReqRespOpcode, Byte bFlagByte,
		Word *wPtrDataLength, Word wDataSequence, Byte *DataBuffer,
		Dword Timeout_mS) {
	WriteToLog(
			LogTypeTrace,
			"Entry:%s, bCellno=%d,bFlagByte=0x%x, bReqRespOpcode=0x%x, wPtrDataLength=%p, wDataSeqnce=%d, DataBuffer=%p, T/O=%ld\n",
			__FUNCTION__, bCellNo, bFlagByte, bReqRespOpcode, wPtrDataLength,
			wDataSequence, DataBuffer, Timeout_mS);

	Byte rc = success;
	Byte requestHeader[UART3_REQUEST_RESPONSE_HEADER_LENGTH];
	Byte responseHeader[UART3_REQUEST_RESPONSE_HEADER_LENGTH];
	Byte ack[UART3_RECEIVE_ACK_LENGTH];
	Dword dwDataChecksum = 0;
	Word wHeaderChecksum = 0;
	unsigned int respLength = 0;
	Dword dwCalcChecksum32 = 0;
	Word wCalcChecksum16 = 0;
	Byte DoSendData = 0;
	Byte DoReceiveHeaderAndData = 0;
	TER_Status status = TER_Status_none;
	unsigned int recvBufStatus = TER_Status_none;

	Word wTimeoutInMillSec = Timeout_mS;
	struct timeval startTime, endTime;
	Word wElapsedTimeInMillSec = 0;
	Word wTotalElapsedTimeMillSec = 0;

	// Validate arguments
	if ((rc = SanityCheck(bCellNo)) != success) {
		WriteToLog(LogTypeError,
				"ERROR: %s - Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	if (Timeout_mS < 1) {
		WriteToLog(LogTypeError, "ERROR: %s - Timeout %ld is invalid\n",
				__FUNCTION__, Timeout_mS);
		rc = invalidArgumentError;
		goto FUNCTIONEND;
	}

	switch (bReqRespOpcode) {
	case uart3OpcodeAbortSCSICmd:
	case uart3OpcodeDriveResetCmd:
	case uart3OpcodeQueryCmd:
	case uart3OpcodeSetUART2ModeCmd:
		break;

	case uart3OpcodeEchoCmd:
	case uart3OpcodeGetDriveStateCmd:
	case uart3OpcodeGetUARTParametersCmd:
	case uart3OpcodeSCSICmd:
	case uart3OpcodeSetUARTParametersCmd:
		if (DataBuffer == NULL) {
			WriteToLog(
					LogTypeError,
					"ERROR: %s - Null argument has been passed. DataBuffer=%p\n",
					__FUNCTION__, DataBuffer);
			rc = nullArgumentError;
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeMemoryReadCmd:
	case uart3OpcodeMemoryWriteCmd:
	case uart3OpcodeReadDataCmd:
	case uart3OpcodeWriteDataCmd:
		if (DataBuffer == NULL) {
			WriteToLog(
					LogTypeError,
					"ERROR: %s - Null argument has been passed. DataBuffer=%p\n",
					__FUNCTION__, DataBuffer);
			rc = nullArgumentError;
			goto FUNCTIONEND;
		}
		if (wDataSequence == 0) {
			WriteToLog(LogTypeError, "ERROR: %s - Invalid sequence number %d\n",
					__FUNCTION__, wDataSequence);
			rc = invalidArgumentError;
			goto FUNCTIONEND;
		}
		break;
	default:
		WriteToLog(LogTypeError, "ERROR: %s - Invalid opcode %d\n",
				__FUNCTION__, bReqRespOpcode);
		rc = invalidArgumentError;
		goto FUNCTIONEND;
	}

	if (bFlagByte > MAX_UART3_FLAG_BYTE) {
		WriteToLog(LogTypeError, "ERROR: %s - Invalid flag byte %d\n",
				__FUNCTION__, bFlagByte);
		rc = invalidArgumentError;
		goto FUNCTIONEND;
	}

	// ----------------------------------------------------------------------
	// << Initialize >>
	// ----------------------------------------------------------------------

	WriteToLog(LogTypeTrace, "initializing...\n");

	// Make sure that none of the pointers are null
	if (wPtrDataLength == NULL) {
		WriteToLog(
				LogTypeError,
				"ERROR: %s - Null argument has been passed. wPtrDataLength=%p\n",
				__FUNCTION__, wPtrDataLength);
		rc = nullArgumentError;
		goto FUNCTIONEND;
	}

	memset(requestHeader, 0x00, UART3_REQUEST_RESPONSE_HEADER_LENGTH);
	memset(responseHeader, 0x00, UART3_REQUEST_RESPONSE_HEADER_LENGTH);

	// Clear the receive buffer before starting a new command
	TerFlushReceiveBuffer(bCellNo);

	// ----------------------------------------------------------------------
	// << Send Request Header >>
	// ----------------------------------------------------------------------

	WriteToLog(LogTypeTrace, "sending request header...\n");

	// Sync Word. Indicates start of header. It should always be 0xAA 0x55
	requestHeader[bUart3SyncWordH] = 0xAA;
	requestHeader[bUart3SyncWordL] = 0x55;

	// Opcode
	requestHeader[bUart3ReqRespOpcode] = bReqRespOpcode;
	requestHeader[bUart3FlagByte] = bFlagByte;

	// Data Length
	requestHeader[bUart3DataLengthH] = (*wPtrDataLength >> 8) & 0xff;
	requestHeader[bUart3DataLengthL] = *wPtrDataLength & 0xff;

	// Data Sequence number
	requestHeader[bUart3DataSequenceH] = (wDataSequence >> 8) & 0xff;
	requestHeader[bUart3DataSequenceL] = wDataSequence & 0xff;

	// Data Checksum
	dwDataChecksum = ComputeUart3Checksum32Bits(DataBuffer, *wPtrDataLength);
	requestHeader[bUart3DataChecksumHH] = (dwDataChecksum >> 24) & 0xff;
	requestHeader[bUart3DataChecksumHL] = (dwDataChecksum >> 16) & 0xff;
	requestHeader[bUart3DataChecksumLH] = (dwDataChecksum >> 8) & 0xff;
	requestHeader[bUart3DataChecksumLL] = dwDataChecksum & 0xff;

	// Header Checksum
	wHeaderChecksum = ComputeUart3Checksum16Bits((Byte *) &requestHeader,
			sizeof(requestHeader));
	requestHeader[bUart3HeaderChecksumH] = (wHeaderChecksum >> 8) & 0xff;
	requestHeader[bUart3HeaderChecksumL] = wHeaderChecksum & 0xff;

	LogSendReceiveBuffer("Req Header: ", (Byte*) &requestHeader,
			UART3_REQUEST_RESPONSE_HEADER_LENGTH);

	// Start the timer
	gettimeofday(&startTime, NULL);

	WAPI_CALL_RPC_WITh_RETRY(
			status = sioLib->TER_SioSendBuffer(
					bCellNo, UART3_REQUEST_RESPONSE_HEADER_LENGTH,
					requestHeader));
	// Get the end time for sending request header.
	gettimeofday(&endTime, NULL);


	if (status != TER_Status_none) {
		LogError(__FUNCTION__, bCellNo, status);
		rc = GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}
	wElapsedTimeInMillSec = GetElapsedTimeInMillSec(startTime, endTime);
	wTotalElapsedTimeMillSec += wElapsedTimeInMillSec;
	WriteToLog(LogTypeTrace, "Elapsed time for sending request header: %d msec\n", wElapsedTimeInMillSec);

	// ----------------------------------------------------------------------
	// << Receive Ack >>
	// ----------------------------------------------------------------------
	WriteToLog(LogTypeTrace, "receiving ack...\n");

	memset(ack, 0, UART3_RECEIVE_ACK_LENGTH);
	respLength = UART3_RECEIVE_ACK_LENGTH;


	// Start the timer for receiving ack
	gettimeofday(&startTime, NULL);

	WAPI_CALL_RPC_WITh_RETRY(
			status = sioLib->TER_SioReceiveBuffer(
					bCellNo, &respLength, wTimeoutInMillSec, ack,
					&recvBufStatus));

	// Stop the timer for receiving ack
	gettimeofday(&endTime, NULL);

	wElapsedTimeInMillSec = GetElapsedTimeInMillSec(startTime, endTime);

	// ==========  Start the timer for elapsed time bet recpt of ack and send of data
	gettimeofday(&startTime, NULL);

	wTotalElapsedTimeMillSec += wElapsedTimeInMillSec;
	LogSendReceiveBuffer("Resp Ack: ", ack, UART3_RECEIVE_ACK_LENGTH);
	WriteToLog(LogTypeTrace, "Elapsed time to receive ack: %d msec\n", wElapsedTimeInMillSec);


	if (status != TER_Status_none) {
		LogError(__FUNCTION__, bCellNo, status);
		rc = GetWrapperErrorCode(status, TER_Notifier_Cleared);
		goto FUNCTIONEND;
	}

	if (respLength != UART3_RECEIVE_ACK_LENGTH) {
		WriteToLog(
				LogTypeError,
				"ERROR: Did not receive expected number of bytes. Expected - %d, Received - %d\n",
				UART3_RECEIVE_ACK_LENGTH, respLength);
		rc = respPacketLengthMismatch;
		goto FUNCTIONEND;
	}
	if (((ack[0] + ack[1]) & 0xff) != 0x00) {
		WriteToLog(LogTypeError, "ERROR: Ack checksum error\n");
		rc = checksumMismatchError;
		goto FUNCTIONEND;
	}

	// ----------------------------------------------------------------------
	// << Evaluate Ack >>
	// ----------------------------------------------------------------------

	WriteToLog(LogTypeTrace, "evaluating ack...\n");
	rc = ack[0];

	switch (bReqRespOpcode) {
	case uart3OpcodeSCSICmd:
		if (ack[0] == uart3AckDataRequested)
			DoSendData = 1;
		else {
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeQueryCmd:
		goto FUNCTIONEND;
	case uart3OpcodeReadDataCmd:
		if (ack[0] == uart3AckDataAvailable_DriveRetainsControl) {
			DoReceiveHeaderAndData = 1;
		} else {
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeWriteDataCmd:
		if (ack[0] == uart3AckDataRequested) {
			DoSendData = 1;
		} else {
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeAbortSCSICmd:
		goto FUNCTIONEND;
	case uart3OpcodeGetUARTParametersCmd:
		if (ack[0] == uart3AckDataAvailable_DriveRetainsControl) {
			DoReceiveHeaderAndData = 1;
		} else {
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeSetUARTParametersCmd:
		if (ack[0] == uart3AckDataRequested) {
			DoSendData = 1;
		} else {
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeSetUART2ModeCmd:
		goto FUNCTIONEND;
	case uart3OpcodeMemoryReadCmd:
		if (ack[0] == uart3AckDataAvailable_DriveRetainsControl) {
			DoReceiveHeaderAndData = 1;
		} else {
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeMemoryWriteCmd:
		if (ack[0] == uart3AckDataRequested) {
			DoSendData = 1;
		} else {
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeEchoCmd:
		if (ack[0] == uart3AckDataAvailable_DriveRetainsControl) {
			DoReceiveHeaderAndData = 1;
		} else {
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeGetDriveStateCmd:
		if (ack[0] == uart3AckDataAvailable_DriveRetainsControl) {
			DoReceiveHeaderAndData = 1;
		} else {
			goto FUNCTIONEND;
		}
		break;
	case uart3OpcodeDriveResetCmd:
		goto FUNCTIONEND;
	default:
		WriteToLog(LogTypeError, "ERROR: Invalid opcode 0x%x\n", bReqRespOpcode);
		rc = invalidArgumentError;
		goto FUNCTIONEND;
	}

	// ----------------------------------------------------------------------
	// << Send Data >>
	// ----------------------------------------------------------------------


	if (DoSendData) {
		WriteToLog(LogTypeTrace, "sending data...\n");

		LogSendReceiveBuffer("Send Data: ", DataBuffer, *wPtrDataLength);


		// =============== Stop the timer for elapsed time bet recpt of ack and start of send data
		gettimeofday(&endTime, NULL);
		WriteToLog(LogTypeTrace, "elapsed time bet recpt of ack and start of send data: %d usec\n", GetElapsedTimeInMicroSec(startTime, endTime));
		
		// Start the timer for sending the data
		gettimeofday(&startTime, NULL);

		WAPI_CALL_RPC_WITh_RETRY(
				status = sioLib->TER_SioSendBuffer(
						bCellNo, *wPtrDataLength,
						DataBuffer));

		// Stop the timer for sending the data
		gettimeofday(&endTime, NULL);

		wElapsedTimeInMillSec = GetElapsedTimeInMillSec(startTime, endTime);
		wTotalElapsedTimeMillSec += wElapsedTimeInMillSec;
		WriteToLog(LogTypeTrace, "Elapsed time in sending data: %d msec\n", wElapsedTimeInMillSec);

		if (status != TER_Status_none) {
			LogError(__FUNCTION__, bCellNo, status);
			rc = GetWrapperErrorCode(status, TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}

		WriteToLog(LogTypeTrace, "receiving ack... (for data block)\n");

		memset(ack, 0, UART3_RECEIVE_ACK_LENGTH);
		respLength = UART3_RECEIVE_ACK_LENGTH;


		// Start the timer for receiving ack
		gettimeofday(&startTime, NULL);

		WAPI_CALL_RPC_WITh_RETRY(
				status = sioLib->TER_SioReceiveBuffer(
								bCellNo, &respLength,
								wTimeoutInMillSec, ack, &recvBufStatus));

		// Stop the timer for receiving the ack
		gettimeofday(&endTime, NULL);


		LogSendReceiveBuffer("Resp Ack: ", ack, UART3_RECEIVE_ACK_LENGTH);
		rc = ack[0];

		wElapsedTimeInMillSec = GetElapsedTimeInMillSec(startTime, endTime);
		wTotalElapsedTimeMillSec += wElapsedTimeInMillSec;
		WriteToLog(LogTypeTrace, "Elapsed time in receiving ack for data block: %d msec\n", wElapsedTimeInMillSec);

		if (status != TER_Status_none) {
			LogError(__FUNCTION__, bCellNo, status);
			rc = GetWrapperErrorCode(status, TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}

		if (respLength != UART3_RECEIVE_ACK_LENGTH) {
			WriteToLog(
					LogTypeError,
					"ERROR: Did not receive expected number of bytes. Expected - %d, Received - %d\n",
					UART3_RECEIVE_ACK_LENGTH, respLength);
			rc = respPacketLengthMismatch;
			goto FUNCTIONEND;
		}
		if (((ack[0] + ack[1]) & 0xff) != 0x00) {
			WriteToLog(LogTypeError, "ERROR: Ack checksum error\n");
			rc = checksumMismatchError;
			goto FUNCTIONEND;
		}

		// Evaluate Ack.
		if (ack[0] == uart3AckBusy) {
			WriteToLog(LogTypeTrace,
					"Ack rewriting... busy to busy with data received.\n");
			rc = ack[0] = uart3DriveBusyWithDataReceived;
		}
	}

	// ----------------------------------------------------------------------
	// << Receive Response Header >>
	// ----------------------------------------------------------------------

	if (DoReceiveHeaderAndData) {
		WriteToLog(LogTypeTrace, "receiving response header...\n");

		respLength = UART3_REQUEST_RESPONSE_HEADER_LENGTH;


		// Start the timer for receiving response header
		gettimeofday(&startTime, NULL);
		WAPI_CALL_RPC_WITh_RETRY(
				status = sioLib->TER_SioReceiveBuffer(
								bCellNo, &respLength,
								wTimeoutInMillSec, responseHeader,
								&recvBufStatus));

		// Stop the timer for receiving response header
		gettimeofday(&endTime, NULL);


		wElapsedTimeInMillSec = GetElapsedTimeInMillSec(startTime, endTime);
		wTotalElapsedTimeMillSec += wElapsedTimeInMillSec;
		WriteToLog(LogTypeTrace, "Elasped time in receiving response header: %d msec\n", wElapsedTimeInMillSec);

		if (status != TER_Status_none) {
			LogError(__FUNCTION__, bCellNo, status);
			rc = GetWrapperErrorCode(status, TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}
		if (respLength != UART3_REQUEST_RESPONSE_HEADER_LENGTH) {
			WriteToLog(
					LogTypeError,
					"ERROR: Did not receive expected number of bytes. Expected - %d, Received - %d\n",
					UART3_REQUEST_RESPONSE_HEADER_LENGTH, respLength);
			rc = respPacketLengthMismatch;
			goto FUNCTIONEND;
		}

		LogSendReceiveBuffer("Resp Header: ", responseHeader,
				UART3_REQUEST_RESPONSE_HEADER_LENGTH);

		wCalcChecksum16 = ComputeUart3Checksum16Bits(responseHeader,
				(UART3_REQUEST_RESPONSE_HEADER_LENGTH - 2));
		wHeaderChecksum = (responseHeader[bUart3HeaderChecksumH] << 8)
				+ responseHeader[bUart3HeaderChecksumL];
		if (wHeaderChecksum != wCalcChecksum16) {
			WriteToLog(LogTypeError,
					"ERROR: Response header checksum mismatch\n");
			rc = checksumMismatchError;
			goto FUNCTIONEND;
		}
		if ((responseHeader[bUart3SyncWordH] != 0xAA)
				|| (responseHeader[bUart3SyncWordL] != 0x55)) {
			WriteToLog(LogTypeError,
					"ERROR: Invalid response header sync word 0x%x 0x%x\n",
					responseHeader[bUart3SyncWordH],
					responseHeader[bUart3SyncWordL]);
			rc = invalidRespSyncPacket;
			goto FUNCTIONEND;
		}
		if (responseHeader[bUart3ReqRespOpcode]
				!= requestHeader[bUart3ReqRespOpcode]) {
			WriteToLog(LogTypeError,
					"ERROR: Opcode mismatch. Request 0x%x, Response 0x%x\n",
					requestHeader[bUart3ReqRespOpcode],
					responseHeader[bUart3ReqRespOpcode]);
			rc = opcodeMismtach;
			goto FUNCTIONEND;
		}

		// compare with data length between source parameter and response header.
		Word dataLength = (responseHeader[bUart3DataLengthH] << 8)
				+ responseHeader[bUart3DataLengthL];
		if (dataLength > *wPtrDataLength) {
			WriteToLog(LogTypeError,
					"ERROR: Data length in response header %d exceeds max length %d\n",
					dataLength, *wPtrDataLength);
			rc= invalidDataLengthError;
			goto FUNCTIONEND;
		}

		// rewrite receive data length from response headder.
		WriteToLog(LogTypeTrace, "overwriting wDataLength to %d\n", dataLength);
		*wPtrDataLength = dataLength;
		if ((responseHeader[bUart3DataSequenceH] != requestHeader[bUart3DataSequenceH])
				|| (responseHeader[bUart3DataSequenceL]
						!= requestHeader[bUart3DataSequenceL])) {
			WriteToLog(LogTypeError,
					"ERROR: Invalid response header sequence number\n");
			rc = uart3DataSequenceMismatch;
			goto FUNCTIONEND;
		}
	}

	// ----------------------------------------------------------------------
	// << Receive Data >>
	// ----------------------------------------------------------------------

	if (DoReceiveHeaderAndData) {
		WriteToLog(LogTypeTrace, "receiving data...\n");

		respLength = *wPtrDataLength;


		// Start the timer for receiving data
		gettimeofday(&startTime, NULL);
		WAPI_CALL_RPC_WITh_RETRY(
				status = sioLib->TER_SioReceiveBuffer(
								bCellNo, &respLength,
								wTimeoutInMillSec, DataBuffer, &recvBufStatus));

		// Stop the timer
		gettimeofday(&endTime, NULL);

		wElapsedTimeInMillSec = GetElapsedTimeInMillSec(startTime, endTime);
		wTotalElapsedTimeMillSec += wElapsedTimeInMillSec;
		WriteToLog(LogTypeTrace, "Elapsed time in receiving data: %d msec\n", wElapsedTimeInMillSec);

		if (status != TER_Status_none) {
			LogError(__FUNCTION__, bCellNo, status);
			rc = GetWrapperErrorCode(status, TER_Notifier_Cleared);
			goto FUNCTIONEND;
		}

		if (respLength != *wPtrDataLength) {
			WriteToLog(LogTypeError, "ERROR: Receive data length mismatch\n");
			rc = invalidDataLengthError;
			goto FUNCTIONEND;
		}

		LogSendReceiveBuffer("Resp Data: ", DataBuffer, *wPtrDataLength);

		dwCalcChecksum32 = ComputeUart3Checksum32Bits(DataBuffer,
				*wPtrDataLength);
		dwDataChecksum = ((responseHeader[bUart3DataChecksumHH] << 24)
				& 0xff000000)
				+ ((responseHeader[bUart3DataChecksumHL] << 16) & 0x00ff0000)
				+ ((responseHeader[bUart3DataChecksumLH] << 8) & 0x0000ff00)
				+ (responseHeader[bUart3DataChecksumLL] & 0x000000ff);
		if (dwDataChecksum != dwCalcChecksum32) {
			WriteToLog(LogTypeError, "ERROR: Data checksum mismatch\n");
			rc = checksumMismatchError;
			goto FUNCTIONEND;
		}
	}

FUNCTIONEND:
	WriteToLog(LogTypeTrace,
			"Exit : %s, exit code = 0x%x\n", __FUNCTION__, rc);
	if (rc == 0x26) {
		TER_TransactionTrace transactionTrace[MAX_TRANSACTION_TRACE];
		memset (transactionTrace, 0, sizeof(transactionTrace));
		WriteToLog(LogTypeError, "------------- SIO Trace: for last 4 transactions ------------- \n");
		sioLib->TER_GetTransactionTrace (bCellNo, &(transactionTrace[0]));
		for (int ii = 0; ii < MAX_TRANSACTION_TRACE; ii++) {
			int seconds = 0;
			int microsecs = 0;
			// calculate elapsed time since last transaction
			if (ii) {
				seconds = transactionTrace[ii].tv_start_sec - transactionTrace[ii-1].tv_end_sec;
				microsecs = transactionTrace[ii].tv_start_usec - transactionTrace[ii-1].tv_end_usec;
			        if (microsecs <  0) {
                 	              	seconds --;
					microsecs += 1000000;
				}
			}
			char description [GET_DIAG_INFO_LEN_64];
			sprintf (description, "(%s),",transactionTrace[ii].eventCodeDescription);
			WriteToLog(LogTypeError, " 0x%0x %-18s %d bytes,\ttimeStart: %d.%06d, timeEnd: %d.%06d, time elapsed since last op: %d.%06d sec\n",
				transactionTrace[ii].eventCode, description, transactionTrace[ii].transferredBytes, 
				transactionTrace[ii].tv_start_sec, transactionTrace[ii].tv_start_usec,
				transactionTrace[ii].tv_end_sec, transactionTrace[ii].tv_end_usec, seconds, microsecs);
		}
		WriteToLog(LogTypeError, "----------------------- End SIO Trace ------------------------ \n");
	}
	RET(rc);
}
