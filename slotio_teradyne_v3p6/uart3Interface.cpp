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
#include "tcTypes.h"

//----------------------------------------------------------------------
Word getMaxChunkSize()
//----------------------------------------------------------------------
{
    return 16*1024;
}


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
}

Byte siSerialSendReceiveComp(Byte bCellNo, Word wTimeoutInMillSec){
  Byte ack = success;
  return ack;
}

#ifdef __cplusplus
}
#endif
