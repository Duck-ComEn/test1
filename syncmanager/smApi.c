#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "sm.h"

/* ---------------------------------------------------------------------- */
int smCheckTimeOut(time_t initialTime, int timeout_time)
/* ---------------------------------------------------------------------- */
{
  Dword dwAbsoluteMaxTimeValue = (Dword)timeout_time - 1;
  //TCPRINTF("initial time %d timeout time %d difftime %d dwAbsoluteMaxTimeValue %d",(int)initialTime,timeout_time,(int)difftime(time(NULL),initialTime),(int)dwAbsoluteMaxTimeValue);
  if (timeout_time == 0) {
    return 1;
  } else if ((Dword)difftime(time(NULL), initialTime) > dwAbsoluteMaxTimeValue) {
    return 1;
  }
  return 0;
}

/* ---------------------------------------------------------------------- */
time_t smInitializeTimer(void)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("Entry:");
  time_t dwSyncManagerTimeSec = time(NULL);
  TCPRINTF("Exit:");
  return dwSyncManagerTimeSec;
}

