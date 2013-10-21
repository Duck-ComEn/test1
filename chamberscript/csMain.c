#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>  // for getpid, sleep, usleep
#include "cs.h"


#define MATCH                          (0)     // makes use of strncmp() more readable


void parseChamberScript(const char *sequence);


/* ---------------------------------------------------------------------- */
void *runChamberScriptThread(void *arg)
/* ---------------------------------------------------------------------- */
{
  int ret = 0;
  ZQUE *zque = arg;
  int simpleInterpreter = 0;
  Byte rc = 0;
  SM_MESSAGE smMessage;

  TCPRINTF("Entry: pthread_t,%08lxh", (unsigned long)pthread_self());

  // Mutex Lock
  ret = pthread_mutex_lock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto END_RUN_CHAMBER_SCRIPT_THREAD;
  }

  // execute chamber script here
  if (arg == NULL) {
    TCPRINTF("nop");
    if (tccb.optionSyncManagerDebug) {
//  strncpy(smMessage.string,string,64);
      smMessage.type   = SM_CS_SD_CSREADY;
      char string[] = "ChamberScript:READY";
      strncpy(smMessage.string, string, sizeof(smMessage.string));
      rc = smSendSyncMessage(&smMessage, SM_CS_SD_TIMEOUT);
      if (rc) {
        goto END_RUN_CHAMBER_SCRIPT_THREAD;
      }
    } else {
      tccb.isChamberScriptReadyToStart = 1;
    }
  } else {
    TCPRINTF("simpleInterpreter,%d", tccb.isChamberScriptReadyToStart);
    if (simpleInterpreter) {
      if (tccb.optionSyncManagerDebug) {
        smMessage.type   = SM_CS_SD_CSREADY;
        char string[] = "ChamberScript:READY";
        strncpy(smMessage.string, string, sizeof(smMessage.string));
        rc = smSendSyncMessage(&smMessage, SM_CS_SD_TIMEOUT);
        if (rc) {
          goto END_RUN_CHAMBER_SCRIPT_THREAD;
        }
      } else {
        tccb.isChamberScriptReadyToStart = 1;
      }
//      for (;;) {
//          sleep(60);
//      }
      parseChamberScript(&zque->bData[0]);
    } else {
      rc = 0;
      //rc = csInterpreter(&zque->bData[0], zque->wLgth);  wLgth includes 0x00 termination. it will be error in csInterpreter()
      rc = csInterpreter(&zque->bData[0], strlen(&zque->bData[0]));
      TCPRINTF("status code from \"csInterpreter\" : %d ", rc);
      if (rc) {
        goto END_RUN_CHAMBER_SCRIPT_THREAD;
      }
    }
  }

  // Mutex Unlock
  ret = pthread_mutex_unlock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto END_RUN_CHAMBER_SCRIPT_THREAD;
  }

  // 1 hour extension
  TCPRINTF("[DEBUG] 1 hour extensoin to escape from a situation that chamber script finishes before");
  TCPRINTF("[DEBUG] test script finishes if both chamber script and test script have same timeout value");
  sleep(60 * 60);


  // complete
  TCPRINTF("completed");
  for (;;) {
    sleep(60);
  }

END_RUN_CHAMBER_SCRIPT_THREAD:
  tcExit(EXIT_FAILURE);
  return NULL;
}

/* ---------------------------------------------------------------------- */
void parseChamberScript(const char *sequence)
/* ---------------------------------------------------------------------- */
{
  const char keySeqBegin[] = "sequence(";
  const char keyLoopBegin[] = "save(";
  const char keyLoopEnd[] = "loop(";
  const char keyWait[] = "holdMin(";
  const char keyTemp[] = "setTemp(";
  const char keySeqEnd[] = "endSequence()";
  int idx = 0;
  int loop = 0;
  int waitInSec = 0;
  Word wTempInHundredth = 0;
  Byte rc = 0;

  for (idx = 0 ; idx < strlen(sequence) ; idx++) {

    if (MATCH == memcmp(&sequence[idx], keySeqBegin, strlen(keySeqBegin))) {
      Word wMyCellNo = 0;

      wTempInHundredth = 2500;
      TCPRINTF("find %s %d", keySeqBegin, wTempInHundredth);

      rc = get_MY_CELL_NO_by_index(0, &wMyCellNo);
      if (rc) {
        TCPRINTF("error");
        tcExit(EXIT_FAILURE);
      }

      rc = setTargetTemperature(wMyCellNo + 1, wTempInHundredth);
      if (rc) {
        TCPRINTF("error");
        tcExit(EXIT_FAILURE);
      }

    } else if (MATCH == memcmp(&sequence[idx], keyLoopBegin, strlen(keyLoopBegin))) {
      int i = 0;

      loop = atoi(&sequence[idx + strlen(keyLoopBegin)]);
      TCPRINTF("find %s %d", keyLoopBegin, loop);
      for (i = 0 ; i < (loop - 1); i++) {
        parseChamberScript(&sequence[idx + 1]);
      }

    } else if (MATCH == memcmp(&sequence[idx], keyLoopEnd, strlen(keyLoopEnd))) {
      TCPRINTF("find %s", keyLoopEnd);
      if (loop > 0) {
        loop = 0;
      } else {
        return;
      }

    } else if (MATCH == memcmp(&sequence[idx], keyWait, strlen(keyWait))) {
      int ret = 0;
      int sec = 0;
      Word wMyCellNo = 0;

      waitInSec = atoi(&sequence[idx + strlen(keyWait)]) * 60;
      TCPRINTF("find %s %d %d %d", keyWait, waitInSec / 3600, waitInSec / 60, waitInSec);

      rc = get_MY_CELL_NO_by_index(0, &wMyCellNo);
      if (rc) {
        TCPRINTF("error");
        tcExit(EXIT_FAILURE);
      }

#define TEMP_CHECK_POLL_INTERVAL_IN_SEC  (60)

      for (sec = 0 ; sec < waitInSec ; sec += TEMP_CHECK_POLL_INTERVAL_IN_SEC) {
        Word wTargetTempInHundredth = 0;
        Word wCurrentTempInHundredth = 0;

        // Mutex Unlock
        ret = pthread_mutex_unlock(&mutex_slotio);
        if (ret) {
          TCPRINTF("error");
          tcExit(EXIT_FAILURE);
        }

        sleep(TEMP_CHECK_POLL_INTERVAL_IN_SEC);

        // Mutex Lock
        ret = pthread_mutex_lock(&mutex_slotio);
        if (ret) {
          TCPRINTF("error");
          tcExit(EXIT_FAILURE);
        }

        rc = getTargetTemperature(wMyCellNo + 1, &wTargetTempInHundredth);
        if (rc) {
          TCPRINTF("error");
          tcExit(EXIT_FAILURE);
        }

        rc = getCurrentTemperature(wMyCellNo + 1, &wCurrentTempInHundredth);
        if (rc) {
          TCPRINTF("error");
          tcExit(EXIT_FAILURE);
        }

        TCPRINTF("target,%d,current,%d", wTargetTempInHundredth, wCurrentTempInHundredth);
      }

    } else if (MATCH == memcmp(&sequence[idx], keyTemp, strlen(keyTemp))) {
      wTempInHundredth = atoi(&sequence[idx + strlen(keyTemp)]) * 100;
      TCPRINTF("find %s %d", keyTemp, wTempInHundredth);

      for (;;) {
        Word wMyCellNo = 0;
        rc = get_MY_CELL_NO_by_index(0, &wMyCellNo);
        if (rc) {
          TCPRINTF("error");
          tcExit(EXIT_FAILURE);
        }
        rc = setTargetTemperature(wMyCellNo + 1, wTempInHundredth);
        if (rc) {
          TCPRINTF("error");
          tcExit(EXIT_FAILURE);
        }
        break;
      }

    } else if (MATCH == memcmp(&sequence[idx], keySeqEnd, strlen(keySeqEnd))) {
      TCPRINTF("find %s", keySeqEnd);
      return;
    }

  }
  return;
}


/* ---------------------------------------------------------------------- */
int csIsLabelInRemainingSequence(void)
/* ---------------------------------------------------------------------- */
{
  //
  // return non-zero value if label exits in remaining sequence
  // remaining sequence is from current pointer to end of the sequence
  //
  // example)
  //
  //  saveInterval(2);
  //  restart(0,0,0);
  //  setVolt(1200,500);
  //  command(#OVL0500);      command(#OVU0500);
  //  setTemp(58,1);
  //  save(80);
  //    restart(4,1,0);
  //    setVolt(1200,500);      holdMin(60);         // return 1 at here
  //  loop();
  //  label(#A);
  //  restart(0,0,0);
  //  command(#PORTLABELA);
  //  setTemp(25,1);
  //  save(7);
  //    restart(4,1,0);
  //    setVolt(1200,500);      holdMin(60);         // return 0 at here
  //  loop();
  //  setVolt(1200,500);      holdMin(48);
  //  command(#PORTPREPDATA);holdMin(30);
  //  setVolt(0,0);   setTemp(25,1);                 // return 0 at here
  //

  return 1;
}


/* ---------------------------------------------------------------------- */
unsigned long csGotoLabel(void)
/* ---------------------------------------------------------------------- */
{
  //
  // goto label() and then return the fowarding time in seconds that includes not
  // actual time passing but fowarding time from current sequence pointer
  // to the label location by this function.
  //
  // if no label is found, return 0;
  //

  return 0;
}

/* ---------------------------------------------------------------------- */
Byte csGotoRequest(Dword *dwForwardingTimeInSec)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:");

  Byte rc = 1;
  int i = 0;
  static pthread_t thread[MAX_NUM_OF_TEST_SCRIPT_THREAD + 1] = {[0 ... MAX_NUM_OF_TEST_SCRIPT_THREAD] = 0};
  static Dword dwForwardingTimeInSec_save = 0;

  // label exists in remaining sequnece?
  if (!csIsLabelInRemainingSequence()) {
    rc = 1;
    goto END_CS_GOTO;
  }

  // register thread id
  for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
    if (thread[i] == pthread_self()) {
      break;
    } else if (thread[i] == 0) {
      thread[i] = pthread_self();
      break;
    }
  }

  // num of thread id is enough?
  for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
    if (thread[i] == 0) {
      break;
    }
  }
  if (i != tccb.optionNumOfTestScriptThread) {
    rc = 2;
    goto END_CS_GOTO;
  }

  // goto label
  if (dwForwardingTimeInSec_save == 0) {
    dwForwardingTimeInSec_save = csGotoLabel();
    *dwForwardingTimeInSec = dwForwardingTimeInSec_save;
  }
  rc = 0;

END_CS_GOTO:
  TCPRINTF("Exit:%d,%ld", rc, *dwForwardingTimeInSec);
  return rc;
}
