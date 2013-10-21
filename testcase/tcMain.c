#include "tc.h"

enum {
  DRIVE_POWER_OFF                       = 0,
  EXIT_CELL_OFFLINE                     = 2,       // EXIT_SUCCESS, EXIT_FAILURE are defined in /usr/include/stdlib.h
  ENVELOP_TEMP                          = 2000,    // Workaround ONLY for EP No UART [DEBUG]
  NOMINAL_TEMP                          = 3200,
  POS_RAMP_RATE_TEMP_FOR_PRODUCTION     = 100,
  NEG_RAMP_RATE_TEMP_FOR_PRODUCTION     = 100,
  POS_RAMP_RATE_TEMP_FOR_BENCHDEBUG     = 600,
  NEG_RAMP_RATE_TEMP_FOR_BENCHDEBUG     = 999,
  SAFE_HANDLING_TEMP                    = 4000,    // [NOTE] error happens if it is 3500 AND secondary water temp is 2500
  SAFE_HANDLING_TEMP_FOR_TIMEOUT_CALC   = 3500,
  MAX_SAFE_HANDLING_TIMEOUT_IN_SEC      = 1800,    // 30 * 60,
};

pthread_mutex_t mutex_tcPrintf;
pthread_mutex_t mutex_tcExit;

TCCB tccb;
const char TEST_PARAM_INITIAL_TEST[] = "\nTEST_PARAM  dwDriveTestElapsedTimeSec                      0000:00:00    // inserted by testcase automatically";
char tcPrintfFilename[128] = {'\0'};
int tcPrintfDisable = 0;


/* ---------------------------------------------------------------------- */
int main(int argc, char *argv[])
/* ---------------------------------------------------------------------- */
{
  int ret = 0;
  int i = 0;
  Byte rc = 0;
  //
  // initialize mutex
  //
  pthread_mutex_init(&mutex_tcPrintf, NULL);
  pthread_mutex_init(&mutex_tcExit, NULL);
  pthread_mutex_init(&mutex_slotio, NULL);
  pthread_mutex_init(&mutex_syncmanager, NULL);


  //
  // initialize tccb
  //
  memset(&tccb, 0x00, sizeof(tccb));
  tccb.argc = argc;
  tccb.argv = argv;

  tccb.optionNoSignalHandlerThread = 0;
  tccb.tc.thread = pthread_self();
  ret = pthread_attr_init(&tccb.tc.attr);
  if (ret) {
    exit(EXIT_FAILURE);
  }
  tccb.tc.start_routine = NULL;
  tccb.tc.arg = NULL;
  tccb.tc.thread_return = NULL;

  tccb.sh.thread = 0;
  ret = pthread_attr_init(&tccb.sh.attr);
  if (ret) {
    exit(EXIT_FAILURE);
  }
  tccb.sh.start_routine = runSignalHandlerThread;
  tccb.sh.arg = NULL;
  tccb.sh.thread_return = NULL;

  tccb.cs.thread = 0;
  ret = pthread_attr_init(&tccb.cs.attr);
  if (ret) {
    exit(EXIT_FAILURE);
  }
  tccb.cs.start_routine = runChamberScriptThread;
  tccb.cs.arg = NULL;
  tccb.cs.thread_return = NULL;

  tccb.sm.thread = 0;
  ret = pthread_attr_init(&tccb.sm.attr);
  if (ret) {
    exit(EXIT_FAILURE);
  }
  tccb.sm.start_routine = runSyncManagerThread;
  tccb.sm.arg = NULL;
  tccb.sm.thread_return = NULL;

  for (i = 0 ; i < (sizeof(tccb.ts) / sizeof(tccb.ts[0])) ; i++) {
    tccb.ts[i].thread = 0;
    ret = pthread_attr_init(&tccb.ts[i].attr);
    if (ret) {
      exit(EXIT_FAILURE);
    }
    tccb.ts[i].start_routine = runTestScriptThread;
    tccb.ts[i].arg = NULL;
    tccb.ts[i].thread_return = NULL;
  }


  //
  // attach finalize functions to exit()
  //
  // note - Functions registered using atexit() (and on_exit()) shall not call exit()
  //        It may behave eternal loop.
  //
  ret = atexit(tcPrintfFileInZip);
  if (ret) {
    exit(EXIT_FAILURE);
  }
  ret = atexit(tcDumpCore);
  if (ret) {
    exit(EXIT_FAILURE);
  }

  //
  // setup tcPrintf
  //
  if (strlen(tcPrintfFilename) == 0) {
    struct timeval tv;
    struct timezone tz;
    struct tm tm;
    gettimeofday(&tv, &tz);
    localtime_r((const time_t *)&tv.tv_sec, &tm);

    pid_t pid = getpid();
    snprintf(&tcPrintfFilename[0],
             sizeof(tcPrintfFilename),
             "tclogger.%04d.%02d.%02d.%02d.%02d.%02d.%ld.log",
             tm.tm_year + 1900,
             tm.tm_mon + 1,
             tm.tm_mday,
             tm.tm_hour,
             tm.tm_min,
             tm.tm_sec,
             (unsigned long)pid
            );
  }


  //
  // parse arguments
  //
  if (argc == 1) {
    Byte *bVersionString = NULL;
    Dword dwLength = 0;

    vcGetVersion(&bVersionString, &dwLength);

    printf("         \n");
    printf("         \n");
    printf("Usage:   \n");
    printf("         testcase [FILE] [OPTION]\n");
    printf("         example: testcase ptb.cfg stn.cfg\n");
    printf("         \n");
    printf("Option:  \n");
    printf("         -ps,        mirror testcase print log to stdout\n");
    printf("         -ns,        do not launch signal handler (useful for valgraid / gdb)\n");
    printf("         -nh,        run without host pc\n");
    printf("         -gd,        go to die (by zero divide) for debug purpose\n");
    printf("         -cs[file],  execute chamber script\n");
    printf("                     example: testcase ptb1.cfg ptb1.cfg -csptb.seq\n");
    printf("         -lf,        log filter for test result\n");
    printf("                     usage:   testcase -lf resultfile\n");
    printf("                     example: testcase -lf 00000001.000\n");
    printf("         -qe,        quick environment status check\n");
    printf("         -cu,        clean up envirnment (power-off, safe handling temperature)\n");
    printf("         -bd,        bench debug mode (no wait for safe handling mode)\n");
    printf("         -wa,        apply workaround for ack missing in optimus\n");
    printf("         -ln[file],  apply log file name\n");
    printf("                     example: testcase ptb1.cfg ptb1.cfg -lnABCDEFG\n");
    printf("                              ==> ABCDEFG.log\n");
    printf("         -sd,        sync manager debug\n");
    printf("         -ds,        development station mode\n");
    printf("         \n");
    printf("Note:    \n");
    printf("         this program was compiled on %s %s\n", __TIME__, __DATE__);
    printf("         version : %s\n", bVersionString);
    printf("         \n");
    tccb.deleteLogFiles = 1;
    exit(EXIT_FAILURE);
  }
  for (i = 1 ; i < argc ; i++) {
    if (MATCH == strcmp("-ps", argv[i])) {
      // mirror testcase print log to stdout
      tccb.optionPrintLogMirrorToStdout = 1;

    } else if (MATCH == strcmp("-ns", argv[i])) {
      // do not launch signal handler (useful for valgraid / gdb)
      tccb.optionNoSignalHandlerThread = 1;

    } else if (MATCH == strcmp("-nh", argv[i])) {
      // run without host pc
      tccb.optionNoHostCommunication = 1;

    } else if (MATCH == strcmp("-gd", argv[i])) {
      // go to die (by zero divide) for debug purpose
      tccb.optionGoToDie = 1;
      if (tccb.optionGoToDie) {
        i = 0;
        i = i / i;
      }
      exit(EXIT_FAILURE);

    } else if (MATCH == memcmp("-cs", argv[i], 3)) {
      // execute chamber script
      FILE *fp = NULL;
      Dword size = 0;

      fp = fopen(&argv[i][3], "rb");
      if (fp == NULL) {
        exit(EXIT_FAILURE);
      }
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      tccb.cs.arg = calloc(sizeof(ZQUE) + size + 16, 1);
      if (tccb.cs.arg == NULL) {
        exit(EXIT_FAILURE);
      }
      fread(&(((ZQUE *)tccb.cs.arg)->bData[0]), 1, size, fp);
      fclose(fp);
      ((ZQUE *)tccb.cs.arg)->wLgth = size + 1;
      ((ZQUE *)tccb.cs.arg)->bData[((ZQUE *)tccb.cs.arg)->wLgth - 1] = '\0';

    } else if (MATCH == strcmp("-lf", argv[i])) {
      // log filter for test result
      tccb.deleteLogFiles = 1;
      tcLogFilter(argv[i+1]);
      exit(EXIT_FAILURE);

    } else if (MATCH == memcmp("-qe", argv[i], 3)) {
      // quick environment status check
      tccb.deleteLogFiles = 1;
      tcQuickEnv(&argv[i][3]);
      exit(EXIT_FAILURE);

    } else if (MATCH == memcmp("-cu", argv[i], 3)) {
      // clean up envirnment (power-off, safe handling temperature
      tcCleanUpCell(&argv[i][3]);
      exit(EXIT_FAILURE);

    } else if (MATCH == memcmp("-bd", argv[i], 3)) {
      // bench debug mode (no wait for safe handling mode)
      int thread = 0;

      tccb.optionBenchDebug = 1;

      for (thread = 0 ; thread < tccb.optionNumOfTestScriptThread ; thread++) {
        rc = setPositiveTemperatureRampRate(tccb.ts[thread].wHGSTCellNo + 1, POS_RAMP_RATE_TEMP_FOR_BENCHDEBUG);
        if (rc) {
          exit(EXIT_FAILURE);
        }
        rc = setNegativeTemperatureRampRate(tccb.ts[thread].wHGSTCellNo + 1, NEG_RAMP_RATE_TEMP_FOR_BENCHDEBUG);
        if (rc) {
          exit(EXIT_FAILURE);
        }
      }

    } else if (MATCH == memcmp("-wa", argv[i], 3)) {
      tccb.workaroundForAckMissingInOptimus = 1;

    } else if (MATCH == memcmp("-ds", argv[i], 3)) {
      tccb.optionDevelopmentStation = 1;

    } else if (MATCH == memcmp("-ln", argv[i], 3)) {
      snprintf(&tcPrintfFilename[0], sizeof(tcPrintfFilename), "%s.log", &argv[i][3]);

    } else {
      // config file
      FILE *fp = NULL;
      Dword size = 0;
      printf(" Enter: config file \n");

      if (tccb.optionNumOfTestScriptThread > (sizeof(tccb.ts) / sizeof(tccb.ts[0]))) {
        exit(EXIT_FAILURE);
      }

      fp = fopen(argv[i], "rb");
      if (fp == NULL) {
        exit(EXIT_FAILURE);
      }
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      tccb.ts[tccb.optionNumOfTestScriptThread].arg = calloc(sizeof(ZQUE) + sizeof(TEST_PARAM_INITIAL_TEST) + size + 16, 1); // free at runTestScript in tsLib.c
      if (tccb.ts[tccb.optionNumOfTestScriptThread].arg == NULL) {
        exit(EXIT_FAILURE);
      }
      fread(&(((ZQUE *)tccb.ts[tccb.optionNumOfTestScriptThread].arg)->bData[0]), 1, size, fp);
      fclose(fp);
      ((ZQUE *)tccb.ts[tccb.optionNumOfTestScriptThread].arg)->wLgth = size + sizeof(TEST_PARAM_INITIAL_TEST);    //it should include '\0'
      ((ZQUE *)tccb.ts[tccb.optionNumOfTestScriptThread].arg)->bData[((ZQUE *)tccb.ts[tccb.optionNumOfTestScriptThread].arg)->wLgth - 1] = '\0';

      // parse to find index and set task id for GAIA compatibility
      rc = tcParseTestLocationIndex(&((ZQUE *)tccb.ts[tccb.optionNumOfTestScriptThread].arg)->bData[0], &tccb.ts[tccb.optionNumOfTestScriptThread]);
      if (rc) {
        exit(EXIT_FAILURE);
      }

      // Cell Index should be same over all config file
      if (tccb.optionNumOfTestScriptThread > 0) {
        if (tccb.ts[tccb.optionNumOfTestScriptThread].wCellIndex != tccb.ts[tccb.optionNumOfTestScriptThread - 1].wCellIndex) {
          exit(EXIT_FAILURE);
        }
      }

      // call constructor
      printf(" siInitialize \n");
      rc = siInitialize(tccb.ts[tccb.optionNumOfTestScriptThread].wHGSTCellNo + 1);
      if (rc) {
        printf(" Initialize error \n");
        exit(EXIT_FAILURE);
      }

      //
      // Workaround ONLY for EP No UART [DEBUG]
      //
      // CAUTION : Please do not apply this for Mobile system!!!!!!!
      //
      // 2011/05/26 apply for Mobile system because extended neptune firmware criteria.
      //
      rc = setTemperatureEnvelope(tccb.ts[tccb.optionNumOfTestScriptThread].wHGSTCellNo + 1, ENVELOP_TEMP);
      if (rc) {
        printf(" setTemperatureEnvelope error %d\n", rc);
        exit(EXIT_FAILURE);
      }


      // emulate CONT card control -- set nominal temp at test start
      rc = setTargetTemperature(tccb.ts[tccb.optionNumOfTestScriptThread].wHGSTCellNo + 1, NOMINAL_TEMP);
      if (rc) {
        printf(" setTartgetTemperatureRampRate error \n");
        exit(EXIT_FAILURE);
      }
      rc = setPositiveTemperatureRampRate(tccb.ts[tccb.optionNumOfTestScriptThread].wHGSTCellNo + 1, POS_RAMP_RATE_TEMP_FOR_PRODUCTION);
      if (rc) {
        printf(" setPositiveTemperatureRampRate error \n");
        exit(EXIT_FAILURE);
      }
      rc = setNegativeTemperatureRampRate(tccb.ts[tccb.optionNumOfTestScriptThread].wHGSTCellNo + 1, NEG_RAMP_RATE_TEMP_FOR_PRODUCTION);
      if (rc) {
        printf(" setNegativeTemperatureRampRate error \n");
        exit(EXIT_FAILURE);
      }

      // set default fan rpm
      rc = siSetFanRPMDefault(tccb.ts[tccb.optionNumOfTestScriptThread].wHGSTCellNo + 1);
      if (rc) {
        printf(" setFanRPMDefault error \n");
        exit(EXIT_FAILURE);
      }

      // set default uart I/O voltage
      rc = setUartPullupVoltage(tccb.ts[tccb.optionNumOfTestScriptThread].wHGSTCellNo + 1,0);
      if (rc) {
        printf(" setUartPullupVoltageDefault error \n");
        exit(EXIT_FAILURE);
      }
      printf(" Exit: config file \n");

// this line should be at bottom of this scope
      tccb.optionNumOfTestScriptThread++;

    }
  }
  tccb.optionSyncManagerDebug = 1;
  //tccb.optionDevelopmentStation = 1;

  //
  // hello message
  //
  TCPRINTF("Entry: pthread_t,%08lxh", (unsigned long)pthread_self());
  TCPRINTF("this was compiled on %s %s", __TIME__, __DATE__);
  vcGetVersion(NULL, NULL);
  for (i = 0 ; i < argc ; i++) {
    TCPRINTF("argv[%d] = %s", i, argv[i]);
  }
  tcDumpIpAddress();


  //
  // Keep testcase executable for log filtering
  //
  tcTestcaseInZip();


  //
  // Launch Sync Manager Thread and Detach
  //

  // analyze chamber file :delete resume index file if no elapsed() or elapsed(0)
  if (((ZQUE *)tccb.cs.arg)->wLgth != 0 ) {
    char *sp;
    sp = strstr(((ZQUE *)tccb.cs.arg)->bData, "elapsed");
    if (sp != NULL) {
      tcAddResumeInfomation(sp);
    } else {
      TCPRINTF("!!!!!!!!!!!!!!!!!!!!!not found elapsed INITIAL TEST!!!!!!!!!!!!!!!!!!!!!!!!!!");
      rc = csDeleteResumeIdx();
      if (rc) {
        TCPRINTF("!!!!!!!!!!!ERROR cannot delete save index file!!!!!!!!!!!!!!!!!");
        exit(EXIT_FAILURE);
      }
//      for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
//        strcat(((ZQUE *)tccb.ts[i].arg)->bData, TEST_PARAM_INITIAL_TEST);
//        ((ZQUE *)tccb.ts[i].arg)->bData[(((ZQUE *)tccb.ts[i].arg)->wLgth) - 1] = '\0';
//      }
    }
  }

  //
  // Launch Signal Handler Thread and Detach
  //
  if (tccb.optionNoSignalHandlerThread == 0) {
    sigset_t  set;
    ret = sigfillset(&set);
    if (ret) {
      TCPRINTF("error");
      goto ERROR_MAIN;
    }
    ret = pthread_sigmask(SIG_SETMASK, &set, NULL);
    if (ret) {
      TCPRINTF("error");
      goto ERROR_MAIN;
    }
    ret = pthread_create(&tccb.sh.thread, &tccb.sh.attr, tccb.sh.start_routine, tccb.sh.arg);
    if (ret) {
      TCPRINTF("error");
      goto ERROR_MAIN;
    }
    ret = pthread_detach(tccb.sh.thread);
    if (ret) {
      TCPRINTF("error");
      goto ERROR_MAIN;
    }
  }


  //
  // Launch Sync Manager Thread and Detach
  //
  if (tccb.optionSyncManagerDebug) {
    ret = pthread_create(&tccb.sm.thread, &tccb.sm.attr, tccb.sm.start_routine, tccb.sm.arg);
    if (ret) {
      TCPRINTF("error");
      goto ERROR_MAIN;
    }
    ret = pthread_detach(tccb.sm.thread);
    if (ret) {
      TCPRINTF("error");
      goto ERROR_MAIN;
    }
  }

  ret = pthread_create(&tccb.cs.thread, &tccb.cs.attr, tccb.cs.start_routine, tccb.cs.arg);
  if (ret) {
    TCPRINTF("error");
    goto ERROR_MAIN;
  }
  ret = pthread_detach(tccb.cs.thread);
  if (ret) {
    TCPRINTF("error");
    goto ERROR_MAIN;
  }

  //
  // Sync with Chamber Script Thread for Resume Test
  //
  TCPRINTF("waiting until chamber script ready to start...");
  for (;;) {
    if (tccb.optionSyncManagerDebug) {
      rc = tcWaitCSReady();
      if (rc) {
        goto ERROR_MAIN;
      }
      break;
    } else {
      if (tccb.isChamberScriptReadyToStart) {
        break;
      }
    }
  }
  TCPRINTF("ready to go now");

  //
  // Launch Test Script Thread and Wait for Join
  //
  for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
    TCPRINTF("%s", ((ZQUE *)tccb.ts[i].arg)->bData);
    ret = pthread_create(&tccb.ts[i].thread, &tccb.ts[i].attr, tccb.ts[i].start_routine, tccb.ts[i].arg);
    if (ret) {
      TCPRINTF("error");
      goto ERROR_MAIN;
    }
  }
  if (tccb.optionSyncManagerDebug) {
    ret = pthread_mutex_unlock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error mutex unlock %d", ret);
      goto ERROR_MAIN;
    }
  }
  for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
    ret = pthread_join(tccb.ts[i].thread, &tccb.ts[i].thread_return);
    if (ret) {
      TCPRINTF("error");
      goto ERROR_MAIN;
    }
  }


  //
  // Finalize
  //
  TCPRINTF("TESTCASE FINALIZE PROCESS");
  tcExit(EXIT_SUCCESS);
ERROR_MAIN:
  tcExit(EXIT_FAILURE);
  exit(EXIT_FAILURE);
}


/* ---------------------------------------------------------------------- */
void tcPrintfFileInZip(void)
/* ---------------------------------------------------------------------- */
{
  char command[1024];

  tcPrintfDisable = 1;

  if (tccb.deleteLogFiles) {
    snprintf(command, sizeof(command), "rm -f %s > /dev/null 2>&1", &tcPrintfFilename[0]);
    system(command);
  } else {
    snprintf(command, sizeof(command), "zip -m %s.zip %s > /dev/null 2>&1", &tcPrintfFilename[0], &tcPrintfFilename[0]);
    system(command);
  }
}


/* ---------------------------------------------------------------------- */
void tcPrintf(char *bFileName, unsigned long dwNumOfLine, const char *bFuncName, char *description, ...)
/* ---------------------------------------------------------------------- */
{
  char lineBuf[16384];
  int lineSizeNow = 0;
  int lineSizeMax = sizeof(lineBuf) - 6;  // 1 byte for addtional '\n'
  // 1 byte for '\0'
  // 4 byte for margin
  int ret = 0;
  int size = 0;
  static FILE *logFilePtr = NULL;

  if (tcPrintfDisable) {
    return;
  }

#define DISPLAY_MSEC_TIME

  // get time
#if defined(DISPLAY_MSEC_TIME)
  struct timeval tv;
  struct timezone tz;
  struct tm tm;
  gettimeofday(&tv, &tz);
  localtime_r((const time_t *)&tv.tv_sec, &tm);
#else
  // get time
  time_t timep;
  struct tm tm;

  time(&timep);
  localtime_r(&timep, &tm);
#endif

  ret = pthread_mutex_lock(&mutex_tcPrintf);
  if (ret) {
    exit(EXIT_FAILURE);
  }

  if (strlen(tcPrintfFilename) > 0) {
    logFilePtr = fopen(tcPrintfFilename, "a+b");
    if (NULL) {
      exit(EXIT_FAILURE);
    }
  }

  // time stamp
  size = lineSizeNow < lineSizeMax ? lineSizeMax - lineSizeNow : 0;
#if defined(DISPLAY_MSEC_TIME)
  ret = snprintf(&lineBuf[lineSizeNow], size, "%04d/%02d/%02d,%02d.%02d:%02d:%03ld,",
                 tm.tm_year + 1900,
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec,
                 tv.tv_usec / 1000
                );
#else
  ret = snprintf(&lineBuf[lineSizeNow], size, "%04d/%02d/%02d,%02d:%02d:%02d,",
                 tm.tm_year + 1900,
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec
                );
#endif
  if (ret < size) {
    lineSizeNow += ret;
  } else {
    lineSizeNow += size - 1;
  }

  // file name and line #
  size = lineSizeNow < lineSizeMax ? lineSizeMax - lineSizeNow : 0;
  ret = snprintf(&lineBuf[lineSizeNow], size, "%s,%ld,%s,", bFileName, dwNumOfLine, bFuncName);
  if (ret < size) {
    lineSizeNow += ret;
  } else {
    lineSizeNow += size - 1;
  }

  // description
  va_list ap;
  va_start(ap, description);
  size = lineSizeNow < lineSizeMax ? lineSizeMax - lineSizeNow : 0;
  ret = vsnprintf(&lineBuf[lineSizeNow], size, description, ap);
  if (ret < size) {
    lineSizeNow += ret;
  } else {
    lineSizeNow += size - 1;
  }
  va_end(ap);

  // add a newline only if not one already
  if (lineBuf[lineSizeNow - 1] != '\n') {
    ret = snprintf(&lineBuf[lineSizeNow], 2, "\n");  // 2 = '\n' + '\0'
    lineSizeNow += 1;
    lineSizeMax += 1;
  }

  // Sweep out to Console
  if (logFilePtr != NULL) {
    fwrite(&lineBuf[0], 1, lineSizeNow, logFilePtr);
    fflush(logFilePtr);
  }
  if (tccb.optionPrintLogMirrorToStdout) {
    printf("%s", &lineBuf[0]);
    fflush(stdout);
  }

  // closing
  if (logFilePtr != NULL) {
    fclose(logFilePtr);
  }
  ret = pthread_mutex_unlock(&mutex_tcPrintf);
  if (ret) {
    exit(EXIT_FAILURE);
  }
}


/* ---------------------------------------------------------------------- */
void tcExit(int exitcode)
/* ---------------------------------------------------------------------- */
{
  int i = 0;
  Byte rc = 0;
  static int isFirstVisit = 1;
  int ret = 0;
  int isAirTempCheckOnly = 0;
  Byte isRecursiveCall = 0;

  TCPRINTF("%d %d", exitcode, isFirstVisit);

  /* dump */
  isRecursiveCall = tcDumpBacktrace(__func__);
  if (isRecursiveCall) {
    TCPRINTF("reentrant tcExit from tcExit,%d", isRecursiveCall);
    exit(EXIT_FAILURE);
  }

  /* exit immediately if reentrant this function */
  ret = pthread_mutex_lock(&mutex_tcExit);
  if (ret) {
    TCPRINTF("mutex error");
    exit(EXIT_FAILURE);
  }
  if (isFirstVisit == 0) {
    TCPRINTF("reentrant tcExit, sleep...");
    for (;;) {
      sleep(60);
    }
  }
  isFirstVisit = 0;
  ret = pthread_mutex_unlock(&mutex_tcExit);
  if (ret) {
    TCPRINTF("mutex error");
    exit(EXIT_FAILURE);
  }

  //
  // Mutex TRY Lock (to avoid dead-lock) to block other threads controling slot io.
  // note: consider what if chamber script sets temperature while safe handling mode in here.
  //
  TCPRINTF("getting slotio mutex...");
  for (i = 10 ; i > 0 ; i--) {
    ret = pthread_mutex_trylock(&mutex_slotio);
    if (ret == 0) {
      break;
    } else {
      sleep(1);
    }
  }
  if (i == 0) {
    TCPRINTF("getting slotio mutex... ---> failed. but keep going to finalize");
  } else {
    TCPRINTF("getting slotio mutex... ---> pass");
  }


  if (isAirTempCheckOnly) {
    Word wTempInHundredth = 0;
    int safeHandlingTimeoutInSec = 0;

    // turn off driver power and set safe handling temperature
    // continue to next step even though error at here
    for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
      setTargetVoltage(tccb.ts[i].wHGSTCellNo + 1, DRIVE_POWER_OFF, DRIVE_POWER_OFF);
      setSafeHandlingTemperature(tccb.ts[i].wHGSTCellNo + 1, SAFE_HANDLING_TEMP);
    }

    // wait unless temp < SAFE_HANDLING_TEMP
    for (safeHandlingTimeoutInSec = 0 ; safeHandlingTimeoutInSec < MAX_SAFE_HANDLING_TIMEOUT_IN_SEC ; ) {
      int intervalSec = 15;
      int rearchSafeHandlingTemperature = 0;

      for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
        rc = getCurrentTemperature(tccb.ts[i].wHGSTCellNo + 1, &wTempInHundredth);
        if (rc) {
          TCPRINTF("error");
          exitcode = EXIT_FAILURE;
          goto END_TC_EXIT;
        }
        TCPRINTF("current temperature %d", wTempInHundredth);
        if (wTempInHundredth <= SAFE_HANDLING_TEMP) {
          rearchSafeHandlingTemperature = 1;
        } else {
          rearchSafeHandlingTemperature = 0;
        }
      }

      if (rearchSafeHandlingTemperature) {
        TCPRINTF("rearch safe handling temp");
        break;
      }

      if (tccb.optionBenchDebug) {
        TCPRINTF("bench debug mode --> quit without waiting safe handling temperature");
        break;
      }

      sleep(intervalSec);
      safeHandlingTimeoutInSec += intervalSec;
    }

  } else {
    Word wTempInHundredth = 0;
    int timeoutInMin = 0;

    // turn off driver power, continue to next step even though error at here
    for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
      setTargetVoltage(tccb.ts[i].wHGSTCellNo + 1, DRIVE_POWER_OFF, DRIVE_POWER_OFF);
    }

    rc = getCurrentTemperature(tccb.ts[0].wHGSTCellNo + 1, &wTempInHundredth);
    if (rc) {
      TCPRINTF("error");
      exitcode = EXIT_FAILURE;
      goto END_TC_EXIT;
    }
    TCPRINTF("current temperature %d", wTempInHundredth);

#if 0 // [DEBUG] change it for GSP XCalibre that has 25degC secondary water
    //
    // calculate timeout due to delta between current temp and safe temp
    //
    // Equation        : Timeout[min] = (CurrentTempDegC - 35) + 1
    //
    // CurrentTemp     : Timeout
    // ------------------------------------
    // ~35 degC        : 1 min
    //  40 degC        : 6 min (5 + 1)
    //  45 degC        : 11 min (10 + 1)
    //  50 degC        : 16 min (15 + 1)
    //  55 degC        : 21 min (20 + 1)
    //  60 degC        : 26 min (25 + 1)
    //
    timeoutInMin = 1;
    if (SAFE_HANDLING_TEMP < wTempInHundredth) {
      timeoutInMin += (wTempInHundredth - SAFE_HANDLING_TEMP) / 100;
    }
    TCPRINTF("safe handling timeout = %d", timeoutInMin);
#else
    //
    // calculate timeout due to delta between current temp and safe temp
    //
    // Equation        : Timeout[min] = (CurrentTempDegC - 35)
    //
    // CurrentTemp     : Timeout
    // ------------------------------------
    // ~35 degC        : 0 min
    //  40 degC        : 5 min
    //  45 degC        : 10 min
    //  50 degC        : 15 min
    //  55 degC        : 20 min
    //  60 degC        : 25 min
    //
    if (SAFE_HANDLING_TEMP_FOR_TIMEOUT_CALC < wTempInHundredth) {
      timeoutInMin = (wTempInHundredth - SAFE_HANDLING_TEMP_FOR_TIMEOUT_CALC) / 100;
    }
    TCPRINTF("safe handling timeout = %d", timeoutInMin);
#endif

    rc = getTargetTemperature(tccb.ts[0].wHGSTCellNo + 1, &wTempInHundredth);
    if (rc) {
      TCPRINTF("error");
      exitcode = EXIT_FAILURE;
      goto END_TC_EXIT;
    }
    TCPRINTF("target temperature = %d", wTempInHundredth);

    rc = setSafeHandlingTemperature(tccb.ts[0].wHGSTCellNo + 1, SAFE_HANDLING_TEMP);
    if (rc) {
      TCPRINTF("error");
      exitcode = EXIT_FAILURE;
      goto END_TC_EXIT;
    }

    // wait for timeout
    for ( ; timeoutInMin > 0 ; timeoutInMin--) {
      sleep(60);

      rc = getCurrentTemperature(tccb.ts[0].wHGSTCellNo + 1, &wTempInHundredth);
      if (rc) {
        TCPRINTF("error");
        exitcode = EXIT_FAILURE;
        goto END_TC_EXIT;
      }
      TCPRINTF("current temperature = %d", wTempInHundredth);

      if (tccb.optionBenchDebug) {
        TCPRINTF("bench debug mode --> quit without waiting safe handling temperature");
        break;
      }
    }
  }

END_TC_EXIT:
  // finalize slot io, no error check because of nothing to do...
  for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
    siFinalize(tccb.ts[i].wHGSTCellNo + 1);
  }

  //
  // [DEBUG] to do -- will implement for brownout
  //
  // overwrite exitcode to notify host Cell offline status.
  // host may work around for brownout
  //
  // offline trigger by xyrc
  //   xyrcConnReset
  //   xyrcCellNotAccepting
  //   xyrcCellNotOnline
  //
  if (tccb.isCellOffline) {
    exitcode = EXIT_CELL_OFFLINE;
  }

  exit(exitcode);
}


/* ---------------------------------------------------------------------- */
Byte tcParseTestLocationIndex(const char config[], GTCB *gtcb)
/* ---------------------------------------------------------------------- */
{
  const char *signaturewHGSTCellNo = "//bCellNo:";
  const char *signaturewCellIndex = "//wCellIndex:";
  const char *signaturewTrayIndex = "//wTrayIndex:";
  int val = 0;
  int offset = 0;
  Byte rc = 0;

  TCPRINTF("Entry");

  //
  // Parse wHGSTCellNo
  //
  offset = 0;
  if (MATCH != memcmp(signaturewHGSTCellNo, &config[offset], strlen(signaturewHGSTCellNo))) {
    TCPRINTF("wHGSTCellNo signature error. it should be %s", signaturewHGSTCellNo);
    rc = 1;
    goto END_TC_PARSE_TEST_LOCATION_INDEX;
  }
  val = atoi(&config[offset + strlen(signaturewHGSTCellNo)]);
  if ((val < 1) || (UCHAR_MAX < val)) {
    TCPRINTF("wHGSTCellNo error %d", val);
    rc = 1;
    goto END_TC_PARSE_TEST_LOCATION_INDEX;
  }
  val--; // convert one-base to zero-base
  gtcb->wHGSTCellNo = val;

  // goto next line
  while (config[offset++] != '\n');

  //
  // Parse bCellIndex
  //
  if (MATCH != memcmp(signaturewCellIndex, &config[offset], strlen(signaturewCellIndex))) {
    TCPRINTF("wCellIndex signature error. it should be %s", signaturewCellIndex);
    rc = 1;
    goto END_TC_PARSE_TEST_LOCATION_INDEX;
  }
  val = atoi(&config[offset + strlen(signaturewCellIndex)]);
  if ((val < 0) || (UCHAR_MAX < val)) {
    TCPRINTF("wCellIndex error %d", val);
    rc = 1;
    goto END_TC_PARSE_TEST_LOCATION_INDEX;
  }
  gtcb->wCellIndex = val;

  // goto next line
  while (config[offset++] != '\n');

  //
  // Parse bTrayIndex
  //
  if (MATCH != memcmp(signaturewTrayIndex, &config[offset], strlen(signaturewTrayIndex))) {
    TCPRINTF("wTrayIndex signature error. it should be %s", signaturewTrayIndex);
    rc = 1;
    goto END_TC_PARSE_TEST_LOCATION_INDEX;
  }
  val = atoi(&config[offset + strlen(signaturewTrayIndex)]);
  if ((val < 0) || (UCHAR_MAX < val)) {
    TCPRINTF("wTrayIndex error %d", val);
    rc = 1;
    goto END_TC_PARSE_TEST_LOCATION_INDEX;
  }
  gtcb->wTrayIndex = val;

END_TC_PARSE_TEST_LOCATION_INDEX:
  TCPRINTF("rc,%d,wHGSTCellNo(zero-base),%d,wCellIndex,%d,wTrayIndex,%d", rc, gtcb->wHGSTCellNo, gtcb->wCellIndex, gtcb->wTrayIndex);
  return rc;
}


/* ---------------------------------------------------------------------- */
void tcDumpCore(void)
/* ---------------------------------------------------------------------- */
{
  const char core_extension[] = "core";
  char core_filename[sizeof(tcPrintfFilename)];
  char command[1024];

  if (tcPrintfFilename[0] == 0) {
    return;
  }

  memcpy(&core_filename[0], &tcPrintfFilename[0], sizeof(core_filename));
  memcpy(&core_filename[strlen(core_filename) - 3], &core_extension[0], strlen(core_extension));

  //
  // Note: you may find following message by gcore
  //           warning: .dynamic section for "/lib/libc.so.6" is not at the expected address
  //           warning: difference appears to be caused by prelink, adjusting expectations
  //
  //       following web site explains the reason for the message (gdb fixes prelink base address)
  //           Subject: cope with varying prelink base addresses
  //           URL:     http://www.cygwin.com/ml/gdb-patches/2006-02/msg00164.html for gdb patch
  //
  if (tccb.deleteLogFiles) {
    /* nop */
  } else {
    snprintf(command, sizeof(command), "gcore -o %s %d > /dev/null 2>&1", &core_filename[0], getpid());
    system(command);

    snprintf(command, sizeof(command), "zip -m %s.zip %s.%d > /dev/null 2>&1", &core_filename[0], &core_filename[0], getpid());
    system(command);
  }
}

/**
 * <pre>
 * Description:
 *  Display back trace by using tcPrintf() and check recursive call
 * Arguments:
 *   recursiveCheckFunc - INPUT - function name to be checked recursive call
 * Return:
 *   0: no recursive call. 1:recursive call 2:back trace is empty 3:recursiveCheckFunc is empty
 * Note:
 * </pre>
 *****************************************************************************/
/* ---------------------------------------------------------------------- */
Byte tcDumpBacktrace(const char *recursiveCheckFunc )
/* ---------------------------------------------------------------------- */
{
  int j = 0, nptrs = 0;
  void *buffer[100];
  char **strings = NULL;
  Byte recursiveCounter = 0;
  Byte rc = 0;
  char *return_strstr = NULL;

  TCPRINTF("Entry");
  /* Note) Why to check empty string?
  *
  * Since strstr() behavior is tricky with empty string -- see following strstr() spec --,
  * we exclude empty string in order to simplify this function.
  *
  * Function: char * strstr (const char *HAYSTACK, const char *NEEDLE)
  *
  *     This is like `strchr', except that it searches HAYSTACK for a
  *    substring NEEDLE rather than just a single character.  It returns
  *     a pointer into the string HAYSTACK that is the first character of
  *     the substring, or a null pointer if no match was found.  If NEEDLE
  *     is an empty string, the function returns HAYSTACK.
  */

  if (recursiveCheckFunc == NULL) {
    TCPRINTF("ERROR - recursiveCheckFunc is NULL");
    rc = 3;
    goto END_TC_DUMP_BACKTRACE;
  }
  if (recursiveCheckFunc == "") {
    TCPRINTF("ERROR - recursiveCheckFunc is empty");
    rc = 3;
    goto END_TC_DUMP_BACKTRACE;
  }

  nptrs = backtrace(buffer, sizeof(buffer));
  TCPRINTF("backtrace() returned %d addresses", nptrs);

  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    TCPRINTF("warning - backtrace_symbols");
    rc = 2;
    goto END_TC_DUMP_BACKTRACE;
  }
  for (j = 0; j < nptrs; j++) {
    TCPRINTF("%s", strings[j]);
    return_strstr = strstr(strings[j], recursiveCheckFunc);
    if (return_strstr != NULL) {
      if ((*(return_strstr + strlen(recursiveCheckFunc)) == '+') && (*(return_strstr - 1) == '(')) {
        recursiveCounter++;
        TCPRINTF("recursiveCounter %d", recursiveCounter);
      }
    }
  }
  if (recursiveCounter >= 2) {
    TCPRINTF("warning - found recursive call %s", recursiveCheckFunc);
    rc = 1;
  }
END_TC_DUMP_BACKTRACE:
  free(strings);
  return rc;
}


/* ---------------------------------------------------------------------- */
void tcQuickEnv(const char *param)
/* ---------------------------------------------------------------------- */
{
#define HGST_CELL_NUM_MAX    240
  Byte bHGSTCellNo = 0;
  Byte bHGSTCellNoMax = HGST_CELL_NUM_MAX;
  char *cellStatus[HGST_CELL_NUM_MAX];
  char *plug[HGST_CELL_NUM_MAX];
  char *envError[HGST_CELL_NUM_MAX];
  float currentTemp[HGST_CELL_NUM_MAX];
  float currentVolt5V[HGST_CELL_NUM_MAX];
  float currentVolt12V[HGST_CELL_NUM_MAX];
  int paramInt = atoi(param);

  if (paramInt == 0) {
    bHGSTCellNo = 0;
    bHGSTCellNoMax = HGST_CELL_NUM_MAX;
  } else {
    bHGSTCellNo = paramInt - 1;
    bHGSTCellNoMax = paramInt;
  }

  printf("\n");
  printf("HGST,Cell   ,Plug,Env  ,Curr   ,CurrVolt,CurrVolt,\n");
  printf("Cell,Status ,    ,Error,Temp[C],X[mV]   ,Y[mV]   ,\n");
  printf("----------------------------------------------------------------------\n");

  for ( ; bHGSTCellNo < bHGSTCellNoMax ; bHGSTCellNo++) {
    Byte rc = 0;
    Word wTempInHundredth = 0;
    Word wCurrentV5InMilliVolts = 0;
    Word wCurrentV12InMilliVolts = 0;

    cellStatus[bHGSTCellNo] = "";
    envError[bHGSTCellNo] = "";
    plug[bHGSTCellNo] = "";
    currentTemp[bHGSTCellNo] = 0;
    currentVolt5V[bHGSTCellNo] = 0;
    currentVolt12V[bHGSTCellNo] = 0;

    rc = siInitialize(bHGSTCellNo + 1);
    if (rc) {
      cellStatus[bHGSTCellNo] = "Offline";
      continue;
    } else {
      cellStatus[bHGSTCellNo] = "Online";
    }

    rc = isCellEnvironmentError(bHGSTCellNo + 1);
    if (rc) {
      envError[bHGSTCellNo] = "Yes";
    } else {
      envError[bHGSTCellNo] = "No";
    }

    rc = isDrivePlugged(bHGSTCellNo + 1);
    if (rc) {
      plug[bHGSTCellNo] = "Yes";
    } else {
      plug[bHGSTCellNo] = "No";
    }

    rc = getCurrentTemperature(bHGSTCellNo + 1, &wTempInHundredth);
    if (rc) {
      continue;
    }
    currentTemp[bHGSTCellNo] = wTempInHundredth / 100.0;

    rc = getCurrentVoltage(bHGSTCellNo + 1, &wCurrentV5InMilliVolts, &wCurrentV12InMilliVolts);
    if (rc) {
      continue;
    }
    currentVolt5V[bHGSTCellNo] = wCurrentV5InMilliVolts / 1000.0;
    currentVolt12V[bHGSTCellNo] = wCurrentV12InMilliVolts / 1000.0;

    siFinalize(bHGSTCellNo + 1);

    printf("%4d,%7.7s,%4.4s,%5.5s,%7.2f,%8.2f,%8.2f,\n",
           bHGSTCellNo + 1,
           cellStatus[bHGSTCellNo],
           plug[bHGSTCellNo],
           envError[bHGSTCellNo],
           currentTemp[bHGSTCellNo],
           currentVolt5V[bHGSTCellNo],
           currentVolt12V[bHGSTCellNo]
          );
  }

  exit(EXIT_FAILURE);
}


/* ---------------------------------------------------------------------- */
void tcCleanUpCell(const char *param)
/* ---------------------------------------------------------------------- */
{
  Byte bHGSTCellNo = 0;
  Byte rc = 0;
  Byte exitcode = EXIT_SUCCESS;

  TCPRINTF("Entry:%s", param);

  bHGSTCellNo = (atoi(param) - 1) & 0xff;
  if (240 <= bHGSTCellNo) {
    TCPRINTF("error");
    exit(EXIT_FAILURE);
  }

  rc = siInitialize(bHGSTCellNo + 1);
  if (rc) {
    TCPRINTF("error");
    exit(EXIT_FAILURE);
  }

  rc = setTargetVoltage(bHGSTCellNo + 1, DRIVE_POWER_OFF, DRIVE_POWER_OFF);
  if (rc) {
    TCPRINTF("error");
    exit(EXIT_FAILURE);
  }

  rc = setSafeHandlingTemperature(bHGSTCellNo + 1, SAFE_HANDLING_TEMP);
  if (rc) {
    TCPRINTF("error");
    exit(EXIT_FAILURE);
  }

  rc = siFinalize(bHGSTCellNo + 1);
  if (rc) {
    TCPRINTF("error");
    exit(EXIT_FAILURE);
  }

  if (tccb.isCellOffline) {
    exitcode = EXIT_CELL_OFFLINE;
  }

  exit(exitcode);
}


/* ---------------------------------------------------------------------- */
void tcTestcaseInZip(void)
/* ---------------------------------------------------------------------- */
{
  char testcase_filename[sizeof(tcPrintfFilename)];
  char command[1024];
  int i = 0;

  if (tcPrintfFilename[0] == 0) {
    return;
  }

  // remove path (e.g ./testcase ==> testcase)
  if (sizeof(testcase_filename) < strlen(tccb.argv[0])) {
    return;
  }

  // find '/' from end to top
  for (i = strlen(tccb.argv[0]) - 1 ; i > 1 ; i--) {
    if (tccb.argv[0][i - 1] == '/') {
      break;
    }
  }
  memcpy(&testcase_filename[0], &tcPrintfFilename[0], sizeof(testcase_filename));
  snprintf(&testcase_filename[strlen(testcase_filename) - 3],
           sizeof(testcase_filename) - strlen(tccb.argv[0]),
           "%s",
           &tccb.argv[0][i]);

  if (tccb.deleteLogFiles) {
    /* nop */
  } else {
    snprintf(command, sizeof(command), "zip %s.zip %s > /dev/null 2>&1", &testcase_filename[0], tccb.argv[0]);
    system(command);
  }
}


/* ---------------------------------------------------------------------- */
void tcDumpIpAddress(void)
/* ---------------------------------------------------------------------- */
{
  int fd = 0;
  struct ifreq ifr;
  int rc = 0;

  fd = socket(AF_INET, SOCK_DGRAM, 0);

  strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
  rc = ioctl(fd, SIOCGIFADDR, &ifr);
  if (rc == 0) {
    TCPRINTF("%s,%s", ifr.ifr_name, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
  }

  strncpy(ifr.ifr_name, "eth1", IFNAMSIZ - 1);
  rc = ioctl(fd, SIOCGIFADDR, &ifr);
  if (rc == 0) {
    TCPRINTF("%s,%s", ifr.ifr_name, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
  }

  close(fd);

  return;
}


/* ---------------------------------------------------------------------- */
void tcDumpData(Byte *bData, Dword dwSize, Dword dwAddress)   // [DEBUG] [UART3]
/* ---------------------------------------------------------------------- */
{
  const Byte bBinary2AsciiOfPrintf[] = "0123456789ABCDEF";
  Byte bLineBuffer[64 + 1];
  Byte bByteBuffer;
  Dword dwOffsetToBinary = 0;
  Dword dwOffsetToAscii = 48;
  Dword i;

  memset(&bLineBuffer[0], ' ', sizeof(bLineBuffer));
  bLineBuffer[sizeof(bLineBuffer)-1] = '\0';

  for (i = 0 ; i < dwSize ; i++) {
    /* Get One Byte */
    bByteBuffer = *(Byte *)((Dword)bData + i);

    /* Set Binary Format */
    bLineBuffer[dwOffsetToBinary] = bBinary2AsciiOfPrintf[(bByteBuffer & 0xF0) >> 4];
    bLineBuffer[dwOffsetToBinary + 1] = bBinary2AsciiOfPrintf[bByteBuffer & 0x0F];
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
      printf("  [%08lx] %s\n", dwAddress + i - 15, &bLineBuffer[0]);
      memset(bLineBuffer, ' ', 64);
      bLineBuffer[sizeof(bLineBuffer)-1] = '\0';

    } else {
      /* Move foward */
      dwOffsetToBinary += 3;
      dwOffsetToAscii++;
    }
  }

  /* Sweep out buffer */
  if (dwOffsetToAscii != 48) {
    tcPrintf(__FILE__, __LINE__, __func__, "  [%08lx] %s\n", dwAddress + i - (i % 16), &bLineBuffer[0]);
  }
}

/* ---------------------------------------------------------------------- */
void tcAddResumeInfomation(char *sp)
/* ---------------------------------------------------------------------- */
{
//  int i = 0;
  Byte rc = 0;
  TCPRINTF("!!!!!!!!!!!!!!!!!!!!!found elapsed!!!!!!!!!!!!!!!!!!!!!!!!!!");
  sp = sp + 7;
  for (;*sp != ')';sp++) {
    if (isdigit(*sp)) {
      if (*sp != '0') {
        TCPRINTF("!!!!!!!!!!!sequence time is not '0' RESUME TEST !!!!!!!!!!!!!!!!!");
        break;
      }
    }
  }
  if (*sp == ')') {
    TCPRINTF("!!!!!!!!!!!sequence time is '0' INITIAL TEST !!!!!!!!!!!!!!!!!");
    rc = csDeleteResumeIdx();
    if (rc) {
      TCPRINTF("!!!!!!!!!!!ERROR cannot delete save index file!!!!!!!!!!!!!!!!!");
      exit(EXIT_FAILURE);
    }
//   for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
//     strcat(((ZQUE *)tccb.ts[i].arg)->bData, TEST_PARAM_INITIAL_TEST);
//      ((ZQUE *)tccb.ts[i].arg)->bData[(((ZQUE *)tccb.ts[i].arg)->wLgth) - 1] = '\0';
//    }
  }
}

/* ---------------------------------------------------------------------- */
Byte tcWaitCSReady(void)
/* ---------------------------------------------------------------------- */
{
  char addString[112] = "";
  SM_MESSAGE smMessage;
  int ret;
  int i;
  Byte rc = 0;
  ret = pthread_mutex_lock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error mutex lock %d", ret);
    rc = SM_MUTEX_UNLOCK_ERROR;
    return rc;
  }
  rc = smReceiveSyncMessage(SM_TC_RV_TIMEOUT, &smMessage);
  TCPRINTF("smMessage.type = %d SM_TC_RV_CSREADY = %d", (int)smMessage.type, SM_TC_RV_CSREADY);
  if (rc) {
    TCPRINTF("error");
    return rc;
  }
  if (smMessage.type == SM_TC_RV_CSREADY) {
    Dword minute = smMessage.param[0];
    if (minute > (60*24*365)) {
      TCPRINTF("error");
      rc = 1;
    } else {
      snprintf(addString, sizeof(addString), "\nTEST_PARAM  dwDriveTestElapsedTimeSec                      %04d:%02d:00    // inserted by testcase automatically", (int)(minute / 60), (int)(minute % 60));
      for (i = 0 ; i < tccb.optionNumOfTestScriptThread ; i++) {
        strcat(((ZQUE *)tccb.ts[i].arg)->bData, addString);
        ((ZQUE *)tccb.ts[i].arg)->bData[(((ZQUE *)tccb.ts[i].arg)->wLgth) - 1] = '\0';
      }
    }
    rc = 0;
  } else {
    rc = 1;
    TCPRINTF("smMessage.type = %d SM_TC_RV_CSREADY = %d", (int)smMessage.type, SM_TC_RV_CSREADY);
    TCPRINTF("error");
  }
  return rc;
}
