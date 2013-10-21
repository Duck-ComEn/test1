#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>  // for sleep

#if defined(UNIT_TEST)

#include "csWrapper.h"

#else /* UNIT_TEST */

#include "../testcase/tcTypes.h"
#include "../slotio/libsi.h"
#include "../testscript/libts.h"
#include "../syncmanager/libsm.h"
#include "libcs.h"

#endif /* UNIT_TEST */


#define MAX_FILE_SIZE  ( 64 * 1024 ) // 64 KBytes
#define MAX_CTRL_STEP  ( 16 * 1024 ) // 16 k Steps

enum {
  TEMP_CHECK_DURATION = 30,
};

/*  if you need add function, when you must add calling API function(csCallAPI.c)
 *  and modification csStringToStructure(csSyntaxProcess.c).
 */

/* Definition sequence structure */

struct csStep {

  int (* function)(long arg1, long arg2, long arg3, char *string, int length);
  long arg1;
  long arg2;
  long arg3;
  char string[64];
  long length;
  char cmdName[64];
  char argStr[64];

};

typedef struct csStep csCtrlStep;

/* Definition label structure for jump */

struct csJumpLabel {
  Byte   string[64];
  Word  index;
  Dword  minute;
};

typedef struct csITemperature {
  Byte  initialTemperatureFlag;
  Word  targetTemperature;
  Word  minimumTemperature;
  Word  maximumTemperature;
} csInitialTemperature;

struct csJumpLabel csJumpTable[MAX_CTRL_STEP];

/* Prototype declaration */

int csInterpreter( const char *sequenceFile, long sequenceLength );

int csAllocateMyArea(const char *sequenceFile, long sequenceLength,
                     char *mySequence, long *myLength);

int csCosmeticSurgery(char *mySequence, long *myLength);

int csValidateSyntax(const char *mySequence, long myLength, csCtrlStep *csCtrlStruct, long *stIdx);

int csStringToStructure(csCtrlStep *csCtrlStruct, long stIdx);

int csSplitArg(csCtrlStep *csCtrlStruct, const int argNum);

int csExpandLoop(csCtrlStep *csCtrlStruct, const long maxStructCell, long *stIdx);

int csConfigureResume( csCtrlStep *csCtrlStruct, const long stIdx, long *restartIdx, Dword *restartTime );

Byte csConfigureJumpTable( csCtrlStep *csCtrlStruct);
int csExecuteChamberControl( csCtrlStep *csCtrlStruct, const long structIdx, const long restartIdx , Dword restartTime);
//int csExecuteChamberControl(csCtrlStep *csCtrlStruct, const long structIdx);

int csInitializeTimer(void);
int csCallSequence(long arg1, long arg2, long arg3, char *string, int length);
int csCallEndSequence(long arg1, long arg2, long arg3, char *string, int length);
int csCallSaveInterval(long arg1, long arg2, long arg3, char *string, int length);
int csCallRestart(long arg1, long arg2, long arg3, char *string, int length);
int csCallSetVolt(long arg1, long arg2, long arg3, char *string, int length);
int csCallSetTemp(long arg1, long arg2, long arg3, char *string, int length);
int csCallHoldMin(long arg1, long arg2, long arg3, char *string, int length);
int csCallCommand(long arg1, long arg2, long arg3, char *string, int length);
int csCallResumeCommand(long arg1, long arg2, long arg3, char *string, int length);
int csCallSave(long arg1, long arg2, long arg3, char *string, int length);
int csCallLoop(long arg1, long arg2, long arg3, char *string, int length);
int csCallVersion(long arg1, long arg2, long arg3, char *string, int length);
int csCallSumValue(long arg1, long arg2, long arg3, char *string, int length);
int csCallLabel(long arg1, long arg2, long arg3, char *string, int length);
int csCallResumeAsDummy(long arg1, long arg2, long arg3, char *string, int length);
int csCallInitialTemperatureSet(long arg1, long arg2, long arg3, char *string, int length);
Byte csJump(char *label);
Byte csDeleteElapsedStep( csCtrlStep *csCtrlStruct, long *stIdx);
Byte csInquiryLabel(char *label);
Byte csSetInitialTemperature( csCtrlStep *csCtrlStruct);

int csWaitUntilTemperatureOnTarget(void);

int csUnitTest(void);

int csUnitTest_chkZeroLoop(void);
int csUnitTest_noLoop(void);

int csUnitTest_resumeOK1(void);
int csUnitTest_resumeOK2(void);
int csUnitTest_resumeOK3(void);
int csUnitTest_resumeZeroOK(void);

int csUnitTest_chkResume1(void);
int csUnitTest_chkResume2(void);
int csUnitTest_chkResume3(void);
int csUnitTest_chkResume4(void);
int csUnitTest_chkResume5(void);

int csUnitTest_chkHoldMin1(void);
int csUnitTest_chkHoldMin2(void);
int csUnitTest_chkHoldMin3(void);
int csUnitTest_chkHoldMin4(void);
int csUnitTest_chkHoldMin5(void);

int csUnitTest_chkLoopTest1(void);
int csUnitTest_chkLoopTest2(void);
int csUnitTest_chkLoopTest3(void);
int csUnitTest_chkLoopTest4(void);

int csUnitTest_erResume1(void);
int csUnitTest_erResume2(void);
int csUnitTest_erResume3(void);

int csUnitTest_erFunction1(void);
int csUnitTest_erFunction2(void);
int csUnitTest_erFunction3(void);
int csUnitTest_erFunction4(void);
int csUnitTest_erFunction5(void);
int csUnitTest_erSeparator1(void);
int csUnitTest_erSeparator2(void);
int csUnitTest_erSeparator3(void);
int csUnitTest_erSeparator4(void);
int csUnitTest_erSeparator5(void);
int csUnitTest_erSeparator6(void);
int csUnitTest_erCamma1(void);
int csUnitTest_erCamma2(void);
int csUnitTest_erCamma3(void);
int csUnitTest_erBinFile(void);
int csUnitTest_erMissAddress(void);
int csUnitTest_erHugeInputFile(void);
int csUnitTest_erZeroLengthFile(void);

int csUnitTest_EOF1(void);
int csUnitTest_EOF2(void);
int csUnitTest_EOF3(void);
int csUnitTest_erEOF1(void);
int csUnitTest_erEOF2(void);

int csUnitTest_erParam1(void);
int csUnitTest_erParam2(void);
int csUnitTest_erParam3(void);
int csUnitTest_erParam4(void);
int csUnitTest_erParam5(void);
int csUnitTest_erParam6(void);
int csUnitTest_erParam7(void);

//int csEXEC( const char *sequenceFile, long sequenceLength );
