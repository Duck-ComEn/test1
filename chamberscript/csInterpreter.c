#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "cs.h"

#define MATCH                          (0)     // makes use of strncmp() more readable
extern const char csNameSetTemp[];
extern const char csNameSetVolt[];
extern const char csNameHoldMin[];
extern const char csNameCommand[];
extern const char csNameResumeCommand[];
extern const char csNameSaveInterval[];
extern const char csNameRestart[];
extern const char csNameSequence[];
extern const char csNameEndSequence[];
extern const char csNameLoop[];
extern const char csNameSave[];
extern const char csNameVersion[];
extern const char csNameSumValue[];
extern const char csNameLabel[];
extern const char csNameResume[];
extern const char csNameInitialTemperatureSet[];
int execIdx;
Byte resumeFlg = 0;
csInitialTemperature csITemp;

/*****************************************************************************/
int csInterpreter( const char *sequenceFile, long sequenceLength )
/*****************************************************************************/
{
  const long MaxFileSize = MAX_FILE_SIZE;
  const long MaxCtrlStep = MAX_CTRL_STEP;
  const csCtrlStep csZeroStructure = { NULL };

  char  mySequence[MaxFileSize];
  long  myLength = 0;
  long  idx = 0;
  long  stIdx = 0;
  long  clearIdx = 0;
  long  restartIdx = 0;
  Dword restartTime = 0;

  int   status;
  //  long  *seqStatus;
  int   rc = 0;
  Byte  returnCode;

  csCtrlStep  csCtrlStructure[MaxCtrlStep];

  TCPRINTF("Entry:%p,%ld", sequenceFile, sequenceLength);

  // Zero clear for all structure

  for ( clearIdx = 0; clearIdx < MaxCtrlStep; clearIdx++ ) {
    csCtrlStructure[clearIdx] = csZeroStructure;
  }

  // Zero clear for sequence area

  memset( mySequence, '\0', MaxFileSize );

  TCPRINTF(" +++ csInterpreter run.");

  // Error Check part ( return : 2 )
  if ( sequenceFile == NULL ) {
    TCPRINTF(" ** Input file error! : Address is NULL.");
    rc = 1;
    goto END_CS_INTERPRETER;
  }

  if ( (unsigned long)&sequenceFile & 0x00000003 ) {
    TCPRINTF(" ** Input file error! : Address is not 4bytes aligned.");
    rc = 1;
    goto END_CS_INTERPRETER;
  }

  if ( sequenceLength > MaxFileSize ) {
    TCPRINTF(" ** Input file error! : Size over %.0f kbytes.", (float)(MaxFileSize / 1024) );
    rc = 1;
    goto END_CS_INTERPRETER;
  }

  if ( sequenceLength <= 0 ) {
    TCPRINTF(" ** Input file error! : Size is under 0.");
    rc = 1;
    goto END_CS_INTERPRETER;
  }

  TCPRINTF(" +++ input file has no error.");

  // Memory allocation for sequence data in my thread.
  status = csAllocateMyArea( sequenceFile, sequenceLength, mySequence, &myLength );
  if ( status > 0 ) {
    rc = 3;
    goto END_CS_INTERPRETER;
  }

  while ( idx < myLength ) {
    //    putchar(mySequence[idx]);
    idx++;
  }

  idx = 0;
  TCPRINTF(" -- myLength : %ld", myLength);
  fflush(stdout);


  // Sugering data.
  status = csCosmeticSurgery( mySequence, &myLength );
  if ( status > 0 ) {
    rc = 4;
    goto END_CS_INTERPRETER;
  }

  while ( idx < myLength ) {
    //    putchar(mySequence[idx]);
    idx++;
  }

  TCPRINTF(" -- myLength after surgery : %ld", myLength);
  idx = 0;

  // Check syntax & build structure for control API.
  status = csValidateSyntax( mySequence, myLength, csCtrlStructure, &stIdx );
  if ( status > 0 ) {
    rc = 5;
    goto END_CS_INTERPRETER;
  }

  while ( idx < stIdx ) {

    TCPRINTF(" -- cmdWord : %s, \t\t argStr : %s", csCtrlStructure[idx].cmdName, csCtrlStructure[idx].argStr );
    //    puts( csCtrlStructure[idx].cmdName );
    //   putchar(',');
    //    puts( csCtrlStructure[idx].argStr );
    //    putchar('\n');
    idx++;
  }

  status = csStringToStructure( csCtrlStructure, stIdx );
  if (status > 0) {
    rc = 6;
    goto END_CS_INTERPRETER;
  }
  status = csDeleteElapsedStep( csCtrlStructure, &stIdx );
  if (status > 0) {
    rc = 6;
    goto END_CS_INTERPRETER;
  }

  status = csExpandLoop( csCtrlStructure, MaxCtrlStep, &stIdx);
  if (status > 0) {
    rc = 7;
    goto END_CS_INTERPRETER;
  }
  returnCode = csConfigureJumpTable( csCtrlStructure);

  status = csConfigureResume( csCtrlStructure, stIdx, &restartIdx, &restartTime);
  if (status > 0) {
    rc = 8;
    goto END_CS_INTERPRETER;
  }
  status = csSetInitialTemperature( csCtrlStructure);
  if (status > 0) {
    rc = 10;
    goto END_CS_INTERPRETER;
  }

  TCPRINTF(" +++ End of processing sequence file. Next, Execute control.");

  status = csExecuteChamberControl( csCtrlStructure, stIdx , restartIdx , restartTime);
  if (status > 0) {
    rc = 9;
    goto END_CS_INTERPRETER;
  }


  // Chamber control thread is End
  TCPRINTF(" +++ End of csThread program.");

END_CS_INTERPRETER:
  TCPRINTF("Exit:%d", rc);

  return rc;
}

/*****************************************************************************/
int csAllocateMyArea(const char *sequenceFile, long sequenceLength,
                     char *mySequence, long *myLength )
/*****************************************************************************/
{
  int  rc = 0;
  long readIdx = 0;

  long eofFlag = 0;

  TCPRINTF("Entry: %p, %ld, %p, %ld", sequenceFile, sequenceLength, mySequence, *myLength);

  /* checking for EOF at last character. only check of end of file.
   * if EOF exist in mid of sequence file, return error code this function.
   */

  TCPRINTF("++ End character of sequence file : %c / %ld", sequenceFile[sequenceLength - 1], sequenceLength);

  if ( sequenceFile[sequenceLength - 1] == 0x1A ) {
    // exist EOF. delete character or decrease length of source data.(-1)
    TCPRINTF("++ EOF exist in end of sequence file. \n" );

    // decrease length.  * bad idea *
    /*    sequenceLength--;
    TCPRINTF("++ sequence length : %ld / %c", sequenceLength, sequenceFile[sequenceLength]);
    */

    // alternate idea: replace null from EOF(0x1A)
    //    sequenceFile[sequenceLength - 1] = NULL;

    // eof flag on
    eofFlag = 1;

  } else {
    // nothing EOF. no action.
    TCPRINTF("++ not found EOF in sequence file. \n" );
  }

  for ( readIdx = 0; readIdx < ( sequenceLength - eofFlag ); readIdx++ ) {

    mySequence[readIdx] = sequenceFile[readIdx];

    if ( !isprint( mySequence[readIdx]) && !isspace( mySequence[readIdx]) ) {
      TCPRINTF(" *** Read file error!! : found unexpected character. please, check a reading file.");
      TCPRINTF(" *** offset %ld 0x%02x", readIdx, mySequence[readIdx]);
      rc = 1;
      goto END_CS_ALLOCATE_MY_AREA;
    }
  }

  *myLength = readIdx;

END_CS_ALLOCATE_MY_AREA:
  TCPRINTF("Exit:%d", rc);

  return rc;
}

/*****************************************************************************/
int csCosmeticSurgery(char *mySequence, long *myLength)
/*****************************************************************************/
{
  const long bufLength = MAX_FILE_SIZE;

  int  rc = 0;
  char seqBuffer[bufLength];
  char ch;
  long copyIdx = 0;
  long visibleIdx = 0;

  TCPRINTF("Entry:%p,%ld", mySequence, *myLength);
  //  TCPRINTF("=> myLength in Surgery : %ld\n", *myLength);

  while ( copyIdx < *myLength ) {

    ch = mySequence[copyIdx];

    if ( isgraph( ch ) ) {  // isgraph() arrowed only visible character.
      seqBuffer[visibleIdx] = ch;
      visibleIdx++;
    }
    copyIdx++;

    if ( visibleIdx > copyIdx ) {
      TCPRINTF(" ** File processing error!! : Unexpected accident.");
      rc = 1;
      goto END_CS_SURGERY;
    }
  }

  memset( mySequence, '\0', *myLength );
  *myLength = visibleIdx;

  for ( copyIdx = 0; copyIdx < visibleIdx; copyIdx++ ) {
    mySequence[copyIdx] = seqBuffer[copyIdx];
  }

END_CS_SURGERY:
  TCPRINTF("Exit:%d", rc);

  return 0;
}


/*****************************************************************************/
int csExecuteChamberControl( csCtrlStep *csCtrlStruct, const long structIdx, const long restartIdx , Dword restartTime)
/*****************************************************************************/
{
  execIdx = 0;
  int rc = 0;
  SM_MESSAGE smMessage;
  TCPRINTF("Entry: %p, %ld, %ld", csCtrlStruct, structIdx, restartIdx);

#if defined(UNIT_TEST)
#else
  if (tccb.optionSyncManagerDebug) {
//  strncpy(smMessage.string,string,64);
    smMessage.type   = SM_CS_SD_CSREADY;
    char string[] = "ChamberScript:READY";
    strncpy(smMessage.string, string, sizeof(smMessage.string));
    smMessage.param[0] = restartTime;
    rc = smSendSyncMessage(&smMessage, SM_CS_SD_TIMEOUT);
    if (rc) {
      goto END_CS_EXECUTE_CHAMBER_CONTROL;
    }
  } else {
    tccb.isChamberScriptReadyToStart = 1;
  }
//  tccb.isChamberScriptReadyToStart = 1;
#endif /* UNIT_TEST */

  if ( restartIdx < 0 ) {
    TCPRINTF(" ** Arguments error!! : invalid parameter restartIdx >> %ld", restartIdx);
    rc = 5;
    goto END_CS_EXECUTE_CHAMBER_CONTROL;
  }

  if ( restartIdx > structIdx ) {
    TCPRINTF(" ** Arguments error!! : invalid parameter structIdx >> %ld, restartIdx >> %ld", structIdx, restartIdx);
    rc = 6;
    goto END_CS_EXECUTE_CHAMBER_CONTROL;
  }

  csInitializeTimer();

  for ( execIdx = restartIdx; execIdx < structIdx; execIdx++ ) {

    rc = csCtrlStruct[execIdx].function( csCtrlStruct[execIdx].arg1,
                                         csCtrlStruct[execIdx].arg2,
                                         csCtrlStruct[execIdx].arg3,
                                         csCtrlStruct[execIdx].string,
                                         csCtrlStruct[execIdx].length  );

    if (rc > 0) {
      TCPRINTF(" ** Control error!! : %s function cause problem.", csCtrlStruct[execIdx].cmdName);
      rc = 1;
      goto END_CS_EXECUTE_CHAMBER_CONTROL;
    }

  }

END_CS_EXECUTE_CHAMBER_CONTROL:
  TCPRINTF("Exit:%d", rc);
  return rc;
}

/****************************************************************/
Byte csDeleteResumeIdx(void)
/****************************************************************/
{
  int i;
  Byte rc = 0;
  int  ret = 0;
  FILE *fp;
  char saveFileName[30] = "";
  TCPRINTF("ENTRY:");
  for (i = 0;i < tccb.optionNumOfTestScriptThread;i++) {
    ret = snprintf(saveFileName, sizeof(saveFileName), "/var/lib/hgst/cs%03d.saveindex", (int)tccb.ts[i].wHGSTCellNo);
    if (ret != 29) {
      rc = 1;
      goto END_CS_DELETE_RESUME_IDX;
    }
    const char *cSaveFileName = saveFileName;
    if ((fp = fopen(cSaveFileName, "w")) == NULL) {
      rc = 1;
      TCPRINTF("!!!!!!!!!!!!!!!!!index file delete error %s", cSaveFileName);
      goto END_CS_DELETE_RESUME_IDX;
    }
    fclose(fp);
  }
END_CS_DELETE_RESUME_IDX:
  TCPRINTF("Exit:%d", rc);
  return rc;
}

/****************************************************************/
Byte csWriteResumeIdx( Dword minute, Word csindex, Byte tempResumeMode)
/****************************************************************/
{
  int i;
  Byte rc = 0;
  int  ret = 0;
  FILE *fp;
  char saveFileName[30] = "";
  char saveString[32] = "";
  TCPRINTF("ENTRY:");
  if ((tempResumeMode != 0) && (tempResumeMode != 1)) {
    rc = 1;
    TCPRINTF("!!!!!!!!!!Value of tempResumeMode is over the limit !!!!!!!%d", tempResumeMode);
    goto END_CS_WRITE_RESUME_IDX;
  }


  ret = snprintf(saveString, sizeof(saveString), "%08d,%08d,%1d,", (int)minute, (int)csindex, (int)tempResumeMode);
  unsigned long crcByCalcuration = ts_crc32(saveString, sizeof(saveString) - 12);
  ret = snprintf(saveString, sizeof(saveString), "%08d,%08d,%1d,0x%08lx\n", (int)minute, (int)csindex, (int)tempResumeMode, crcByCalcuration);
  TCPRINTF("savestring:%s", saveString);
  if (ret == EOF) {
    rc = 1;
    goto END_CS_WRITE_RESUME_IDX;
  }
  for (i = 0;i < tccb.optionNumOfTestScriptThread;i++) {
    ret = snprintf(saveFileName, sizeof(saveFileName), "/var/lib/hgst/cs%03d.saveindex", (int)tccb.ts[i].wHGSTCellNo);
    if (ret != 29) {
      rc = 1;
      goto END_CS_WRITE_RESUME_IDX;
    }
    if ((fp = fopen((const char *)saveFileName, "a")) == NULL) {
      rc = 1;
      TCPRINTF("!!!!!!!!!!!!!!!!!index file open error %s", saveFileName);
      goto END_CS_WRITE_RESUME_IDX;
    }
    ret = fputs(saveString, fp);
    if (ret == EOF) {
      TCPRINTF("!!!!!!!!!!!!!!!!!index file write error %s", saveFileName);
      rc = 1;
      goto END_CS_WRITE_RESUME_IDX;
    }
    fclose(fp);
  }

END_CS_WRITE_RESUME_IDX:
  TCPRINTF("Exit:%d", rc);
  return rc;
}

/****************************************************************/
Byte csReadResumeIdx( Dword *minute, Word *csIndex, Byte *tempResumeMode)
/****************************************************************/
{
  int i, j;
  Byte rc = 0;
  int  ret = 0;
  FILE *fp;
  char saveFileName[30] = "";
  char saveString[32] = "";
  char readCrc[11];
  char readData[9];
  TCPRINTF("ENTRY:");
  *minute = 0;
  *csIndex  = 0;
  i = 0;
  ret = snprintf(saveFileName, sizeof(saveFileName), "/var/lib/hgst/cs%03d.saveindex", (int)tccb.ts[i].wHGSTCellNo);
  if (ret != 29 ) {
    rc = 1;
    goto END_CS_READ_RESUME_IDX;
  }
  if ((fp = fopen((const char *)saveFileName, "rb")) == NULL) {
    rc = 1;
    TCPRINTF("!!!!!!!!!!!!!!!!!index file open error %s", saveFileName);
    goto END_CS_READ_RESUME_IDX;
  }
  if (fseek(fp, -(sizeof(saveString) - 1), SEEK_END) != 0) {
    rc = 1;
    TCPRINTF("!!!!!!!!!!!!!!!!!index file open error %s", saveFileName);
    goto END_CS_READ_RESUME_IDX;
  }
  if ((sizeof(saveString) - 1) != fread(saveString, 1, sizeof(saveString) - 1, fp)) {
    ;
    rc = 1;
    TCPRINTF("!!!!!!!!!!!!!!!!!index file open error %s", saveFileName);
    goto END_CS_READ_RESUME_IDX;
  }
  unsigned long crcByCalcuration = ts_crc32(saveString, sizeof(saveString) - 12);
  char crcByCalcurationString[11];
  ret = snprintf(crcByCalcurationString, sizeof(crcByCalcurationString), "0x%08lx", crcByCalcuration);
  if (ret != 10 ) {
    rc = 1;
    TCPRINTF("!!!!!!!!!!!!!!!!!CRC calcuration error");
    goto END_CS_READ_RESUME_IDX;
  }
  strncpy(readCrc, saveString + 20, sizeof(readCrc));
  readCrc[10] = '\0';
  if (strcmp(crcByCalcurationString, readCrc) != MATCH) {
    rc = 1;
    for (j = 0;j < 11;j++) {
      TCPRINTF("%d,%d", crcByCalcurationString[j], readCrc[j]);
    }
    TCPRINTF("!!!!!!!!!!!!!!!!!CRC error %s - %s", crcByCalcurationString, readCrc);
    goto END_CS_READ_RESUME_IDX;
  }
  strncpy(readData, saveString, sizeof(readData));
  for (j = 0;j < 8;j++) {
    if (isdigit(readData[j])) {
    } else {
      rc = 1;
      TCPRINTF("!!!!!!!!!!!!!!!!! minute read error !!!!!!!!!!!!!!!");
      goto END_CS_READ_RESUME_IDX;
    }
  }
  *minute = (Dword)atoi(readData);
  if (*minute > 60*24*365) {
    rc = 1;
    TCPRINTF("!!!!!!!!!!!!!!!!! read minute is too big error!!!!!!!!!!!!!!!");
    goto END_CS_READ_RESUME_IDX;
  }
  strncpy(readData, saveString + 9, sizeof(readData));
  for (j = 0;j < 8;j++) {
    if (isdigit(readData[j])) {
    } else {
      rc = 1;
      TCPRINTF("!!!!!!!!!!!!!!!!! Index read error !!!!!!!!!!!!!!!");
      goto END_CS_READ_RESUME_IDX;
    }
  }
  *csIndex = (Word)atoi(readData);
  if (*csIndex > MAX_CTRL_STEP) {
    rc = 1;
    TCPRINTF("!!!!!!!!!!!!!!!!! read index is too big error!!!!!!!!!!!!!!!");
    goto END_CS_READ_RESUME_IDX;
  }
  readData[0] = *(saveString + 18);
  if (isdigit(readData[0])) {
    if (readData[0] == '0') {
      *tempResumeMode = 0;
    } else if (readData[0] == '1') {
      *tempResumeMode = 1;
    } else {
      rc = 1;
      TCPRINTF("!!!!!!!!!!Value of tempResumeMode is over the limit !!!!!!!%s", readData);
      goto END_CS_READ_RESUME_IDX;
    }
  }

// }

END_CS_READ_RESUME_IDX:
  TCPRINTF("Exit:%d", rc);
  return rc;

}

/*************************************************************************************/
Byte csConfigureJumpTable( csCtrlStep *csCtrlStruct)
/*************************************************************************************/
{
  Dword totalMinute = 0;
  Word  ctrlStepIndex = 0;
  Word  jumpTableIndex = 0;
  Word  labelTableIndex = 0;
  TCPRINTF("ENTRY:");
  const long MaxCtrlStep = MAX_CTRL_STEP;
  for (ctrlStepIndex = 0;ctrlStepIndex < MaxCtrlStep;ctrlStepIndex++) {
    if (strcmp(csCtrlStruct[ctrlStepIndex].cmdName, csNameHoldMin) == MATCH) {
      totalMinute = totalMinute + (Dword)csCtrlStruct[ctrlStepIndex].arg1;
    } else if (strcmp(csCtrlStruct[ctrlStepIndex].cmdName, csNameResume) == MATCH) {
      jumpTableIndex--;
    } else if (strcmp(csCtrlStruct[ctrlStepIndex].cmdName, csNameLabel) == MATCH) {
      if (csCtrlStruct[ctrlStepIndex].argStr[0] == '#') {
        strncpy(csJumpTable[labelTableIndex].string, csCtrlStruct[ctrlStepIndex].argStr + 1, sizeof(csCtrlStruct[ctrlStepIndex].argStr) - 1);
      } else {
        strncpy(csJumpTable[labelTableIndex].string, csCtrlStruct[ctrlStepIndex].argStr, sizeof(csCtrlStruct[ctrlStepIndex].argStr));
      }
      csJumpTable[labelTableIndex].index = jumpTableIndex;
      csJumpTable[labelTableIndex].minute = totalMinute;
      labelTableIndex++;
    }
    jumpTableIndex++;
  }
  TCPRINTF("Exit:");
  return 0;
}
/*************************************************************************************/
Byte csDeleteElapsedStep( csCtrlStep *csCtrlStruct, long *stIdx)
/*************************************************************************************/
{
  Word  inputStepIndex = 0;
  Word  outputStepIndex = 0;
  Byte  rc = 0;
  long  totalIndex = *stIdx;
  TCPRINTF("ENTRY:");
  if (csCtrlStruct == NULL) {
    rc = 1;
    TCPRINTF("!!!!!!!!!!!!!!!!! control struct is NULL !!!!!!!!!!!!!!!");
    goto END_CS_DELETE_ELAPSED_STEP;
  }
  if (*stIdx > MAX_CTRL_STEP) {
    rc = 1;
    TCPRINTF("!!!!!!!!!!!!!!!!! index is too big  !!!!!!!!!!!!!!!");
    goto END_CS_DELETE_ELAPSED_STEP;
  }
  resumeFlg = 0;
  csITemp.initialTemperatureFlag = 0;
  csITemp.targetTemperature = 25;
  csITemp.minimumTemperature = 15;
  csITemp.maximumTemperature = 35;
  for (inputStepIndex = 0;inputStepIndex < *stIdx;inputStepIndex++) {
    if (strcmp(csCtrlStruct[inputStepIndex].cmdName, csNameResume) == MATCH) {
      totalIndex--;
      if ((csCtrlStruct[inputStepIndex].arg1 != 0)
          || (csCtrlStruct[inputStepIndex].arg2 != 0)
          || (csCtrlStruct[inputStepIndex].arg3 != 0)) {
        resumeFlg = 1;
      }
    } else {
      if (strcmp(csCtrlStruct[inputStepIndex].cmdName, csNameInitialTemperatureSet) == MATCH) {
        csITemp.initialTemperatureFlag = 1;
        csITemp.targetTemperature = csCtrlStruct[inputStepIndex].arg1;
        csITemp.minimumTemperature = csCtrlStruct[inputStepIndex].arg2;
        csITemp.maximumTemperature = csCtrlStruct[inputStepIndex].arg3;
      }
      csCtrlStruct[outputStepIndex] = csCtrlStruct[inputStepIndex];
      outputStepIndex++;
    }
  }
  *stIdx = totalIndex;

END_CS_DELETE_ELAPSED_STEP:
  TCPRINTF("Exit:%d", rc);
  return rc;
}
/**
 * <pre>
 * Description:
 *  Display back trace by using tcPrintf() and check recursive call
 * Arguments:
 *   recursiveCheckFunc - INPUT - function name to be checked recursive call
 * Return:
 *   0: no recursive call. 1:recursive call 2:back trace is empty
 * Note:
 * </pre>
 *****************************************************************************/
/*************************************************************************************/
Byte csSetInitialTemperature( csCtrlStep *csCtrlStruct)
/*************************************************************************************/
{
  TCPRINTF("ENTRY:");
  Word wTempInHundredth = 0;
  Byte rc = 0;
  TCPRINTF("resumeFlg= %d,setInitialTemperatureFlg= %d", resumeFlg, csITemp.initialTemperatureFlag);
  if ( csITemp.targetTemperature < 10 ) {
    TCPRINTF(" ** Arguments error!! : invalid parameter target temperature< 10 degreeC %d", csITemp.targetTemperature);
    rc = 1;
    goto END_CS_SET_INITIAL_TEMPERATURE;
  }
  if ( csITemp.targetTemperature > 70 ) {
    TCPRINTF(" ** Arguments error!! : invalid parameter target temperature> 70 degreeC %d", csITemp.targetTemperature);
    rc = 1;
    goto END_CS_SET_INITIAL_TEMPERATURE;
  }
  if ( csITemp.targetTemperature > csITemp.maximumTemperature ) {
    TCPRINTF(" ** Arguments error!! : invalid parameter target temperature> maximum temperature %d", csITemp.maximumTemperature);
    rc = 1;
    goto END_CS_SET_INITIAL_TEMPERATURE;
  }
  if ( csITemp.targetTemperature < csITemp.minimumTemperature ) {
    TCPRINTF(" ** Arguments error!! : invalid parameter target temperature< minimum temperature %d", csITemp.minimumTemperature);
    rc = 1;
    goto END_CS_SET_INITIAL_TEMPERATURE;
  }
  if ( csITemp.maximumTemperature == csITemp.minimumTemperature ) {
    TCPRINTF(" ** Arguments error!! : invalid parameter maximum temperature == minimum temperature %d,%d", csITemp.maximumTemperature, csITemp.minimumTemperature);
    rc = 1;
    goto END_CS_SET_INITIAL_TEMPERATURE;
  }
  if ((resumeFlg == 0) && (csITemp.initialTemperatureFlag == 1)) {
    TCPRINTF("ramp rate change is not supported right now");
    rc = setTargetTemperature(tccb.ts[0].wHGSTCellNo + 1, csITemp.targetTemperature * 100);
    if (rc) {
      TCPRINTF("error");
      exit(EXIT_FAILURE);
    }
    for (;;) {
      rc = getCurrentTemperature(tccb.ts[0].wHGSTCellNo + 1, &wTempInHundredth);
      if (rc) {
        TCPRINTF("error");
        exit(EXIT_FAILURE);
      }
      TCPRINTF("current temperature %d", wTempInHundredth);
      if ((wTempInHundredth >= csITemp.minimumTemperature * 100)
          && (wTempInHundredth <= csITemp.maximumTemperature * 100)) {
        TCPRINTF("current temperature is reached target temperature");
        break;
      }
      // Mutex Unlock
      rc = pthread_mutex_unlock(&mutex_slotio);
      if (rc) {
        TCPRINTF("error");
        tcExit(EXIT_FAILURE);
      }

      sleep(TEMP_CHECK_DURATION);

      // Mutex Lock
      rc = pthread_mutex_lock(&mutex_slotio);
      if (rc) {
        TCPRINTF("error");
        tcExit(EXIT_FAILURE);
      }
    }
  }

END_CS_SET_INITIAL_TEMPERATURE:

  TCPRINTF("Exit:");
  return rc;
}

