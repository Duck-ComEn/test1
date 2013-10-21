#define WAPI_RPC_LAST_ATTEMPT 2
#define WAPI_CALL_RPC_WITh_RETRY(SIO_API)  	SIO_API;             \
	if (status == TER_Status_rpc_fail) {                         \
	   for(int cnt = 1; cnt <= WAPI_RPC_LAST_ATTEMPT; ++cnt) {   \
    	   rc = terSio2Reconnect(Byte bCellNo);                  \
		   if( rc != Success ) {                                 \
	    	   TLOG_ERROR("ERROR: Could not reconnect to socket \n"); \ return(TerError); \
           }                                          \
	       SIO_API;                                   \
		   if( status != TER_Status_rpc_fail ) break; \
	   } \
	}






WAPI_CALL_RPC_WITh_RETRY((status = ns2->TER_GetSlotStatus(wapi2sio(bCellNo), &slotStatus)))
