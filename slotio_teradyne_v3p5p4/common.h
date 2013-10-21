/*
 * This file defines all the common enums, macros, methods
 * which will be used across the whole project
 */

#ifndef _COMMON_H_
#define _COMMON_H_


#include "tcTypes.h"
#include "commonEnums.h"
#include "terSioLib.h"
#include "terSioCommon.h"

class Common
{
private:
//	FILE* logFile;
	int slotIndex;
	int logLevel;
	bool sioConnected;
	Uart2ProtocolType uart2Protocol;
	char logFileName[256];
	bool resetSlotDuringInitialize;
	TerSioLib *sioLib;
	Status wrapperErrorCode;
	TER_Status sioErrorStatus;
	TER_Notifier sioSlotError;

	void OpenLogFile();
	Common();

public:
	static Common Instance;
	~Common();

	/*
	 * UART2 Support Methods
	 */
	Word ComputeUart2Checksum(Byte *buffer, Word len);
	Byte ValidateUart2ReceivedData(Byte *sendRecvBuf, Word cmdLength, unsigned int respLength,
	                                 unsigned int expLength, Word *pBuffer, unsigned int status1);


	/*
	 * UART3 Support methods
	 */
	Byte Uart3LinkControl(Byte bCellNo, Byte bReqRespOpcode, Byte bFlagByte, Word *wPtrDataLength, Word wDataSequence, Word wReserved, Byte *DataBuffer, Dword Timeout_mS);
	Word ComputeUart3Checksum16Bits(Byte *DataBuffer, Word DataLength);
	Dword ComputeUart3Checksum32Bits(Byte *DataBuffer, Word DataLength);

	/*
	 * Wrapper Support Methods
	 */
	Byte SanityCheck(Byte);
	void SetSlotIndex(int index);
	Byte TerSioConnect(void);
	Byte TerSioDisconnect(void);
	Byte TerSioReconnect(void);
	void WriteToLog(LogType logType, const char* msg, ...);
	void SetUart2Protocol(Uart2ProtocolType protocol);
	void SetLogLevel(int level);
	void LogError(const char *funcName, Byte bCellNo, TER_Status status);
	void LogSendReceiveBuffer(char *txt, Byte *buf, Word len);
	int GetLogLevel();
	void SetWapitestResetSlotDuringInitialize(unsigned int resetEnable);
	unsigned int GetWapitestResetSlotDuringInitialize();
	TerSioLib* GetSIOHandle();
	void TerFlushReceiveBuffer(Byte bCellNo);
	Byte TerEnableFpgaUart2Mode(Byte bCellNo, unsigned int enable);
	Dword GetElapsedTimeInMillSec(struct timeval startTime, struct timeval endTime);
	Dword GetElapsedTimeInMicroSec(struct timeval startTime, struct timeval endTime);
	void GetErrorString(Byte bCellNo, char* errorString, Byte* errorCode);
	Status GetWrapperErrorCode(TER_Status sioError, TER_Notifier slotError);
	void SetWrapperErrorCode(Status wrapperError);

	/** @details
	 * Signature to String forms a string representation of a signature...
	 * @param [in] SIG (Signature) points to the first Byte of the signature...
	 * 	If this argument is @c (Byte*)0 @c (NULL), the special string result
	 * 	@c "NULL" is returned.
	 * @param [in] LEN (Length) specifies the signature's length (in Bytes) and
	 * 	should be in the domain [0, @ref MFGSignatureLengthMAXIMUM] (declared in
	 * 	@ref commonEnums.h). Zero (0) produces the special string result:
	 * 	@c "{}". Any value greater than @ref MFGSignatureLengthMAXIMUM is treated
	 * 	as if it were @ref MFGSignatureLengthMAXIMUM but the additional, @c Byte,
	 * 	signature values are shown as a single ellipsis @c "..."; for example,
	 * 	@c {0x12, 0x4F, 0x67...}
	 * @returns string representation of signature...
	 */
	static const char* Sig2Str(const void* SIG, size_t LEN);

	/** @details
	 * String to Signature forms a signature from a string...
	 * @param [out] sig (Signature) points to the first @c Byte of a
	 * 	@ref MFGSignatureLengthMAXIMUM @c Byte vector that is to be filled with the
	 * 	decoded signature...
	 * @param [in] STR (String) points to the string that is to be decoded into
	 * 	a signature... The signature values are pairs of hexadecimal digits
	 * 	(@c '0', @c '2', ... @c '9', @c 'a' or @c 'A', @c 'b' or @c 'B', ...
	 * 	@c 'f' or @c 'F'). All other characters are ignored.
	 * @returns length of the new signature...
	 * @note- This method converts any and all hexadecimal digits! Thus, @c 0x12 is
	 * 	treated as if it were 012 (the @c '0' is converted, as well).
	 */
	static const Byte Str2Sig(Byte sig[MFGSignatureLengthMAXIMUM],
			const char STR[]);
};

#endif
