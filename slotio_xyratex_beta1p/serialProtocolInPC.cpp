//============================================================================================
// serialProtocolInPC.cpp
//
//  HGST SRST serial protocol - IMPELENTED IN THE PC - using 'simple' raw serial APIs to Cell/Drive
//
//============================================================================================

#include "libsi.h"
#include "libsi_xyexterns.h" // // for lib's externs of Tracing flags, pstats etc

#include "siMain.h"
 
#include "serialProtocolInPC.h" 
 
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define MIN(a,b) (a<b)?a:b;

 
Byte ProtocolType;  // now has own API to set  // 0=old, 1=new(er)

// parametrics

struct _protocol_stats pstats[255]; // global, to allow read-access from TestCase (indexed by bCellNo)


static uint8 u8PacketSize = 2;
static uint8 u8MaxRetransmitsPerPacket = 5;
static uint16 u16Timeout_PktSendToAckReceipt_mS = 100;  

// this is now a parameter...
//static uint16 u16Timeout_CmdToCompleteReponse_mS = 2550; // (255*10mS is max supported by ReceiveWithWait API)  // 3000;  // 1000 was only good enough for dataread sizes up to about 30K bytes)

// This one is interesting - seems we had ~400uS to allow for drive to float line after sending
//  - but For Protcol-in-PC, this should SHOULD not be anything we need to worry about  (USB/8051 latency is  ~ 3mS anyway)
//uint8 u8Timer_AckReceiptToTurnaround = 5;   // wait for drive to stop driving line after sending its ack/nak ~500uS  85.333uS per count (allow for +- 1 each end)

#define ACK 0x80
#define RETRYABLE_NAK_RANGE_START 0x81
#define RETRYABLE_NAK_RANGE_END   0x88

//[DEBUG] add this for uart debug. it is minimum modification from SerialSimpleReadMemory()
//[DEBUG] to support write memory protocol.
//[DEBUG] it allocates its own memory for receive buffer.
//-----------------------------------------------------------------------------------------------
Byte SerialSimpleWriteMemoryDEBUG( Byte bCellNo, uint8 ChannelId, uint32 u32StartAddress, uint16 u16nBytes, Byte * u8UserDataBuffer, uint16 Timeout_mS )
//-----------------------------------------------------------------------------------------------
{   
#define MAX_WRITE_LENGTH     (10)
  // Threre are only 1 byte write operation available for STN,PTB 
  // New protocol generation drives (JPT, EB7, etc) allows more 2 bytes write (status and control), 
  // and 6 bytes writes (grading Mfg. Id). 10 Bytes is new limit.
	if (u16nBytes > MAX_WRITE_LENGTH) {
		return 1;
	}

	Byte rc = 0;  
	static int nConsecutiveErrors = 0;
	
	#define RESPONSE_PKT_OVERHEAD   (6 + 2) // Relates to CUSTOMER overhead !!!!! (NOT any Xyratex bulk-transport overhead - which is 1+1+2) !!!!
	
	#define MAX_RESPONSE_DATA_LENGTH  (0xffff - RESPONSE_PKT_OVERHEAD) // max for uint16 params!!
	
	uint8 ResponseBuffer[MAX_RESPONSE_DATA_LENGTH + RESPONSE_PKT_OVERHEAD];  // max possible size 
	
									
	// set-up the command structure
	uint8 ReadCmd[] = { 0xAA,0x55,
												6,6,         // command length in packets (repeated)
												0,2,         // command opcode
												0x00,0x00,   // address (32-bit)  (placeholder)
												0x00,0x00,   // address (32-bit)  (placeholder) 
												0x00,0x00,   // data length (16-bit)  (placeholder)
												0x00,0x00,   // data value (16-bit)  (placeholder)
												0x0D,0x00 }; // checksum  (16-bit) (placeholder) 

	
	ReadCmd[6] = (uint8) (u32StartAddress >> 24); // hi-byte                        
	ReadCmd[7] = (uint8) (u32StartAddress >> 16);
	ReadCmd[8] = (uint8) (u32StartAddress >> 8);
	ReadCmd[9] = (uint8)  u32StartAddress; // lo-byte

							
	ReadCmd[10] = u16nBytes / 256; // hi-byte                        
	ReadCmd[11] = (uint8) u16nBytes; // lo-byte
													
	ReadCmd[12] = 0;
	ReadCmd[13] = u8UserDataBuffer[0];

	CalculateCmdChecksum( ReadCmd, sizeof(ReadCmd) );
	
	
	// set-up expected response size
	uint16 u16ResponseLength =  RESPONSE_PKT_OVERHEAD;

#if 0	// not applicalbe for write operation, because response block lenght is fixed.
	if (ProtocolType==1)
	{
		// new protocol drives will always return an EVEN numbers of bytes in response (rounded up),  
		// so, if request is odd, then add one to expected ResponseLength 
		if ((u16nBytes & 0x0001) == 0x0001)
		{
				LOG_TRACE "SerialSimpleReadMemory: NEW protocol: requested u16nBytes(%u) is ODD - so adding one to expected u16ResponseLength", u16nBytes);
				u16ResponseLength++; // make even
		}
	} 
#endif

	
	// Send Command And Receive Response
	protocol_rc prc = SendCmdAndReceiveResponse( bCellNo, ChannelId, ReadCmd, sizeof(ReadCmd), ResponseBuffer, &u16ResponseLength, Timeout_mS );

	
	if (prc == prcOK)
	{
		// validate checksum
		if ( !ValidateChecksum( ResponseBuffer, u16ResponseLength))
		{
			// (failing ValidateChecksum logs its own error message (with details))
			nConsecutiveErrors++;  
			rc = 1;  
		} 
		else
		{
			memcpy(&u8UserDataBuffer[0], &ResponseBuffer[RESPONSE_PKT_OVERHEAD - 2], u16nBytes);
			nConsecutiveErrors = 0; 
		} 
	}
	else
	{  
		// reasons will have been displayed by SendCmdAndReceiveResponse
		nConsecutiveErrors++;  
		rc = 2;  
	}                        
	
	#define EXCESSIVE_CONSECUTIVE_ERRORS   50  // totally arbitrary
	if (nConsecutiveErrors >= EXCESSIVE_CONSECUTIVE_ERRORS)
	{
		LOG_WARNING "EXCESSIVE CONSECUTIVE Cmd/Response errors (>%d)", EXCESSIVE_CONSECUTIVE_ERRORS);
		LOG_WARNING "To give an idea at what stage in the test this happened:\n");
		LOG_WARNING "  TotalCommands=%u TotalDataBytesReceived=%.0f, TotalTimeTransferring_mS=%.0f", pstats[bCellNo].TotalCommands, pstats[bCellNo].TotalDataBytesReceived, pstats[bCellNo].TotalTimeTransferring_mS);
	}


	pstats[bCellNo].TotalCommands++;
	
	return rc;
}

//[DEBUG] add this for uart debug. it is minimum modification from SerialSimpleReadMemory()
//[DEBUG] to support echo protocol.
//[DEBUG] it allocates its own memory for receive buffer.
//[DEBUG] it copies data part to u8UserDataBuffer.
//-----------------------------------------------------------------------------------------------
Byte SerialSimpleEchoDEBUG( Byte bCellNo, uint8 ChannelId, uint16 u16nBytes, Byte * u8UserDataBuffer, uint16 Timeout_mS )
//-----------------------------------------------------------------------------------------------
{   
	Byte rc = 0;  
	static int nConsecutiveErrors = 0;
	
	#define RESPONSE_PKT_OVERHEAD   (6 + 2) // Relates to CUSTOMER overhead !!!!! (NOT any Xyratex bulk-transport overhead - which is 1+1+2) !!!!
	
	#define MAX_RESPONSE_DATA_LENGTH  (0xffff - RESPONSE_PKT_OVERHEAD) // max for uint16 params!!
	
	uint8 ResponseBuffer[MAX_RESPONSE_DATA_LENGTH + RESPONSE_PKT_OVERHEAD];  // max possible size 
	
									
	// set-up the command structure
	uint8 ReadCmd[] = { 0xAA,0x55,
												4,4,         // command length in packets (repeated)
												0,0,         // command opcode
												0,0,         // command flag
												0x00,0x00,   // data length (16-bit)  (placeholder)
												0x0D,0x00 }; // checksum  (16-bit) (placeholder) 
	
	ReadCmd[8] = u16nBytes / 256; // hi-byte                        
	ReadCmd[9] = (uint8) u16nBytes; // lo-byte
													
	CalculateCmdChecksum( ReadCmd, sizeof(ReadCmd) );
	
	
	// set-up expected response size
	uint16 u16ResponseLength =  u16nBytes + RESPONSE_PKT_OVERHEAD;
	 
	if (ProtocolType==1)
	{
		// new protocol drives will always return an EVEN numbers of bytes in response (rounded up),  
		// so, if request is odd, then add one to expected ResponseLength 
		if ((u16nBytes & 0x0001) == 0x0001)
		{
				LOG_TRACE "SerialSimpleReadMemory: NEW protocol: requested u16nBytes(%u) is ODD - so adding one to expected u16ResponseLength", u16nBytes);
				u16ResponseLength++; // make even
		}
	} 

	
	// Send Command And Receive Response
	protocol_rc prc = SendCmdAndReceiveResponse( bCellNo, ChannelId, ReadCmd, sizeof(ReadCmd), ResponseBuffer, &u16ResponseLength, Timeout_mS );

	
	if (prc == prcOK)
	{
		// validate checksum
		if ( !ValidateChecksum( ResponseBuffer, u16ResponseLength))
		{
			// (failing ValidateChecksum logs its own error message (with details))
			nConsecutiveErrors++;  
			rc = 1;  
		} 
		else
		{
			memcpy(&u8UserDataBuffer[0], &ResponseBuffer[RESPONSE_PKT_OVERHEAD - 2], u16nBytes);
			nConsecutiveErrors = 0; 
		} 
	}
	else
	{  
		// reasons will have been displayed by SendCmdAndReceiveResponse
		nConsecutiveErrors++;  
		rc = 2;  
	}                        
	
	#define EXCESSIVE_CONSECUTIVE_ERRORS   50  // totally arbitrary
	if (nConsecutiveErrors >= EXCESSIVE_CONSECUTIVE_ERRORS)
	{
		LOG_WARNING "EXCESSIVE CONSECUTIVE Cmd/Response errors (>%d)", EXCESSIVE_CONSECUTIVE_ERRORS);
		LOG_WARNING "To give an idea at what stage in the test this happened:\n");
		LOG_WARNING "  TotalCommands=%u TotalDataBytesReceived=%.0f, TotalTimeTransferring_mS=%.0f", pstats[bCellNo].TotalCommands, pstats[bCellNo].TotalDataBytesReceived, pstats[bCellNo].TotalTimeTransferring_mS);
	}


	pstats[bCellNo].TotalCommands++;
	
	return rc;
}

//[DEBUG] add this for uart debug. it is minimum modification from SerialSimpleReadMemory()
//[DEBUG] to support read protocol.
//[DEBUG] it allocates its own memory for receive buffer.
//[DEBUG] it copies data part to u8UserDataBuffer.
//-----------------------------------------------------------------------------------------------
Byte SerialSimpleReadMemoryDEBUG( Byte bCellNo, uint8 ChannelId, uint32 u32StartAddress, uint16 u16nBytes, Byte * u8UserDataBuffer, uint16 Timeout_mS )
//-----------------------------------------------------------------------------------------------
{   
  Byte rc = 0;  
  static int nConsecutiveErrors = 0;
	
#define RESPONSE_PKT_OVERHEAD   (6 + 2) // Relates to CUSTOMER overhead !!!!! (NOT any Xyratex bulk-transport overhead - which is 1+1+2) !!!!
	
#define MAX_RESPONSE_DATA_LENGTH  (0xffff - RESPONSE_PKT_OVERHEAD) // max for uint16 params!!
	
  uint8 ResponseBuffer[MAX_RESPONSE_DATA_LENGTH + RESPONSE_PKT_OVERHEAD];  // max possible size 
	
									
  // set-up the command structure
  uint8 ReadCmd[] = { 0xAA,0x55,
		      5,5,         // command length in packets (repeated)
		      0,1,         // command opcode
		      0x00,0x00,   // address (32-bit)  (placeholder)
		      0x00,0x00,   // address (32-bit)  (placeholder) 
		      0x00,0x00,   // data length (16-bit)  (placeholder)
		      0x0D,0x00 }; // checksum  (16-bit) (placeholder) 
	
  ReadCmd[6] = (uint8) (u32StartAddress >> 24); // hi-byte                        
  ReadCmd[7] = (uint8) (u32StartAddress >> 16);
  ReadCmd[8] = (uint8) (u32StartAddress >> 8);
  ReadCmd[9] = (uint8)  u32StartAddress; // lo-byte

							
  ReadCmd[10] = u16nBytes / 256; // hi-byte                        
  ReadCmd[11] = (uint8) u16nBytes; // lo-byte
													
  CalculateCmdChecksum( ReadCmd, sizeof(ReadCmd) );
	
	
  // set-up expected response size
  uint16 u16ResponseLength =  u16nBytes + RESPONSE_PKT_OVERHEAD;
	 
  if (ProtocolType==1)
    {
      // new protocol drives will always return an EVEN numbers of bytes in response (rounded up),  
      // so, if request is odd, then add one to expected ResponseLength 
      if ((u16nBytes & 0x0001) == 0x0001)
	{
	  LOG_TRACE "SerialSimpleReadMemory: NEW protocol: requested u16nBytes(%u) is ODD - so adding one to expected u16ResponseLength", u16nBytes);
      u16ResponseLength++; // make even
    }
} 

	
	// Send Command And Receive Response
	protocol_rc prc = SendCmdAndReceiveResponse( bCellNo, ChannelId, ReadCmd, sizeof(ReadCmd), ResponseBuffer, &u16ResponseLength, Timeout_mS );

	
	if (prc == prcOK)
	{
		// validate checksum
		if ( !ValidateChecksum( ResponseBuffer, u16ResponseLength))
		{
			// (failing ValidateChecksum logs its own error message (with details))
			nConsecutiveErrors++;  
			rc = 1;  
		} 
		else
		{
			memcpy(&u8UserDataBuffer[0], &ResponseBuffer[RESPONSE_PKT_OVERHEAD - 2], u16nBytes);
			nConsecutiveErrors = 0; 
		} 
	}
	else
	{  
		// reasons will have been displayed by SendCmdAndReceiveResponse
		nConsecutiveErrors++;  
		rc = 2;  
	}                        
	
	#define EXCESSIVE_CONSECUTIVE_ERRORS   50  // totally arbitrary
	if (nConsecutiveErrors >= EXCESSIVE_CONSECUTIVE_ERRORS)
	{
		LOG_WARNING "EXCESSIVE CONSECUTIVE Cmd/Response errors (>%d)", EXCESSIVE_CONSECUTIVE_ERRORS);
		LOG_WARNING "To give an idea at what stage in the test this happened:\n");
		LOG_WARNING "  TotalCommands=%u TotalDataBytesReceived=%.0f, TotalTimeTransferring_mS=%.0f", pstats[bCellNo].TotalCommands, pstats[bCellNo].TotalDataBytesReceived, pstats[bCellNo].TotalTimeTransferring_mS);
	}


	pstats[bCellNo].TotalCommands++;
	
	return rc;
}
		

//-----------------------------------------------------------------------------------------------
Byte SerialSimpleReadMemory( Byte bCellNo, uint8 ChannelId, uint32 u32StartAddress, uint16 u16nBytes, Byte * ResponseBuffer, uint16 Timeout_mS )
//-----------------------------------------------------------------------------------------------
{   
	Byte rc = 0;  
	static int nConsecutiveErrors = 0;
	
	#define RESPONSE_PKT_OVERHEAD   (6 + 2) // Relates to CUSTOMER overhead !!!!! (NOT any Xyratex bulk-transport overhead - which is 1+1+2) !!!!
	
	//#define MAX_RESPONSE_DATA_LENGTH  (0xffff - RESPONSE_PKT_OVERHEAD) // max for uint16 params!!
	
	//uint8 ResponseBuffer[MAX_RESPONSE_DATA_LENGTH + RESPONSE_PKT_OVERHEAD];  // max possible size 
	
									
	// set-up the command structure
	uint8 ReadCmd[] = { 0xAA,0x55,
												5,5,         // command length in packets (repeated)
												0,1,         // command opcode
												0x00,0x00,   // address (32-bit)  (placeholder)
												0x00,0x00,   // address (32-bit)  (placeholder) 
												0x00,0x00,   // data length (16-bit)  (placeholder)
												0x0D,0x00 }; // checksum  (16-bit) (placeholder) 
	
	ReadCmd[6] = (uint8) (u32StartAddress >> 24); // hi-byte                        
	ReadCmd[7] = (uint8) (u32StartAddress >> 16);
	ReadCmd[8] = (uint8) (u32StartAddress >> 8);
	ReadCmd[9] = (uint8)  u32StartAddress; // lo-byte

							
	ReadCmd[10] = u16nBytes / 256; // hi-byte                        
	ReadCmd[11] = (uint8) u16nBytes; // lo-byte
													
	CalculateCmdChecksum( ReadCmd, sizeof(ReadCmd) );
	
	
	// set-up expected response size
	uint16 u16ResponseLength =  u16nBytes + RESPONSE_PKT_OVERHEAD;
	 
	if (ProtocolType==1)
	{
		// new protocol drives will always return an EVEN numbers of bytes in response (rounded up),  
		// so, if request is odd, then add one to expected ResponseLength 
			if ((u16nBytes & 0x0001) == 0x0001)
		{
				LOG_TRACE "SerialSimpleReadMemory: NEW protocol: requested u16nBytes(%u) is ODD - so adding one to expected u16ResponseLength", u16nBytes);
				u16ResponseLength++; // make even
			}
	} 

	
	// Send Command And Receive Response
	protocol_rc prc = SendCmdAndReceiveResponse( bCellNo, ChannelId, ReadCmd, sizeof(ReadCmd), ResponseBuffer, &u16ResponseLength, Timeout_mS );

	
	if (prc == prcOK)
	{
		// validate checksum
		if ( !ValidateChecksum( ResponseBuffer, u16ResponseLength))
		{
			// (failing ValidateChecksum logs its own error message (with details))
			nConsecutiveErrors++;  
			rc = 1;  
		} 
		else
		{
			nConsecutiveErrors = 0; 
		} 
	}
	else
	{  
		// reasons will have been displayed by SendCmdAndReceiveResponse
		nConsecutiveErrors++;  
		rc = 2;  
	}                        
	
	#define EXCESSIVE_CONSECUTIVE_ERRORS   50  // totally arbitrary
	if (nConsecutiveErrors >= EXCESSIVE_CONSECUTIVE_ERRORS)
	{
		LOG_WARNING "EXCESSIVE CONSECUTIVE Cmd/Response errors (>%d)", EXCESSIVE_CONSECUTIVE_ERRORS);
		LOG_WARNING "To give an idea at what stage in the test this happened:\n");
		LOG_WARNING "  TotalCommands=%u TotalDataBytesReceived=%.0f, TotalTimeTransferring_mS=%.0f", pstats[bCellNo].TotalCommands, pstats[bCellNo].TotalDataBytesReceived, pstats[bCellNo].TotalTimeTransferring_mS);
	}


	pstats[bCellNo].TotalCommands++;
	
	return rc;
}
		

//---------------------------------------------------------------------------------------------------------------------------------------------
protocol_rc SendCmdAndReceiveResponse( Byte bCellNo, uint8 u8ChannelId, uint8 Cmd[], uint16 u16CmdLength, uint8 ResponseBuffer[], uint16 *pu16ResponseLength, uint16 Timeout_mS)
//---------------------------------------------------------------------------------------------------------------------------------------------
{    
		protocol_rc prc = prcOK;    

		LOG_TRACE "Sending Command to drive, len=%d, data=0x:", u16CmdLength);
		DisplayData( Cmd, u16CmdLength ); // using LOG_TRACE

		TIME_TYPE TimeStartSendCmd;
		GET_TIME (&TimeStartSendCmd);

		prc = BulkSerialSendCmd( bCellNo, u8ChannelId, u16CmdLength, Cmd);
		if (prc != prcOK)
		{  
			LOG_ERROR "BulkSerialSendCmd( ) FAILED - %s (%d)",  CONVERT_PROTOCOLRC_TO_PSZ(prc), prc );
			// statisitcs maintained in globals in serialProtocolInPC
			LOG_ERROR "possibly of interest: u8BadAckValue=0x%x, u8RetriedAckValue=0x%x, u8PacketNumberWithinCmd=%d, u8RetransmitsLastCommand=%d, u8RetransmitsThisPacket=%d",
																			pstats[bCellNo].u8BadAckValue, pstats[bCellNo].u8RetriedAckValue, pstats[bCellNo].u8PacketNumberWithinCmd, pstats[bCellNo].u8RetransmitsLastCommand, pstats[bCellNo].u8RetransmitsThisPacket );
			LOG_ERROR "general stats: TotalPacketRetransmits(this program to date)=%d", pstats[bCellNo].TotalPacketRetransmits); 
		}  
		else
		{ 
			LOG_TRACE "Receiving Resonse from drive (of expected length %u)", *pu16ResponseLength);
			
			TIME_TYPE TimeStartReceiveRespose;
			GET_TIME (&TimeStartReceiveRespose);
			
			// pu16ResponseLength is an INPUT (AND, in the case of xyrcTimeoutAwaitingResponse, AN OUTPUT)
			prc = BulkSerialReadResponse( bCellNo, u8ChannelId, pu16ResponseLength, ResponseBuffer, Timeout_mS);
			if (prc != prcOK)
			{
				LOG_ERROR "BulkSerialReadResponse( ) FAILED - %s (actual ResponseLength=%d)",  CONVERT_PROTOCOLRC_TO_PSZ(prc), *pu16ResponseLength );
			} 
			else
			{
				TIME_TYPE TimeCompleteReceiveResponse;
				GET_TIME (&TimeCompleteReceiveResponse);
				
				pstats[bCellNo].TotalDataBytesReceived += (*pu16ResponseLength  - RESPONSE_PKT_OVERHEAD);
				pstats[bCellNo].TotalTimeTransferring_mS += ElapsedTimeIn_mS( &TimeStartSendCmd, &TimeCompleteReceiveResponse); 
				pstats[bCellNo].TotalTimeReceiving_mS += ElapsedTimeIn_mS( &TimeStartReceiveRespose, &TimeCompleteReceiveResponse); 
				pstats[bCellNo].TotalTimeTransferring_uS += ElapsedTimeIn_uS( &TimeStartSendCmd, &TimeCompleteReceiveResponse); 
				pstats[bCellNo].TotalTimeReceiving_uS += ElapsedTimeIn_uS( &TimeStartReceiveRespose, &TimeCompleteReceiveResponse); 
			} 
			 
			// display response data
			LOG_TRACE "Response was %d bytes, data=0x:", *pu16ResponseLength); 
			DisplayData( ResponseBuffer, *pu16ResponseLength ); // using LOG_TRACE
		}  
		
		return prc; 
}






//=======================================================================================================================================================================
// CommandBlock is transferred to device in segments of DrivePacketSize (performing serial line turn-around and ack checking)
// (This call will block until u8CommandBlock successfully sent to drive, unrecoverable problem occurs, or timeout waiting for ack from drive)
protocol_rc BulkSerialSendCmd (
																	Byte bCellNo,
																	uint8  u8ChannelId,             // 0 or 1 (identifies which UART, ie which drive)
																	uint16 u16CommandLen,           // total num bytes in command block
																	const  uint8 u8CommandBlock[]   // caller's command block 
																)    
//=======================================================================================================================================================================
{
	protocol_rc prc = prcOK;
	
	// maybe a good idea to flush the UART receive buffer first
	FlushRxUART( bCellNo, u8ChannelId ); 
	
	// send the Command block data, pktsize(2) bytes at a time - waiting for DUTs ACK in between each pkt
	//-----------------------------------------------------------------------------------------------------
	pstats[bCellNo].u8PacketNumberWithinCmd = 1;
	int nBytesSent = 0;
	while ( nBytesSent < u16CommandLen)
	{
		prc = SendPktAndWaitForACK(  bCellNo, u8ChannelId, &(u8CommandBlock[ nBytesSent ]), u8PacketSize ); // may do a short-send ?
		if (prc == prcOK)
		{
			nBytesSent += u8PacketSize;
		}
		else
		{
			pstats[bCellNo].TotalBadCommands++;
			LOG_ERROR "BulkSerialSendCmd: SendPktAndWaitForACK failed protocol_rc=%d (on Pkt %u of %u)- aborting this BulkSerialSendCmd",
								prc, (nBytesSent/u8PacketSize)+1, u16CommandLen/u8PacketSize  ); 
			break; // BREAK OUT OF WHILE (with xyrc set appropriately)
		}
		pstats[bCellNo].u8PacketNumberWithinCmd++; 
	}
	
	return prc;
}

						 
//=======================================================================================================================================================================
// Every command has a response of variable length
// (This call will block until drive sends the requested number of bytes (or timeout, or RxBuffOverrun))
protocol_rc BulkSerialReadResponse(
							Byte bCellNo,
							uint8  u8ChannelId,             // 0 or 1 (identifies which UART, ie which drive)
							uint16 *pu16ResponseLen,        // INPUT - num bytes to read from drive (and, in the case of xyrcTimeoutOnBlockedRead, output = number of bytes actually received)
							uint8  u8ResponseBuffer[],       // caller's response block buffer - method fills with data from drive
							uint16 Timeout_mS
							)        
//=======================================================================================================================================================================
{
	protocol_rc prc = prcOK;
	xyrc xyrc = xyrcOK;
	
	// Try to receive specified number of bytes (as Drive's response to a previous BulkSerialSendCmd)
	//  NOTE:  NO handshaking involved before, during, or after these bytes
	
	uint16 nReceivesForThisResponse = 0; // stats


	// Two different approaches here - depending on if we are trying to receive more bytes that the cell Rx FIFO/buffer can hold
	#define MAX_CAN_REQUEST_IN_RECEIVEWITHWAIT  2048  // LESS_THAN_OR_EQUAL_TO_CELL_RX_FIFO_SIZE   2048
	//-------------------------------------------------------------------------------------------------------------------------------------
	if (*pu16ResponseLen > MAX_CAN_REQUEST_IN_RECEIVEWITHWAIT)
	{
		// over-commit case:
		// the amount we are expecting will NOT fit into the cell rx buffer
		// - so we need to do a LOOP of receives (ie 'pull' as fast as we can)
		//  However - it is still best to use a series of BLOCKED receives 
		//  (to avoid overloading transport system by moving small numbers of bytes at a time)
		//  The best size for the blocked receive may need some experimentation/tuning
		//  - but the best-guess is to pull HALF of the the buffer size (ie effectively create a delivery 'watermark' half way up the buffer)
		
		// (decided to split this into a seperate function for clarity)
		prc = BulkSerialReadResponse_OverCommitCase( bCellNo, u8ChannelId, pu16ResponseLen, u8ResponseBuffer, &nReceivesForThisResponse, Timeout_mS);
	}
	else
	{
		// 'normal' case:
		//  the amount we are expecting will fit into cell rx buffer
		// - so we can just do a single blocked receive for the full amount
		//-------------------------------------------------------------------   
		// - EXCEPT we need a loop to handle the situation where the caller's timeout is greater than the largest Xyratex blocked timeout (255*10mS)
		 
		uint16 u16ReceivedSoFar = 0;
		uint16 u16BytesStillWanted = *pu16ResponseLen;

		#define MAX_XY_TIMEOUT_MS  2550  // 255 * 10us (xyratex call takes a uint8 (and its units are 10mS)
		uint16 Timeout_mS_remaining = Timeout_mS;                         
		while  (u16BytesStillWanted > 0)                        
		{  
			uint16 nBytes_WantedIn_GotOut = u16BytesStillWanted;
			uint8 OverFlowFlag = 0;
			uint16 Timeout_mS_this_call = MIN( Timeout_mS_remaining, MAX_XY_TIMEOUT_MS);
  		   
			LOG_TRACE "about to RxWithWait( t/o=%u, wantedin=%u)", (uint8) (Timeout_mS_this_call/10), nBytes_WantedIn_GotOut);
  		                        
			xyrc = pCellRIM->BulkSerialReceiveWithWait( u8ChannelId, (uint8) (Timeout_mS_this_call/10), &(u8ResponseBuffer[u16ReceivedSoFar]), nBytes_WantedIn_GotOut, OverFlowFlag);
			if (xyrc == xyrcOK)             //--------
			{
				// In this 'normal' case, *pu16ResponseLen <= MAX_CAN_REQUEST_IN_RECEIVEWITHWAIT,
				// so Overrun should NEVER occur in this case (unless the drive has burst more than asked, or we did not flush Rx before allowing drive to burst)
				if (OverFlowFlag > 0) // 
				{
					LOG_ERROR "BulkSerialReadResponse OVERFLOW on receive call (picked up %u bytes of requested %u (MAX_CAN_REQUEST_IN_RECEIVEWITHWAIT=%u))", nBytes_WantedIn_GotOut, *pu16ResponseLen, MAX_CAN_REQUEST_IN_RECEIVEWITHWAIT);  
					// We will have lost some of the drive's burst of data,
					// and will have to repeat the whole protocol request
					//
					// But first we MUST wait until the drive has definately finshed its burst (and gone back to listening mode)
					// - before we pick-up and discard whatever else is in our RxBuffer, and allow the higher-levels to sart the whole exchange again
					//
  				
					// choice of wait time needs careful thought/experimentation
					//-----------------------------------------------------------
					//  eg 32K packet takes 500 ms (with typical drive bursting at 50Kbytes/sec @ 2MBaud) 
					// but no harm in waiting loger, if we get lots of overruns -it will further affect performance,
					// - but overruns often result from processor overload (eg process start-up),
					//  so our sleeping a while will:
					//   a) help out the processor
					//   b) mean we are less likely to suffer a second overrun due to the original problem
					#define SLEEP_SECS_BAFTER_OVERRUN_BEFORE_FLUSH_AND_RETURN  2 // see above
					LOG_ERROR "  - waiting %d secs to allow drive to finish its burst, before flushing Rx Buffer, and returning error to higher-level",SLEEP_SECS_BAFTER_OVERRUN_BEFORE_FLUSH_AND_RETURN);
					sleep(SLEEP_SECS_BAFTER_OVERRUN_BEFORE_FLUSH_AND_RETURN); 
  				
					FlushRxUART( bCellNo, u8ChannelId);
  				
					prc = prcRxBufferOverrun; 
  				
					pstats[bCellNo].TotalBadResponses++;
					pstats[bCellNo].TotalResponseOverruns++;
  				
					// NOTE: we do NOT count any bytes received for this resposne  (ie the stats reflect TRUE actual throughput of VALID data)
					break; // BREAK out of while
				}
				else
				{
					LOG_TRACE "back from RxWithWait( got_out=%u, old u16BytesStillWanted=%u)", nBytes_WantedIn_GotOut, u16BytesStillWanted);
					nReceivesForThisResponse++;
					Timeout_mS_remaining -= Timeout_mS_this_call;
					u16ReceivedSoFar += nBytes_WantedIn_GotOut;
					u16BytesStillWanted = *pu16ResponseLen -  u16ReceivedSoFar;
					
					if (u16BytesStillWanted > 0)
					{
						// short-read - must be due to ReceiveWithWait timing out 
              					
						LOG_TRACE "short receive( updated: u16BytesStillWanted=%u, u16ReceivedSoFar=%u, Timeout_mS_remaining=%u)", u16BytesStillWanted, u16ReceivedSoFar, Timeout_mS_remaining);
						if ( Timeout_mS_remaining == 0)
						{
							prc = prcTimeoutWaitingForCompleteResponse; 
							LOG_ERROR "BulkSerialReadResponse: self-enforced timeout waiting for full response (over %u mSecs) (got %u of requested %u bytes)",
												Timeout_mS, u16ReceivedSoFar, *pu16ResponseLen  ); 
							pstats[bCellNo].TotalBadResponses++;
							break; // BREAK out of while
   					
						}
					}
				}
			}
			else
			{
				LOG_ERROR "BulkSerialReadResponse: BulkSerialReceiveWithWait( for upto %u) failed %s  - aborting BulkSerialReadResponse ", u16BytesStillWanted, CELLRIM_XYRC_TO_STRING(xyrc) );
				pstats[bCellNo].last_bad_xyrc = xyrc; 
				break; // BREAK out of while
			}
  		
		} // while
		
		*pu16ResponseLen = u16ReceivedSoFar; // TELL caller how much of his buffer is valid (in the case where we timed-out (or overran) receiving requested number of bytes)
	}


	// stats
	if (nReceivesForThisResponse > pstats[bCellNo].HighestReceivesForOneResponse)
	{
		pstats[bCellNo].HighestReceivesForOneResponse = nReceivesForThisResponse;
	}
	
	pstats[bCellNo].AvgReceivesForOneResponse = (int) (  ((pstats[bCellNo].AvgReceivesForOneResponse * pstats[bCellNo].TotalNumberOfResponsesReceived) + nReceivesForThisResponse) / 	(pstats[bCellNo].TotalNumberOfResponsesReceived+1) );
	pstats[bCellNo].TotalNumberOfResponsesReceived++;

		
	//LOG_TRACE " - BulkSerialReadResponse: returning");
	return prc;
}

 
 
//=======================================================================================================================================================================
protocol_rc SendPktAndWaitForACK(  Byte bCellNo, uint8 u8ChannelId, const uint8 u8PktData[ ], uint8 u8PacketSize )
//=======================================================================================================================================================================
{
	protocol_rc prc = prcOK;
	xyrc xyrc = xyrcOK;
		
	LOG_TRACE "SendPktAndWaitForACK: entered: ChannelId=%u pktsize=%u,Data[0]=0x%.2x Data[1]=0x%.2x", u8ChannelId, u8PacketSize, u8PktData[0], u8PktData[1] ); 
	
	int nReceivesForThisAck = 0;  // just for statistics/interest

	pstats[bCellNo].u8RetransmitsThisPacket = 0;  // global (so caller can get visibility (eg by a new call something like GetLastErrorStatsForSerial()
	bool RetransmitWanted = false; // for certain types of NAK, we will Re-transimt within here

	do //  while (RetransmitWanted == true)
	{
		RetransmitWanted = false;

//#define  MORE_CHARACTER_INTERVAL     // [DEBUG]  many UART error occurs during actual SRST testing
                                       //          drive might be too busy to receive UART data timely then overrun.
                                       //          this macro enables to insert 'sleep' between eacy byte transmitting.
                                       //          it maybe reliable, but obviously degrade UART performance.

#if defined(MORE_CHARACTER_INTERVAL)          // [DEBUG]
		LOG_INFO "MORE_CHARACTER_INTERVAL");
		int i = 0;
		for (i = 0 ; i < u8PacketSize ; i++) {
		  xyrc = pCellRIM->BulkSerialSend( u8ChannelId, (uint8 *)(u8PktData + i), 1);
		  if (xyrc != xyrcOK) {
		    break;
		  }
		  usleep(1000 * 1000);
		}
#else
		xyrc = pCellRIM->BulkSerialSend( u8ChannelId, (uint8 *)u8PktData, u8PacketSize);
#endif
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "SendPktAndWaitForACK: BulkSerialSend failed %s (on re-transmit %u for this packet)", CELLRIM_XYRC_TO_STRING(xyrc), pstats[bCellNo].u8RetransmitsThisPacket); 
			prc = prcXYRC;
			pstats[bCellNo].last_bad_xyrc = xyrc;
		}
		else
		{
			// blocked receive for 1-byte (the ACK or NAK*), with shorter timeout
			uint8 u8Char;
			uint16 nbytes = 1; // NOTE this is a IN/OUT parameter !
			uint8 OverFlowFlag; // maintained by receive
			xyrc = pCellRIM->BulkSerialReceiveWithWait( u8ChannelId, (uint8)(u16Timeout_PktSendToAckReceipt_mS/10), &u8Char, nbytes, OverFlowFlag);
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "SendPktAndWaitForACK: BulkSerialReceive failed %s", CELLRIM_XYRC_TO_STRING(xyrc)); 
				prc = prcXYRC;
				pstats[bCellNo].last_bad_xyrc = xyrc; 
			}
			else
			{
				nReceivesForThisAck++; // stats 
				
				if (nbytes == 0) // got a byte
				{
					prc = prcTimeoutWaitingForAck; 
					LOG_ERROR "SendPktAndWaitForACK: self-enforced timeout waiting for ack (over %u mS)",  u16Timeout_PktSendToAckReceipt_mS ); 

#if defined(OPTIMUS)  //[DEBUG]------------------------------------------------------------------------
					if (tccb.workaroundForAckMissingInOptimus) {
						LOG_WARNING "workaround for ack missing in optimus --> recreate pCellRIM");
						delete pCellRIM;
						pCellRIM = CCELLRIM::CreateInstance(  psda[bCellNo].CellIndex, psda[bCellNo].EnvironmentId);
						if (pCellRIM != NULL) {
							LOG_INFO "%s,%d: automatically Configure_SATA_P11_as_Float_To_PREVENT_Autospinup\n", __func__, bCellNo );
						} else {
							LOG_ERROR "%s,%d: CCellRIM...::CreateInstance returned NULL - MAJOR SYSTEM ERROR / resource shortage!!!!", __func__, bCellNo); 
							tcExit(EXIT_FAILURE);
				       		}
					}
#endif               //[DEBUG]------------------------------------------------------------------------

				} 
				else
				{
					// got our requested 1 byte, see what it is...
					if (u8Char == ACK)
					{
						LOG_TRACE " got our ACK (on %dth BulkSerialReceive)", nReceivesForThisAck);
					}
					else // bad ack 
					{
						// is it something WE can retransmit, or do we abort the command ?
						if ( (u8Char >= RETRYABLE_NAK_RANGE_START) && (u8Char <= RETRYABLE_NAK_RANGE_END) )
						{
							if (pstats[bCellNo].u8RetransmitsThisPacket <= u8MaxRetransmitsPerPacket)
							{
								pstats[bCellNo].TotalPacketRetransmits++;
								pstats[bCellNo].u8RetransmitsLastCommand++;
								pstats[bCellNo].u8RetransmitsThisPacket++;

								pstats[bCellNo].u8RetriedAckValue = u8Char; // GetLastError allows caller to be aware of this 

								// before we can resend packet, we must wait for drive to stop driving line after sending its ack/nak
								// NOTE: should not need any delay here when driving from PC 
								//       -since typical USB/8051 latency is  ~ 3mS anyway
								
								LOG_INFO "got RETRYABLE_NAK (0x%.2x)  (on %dth BulkSerialReceive)", u8Char, nReceivesForThisAck);
							
								RetransmitWanted = true;  // this will trigger outer do-while loop to repeat
							}
							else
							{
								// just give-up, and inform the PC as we always do for non-retryable naks
								pstats[bCellNo].u8BadAckValue = u8Char;  // tell PC which retryable ack we were retrying
								prc = prcExcessivePacketReTransmits;
								LOG_ERROR "SendPktAndWaitForACK: ExcessivePacketReTransmits (>%u), latestBadAckValue=0x%.2x", u8MaxRetransmitsPerPacket, u8Char); 
							}
						}
						else
						{
							// bad-ack is so bad we can go no further...
							prc = prcBadAckValue;
							LOG_ERROR "SendPktAndWaitForACK: BadAckValue=0x%.2x", u8Char); 
						}
					} 
				}
			}	
		} 
	}  while (RetransmitWanted == true);


	LOG_TRACE "SendPktAndWaitForACK: leaving: nReceivesForThisAck=%u u8RetransmitsThisPacket=%u, u8RetriedAckValue=0x%.2x, u8BadAckValue=0x%.2x ", nReceivesForThisAck, pstats[bCellNo].u8RetransmitsThisPacket, pstats[bCellNo].u8RetriedAckValue, pstats[bCellNo].u8BadAckValue ); 
	
	if (nReceivesForThisAck > pstats[bCellNo].HighestReceivesForOneAck)
	{
		pstats[bCellNo].HighestReceivesForOneAck = nReceivesForThisAck;
	}
	pstats[bCellNo].AvgReceivesForOneAck = (int) ( ((pstats[bCellNo].AvgReceivesForOneAck * pstats[bCellNo].TotalNumberOfAcksReceived) + nReceivesForThisAck)	/ (pstats[bCellNo].TotalNumberOfAcksReceived+1));
	pstats[bCellNo].TotalNumberOfAcksReceived++;
	
	return prc;
}



//=======================================================================================================================================================================
void  FlushRxUART( Byte bCellNo, uint8 u8ChannelId )
//=======================================================================================================================================================================
{
	xyrc xyrc = xyrcOK;

	//#define MAX_CELL_RX_BUFFER  4096 // this just needs to equal to (or greater than) the size of the RX_FIFO(buffer) in the cell (deliberately set 'greater than' to allow for 'enhancements' in cell)
	//#define MAX_CELL_RX_BUFFER  2048
	#define MAX_CELL_RX_BUFFER  500

	uint8 ScratchBuf[ MAX_CELL_RX_BUFFER ];
	uint16 nbytes = MAX_CELL_RX_BUFFER; // NOTE this is a IN/OUT parameter !
	uint8 OverFlowFlag;

	#define REPEAT_FLUSH_UNTIL_EMPTY

#if defined(REPEAT_FLUSH_UNTIL_EMPTY)
	while (1) {
		xyrc = pCellRIM->BulkSerialReceive( u8ChannelId, ScratchBuf, nbytes, OverFlowFlag);
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "FlushRxUART: BulkSerialReceive failed %s", CELLRIM_XYRC_TO_STRING(xyrc)); 
			pstats[bCellNo].last_bad_xyrc = xyrc; 
		}
		else
		{
			//if (nbytes > 0)
			{
				LOG_TRACE "FlushRxUART: BulkSerialReceive(upto %u) picked-up %u bytes (OverFlowFlag was %d)", MAX_CELL_RX_BUFFER, nbytes, OverFlowFlag); 
			} 
			if (nbytes < MAX_CELL_RX_BUFFER) {
				LOG_TRACE "FlushRxUART: Have made sure rx fifo empty"); 
				break;
			} else {
				LOG_TRACE "FlushRxUART: Some data is still in rx buffer"); 
				nbytes = MAX_CELL_RX_BUFFER;
			}
		}
	}
#else
	xyrc = pCellRIM->BulkSerialReceive( u8ChannelId, ScratchBuf, nbytes, OverFlowFlag);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "FlushRxUART: BulkSerialReceive failed %s", CELLRIM_XYRC_TO_STRING(xyrc)); 
		pstats[bCellNo].last_bad_xyrc = xyrc; 
	}
	else
	{
		//if (nbytes > 0)
		{
			LOG_TRACE "FlushRxUART: BulkSerialReceive(upto %u) picked-up %u bytes (OverFlowFlag was %d)", MAX_CELL_RX_BUFFER, nbytes, OverFlowFlag); 
		} 
	}
#endif
}


//=======================================================================================================================================================================
char * CONVERT_PROTOCOLRC_TO_PSZ ( protocol_rc prc)
//=======================================================================================================================================================================
{
	switch (prc)
	{
		case prcOK: return("prcOK");
		case prcXYRC: return("prcXYRC");
		case prcTimeoutWaitingForAck: return("prcTimeoutWaitingForAck");
		case prcBadAckValue: return("prcBadAckValue");
		case prcExcessivePacketReTransmits: return("prcExcessivePacketReTransmits");
		case prcRxBufferOverrun: return("prcRxBufferOverrun");
		case prcTimeoutWaitingForCompleteResponse: return("prcTimeoutWaitingForCompleteResponse");
		case prcNotImplementedYet:return("prcNotImplementedYet");
		
		
		// insert new values immediately ABOVE here...
		case prcEnd: return("prcEnd");
		
		default: return("Unknown??");
	}
}


//=======================================================================================================================================================================
protocol_rc BulkSerialReadResponse_OverCommitCase(  Byte bCellNo, uint8 u8ChannelId, uint16 *pu16ResponseLen, uint8 u8ResponseBuffer[], uint16* pnReceivesForThisResponse, uint16 Timeout_mS)
{
	protocol_rc prc = prcOK;
	xyrc xyrc = xyrcOK;
	
	// Try to receive specified number of bytes (as Drive's response to a previous BulkSerialSendCmd)
	//  NOTE:  NO handshaking involved before, during, or after these bytes
	
	// This is a seperate function designed to handle the 'over-commit' case:
	// where the amount we are expecting will NOT fit into the cell rx buffer
	// - so we need to do a LOOP of receives (ie 'pull' as fast as we can)
	//  However - it is still best to use a series of BLOCKED receives 
	//  (to avoid overloading transport system by moving small numbers of bytes at a time)
	//  The best size for the blocked receive may need some experimentation/tuning
	//  - but the best-guess is to pull HALF of the the buffer size (ie effectively create a delivery 'watermark' half way up the buffer)

	//#define MAX_CAN_REQUEST_IN_RECEIVEWITHWAIT  2048  // LESS_THAN_OR_EQUAL_TO_CELL_RX_FIFO_SIZE   2048
	//-------------------------------------------------------------------------------------------------------------------------------------
	
	#define CHOSEN_WAIT_PULL_SIZE  (MAX_CAN_REQUEST_IN_RECEIVEWITHWAIT / 2)  // concept of a 'watermark' half way up buffer (unblock the read whenever we reaches that level)
	
	uint16 nBytesStillWanted = *pu16ResponseLen;
	uint16 nBytesReceivedTotal = 0;
	*pnReceivesForThisResponse = 0; // stats
	
	while ( nBytesStillWanted > 0)
	{
		uint16 nBytes_WantedThisTimeIn_GotOut = MIN (nBytesStillWanted, CHOSEN_WAIT_PULL_SIZE);
		uint16 savednBytesRequestedThisTime = nBytes_WantedThisTimeIn_GotOut; // useful info for overflow printf
		uint8 OverFlowFlag = 0;
		
		(*pnReceivesForThisResponse)++; // stats
		LOG_TRACE "BulkSerialReadResponse_OverCommitCase making %uth call to ReceiveWithWait, for %u bytes (Expected %u total - so %d still to go)",
																					*pnReceivesForThisResponse, nBytes_WantedThisTimeIn_GotOut, *pu16ResponseLen, nBytesStillWanted);
								                          
		xyrc = pCellRIM->BulkSerialReceiveWithWait( u8ChannelId, (uint8) (Timeout_mS/10), &u8ResponseBuffer[nBytesReceivedTotal], nBytes_WantedThisTimeIn_GotOut, OverFlowFlag);
		if (xyrc != xyrcOK)             //--------
		{
			LOG_ERROR "BulkSerialReadResponse_OverCommitCase: BulkSerialReceiveWithWait( for upto %u) failed %s  - aborting BulkSerialReadResponse ",*pu16ResponseLen, CELLRIM_XYRC_TO_STRING(xyrc) );
			pstats[bCellNo].last_bad_xyrc = xyrc; // this will also affect ultimate prc (even though we then receve prcOK here) ??
		}
		else
		{
			// process what we received	
			if (OverFlowFlag > 0) 
			{
				LOG_INFO "BulkSerialReadResponse_OverCommitCase Rx OVERFLOW on receive call number %d (picked up %u bytes of requested %u,   Expected %u total - so %u still to go)",
													*pnReceivesForThisResponse, nBytes_WantedThisTimeIn_GotOut, savednBytesRequestedThisTime, *pu16ResponseLen, nBytesStillWanted);  
				// We will have lost some of the drive's burst of data, (probably not 'pulling' fast enough)
				// so we will have to repeat the whole protocol request (NOT THE END OF THE WORLD!!!!)
				//
				// But first, we MUST WAIT until the drive has definatley finshed its burst (and gone back to listening mode)
				// - before we then pick-up and discard whatever is in our RxBuffer,
				//  and allow the higher-levels to sart the whole exchange again
				
				
				// choice of wait time needs careful thought/experimentation
				//-----------------------------------------------------------
				//  eg 32K packet takes 500 ms (with typical drive bursting at 50Kbytes/sec @ 2MBaud) 
				// but no harm in waiting loger, if we get lots of overruns -it will further affect performance,
				// - but overruns often result from processor overload (eg process start-up),
				//  so our sleeping a while will:
				//   a) help out the processor
				//   b) mean we are less likely to suffer a second overrun due to the original problem
				#define OC_SLEEP_SECS_AFTER_OVERRUN_BEFORE_FLUSH_AND_RETURN  2 // see above
				LOG_INFO "  - waiting %d secs to allow drive to finish its burst, before flushing Rx Buffer, and returning error to higher-level",OC_SLEEP_SECS_AFTER_OVERRUN_BEFORE_FLUSH_AND_RETURN);
				sleep(OC_SLEEP_SECS_AFTER_OVERRUN_BEFORE_FLUSH_AND_RETURN); 
				
				FlushRxUART( bCellNo, u8ChannelId);
				
				prc = prcRxBufferOverrun; 
				
				pstats[bCellNo].TotalBadResponses++;
				pstats[bCellNo].TotalResponseOverruns++;
				
				// NOTE: we do NOT count any bytes received for this resposne  (ie the stats reflect TRUE actual throughput of VALID data)
				
				break;  // BREAK OUT OF RECEIVE while LOOP - we have GIVEN-UP on this packet/response/burst !
			}
			else
			{
				// loop-control housekeeping
				nBytesReceivedTotal += nBytes_WantedThisTimeIn_GotOut;
				nBytesStillWanted -= nBytes_WantedThisTimeIn_GotOut;

				LOG_TRACE "BulkSerialReadResponse_OverCommitCase receive call %d picked up %u bytes (Expected %u total - so %d still to go)",
																							*pnReceivesForThisResponse, nBytes_WantedThisTimeIn_GotOut, *pu16ResponseLen, nBytesStillWanted);
				
				// if we didn't get the number we wanted (from BLOCKED read), then that is a timeout
				if (nBytes_WantedThisTimeIn_GotOut != savednBytesRequestedThisTime )
				{
					prc = prcTimeoutWaitingForCompleteResponse; 
					LOG_ERROR "BulkSerialReadResponse_OverCommitCase: self-enforced timeout waiting for full response (over %u mSecs) (got %u of requested %u bytes)",
										Timeout_mS, nBytesReceivedTotal, *pu16ResponseLen  ); 
					pstats[bCellNo].TotalBadResponses++;
					break; // out of while !!!
				}
	 
			}
		}
		
	} // while  nBytesStillWanted 
	
	
	*pu16ResponseLen = nBytesReceivedTotal; // Tell caller how much of his buffer is valid (in the case where we timed-out (or overran) receiving requested number of bytes)
	
	LOG_TRACE "BulkSerialReadResponse_OverCommitCase: returning (nCellReceives was %u)", *pnReceivesForThisResponse);
	
	return prc;
}




//-----------------------------------------------------------------------------------------------------------------------------------------------
bool ValidateChecksum( uint8 Response[], uint16 u16ResponseLength)
//-----------------------------------------------------------------------------------------------------------------------------------------------
{
	// checksums "the IBM way"      
	bool brc = true;
	uint16 u16CalculatedChecksum = 0;
	uint16 u16ReceivedChecksum = 0;
	
	if (ProtocolType==1) // protocol -------------------------------------------------------------------------
	{
		// NEW protocol: only ever returns aa55  (ie the data is ALWAYS in big endian (Motorola/natural) format
		// 
		//  [DEBUG] only Echo command returns 0x55aa.
		//  [DEBUG] i do not know why, but it is my observation.
		//
		//	  if ((Response[0] == 0xaa) && (Response[1] == 0x55))
		if (((Response[0] == 0xaa) && (Response[1] == 0x55)) ||
				((Response[0] == 0x55) && (Response[1] == 0xaa)) )
		{
			if ((u16ResponseLength & 0x0001) == 0x0000) // check length is even
			{
				u16ReceivedChecksum = (uint16) ((uint16) Response[ u16ResponseLength-2 ] << 8) + Response[ u16ResponseLength-1 ] ;
				for (uint16 i = 0; i < u16ResponseLength - 2; i+=2 )   
				{
					u16CalculatedChecksum += ((uint16)Response[i]) << 8;
					u16CalculatedChecksum += Response[i+1];
				} 
			   
				u16CalculatedChecksum = uint16 (((uint16) 0x0000) - u16CalculatedChecksum);  
			
				LOG_TRACE "(NEW protocol): ReceivedChecksum=0x%.4x  CalculatedChecksum=0x%.4x (drive big-endian)", u16ReceivedChecksum, u16CalculatedChecksum); 
			}
			else
			{
				LOG_ERROR "ValidateChecksum (NEW protocol): u16ResponseLength is not an even value (it is 0x%X) ???",u16ResponseLength);  
				u16ReceivedChecksum = 1;  
				brc = false;
			}
		} 
		else
		{ 
			LOG_ERROR "ValidateChecksum (NEW protocol): response does not start aa55 (it starts 0x%.2X%.2X)",Response[0],Response[1]);  
			u16ReceivedChecksum = 1;  
			brc = false;
		}     

	}
	else 
	{
		// old protocol: order of first two bytes is an indication of 'endian-ness' -------------------------------------------------------------------------
		// if response start aa55, then data is to be interpreted in big endian (Motorola/natural) format
		// else if response start 55aa, then data is to be interpreted in little endian (intel/backwards) format
		if ((Response[0] == 0xaa) && (Response[1] == 0x55))
		{
			// drive has returned data in big endian (Motorola/natural) format
			u16ReceivedChecksum = (uint16) ((uint16) Response[ u16ResponseLength-2 ] << 8) + Response[ u16ResponseLength-1 ] ;
			for (uint16 i = 0; i < u16ResponseLength - 2; i+=2 )   
			{
				u16CalculatedChecksum += ((uint16)Response[i]) << 8;
				u16CalculatedChecksum += Response[i+1];
			}  
			u16CalculatedChecksum = uint16 (((uint16) 0x0000) - u16CalculatedChecksum);  
			
			LOG_TRACE "(OLD protocol):ReceivedChecksum=0x%.4x  CalculatedChecksum=0x%.4x (drive big-endian)", u16ReceivedChecksum, u16CalculatedChecksum); 
		} 
		else if ((Response[0] == 0x55) && (Response[1] == 0xaa))
		{
			u16ReceivedChecksum = (uint16) ((uint16) Response[ u16ResponseLength-1 ] << 8) + Response[ u16ResponseLength-2 ] ;
	
			for (uint16 i = 0; i < u16ResponseLength - 2; i+=2 )   
			{
				u16CalculatedChecksum += Response[i];
				if (i < (u16ResponseLength - 3 ))  // quick-fix for odd-sized responses!! (28June06)
				{
					u16CalculatedChecksum += ((uint16)Response[i+1]) << 8;
				}
			} 
			
			u16CalculatedChecksum = uint16 (((uint16) 0x0000) - u16CalculatedChecksum);  
	
			
			LOG_TRACE "(OLD protocol):ReceivedChecksum=0x%.4x  CalculatedChecksum=0x%.4x (drive little-endian)", u16ReceivedChecksum, u16CalculatedChecksum); 
			
		}  
		else
		{ 
			LOG_ERROR "ValidateChecksum (OLD protocol):cannot calculate checksum - first 2 bytes are not aa55 or 55aa (they are 0x%.2X%.2X)",Response[0],Response[1]);  
			u16ReceivedChecksum = 1;  
			brc = false;
		}     
	}			 
			 
	// now do the validation part 
	//----------------------------
			 
	if (brc != false)  // was able to calculate ok
	{
		if (u16ReceivedChecksum != u16CalculatedChecksum)
		{
			LOG_ERROR "Checksums dont match!!!! ReceivedChecksum=0x%.4x  CalculatedChecksum=0x%.4x", u16ReceivedChecksum, u16CalculatedChecksum); 
			brc = false;
		}         
		else
		{
			brc = true;
		} 
	}
	return brc;
} 



#define DATA_BYTES_PER_LINE  16
void DisplayLine( uint8 u8Data[], uint16 u16DataLineLength);

//-----------------------------------------------------------------------------------------------------------------------------------------------
void DisplayData( uint8 u8Data[], uint16 u16DataLength)
//-----------------------------------------------------------------------------------------------------------------------------------------------
{
	for (unsigned int i = 0;  i < u16DataLength; i+= DATA_BYTES_PER_LINE )
	{
		int linesize = ( (u16DataLength - i) > DATA_BYTES_PER_LINE ) ? DATA_BYTES_PER_LINE :  u16DataLength - i ; 
		DisplayLine( u8Data+i, linesize); 
	}
}

//-----------------------------------------------------------------------------------------------
void DisplayLine( uint8 u8Data[], uint16 u16DataLineLength)
//-----------------------------------------------------------------------------------------------
{
	char szLog[1024];
	int n = 0;
	
	for (unsigned int i = 0;  i < u16DataLineLength; i++)
	{
		n+=sprintf( &szLog[n], " %.2x", u8Data[i]);
	}
	
	// got an output line of data bytes
	// - now append ascii values
	n+=sprintf( &szLog[n], "    ");

	for (unsigned int i = 0;  i < u16DataLineLength; i++)
	{
		if (isprint( (int) u8Data[i] ) )
		{
			n+=sprintf( &szLog[n], "%c", u8Data[i]);
		}
		else
		{
			n+=sprintf( &szLog[n], "%c", '.');
		}
	}
	
	// now output the whole line
	LOG_TRACE "%s",szLog);
}


//-----------------------------------------------------------------------------------------------
void CalculateCmdChecksum( uint8 Cmd[], uint16 u16CmdLength )    // u16CmdLength INCLUDES space for checksum!
//-----------------------------------------------------------------------------------------------
{   
	uint16 u16ChkSum = 0;
	if ((Cmd[0] == 0xaa) && (Cmd[1] == 0x55))
	{
		// command datato drive appears to be in big endian (Motorola/natural) format
		for (uint16 i = 4; i < u16CmdLength - 2; i+=2 )   
		{
			u16ChkSum += ((uint16)Cmd[i]) << 8;
			u16ChkSum += Cmd[i+1];
		}  
			
		//LOG_INFO "TRACE - CalculateCmdChecksum: running-total u16ChkSum=0x%.4x", u16ChkSum); 
		
		uint16 u16Result = uint16 (((uint16) 0x0000) - u16ChkSum);   
		
		LOG_TRACE "CalculateCmdChecksum:: 0-u16ChkSum=0x%.4x", u16Result);
		
		Cmd[u16CmdLength-2] = (uint8) (u16Result >> 8);
		Cmd[u16CmdLength-1] = (uint8) u16Result; 
	}
	else
	{
		LOG_ERROR "CalculateCmdChecksum: Command data should start with 0xaa55 (but yours starts with 0x%.2x%.2x) !!!!!!!!!!!!", Cmd[0], Cmd[1]);
	}     
} 


unsigned long ElapsedTimeIn_mS ( TIME_TYPE *ptvStart, TIME_TYPE *ptvEnd)
//---------------------------------------------------------------------------
{
	unsigned long ElapsedMs;
	unsigned long ElapsedWholeSecs;
	unsigned long Elapsed_us_part;
	
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
	ElapsedMs = (ElapsedWholeSecs * 1000) + (Elapsed_us_part/1000);

	return ElapsedMs;
}


unsigned long ElapsedTimeIn_uS ( TIME_TYPE *ptvStart, TIME_TYPE *ptvEnd)
//---------------------------------------------------------------------------
{
	unsigned long ElapsedWholeSecs;
	unsigned long Elapsed_uS;
	unsigned long Elapsed_us_part;

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
	Elapsed_uS = (ElapsedWholeSecs * 1000 * 1000) + (Elapsed_us_part);

	return Elapsed_uS;
}



