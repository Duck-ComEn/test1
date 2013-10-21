#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>  // for getpid, sleep, usleep
#include "../testcase/tcTypes.h"
#include "../slotio/libsi.h"
#include "../testscript/libts.h"
#include "libsm.h"
#include "sm.h"

pthread_mutex_t mutex_syncmanager;


//----------------------------------------------------------------------
void *runSyncManagerThread(void *arg)
//----------------------------------------------------------------------
{

  TCPRINTF("Entry: pthread_t,%08lxh", (unsigned long)pthread_self());

  for (;;) {
    int ret = 0;
    // mutex lock
    ret = pthread_mutex_lock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error mutex lock %d", ret);
      goto ERROR_RUN_SYNC_MANAGER_THREAD;
    }

    /***************************************************************************************/
    /************************* initialize  *************************************************/
    /***************************************************************************************/

    SM_BUFFER *smBuffer = NULL;
    SM_MESSAGE smMessage;
    Byte rc = SM_NO_ERROR;
    Byte loop = 0;
    Byte tsno = 0;

    /**************************************************************************************/
    /************* testcase send  message ***********************************************/
    /**************************************************************************************/


    smBuffer = &tccb.tc.smSendBuffer;
    rc = smReceiveSyncMessageForSM(smBuffer, &smMessage);
    if (rc == SM_NO_ERROR) {
      switch (smMessage.type) {
      case 0:
        break;
      case 1:
        break;
      case 2:
        break;
      default:
        break;
      }
    } else if (rc != SM_BUFFER_EMPTY_ERROR) {
      TCPRINTF("SM_TC_RECEIVE <-- fail");//y.katayama 2010.09.29
      goto ERROR_RUN_SYNC_MANAGER_THREAD;
    }

    /*************************************************************************************/
    /*************  signal handler send message ******************************************/
    /*************************************************************************************/

    smBuffer = &tccb.sh.smSendBuffer;
    rc = smReceiveSyncMessageForSM(smBuffer, &smMessage);
    if (rc == SM_NO_ERROR) {
      switch (smMessage.type) {
      case 0:
        break;
      case SM_SH_SD_ABORT:               //Signal Handler receive signal and kill task
        for (loop = 0; loop < tccb.optionNumOfTestScriptThread; loop++) {
          smBuffer = &tccb.ts[loop].smReceiveBuffer;
          smMessage.type = SM_TS_RV_ABORT;
          while (smBuffer->numOfMessage > 0) {
            rc = smReceiveSyncMessageForSM(smBuffer, &smMessage);
            if (rc != SM_NO_ERROR) {  //rc==2:buffer empty
              TCPRINTF("SM_SH_RECEIVE <-- fail");//y.katayama 2010.09.29
              goto ERROR_RUN_SYNC_MANAGER_THREAD;
            }
          }
          smSendSyncMessageForSM(smBuffer, &smMessage, SM_SD_TIMEOUT);
          if (rc != SM_NO_ERROR) {
            TCPRINTF("SM_SEND <-- fail");//y.katayama 2010.09.29
            goto ERROR_RUN_SYNC_MANAGER_THREAD;
          }
        }
        break;
      case 2:
        break;
      default:
        break;
      }
    } else if (rc != SM_BUFFER_EMPTY_ERROR) {  //rc==2:buffer empty
      TCPRINTF("SM_SH_RECEIVE <-- fail");//y.katayama 2010.09.29
      goto ERROR_RUN_SYNC_MANAGER_THREAD;
    }

    /**************************************************************************************/
    /****************** chamber script send message ***************************************/
    /**************************************************************************************/


    smBuffer = &tccb.cs.smSendBuffer;
    rc = smReceiveSyncMessageForSM(smBuffer, &smMessage);
    if (rc == SM_NO_ERROR) {
      switch (smMessage.type) {
      case 0:
        break;
      case SM_CS_SD_CSREADY:         //chamber script ready to testcase
        smBuffer = &tccb.tc.smReceiveBuffer;
        smMessage.type = SM_TC_RV_CSREADY;
        rc = smSendSyncMessageForSM(smBuffer, &smMessage, SM_SD_TIMEOUT);
        if (rc != SM_NO_ERROR) {
          TCPRINTF("SM_SEND <-- fail");//y.katayama 2010.09.29
          goto ERROR_RUN_SYNC_MANAGER_THREAD;
        }
        break;
      case SM_CS_SD_JMPACK:          //chamber script jump acknowledge
        for (loop = 0; loop < tccb.optionNumOfTestScriptThread; loop++) {
          smBuffer = &tccb.ts[loop].smReceiveBuffer;
          smMessage.type = SM_TS_RV_JMPACK;
          smSendSyncMessageForSM(smBuffer, &smMessage, SM_SD_TIMEOUT);
          if (rc != SM_NO_ERROR) {
            TCPRINTF("SM_SEND <-- fail");//y.katayama 2010.09.29
            goto ERROR_RUN_SYNC_MANAGER_THREAD;
          }
        }
        break;
      case SM_CS_SD_PREPDATA:      //chamber_script prepdata message
        for (loop = 0; loop < tccb.optionNumOfTestScriptThread; loop++) {
          smBuffer = &tccb.ts[loop].smReceiveBuffer;
          smMessage.type = SM_TS_RV_PREPDATA;
          smSendSyncMessageForSM(smBuffer, &smMessage, SM_SD_TIMEOUT);
          if (rc != SM_NO_ERROR) {
            TCPRINTF("SM_SEND <-- fail");//y.katayama 2010.09.29
            goto ERROR_RUN_SYNC_MANAGER_THREAD;
          }
        }
        break;
      case SM_CS_SD_FINALSTART:      //chamber_script prepdata message
        for (loop = 0; loop < tccb.optionNumOfTestScriptThread; loop++) {
          smBuffer = &tccb.ts[loop].smReceiveBuffer;
          smMessage.type = SM_TS_RV_FINALSTART;
          smSendSyncMessageForSM(smBuffer, &smMessage, SM_SD_TIMEOUT);
          if (rc != SM_NO_ERROR) {
            TCPRINTF("SM_SEND <-- fail");//y.katayama 2010.09.29
            goto ERROR_RUN_SYNC_MANAGER_THREAD;
          }
        }
        break;
      default:
        break;
      }
    } else if (rc != SM_BUFFER_EMPTY_ERROR) {  //rc==2:buffer empty
      TCPRINTF("SM_CS_RECEIVE <-- fail");//y.katayama 2010.09.29
      goto ERROR_RUN_SYNC_MANAGER_THREAD;
    }

    /**************************************************************************************/
    /****************** test script send message ***************************************/
    /**************************************************************************************/

    for (loop = 0; loop < tccb.optionNumOfTestScriptThread; loop++) {
      // test script
      smBuffer = &tccb.ts[loop].smSendBuffer;
      rc = smReceiveSyncMessageForSM(smBuffer, &smMessage);
      if (rc == SM_NO_ERROR) {
        switch (smMessage.type) {
        case 0:
          break;
        case SM_TS_SD_FINISH:
          tccb.ts[loop].isFinished = 1;
          TCPRINTF("TestScript[%d] finish!!!!! tccb.ts[%d].isFinished = %d ", loop, loop, tccb.ts[loop].isFinished );//y.katayama 2010.09.29
          break;
        case SM_TS_SD_JMPREQ:
          strcpy(tccb.ts[loop].requestLabel, smMessage.string);
          Byte isReadyToAck = 1;
          for (tsno = 0;tsno < tccb.optionNumOfTestScriptThread;tsno++) {
            if (tccb.ts[tsno].isFinished) {
              //do nothing
            } else if (strcmp(tccb.ts[tsno].requestLabel, smMessage.string) != MATCH) {
              isReadyToAck = 0;
            }
          }
          if (isReadyToAck) {
            smBuffer = &tccb.cs.smReceiveBuffer;
            smMessage.type = SM_CS_RV_JMPREQ;
            rc = smSendSyncMessageForSM(smBuffer, &smMessage, SM_SD_TIMEOUT);
            if (rc != SM_NO_ERROR) {
              TCPRINTF("SM_CS_SEND <-- fail");//y.katayama 2010.09.29
              goto ERROR_RUN_SYNC_MANAGER_THREAD;
            }

          }
          break;
        default:
          break;
        }
      } else if (rc != SM_BUFFER_EMPTY_ERROR) {
        TCPRINTF("SM_RECEIVE <-- fail");//y.katayama 2010.09.29
        goto ERROR_RUN_SYNC_MANAGER_THREAD;
      }
    }
    // mutex unlock
    ret = pthread_mutex_unlock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error %d", ret);
      break;
    }
    // sleep if no message is available
    sleep(5);
  }

ERROR_RUN_SYNC_MANAGER_THREAD:
  tcExit(EXIT_FAILURE);
  return NULL;
}


//----------------------------------------------------------------------
Byte smGetBuffer(SM_BUFFER **smSendBuffer, SM_BUFFER **smReceiveBuffer)
//----------------------------------------------------------------------
{
  Byte rc = SM_FAILURE_ERROR;
  pthread_t thread = pthread_self();

  *smSendBuffer = NULL;
  *smReceiveBuffer = NULL;

  if (pthread_equal(thread, tccb.tc.thread)) {
    *smSendBuffer = &tccb.tc.smSendBuffer;
    *smReceiveBuffer = &tccb.tc.smReceiveBuffer;
    rc = 0;

  } else if (pthread_equal(thread, tccb.sh.thread)) {
    *smSendBuffer = &tccb.sh.smSendBuffer;
    *smReceiveBuffer = &tccb.sh.smReceiveBuffer;
    rc = 0;

  } else if (pthread_equal(thread, tccb.cs.thread)) {
    *smSendBuffer = &tccb.cs.smSendBuffer;
    *smReceiveBuffer = &tccb.cs.smReceiveBuffer;
    rc = 0;
  } else if (pthread_equal(thread, tccb.sm.thread)) {
    *smSendBuffer = &tccb.sm.smSendBuffer;
    *smReceiveBuffer = &tccb.sm.smReceiveBuffer;
    rc = 0;

  } else {
    int i = 0;
    for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
      if (pthread_equal(thread, tccb.ts[i].thread)) {
        *smSendBuffer = &tccb.ts[i].smSendBuffer;
        *smReceiveBuffer = &tccb.ts[i].smReceiveBuffer;
        rc = 0;
        break;
      }
    }
  }

  return rc;
}


//----------------------------------------------------------------------
Byte smSendMessage(SM_BUFFER *smSendBuffer, SM_MESSAGE *smMessage)
//----------------------------------------------------------------------
{
  Byte rc = SM_FAILURE_ERROR;
  int ret = 0;
  Byte *caller;
  Byte tsid;

  ret = pthread_mutex_lock(&mutex_syncmanager);
  if (ret) {
    TCPRINTF("error mutex lock %d", ret);
    goto ERROR;
  }
// for log
  pthread_t thread = pthread_self();
  if (pthread_equal(thread, tccb.sm.thread)) {
    if (smSendBuffer == &tccb.sh.smReceiveBuffer) {
      caller = "SH";
    } else if (smSendBuffer == &tccb.cs.smReceiveBuffer) {
      caller = "CS";
    } else if (smSendBuffer == &tccb.tc.smReceiveBuffer) {
      caller = "TC";
    } else if (smSendBuffer == &tccb.sm.smReceiveBuffer) {
      caller = "SM";
    } else if (smSendBuffer == &tccb.sm.smSendBuffer) {
      caller = "SM";
    } else {
      tsid = 0;
      for (tsid = 0 ; tsid < tccb.optionNumOfTestScriptThread ; tsid++) {
        if (smSendBuffer == &tccb.ts[tsid].smReceiveBuffer) {
          caller = "TS";
          break;
        }
      }
      if (tsid == tccb.optionNumOfTestScriptThread) {
        TCPRINTF("error!! SendBuffer is unrecognized");
        goto ERROR;
      }
    }
  } else {
    if (smSendBuffer == &tccb.sh.smSendBuffer) {
      caller = "SH";
    } else if (smSendBuffer == &tccb.cs.smSendBuffer) {
      caller = "CS";
    } else if (smSendBuffer == &tccb.tc.smSendBuffer) {
      caller = "TC";
    } else if (smSendBuffer == &tccb.sm.smReceiveBuffer) {
      caller = "SM";
    } else if (smSendBuffer == &tccb.sm.smSendBuffer) {
      caller = "SM";
    } else {
      tsid = 0;
      for (tsid = 0 ; tsid < tccb.optionNumOfTestScriptThread ; tsid++) {
        if (smSendBuffer == &tccb.ts[tsid].smSendBuffer) {
          caller = "TS";
          break;
        }
      }
      if (tsid == tccb.optionNumOfTestScriptThread) {
        TCPRINTF("error!! SendBuffer is unrecognized");
        goto ERROR;
      }
    }
  }
//

  if (smSendBuffer->numOfMessage < (sizeof(smSendBuffer->message) / sizeof(smSendBuffer->message[0]))) {
    memmove(&smSendBuffer->message[smSendBuffer->writePointer++], smMessage, sizeof(smSendBuffer->message[0]));
    if (smSendBuffer->writePointer >= (sizeof(smSendBuffer->message) / sizeof(smSendBuffer->message[0]))) {
      smSendBuffer->writePointer = 0;
    }
    smSendBuffer->numOfMessage++;
// log
    if (pthread_equal(thread, tccb.sm.thread)) {
      if (caller == (Byte *)"TS") {
        TCPRINTF("SM_TS[%d]_SEND <-- pass", (int)tsid);
        TCPRINTF("SM_TS[%d]_BUFFER  <-- rp=%d, wp=%d, num=%d", (int)tsid, (int)smSendBuffer->readPointer, (int) smSendBuffer->writePointer, (int)smSendBuffer->numOfMessage);
        TCPRINTF("SM_TS[%d]_MESSAGE <-- type=%d", (int)tsid, (int)smMessage->type);
        TCPRINTF("SM_TS[%d]_MESSAGE <-- string=%s", (int)tsid, smMessage->string);
        TCPRINTF("SM_TS[%d]_MESSAGE <-- param =%d,%d,%d,%d,%d,%d,%d,%d", (int)tsid, (int)smMessage->param[0], (int)smMessage->param[1], (int)smMessage->param[2], (int)smMessage->param[3], (int)smMessage->param[4], (int)smMessage->param[5], (int)smMessage->param[6], (int)smMessage->param[7]);
      } else {
        TCPRINTF("SM_%s_SEND <-- pass", caller);
        TCPRINTF("SM_%s_BUFFER  <-- rp=%d, wp=%d, num=%d", caller, (int)smSendBuffer->readPointer, (int)smSendBuffer->writePointer, (int)smSendBuffer->numOfMessage);
        TCPRINTF("SM_%s_MESSAGE <-- type=%d", caller, (int)smMessage->type);
        TCPRINTF("SM_%s_MESSAGE <-- string=%s", caller, smMessage->string);
        TCPRINTF("SM_%s_MESSAGE <-- param =%d,%d,%d,%d,%d,%d,%d,%d", caller, (int)smMessage->param[0], (int)smMessage->param[1], (int)smMessage->param[2], (int)smMessage->param[3], (int)smMessage->param[4], (int)smMessage->param[5], (int)smMessage->param[6], (int)smMessage->param[7]);
      }
    } else {
      if (caller == (Byte *)"TS") {
        TCPRINTF("TS[%d]_SEND <-- pass", (int)tsid);
        TCPRINTF("TS[%d]_BUFFER  <-- rp=%d, wp=%d, num=%d", (int)tsid, (int)smSendBuffer->readPointer, (int) smSendBuffer->writePointer, (int)smSendBuffer->numOfMessage);
        TCPRINTF("TS[%d]_MESSAGE <-- type=%d", (int)tsid, (int)smMessage->type);
        TCPRINTF("TS[%d]_MESSAGE <-- string=%s", (int)tsid, smMessage->string);
        TCPRINTF("TS[%d]_MESSAGE <-- param =%d,%d,%d,%d,%d,%d,%d,%d", (int)tsid, (int)smMessage->param[0], (int)smMessage->param[1], (int)smMessage->param[2], (int)smMessage->param[3], (int)smMessage->param[4], (int)smMessage->param[5], (int)smMessage->param[6], (int)smMessage->param[7]);
      } else {
        TCPRINTF("%s_SEND <-- pass", caller);
        TCPRINTF("%s_BUFFER  <-- rp=%d, wp=%d, num=%d", caller, (int)smSendBuffer->readPointer, (int)smSendBuffer->writePointer, (int)smSendBuffer->numOfMessage);
        TCPRINTF("%s_MESSAGE <-- type=%d", caller, (int)smMessage->type);
        TCPRINTF("%s_MESSAGE <-- string=%s", caller, smMessage->string);
        TCPRINTF("%s_MESSAGE <-- param =%d,%d,%d,%d,%d,%d,%d,%d", caller, (int)smMessage->param[0], (int)smMessage->param[1], (int)smMessage->param[2], (int)smMessage->param[3], (int)smMessage->param[4], (int)smMessage->param[5], (int)smMessage->param[6], (int)smMessage->param[7]);
      }
    }
    rc = SM_NO_ERROR;
  } else {
    rc = SM_BUFFER_FULL_ERROR;
  }

  ret = pthread_mutex_unlock(&mutex_syncmanager);
  if (ret) {
    TCPRINTF("error mutex unlock %d", ret);
    goto ERROR;
  }

//  TCPRINTF("Exit,%d", rc);
  return rc;

ERROR:
  tcExit(EXIT_FAILURE);
  return SM_FAILURE_ERROR;
}


//----------------------------------------------------------------------
Byte smReceiveMessage(SM_BUFFER *smReceiveBuffer, SM_MESSAGE *smMessage)
//----------------------------------------------------------------------
{
  Byte rc = SM_FAILURE_ERROR;
  int ret = 0;
  Byte *caller;
  Byte tsid;
//  TCPRINTF("Entry");
//for log
  pthread_t thread = pthread_self();
  if (pthread_equal(thread, tccb.sm.thread)) {
    if (smReceiveBuffer == &tccb.sh.smSendBuffer) {
      caller = "SH";
    } else if (smReceiveBuffer == &tccb.cs.smSendBuffer) {
      caller = "CS";
    } else if (smReceiveBuffer == &tccb.tc.smSendBuffer) {
      caller = "TC";
    } else if (smReceiveBuffer == &tccb.sm.smReceiveBuffer) {
      caller = "SM";
    } else if (smReceiveBuffer == &tccb.sm.smSendBuffer) {
      caller = "SM";
    } else {
      tsid = 0;
      for (tsid = 0 ; tsid < tccb.optionNumOfTestScriptThread ; tsid++) {
        if (smReceiveBuffer == &tccb.ts[tsid].smSendBuffer) {
          caller = "TS";
          break;
        }
      }
      if (tsid == tccb.optionNumOfTestScriptThread) {
        TCPRINTF("error!! SendBuffer is unrecognized");
        goto ERROR;
      }
    }
  } else {
    if (smReceiveBuffer == &tccb.sh.smReceiveBuffer) {
      caller = "SH";
    } else if (smReceiveBuffer == &tccb.cs.smReceiveBuffer) {
      caller = "CS";
    } else if (smReceiveBuffer == &tccb.tc.smReceiveBuffer) {
      caller = "TC";
    } else if (smReceiveBuffer == &tccb.sm.smReceiveBuffer) {
      caller = "SM";
    } else if (smReceiveBuffer == &tccb.sm.smSendBuffer) {
      caller = "SM";
    } else {
      tsid = 0;
      for (tsid = 0 ; tsid < tccb.optionNumOfTestScriptThread ; tsid++) {
        if (smReceiveBuffer == &tccb.ts[tsid].smReceiveBuffer) {
          caller = "TS";
          break;
        }
      }
      if (tsid == tccb.optionNumOfTestScriptThread) {
        TCPRINTF("error!! SendBuffer is unrecognized");
        goto ERROR;
      }
    }
  }
//
  ret = pthread_mutex_lock(&mutex_syncmanager);
  if (ret) {
    TCPRINTF("error mutex unlock %d", ret);
    goto ERROR;
  }
  if (smReceiveBuffer->numOfMessage > 0) {
    memmove(smMessage, &smReceiveBuffer->message[smReceiveBuffer->readPointer++], sizeof(*smMessage));
    if (smReceiveBuffer->readPointer >= (sizeof(smReceiveBuffer->message) / sizeof(smReceiveBuffer->message[0]))) {
      smReceiveBuffer->readPointer = 0;
    }
    smReceiveBuffer->numOfMessage--;
    // log
    if (pthread_equal(thread, tccb.sm.thread)) {
      if (caller == (Byte *)"TS") {
        TCPRINTF("SM_TS[%d]_RECEIVE <-- pass", (int)tsid);
        TCPRINTF("SM_TS[%d]_BUFFER  <-- rp=%d, wp=%d, num=%d", (int)tsid, (int)smReceiveBuffer->readPointer, (int) smReceiveBuffer->writePointer, (int)smReceiveBuffer->numOfMessage);
        TCPRINTF("SM_TS[%d]_MESSAGE <-- type=%d", (int)tsid, (int)smMessage->type);
        TCPRINTF("SM_TS[%d]_MESSAGE <-- string=%s", (int)tsid, smMessage->string);
        TCPRINTF("SM_TS[%d]_MESSAGE <-- param =%d,%d,%d,%d,%d,%d,%d,%d", (int)tsid, (int)smMessage->param[0], (int)smMessage->param[1], (int)smMessage->param[2], (int)smMessage->param[3], (int)smMessage->param[4], (int)smMessage->param[5], (int)smMessage->param[6], (int)smMessage->param[7]);
      } else {
        TCPRINTF("SM_%s_RECEIVE <-- pass", caller);
        TCPRINTF("SM_%s_BUFFER  <-- rp=%d, wp=%d, num=%d", caller, (int)smReceiveBuffer->readPointer, (int)smReceiveBuffer->writePointer, (int)smReceiveBuffer->numOfMessage);
        TCPRINTF("SM_%s_MESSAGE <-- type=%d", caller, (int)smMessage->type);
        TCPRINTF("SM_%s_MESSAGE <-- string=%s", caller, smMessage->string);
        TCPRINTF("SM_%s_MESSAGE <-- param =%d,%d,%d,%d,%d,%d,%d,%d", caller, (int)smMessage->param[0], (int)smMessage->param[1], (int)smMessage->param[2], (int)smMessage->param[3], (int)smMessage->param[4], (int)smMessage->param[5], (int)smMessage->param[6], (int)smMessage->param[7]);
      }
    } else {
      if (caller == (Byte *)"TS") {
        TCPRINTF("TS[%d]_RECEIVE <-- pass", (int)tsid);
        TCPRINTF("TS[%d]_BUFFER  <-- rp=%d, wp=%d, num=%d", (int)tsid, (int)smReceiveBuffer->readPointer, (int) smReceiveBuffer->writePointer, (int)smReceiveBuffer->numOfMessage);
        TCPRINTF("TS[%d]_MESSAGE <-- type=%d", (int)tsid, (int)smMessage->type);
        TCPRINTF("TS[%d]_MESSAGE <-- string=%s", tsid, smMessage->string);
        TCPRINTF("TS[%d]_MESSAGE <-- param =%d,%d,%d,%d,%d,%d,%d,%d", (int)tsid, (int)smMessage->param[0], (int)smMessage->param[1], (int)smMessage->param[2], (int)smMessage->param[3], (int)smMessage->param[4], (int)smMessage->param[5], (int)smMessage->param[6], (int)smMessage->param[7]);
      } else {
        TCPRINTF("%s_RECEIVE <-- pass", caller);
        TCPRINTF("%s_BUFFER  <-- rp=%d, wp=%d, num=%d", caller, (int)smReceiveBuffer->readPointer, (int)smReceiveBuffer->writePointer, (int)smReceiveBuffer->numOfMessage);
        TCPRINTF("%s_MESSAGE <-- type=%d", caller, (int)smMessage->type);
        TCPRINTF("%s_MESSAGE <-- string=%s", caller, smMessage->string);
        TCPRINTF("%s_MESSAGE <-- param =%d,%d,%d,%d,%d,%d,%d,%d", caller, (int)smMessage->param[0], (int)smMessage->param[1], (int)smMessage->param[2], (int)smMessage->param[3], (int)smMessage->param[4], (int)smMessage->param[5], (int)smMessage->param[6], (int)smMessage->param[7]);
      }
    }
    //
    rc = SM_NO_ERROR;
  } else if (smReceiveBuffer->numOfMessage == 0) {
    rc = SM_BUFFER_EMPTY_ERROR;      //buffer empty
  } else {
    TCPRINTF("error:num of message is negative.");
    goto ERROR;
  }

  ret = pthread_mutex_unlock(&mutex_syncmanager);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR;
  }

//  TCPRINTF("Exit,%d", rc);
  return rc;

ERROR:
  tcExit(EXIT_FAILURE);
  return SM_FAILURE_ERROR;
}

/* ----------------------------------------------------------- */
Byte smSendSyncMessage(SM_MESSAGE *smMessage, Dword timeout)
/* ----------------------------------------------------------- */
{
  SM_BUFFER *smSendBuffer = NULL;
  SM_BUFFER *smReceiveBuffer = NULL;
  time_t initialTime;
  Byte rc = SM_BUFFER_FULL_ERROR;
  smGetBuffer(&smSendBuffer, &smReceiveBuffer);
  int ret = 0;

  initialTime = smInitializeTimer();
  while (rc == SM_BUFFER_FULL_ERROR) {  //rc==2 :buffer is full
    rc = smSendMessage(smSendBuffer, smMessage);
    if (rc == SM_NO_ERROR) {
      break;
    }
    if (smCheckTimeOut(initialTime, timeout)) {
      rc = SM_TIMEOUT_ERROR;
      TCPRINTF("error:timeout");
    }
    ret = pthread_mutex_unlock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error mutex unlock %d", ret);
      rc = SM_MUTEX_UNLOCK_ERROR;
      break;
    }
    sleep(5);
    ret = pthread_mutex_lock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error mutex unlock %d", ret);
      rc = SM_MUTEX_LOCK_ERROR;
      break;
    }
  }
  return rc;
}

/* ----------------------------------------------------------- */
Byte smReceiveSyncMessage(Dword timeout, SM_MESSAGE *smMessage)
/* ----------------------------------------------------------- */
{
  SM_BUFFER *smSendBuffer = NULL;
  SM_BUFFER *smReceiveBuffer = NULL;
  SM_MESSAGE receiveMessage;
  time_t initialTime;
  int ret = 0;

  Byte rc = SM_BUFFER_EMPTY_ERROR;
  smGetBuffer(&smSendBuffer, &smReceiveBuffer);

  initialTime = smInitializeTimer();
  while (rc == SM_BUFFER_EMPTY_ERROR) {  //rc==2 :buffer is empty
    rc = smReceiveMessage(smReceiveBuffer, &receiveMessage);
    if (rc == SM_BUFFER_EMPTY_ERROR) {
      TCPRINTF("receiveMessage buffer empty");
    }
    if (rc == SM_NO_ERROR) {
      break;
    }
    if (smCheckTimeOut(initialTime, timeout)) {
      rc = SM_TIMEOUT_ERROR;
      TCPRINTF("error:timeout");
      break;
    }
    ret = pthread_mutex_unlock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error mutex unlock %d", ret);
      rc = SM_MUTEX_UNLOCK_ERROR;
      break;
    }
    sleep(5);
    ret = pthread_mutex_lock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error mutex unlock %d", ret);
      rc = SM_MUTEX_LOCK_ERROR;
      break;
    }
  }
  if (rc == SM_NO_ERROR) {
    *smMessage = receiveMessage;
  }
  return rc;
}

/* ----------------------------------------------------------- */
Byte smSendSyncMessageForSM(SM_BUFFER *smBuffer, SM_MESSAGE *smMessage, Dword timeout)
/* ----------------------------------------------------------- */
{
  time_t initialTime;
  int ret;
  initialTime = smInitializeTimer();
  Byte rc = SM_BUFFER_FULL_ERROR;
  while (rc == SM_BUFFER_FULL_ERROR) {  //rc==2 :buffer is full
    rc = smSendMessage(smBuffer, smMessage);
    if (rc == SM_NO_ERROR) {
    } else if (smCheckTimeOut(initialTime, timeout)) {
      rc = SM_TIMEOUT_ERROR;   //rc==5: timeout
    }
    ret = pthread_mutex_unlock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error mutex unlock %d", ret);
      rc = SM_MUTEX_UNLOCK_ERROR;
      break;
    }
    sleep(5);
    ret = pthread_mutex_lock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error mutex unlock %d", ret);
      rc = SM_MUTEX_LOCK_ERROR;
      break;
    }
  }
  if (rc == SM_NO_ERROR) {
  } else {
    TCPRINTF("SM_SEND  --> fail%d", (int)rc);
  }
  return rc;
}

/* ----------------------------------------------------------- */
Byte smReceiveSyncMessageForSM(SM_BUFFER *smBuffer, SM_MESSAGE *smMessage)
/* ----------------------------------------------------------- */
{
  Byte rc;
  rc = smReceiveMessage(smBuffer, smMessage);
  return rc;
}

/* ----------------------------------------------------------- */
Byte smQuerySyncMessage(Dword otype)
/* ----------------------------------------------------------- */
{
  SM_BUFFER *smSendBuffer = NULL;
  SM_BUFFER *smReceiveBuffer = NULL;
  Byte rc = SM_MESSAGE_UNAVAILABLE;
  //TCPRINTF("QUERY SYNC MESSAGE");
  smGetBuffer(&smSendBuffer, &smReceiveBuffer);
  rc = smQueryMessage(otype, smReceiveBuffer);
  return rc;
}

//----------------------------------------------------------------------
Byte smQueryMessage(Dword otype, SM_BUFFER *smBuffer)
//----------------------------------------------------------------------
{
  SM_MESSAGE smMessage;
  Byte rc = SM_MESSAGE_UNAVAILABLE;
  int ret = 0;

//  TCPRINTF("Entry");

  ret = pthread_mutex_lock(&mutex_syncmanager);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR;
  }

  if (smBuffer->numOfMessage > 0) {
    memmove(&smMessage, &smBuffer->message[smBuffer->readPointer], sizeof(smMessage));
    if (smMessage.type == otype) {
      rc = SM_MESSAGE_AVAILABLE;
    } else {
      rc = SM_MESSAGE_UNAVAILABLE;
    }
  } else if (smBuffer->numOfMessage == 0) {
    rc = SM_MESSAGE_UNAVAILABLE;
  } else {
    TCPRINTF("error:num of message is negative.");
    goto ERROR;
  }

  ret = pthread_mutex_unlock(&mutex_syncmanager);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR;
  }

//  TCPRINTF("Exit,%d", rc);
  return rc;

ERROR:
  tcExit(EXIT_FAILURE);
  return SM_FAILURE_ERROR;
}
