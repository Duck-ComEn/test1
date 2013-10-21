#ifndef _LIBU3_H_
#define _LIBU3_H_

#include "tcTypes.h"
#include "commonEnums.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * <pre>
 * Description:
 *   The SCSI Cmd request allows the host to send any SCSI command to 
 *   the drive for execution.  The header provides the length of the command 
 *   to be sent and a checksum for the command data.
 *
 *   The general sequence of events (without errors) would be as follows:
 *     - The host sends a SCSI Cmd request header to the drive.
 *     - Drive receives the header and checks the header checksum.
 *     - Drive responds with ack (Data Requested) and sets up the UART hardware for data transfer using DMA if necessary.
 *     - Host sends SCSI command as data.
 *     - After the transfer drive checks checksum and the request and responds with ack (Busy).
 *     - The Host at this point will use a Query request, Read request, or Write request depending on the SCSI operation that was initiated.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wLength - INPUT - bCmd size
 *   bCmd - INPUT - SCSI command block
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 * </pre>
 *****************************************************************************/
Byte siSCSICmdRequest(Byte bCellNo, Word wLength, Byte *bCmd, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   The Query request may be used at any time to obtain the status of the drive.
 *   The drive will acknowledge the request header with a byte that conveys the 
 *   status information.  Typically, Ready, SCSI Command Complete, Busy, 
 *   Data Requested, Data Available, Sense Available, or TMF Complete will be 
 *   the acknowledgments used although others could be returned due to errors 
 *   for example.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   bFlagByte - INPUT - flag (see note)
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   The flags byte currently has one bit/flag defined. At the end of a read 
 *   data transfer the host needs a way to let the drive know that the last 
 *   block of data was received correctly so the drive may continue with status. 
 *   If a query is sent with the low order bit of this byte on, that indicates to 
 *   the drive the the last transfer was received ok and that the drive no longer 
 *   needs to keep the data for the previous sequence number as there will be 
 *   no retry necessary.  (For any block other than the last block of data 
 *   the drive finds out the data was received correctly by receiving 
 *   the Read Data request with the next sequence number.)
 *
 *       Bit Value       Meaning
 *       0x01            Previous data received ok.
 * </pre>
 *****************************************************************************/
Byte siQueryRequest(Byte bCellNo, Byte bFlagByte, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   For SCSI commands that will return data to the host the Read Data request 
 *   is used to read the data from the drive.  The host can limit the amount 
 *   of data that the drive will return for a read request by specifying 
 *   a length using the Set UART Parameters request.  The sequence number 
 *   in the request/response can be used to make sure the host and drive are 
 *   in sync on which block of data is being transferred.  For the first block 
 *   of data the host should set the sequence to 1, and the drive should return 
 *   that same number for the host to verify.  If errors occur it will allow 
 *   the host to specify the block to retry so that an entire transfer 
 *   is not ruined because of an error on one block.  If the drive is not 
 *   ready with the data it can return ack Busy, the host can resend 
 *   the request until the data is ready.  When the data is available 
 *   the drive will send ack Data Available and will return a Read Data 
 *   response followed by the data.
 *
 *    The general sequence of events (without errors) would be as follows:
 *      - Host sends the Read Data request header to the drive.
 *      - Drive receives the header and checks the header checksum.
 *      - Drive responds with ack (Data Available) indicating it is retaining the line.
 *      - Drive sends Read Data response header, followed by data.
 *      - Host may repeat process for more data (incrementing the Sequence Number each time), or send a Query request for final status.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wLength - INPUT/OUTPUT - data length (bytes)
 *   wSeqNum - INPUT - sequence number
 *   bData - OUTPUT - data
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   none
 * </pre>
 *****************************************************************************/
Byte siReadDataRequest(Byte bCellNo, Word *wLength, Word wSeqNum, Byte *bData, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   For SCSI commands that will require data from the host the Write Data request 
 *   is used to write the data to the drive.  The sequence number in the request 
 *   can be used to make sure the host and drive are in sync on which block of 
 *   data is being transferred.  For the first block of data the host should 
 *   set the sequence to 1.  If errors occur it will allow the host to specify 
 *   the block to retry so that an entire transfer is not ruined because of 
 *   an error on one block.  If the drive is not ready for the data it can 
 *   return a Busy ack, the host can resend the request until the drive is ready.
 *   When the drive is ready for the data it will ack the request header 
 *   (Data Requested) and wait for the data transfer.
 *
 *    The general sequence of events (without errors or busy) would be as follows:
 *      - Host sends the Write Data request header to the drive.
 *      - Drive receives the header and checks the header checksum.
 *      - Drive responds with ack (Data Requested) and sets up the UART hardware for data transfer using DMA if necessary.
 *      - Host sends data.
 *      - After the transfer drive checks checksum and responds with ack (Ready or Busy).
 *      - Host may repeat process for more data (incrementing the Sequence Number each time), or send Query request for final status.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wLength - INPUT - data length (bytes)
 *   wSeqNum - INPUT - sequence number
 *   bData - INPUT - data
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   none
 * </pre>
 *****************************************************************************/
Byte siWriteDataRequest(Byte bCellNo, Word wLength, Word wSeqNum, Byte *bData, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   The Abort SCSI Cmd request allows the host to abort the current (if any) 
 *   SCSI command handling being performed on the UART interface.  This request 
 *   may also be used to put the UART SCSI command handling into a known state 
 *   after an error or misscommunication.  The drive will respond to a correctly 
 *   formed/received abort request with the ack Ready. 
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   none
 * </pre>
 *****************************************************************************/
Byte siAbortSCSICmdRequest(Byte bCellNo, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   The Get UART Parameters request is used by the host to retrieve a list of 
 *   parameters related to the operation of the UART interface. The drive will 
 *   respond to the request with the ack Data Available and will return 
 *   a Get UART Parameters response followed by the parameter data.  
 *
 *   The general sequence of events (without errors) would be as follows:
 *     - Host sends the Get UART Parameters request header to the drive.
 *     - Drive receives the header and checks the header checksum.
 *     - Drive responds with ack (Data Available) indicating it is retaining the line.
 *     - Drive sends Get UART Parameters response header, followed by parameter data.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   bData - OUTPUT - data
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   For information on the parameter data returned see siSetUARTParametersRequest
 * </pre>
 *****************************************************************************/
// Byte siGetUARTParametersRequest(Byte bCellNo,  GETSETU3PARAM *bData, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   The Set UART Parameters request is used by the host to modify parameters 
 *   related to the operation of the UART interface. When the drive is ready 
 *   for the data it will ack the request header (Data Requested) and wait 
 *   for the data transfer.  The drive may not allow all parameters to 
 *   be changed, for example the "UART Protocol Version" or 
 *   "Maximum blocksize for Write requests" which will typically be based on 
 *   hardware/firmware limitations.  If there are any errors reported 
 *   in the request or the data the drive will make no changes to any values.
 *   The drive will not change to any of the new setting(s) until after 
 *   the ack is sent for the data.
 *
 *   The general sequence of events (without errors) would be as follows:
 *     - Host sends the Set UART Parameters request header to the drive.
 *     - Drive receives the header and checks the header checksum.
 *     - Drive responds with ack (Data Requested) and sets up the UART hardware for data transfer using DMA if necessary.
 *     - Host sends UART Parameter data.
 *     - After the transfer drive checks checksum and setting values and responds with ack (Ready).
 *     - Drive changes to use new UART setting(s).
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   bData - INPUT - data
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   Get/Set UART Parameters data 
 *       Word #  Data    Description                                                        
 *       1       0x0005  UART Protocol Version (read only).
 *       2       0x????  UART Line Speed (see Table 15) (read/write).
 *       3       0x????  Ack Delay in microseconds (read/write).
 *       4,5     0x0000
 *               0x????  Maximum blocksize (bytes) for Write requests (read only).
 *       6,7     0x0000
 *               0x????  Maximum blocksize (bytes) for Read requests (read/write).
 *       8       0x????  DMA Timeout in milliseconds (read/write).
 *       9       0x????  FIFO Timeout in milliseconds (read/write).
 *       10,11   0x????
 *               0x????  Transmit segment size.  The drive can break up 
 *                       long transfers to the host and this specifies 
 *                       the size of each 'segment' of data.  Between 
 *                       segments a delay is inserted (see next entry).
 *                       This allows hosts with a limited amount of 
 *                       buffering to pace the data being transmitted 
 *                       to them without the overhead of a smaller 
 *                       blocksize (read/write).
 *       12      0x????  Inter-segment delay in microseconds (read/write).
 *
 *   line speed format
 *       Code    UART Speed
 *       0x0000  115,200 bps (Legacy UART - Enterprise)
 *       0x0001  76,800 bps
 *       0x0002  57,600 bps
 *       0x0003  38,400 bps
 *       0x0004  28,800 bps
 *       0x0005  19,200 bps
 *       0x0006  14,400 bps
 *       0x0007  9,600 bps
 *       0x0008  1,843,200 bps (Legacy UART - C&C)
 *       0x0009  1,228,800 bps
 *       0x000A  921,600 bps
 *       0x000B  614,400 bps
 *       0x000C  460,800 bps
 *       0x000D  307,200 bps
 *       0x000E  230,400 bps
 *       0x000F  153,600 bps
 *       0x0010  2.083... Mbps (16.6666Mbps / 8)
 *       0x0011  2.380... Mbps (16.6666Mbps / 7)
 *       0x0012  2.777... Mbps (16.6666Mbps / 6)
 *       0x0013  3.333... Mbps (16.6666Mbps / 5)
 *       0x0014  4.166... Mbps (16.6666Mbps / 4)
 *       0x0015  5.555... Mbps (16.6666Mbps / 3)
 *       0x0016  8.333... Mbps (16.6666Mbps / 2)
 *       0x0017  11.11... Mbps (16.6666Mbps / 1.5)
 *       0x0018  16.66... Mbps (16.6666Mbps / 1)
 * </pre>
 *****************************************************************************/
//Byte siSetUARTParametersRequest(Byte bCellNo, GETSETU3PARAM *bData, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   The Set UART2 Mode request will switch the UART interface that 
 *   the request was sent to back to UART2 protocol. If a valid request 
 *   is received the drive will respond with the ack Ready and will 
 *   immediately switch to UART2 mode at the default speed.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   none
 * </pre>
 *****************************************************************************/
Byte siSetUart2ModeRequest(Byte bCellNo, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   The Memory Read request is used to read the contents of any memory 
 *   on the drive.   This includes all hardware registers accessible from the IP.
 *
 *   The general sequence of events (without errors) would be as follows:
 *     - Host sends the Memory Read request header to the drive.
 *     - Drive receives the header and checks the header checksum.
 *     - Drive responds with ack (Data Available) indicating it is retaining the line.
 *     - Drive sends Memory Read response header, followed by the requested data.
 *   The maximum amount of data that can be read with this command is limited to 16,384 bytes.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wLength - INPUT - data size (bytes)
 *   dwAddress - INPUT - Memory address to read the data
 *   bData - OUTPUT - data
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   none
 * </pre>
 *****************************************************************************/
Byte siMemoryReadRequest(Byte bCellNo, Word wLength, Dword dwAddress, Byte *bData, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   The Memory Write request will write the specified data to any normally 
 *   writable area of the drive, including the hardware registers.  Data is 
 *   to be sent in a Big Endian format on a byte basis.
 *
 *   The general sequence of events (without errors) would be as follows:
 *     - Host sends the Memory Write request header to the drive.
 *     - Drive receives the header and checks the header checksum.
 *     - Drive responds with ack (Data Requested) and sets up the UART hardware for data transfer using DMA if necessary.
 *     - Host sends write data.
 *     - After the transfer the drive checks the checksum, writes the data to the requested location, and responds with ack (Ready).
 *     - Note: unless the (Ready) ack is received; the data may or may not have been successfully written.
 *
 *  The maximum amount of data that can be written with this command is limited to 16,384 bytes.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wSize - INPUT - data size (bytes)
 *   bData - INPUT - data
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   none
 * </pre>
 *****************************************************************************/
Byte siMemoryWriteRequest(Byte bCellNo, Word wLength, Dword dwAddress, Byte *bData, Dword dwTimeoutInMillSecc);

/**
 * <pre>
 * Description:
 *   The Echo request can be used to ensure that a good data connection has 
 *   been made from the host to the drive. It returns the first 56 bytes of 
 *   standard inquiry data; except bytes 44 and 45 contain the Code ID and 
 *   bytes 46 and 47 contain the Compatibility ID 
 *   (assuming byte numbering starts at 0).
 *
 *   The general sequence of events (without errors) would be as follows:
 *     - Host sends the Echo request header to the drive.
 *     - Drive receives the header and checks the header checksum.
 *     - Drive responds with ack (Data Available) indicating it is retaining the line.
 *     - Drive sends Echo response header, followed by the requested data.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   wLength - INPUT - data size (bytes) - maximum 38 bytes
 *   bData - OUTPUT - data
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   none
 * </pre>
 *****************************************************************************/
//Byte siEchoRequest(Byte bCellNo, Word wLength, Byte *bData, Dword dwTimeoutInMillSec);
Byte siEchoRequest(Byte bCellNo, Byte *bData, Dword dwTimeoutInMillSec);
/**
 * <pre>
 * Description:
 *   The Get Drive State request can be used by the host to check on 
 *   the status of a long running SCSI command (such as a manufacturing command).
 *   The drive will respond to the request with the ack Data Available and 
 *   will return a Get Drive State response followed by the state data.  
 *   For information on the state data returned see Table 24: Drive State data.
 *
 *   The general sequence of events (without errors) would be as follows:
 *     - Host sends the Get Drive State request header to the drive.
 *     - Drive receives the header and checks the header checksum.
 *     - Drive responds with ack (Data Available) indicating it is retaining the line.
 *     - Drive sends Get Drive State response header, followed by parameter data.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   bData - OUTPUT - data
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   The data offset and length field values must be a multiple of two bytes.
 *
 *       Byte    Description
 *       0-1     Drive state version (0x0001)
 *       2-3     Operating State (Equivalent to the Operating State field of SCSI Inquiry page 3.)
 *       4-5     Functional Mode (Equivalent to the Functional Mode field of SCSI Inquiry page 3.)
 *       6-7     Degraded Reason (Equivalent to the Degraded Reason field of SCSI Inquiry page 3.)
 *       8-9     Broken Reason (Equivalent to the Broken Reason field of SCSI Inquiry page 3.)
 *       10-11   Reset Cause (Value definition is beyond the scope of this document.)
 *       12-13   Showstop state:
 *                 0x0000 - No Showstop has occurred.
 *                 0x0001 - Showstop has occurred and drive is 'stopped'.
 *                 0x0002 - Showstop has occurred and the drive is attempting to reset itself.
 *       14-15   Showstop reason (Value definition is beyond the scope of this document.)
 *       16-19   Showstop value (Value definition is beyond the scope of this document.)
 * </pre>
 *****************************************************************************/
Byte siGetDriveStateRequest(Byte bCellNo, GET_DRIVE_STATE_DATA *bData, Dword dwTimeoutInMillSec);

/**
 * <pre>
 * Description:
 *   The Drive Reset request initiates a hard reset of the drive. The drive will 
 *   respond to the reset with the ack Ready and call the hardware reset function.
 * Arguments:
 *   bCellNo - INPUT - cell id. range/mapping is different for each hardware
 *   dwTimeoutInMillSec - INPUT - time out value in mill-seconds
 * Return:
 *   zero if success. otherwise, non-zero value.
 * Note:
 *   none
 * </pre>
 *****************************************************************************/
//Byte siDriveResetRequest(Byte bCellNo, Dword dwTimeoutInMillSec);

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
Byte siExecuteSCSICommand(CCB *cb, void *data, Byte bCellNo);

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
Byte siIssueSCSICommand(Byte bCellNo, Word wSize, Byte *bData, Dword dwTimeoutInMillSec);

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
Byte siQueryRequestWaitForCommandComplete(Byte bCellNo, Dword dwTimeoutInMillSec);

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
Byte siReadDataTransfer(Byte bCellNo, Dword dwLength, Byte *bData, Dword dwTimeoutInMillSec);

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
Byte siWriteDataTransfer(Byte bCellNo, Dword dwLength, Byte *bData, Dword dwTimeoutInMillSec);

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
Byte siSetDriveResponseDelayTimeForAck(Byte bCellNo, Word wDelayTimeInMicroSec);

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
Byte siSetMaxReadBlockSize(Byte bCellNo, Word wMaxBlockSizeForRead);

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
Byte siSetUartLineSpeed(Byte bCellNo, Dword dwLineSpeedInBps);

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
Byte siSetUartInterSegmentDelay(Byte bCellNo, Word wInterSegmentDelay);

/**
 * <pre>
 * Description:
 *   Execute SCSI CCB command on UART-3 protocol
 * Arguments:
 *   CCB_DEBUG  *cb   : INPUT - CCB structure as below.
 *    - unsigned char cdblength;         // CDB length
 *    - unsigned char senselength;       // sense data length
 *    - unsigned char cdb[CDBSIZE];      // CDB block
 *    - unsigned char sense[SENSESIZE];  // sense data block
 *    - unsigned long read;              // read data length
 *    - unsigned long write;             // write data length
 *    - long timeout;                    // timeout [msec]
 *
 *   void *data : INPUT - Transfer data buffer pointer.
 *
 * Return:
 *  T.B.D.
 *
 * </pre>
 *****************************************************************************/
long execCCBCommand(CCB_DEBUG *cb, void *data);

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
Byte siChangeToUart3_debug(Byte bCellNo, Dword dwUartLineSpeed, Word wTimeoutInMillSec);


#ifdef __cplusplus
}
#endif

#endif
