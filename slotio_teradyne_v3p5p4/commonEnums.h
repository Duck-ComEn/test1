#ifndef _COMMON_ENUMS_H_
#define _COMMON_ENUMS_H_

//////////////////////////////// MACROS ////////////////////////////////////
#define UART_PULLUP_VOLTAGE_DEFAULT 					2500
#define V5_LIMIT_IN_MILLI_AMPS 							3000
#define V5_LOWER_LIMIT_MV 								4500
#define V5_UPPER_LIMIT_MV 								5500
#define V5_OVER_VOLTAGE_PROT_LIMIT_MV 					5000
#define V12_LIMIT_IN_MILLI_AMPS 						3000
#define V12_LOWER_LIMIT_MV 								10800
#define V12_UPPER_LIMIT_MV 								13200
#define V12_OVER_VOLTAGE_PROT_LIMIT_MV					12000


#define UART2_TX_HALT_ON_ERROR_MASK						(1 << 18)
#define UART2_ACK_TIMEOUT_MASK							(1 << 11)
#define LOG_LEVEL_MIN									0
#define LOG_LEVEL_MAX									4
#define SLOT_INDEX_MIN									1
#define SLOT_INDEX_MAX									140
#define JOURNAL_LINE_MAX								100
#define SEND_RECEIVE_BUFFER_LINE_MAX					128
#define UART3_REQUEST_RESPONSE_HEADER_LENGTH			16
#define UART3_RECEIVE_ACK_LENGTH						2
#define MAX_UART3_FLAG_BYTE								0x1

#define MAX_ERROR_STRING_LENGTH							2048

#define MINCDBLENGTH									1	/* minimum CDB size */
#define MAXCDB									32	/* maximum CDB size (must be 4bytes boundary) */
#define MAXCDBLENGTH									32	/* maximum CDB size (must be 4bytes boundary) */
#define MAXSENSELENGTH									32	/* max sense data size (must be 4bytes boundary) */
#define MAXSCSBLENGTH									512 /* max SCSB data size (must be 4bytes boundary) */
#define MAXSCSB									512 /* max SCSB data size (must be 4bytes boundary) */
#define MAXSENSE_D      252           /* max sense data size (must be 4bytes boundary) for Descriptor Format  */
#define MIN(a,b) ((a)<(b))?(a):(b);
//#define UART3_MAX_CHUNK_SIZE 							(64 * 1024) /* Maximum data chunk size to request from drive at a time */
#define UART3_MAX_CHUNK_SIZE 							(16 * 1024) /* Maximum data chunk size to request from drive at a time */
#define MAX_UART3_SCSI_DATA_LENGTH						(2 * 1024 * 1024)
#define MAX_DATA_BUFFER_LENGTH							65536

#define GET_DIAG_INFO_LEN_64	65

#define WAPI_RPC_LAST_ATTEMPT 2
#define WAPI_CALL_RPC_WITh_RETRY(SIO_API)  	SIO_API;             \
 	if ((status == TER_Status_rpc_fail) || (status == TER_Status_Timeout)) {                         \
       Common::Instance.WriteToLog(LogTypeError, "ERROR: TER_Status_rpc_fail. Trying to reconnect.. \n"); \
	   for(int cnt = 1; cnt <= WAPI_RPC_LAST_ATTEMPT; ++cnt) {   \
    	   Byte rc = Common::Instance.TerSioReconnect();                  \
		   if( rc != success ) {                                 \
	    	   Common::Instance.WriteToLog(LogTypeError, "ERROR: Could not reconnect to socket \n"); \
	    	   return(terError); \
           }                                          \
	       SIO_API;                                   \
		   if( (status != TER_Status_rpc_fail) &&     \
			   (status != TER_Status_Timeout)) break; \
	   } \
	}


#define RET(x) Common::Instance.SetWrapperErrorCode((Status)x); \
		return x;
///////////////////////////////////////////////////////////////////////////

// The type of log message that is being written to the log file
enum LogType {
	LogTypeError = 1,
	LogTypeTrace = 2,
	LogTypeJournal = 3
};

// UART2 protocol type. This is used for endian swapping.
enum Uart2ProtocolType {
	Uart2ProtocolTypeA00 = 0,
	Uart2ProtocolTypeARM = 1
};

// Error Status
enum Status {
	success 										= 0x00,

	// Teradyne Internal Error Codes
	terError 										= 0x10,
	invalidSlotIndex								= 0x11, // TER_Status_slotid_error
	driveAckTimeout 								= 0x12,	// Drive did not return acknowledgment on time
    sioNotReachable									= 0x13, // Either SIO is down or network error
	sioSocketError									= 0x14, // SIO is not accepting Socket connection
	dioNotResponsive								= 0x15, // TER_Status_dio_timeout
	dioInvalidTPN									= 0x16, // DIO is Bootloader, not running App f/w
	dioFirmwareUpdateError							= 0x17, // DIO f/w update failed
	nullArgumentError								= 0x18,
	invalidArgumentError							= 0x19, // TER_Status_Error_Argument_Invalid
	uart3DriveBusyWithDataReceived					= 0x1A,	// Drive is busy with the data that was sent to it
	checksumMismatchError							= 0x1B,
	invalidRespSyncPacket							= 0x1C,	// Invalid sync word received from the drive
	opcodeMismtach									= 0x1D,	// Request and Response opcode don't match
	uart3DataSequenceMismatch						= 0x1E,
	sioNotConnected									= 0x1F, // TER_Status_no_client_connection
	invalidAck										= 0x20,
	transmitHaltedError								= 0x21, // TER_Status_transmitter_is_halted
	hTempError										= 0x22, // TER_Status_HTemp_Sled_Temperature_Fault
															// TER_Status_HTemp_Max_Adjustment_Reached
															// TER_Status_HTemp_Min_Adjustment_Reached
	terSlotError									= 0x23, // TER_Notifier faults
	rpcError										= 0x24, // TER_Status_rpc_fail
	outOfRange										= 0x25, // Values are out of the limits
	respPacketLengthMismatch						= 0x26, // Response packet length doesn't match the expected length
	uart2InvalidNumOfAcks							= 0x27, // Received invalid number of acks from drive (UART 2 only)
	invalidDataLengthError							= 0x28,
	commandTimeoutError								= 0x29, // Timed out while executing a command

	maxTerError										= 0x6F,
	// UART2 Drive Acknowledgments
	uart2AckSuccess 								= 0x80,
	uart2AckReceiverOverrun 						= 0x81,
	uart2AckDataUnstable 							= 0x82,
	uart2AckFrameError 								= 0x84,
	uart2AckInvalidSyncPacket 						= 0x88,
	uart2AckInvalidLengthPacket 					= 0x90,
	uart2AckCommandChecksumError 					= 0xA0,
	uart2AckFlowControlError 						= 0xA1,
	uart2AckInvalidState 							= 0xC0,
	uart2AckCommandReject 							= 0xC1,

	// UART3 Drive Acknowledgments
	uart3AckDataAvailable_DriveRetainsControl 		= 0x7D, // Host Response : Receive Data
	uart3AckReady 									= 0x80, // Host Response : None - Transmit
	uart3AckInvalidSyncWord 						= 0x88, // Host Response : Retransmit
	uart3AckChecksumError 							= 0xA0, // Host Response : Retransmit
	uart3AckFlowControlError 						= 0xA1, // Host Response : Retransmit
	uart3AckInvalidState 							= 0xC0, // Host Response : Restart Sequence
	uart3AckCommandReject 							= 0xC1, // Host Response : Rebuild Command*
	uart3AckReservationConflict 					= 0xE0,
	uart3AckQueueFull 								= 0xE1,
	uart3AckResourceBusy 							= 0xE2,
	uart3AckSCSICommandComplete 					= 0xF0,
	uart3AckCommandParameterError 					= 0xFA,
	uart3AckBusy 									= 0xFB, // Host Response : Retry (delay?)
	uart3AckDataRequested 							= 0xFC, // Host Response : Write Data
	uart3AckDataAvailable 							= 0xFD, // Host Response : Read Data
	uart3AckSenseAvailable 							= 0xFE, // Host Response : Read Sense
	uart3AckTMFComplete 							= 0xFF,
};

enum Uart3ReqRespOpcode {
	uart3OpcodeSCSICmd = 0x40,
	uart3OpcodeQueryCmd = 0x41,
	uart3OpcodeReadDataCmd = 0x42,
	uart3OpcodeWriteDataCmd = 0x43,
	uart3OpcodeAbortSCSICmd = 0x44,
	uart3OpcodeGetUARTParametersCmd = 0x45,
	uart3OpcodeSetUARTParametersCmd = 0x46,
	uart3OpcodeSetUART2ModeCmd = 0x47,
	uart3OpcodeMemoryReadCmd = 0x48,
	uart3OpcodeMemoryWriteCmd = 0x49,
	uart3OpcodeEchoCmd = 0x4A,
	uart3OpcodeGetDriveStateCmd = 0x4E,
	uart3OpcodeDriveResetCmd = 0x4F
};

enum Uart3ReqRespHeaderIndex {
	bUart3SyncWordH = 0,
	bUart3SyncWordL = 1,
	bUart3ReqRespOpcode = 2,
	bUart3FlagByte = 3,
	bUart3DataLengthH = 4,
	bUart3DataLengthL = 5,
	bUart3DataSequenceH = 6,
	bUart3DataSequenceL = 7,
	bUart3DataChecksumHH = 8,
	bUart3DataChecksumHL = 9,
	bUart3DataChecksumLH = 10,
	bUart3DataChecksumLL = 11,
	bUart3ReserverdH = 12,
	bUart3ReservedL = 13,
	bUart3HeaderChecksumH = 14,
	bUart3HeaderChecksumL = 15
};

//typedef struct GetSetUart3Param {
//	// UART protocol version - Read only
//	Byte bUartProtocolVersionH; // always 0
//	Byte bUartProtocolVersionL; // 4 or 5
//
//	// UART Line Speed - Read/Write
//	Byte bUartLineSpeedH;
//	Byte bUartLineSpeedL;
//
//	// UART Ack Delay Time in microseconds - Read/Write
//	Byte bAckDelayTimeInMicroSecondsH;
//	Byte bAckDelayTimeInMicroSecondsL;
//
//	// Max Write Block Size - Read only
//	Byte bMaxBlockSizeForWriteHH;
//	Byte bMaxBlockSizeForWriteHL;
//	Byte bMaxBlockSizeForWriteLH;
//	Byte bMaxBlockSizeForWriteLL;
//
//	// Max Read Block Size - Read/Write
//	Byte bMaxBlockSizeForReadHH;
//	Byte bMaxBlockSizeForReadHL;
//	Byte bMaxBlockSizeForReadLH;
//	Byte bMaxBlockSizeForReadLL;
//
//	// DMA Timeout in milliseconds - Read/Write
//	Byte bDMAtimeoutInMilliSecondsH;
//	Byte bDMAtimeoutInMilliSecondsL;
//
//	// FIFO Timeout in milliseconds - Read/Write
//	Byte bFIFOtimeoutInMilliSecondsH;
//	Byte bFIFOtimeoutInMilliSecondsL;
//
//	// Transmit Segment Size - Read/Write
//	Byte bTransmitSegmentSizeHH;
//	Byte bTransmitSegmentSizeHL;
//	Byte bTransmitSegmentSizeLH;
//	Byte bTransmitSegmentSizeLL;
//
	// Inter-segment Delay in microseconds - Read/Write
//	Byte bInterSegmentDelayInMicroSecondsH;
//	Byte bInterSegmentDelayInMicroSecondsL;
//} GETSETU3PARAM;

enum Uart3LineSpeedCoding {
	lineSpeed115200		= 0x0000,
	lineSpeed76800 		= 0x0001,
	lineSpeed57600		= 0x0002,
	lineSpeed38400		= 0x0003,
	lineSpeed28800		= 0x0004,
	lineSpeed19200		= 0x0005,
	lineSpeed14400		= 0x0006,
	lineSpeed9600		= 0x0007,
	lineSpeed1843200	= 0x0008,
	lineSpeed1228800	= 0x0009,
	lineSpeed921600		= 0x000A,
	lineSpeed614400		= 0x000B,
	lineSpeed460800		= 0x000C,
	lineSpeed307200		= 0x000D,
	lineSpeed230400		= 0x000E,
	lineSpeed153600		= 0x000F,
	lineSpeed2083000	= 0x0010,
	lineSpeed2380000	= 0x0011,
	lineSpeed2777000	= 0x0012,
	lineSpeed3333000	= 0x0013,
	lineSpeed4166000	= 0x0014,
	lineSpeed5555000	= 0x0015,
	lineSpeed8333000	= 0x0016,
	lineSpeed11111000	= 0x0017,
	lineSpeed16666000	= 0x0018,
};

//typedef struct GetDriveStateData {
//	Word wDriveStateVersion;
//	Word wOperatingState;
//	Word wFunctionalMode;
//	Word wDegradedReason;
//	Word wBrokenReason;
//	Word wResetCause;
//	Word wShowstopState;
//	Word wShowstopReason;
//	Dword dwShowstopValue;
//}GET_DRIVE_STATE_DATA;

/* CDB and sense data for CCB special command */
//typedef struct {
//  Byte cdblength;        		/* CDB length */
//  Word senselength;      		/* sense data length */
//  Dword pad1;
//  Byte cdb[MAXCDBLENGTH];     	/* CDB block */
//  Dword pad2;
//  Byte sense[MAXSENSELENGTH]; 	/* sense data block */
//  Dword read;             		/* read data length */
//  Dword write;            		/* write data length */
//  long timeout;          		/* timeout [msec] */
//} CCB;

//enum CCB_CMDTYPE{
//  withoutDataTransfer = 0,
//  writeDataTransfer   = 1,
//  readDataTransfer    = 2,
//  NOT_DEFINED         = 9,
//};

/* CDB, sense data, SCSB for CCB special command */
//typedef struct {
//  unsigned long cdblength;				/* CDB length */
//  unsigned long senselength;			/* sense data length */
//  unsigned long scsblength;				/* SCSB data length */
//  unsigned char cdb[MAXCDBLENGTH];		/* CDB block (4byte boundary)*/
//  unsigned char sense[MAXSENSELENGTH];	/* sense data block */
//  unsigned char scsb[MAXSCSBLENGTH];	/* Special Command Status Block */
//  unsigned long forcescsbreport;		/* 1: always report SCSB, 0: report SCSB only error, scsblength > 0 */
//  unsigned long read;					/* read data length */
//  unsigned long write;					/* write data length */
//  unsigned long bytesTransferred;	/* number of bytes transferred during read or write operation */
//  unsigned long timeout;				/* timeout [msec] */
//  Byte          bCellNo;
//} CCB_DEBUG;

//typedef struct {
//	char eventCode;
//	char eventCodeDescription [GET_DIAG_INFO_LEN_64];
//	unsigned char errorCode;
//	int bytesToTransfer;
//	int transferredBytes;
//	long int tv_start_sec;
//	long int tv_start_usec;
//	long int tv_end_sec;
//	long int tv_end_usec;
//} UartEventTrace;

typedef struct {
	Byte event_code;// 0x00=no event, 0x10=send, 0x20=receive, 0x21=delay receive (*)
//- (*) Difference between 0x20=receive, 0x21=delay receive : If some data is already available in receive FIFO when receive API called, event code is 0x20. If no data is available when receive API called and firmware wait for data receiving, event code is 0x21
        Byte event_code_description[65]; // (optional) A NULL terminated string containing the name of the operation performed. Current possible values are: "Read","Write","Delayed Read"
	Byte error_code;// (optional) 0=success, 1=overrun error, 2=framing error, 3=timeout, 4=not supported
	Word num_bytes;// number of transmit or receive bytes (actual)
        Dword num_bytes_requested; // (optional) number of transmit or receive bytes (requested)
	Dword tv_start_sec; // timestamp at event start (whole second : unit in second)
	Dword tv_start_usec; // timestamp at event start (fraction of second : unit in microsecond)
	Dword tv_end_sec;// timestamp at event end (whole second : unit in second)
	Dword tv_end_usec;// timestamp at event end (fraction of second : unit in microsecond)
} UART_EVENT;







/** @details
 * Delay Limit is the specified maximum duration (ms) between power
 * application and the end of the impulse stimulus...
 */
#define MFGDelayLIMIT 140

/** @details
 * Duration Limit is the specified maximum duration (ms) between power
 * application and the end of the response acquisition...
 */
#define MFGDurationLIMIT 300

/** @details
 * Impulse Code Default is the impulse code's default value...
 */
#define MFGImpulseCodeDEFAULT /*'P'*/ 0x50

/** @details
 * Impulse Code Maximum is the impulse code's maximum, allowed value...
 */
#define MFGImpulseCodeMAXIMUM 0xFF

/** @details
 * Impulse Code Minimum is the impulse code's minimum, allowed value...
 */
#define MFGImpulseCodeMINIMUM 0

/** @details
 * Impulse Count Default is the default value (count) for an impulse count...
 */
#define MFGImpulseCountDEFAULT 50

/** @details
 * Impulse Count Maximum is the maximum value (count) allowed for an impulse
 * count...
 */
#define MFGImpulseCountMAXIMUM 255

/** @details
 * Impulse Count Minimum is the minimum value (count) allowed for an impulse
 * count...
 */
#define MFGImpulseCountMINIMUM 1

/** @details
 * Inter character Delay Default is the default value (bits) for an
 * inter character delay: the number of additional stop bits that succeed the
 * required, one (1) stop bit...
 */
#define MFGIntercharacterDelayDEFAULT 20

/** @details
 * Inter character Delay Maximum is the maximum value (bits) allowed for an
 * inter character delay: the number of additional stop bits that succeed the
 * required, one (1) stop bit...
 */
#define MFGIntercharacterDelayMAXIMUM 31

/** @details
 * Inter character Delay Minimum is the minimum value (bits) allowed for an
 * inter character delay: the number of additional stop bits that succeed the
 * required, one (1) stop bit...
 */
#define MFGIntercharacterDelayMINIMUM 0

/** @details
 * Power Delay Default is the default value (ms) for a delay period...
 */
#define MFGPowerDelayDEFAULT 80

/** @details
 * Power Delay Maximum is the maximum value (ms) allowed for a delay period...
 */
#define MFGPowerDelayMAXIMUM 100

/** @details
 * Power Delay Minimum is the minimum value (ms) allowed for a delay period...
 */
#define MFGPowerDelayMINIMUM 5

/** @details
 * Signature Default is the default signature... Signatures are stored as Byte
 * (unsigned char) vectors; unlike strings, there is no terminating NUL byte.
 * For convenience (and because {0x4D, 0x46, 0x47} does not compile as
 * expected), the default value is defined, here, as a signed char vector
 * (string, terminated by a NUL): DO NOT CODE sizeof(MFGSignatureDEFAULT) as it
 * will resolve to four (4), rather than the expected three (3) bytes! Use,
 * instead, MFGSignatureLengthDEFAULT.
 */
#define MFGSignatureDEFAULT "MFG"

/** @details
 * Signature Length Default is the length of the default signature
 * (@ref MFGSignatureDEFAULT) and the default length for all of the API's
 * signatures... Note that coding sizeof(MFGSignatureDEFAULT) will not form the
 * proper value because {0x4DU, 0x46U, 0x47U} is compiled as three (3) int
 * values rather than three (3) Byte (unsigned char) values.
 */
#define MFGSignatureLengthDEFAULT 3

/** @details
 * Signature Length Maximum is the maximum length (sizeof) for a signature...
 */
#define MFGSignatureLengthMAXIMUM 255

/** @details
 * Signature Length Minimum is the minimum length (sizeof) for a signature...
 */
#define MFGSignatureLengthMINIMUM 1

#endif
