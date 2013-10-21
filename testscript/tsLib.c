#include "ts.h"

/* ---------------------------------------------------------------------- */
#if defined(LINUX)
/* ---------------------------------------------------------------------- */
/**
 * <pre>
 * Description:
 *   call main body of test script with constructor and destructor.
 * Arguments:
 *   arg - INPUT - config file image in byte stream
 * Return:
 *   always NULL.
 * Note:
 *   main body of test script is designed for GAIA (HGST automation tester) that runs
 *   on embeded card and OS, and one task to control one hgst cell (1 slot / 1 cell).
 *   some of initialization is required for supplier automation test pc:-
 *     + manipulate cell/slot id to bCellNo
 *     + create instance of slot environment control c++ class for example.
 *     + destroy instance of slot environment control c++ class for example.
 *
 *   given hgst cell id of one start (1, 2, ...). it is converted to supplier cell/slot
 *   id to access slot environment.
 *     xc3dp: (cellIndex - 1) x 2 + trayIndex
 *     op:    (cellIndex - 1) x 12 + trayIndex
 * </pre>
 ***************************************************************************************/
void *runTestScriptThread(void *arg)
{
  ZQUE *zque = arg;
  Byte rc = 1;
  int ret = 0;

  TCPRINTF("Entry: pthread_t,%08lxh", (unsigned long)pthread_self());

  //
  // mutex lock
  //
  ret = pthread_mutex_lock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto END_RUN_TEST_SCRIPT_THREAD;
  }

  //
  // parse to find index and set task id for GAIA compatibility
  //
  int i = 0;
  rc = 1;
  for (i = 0 ; i < (sizeof(tccb.ts) / sizeof(tccb.ts[0])) ; i++) {
    if (pthread_equal(tccb.ts[i].thread, pthread_self())) {
      rc = 0;
      break;
    }
  }
  if (rc) {
    TCPRINTF("error %d", rc);
    goto END_RUN_TEST_SCRIPT_THREAD;
  }
  TCPRINTF("threadid,%08lxh,wHGSTCellNo(zero-base),%d,wCellIndex,%d,wTrayIndex,%d",
           (unsigned long)pthread_self(),
           tccb.ts[i].wHGSTCellNo,
           tccb.ts[i].wCellIndex,
           tccb.ts[i].wTrayIndex
          );
  rc = set_task(get_task_base_offset() + tccb.ts[i].wHGSTCellNo);
  if (rc) {
    TCPRINTF("set_task out of range error\n");
    goto END_RUN_TEST_SCRIPT_THREAD;
  }

  //
  // call main body of test script with mutex locked
  //
  zque->wSID = 241;
  runTestScript(zque);

  //
  // mutex unlock
  //
  ret = pthread_mutex_unlock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto END_RUN_TEST_SCRIPT_THREAD;
  }

  return NULL;

END_RUN_TEST_SCRIPT_THREAD:
  tcExit(EXIT_FAILURE);
  return NULL;
}


#define task_twait(a)  task_delay(a)

extern int pthread_yield (void) __THROW;   /**<  might be better to include "pthread.h" with __USE_GNU flag */

/* ---------------------------------------------------------------------- */
void task_delay(Dword dwSecInHundredth)
/* ---------------------------------------------------------------------- */
{
  /* ---------------------------------------------------------------------- */
#if defined(UNIT_TEST)
  /* ---------------------------------------------------------------------- */
  dwSecInHundredth = 0;
  /* ---------------------------------------------------------------------- */
#endif   // end of UNIT_TEST
  /* ---------------------------------------------------------------------- */

  // Mutex Unlock
  for (;;) {
    int ret = 0;
    ret = pthread_mutex_unlock(&mutex_slotio);
    if (ret == 0) {
      break;
    }
    TCPRINTF("error on mutex unlock, but continue");
    fflush(stdout);
  }

  //
  // sleep(0) or yield
  //
  // NOTE: at least one implementation of sleep() returns immediately for the sleep(0) case,
  // so it is not useful for yielding (this was an older version of glibc.)
  //
  // turn on USE_YIELD to use pthread_yield() instead of sleep(0)
  //
#define USE_YIELD

  TCPRINTF("sleep %ld msec", dwSecInHundredth * 10);
  if (dwSecInHundredth == 0) {
#if defined(USE_YIELD)
    pthread_yield();
#else
    sleep(0);
#endif
  } else {
    usleep(dwSecInHundredth * 10 * 1000);
  }

  // Mutex lock
  for (;;) {
    int ret = 0;
    ret = pthread_mutex_lock(&mutex_slotio);
    if (ret == 0) {
      break;
    }
    TCPRINTF("error on mutex lock, but continue");
    fflush(stdout);
  }
}

struct {
  Word taskId;
  pthread_t threadId;
  Byte isAvailable; // zero if not available, otherwize available.
} task_id[TOP_OF_CELL_CONTROLLER_TASK_ID + NUM_OF_CELL_CONTROLLER_TASK];

/* ---------------------------------------------------------------------- */
Word get_task_base_offset(void)
/* ---------------------------------------------------------------------- */
{
  return TOP_OF_CELL_CONTROLLER_TASK_ID;
}

/* ---------------------------------------------------------------------- */
Byte set_task(Word w)   /* return non-zero value if out of range */
/* ---------------------------------------------------------------------- */
{

  if ((w < TOP_OF_CELL_CONTROLLER_TASK_ID) || ((sizeof(task_id) / sizeof(task_id[0])) <= w)) {
    return 1;
  }

  task_id[w].taskId = w;
  task_id[w].threadId = pthread_self();
  task_id[w].isAvailable = 1;
  return 0;
}

/* ---------------------------------------------------------------------- */
Word get_task(void)
/* ---------------------------------------------------------------------- */
{
  int i;
  int last = (sizeof(task_id) / sizeof(task_id[0]));

  for (i = TOP_OF_CELL_CONTROLLER_TASK_ID ; i < last ; i++) {
    if (pthread_equal(task_id[i].threadId, pthread_self())) {
      break;
    }
  }
  if (last <= i) {
    printf("(%s:%d)fatal error\n", __FILE__, __LINE__);
    fflush(stdout);
    tcExit(EXIT_FAILURE);
  }
  if (task_id[i].isAvailable == 0) {
    printf("(%s:%d)fatal error\n", __FILE__, __LINE__);
    fflush(stdout);
    tcExit(EXIT_FAILURE);
  }

  return task_id[i].taskId;
}

/* ---------------------------------------------------------------------- */
Byte get_MY_CELL_NO_by_index(Word wIndex, Word *wMyCellNo)
/* ---------------------------------------------------------------------- */
{
  int i;
  int last = (sizeof(task_id) / sizeof(task_id[0]));

  //  TCPRINTF("get_MY_CELL_NO_by_index %d", wIndex);

  for (i = TOP_OF_CELL_CONTROLLER_TASK_ID ; i < last ; i++) {
    if (task_id[i].isAvailable) {
      if (wIndex == 0) {
        break;
      } else {
        wIndex--;
      }
    }
  }

  if (last <= i) {
    return 1;
  }

  *wMyCellNo = task_id[i].taskId - TOP_OF_CELL_CONTROLLER_TASK_ID;
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte is_que(void)
/* ---------------------------------------------------------------------- */
{
  return 1;
}

/* ---------------------------------------------------------------------- */
Word getCellComModuleTaskNo(Byte bCellAddress)
/* ---------------------------------------------------------------------- */
{
  return 0;
}

/* ---------------------------------------------------------------------- */
void DI_(void)
/* ---------------------------------------------------------------------- */
{}

/* ---------------------------------------------------------------------- */
void EI_(void)
/* ---------------------------------------------------------------------- */
{}

Word tmp_wLength;
Dword tmp_dwAddress;
Byte tmp_bType;

/* ---------------------------------------------------------------------- */
void put_que(QUE *que)
/* ---------------------------------------------------------------------- */
{
  HQUE *hque = (HQUE *)que;
  ZDAT *zdat = (ZDAT *)que;

  //  printf("<=== put_que\n");
  fflush(stdout);
  putBinaryDataDump((Byte *)que, (Dword)que, 32);
  //  putBinaryDataDump((Byte *)que->bData, (Dword)que, 32);

  if (hque->wDID == getCellComModuleTaskNo(MY_CELL_NO + 1)) {
    tmp_wLength = hque->wLength;
    tmp_dwAddress = hque->dwAddress;
    tmp_bType = hque->bType;

  }
  if (que->bType == ZUpload) {
    {
      char filename[64];
      FILE *fp = NULL;

      snprintf(filename, sizeof(filename), "%08d.000", que->bAdrs);
      fp = fopen(filename, "r");
      if (fp == NULL) {
        Byte header[300];

        fp = fopen(filename, "w+b");
        if (fp == NULL) {
          tcExit(EXIT_FAILURE);
        }
        memset(header, 1, sizeof(header));
        fwrite(&header[0], 1, sizeof(header), fp);
      }
      fclose(fp);
      fp = fopen(filename, "a+b");
      if (fp == NULL) {
        tcExit(EXIT_FAILURE);
      }
      fwrite(zdat->pData, 1, zdat->dwLgth, fp);
      fclose(fp);
    }
  }

  free(que);
  return;
}

/* ---------------------------------------------------------------------- */
QUE *get_que(void)
/* ---------------------------------------------------------------------- */
{
#define STEP(a) counter <= (seek += a)
  HQUE *que;
  int counter = 0;
  static int counterMb = 0;
  static int counterDt = 0;
  int seek = 0;
  Dword isMobileMode = 0;

  if (((MY_CELL_NO + 1) == 1) || (MY_CELL_NO + 1) == 13) {
    isMobileMode = 1;
    counter = ++counterMb;
  } else {
    isMobileMode = 0;
    counter = ++counterDt;
  }

  que = calloc(sizeof(HQUE) + 64 * 1024, 1);
  memset(que, 0x00, 64*1024);
  que->wSID = getCellComModuleTaskNo(MY_CELL_NO + 1);
  que->wLength = tmp_wLength;
  que->dwAddress = tmp_dwAddress;
  que->bType = tmp_bType;

  //  printf("===> get_que(%d)\n", counter);
  fflush(stdout);

  if (isMobileMode) {
    if (0) {
      /* identify */
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "SELFSRST        ");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "MMUKOREM");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "TP4B00");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "PTCDEF");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "I_______?_______?_______d_______?_______E_______");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* status */
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* status */
    } else if (STEP(1)) {
      Byte a[] = {30};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get parameters */
    } else if (STEP(1)) {
      Byte a[] = {28};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0x0], "SELFSRST SRSTPARAM");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0x0], "TPI TPI");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "GEO GEO        ");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "CMD CMD       ");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* get result size */
    } else if (STEP(1)) {
      Byte a[] = {0x10, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get srst result */
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* should be validation fail */
      Byte a[] = {0xff};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x0a, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'S', 'R', 'S', 'T', 0x01, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x06, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'S', 'R', 'S', 'T', 0x0b, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get pes parameters */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x04, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get pes */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'C', 'Y', 'L', 'N'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x04};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x72};
      memcpy(&que->bData[0x184], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

    } else if (STEP(1)) {
      Byte a[] = {0x02, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'P', 'E', 'S', 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get pes */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'C', 'Y', 'L', 'N'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {1};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {57};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x04};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x72};
      memcpy(&que->bData[0x184], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

    } else if (STEP(1)) {
      Byte a[] = {0x02, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'P', 'E', 'S', 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get mcs */
    } else if (STEP(1)) {
      Byte a[] = {0x31};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'M', 'C', 'S'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));

      /* fin rawdata dump */
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "COMPLETE";
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* re-srst prohibit */
    } else if (STEP(1)) {
      Byte a[] = {0x05, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* error code */
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* TESTER LOG */
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* 'Uxxxx' after comp */
    } else if (STEP(1)) {
      Byte a[] = {'U', 0x00, 0x01, 0x02, 0x03};
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = sizeof(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      counterMb = 0;
    } else {
      /* Byte a[]={0x00,0x00,0x00,0x00}; memcpy(&que->bData[0], a, sizeof(a)); */
      counterMb = 0;
    }

  } else {
    if (0) {
      /* identify */
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "SELFSRST        ");
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "MMUKOREM");
      strcpy(&que->bData[0x1D6], "MMUKOREM");
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "TS6N00");
      strcpy(&que->bData[0x1D0], "TS6N00");
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "STCDEF");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "I_______?_______?_______d_______?_______E_______");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* status */
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* status */
    } else if (STEP(1)) {
      Byte a[] = {30};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get parameters */
    } else if (STEP(1)) {
      Byte a[] = {29};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0x0], "HhHhHhHhHh");
      strcpy(&que->bData[0x118], "HhHhHhHhHh");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0x0], "TBTBTBTBTBTBTBTBTBTB");
      strcpy(&que->bData[0x122], "TBTBTBTBTBTBTBTBTBTB");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "COMPSRST        ");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "Ss");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "Zz");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* get result size */
    } else if (STEP(1)) {
      Byte a[] = {0x10, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get srst result */
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x0a, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'S', 'R', 'S', 'T', 0x01, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x06, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'S', 'R', 'S', 'T', 0x0b, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get pes parameters */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x04, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get pes */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'C', 'Y', 'L', 'N'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x04};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x72};
      memcpy(&que->bData[0x184], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

    } else if (STEP(1)) {
      Byte a[] = {0x02, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'P', 'E', 'S', 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get pes */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'C', 'Y', 'L', 'N'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {1};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {57};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x04};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x72};
      memcpy(&que->bData[0x184], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

    } else if (STEP(1)) {
      Byte a[] = {0x02, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'P', 'E', 'S', 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get mcs */
    } else if (STEP(1)) {
      Byte a[] = {0x31};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'M', 'C', 'S'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));

      /* fin rawdata dump */
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "HEADPARAM";
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "PRE_SAT";
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* bad head */
    } else if (STEP(1)) {
      Byte a[] = {0x20, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x09, 0x00};
      memcpy(&que->bData[0x116], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {9, 8, 7, 6, 4, 3, 2, 1, 0, 5};
      memcpy(&que->bData[0x118], a, sizeof(a));



      /* error code */
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* TESTER LOG */
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
      /* 'Uxxxx' after comp */
    } else if (STEP(1)) {
      Byte a[] = {'U', 0x00, 0x01, 0x02, 0x03};
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = sizeof(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      counterDt = 0;
    } else {
      /* Byte a[]={0x00,0x00,0x00,0x00}; memcpy(&que->bData[0], a, sizeof(a)); */
      counterDt = 0;
    }

  }

  return (QUE *)que;
}

/* ---------------------------------------------------------------------- */
#else    // else of LINUX
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
#if defined(UNIT_TEST)
/* ---------------------------------------------------------------------- */

#define task_twait(a)  task_delay(a)

/* ---------------------------------------------------------------------- */
void task_delay(Dword dwSecInHundredth)
/* ---------------------------------------------------------------------- */
{}

/* ---------------------------------------------------------------------- */
Byte is_que(void)
/* ---------------------------------------------------------------------- */
{
  return 1;
}

/* ---------------------------------------------------------------------- */
Word getCellComModuleTaskNo(Byte bCellAddress)
/* ---------------------------------------------------------------------- */
{
  return 0;
}

/* ---------------------------------------------------------------------- */
void DI_(void)
/* ---------------------------------------------------------------------- */
{}

/* ---------------------------------------------------------------------- */
void EI_(void)
/* ---------------------------------------------------------------------- */
{}

Word tmp_wLength;
Dword tmp_dwAddress;
Byte tmp_bType;

/* ---------------------------------------------------------------------- */
void put_que(QUE *que)
/* ---------------------------------------------------------------------- */
{
  HQUE *hque = (HQUE *)que;
  ZDAT *zdat = (ZDAT *)que;

  printf("<=== put_que\n");
  fflush(stdout);
  putBinaryDataDump((Byte *)que, (Dword)que, 32);
  //  putBinaryDataDump((Byte *)que->bData, (Dword)que, 32);

  if (hque->wDID == getCellComModuleTaskNo(MY_CELL_NO + 1)) {
    tmp_wLength = hque->wLength;
    tmp_dwAddress = hque->dwAddress;
    tmp_bType = hque->bType;

  }
  if (que->bType == ZUpload) {
    {
      char filename[64];
      FILE *fp = NULL;

      snprintf(filename, sizeof(filename), "%08d.000", que->bAdrs);
      fp = fopen(filename, "r");
      if (fp == NULL) {
        Byte header[300];

        fp = fopen(filename, "w+b");
        if (fp == NULL) {
          tcExit(EXIT_FAILURE);
        }
        memset(header, 1, sizeof(header));
        fwrite(&header[0], 1, sizeof(header), fp);
      }
      fclose(fp);
      fp = fopen(filename, "a+b");
      if (fp == NULL) {
        tcExit(EXIT_FAILURE);
      }
      fwrite(zdat->pData, 1, zdat->dwLgth, fp);
      fclose(fp);
    }
  }

  free(que);
  return;
}

/* ---------------------------------------------------------------------- */
QUE *get_que(void)
/* ---------------------------------------------------------------------- */
{
#define STEP(a) counter <= (seek += a)
  HQUE *que;
  int counter = 0;
  static int counterMb = 0;
  static int counterDt = 0;
  int seek = 0;
  Dword isMobileMode = 0;

  if (((MY_CELL_NO + 1) == 1) || (MY_CELL_NO + 1) == 13) {
    isMobileMode = 1;
    counter = ++counterMb;
  } else {
    isMobileMode = 0;
    counter = ++counterDt;
  }

  que = calloc(sizeof(HQUE) + 64 * 1024, 1);
  memset(que, 0x00, 64*1024);
  que->wSID = getCellComModuleTaskNo(MY_CELL_NO + 1);
  que->wLength = tmp_wLength;
  que->dwAddress = tmp_dwAddress;
  que->bType = tmp_bType;

  printf("===> get_que(%d)\n", counter);
  fflush(stdout);

  if (isMobileMode) {
    if (0) {
      /* identify */
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "SELFSRST        ");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "MMUKOREM");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "TP4B00");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "PTCDEF");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "I_______?_______?_______d_______?_______E_______");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* status */
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* status */
    } else if (STEP(1)) {
      Byte a[] = {30};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get parameters */
    } else if (STEP(1)) {
      Byte a[] = {28};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0x0], "SELFSRST SRSTPARAM");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0x0], "TPI TPI");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "GEO GEO        ");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "CMD CMD       ");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* get result size */
    } else if (STEP(1)) {
      Byte a[] = {0x10, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get srst result */
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* should be validation fail */
      Byte a[] = {0xff};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x0a, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'S', 'R', 'S', 'T', 0x01, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x06, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'S', 'R', 'S', 'T', 0x0b, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get pes parameters */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x04, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get pes */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'C', 'Y', 'L', 'N'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x04};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x72};
      memcpy(&que->bData[0x184], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

    } else if (STEP(1)) {
      Byte a[] = {0x02, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'P', 'E', 'S', 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get pes */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'C', 'Y', 'L', 'N'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {1};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {57};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x04};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x72};
      memcpy(&que->bData[0x184], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

    } else if (STEP(1)) {
      Byte a[] = {0x02, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'P', 'E', 'S', 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get mcs */
    } else if (STEP(1)) {
      Byte a[] = {0x31};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'M', 'C', 'S'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));

      /* fin rawdata dump */
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "COMPLETE";
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* re-srst prohibit */
    } else if (STEP(1)) {
      Byte a[] = {0x05, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* error code */
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* TESTER LOG */
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* 'Uxxxx' after comp */
    } else if (STEP(1)) {
      Byte a[] = {'U', 0x00, 0x01, 0x02, 0x03};
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = sizeof(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      counterMb = 0;
    } else {
      /* Byte a[]={0x00,0x00,0x00,0x00}; memcpy(&que->bData[0], a, sizeof(a)); */
      counterMb = 0;
    }

  } else {
    if (0) {
      /* identify */
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "SELFSRST        ");
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "MMUKOREM");
      strcpy(&que->bData[0x1D6], "MMUKOREM");
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "TS6N00");
      strcpy(&que->bData[0x1D0], "TS6N00");
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "STCDEF");
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "I_______?_______?_______d_______?_______E_______");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* status */
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* status */
    } else if (STEP(1)) {
      Byte a[] = {30};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get parameters */
    } else if (STEP(1)) {
      Byte a[] = {29};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x02};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0x0], "HhHhHhHhHh");
      strcpy(&que->bData[0x118], "HhHhHhHhHh");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0x0], "TBTBTBTBTBTBTBTBTBTB");
      strcpy(&que->bData[0x122], "TBTBTBTBTBTBTBTBTBTB");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "COMPSRST        ");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "Ss");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x67, 0x45, 0x23, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      strcpy(&que->bData[0], "Zz");
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* get result size */
    } else if (STEP(1)) {
      Byte a[] = {0x10, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x01};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get srst result */
    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x0a, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'S', 'R', 'S', 'T', 0x01, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

    } else if (STEP(1)) {
      Byte a[] = {0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x03};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x06, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'S', 'R', 'S', 'T', 0x0b, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get pes parameters */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x04, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

      /* get pes */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'C', 'Y', 'L', 'N'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {25};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x04};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x72};
      memcpy(&que->bData[0x184], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

    } else if (STEP(1)) {
      Byte a[] = {0x02, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'P', 'E', 'S', 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get pes */
    } else if (STEP(1)) {
      Byte a[] = {0x11};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x13};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'C', 'Y', 'L', 'N'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {1};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {57};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x04};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x72};
      memcpy(&que->bData[0x184], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));

    } else if (STEP(1)) {
      Byte a[] = {0x02, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'P', 'E', 'S', 0x01};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* get mcs */
    } else if (STEP(1)) {
      Byte a[] = {0x31};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {'M', 'C', 'S'};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));

      /* fin rawdata dump */
    } else if (STEP(1)) {
      Byte a[] = {0x33};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */
    } else if (STEP(1)) {
      Byte a[] = {0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "HEADPARAM";
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "PRE_SAT";
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      /* bad head */
    } else if (STEP(1)) {
      Byte a[] = {0x20, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {0x09, 0x00};
      memcpy(&que->bData[0x116], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = {9, 8, 7, 6, 4, 3, 2, 1, 0, 5};
      memcpy(&que->bData[0x118], a, sizeof(a));



      /* error code */
    } else if (STEP(1)) {
      Byte a[] = {0x00, 0x00};
      memcpy(&que->bData[0], a, sizeof(a));
    } else if (STEP(1)) { /* write drive flag */

      /* TESTER LOG */
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
    } else if (STEP(1)) {
      Byte a[] = "recvFile:";
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = strlen(a);
      memcpy(&zque->bData[0], a, sizeof(a));
      /* 'Uxxxx' after comp */
    } else if (STEP(1)) {
      Byte a[] = {'U', 0x00, 0x01, 0x02, 0x03};
      ZQUE *zque = (ZQUE *)que;
      zque->wSID = 0;
      zque->wLgth = sizeof(a);
      memcpy(&zque->bData[0], a, sizeof(a));

      counterDt = 0;
    } else {
      /* Byte a[]={0x00,0x00,0x00,0x00}; memcpy(&que->bData[0], a, sizeof(a)); */
      counterDt = 0;
    }

  }

  return (QUE *)que;
}

/* ---------------------------------------------------------------------- */
#endif   // end of UNIT_TEST
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
#endif   // end of LINUX
/* ---------------------------------------------------------------------- */



ZQUE *zqueToTscb[NUM_OF_CELL_CONTROLLER_TASK];
TEST_SCRIPT_ABORT_BLOCK testScriptAbortBlock[NUM_OF_CELL_CONTROLLER_TASK];
TEST_SCRIPT_LOG_BLOCK testScriptLogBlock[NUM_OF_CELL_CONTROLLER_TASK];

/**
 * <pre>
 * Description:
 *   Run drive test script in SRST process.
 * Arguments:
 *   *que - Drive Test Script in ASCII Text Data
 * Host Communication (I/F Mode):
 *     PC/CellManager                                           CellController
 *          |                                                        |
 *          |       CMD:Protocol Change(I/F,SRST,DirectUART)         |
 *          |------------------------------------------------------->|
 *          |                                                        |
 *          |       FILE:Binary Code                                 |
 *          |------------------------------------------------------->|
 *          |                                                        |
 *          |       CMD:stdinPreload                                 |
 *          |<------------------------------------------------------>|
 *          |                                                        |
 *          |       CMD:Test Sequence                                |
 *          |------------------------------------------------------->|
 *          |                                                        |
 *          |       CMD:Download Request                             |
 *          |<-------------------------------------------------------|
 *          |                                                        |
 *          |       CMD/FILE:Download File                           |
 *          |------------------------------------------------------->|
 *          |                                                        |
 *          |       FILE:Rawdata                                     |
 *          |<-------------------------------------------------------|
 *          |                                                        |
 *          |       CMD:msg:                                         |
 *          |<-------------------------------------------------------|
 *          |                                                        |
 *          |       CMD:badHead:                                     |
 *          |<-------------------------------------------------------|
 *          |                                                        |
 *          |       CMD:testCompleted                                |
 *          |<-------------------------------------------------------|
 *          |                                                        |
 *          |       CMD:Response                                     |
 *          |------------------------------------------------------->|
 *          |                                                        |
 * </pre>
 *****************************************************************************/
void runTestScript(ZQUE *que)
{
  static Byte isFirstVisit = 1;
  TEST_SCRIPT_CONTROL_BLOCK *tscb = NULL;
  TEST_SCRIPT_ERROR_BLOCK *tseb = NULL;
  TEST_SCRIPT_ABORT_BLOCK *tsab = &testScriptAbortBlock[MY_CELL_NO];
  Byte rc = 1;
  Dword i = 0;
  SM_MESSAGE smMessage;

  /* disable test abort function (shall do it before calling ts_sleep_partial() */
  tsab->isEnable = 0;
  tsab->isRequested = 0;

  /* initialize timer (shall do it before calling ts_printf) */
  resetTestScriptTimeInSec();

  /* initialize log */
  resetTestScriptLog(0, DEFAULT_LOG_MIRROR_TO_STDOUT); /* delete previous test log */
  resetTestScriptLog(DEFAULT_LOG_SIZE_IN_KBYTE * 1024, DEFAULT_LOG_MIRROR_TO_STDOUT);
  ts_printf(__FILE__, __LINE__, "runTestScript");

  /* que contents check (shall do it before initializing tscb) */
  putBinaryDataDump((Byte *)que, (Dword)que, 64);
  ts_printf(__FILE__, __LINE__, "config file size = %d byte", que->wLgth);
  if ('\0' != que->bData[que->wLgth - 1]) {
    ts_printf(__FILE__, __LINE__, "invalid que");
    ts_qfree((void *)que);
    return;
  }
  if (que->wLgth > MAX_CONFIG_FILE_SIZE_IN_BYTE) {
    ts_printf(__FILE__, __LINE__, "invalid que");
    ts_qfree((void *)que);
    return;
  }
  if (que->wLgth != (strlen(&que->bData[0]) + 1)) {
    ts_printf(__FILE__, __LINE__, "invalid que");
    ts_qfree((void *)que);
    return;
  }

  /* memory alloc */
  DI_();
  if (isFirstVisit) {
    Dword dwNumOfBlock = sizeof(zqueToTscb) / sizeof(zqueToTscb[0]);

    isFirstVisit = 0;
    for (i = 0 ; i < dwNumOfBlock ; i++) {
      zqueToTscb[i] = NULL;
    }
  }
  EI_();
  if (zqueToTscb[MY_CELL_NO] == NULL) {
    zqueToTscb[MY_CELL_NO] = ts_qalloc(sizeof(ZQUE) + sizeof(TEST_SCRIPT_CONTROL_BLOCK));
  }
  tscb = (TEST_SCRIPT_CONTROL_BLOCK *) & zqueToTscb[MY_CELL_NO]->bData[0];

  /* initialize tscb (shall do it before parsing config file) */
  memset((Byte *)tscb, 0, sizeof(*tscb));
  tscb->dwSignature = TSCB_SIGNATURE;
  tscb->wHostID = que->wSID; /* que contents must be valid to come here */
  tseb = &tscb->testScriptErrorBlock;

  /* parse config file */
  rc = parseTestScriptFile(tscb, &que->bData[0], que->wLgth); /*[019KPE]*/
  ts_qfree((void *)que);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "error - parseTestScriptFile %d", rc);
    return;
  }
  tscb->dwTimeStampAtTestStart = getTestScriptTimeInSec();

  /* get firmware, card information */
  strcpy(&tscb->bCellTestLibraryVersion[0], getCellTestLibraryVersion());
  ts_printf(__FILE__, __LINE__, "getCellTestLibraryVersion = %s", &tscb->bCellTestLibraryVersion[0]);
  rc = dumpCellInventoryData(tscb);
  if (rc) {
    return;
  }

  /* find drive plug

  shall wait a few seconds for time lag from set drive to find drive by RS422 polling.
  even if drive plug is not found, no error is generated because some test script
  does not need drive plug (e.g. host com test).
  */
  for (i = 0 ; i < DRIVE_PLUG_RETRY_SEC ; i++) {
    rc = isDrivePlugged(MY_CELL_NO + 1);
    if (rc) {
      break;
    }
    ts_sleep_slumber(1);
  }

  /* enable test abort function */
  tsab->isEnable = 1;
  tsab->isRequested = 0;

  switch (setjmp(tsab->env)) {
  case 0:
    runTestScriptMain(tscb);
    break;
  case ABORT_REQUEST_BY_USER:
    ts_printf(__FILE__, __LINE__, "abort by user");
    ts_printf(__FILE__, __LINE__, "returning from longjmp - abort test script");
    ts_printf(__FILE__, __LINE__, "warning: variable `tscb' might be clobbered by `longjmp' or `vfork'");
    tseb->isAbortByUser = 1;
    break;
  case ABORT_REQUEST_BY_TEST_SCRIPT:
    /* FALLTHROUGH */
  default:
    ts_printf(__FILE__, __LINE__, "abort by test script");
    ts_printf(__FILE__, __LINE__, "returning from longjmp - abort test script");
    ts_printf(__FILE__, __LINE__, "warning: variable `tscb' might be clobbered by `longjmp' or `vfork'");
    tseb->isFatalError = 1;
    break;
  }

  /* disable test abort function */
  tsab->isEnable = 0;
  tsab->isRequested = 0;

  /* finalize */
  runTestScriptDestructor(tscb);
  ts_printf(__FILE__, __LINE__, "runTestScript -- Completed");
  resetTestScriptLog(0, DEFAULT_LOG_MIRROR_TO_STDOUT); /* delete log */

  /*********  y.katayama 2010.11.04 finish report to syncmanager ******************/
  if (tccb.optionSyncManagerDebug) {
    smMessage.type   = SM_TS_SD_FINISH;
    char string[] = "TestScript:finish";
    strncpy(smMessage.string, string, sizeof(smMessage.string));
    rc = smSendSyncMessage(&smMessage, SM_TS_SD_TIMEOUT);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "TestScript finish message send error!!!!");
    }
  }
  /*****************************************************************************/
  return;
}

/**
 * <pre>
 * Description:
 *   return non-zero value if test script abort request is.
 * </pre>
 *****************************************************************************/
Byte isTestScriptAbortRequested(void)
{
  TEST_SCRIPT_ABORT_BLOCK *tsab = &testScriptAbortBlock[MY_CELL_NO];

  /*********  y.katayama 2010.11.04 receive abort message from  syncmanager ************/
  if (tccb.optionSyncManagerDebug) {
    if (tsab->isEnable) {
      Byte rc = 0;
      SM_MESSAGE smMessage;
      rc = smQuerySyncMessage(SM_TS_RV_ABORT);
      if (rc == SM_MESSAGE_AVAILABLE) {
        TCPRINTF("MESSAGE AVAILABLE");
        rc = smReceiveSyncMessage(SM_TS_RV_TIMEOUT, &smMessage);
        if (rc == SM_NO_ERROR) {
          TCPRINTF("abort process start");
          tsab->isRequested = ABORT_REQUEST_BY_USER;
          return SM_FAILURE_ERROR;
        } else {
          TCPRINTF("error");
          TCPRINTF("abort process start");
          tsab->isRequested = ABORT_REQUEST_BY_USER;
          return SM_FAILURE_ERROR;
        }
      } else if (rc == SM_MESSAGE_UNAVAILABLE) {
      } else {
        TCPRINTF("error");
        TCPRINTF("abort process start");
        tsab->isRequested = ABORT_REQUEST_BY_USER;
        return SM_FAILURE_ERROR;
      }
    }
  }
  /*********  y.katayama 2010.11.04 receive abort message from  syncmanager ************/

  if (tsab->isEnable && tsab->isRequested) {
    /*********  y.katayama 2010.11.04 add message ************/
    TCPRINTF("abort process start");
    /*********  y.katayama 2010.11.04 add message ************/
    return 1;
  } else {
    return 0;
  }
}

/**
 * <pre>
 * Description:
 *   Abort this program. It is callled by user/host (not test script itself).
 * </pre>
 *****************************************************************************/
void abortTestScript(Byte bCellNo)
{
  TEST_SCRIPT_ABORT_BLOCK *tsab = NULL;

  TCPRINTF("abortTestScript %d", bCellNo);

  bCellNo -= 1; /* 1,2,...240 ==> 0,1,...239  */

  /* range check */
  if (((Dword)bCellNo + 1) > (sizeof(testScriptAbortBlock) / sizeof(testScriptAbortBlock[0]))) {
    return;
  }

  /* set abort request flag */
  tsab = &testScriptAbortBlock[bCellNo];
  if (tsab->isEnable) {
    tsab->isRequested = ABORT_REQUEST_BY_USER;
  }
}

/**
 * <pre>
 * Description:
 *   Abort this program. It is callled by test script (not user).
 * </pre>
 ***************************************************************************************/
void ts_abort(void)
{
  TEST_SCRIPT_ABORT_BLOCK *tsab = &testScriptAbortBlock[MY_CELL_NO];

#if defined(LINUX)
  tcDumpBacktrace(__func__);
#endif

  /* set abort request flag */
  if (tsab->isEnable) {
    tsab->isRequested = ABORT_REQUEST_BY_TEST_SCRIPT;
    ts_sleep_partial(0);
  }
}
/**
 * <pre>
 * Description:
 *   Gets pointer to CellTest library version string (20 bytes string + 1 byte '\0')
 * Example:
 *   "Aug  7 2008 15:49:40"
 * </pre>
 ***************************************************************************************/
Byte *getCellTestLibraryVersion(void)
{
  static Byte bStr[CELL_TEST_LIBRARY_VERSION_SIZE] = {'\0'};

  if (!isalnum(bStr[0])) {
    sprintf(&bStr[0], "%8.8s %11.11s", __TIME__, __DATE__);
  }

  return &bStr[0];
}
/**
 * <pre>
 * Description:
 *   Converts "?????" format to string. It adds '\0' at last of
 *   given string buffer.
 *   Return value is 0 if no error. Otherwise, non-zero value.
 * Arguments:
 *   *textInQuotationMark - String of "?????" format
 *   *bString - Pointer to result string
 *   dwStringSize - Array size of result string
 * </pre>
 ***************************************************************************************/
Byte textInQuotationMark2String(Byte *textInQuotationMark, Byte *bString, Dword dwStringSize)
{
  Byte *p = NULL;

  if ((textInQuotationMark[0] != '\"') || (textInQuotationMark[1] == '\"')) {
    ts_printf(__FILE__, __LINE__, "error - invalid field");
    return 1;
  }
  p = strchr(&textInQuotationMark[2], '\"');
  if (p == NULL) {
    ts_printf(__FILE__, __LINE__, "error - invalid field");
    return 1;
  }
  if ((Dword)(p - &textInQuotationMark[1]) > dwStringSize) {
    ts_printf(__FILE__, __LINE__, "too long text size");
    return 1;
  }
  memmove(&bString[0], &textInQuotationMark[1], (Dword)(p - &textInQuotationMark[1]));
  bString[(Dword)(p - &textInQuotationMark[1])] = '\0';
  return 0;
}

/**
 * <pre>
 * Description:
 *   Converts hhhhmmss format (e.g. 020:30:00) to integer.
 *   Return value is 0 if no error. Otherwise, non-zero value.
 * Arguments:
 *   *hhhhmmss - String of hhhhmmss format (e.g. "0020:30:00")
 *   *dwTimeSec - Pointer to result value in seconds
 * </pre>
 ***************************************************************************************/
Byte hhhhmmssTimeFormat2Sec(Byte *hhhhmmss, Dword *dwTimeSec)
{
  Dword hour = 0;
  Dword min = 0;
  Dword sec = 0;

  if (isdigit(hhhhmmss[0]) && isdigit(hhhhmmss[1]) && isdigit(hhhhmmss[2]) && isdigit(hhhhmmss[3]) && (hhhhmmss[4] == ':') &&
      isdigit(hhhhmmss[5]) && isdigit(hhhhmmss[6]) && (hhhhmmss[7] == ':') &&
      isdigit(hhhhmmss[8]) && isdigit(hhhhmmss[9]) && !isdigit(hhhhmmss[10])) {
    hour = ts_strtoul(&hhhhmmss[0], NULL, 10);
    min = ts_strtoul(&hhhhmmss[5], NULL, 10);
    sec = ts_strtoul(&hhhhmmss[8], NULL, 10);
    *dwTimeSec = sec + (min * 60) + (hour * 3600);
    return 0;
  } else {
    return 1;
  }
}

/**
 * <pre>
 * Description:
 *   Converts integer to hhhhmmss format (e.g. 020:30:00) includes
 *   last nul charactor. It assumes given hhhhmmss array size is
 *   enough large (11 bytes).
 *   Return value is 0 if no error. Otherwise, non-zero value.
 * Arguments:
 *   dwTimeSec - Pointer to result value in seconds
 *   *hhhhmmss - String of hhhhmmss format (e.g. "0020:30:00")
 *               It shall be
 * </pre>
 ***************************************************************************************/
void sec2hhhhmmssTimeFormat(Dword dwTimeSec, Byte *hhhhmmss)
{
  Dword hour = 0;
  Dword min = 0;
  Dword sec = 0;

  hour = dwTimeSec / 3600;
  dwTimeSec %= 3600;
  min = dwTimeSec / 60;
  dwTimeSec %= 60;
  sec = dwTimeSec;
  if (hour > 9999) {
    hour = 9999;
    min = 59;
    sec = 59;
  }
  sprintf(&hhhhmmss[0], "%4.4d:%2.2d:%2.2d", (int)hour, (int)min, (int)sec);

  return;
}

/**
 * <pre>
 * Description:
 *   Compares the first n bytes of str1 and str2. Does not stop comparing
 *   even after the null character (it always checks n characters).
 *   Returns zero if the first n bytes of str1 and str2 are equal.
 *   Otherwize, returns non-zero value.
 *   It compares all given data except '?'.
 * </pre>
 *****************************************************************************/
Dword ts_memcmp_exp(Byte *s1, Byte *s2, Dword n)
{
  Dword i = 0;

  for (i = 0 ; i < n ; i++) {
    if (s1[i] == '?') {
      /* no compare */
    } else if (s2[i] == '?') {
      /* no compare */
    } else if (s1[i] != s2[i]) {
      /* unmatch */
      return !MATCH;
    }
  }
  return MATCH;
}

static int ts_cval(c)
int c;
{
  if (isdigit(c)) return (c -'0');
  if (isalpha(c)) return (toupper(c) - 'A' + 10);
  return 37;

}

/**
 * <pre>
 * Description:
 *   The  function `strtoul' converts the string `*S' to an `unsigned long'.
 *   First, it breaks down the string into three parts: leading  whitespace,
 *   which  is ignored; a subject string consisting of the digits meaningful
 *   in the radix specified by BASE (for example, `0'  through  `7'  if  the
 *   value  of  BASE is 8); and a trailing portion consisting of one or more
 *   unparseable characters, which  always  includes  the  terminating  null
 *   character.  Then,  it  attempts  to  convert the subject string into an
 *   unsigned long integer, and returns the result.
 *
 *      If the value of BASE is zero, the subject string is expected to look
 *   like a normal C integer constant (save that no optional sign is permit-
 *   ted): a possible `0x' indicating hexadecimal radix, and a  number.   If
 *   BASE  is  between  2  and  36,  the  expected  form of the subject is a
 *   sequence of digits (which may include letters, depending on  the  base)
 *   representing  an  integer  in the radix specified by BASE.  The letters
 *   `a'-`z' (or `A'-`Z') are used as digits valued from 10 to 35.  If  BASE
 *   is 16, a leading `0x' is permitted.
 *
 *      The  subject  sequence  is the longest initial sequence of the input
 *   string that has the expected form, starting with the first  non-whites-
 *   pace character.  If the string is empty or consists entirely of whites-
 *   pace, or if the first non-whitespace character  is  not  a  permissible
 *   digit, the subject string is empty.
 *
 *      If  the subject string is acceptable, and the value of BASE is zero,
 *   `strtoul' attempts to determine the radix  from  the  input  string.  A
 *   string  with a leading `0x' is treated as a hexadecimal value; a string
 *   with a leading `0' and no `x' is treated as octal;  all  other  strings
 *   are  treated as decimal. If BASE is between 2 and 36, it is used as the
 *   conversion radix, as described above. Finally, a pointer to  the  first
 *   character past the converted subject string is stored in PTR, if PTR is
 *   not `NULL'.
 *
 * Returns:
 *   `strtoul'  returns  the  converted  value, if any. If no conversion was
 *   made, `0' is returned.
 *
 *    `strtoul' returns `ULONG_MAX' if  the  magnitude  of  the  converted
 *   value is too large, and sets `errno' to `ERANGE'.
 * </pre>
 *****************************************************************************/
Dword ts_strtoul(Byte *s, Byte **endptr, Dword radix)
{
  unsigned long val;
  unsigned long d;
  char sign;

  val = 0L;

  if (radix >= 36 || radix == 1 /* || radix <0 */) {
    if (endptr != NULL) *endptr = (char *)s;
    return 0;
  }

  while (isspace(*s)) s++;
  if ((sign = *s) == '-' || sign == '+') s++;

  if (radix == 0) {
    if (*s == '0') radix = ((toupper(*(s + 1)) == 'X') ? 16 : 8);
    else radix = 10;
  }
  if ((radix == 16) && (*s == '0') && (toupper(*(s + 1)) == 'X')) s += 2;

  {
    while ((d = ts_cval(*s++)) < radix) {
      /*
      if(val>(ULONG_MAX/radix)){
      errno=ERANGE;
      val=ULONG_MAX;
      break;
         }
      */
      val *= radix;
      /*
      if(d>(ULONG_MAX-val)){
      errno=ERANGE;
      val=ULONG_MAX;
      break;
         }
      */
      val += d;
    }
    if (endptr != NULL) *endptr = (char *)s - 1;
  }

  if (sign == '-') val = -val;
  return val;
}

/**
 * <pre>
 * Description:
 *  checks for any printable character including space.
 * </pre>
 *****************************************************************************/
Byte ts_isprint_mem(char *s, long n)
{
  int c = 0;
  for (;;n--) {
    if (n == 0) {
      break;
    }
    c = s[n-1];
    if (!isprint(c)) {
      return 0;
    }
  }
  return 1;
}

Byte checkAlpha(char *s, long n)
{
  int i;

  for (i = 0;i < n;i++) {
    if ((s[i] >= 0x30 && s[i] <= 0x39) ||     /* Number 0-9 */
        (s[i] >= 0x41 && s[i] <= 0x5a) ||     /* Capital word A-Z */
        (s[i] == 0x24)) {                     /* $ for uart check,temp check */
    } else {
      return i;
    }
  }
  return 0;
}
/**
 * <pre>
 * Description:
 *  The functions test the given buffer and return a nonzero (true)
 *  result if it satisfies the following conditions. If not, then 0 (false)
 *  is returned.
 *  Conditions: all letters in buffer (A to Z or a to z) or a digit (0 to 9)
 * </pre>
 *****************************************************************************/
Byte ts_isalnum_mem(Byte *s, Dword n)
{
  for (;;n--) {
    if (n == 0) {
      break;
    }
    if (!isalnum(s[n-1])) {
      return 0;
    }
  }
  return 1;
}

/**
 * <pre>
 * Description:
 *   This function copies N bytes from the memory region pointed to by bIN to
 *   the memory region pointed to by dwOUT. In the copy process, bIN is asuumed
 *   32 bit little endian format.
 *   If the regions overlap, the behavior is undefined.
 *   If the N is not a multiple of 4, do nothing.
 *   Returns a pointer to the first byte of the bOUT region.
 * </pre>
 *****************************************************************************/
Dword *memcpyWith32bitLittleEndianTo32bitConversion(Dword *dwOUT, Byte *bIN, Dword N)
{
  Dword i = 0;

  /* range check */
  if (N & 0x03) {
    ts_printf(__FILE__, __LINE__, "error - invalid N value %d", N);
    ts_abort();
    return NULL;
  }

  /* alignment check */
  if ((Dword)dwOUT & 0x3) {
    ts_printf(__FILE__, __LINE__, "error - invalid dwOUT value");
    ts_abort();
    return NULL;
  }

  /* endian swap */
  for (i = 0 ; i < N ; i += 4) {
    dwOUT[i/4] = (bIN[i+3] << 24) + (bIN[i+2] << 16) + (bIN[i+1] << 8) + bIN[i];
  }

  return dwOUT;
}

/**
 * <pre>
 * Description:
 *   This function copies N bytes from the memory region pointed to by bIN to
 *   the memory region pointed to by dwOUT. In the copy process, bIN is asuumed
 *   32 bit big endian format.
 *   If the regions overlap, the behavior is undefined.
 *   If the N is not a multiple of 4, do nothing.
 *   Returns a pointer to the first byte of the bOUT region.
 * </pre>
 *****************************************************************************/
Dword *memcpyWith32bitBigEndianTo32bitConversion(Dword *dwOUT, Byte *bIN, Dword N)
{
  Dword i = 0;

  /* range check */
  if (N & 0x03) {
    ts_printf(__FILE__, __LINE__, "error - invalid N value %d", N);
    ts_abort();
    return NULL;
  }

  /* alignment check */
  if ((Dword)dwOUT & 0x3) {
    ts_printf(__FILE__, __LINE__, "error - invalid dwOUT value");
    ts_abort();
    return NULL;
  }

  /* endian swap */
  for (i = 0 ; i < N ; i += 4) {
    dwOUT[i/4] = (bIN[i] << 24) + (bIN[i+1] << 16) + (bIN[i+2] << 8) + bIN[i+3];
  }

  return dwOUT;
}

/**
 * <pre>
 * Description:
 *   This function copies N bytes from the memory region pointed to by bIN to
 *   the memory region pointed to by wOUT. In the copy process, bIN is asuumed
 *   16 bit little endian format.
 *   If the regions overlap, the behavior is undefined.
 *   If the N is not a multiple of 2, do nothing.
 *   Returns a pointer to the first byte of the bOUT region.
 * </pre>
 *****************************************************************************/
Word *memcpyWith16bitLittleEndianTo16bitConversion(Word *wOUT, Byte *bIN, Dword N)
{
  Dword i = 0;

  /* range check */
  if (N & 0x01) {
    ts_printf(__FILE__, __LINE__, "error - invalid N value %d", N);
    ts_abort();
    return NULL;
  }

  /* alignment check */
  if ((Dword)wOUT & 0x1) {
    ts_printf(__FILE__, __LINE__, "error - invalid wOUT value");
    ts_abort();
    return NULL;
  }

  /* endian swap */
  for (i = 0 ; i < N ; i += 2) {
    wOUT[i/2] = (bIN[i+1] << 8) + bIN[i];
  }

  return wOUT;
}

/**
 * <pre>
 * Description:
 *   This function copies N bytes from the memory region pointed to by bIN to
 *   the memory region pointed to by wOUT. In the copy process, bIN is asuumed
 *   16 bit big endian format.
 *   If the regions overlap, the behavior is undefined.
 *   If the N is not a multiple of 2, do nothing.
 *   Returns a pointer to the first byte of the bOUT region.
 * </pre>
 *****************************************************************************/
Word *memcpyWith16bitBigEndianTo16bitConversion(Word *wOUT, Byte *bIN, Dword N)
{
  Dword i = 0;

  /* range check */
  if (N & 0x01) {
    ts_printf(__FILE__, __LINE__, "error - invalid N value %d", N);
    ts_abort();
    return NULL;
  }

  /* alignment check */
  if ((Dword)wOUT & 0x1) {
    ts_printf(__FILE__, __LINE__, "error - invalid wOUT value");
    ts_abort();
    return NULL;
  }

  /* endian swap */
  for (i = 0 ; i < N ; i += 2) {
    wOUT[i/2] = (bIN[i] << 8) + bIN[i+1];
  }

  return wOUT;
}

/**
 * <pre>
 * Description:
 *   This function copies N bytes from the memory region pointed to by bIN to
 *   the memory region pointed to by bOUT. In the copy process, bIN is asuumed
 *   32 bit little endian format.
 *   If the regions overlap, the behavior is undefined.
 *   If the N is not a multiple of 4, do nothing.
 *   Returns a pointer to the first byte of the bOUT region.
 * </pre>
 *****************************************************************************/
Byte *memcpyWith32bitLittleEndianTo8bitConversion(Byte *bOUT, Byte *bIN, Dword N)
{
  Dword i = 0;
  Byte b0 = 0, b1 = 0, b2 = 0, b3 = 0;

  /* range check */
  if (N & 0x03) {
    ts_printf(__FILE__, __LINE__, "error - invalid N value %d", N);
    ts_abort();
    return NULL;
  }

  /* endian swap */
  for (i = 0 ; i < N ; i += 4) {
    b0 = bIN[i+3];
    b1 = bIN[i+2];
    b2 = bIN[i+1];
    b3 = bIN[i];
    bOUT[i] = b0;
    bOUT[i+1] = b1;
    bOUT[i+2] = b2;
    bOUT[i+3] = b3;
  }

  return bOUT;
}

/**
 * <pre>
 * Description:
 *   This function copies N bytes from the memory region pointed to by bIN to
 *   the memory region pointed to by bOUT. In the copy process, bIN is asuumed
 *   16 bit little endian format.
 *   If the regions overlap, the behavior is undefined.
 *   If the N is not a multiple of 2, do nothing.
 *   Returns a pointer to the first byte of the bOUT region.
 * </pre>
 *****************************************************************************/
Byte *memcpyWith16bitLittleEndianTo8bitConversion(Byte *bOUT, Byte *bIN, Dword N)
{
  Dword i = 0;
  Byte b0 = 0, b1 = 0;

  /* range check */
  if (N & 0x01) {
    ts_printf(__FILE__, __LINE__, "error - invalid N value %d", N);
    ts_abort();
    return NULL;
  }

  /* endian swap */
  for (i = 0 ; i < N ; i += 2) {
    b0 = bIN[i+1];
    b1 = bIN[i];
    bOUT[i] = b0;
    bOUT[i+1] = b1;
  }

  return bOUT;
}

/**
 * <pre>
 * Description:
 *   Dispose of que
 * </pre>
 *****************************************************************************/
void ts_qfree(void *ptr)
{
  free(ptr);
}

/**
 * <pre>
 * Description:
 *   Dispose of invalid que
 * </pre>
 *****************************************************************************/
void ts_qfree_then_abort(void *ptr)
{
  ts_printf(__FILE__, __LINE__, "error - invalid que");
  putBinaryDataDump((Byte *)ptr, 0, 128);
  ts_qfree(ptr);
  ts_abort();
}

/**
 * <pre>
 * Description:
 *   Use `qalloc' to request allocation of an object with at least dwNBYTES
 *   bytes of queue storage  available. `qalloc' ever waits until desired
 *   queue storage space available. It never fails, thus your application
 *   does not need error handling.
 *   `qalloc' returns a pointer to a newly allocated space.
 * </pre>
 *****************************************************************************/
void *ts_qalloc(Dword dwNBYTES)
{
  Dword dwMemorySpace = 0;
  Dword dwRemainder = 0;
  Dword dwPad = 0;
  void *pMemory = NULL;

  for (;;) {
#if defined(UNIT_TEST) || defined(LINUX)
    /* ---------------------------------------------------------------------- */
    dwMemorySpace = MEMORY_WATER_MARK_THRESHOLD_IN_BYTE + 1;
    dwRemainder = 0;
    dwPad = 0;
#else
    /* ---------------------------------------------------------------------- */
    dwMemorySpace = (Dword)malloc(0) * BLOCKSIZE;
    dwRemainder = 1;
    dwPad = 1;
#endif
    /* ---------------------------------------------------------------------- */
    if (dwMemorySpace > MEMORY_WATER_MARK_THRESHOLD_IN_BYTE) {
      pMemory = calloc(((sizeof(ZQUE) + dwNBYTES) / BLOCKSIZE) + dwRemainder + dwPad, 1);
    } else {
      pMemory = NULL;
    }

    if (pMemory != NULL) {
      break;
    } else {
      ts_sleep_slumber(WAIT_TIME_SEC_FOR_MEMORY_ALLOC);
    }
  }
  return pMemory;
}

#if defined(UNIT_TEST) || defined(LINUX)
/* ---------------------------------------------------------------------- */
time_t dwTestScriptTimeSec[NUM_OF_CELL_CONTROLLER_TASK]; /** Test Start Time of Each Cells */

#else
/* ---------------------------------------------------------------------- */
Dword dwTestScriptTimeSec[NUM_OF_CELL_CONTROLLER_TASK]; /** Test Start Time of Each Cells */
Dword dwTestScriptTimeSecAsOfNow;
Dword dwTestScriptTimeSecPrevious;
#endif
/* ---------------------------------------------------------------------- */

/**
 * <pre>
 * Description:
 *   Queue availability test.
 *   Return 0 if no que is, otherwise non-zero.
 * </pre>
 *****************************************************************************/
Byte ts_qis(void)
{
  return is_que();
}

/**
 * <pre>
 * Description:
 *   Get queue from system
 * </pre>
 *****************************************************************************/
void *ts_qget(void)
{
  return get_que();
}

/**
 * <pre>
 * Description:
 *   Put queue to system
 * </pre>
 *****************************************************************************/
void ts_qput(void *ptr)
{
  put_que(ptr);
}

/**
 * <pre>
 * Description:
 *   Set time now. It will be used as starting time point of each test script.
 * </pre>
 *****************************************************************************/
void resetTestScriptTimeInSec(void)
{
#if defined(UNIT_TEST) || defined(LINUX)
  /* ---------------------------------------------------------------------- */
  dwTestScriptTimeSec[MY_CELL_NO] = time(NULL);

#else
  /* ---------------------------------------------------------------------- */
  static Byte isFirstVisit = 1;
  Dword rc = 0;

  DI_();
  if (isFirstVisit) {
    isFirstVisit = 0;
    dwTestScriptTimeSecAsOfNow = ULONG_MAX;
    rc = setTmSec(&dwTestScriptTimeSecAsOfNow);  /* Note: EI_() is called in here */
    dwTestScriptTimeSecPrevious = dwTestScriptTimeSecAsOfNow;
  }
  EI_();
  if (rc) {
    printfd("%-10.10s,%5.5d,error - timer registration\r\n");
  }

  dwTestScriptTimeSec[MY_CELL_NO] = dwTestScriptTimeSecAsOfNow;
#endif
  /* ---------------------------------------------------------------------- */
}

/**
 * <pre>
 * Description:
 *   Gets the test elapsed time. Return time in seconds.
 * </pre>
 *****************************************************************************/
Dword getTestScriptTimeInSec(void)
{
  const Dword dwAbsoluteMaxTimeValue = (10000 * 60 * 60) - 1;

#if defined(UNIT_TEST) || defined(LINUX)
  /* ---------------------------------------------------------------------- */
  if ((Dword)difftime(time(NULL), dwTestScriptTimeSec[MY_CELL_NO]) > dwAbsoluteMaxTimeValue) {
    return dwAbsoluteMaxTimeValue;
  }

  return (Dword)difftime(time(NULL), dwTestScriptTimeSec[MY_CELL_NO]);

#else
  /* ---------------------------------------------------------------------- */
  if (dwTestScriptTimeSecAsOfNow > dwTestScriptTimeSecPrevious) { /* timer overflow!! */
    return dwAbsoluteMaxTimeValue;
  }
  dwTestScriptTimeSecPrevious = dwTestScriptTimeSecAsOfNow;

  if ((dwTestScriptTimeSec[MY_CELL_NO] - dwTestScriptTimeSecAsOfNow) > dwAbsoluteMaxTimeValue) {
    return dwAbsoluteMaxTimeValue;
  }

  return dwTestScriptTimeSec[MY_CELL_NO] - dwTestScriptTimeSecAsOfNow;
#endif
  /* ---------------------------------------------------------------------- */
}

/**
 * <pre>
 * Description:
 *   Sleeps the given time in seconds. Accuracy is +/- one second.
 * Arguments:
 *   dwTimeInSec - The name say it all.
 *   bSleepMode - SLEEP_MODE_PARTIAL or SLEEP_MODE_SLUMBER
 * Remark:
 *   task_wait    Return when event occur
 *   task_twait   Return after given value x 10msec (don't return if que avaiable)
 *   task_twait2  Return after given value x 2msec (don't return if que avaiable)
 *   task_delay   Return after given value x 10msec (return if que avaiable)
 *   task_delay2  Return after given value x 2msec (return if que avaiable)
 *   task_qwait   Return when que available
 * </pre>
 *****************************************************************************/
void sub_ts_sleep(Dword dwTimeInSec, Byte bSleepMode)
{
  TEST_SCRIPT_ABORT_BLOCK *tsab = &testScriptAbortBlock[MY_CELL_NO];
  Dword dwSleepUnitTimeInSec = 0;

  ts_printf(__FILE__, __LINE__, "ts_sleep_%s %d", (bSleepMode == SLEEP_MODE_PARTIAL ? "partial" : "slumber"), dwTimeInSec);


  do {
    if (isTestScriptAbortRequested()) {
      longjmp(tsab->env, tsab->isRequested);
    }

//  y.katayama 2010.11.04 (example : polling prepdata message)
    /************************************************************************************/
    /************************************* PREPDATA *************************************/
    /************************************************************************************/

    if (tccb.optionSyncManagerDebug) {
      Byte rc = 0;
      SM_MESSAGE smMessage;
      rc = smQuerySyncMessage(SM_TS_RV_PREPDATA);
      if (rc == SM_MESSAGE_AVAILABLE) {
        TCPRINTF("MESSAGE AVAILABLE");
        rc = smReceiveSyncMessage(SM_TS_RV_TIMEOUT, &smMessage);
        if (rc != SM_NO_ERROR) {
          TCPRINTF("!!!!!!!!!!!!!!!receive prep_data message !!!!!");
        }
        if (smMessage.type != SM_TS_RV_PREPDATA) {
          TCPRINTF("!!!!!!!!!!!!!!!prep_data_get_error!!!!!");
          longjmp(tsab->env, tsab->isRequested);
        }
        /********************* do process prepdata *******************************************/
        TCPRINTF("!!!!!!!!!!!!!!!prep_data!!!!!!!!!!!!!!!!");
      }
    }
    /************************************************************************************/
    /********************************* PREPDATA END *************************************/
    /************************************************************************************/

    if (dwTimeInSec >= SLEEP_UNIT_SEC) {
      dwSleepUnitTimeInSec = SLEEP_UNIT_SEC;
    } else {
      dwSleepUnitTimeInSec = dwTimeInSec;
    }

    if (bSleepMode == SLEEP_MODE_PARTIAL) {
      task_delay(dwSleepUnitTimeInSec * 100);
    } else {
      task_twait(dwSleepUnitTimeInSec * 100);
    }

    dwTimeInSec -= dwSleepUnitTimeInSec;
  } while (0 < dwTimeInSec);

  if (isTestScriptAbortRequested()) {
    longjmp(tsab->env, tsab->isRequested);
  }
}

/**
 * <pre>
 * Description:
 *   Sleeps the given time in seconds. Accuracy is +/- one second.
 *   Wake up if que is available.
 * Arguments:
 *   dwTimeInSec - The name say it all.
 * </pre>
 *****************************************************************************/
void ts_sleep_partial(Dword dwTimeInSec)
{
  sub_ts_sleep(dwTimeInSec, SLEEP_MODE_PARTIAL);
}

/**
 * <pre>
 * Description:
 *   Sleeps the given time in seconds. Accuracy is +/- one second.
 *   Do not wake up even if que is available.
 * Arguments:
 *   dwTimeInSec - The name say it all.
 * </pre>
 *****************************************************************************/
void ts_sleep_slumber(Dword dwTimeInSec)
{
  sub_ts_sleep(dwTimeInSec, SLEEP_MODE_SLUMBER);
}

/**
 * <pre>
 * Description:
 *   Prepare log data block.
 * </pre>
 *****************************************************************************/
void resetTestScriptLog(Dword dwMaxLogSizeInByte, Byte isTesterLogMirrorToStdout)
{
  TEST_SCRIPT_LOG_BLOCK *tslb = &testScriptLogBlock[MY_CELL_NO];
  TEST_SCRIPT_LOG_BLOCK tslb_prev;
  static Byte isFirstVisit = 1;

  DI_();
  if (isFirstVisit) {
    isFirstVisit = 0;
    memset((Byte *)&testScriptLogBlock[0], 0x00, sizeof(testScriptLogBlock));
  }
  EI_();

  if (tslb->bLogTop != NULL) {
    tslb_prev = *tslb;
    memset(tslb, 0x00, sizeof(*tslb));
  } else {
    tslb_prev.bLogTop = NULL;
  }

  tslb->isTesterLogMirrorToStdout = isTesterLogMirrorToStdout;
  if (dwMaxLogSizeInByte == 0) {
    tslb->bLogTop = NULL;
  } else {
    tslb->zque = ts_qalloc(dwMaxLogSizeInByte);
    tslb->bLogTop = &tslb->zque->bData[0];
  }
  tslb->bLogCur = tslb->bLogTop;
  tslb->bLogEnd = &tslb->bLogTop[dwMaxLogSizeInByte];
  tslb->dwCurLogSizeInByte = 0;
  tslb->dwMaxLogSizeInByte = dwMaxLogSizeInByte;

  if (tslb_prev.bLogTop != NULL) {
    if (tslb->dwMaxLogSizeInByte >= tslb_prev.dwMaxLogSizeInByte) {
      memmove(tslb->bLogTop, tslb_prev.bLogTop, tslb_prev.dwMaxLogSizeInByte);
      tslb->bLogCur = &tslb->bLogTop[tslb_prev.bLogCur - tslb_prev.bLogTop];
      tslb->dwCurLogSizeInByte = tslb_prev.dwCurLogSizeInByte;
    }
    ts_qfree((Byte *)tslb_prev.zque);
  }

  memset(tslb->bLogCur, ' ', tslb->bLogEnd - tslb->bLogCur);
}

/**
 * <pre>
 * Description:
 *   Get pointer & size of log data block
 * </pre>
 *****************************************************************************/
void getTestScriptLog(Byte **bLogData, Dword *dwCurLogSizeInByte, Dword *dwMaxLogSizeInByte)
{
  TEST_SCRIPT_LOG_BLOCK *tslb = &testScriptLogBlock[MY_CELL_NO];

  *bLogData = tslb->bLogTop;
  *dwCurLogSizeInByte = tslb->dwCurLogSizeInByte;
  *dwMaxLogSizeInByte = tslb->dwMaxLogSizeInByte;
}

/**
 * <pre>
 * Description:
 *   Puts given strings into serial port. This function inserts current time
 *   in secondes and my cell ID automatically at top of line. Also, it append
 *   'line-feed(0x0A)' at last of description if it is not in description.
 *   Please keep length of given strings < 240 (256 - 11 - 4 -1) bytes.
 * Arguments:
 *   Variable arguments like 'printf'
 * </pre>
 *****************************************************************************/
void ts_printf(Byte *bFileName, Dword dwNumOfLine, Byte *description, ...)
{
  Dword dwLength = 0;
  va_list ap;
  Byte bBuf[256];
  TEST_SCRIPT_LOG_BLOCK *tslb = &testScriptLogBlock[MY_CELL_NO];
  static Byte isFirstVisit = 1;
  static Dword dwFileNameOffset = 0;
  Byte bTime[11];

  DI_();
  if (isFirstVisit) {
    Byte *p = NULL;

    p = strrchr(bFileName, (Dword)'/');
    if (p == NULL) {
      p = strrchr(bFileName, (Dword)'\\');
    }
    if (p == NULL) {
      dwFileNameOffset = 0;
    } else {
      dwFileNameOffset = (Dword)(&p[1] - bFileName);
    }
    isFirstVisit = 0;
  }
  EI_();

  /* length check. it is some useful even if it is not 100% reliable. */
  dwLength = strlen(description);
  if (dwLength > (sizeof(bBuf) / 2)) {
    return;
  }

  /* append time stamp */
  sec2hhhhmmssTimeFormat(getTestScriptTimeInSec(), &bTime[0]);
  dwLength = 0;
  dwLength += sprintf(&bBuf[dwLength], "%s,", &bTime[0]);

  /* append my cell id */
  dwLength += sprintf(&bBuf[dwLength], "%3.3ld,", (long int)MY_CELL_NO + 1);

  /* append file name */
  dwLength += sprintf(&bBuf[dwLength], "%-10.10s,", &bFileName[dwFileNameOffset]);

  /* append num of line */
  dwLength += sprintf(&bBuf[dwLength], "%5.5ld,", (long int)dwNumOfLine);

  /* append log   */
  va_start(ap, description);
#if defined(UNIT_TEST) || defined(LINUX)
  /* ---------------------------------------------------------------------- */
  dwLength += vsprintf(&bBuf[dwLength], description, ap);
#else
  /* ---------------------------------------------------------------------- */
  dwLength += (sprintfdcom(&bBuf[dwLength], description, ap) - &bBuf[dwLength]);
#endif
  /* ---------------------------------------------------------------------- */
  va_end(ap);

  /* length check. it is some useful even if it is not 100% reliable. */
  if (dwLength > (sizeof(bBuf) - 16)) {
    return;
  }

  /* add a newline only if not one already  */
  if (bBuf[dwLength - 1] != '\n') {
#if defined(UNIT_TEST) || defined(LINUX)
    /* ---------------------------------------------------------------------- */
    dwLength += sprintf(&bBuf[dwLength], "\r\n");
#else
    /* ---------------------------------------------------------------------- */
    dwLength += sprintf(&bBuf[dwLength], "\n");
#endif
    /* ---------------------------------------------------------------------- */
  }

  /* Sweep out to Console */
  if (tslb->isTesterLogMirrorToStdout) {
#if defined(UNIT_TEST) || defined(LINUX)
    /* ---------------------------------------------------------------------- */
    fputs(&bBuf[0], stdout);
    fflush(stdout);
#else
    /* ---------------------------------------------------------------------- */
    printfd(&bBuf[0]);

#endif
    /* ---------------------------------------------------------------------- */
  }

  /* Store into memory */
  if (tslb->dwMaxLogSizeInByte) {
    if (dwLength < tslb->dwMaxLogSizeInByte) {
      Byte *p = NULL;

      if (dwLength < (Dword)(tslb->bLogEnd - tslb->bLogCur)) {
        /* nop */
      } else {
        /* fill space in end of log */
        memset(tslb->bLogCur, ' ', (Dword)(tslb->bLogEnd - tslb->bLogCur));
        tslb->bLogCur = tslb->bLogTop;
      }
      /* write log */
      memmove(tslb->bLogCur, &bBuf[0], dwLength);
      tslb->bLogCur += dwLength;
      if (tslb->dwCurLogSizeInByte < (Dword)(tslb->bLogCur - tslb->bLogTop)) {
        /* update log size */
        tslb->dwCurLogSizeInByte = tslb->bLogCur - tslb->bLogTop;
      }
      /* fill space in overlap log */
      p = memchr(tslb->bLogCur, '\n', (Dword)(tslb->bLogEnd - tslb->bLogCur));
      if (p != NULL) {
        memset(tslb->bLogCur, ' ', (Dword)(p - tslb->bLogCur));
      }
    }
  }


  /* ---------------------------------------------------------------------- */
#if defined(LINUX)
  /* ---------------------------------------------------------------------- */
  TCPRINTF("%s", &bBuf[0]);
  /* ---------------------------------------------------------------------- */
#endif
  /* ---------------------------------------------------------------------- */


}

const Byte bBinary2Ascii[] = "0123456789ABCDEF";
/**
 * <pre>
 * Description:
 *   Puts given strings into serial port. This function inserts current time
 *   in secondes and my cell ID automatically at top of line. Also, it append
 *   'line-feed(0x0A)' at last of description if it is not in description.
 *   Please keep length of given strings < 240 (256 - 11 - 4 -1) bytes.
 *
 * Example:
 *   [00000000] 30 36 32 38 00 00 00 00 00 00 00 00 00 00 00 00 0628............
 *   [00000010] 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
 *
 * Arguments:
 *   Variable arguments like 'printf'
 * </pre>
 *****************************************************************************/
void putBinaryDataDump(Byte *dwPhysicalAddress, Dword dwLogicalAddress, Dword dwSize)
{
  Byte bLineBuffer[64 + 1];
  Byte bByteBuffer;
  Dword dwOffsetToBinary = 0;
  Dword dwOffsetToAscii = 48;
  Dword i;

  if (dwSize > MAX_SIZE_OF_DUMP_BINARY) {
    dwSize = MAX_SIZE_OF_DUMP_BINARY;
  }

  memset(&bLineBuffer[0], ' ', sizeof(bLineBuffer));
  bLineBuffer[sizeof(bLineBuffer) - 1] = '\0';

  for (i = 0 ; i < dwSize ; i++) {
    /* Get One Byte */
    bByteBuffer = *(Byte *)((Dword)dwPhysicalAddress + i);

    /* Set Binary Format */
    bLineBuffer[dwOffsetToBinary] = bBinary2Ascii[(bByteBuffer & 0xF0) >> 4];
    bLineBuffer[dwOffsetToBinary + 1] = bBinary2Ascii[bByteBuffer & 0x0F];
    bLineBuffer[dwOffsetToBinary + 2] = ' ';

    /* Set ASCII Format */
    if ((bByteBuffer < ' ') || ('~' < bByteBuffer)) {
      bLineBuffer[dwOffsetToAscii] = '.';
    } else {
      bLineBuffer[dwOffsetToAscii] = bByteBuffer;
    }
    bLineBuffer[dwOffsetToAscii + 1] = '\0';


    if ((dwOffsetToAscii + 1) == 64) {
      /* Sweep out buffer */
      dwOffsetToBinary = 0;
      dwOffsetToAscii = 48;
      ts_printf(__FILE__, __LINE__, "[%08x] %s", dwLogicalAddress + i - 15, &bLineBuffer[0]);
      memset(bLineBuffer, ' ', 64);
      bLineBuffer[sizeof(bLineBuffer) - 1] = '\0';

    } else {
      /* Move foward */
      dwOffsetToBinary += 3;
      dwOffsetToAscii++;
    }
  }

  /* Sweep out buffer */
  if (dwOffsetToAscii != 48) {
    ts_printf(__FILE__, __LINE__, "[%08x] %s", dwLogicalAddress + i - (i % 16), &bLineBuffer[0]);
  }
}
/***************************************************
  Convert from Ascii String to unsigned integer
****************************************************/
Dword calcAtoUL( Byte *data, Dword size )
{
  Byte i;
  Dword value = 0;

  for ( i = 0; i < size; i++ ) {
    if ( data[i] <= 0x20 ) break;
    value <<= 4;
    value += convA2H( data[i] );
  }
  return value;
}
/***************************************************
  Convert from Ascii String to unsigned integer
****************************************************/
Dword convA2H( Byte data )
{
  switch ( data ) {
  case '0':
    return 0x00;
  case '1':
    return 0x01;
  case '2':
    return 0x02;
  case '3':
    return 0x03;
  case '4':
    return 0x04;
  case '5':
    return 0x05;
  case '6':
    return 0x06;
  case '7':
    return 0x07;
  case '8':
    return 0x08;
  case '9':
    return 0x09;
  case 'A':
  case 'a':
    return 0x0A;
  case 'B':
  case 'b':
    return 0x0B;
  case 'C':
  case 'c':
    return 0x0C;
  case 'D':
  case 'd':
    return 0x0D;
  case 'E':
  case 'e':
    return 0x0E;
  case 'F':
  case 'f':
    return 0x0F;
  default:
    return 0x00;
  }
}
