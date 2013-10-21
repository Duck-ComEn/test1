#ifndef _LIBSM_H_
#define _LIBSM_H_

#ifdef __cplusplus
extern "C"
{
#endif

// sync message send/receive TIMEOUT time define
  enum eSMTIMEOUT { SM_SD_TIMEOUT = 0,
                    SM_RV_TIMEOUT = 0,
                    SM_CS_SD_TIMEOUT = 0,
                    SM_CS_RV_TIMEOUT = 0,        //
                    SM_TC_SD_TIMEOUT = 0,
                    /**
                    * When resume test, it can take 40 min (25->65degC + mergin 20 min) in maximum case at ramp rate = 1degC/min
                    *
                    * < Update on April/20th/2011 >
                    * Neptune-II Enterprise needs more time for ramp up with drive power off during resume temperature recovery.
                    * About 60 minutes is taken at initial trial, so 120minutes is set with margin.
                    *****************************************************************************/
                    SM_TC_RV_TIMEOUT = 7200,
                    SM_SH_SD_TIMEOUT = 0,
                    SM_SH_RV_TIMEOUT = 0,
                    SM_TS_SD_TIMEOUT = 0,
                    SM_TS_RV_TIMEOUT = 0
                  };
// sync manager error code for send/receive define
  enum eSMERROR {
    SM_NO_ERROR         = 0,
    SM_FAILURE_ERROR       = 1,
    SM_BUFFER_FULL_ERROR  = 2,
    SM_BUFFER_EMPTY_ERROR = 2,
    SM_MUTEX_UNLOCK_ERROR = 3,
    SM_MUTEX_LOCK_ERROR   = 4,
    SM_TIMEOUT_ERROR      = 5,
    SM_ANOTHER_MESSAGE    = 6
  };
// sync manager error code for query define
  enum eSMERROR_Q {
    SM_MESSAGE_AVAILABLE   = 0,
//                        SM_FAILURE_ERROR       = 1,
    SM_MESSAGE_UNAVAILABLE = 2
  };

// sync message send/reveive type code define
  // chamber script send message define
  enum eSM_CS_SD_MESSAGE {
    SM_CS_SD_CSREADY       = 1,    //chamber script is ready to testcase
    SM_CS_SD_JMPACK        = 2,    //acknowledge for jmp request
    SM_CS_SD_PREPDATA      = 3,    //request for prepdata
    SM_CS_SD_FINALSTART    = 4     //request for final start
  };
  // chamber script receive message define
  enum eSM_CS_RV_MESSAGE {
    SM_CS_RV_JMPREQ        = 2    //acknowledge for jmp request
  };
  // testcase send message define
//  enum eSM_TC_SD_MESSAGE{
//     };
  // testcase receive message define
  enum eSM_TC_RV_MESSAGE {
    SM_TC_RV_CSREADY       = 1    //chamber script is ready
  };
  // signal handler send message define
  enum eSM_SH_SD_MESSAGE {
    SM_SH_SD_ABORT         = 1    // signal handler caught signal
  };
  // signal handler receive message define
//  enum eSM_SH_RV_MESSAGE{
//   };
  // test script send message define
  enum eSM_TS_SD_MESSAGE {
    SM_TS_SD_FINISH        = 1,    //testscript finish
    SM_TS_SD_JMPREQ        = 2    //acknowledge for jmp request
  };
  // test script receive message define
  enum eSM_TS_RV_MESSAGE {
    SM_TS_RV_ABORT         = 1,    //chamber script caught signal
    SM_TS_RV_JMPACK        = 2,   //acknowledge for jmp request
    SM_TS_RV_PREPDATA      = 3,   //request for prepdata
    SM_TS_RV_FINALSTART    = 4    //request for final start
  };
  extern pthread_mutex_t mutex_syncmanager;


  /**
   * <b>
   * Version: V1.00 <br>
   * Date:    28 Sep 2010</b><p>
   *
   * This function is SYNC manager thread. <p>
   *
   ***************************************************************************************/
  void *runSyncManagerThread(void *arg);


  /**
   * <pre>
   * Description:
   *   To get sync buffers of send and receive 2-way communication
   * Arguments:
   *   **sendBuffer - OUTPUT - pointer to pointer to sync buffer for send
   *   **receiveBuffer - OUTPUT - pointer to pointer to sync buffer for receive
   * Return:
   *   zero if no error. otherwise, error
   * Note:
   *   only one set of the buffers will be provided for each threads.
   *   therefore you get same pointer if you call this API twice in same thread.
   * </pre>
   *****************************************************************************/
  Byte smGetBuffer(SM_BUFFER **smSendBuffer, SM_BUFFER **smReceiveBuffer);


  /**
   * <pre>
   * Description:
   *   To send message to specified sync buffer
   * Arguments:
   *   *smBuffer - INPUT - pointer to sync buffer
   *   *smMessage - INPUT - pointer to message to be sent
   * Return:
   *   zero if no error. otherwise, error or no space available
   * Note:
   *   this is non-blocking function that does not wait until the buffer is not full
   * </pre>
   *****************************************************************************/
  Byte smSendMessage(SM_BUFFER *sendBuffer, SM_MESSAGE *smMessage);


  /**
   * <pre>
   * Description:
   *   To receive message from specified sync buffer
   * Arguments:
   *   *smBuffer - INPUT - pointer to sync buffer
   *   *smMessage - INPUT - pointer to message to be sent
   * Return:
   *   zero if no error. otherwise, error or no message available
   * Note:
   *   this is non-blocking function that does not waits until buffer is not empty
   * </pre>
   *****************************************************************************/
  Byte smReceiveMessage(SM_BUFFER *receiveBuffer, SM_MESSAGE *smMessage);


  /**
   * <pre>
   * Description:
   *   To send message to syncmanager
   * Arguments:
   *   *smMessage - INPUT - pointer to message to be sent
   * Return:
   *   zero if no error. otherwise, error
   * Note:
   *   this is non-blocking function that does not wait until the buffer is not full
   * </pre>
   *****************************************************************************/
  Byte smSendSyncMessage(SM_MESSAGE *smMessage, Dword timeout);

  /**
   * <pre>
   * Description:
   *   To receive message from syncmanager
   * Arguments:
   *   timeout     - INPUT - int data as timeout time in sec
   *   *smMessage - INPUT - pointer to message to be sent
   * Return:
   *   zero if no error. otherwise, error
   * Note:
   *   this is non-blocking function that does not wait until the buffer is not full
   * </pre>
   *****************************************************************************/
  Byte smReceiveSyncMessage(Dword timeout, SM_MESSAGE *smMessage);
  /**
   * <pre>
   * Description:
   *   To query specified message
   * Arguments:
   *   otype  - INPUT - expected type data to be sent
   * Return:
   *   0:there isn't expected message. 1:there is expected message
   * </pre>
   *****************************************************************************/
  Byte smQuerySyncMessage(Dword otype);
  /**
   * <pre>
   * Description:
   *   To query specified message in specified sync buffer
   * Arguments:
   *   otype  - INPUT - expected type data
   *   *smBuffer - INPUT - pointer to sync buffer
   * Return:
   *   0:there isn't expected message. 1:there is expected message
   * </pre>
   *****************************************************************************/
  Byte smQueryMessage(Dword otype, SM_BUFFER *smBuffer);

#ifdef __cplusplus
}
#endif

#endif
