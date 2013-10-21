#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "cs.h"

#define MATCH                          (0)     // makes use of strncmp() more readable


time_t dwChamberScriptTimeSec;
time_t dwChamberScriptTimeSecFromJump;
double dwChamberScriptJumpedTime;
extern int execIdx;


/* ---------------------------------------------------------------------- */
Dword csElapsedTime(void)
/* ---------------------------------------------------------------------- */
{
  const Dword dwAbsoluteMaxTimeValue = (10000 * 60 * 60) - 1;

  if ((Dword)difftime(time(NULL), dwChamberScriptTimeSec) > dwAbsoluteMaxTimeValue) {
    return dwAbsoluteMaxTimeValue;
  }

  return (Dword)difftime(time(NULL), dwChamberScriptTimeSec);
}


/* ---------------------------------------------------------------------- */
Dword csElapsedTimeFromJump(void)
/* ---------------------------------------------------------------------- */
{
  const Dword dwAbsoluteMaxTimeValue = (10000 * 60 * 60) - 1;

  if ((Dword)difftime(time(NULL), dwChamberScriptTimeSecFromJump) > dwAbsoluteMaxTimeValue) {
    return dwAbsoluteMaxTimeValue;
  }

  TCPRINTF("time,%Ld,initial,%Ld", (long long int)time(NULL), (long long int)dwChamberScriptTimeSecFromJump);
  return (Dword)difftime(time(NULL), dwChamberScriptTimeSecFromJump);
}

/* ---------------------------------------------------------------------- */
int csInitializeTimer(void)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:");
  dwChamberScriptTimeSec = time(NULL);
  dwChamberScriptTimeSecFromJump = time(NULL);
  TCPRINTF("Exit:");
  return 0;
}

/* ---------------------------------------------------------------------- */
int csInitializeTimerFromJump(void)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:");
  dwChamberScriptTimeSecFromJump = time(NULL);
  TCPRINTF("Exit:");
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallSequence(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%ld,execute_index:%d", arg1, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallEndSequence(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%s,execute_index:%d", string, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallSaveInterval(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%ld,execute_index:%d", arg1, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallRestart(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%ld,%ld,%ld,execute_index:%d", arg1, arg2, arg3, execIdx);
  Byte ret = 0;
  if (tccb.optionSyncManagerDebug) {
    //store saveindex FIle
    Dword dwStartSecFromJump = csElapsedTimeFromJump();
    TCPRINTF("save resume Index File Time from jump=%Ld,total time = %Ld", (long long int)dwStartSecFromJump, (long long int)(dwStartSecFromJump / 60 + dwChamberScriptJumpedTime));
    ret = csWriteResumeIdx((double)dwStartSecFromJump / 60 + dwChamberScriptJumpedTime, execIdx, (Byte)arg2);
    if (ret) {
      TCPRINTF("write resume index error");
      exit(EXIT_FAILURE);
    }
  }
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallSetVolt(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%ld,%ld,execute_index:%d", arg1, arg2, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallSetTemp(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%ld,%ld,execute_index:%d", arg1, arg2, execIdx);
  int rc = 0;

  TCPRINTF("ramp rate change is not supported right now");
  rc = setTargetTemperature(tccb.ts[0].wHGSTCellNo + 1, arg1 * 100);
  if (rc) {
    TCPRINTF("error");
    exit(EXIT_FAILURE);
  }

  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallHoldMin(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%ld,execute_index:%d", arg1, execIdx);

  int rc = 0;
  int ret = 0;

  Dword dwTimeoutSec = arg1 * 60;
  Dword dwStartSec = csElapsedTime();
  Dword dwElapsedSec = dwStartSec;

  for (;;) {
    Word wTargetTempInHundredth = 0;
    Word wCurrentTempInHundredth = 0;
    Word wErrorStatus = 0;

    // monitor temperature
    rc = getTargetTemperature(tccb.ts[0].wHGSTCellNo + 1, &wTargetTempInHundredth);
    if (rc) {
      TCPRINTF("error");
      goto CS_CALL_HOLD_MINT_EXIT_WITH_MUTEX_RELEASE;
    }
    rc = getCurrentTemperature(tccb.ts[0].wHGSTCellNo + 1, &wCurrentTempInHundredth);
    if (rc) {
      TCPRINTF("error");
      goto CS_CALL_HOLD_MINT_EXIT_WITH_MUTEX_RELEASE;
    }
    rc = getTemperatureErrorStatus(tccb.ts[0].wHGSTCellNo + 1, &wErrorStatus);
    if (rc) {
      TCPRINTF("error,%d,%d", rc, wErrorStatus);
      goto CS_CALL_HOLD_MINT_EXIT_WITH_MUTEX_RELEASE;
    }
    // Mutex Unlock
    ret = pthread_mutex_unlock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error");
      tcExit(EXIT_FAILURE);
    }

    // wait
    sleep(TEMP_CHECK_DURATION);

    // Mutex Lock
    ret = pthread_mutex_lock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error");
      tcExit(EXIT_FAILURE);
    }
///////  for test 2010.10.18 y,katayama //////////////////////

    if (tccb.optionSyncManagerDebug) {
      SM_MESSAGE smMessage;
      rc = smReceiveSyncMessage(SM_TS_RV_TIMEOUT, &smMessage);
      if (rc == SM_NO_ERROR) {
        if (smMessage.type == SM_CS_RV_JMPREQ) {
          smMessage.param[0] = (Dword)csJump(smMessage.string);
          smMessage.param[1] = dwChamberScriptJumpedTime;
          smMessage.type   = SM_CS_SD_JMPACK;
          char sdMessageString[] = "ChamberScript:jump_ack";
          strncpy(smMessage.string, sdMessageString, sizeof(smMessage.string));
          rc = smSendSyncMessage(&smMessage, SM_CS_SD_TIMEOUT);
          if (rc) {
            TCPRINTF("error");
            tcExit(EXIT_FAILURE);
          }
          if (!smMessage.param[0]) {
            break;
          }
        } else {
          TCPRINTF("error");
          tcExit(EXIT_FAILURE);
        }
      } else if (rc == SM_TIMEOUT_ERROR) {

      } else if (rc) {
        TCPRINTF("error");
        tcExit(EXIT_FAILURE);
      }
    }



///////  for test 2010.10.18 y,katayama //////////////////////

    // check timeout
    dwElapsedSec = csElapsedTime() - dwStartSec;
    TCPRINTF("elapsed,%ld,timeout,%ld", dwElapsedSec, dwTimeoutSec);
    if (dwElapsedSec >= dwTimeoutSec) {
      TCPRINTF("timeout!!");
      break;
    }
  }

  TCPRINTF("Exit:%d", rc);
  return 0;

CS_CALL_HOLD_MINT_EXIT_WITH_MUTEX_RELEASE:
  TCPRINTF("Exit: with mutex release");
  ret = pthread_mutex_unlock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error");
  }
  tcExit(EXIT_FAILURE);
  exit(EXIT_FAILURE);
}


/* ---------------------------------------------------------------------- */
int csCallCommand(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  SM_MESSAGE smMessage;
  int rc = 0;
  TCPRINTF("Entry:%s,execute_index:%d", string, execIdx);
  if (tccb.optionSyncManagerDebug) {
    //////// PREPDATA MESSAGE ////////////////////////////////////////////
    if (strcmp(string, "#PORTPREPDATA") == MATCH) {
      smMessage.type   = SM_CS_SD_PREPDATA;
      char sendMessageString[] = "ChamberScript:prepData";
      strncpy(smMessage.string, sendMessageString, sizeof(smMessage.string));
      rc = (int)smSendSyncMessage(&smMessage, SM_CS_SD_TIMEOUT);
      if (rc) {
        TCPRINTF("error");
        tcExit(EXIT_FAILURE);
      }
    }
//////// FINAL START MESSAGE ////////////////////////////////////////////
    if (strcmp(string, "#PORTFINALSTART") == MATCH) {
      smMessage.type   = SM_CS_SD_FINALSTART;
      char sendMessageString[] = "ChamberScript:FinalStart";
      strncpy(smMessage.string, sendMessageString, sizeof(smMessage.string));
      rc = (int)smSendSyncMessage(&smMessage, SM_CS_SD_TIMEOUT);
      if (rc) {
        TCPRINTF("error");
        tcExit(EXIT_FAILURE);
      }
    }
  }
  //store saveindex FIle
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallResumeCommand(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%s,execute_index:%d", string, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallSave(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%ld,execute_index:%d", arg1, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallLoop(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%s,execute_index:%d", string, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallVersion(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%s,execute_index:%d", string, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallSumValue(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%s,execute_index:%d", string, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallLabel(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%s,execute_index:%d", string, execIdx);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csCallResumeAsDummy(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%ld,%ld,%ld", arg1, arg2, arg3);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}


/* ---------------------------------------------------------------------- */
int csWaitUntilTemperatureOnTarget(void)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:");
  int rc = 0;
  int i = 0;
  int ret = 0;

  for (i = 0 ; ; i++) {
    Word wTargetTempInHundredth = 0;
    Word wCurrentTempInHundredth = 0;
    Word wErrorStatus = 0;

    rc = getTargetTemperature(tccb.ts[0].wHGSTCellNo + 1, &wTargetTempInHundredth);
    if (rc) {
      TCPRINTF("error");
      goto CS_WAIT_UNTIL_TEMPERATURE_ON_TARGET_EXIT_WITH_MUTEX_RELEASE;
    }
    rc = getCurrentTemperature(tccb.ts[0].wHGSTCellNo + 1, &wCurrentTempInHundredth);
    if (rc) {
      TCPRINTF("error");
      goto CS_WAIT_UNTIL_TEMPERATURE_ON_TARGET_EXIT_WITH_MUTEX_RELEASE;
    }
    rc = getTemperatureErrorStatus(tccb.ts[0].wHGSTCellNo + 1, &wErrorStatus);
    if (rc) {
      TCPRINTF("error,%d,%d", rc, wErrorStatus);
      goto CS_WAIT_UNTIL_TEMPERATURE_ON_TARGET_EXIT_WITH_MUTEX_RELEASE;
    }
    rc = isOnTemp(tccb.ts[0].wHGSTCellNo + 1);
    TCPRINTF("target,%d,current,%d,ontemp%d", wTargetTempInHundredth, wCurrentTempInHundredth, rc);

    if (rc) {
      TCPRINTF("on-temp now, ready to go");
      break;
    }

    // Mutex Unlock
    ret = pthread_mutex_unlock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error");
      tcExit(EXIT_FAILURE);
    }

    sleep(TEMP_CHECK_DURATION);

    // Mutex Lock
    ret = pthread_mutex_lock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error");
      tcExit(EXIT_FAILURE);
    }
  }

  TCPRINTF("Exit:");
  return 0;

CS_WAIT_UNTIL_TEMPERATURE_ON_TARGET_EXIT_WITH_MUTEX_RELEASE:
  TCPRINTF("Exit: with mutex release");
  ret = pthread_mutex_unlock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error");
  }
  tcExit(EXIT_FAILURE);
  exit(EXIT_FAILURE);
}

/******************************************************************************/
Byte csJump(char *label)
/******************************************************************************/
{
  Byte rc = 0;
  Word i;
  TCPRINTF("Entry:");
  for (i = 0;i < MAX_CTRL_STEP;i++) {
    if (strcmp(csJumpTable[i].string, label) == MATCH) {
      if (csJumpTable[i].index >= execIdx) {
        execIdx = csJumpTable[i].index;
        dwChamberScriptJumpedTime = csJumpTable[i].minute;
        csInitializeTimerFromJump();
      } else {
        TCPRINTF("!!!!!!!!!!!!!! LABEL is not defined !!!!!!!!!!!!!!!!!!");
        rc = 1;  //label is not define error
      }
      break;
    } else if (i == MAX_CTRL_STEP - 1) {
      TCPRINTF("!!!!!!!!!!!!!! LABEL is not defined !!!!!!!!!!!!!!!!!!");
      rc = 1;  //label is not define error
    }
  }
  TCPRINTF("Exit:%d", rc);
  return rc;
}
/* ---------------------------------------------------------------------- */
int csCallInitialTemperatureSet(long arg1, long arg2, long arg3, char *string, int length)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:%ld,%ld,%ld", arg1, arg2, arg3);
  int rc = 0;
  TCPRINTF("Exit:%d", rc);
  return 0;
}

