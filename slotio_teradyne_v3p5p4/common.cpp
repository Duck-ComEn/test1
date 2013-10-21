/*
 * common.cpp
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include "common.h"
#include "commonEnums.h"
#include "libsi.h"
#include "terSioLib.h"

/* === Utility Methods (macros) === */

#define /*Maximum Value*/ MFGMax(a, b) ((a) >= (b)? (a) : (b))
#define /*Minimum Value*/ MFGMin(a, b) ((a) <= (b)? (a) : (b))
#define /*"NULL" for NULL*/	MFGN4N(ADR) ((ADR)? (ADR) : "NULL")

Common Common::Instance;

Common::Common(void) {
	OpenLogFile();
	logLevel = 0;
	sioConnected = false;
	slotIndex = 0;
	uart2Protocol = Uart2ProtocolTypeARM;
	resetSlotDuringInitialize = 1;
	wrapperErrorCode = success;
	sioErrorStatus = TER_Status_none;
	sioSlotError = TER_Notifier_Cleared;
}

Common::~Common(void)
{
//	fclose(logFile);
}

void Common::OpenLogFile()
{

}

/*
 * Name			:	SetLogLevel
 * Description	:	Set the logging level
 * Input		:	level - logging level
 *      			0: Disabled.
 *      			1 (Default): Only Wrapper Error Messages (TLOG_ERROR).
 *      			2: All Wrapper Messages (TLOG_ERROR , TLOG_ENTRY, TLOG_EXIT, TLOG_TRACE)
 *      			3: a) All Wrapper Messages (TLOG_ERROR , TLOG_ENTRY, TLOG_EXIT, TLOG_TRACE) b) SIO Journal on error (100 SIO Journal entries)
 *      			4: a) All Wrapper Messages (TLOG_ERROR , TLOG_ENTRY, TLOG_EXIT, TLOG_TRACE) b) SIO Journal on error (100 SIO Journal entries) c) SIO Journal in siFinalize()
 * Return		: 	None
 */
void Common::SetLogLevel(int level)
{
	if (level < LOG_LEVEL_MIN || level > LOG_LEVEL_MAX)
		return;
	logLevel = level;
}


/*
 * Name			:	GetLogLevel
 * Description	:	Get the current logging level
 * Input		:	None
 * Return		:	Log Level
 */
int Common::GetLogLevel()
{
	return logLevel;
}


/*
 * Name			:	SetWapitestResetSlotDuringInitialize
 * Description	:	Set whether a slot gets reset during siInitialize (This is meant for wapitest only)
 * Input		:	resetEnable - reset a slot (True or False)
 * Return		:	None
 */
void Common::SetWapitestResetSlotDuringInitialize(unsigned int resetEnable)
{
	resetSlotDuringInitialize = resetEnable;
}


/*
 * Name			:	GetWapitestResetSlotDuringInitialize
 * Description	:	This method tells whether a slot should be reset during siInitialize(wapitest only)
 * Input		:	None
 * Return		:	True - Reset slot
 * 					False - Don't reset slot
 */
unsigned int Common::GetWapitestResetSlotDuringInitialize()
{
	return resetSlotDuringInitialize;
}


/*
 * Name			:	GetSIOHandle
 * Description	:	Get a handle to the SIO library
 * Input		:	None
 * Return		:	Return a pointer to the SIO library
 */
TerSioLib* Common::GetSIOHandle()
{
	return sioLib;
}

/*
 * Name 		:	WriteToLog
 * Description	:	Write the message to the log file
 * Input		:	logType - Type of message (Error or Trace)
 * 					msg - unformatted message
 * Return		:	None
 */
void Common::WriteToLog(LogType logType, const char* msg, ...)
{
	// If log type is Error then the log level should be at least 1
	// If log type is Trace then the log level should be at least 2
	// If log type is Journaling then the log level should be at least 3

	if (logType == LogTypeError && logLevel < 1)
		return;
	if (logType == LogTypeTrace && logLevel < 2)
		return;
	if (logType == LogTypeJournal && logLevel < 3)
		return;

	FILE* log = fopen(logFileName, "a+");
	if (log == NULL)
		return;

	// get time
	time_t timep;
	struct tm tm;
	time(&timep);
	localtime_r(&timep, &tm);

	char lineBuf[1024];
	int ret, size;
	int currentLine = 0;
	int maxLine = sizeof(lineBuf) - 6;
	size = currentLine < maxLine ? maxLine - currentLine : 0;

	// Print the time
	ret = snprintf(&lineBuf[currentLine], size, "%04d/%02d/%02d,%02d:%02d:%02d,",
	                 tm.tm_year + 1900,
	                 tm.tm_mon + 1,
	                 tm.tm_mday,
	                 tm.tm_hour,
	                 tm.tm_min,
	                 tm.tm_sec
	                );

	if (ret < size)
		currentLine += ret;
	else
		currentLine += size - 1;

	// Print the message
	va_list ap;
	va_start(ap, msg);
	size = currentLine < maxLine ? maxLine - currentLine : 0;
	ret = vsnprintf(&lineBuf[currentLine], size, msg, ap);
	if (ret < size)
		currentLine += ret;
	else
		currentLine += size - 1;
	va_end(ap);

    fwrite(&lineBuf[0], 1, currentLine, log);
    fflush(log);
    fclose(log);

#if defined(WAPI_TCLOGGER_ENABLE)
   tcPrintf("Neptune2", 0, "Wrapper", &lineBuf[0]);
#endif
}


/*
 * Name			:	LogErrorWithJournal
 * Description	:	Write the error to the log file. If the log level allows to write journaling information then write that too.
 * Input		:	funcName - The function name where the error happened
 * 					bCellNo - The Slot ID
 * 					status - Error Status
 * Return		: 	None
 */
void Common::LogError(const char *funcName, Byte bCellNo, TER_Status status) {
	char errorString[256];
	TER_Status status1 = sioLib->TER_ConvertStatusToString(status, errorString);
	if (status1 == TER_Status_none)
		WriteToLog(LogTypeError, "ERROR: %s: %s \n", funcName, errorString);
}

/*
 * Name			:	LogSendReceiveBuffer
 * Description	:	Log the send or receive buffer contents in the log file
 * Input		:	txt - The header  for the buffer data
 * 					buf - The buffer
 * 					len - Length of the buffer data
 * Return		: 	None
 */
void Common::LogSendReceiveBuffer(char *txt, Byte *buf, Word len) {
	// If buffer is empty then we don't need to do anything. Write the header to file
	if (len == 0) {
		WriteToLog(LogTypeTrace, "%s<none>\n", txt);
		return;
	}

	len = (len > (SEND_RECEIVE_BUFFER_LINE_MAX / 2)) ?
			(SEND_RECEIVE_BUFFER_LINE_MAX / 2) : len;

	char line[SEND_RECEIVE_BUFFER_LINE_MAX + (SEND_RECEIVE_BUFFER_LINE_MAX / 2)
			+ 2];for
(	int bufCount = 0, lineCount = 0; bufCount < len; bufCount++, lineCount += 3)
	sprintf(&line[lineCount], "%02x ", buf[bufCount]);

	WriteToLog(LogTypeTrace, "%s%s\n", txt, line);
}


/*
 * Name			:	SetSlotIndex
 * Description	:	Set the slot index that gets called during siInitialize. This will be used for
 * 					sanity in subsequent calls to the wrapper
 * Input		:	index = slot index
 * Return		:	None
 */
void Common::SetSlotIndex(int index) {
	if (index < SLOT_INDEX_MIN || index > SLOT_INDEX_MAX)
	{
		slotIndex = 0;
		return;
	}
	slotIndex = index;
	time_t timep;
	struct tm tm;
	char strDateTime[128];
	time(&timep);
	localtime_r(&timep, &tm);
	sprintf(strDateTime, "%04d.%02d.%02d.%02d.%02d.%02d", tm.tm_year + 1900,
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	sprintf(logFileName, "/var/tmp/slot%03d_%s.txt", index, strDateTime);
}


/*
 * Name			:	TerSioConnect
 * Description	:	Connect to the SIO
 * Input		:	None
 * Return		:	0 = success
 * 					Non-Zero = Failure
 */
Byte Common::TerSioConnect(void) {
	if (sioLib == NULL) {
		sioLib = new TerSioLib();
	}

	if (sioLib == NULL) {
		WriteToLog(LogTypeError, "ERROR: Cannot create NeptuneIso2 Object \n");
		RET(terError);
	}

	TER_Status status;
	char *sioIpAddress;

	sioIpAddress = getenv("SIOIPADDRESS");
	if (sioIpAddress != NULL) {
		WriteToLog(LogTypeTrace, "SIO IP Address = %s\n", sioIpAddress);
		status = sioLib->TER_Connect(sioIpAddress);
	} else
		status = sioLib->TER_Connect((char *) "10.101.1.2");

	if (status != TER_Status_none) {
		char errorString[MAX_ERROR_STRING_LENGTH];
		sioLib->TER_ConvertStatusToString(status, errorString);
		WriteToLog(LogTypeError, "ERROR: terSio2Connect: %s \n", errorString);
		RET(GetWrapperErrorCode(status, TER_Notifier_Cleared));
	}

	sioConnected = true;

	RET(success);
}


/*
 * Name			:	TerSioDisconnect
 * Description	:	Disconnect from the SIO
 * Input		:	None
 * Return		:	0 = success
 * 					Non-Zero = Failure
 */
Byte Common::TerSioDisconnect(void) {
	TER_Status status = TER_Status_none;
	char wbuf[256];
	Byte rc = success;

	status = sioLib->TER_Disconnect();
	if (status != TER_Status_none) {
		sioLib->TER_ConvertStatusToString(status, wbuf);
		WriteToLog(LogTypeError, "ERROR: terSio2Disconnect: %s \n", wbuf);
		rc = GetWrapperErrorCode(status, TER_Notifier_Cleared);
	}

	delete sioLib;
	sioLib = NULL;
	sioConnected = false;

	RET(rc);
}

/*
 * Name			:	TerSioReconnect
 * Description	:	Reconnect to the SIO after a RPC timeout error
 * Input		:	None
 * Return		:	0 = success
 * 					Non-Zero = Failure
 */
Byte Common::TerSioReconnect(void) {
	Byte rc = success;

	TerSioDisconnect();
	rc = TerSioConnect();
	if (rc != TER_Status_none) {
		WriteToLog(LogTypeError, "ERROR: terSio2Reconnect failed: 0x%x \n", rc);
	}

	RET(rc);
}


/*
 * Name			:	computeUart2Checksum
 * Description	:	This method computes Uart2 checksum of a buffer.
 * Input		:	buffer = The data buffer
 * 					len	= The length of the data buffer
 * Return		:	Computed checksum
 */
Word Common::ComputeUart2Checksum(Byte *buffer, Word len) {
	Word j = 4, checkSum = 0;

	/* Calculate checksum */

	while (j < len) {
		checkSum += (buffer[j] << 8) | buffer[j + 1];
		j += 2;
	}
	checkSum = (0 - checkSum);

	return (checkSum);
}


/*
 * Name			:	SetUart2Protocol
 * Description	:	Set the UART2 Protocol Type
 * Input		:	protocol = 0 : A00 Protocol (PTB)
 * 							   1 : ARM Protocol (JPK)
 * Return		: 	None
 */
void Common::SetUart2Protocol(Uart2ProtocolType protocol) {
	uart2Protocol = protocol;
}


/*
 * Name			:	ValidateUart2ReceivedData
 * Description	:	Validate the response from the drive for the UART2 protocol
 * Input 		:	sendRecvBuffer = Data sent to the drive
 * 					cmdLength = Length of the UART2 command
 * 					respLength = Length of the response data
 * 					expLength = Expected length of the response data
 * 					pBuffer =
 * 					status1 =
 * Return		:	0 = success
 * 					Non-Zero = Failure
 */
Byte Common::ValidateUart2ReceivedData(Byte *sendRecvBuf, Word cmdLength,
		unsigned int respLength, unsigned int expLength, Word *pBuffer,
		unsigned int status1) {
	Byte rc = success, temp;
	Word i, j, k, checksum;

	LogSendReceiveBuffer("Res Pkt: ", sendRecvBuf, respLength);

	/* If transmit is halted in FPGA due to error, log error, clear halt condition and get out */
	if (UART2_TX_HALT_ON_ERROR_MASK & status1) {
		WriteToLog(LogTypeError, "ERROR: Transmit halted in FPGA due to error \n");

		// Find out why transmit is halted
		Byte error = success;

		// Did the FPGA time out waiting for a response from the drive?
		if (UART2_ACK_TIMEOUT_MASK & status1) {
			WriteToLog(LogTypeError, "ERROR: Drive ACK timeout \n");
			error = driveAckTimeout;
		}

		if (error == success) {
			// FPGA did not time out. Did we get a bad ack from the drive
			for (i = 0; i < respLength; i++) {
				if (sendRecvBuf[i] != uart2AckSuccess) {
					WriteToLog(
							LogTypeError,
							"ERROR: Did not receive good response (0x80) from the drive %02x \n",
							sendRecvBuf[i]);
					error = sendRecvBuf[i];
					break;
				}
			}
		}

		rc = siClearHaltOnError(slotIndex); /* Clear screeching halt condition */
		if (rc != success) {
			WriteToLog(
					LogTypeError,
					"ERROR: %s - siClearHaltOnError failed with error code 0x%x\n",
					__FUNCTION__, rc);
		}
		RET(error);
	} else {
		WriteToLog(LogTypeTrace, "----> TX success. No screeching halt \n");
	}

	/* Check if we received the expected number of bytes */
	if (respLength != expLength) {
		WriteToLog(
				LogTypeError,
				"ERROR: Did not receive the expected number of bytes: Received %d, Expected %d \n",
				respLength, expLength);
		RET(respPacketLengthMismatch);
	} else {
		WriteToLog(LogTypeTrace, "----> Received expected number of bytes \n");
	}

	// Check if we received expected number of acks
	for (i = 0; i < (cmdLength / 2); i++) {
		if (sendRecvBuf[i] != uart2AckSuccess) {
			WriteToLog(
					LogTypeError,
					"ERROR: Did not receive the expected number of Acks %02x \n",
					sendRecvBuf[i]);
			RET(uart2InvalidNumOfAcks);
		}
	}
	WriteToLog(LogTypeTrace, "----> Received expected number of ACKs \n");

	*pBuffer = i;

	if (uart2Protocol == Uart2ProtocolTypeARM) /* ARM Processor */
	{
		/* Perform endian swap depending on protocol type */
		for (j = i, k = 0; k < ((respLength - i) / 2); j += 2, k++) {
			temp = sendRecvBuf[j];
			sendRecvBuf[j] = sendRecvBuf[j + 1];
			sendRecvBuf[j + 1] = temp;
		}
	}

	/* Check sync packet - 0xAA55 */
	if ((sendRecvBuf[i] == 0x55) && (sendRecvBuf[i + 1] == 0xAA)) {
		//OK
	} else if ((sendRecvBuf[i] == 0xAA) && (sendRecvBuf[i + 1] == 0x55)) {
		//OK
	} else {
		WriteToLog(LogTypeError, "Did not get sync packet - 0x%02x 0x%02x \n",
				sendRecvBuf[i], sendRecvBuf[i + 1]);
		RET(invalidRespSyncPacket);
	}

	WriteToLog(LogTypeTrace, "----> SYNC Packet is OK \n");

	/* Check return code - 0x0011 */
	if (sendRecvBuf[i + 2] != 0x00 || sendRecvBuf[i + 3] != 0x00) {
		rc = 0;
	}

	checksum = Common::Instance.ComputeUart2Checksum(&sendRecvBuf[i],
			respLength - i - 6);
	RET(rc);
}


/*
 * Name			: 	sanityCheck
 * Description	: 	This method checks whether the given slot id is valid
 * Input		: 	bCellNo = The Slot ID
 * Return		: 	0 = Valid
 * 					1 = Invalid
 */
Byte Common::SanityCheck(Byte bCellNo) {
	Byte rc = success;
	if (!sioConnected)
		rc = sioNotConnected;
	else if (slotIndex != bCellNo || bCellNo <= 0)
		rc = invalidSlotIndex;

	RET(rc);
}

/*
 * Name			:	ComputeUart3Checksum16Bits
 * Description	:	Calculate 16 bits checksum for UART3 protocol
 * Input		:	DataBuffer - Pointer to the data buffer
 * 					DataLength - Length of the data
 * Return		: 	16 bit checksum
 */
Word Common::ComputeUart3Checksum16Bits(Byte *DataBuffer, Word wDataLength) {
	int i = 0;
	Word checksum = 0;

	if (wDataLength == 0)
		return checksum;

	if (wDataLength & 1) {
		// data length is odd -- pad 0x00 at last one byte
		for (i = 0; i < (wDataLength - 1); i += 2) {
			checksum -= ((DataBuffer[i] << 8) & 0xff00) + DataBuffer[i + 1];
		}
		checksum -= ((DataBuffer[i] << 8) & 0xff00) + 0x00;

	} else {
		// data length is even
		for (i = 0; i < wDataLength; i += 2) {
			checksum -= ((DataBuffer[i] << 8) & 0xff00) + DataBuffer[i + 1];
		}
	}

	return checksum;
}

/*
 * Name			:	ComputeUart3Checksum32Bits
 * Description	:	Calculate 32 bits checksum for UART3 protocol
 * Input		:	DataBuffer - Pointer to the data buffer
 * 					DataLength - Length of the data
 * Return		: 	32 bit checksum
 */
Dword Common::ComputeUart3Checksum32Bits(Byte *DataBuffer, Word wDataLength) {
	int i = 0;
	Dword checksum = 0;

	if (wDataLength == 0)
		return checksum;

	if (wDataLength & 1) {
		// data length is odd -- pad 0x00 at last one byte
		for (i = 0; i < (wDataLength - 1); i += 2) {
			checksum -= ((DataBuffer[i] << 8) & 0xff00) + DataBuffer[i + 1];
		}
		checksum -= ((DataBuffer[i] << 8) & 0xff00) + 0x00;
	} else {
		// data length is even
		for (i = 0; i < wDataLength; i += 2) {
			checksum -= ((DataBuffer[i] << 8) & 0xff00) + DataBuffer[i + 1];
		}
	}

	return checksum;
}

/*
 * Name			:	TerFlushReceiveBuffer
 * Description	:	Flush out the receive buffer
 * Input		:	bCellNo 	- Slot ID
 * 					sendRecvBuf - Data buffer
 * 					status		- status
 * Return		:	None
 */
void Common::TerFlushReceiveBuffer(Byte bCellNo) {
	unsigned int status = 0;
	TER_Status status1 = TER_Status_none;
	Byte sendRecvBuf[2048];
	unsigned int respLength = 2048;

	while (true) {
		respLength = 2048;
		status1 = (Common::Instance.GetSIOHandle())->TER_SioReceiveBuffer(
				bCellNo, &respLength, 0, sendRecvBuf, &status);
		if (status1 != TER_Status_none) {
			Common::Instance.LogError(__FUNCTION__, bCellNo, status1);
			break;
		}
		if (respLength == 0) {
			break;
		}
	}

	return;
}

/*
 * Name			:	TerEnableFpgaUart2Mode
 * Description	:	This method enabled or disables the FPGA uart2 mode. This is used for UART3 mode
 * Inputs		:	bCellNo				:	Slot ID ( 1 - based)
 * 					unsigned int enable	: 	True (Enabled UART2 Mode)
 * 											False (Disable UART2 Mode)
 * Returns		:
 */
Byte Common::TerEnableFpgaUart2Mode(Byte bCellNo, unsigned int enable) {
	TER_Status status = TER_Status_none;
	Byte rc = success;
	int values[1] = { enable };
	WriteToLog(LogTypeTrace, "ENTRY: %s, bCellNo = %d, enabled = %d\n",
			__FUNCTION__, bCellNo, enable);

	WAPI_CALL_RPC_WITh_RETRY(
			status = (Common::Instance.GetSIOHandle())->TER_SetProperty(bCellNo, terPropertyTwoByteOneByteMode, 1, values));
	if (status != TER_Status_none) {
		LogError(__FUNCTION__, bCellNo, status);
		rc = terError;
	}

	WriteToLog(LogTypeTrace, "EXIT: %s, exit code = 0x%02x\n", __FUNCTION__,
			rc);
	return rc;
}

/*
 * Name			:	GetElapsedTimeInMillSec
 * Description	:	This method returns the elapsed time in milli seconds
 * Inputs		:	startTime	:	Start time
 * 					endTime		: 	End time
 * Returns		:	Elapsed time in milli seconds
 */
Dword Common::GetElapsedTimeInMillSec(struct timeval startTime,
		struct timeval endTime) {

	Dword seconds = 0;
	Dword useconds = 0;
	if ((endTime.tv_usec - startTime.tv_usec) < 0) {
		seconds = endTime.tv_sec - startTime.tv_sec - 1;
		useconds = 1000000 + endTime.tv_usec - startTime.tv_usec;
	} else {
		seconds = endTime.tv_sec - startTime.tv_sec;
		useconds = endTime.tv_usec - startTime.tv_usec;
	}

	return (seconds * 1000) + (useconds / 1000);
}

/*
 * Name                 :       GetElapsedTimeInMicroSec
 * Description  :       This method returns the elapsed time in microseconds
 * Inputs               :       startTime       :       Start time
 *                                      endTime         :       End time
 * Returns              :       Elapsed time in milli seconds
 */
Dword Common::GetElapsedTimeInMicroSec(struct timeval startTime,
                struct timeval endTime) {

	Dword seconds = 0;
	Dword useconds = 0;
	if ((endTime.tv_usec - startTime.tv_usec) < 0) {
		seconds = endTime.tv_sec - startTime.tv_sec - 1;
		useconds = 1000000 + endTime.tv_usec - startTime.tv_usec;
	} else {
		seconds = endTime.tv_sec - startTime.tv_sec;
		useconds = endTime.tv_usec - startTime.tv_usec;
	}

	return (seconds * 1000000) + useconds;
}

/*
 * Name			:	GetErrorString
 * Description	:	This method returns the error string for the last occurred error
 * Inputs		:	bCellNo				:	Slot ID ( 1 - based)
 * 					errorString[out]	: 	Return error string
 * Returns		:
 */
void Common::GetErrorString(Byte bCellNo, char* errorString, Byte *errorCode) {

	char resultString[3 * MAX_ERROR_STRING_LENGTH];
	*errorCode = wrapperErrorCode;

	if	(wrapperErrorCode == success && sioErrorStatus == TER_Status_none && sioSlotError == TER_Notifier_Cleared) {
		strcpy(errorString, "No Errors");
		return;
	}

	strcpy(resultString, "Wrapper Error: ");
	switch (wrapperErrorCode)
	{
		// Teradyne Internal Error Codes
		case terError:
		strcat(resultString, "Teradyne internal error");
		break;
		case invalidSlotIndex:
		strcat(resultString, "Invalid slot index");
		break;
		case driveAckTimeout:
		strcat(resultString, "Drive Ack Timeout: Timed out while waiting drive to acknowledgment");
		break;
		case sioNotReachable:
		strcat(resultString, "SIO is not reachable");
		break;
		case sioSocketError:
		strcat(resultString, "SIO socket error: SIO is not accepting socket connections");
		break;
		case dioNotResponsive:
		strcat(resultString, "DIO is non responsive");
		break;
		case dioInvalidTPN:
		strcat(resultString, "Invalid DIO TPN (Part number)");
		break;
		case dioFirmwareUpdateError:
		strcat(resultString, "Error while updating DIO firmware");
		break;
		case nullArgumentError:
		strcat(resultString, "NULL argument has been passed");
		break;
		case invalidArgumentError:
		strcat(resultString, "Invalid argument has been passed");
		break;
		case uart3DriveBusyWithDataReceived:
		strcat(resultString, "Drive is busy with the data that was sent to it (UART 3)");
		break;
		case checksumMismatchError:
		strcat(resultString, "Checksum mismatch");
		break;
		case invalidRespSyncPacket:
		strcat(resultString, "Invalid sync packet received from the drive");
		break;
		case opcodeMismtach:
		strcat(resultString, "Request/Response opcode mismatch");
		break;
		case uart3DataSequenceMismatch:
		strcat(resultString, "Data sequence number mismatch (UART 3)");
		break;
		case sioNotConnected:
		strcat(resultString, "SIO is not connected");
		break;
		case invalidAck:
		strcat(resultString, "Invalid acknowledgment received from the drive");
		break;
		case transmitHaltedError:
		strcat(resultString, "Transmit has been halted because of bad drive response");
		break;
		case hTempError:
		strcat(resultString, "HTemp algorithm error");
		break;
		case terSlotError:
		strcat(resultString, "Teradyne slot error");
		break;
		case rpcError:
		strcat(resultString, "SIO RPC error");
		break;
		case outOfRange:
		strcat(resultString, "Values are out of range");
		break;
		case respPacketLengthMismatch:
		strcat(resultString, "Received unexpected number of bytes from the drive");
		break;
		case uart2InvalidNumOfAcks:
		strcat(resultString, "Received invalid number of acks from the drive");
		break;
		case invalidDataLengthError:
		strcat(resultString, "Invalid data length has been received from the drive");
		break;
		case commandTimeoutError:
		strcat(resultString, "Timed out while executing a command");
		break;

		// UART2 Protocol Error Codes
		case uart2AckReceiverOverrun:
		strcat(resultString, "UART2 receiver overrrun error");
		break;
		case uart2AckDataUnstable:
		strcat(resultString, "UART2 data unstable error");
		break;
		case uart2AckFrameError:
		strcat(resultString, "UART2 framing error");
		break;
		case uart2AckInvalidSyncPacket:
		//case uart3AckInvalidSyncWord:
		strcat(resultString, "UART invalid sync packet/word error");
		break;
		case uart2AckInvalidLengthPacket:
		strcat(resultString, "UART2 invalid packet length error");
		break;
		case uart2AckCommandChecksumError:
		//case uart3AckChecksumError:
		strcat(resultString, "UART checksum error");
		break;
		case uart2AckFlowControlError:
		//case uart3AckFlowControlError:
		strcat(resultString, "UART flow control error");
		break;
		case uart2AckInvalidState:
		//case uart3AckInvalidState:
		strcat(resultString, "UART invalid state error");
		break;
		case uart2AckCommandReject:
		//case uart3AckCommandReject:
		strcat(resultString, "UART command reject");
		break;

		// UART3 Protocol Error Codes
		case uart3AckReservationConflict:
		strcat(resultString, "UART3 reservation conflict error");
		break;
		case uart3AckQueueFull:
		strcat(resultString, "UART3 queue full error");
		break;
		case uart3AckResourceBusy:
		strcat(resultString, "UART3 resource busy error");
		break;
		case uart3AckCommandParameterError:
		strcat(resultString, "UART3 command parameter error");
		break;

		default:
		strcat(resultString, "Unknown Error");
		break;
	}

	Byte rc = success;
	if ((rc = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s - Sanity Check Failed. exit code = 0x%02x\n",
				__FUNCTION__, rc);
		goto FUNCTIONEND;
	}

	if (sioErrorStatus != TER_Status_none)
	{
		char sioError[MAX_ERROR_STRING_LENGTH];
		sioLib->TER_ConvertStatusToString(sioErrorStatus, sioError);
		strcat(resultString, ", SIO Error: ");
		strcat(resultString, sioError);
	}

	if (sioSlotError != TER_Notifier_Cleared)
	{
		char slotError[MAX_ERROR_STRING_LENGTH];
		sioLib->TER_ConvertSlotErrorToString(sioSlotError, slotError);
		strcat(resultString, ", Slot Error: ");
		strcat(resultString, slotError);
	}

FUNCTIONEND:
	strcpy(errorString, resultString);

	return;
}

/*
 * Name			:	GetWrapperErrorCode
 * Description	:	This method returns the wrapper error code based on the SIO error code or slot error
 * Inputs		:	sioError	:	Sio lib error
 * 					slotError	: 	Sio slot error
 * Returns		:	Wrapper error code
 */
Status Common::GetWrapperErrorCode(TER_Status sioError,
		TER_Notifier slotError) {

	WriteToLog(LogTypeTrace, "Entry: %s, sioError = 0x%x, slotError = 0x%x \n",
			__FUNCTION__, sioError, slotError);

	Status error = success;

	if (sioError == TER_Status_none && slotError == TER_Notifier_Cleared) {
		goto FUNCTIONEND;
	}

	if (slotError != TER_Notifier_Cleared)
		error = terSlotError;
	else {
		switch (sioError) {
		case TER_Status_slotid_error:
			error = invalidSlotIndex;
			break;
		case TER_Status_dio_timeout:
			error = dioNotResponsive;
			break;
		case TER_Status_Error_Argument_Invalid:
			error = invalidArgumentError;
			break;
		case TER_Status_no_client_connection:
			error = sioNotConnected;
			break;
		case TER_Status_transmitter_is_halted:
			error = transmitHaltedError;
			break;
		case TER_Status_HTemp_Sled_Temperature_Fault:
		case TER_Status_HTemp_Max_Adjustment_Reached:
		case TER_Status_HTemp_Min_Adjustment_Reached:
			error = hTempError;
			break;
		case TER_Status_ucode_upgrade_busy:
		case TER_Status_ucode_file_too_big:
		case TER_Status_UpgradeInfoMissing:
		case TER_Status_high_ucode_ugrade_cnt:
			error = dioFirmwareUpdateError;
			break;
		case TER_Status_rpc_fail:
			error = rpcError;
			break;
		case TER_Status_Timeout:
			error = commandTimeoutError;
			break;
		default:
			error = terError;
			break;
		}
	}

	if ((sioErrorStatus == TER_Status_none) && (sioSlotError == TER_Notifier_Cleared)) {
		wrapperErrorCode = error;
		sioErrorStatus = sioError;
		sioSlotError = slotError;
	}

FUNCTIONEND:
	WriteToLog(LogTypeTrace, "Exit: %s, wrapperError = 0x%x \n", __FUNCTION__,
			error);

	return error;
}

/*
 * Name			:	SetWrapperErrorCode
 * Description	:	This method sets the last wrapper error code
 * Inputs		:	wrapperError : Wrapper error code
 * Returns		:	None
 */
void Common::SetWrapperErrorCode(Status wrapperError) {
	if ((wrapperError != success) &&
		(sioErrorStatus == TER_Status_none) &&
		(sioSlotError == TER_Notifier_Cleared)) {
		wrapperErrorCode = wrapperError;
	}
}

const char* Common::Sig2Str(const void* SIG, size_t LEN) {
	/* --- Working string (str) holds the value to be returned; its sizeof is
	 * great enough to hold "{" (opening brace), MFGSignatureLengthMAXIMUM
	 * substrings of ", 0xXX", "..." (ellipsis), "}" (closing brace) and a NUL
	 * terminator. */
	static char /*working String (returned)*/	str
			[MFGSignatureLengthMAXIMUM * /*, 0xXX*/ 6U + /*{...}NUL*/ 6U] = "";

	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	size_t /*Index*/      	ix;
	size_t /*loop Length*/	ln;

	/* --- Prepare local(s), C-style. */
	ix = 0;
	ln = MFGMin(LEN, MFGSignatureLengthMAXIMUM);

	/* --- Validate just signature (SIG); any length (LEN) is OK. */
	if (/*NULL?*/ !SIG) { /* NULL signature: return "NULL" */
		return "NULL";
	}

	/* --- Fill the working string (str). */
	(void)strcpy(str, "{");
	if (/*length OK?*/ LEN) {
		/* --- Append, first, by itself, SIG[0]. Then, in a loop, append the
		 * other, signature values, each one (1) preceded by a comma and a
		 * space (", "). The loop is designed such that, if there are no more
		 * than MFGSignatureLengthMAXIMUM Bytes, on loop-exit, index (ix) is
		 * equal to length (LEN). However, if there are more than
		 * MFGSignatureLengthMAXIMUM Bytes, index (ix) exits the loop with a
		 * value less than length (LEN); if so, append an ellipsis ("...") to
		 * note the unrepresented value(s). The working string (str) is big
		 * enough to hold everything. The indexing (6U * ix - 1U) into the
		 * working string (str) is such that the sprintf applies/appends onto
		 * the NUL at the end of the value of the working string (str) ...
		 * (have faith). The representation (0x%02X) is sufficient for any Byte
		 * value, domain: [0, 0xFF]. */
		(void)sprintf(str + 1U, "0x%02X", /*SIG[0]*/ *(unsigned char*)SIG);
		for (ix = 1; /*more?*/ ix < ln; ix++) { /* Append , and SIG[ix]. */
			(void)sprintf(str + 6U * ix - 1U, ", 0x%02X",
					*((unsigned char*)SIG + ix));
		}
		if (/*too big?*/ ix < LEN) { /* Append an ellipsis (...). */
			(void)strcat(str, "...");
		}
	}
	(void)strcat(str, "}");
	assert(sizeof(str) > strlen(str));
	return str;
}

const Byte Common::Str2Sig(Byte sig[MFGSignatureLengthMAXIMUM], const char STR[]) {
	static const char /*Hexadecimal Digits*/	HexDGT[] = "0123456789ABCDEF";

	/* --- Locals are declared C-style rather than C++ style: initial values
	 * are assigned after declaration; all locals are prepared, even if
	 * those assignments are overset, subsequently. */
	char* /*Character*/	ch;
	int /*Digit Count*/	dC;
	int /*Index*/      	ix;
	int /*Length*/     	ln;
	int /*Value*/      	vu;

	/* --- Prepare local(s), C-style. */
	ch = /*NULL*/ (char*)0;
	dC = ix = ln = vu = 0;

	/* --- Validate the parameters. */
	assert(!ln && !vu);
	if (/*OK?*/ sig && STR) {
		/* --- Process... */
		assert(!ln && !vu);
		for (ch = (char*)STR;
				/*more?*/ *ch &&
					(/*room?*/ ln < (int)MFGSignatureLengthMAXIMUM);
				ch++) {
			/* --- Examine the character. */
			for (ix = 0; /*more?*/ ix < (int)(sizeof(HexDGT) - 1); ix++) {
				/* --- If this character is a hexadecimal digit, process it. */
				if (/*digit?*/ toupper(*ch) == HexDGT[ix]) {
					/* --- Hexadecimal digit... */
					assert((ix >= 0) && (ix <= 15));
					if (/*leading?*/ !dC) {
						/* --- Leading digit... */
						vu = ix;
						dC++;
					} else	{
						/* --- Trailing digit... */
						vu = (vu << 4) + ix;
						assert((vu >= 0) && (vu <= 255));
						sig[ln] = (Byte)vu;
						ln++;
						dC = 0;
					}
				}
			}
		}
	}
	if (/*pending?*/ dC && (/*room?*/ ln < (int)MFGSignatureLengthMAXIMUM)) {
		/* --- Apply the pending value as the last Byte. */
		assert((vu >= 0) && (vu <= 15));
		sig[ln] = (Byte)vu;
		ln++;
	}
	assert((ln >= 0) && (ln <= (int)MFGSignatureLengthMAXIMUM) && (ln <= 255));
	return (Byte)ln;
}
