// prototype private HELPER functions
//-----------------------------------------------------------------------------------

// Note: stats structs moved to tcTypes.h  


// [DEBUG] add following functions for uart debug
Byte SerialSimpleWriteMemoryDEBUG( Byte bCellNo, uint8 ChannelId, uint32 u32StartAddress, uint16 u16nBytes, Byte * ResponseBuffer, uint16 Timeout_mS );
Byte SerialSimpleEchoDEBUG( Byte bCellNo, uint8 ChannelId, uint16 u16nBytes, Byte * ResponseBuffer, uint16 Timeout_mS );
Byte SerialSimpleReadMemoryDEBUG( Byte bCellNo, uint8 ChannelId, uint32 u32StartAddress, uint16 u16nBytes, Byte * ResponseBuffer,  uint16 Timeout_mS );

Byte SerialSimpleReadMemory( Byte bCellNo, uint8 ChannelId, uint32 u32StartAddress, uint16 u16nBytes, Byte * ResponseBuffer,  uint16 Timeout_mS );


protocol_rc SendCmdAndReceiveResponse( Byte bCellNo, uint8 u8ChannelId, uint8 Cmd[], uint16 u16CmdLength, uint8 ResponseBuffer[], uint16 *pu16ResponseLength, uint16 Timeout_mS);


// CommandBlock is transferred to device in segments of DrivePacketSize (performing serial line turn-around and ack checking)
// (This call will block until u8CommandBlock successfully sent to drive, unrecoverable problem occurs, or timeout waiting for ack from drive)
protocol_rc BulkSerialSendCmd (
							Byte bCellNo,
              uint8  u8ChannelId,             // 0 or 1 (identifies which UART, ie which drive)
              uint16 u16CommandLen,           // total num bytes in command block
              const  uint8 u8CommandBlock[]   // caller's command block 
                // pu8BadAckValue OUTPUT parameter removed - use new BulkSerialGetLastError
              );    

             
// Every command has a response of variable length
// (This call will block until drive sends the requested number of bytes (or timeout))
protocol_rc BulkSerialReadResponse(
							Byte bCellNo,
              uint8  u8ChannelId,             // 0 or 1 (identifies which UART, ie which drive)
              uint16 *pu16ResponseLen,        // INPUT - num bytes to read from drive (and, in the case of xyrcTimeoutOnBlockedRead, output = number of bytes actually received)
              uint8  u8ResponseBuffer[],       // caller's response block buffer - method fills with data from drive
              uint16 Timeout_mS
              );        
              

protocol_rc SendPktAndWaitForACK(  Byte bCellNo, uint8 u8ChannelId, const uint8 u8PktData[ ], uint8 u8PacketSize );

void	FlushRxUART( Byte bCellNo, uint8 u8ChannelId );     

char * CONVERT_PROTOCOLRC_TO_PSZ ( protocol_rc prc);

protocol_rc BulkSerialReadResponse_OverCommitCase(  Byte bCellNo, uint8 u8ChannelId, uint16 *pu16ResponseLen, uint8 u8ResponseBuffer[], uint16* pnReceivesForThisResponse, uint16 Timeout_mS);

bool ValidateChecksum( uint8 Response[], uint16 u16ResponseLength);

void DisplayData( uint8 u8Data[], uint16 u16DataLength);
void CalculateCmdChecksum( uint8 Cmd[], uint16 u16CmdLength );   // u16CmdLength must INCLUDE space for checksum!













