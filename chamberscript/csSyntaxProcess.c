#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "cs.h"

#define MATCH                          (0)     // makes use of strncmp() more readable

/************  Function name list **************/

const char csNameSetTemp[] = "setTemp";
const char csNameSetVolt[] = "setVolt";
const char csNameHoldMin[] = "holdMin";
const char csNameCommand[] = "command";
const char csNameResumeCommand[] = "resumeCommand";
const char csNameSaveInterval[] = "saveInterval";
const char csNameRestart[] = "restart";
const char csNameSequence[] = "sequence";
const char csNameEndSequence[] = "endSequence";
const char csNameLoop[] = "loop";
const char csNameSave[] = "save";
const char csNameVersion[] = "version";
const char csNameSumValue[] = "sumValue";
const char csNameLabel[] = "label";
//const char csNameResume[] = "resume";
const char csNameResume[] = "elapsed";
const char csNameInitialTemperatureSet[] = "initializeTemperatureBeforeTestStart";

const char *functionList[] = {
  csNameSetTemp,      // 0
  csNameSetVolt,      // 1
  csNameHoldMin,      // 2
  csNameCommand,      // 3
  csNameResumeCommand,// 4
  csNameSaveInterval, // 5
  csNameRestart,      // 6
  csNameSequence,     // 7
  csNameEndSequence,  // 8
  csNameLoop,         // 9
  csNameSave,         // 10
  csNameVersion,      // 11
  csNameSumValue,     // 12
  csNameLabel,        // 13
  csNameResume,       // 14
  csNameInitialTemperatureSet,       // 15
};

const int csMaxFunctionNumber = 16;
extern double dwChamberScriptJumpedTime;
extern Byte resumeFlg;
extern Byte setInitialTemperatureFlg;
/*************************************************/

int csLoopFlg = 0;

/*****************************************************************************/
int csValidateSyntax(const char *mySequence, long myLength, csCtrlStep *csCtrlStruct, long *stIdx)
/*****************************************************************************/
{
  int  rc = 0;

  long checkIdx = 0;
  long stepIdx = 0;

  long cmdHead = 0;
  long cmdTail = 0;
  long cmdSize = 0;
  long argHead = 0;
  long argTail = 0;
  long argSize = 0;

  char chkChar;
  char *errStatusPt;
  char errString[128] = {0};
  //  long errLength = 0;

  TCPRINTF("Entry:%p,%ld, %p, %ld", mySequence, myLength, csCtrlStruct, *stIdx );

  while ( 1 /* sequence[idx] < lastChNum */ ) {

    /* * * * exit for infinity loop * * * * * * * */
    if ( checkIdx > myLength - 1 ) {
      break;
    }
    /* * * * * * * * * * * * * * * * * * * * * * */
    while ( mySequence[checkIdx] != '(' ) { /* ++++++++++++++++++++++++++++++++++++++++++++++++++++ */

      errStatusPt = strncpy( &chkChar, &mySequence[checkIdx], 1 );
      //      TCPRINTF(" > check character :%c \n", chkChar);
      if ( !isalpha( chkChar ) ) {
        /* errLength = checkIdx + 1 - (argTail + 3);
        if( errLength < 1 ){
          errLength = checkIdx - cmdHead;
        }
        */
        // errStatusPt = strncpy( errString, &mySequence[checkIdx], errLength ); // after semi-colon to now pointer + 1.
        TCPRINTF(" ** Syntax error! : found unexpected character \"%c\" [sorry, nothing statement.]", chkChar );
        //in command name area > %s / %ld \n", chkChar, errString, errLength);
        rc = 1;
        goto END_CS_VALIDATE_SYNTAX;
      }
      if ( checkIdx == myLength ) {
        TCPRINTF(" ** Syntax error! : not found \"(\" symbol.");
        rc = 2;  // => return and error code. ok 0 , error over 1. Don't minus.
        goto END_CS_VALIDATE_SYNTAX;
      }
      checkIdx++;

    }/* ++++++++++++++++++++++++++++++++++++++++++++++++++++ end of loop for finding "(" symbol. */

    cmdTail = checkIdx;
    cmdSize = cmdTail - cmdHead;

    checkIdx++;
    argHead = checkIdx;

    if ( mySequence[argHead] == ',' ) { /* * * Is camma at argument top ? * * */
      errStatusPt = strncpy( errString, &mySequence[cmdHead], (checkIdx - cmdHead + 1) );
      TCPRINTF(" ** Syntax error! : found \",\" at argument top symbol. > %s", errString);
      rc = 3;
      goto END_CS_VALIDATE_SYNTAX;
    }

    while ( mySequence[checkIdx] != ')' ) { /* =================================================== */

      checkIdx++;
      if ( checkIdx == myLength ) {
        TCPRINTF(" ** Syntax error! : nothing \")\" !!");
        rc = 4;
        goto END_CS_VALIDATE_SYNTAX;
      }

      errStatusPt = strncpy( &chkChar, &mySequence[checkIdx], 1 );

      if ( ispunct(chkChar) && chkChar != ')' ) {

        if ( mySequence[checkIdx] == ',' ) { /* * * Is 2camma in arguments line ? * * */
          if ( mySequence[ (checkIdx + 1) ] == ',' ) {
            errStatusPt = strncpy( errString, &mySequence[argHead], (checkIdx - argHead + 2) );
            TCPRINTF(" ** Syntax error! : found two camma. \",,\" > %s", errString);
            rc = 5;
            goto END_CS_VALIDATE_SYNTAX;

          } else if ( mySequence[ (checkIdx + 1) ] == ')' ) { /* * * Is camma at argument end ? * * */
            errStatusPt = strncpy( errString, &mySequence[argHead], (checkIdx - argHead + 2) );
            TCPRINTF(" ** Syntax error! : found \",\" at argument end symbol. > %s", errString);
            rc = 6;
            goto END_CS_VALIDATE_SYNTAX;

          }
        } else { // in case, ispunct() found other than ',' symbol.

          if ( checkIdx == argHead && chkChar == '#' ) { // first symbol of argument string. for passing EFTS command statement.
            TCPRINTF(" -- found \"#\" symbol. first symbol in argument statement");
          } else {
            errStatusPt = strncpy( errString, &mySequence[argHead], (checkIdx - argHead + 2) );
            TCPRINTF(" ** Syntax error! : found \"%c\" symbol in this argument statement. > %s", chkChar, errString);
            rc = 7;
            goto END_CS_VALIDATE_SYNTAX;
          }
        }

      }/* endif - !ispunct(chkChar) */

    }/* ==================================================== end of loop for finding ")" symbol. */

    argTail = checkIdx;
    argSize = argTail - argHead;

    if ( argSize > 30 ) {
      errStatusPt = strncpy( errString, &mySequence[cmdHead], (checkIdx - cmdHead + 1) );
      TCPRINTF(" ** Syntax error! : Argument length is over! please check this statement > %s", errString);
      rc = 8;
      goto END_CS_VALIDATE_SYNTAX;
    }
    csCtrlStruct[checkIdx].length = argSize;

    // throw in to structure buffer area.
    strncpy( csCtrlStruct[stepIdx].cmdName, &mySequence[cmdHead], cmdSize );
    if ( argSize == 0 ) {
      strcpy( csCtrlStruct[stepIdx].argStr, "nothing" );
    } else {
      strncpy( csCtrlStruct[stepIdx].argStr,  &mySequence[argHead], argSize );
    }

    checkIdx++;
    strncpy( &chkChar, &mySequence[checkIdx], 1 );
    if ( chkChar != ';' ) {
      errStatusPt = strncpy( errString, &mySequence[argHead], (checkIdx - argHead + 13) );
      //      TCPRINTF(" ** Syntax error! : found \"%c\" character in stead of \";\". Check this statement > %s", chkChar, errString);
      TCPRINTF(" ** Syntax error! : found \"%c\" character in stead of \";\". Check this statement > %s", chkChar, &mySequence[checkIdx] );
      rc = 5;
      goto END_CS_VALIDATE_SYNTAX;
    }
    checkIdx++;
    cmdHead = checkIdx;

    stepIdx++;
  } // end of infinity while loop.

  *stIdx = stepIdx;
  if ( stepIdx > MAX_CTRL_STEP ) {
    TCPRINTF(" ** Syntax error!! : step number is too over\n");
    rc = 1;
    goto END_CS_VALIDATE_SYNTAX;
  }

END_CS_VALIDATE_SYNTAX:
  TCPRINTF("Exit:%d", rc);

  return rc;
}

/*****************************************************************************/
int csStringToStructure(csCtrlStep *csCtrlStruct, long stIdx)
/*****************************************************************************/
{
  int  rc = 0;
  int  argNum = 0;
  int  status = 0;
  int  loopFlg = 0;
  int  loopStartStep = 0;
  int  loopEndStep = 0;
  int  fncIdx = 0;
  long cmpIdx = 0;

  TCPRINTF("Entry: %p, %ld", csCtrlStruct, stIdx );

  csLoopFlg = 0; //format.

  for (cmpIdx = 0; cmpIdx < stIdx; cmpIdx++) {

    while ( 1 ) { // compare function list.
      // TCPRINTF("st: %s, func: %s \n", csCtrlStruct[cmpIdx].cmdName, functionList[fncIdx]); fflush(stdout);
      status = strcmp( csCtrlStruct[cmpIdx].cmdName, functionList[fncIdx] );
      if ( MATCH == status ) {
        TCPRINTF(" -- idx:%ld / match %s & %s", cmpIdx, csCtrlStruct[cmpIdx].cmdName, functionList[fncIdx] ); // fflush(stdout);
        break; // when match function go out of loop.
      }

      fncIdx++;
      if ( fncIdx == csMaxFunctionNumber ) {
        TCPRINTF(" ** Syntax error!! : No match function! Undefined statement: %s", csCtrlStruct[cmpIdx].cmdName);
        rc = 1;
        goto END_CS_STRING_TO_STRUCTURE;
      }

    } /* > > > > > End of while loop < < < < < */

    // below things doing when match function.

    /* ----------------------- *\
     *  setTemp,       // 0    *
     *  setVolt,       // 1    *
     *  holdMin,       // 2    *
     *  Command,       // 3    *
     *  ResumeCommand, // 4    *
     *  SaveInterval,  // 5    *
     *  Restart,       // 6    *
     *  Sequence,      // 7    *
     *  EndSequence,   // 8    *
     *  Loop,          // 9    *
     *  Save,          // 10   *
     *  Version,       // 11   *
     *  SumValue,      // 12   *
     *  Label,         // 13   *
    \* ----------------------- */

    switch (fncIdx) {

    case 0: // setTemp with 2args

      argNum = 2;
      csCtrlStruct[cmpIdx].function = csCallSetTemp;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 1: // setVolt with 2args

      argNum = 2;
      csCtrlStruct[cmpIdx].function = csCallSetVolt;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 2: // holdMin

      argNum = 1;
      csCtrlStruct[cmpIdx].function = csCallHoldMin;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 3: // command has string arg

      csCtrlStruct[cmpIdx].function = csCallCommand;
      strcpy( csCtrlStruct[cmpIdx].string, csCtrlStruct[cmpIdx].argStr );
      status = 0;

      break;
    case 4: // resumeCommand has string arg

      csCtrlStruct[cmpIdx].function = csCallResumeCommand;
      strcpy( csCtrlStruct[cmpIdx].string, csCtrlStruct[cmpIdx].argStr );
      status = 0;

      break;
    case 5: // saveInterval with 1arg

      argNum = 1;
      csCtrlStruct[cmpIdx].function = csCallSaveInterval;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 6: // restart with 3args

      argNum = 3;
      csCtrlStruct[cmpIdx].function = csCallRestart;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 7: // sequence with 1arg

      argNum = 1;
      csCtrlStruct[cmpIdx].function = csCallSequence;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 8: // endSequence has nothing arg

      argNum = 0;
      csCtrlStruct[cmpIdx].function = csCallEndSequence;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 9: // loop has nothing arg

      csCtrlStruct[cmpIdx].function = csCallLoop;

      argNum = 0;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      strcpy(csCtrlStruct[cmpIdx].string, "loopBackPoint");

      loopEndStep = cmpIdx;
      if ( loopFlg == 0 ) {
        TCPRINTF(" ** Syntax error!! : \"loop\" function called without \"save\" function.");
        rc = 9;
        goto END_CS_STRING_TO_STRUCTURE;
      }

      loopFlg = 0;

      break;
    case 10: // save with 1arg

      csCtrlStruct[cmpIdx].function = csCallSave;

      argNum = 1;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      strcpy(csCtrlStruct[cmpIdx].string, "loopStartPoint");

      loopFlg = 1;
      loopStartStep = cmpIdx;

      csLoopFlg++; // global flg in this file. read by csExpandLoop();

      break;
    case 11: // version with 1arg

      argNum = 1;
      csCtrlStruct[cmpIdx].function = csCallVersion;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 12: // sumValue with 1arg (hex)

      argNum = 1;
      csCtrlStruct[cmpIdx].function = csCallSumValue;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 13: // label has string arg

      csCtrlStruct[cmpIdx].function = csCallLabel;
      strcpy( csCtrlStruct[cmpIdx].string, csCtrlStruct[cmpIdx].argStr );
      status = 0;

      break;
    case 14: // resume has 3 arg ( hour, min, sec )

      argNum = 3;
      csCtrlStruct[cmpIdx].function = csCallResumeAsDummy;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    case 15: // initialTemperatureSet has 3 arg ( hour, min, sec )

      argNum = 3;
      csCtrlStruct[cmpIdx].function = csCallInitialTemperatureSet;
      status = csSplitArg( &csCtrlStruct[cmpIdx], argNum );

      break;
    }

    if (status != 0) {
      TCPRINTF(" ** argment error!! function > %s, need args: %d", functionList[fncIdx], argNum );
      TCPRINTF(" -- cmd word: %s / arg: %s / status: %d", csCtrlStruct[cmpIdx].cmdName, csCtrlStruct[cmpIdx].argStr, status);
      rc = status;
      goto END_CS_STRING_TO_STRUCTURE;
    }

    fncIdx = 0;
  }

END_CS_STRING_TO_STRUCTURE:
  TCPRINTF("Exit:%d", rc);

  return rc;
}

/*****************************************************************************/
int csSplitArg(csCtrlStep *csCtrlStruct, int argNum)
/*****************************************************************************/
{
  int  rc = 0;
  int  status = 0;
  int  argLen = 0;
  int  cnvIdx = 0;
  int  splIdx = 0;
  int  hexFlg = 0;
  long cnvArg[3] = { 0, 0, 0 };
  char chkChar = '0';

  char *errStatusPt;
  char errString[128] = {0};

  TCPRINTF("Entry: %p, %d", csCtrlStruct, argNum );

  if ( !strcmp( csCtrlStruct->argStr, "nothing" ) ) {

    if ( argNum == 0 ) {
      strcpy( csCtrlStruct->string, "nothing" );
      rc = 0;
      goto END_CS_SPLIT_ARG;

    } else {
      TCPRINTF(" ** Missing argument!! (arranging error message.) ");
      rc = 1;
      goto END_CS_SPLIT_ARG;

    }
  }

  argLen = strlen( csCtrlStruct->argStr );
  if ( argLen == 0 ) {
    TCPRINTF(" ** Missing argument!! : unexpected error! Argument is nothing.");
    rc = 2;
    goto END_CS_SPLIT_ARG;
  }

  for ( cnvIdx = 0; cnvIdx < argNum; cnvIdx++) { /* <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> */

    if ( 0 == strncmp( &csCtrlStruct->argStr[splIdx], "0X", 2 ) || // or  = = = = = = = = = = = = = = =.
         0 == strncmp( &csCtrlStruct->argStr[splIdx], "0x", 2 ) ) { // match pattern

      splIdx += 2; // increment index 2 character. for avoid top "0X" character from isXdigit();
      hexFlg = 1;
      TCPRINTF(" ++ hexFlg stand!! (splIdx :%d)", splIdx);

      // convert to parameter value from string data in this line. for Hex.
      cnvArg[cnvIdx] = strtol( &csCtrlStruct->argStr[splIdx], NULL, 16 );
      if ( errno == ERANGE ) {
        TCPRINTF(" *** Standard function error!? : convert error. string to parameter.");
        rc = 3;
        goto END_CS_SPLIT_ARG;
      }

    } else {
      TCPRINTF(" -- hexFlg no stand. (splIdx :%d)", splIdx);

      // convert to parameter value from string data in this line. for decimal.
      cnvArg[cnvIdx] = strtol( &csCtrlStruct->argStr[splIdx], NULL, 10 );
      if ( errno == ERANGE ) {
        TCPRINTF(" *** Standard function error!? : convert error. string to parameter.");
        rc = 3;
        goto END_CS_SPLIT_ARG;
      }


    } // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =.

    TCPRINTF(" -- converted arg no.%d : %ld", cnvIdx, cnvArg[cnvIdx]);
    if ( cnvIdx == (argNum - 1) ) { /* last argument conversion. */

      TCPRINTF(" -- check last arg character >>");

      while ( splIdx < argLen ) { /* -------------------------------------------------------------------------- */

        TCPRINTF("%c", csCtrlStruct->argStr[splIdx]);
        errStatusPt = strncpy( &chkChar, &csCtrlStruct->argStr[splIdx], 1 );

        if ( hexFlg == 1 ) {

          status = isxdigit(chkChar);

          if ( status == 0 && (chkChar != ',') ) {
            if ( chkChar != '-' ) {
              errStatusPt = strncpy( errString, csCtrlStruct->argStr, argLen );
              TCPRINTF(" ** Syntax error!! : found unexpected character \"%c\" in argument statement. > %s", chkChar, errString);
              rc = 4;
              goto END_CS_SPLIT_ARG;
            }
          }
        } else {

          status = isdigit(chkChar);

          if ( status == 0 && (chkChar != ',') ) {
            if ( chkChar != '-' ) {
              errStatusPt = strncpy( errString, csCtrlStruct->argStr, argLen );
              TCPRINTF(" ** Syntax error!! : found unexpected character \"%c\" in argument statement. > %s", chkChar, errString);
              rc = 5;
              goto END_CS_SPLIT_ARG;
            }
          }
        }
        splIdx++;

      } /* ----------------------------------------------------------------------- end while. for  */
      TCPRINTF(" -- checked char: %c / status: %d", chkChar, status);
      TCPRINTF(" ++ break. spliting arg loop.");
      break; // go out this "for" loop. to end splitArg function.

    } /* endif for last argument conversion. */

    TCPRINTF(" -- check %d arg character >>", cnvIdx);

    while ( csCtrlStruct->argStr[splIdx] != ',' ) { /* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

      TCPRINTF("%c", csCtrlStruct->argStr[splIdx]);

      errStatusPt = strncpy( &chkChar, &csCtrlStruct->argStr[splIdx], 1 );

      if ( splIdx == argLen ) {
        TCPRINTF(" ** Syntax error!! : nothing argument separater \',\' : %s", csCtrlStruct->argStr );
        rc = 6;
        goto END_CS_SPLIT_ARG;
      }

      if ( hexFlg == 1 ) {

        status = isxdigit( chkChar );

        if ( status == 0 && (chkChar != ',') ) {
          if ( chkChar != '-' ) {
            errStatusPt = strncpy( errString, csCtrlStruct->argStr, argLen );
            TCPRINTF(" ** Syntax error!! : found unexpected character in argument statement. > %s", errString);
            rc = 4;
            goto END_CS_SPLIT_ARG;
          }
        }
      } else {

        status = isdigit( chkChar );
        if ( status == 0 && (chkChar != ',') ) {
          if ( chkChar != '-' ) {
            errStatusPt = strncpy( errString, csCtrlStruct->argStr, argLen );
            TCPRINTF(" ** Syntax error!! : found unexpected character in argument statement. > %s", errString);
            rc = 5;
            goto END_CS_SPLIT_ARG;
          }
        }
      }//end if( hexFlg )
      TCPRINTF(" -- checked char: %c / status: %d", chkChar, status);
      splIdx++;

    } /* +++++++++++++++++++++++++++++++++++++++++++++++++  End of while ( point character is Camma in this line. ) */

    splIdx++; // after increment, this index pointing character is next camma.
    fflush(stdout);
    TCPRINTF(" -- found Camma. go to checking next character.");
    hexFlg = 0;
  } /* <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> End of for loop. for convert string to parameter.*/
  // come to here by break.

  csCtrlStruct->arg1 = cnvArg[0];
  csCtrlStruct->arg2 = cnvArg[1];
  csCtrlStruct->arg3 = cnvArg[2];

  //parameter rangeCheck
  if ( !strcmp(csCtrlStruct->cmdName, csNameSave) ) {

    if ( csCtrlStruct->arg1 < 0 ) {
      TCPRINTF(" ** Argument error!! : parameter is too fewer for save().");
      rc = 9;
      goto END_CS_SPLIT_ARG;
    }
    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  } else if ( !strcmp(csCtrlStruct->cmdName, csNameSetVolt) ) {

    if ( csCtrlStruct->arg1 >= 10000 || csCtrlStruct->arg2 >= 1000 ) {
      TCPRINTF(" ** Argument error!! : parameter is too large for setVolt().");
      rc = 9;
      goto END_CS_SPLIT_ARG;
    } else if ( csCtrlStruct->arg1 < 0 || csCtrlStruct->arg2 < 0 ) {
      TCPRINTF(" ** Argument error!! : parameter is too fewer for setVolt().");
      rc = 9;
      goto END_CS_SPLIT_ARG;
    }
    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  } else if ( !strcmp(csCtrlStruct->cmdName, csNameSetTemp) ) {

    if ( csCtrlStruct->arg1 > 100 || csCtrlStruct->arg1 < -100 ) {
      TCPRINTF(" ** Argument error!! : parameter is range over for setTemp().\n");
      rc = 9;
      goto END_CS_SPLIT_ARG;
    }
    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
  } else if ( !strcmp(csCtrlStruct->cmdName, csNameHoldMin) ) {

    if ( csCtrlStruct->arg1 > 100000 || csCtrlStruct->arg1 < 0 ) {
      TCPRINTF(" ** Argument error!! : parameter is range over for holdMin().\n");
      rc = 9;
      goto END_CS_SPLIT_ARG;
    }
  }

END_CS_SPLIT_ARG:

  TCPRINTF("Exit:%d", rc);
  return rc;
}

/*****************************************************************************/
int csExpandLoop(csCtrlStep *csCtrlStruct, const long MaxStructCell, long *stIdx)
/*****************************************************************************/
{
  const csCtrlStep csZeroStruct = { NULL };

  csCtrlStep bufStruct[MaxStructCell];

  int  rc = 0;
  int  srcIdx = 0;
  int  bufIdx = 0;
  int  startLoopIdx = 0;
  int  endLoopIdx = 0;
  int  backIdx = 0;
  int  cycleIdx = 0;
  int  rewriteIdx = -1;
  long loopCycle = 0;
  long clearIdx = 0;

  TCPRINTF("Entry: %p, %ld, %ld", csCtrlStruct, MaxStructCell, *stIdx );

  TCPRINTF("csLoopFlg = %d", csLoopFlg);
  if ( csLoopFlg == 0 ) {
    TCPRINTF(" -- Handling sequence file has no loop. go exit.");
    rc = 0;
    goto END_CS_EXPAND_LOOP;
  }

  if ( *stIdx > MaxStructCell ) {
    TCPRINTF(" ** Memory over!! : Step structure array cell is size over!");
    rc = 1;
    goto END_CS_EXPAND_LOOP;
  }

  // Zero clear for all structure
  for ( clearIdx = 0; clearIdx < MaxStructCell; clearIdx++ ) {
    bufStruct[clearIdx] = csZeroStruct;
  }

  // buffering and extend include loop contents.
  while ( MATCH != strcmp( csCtrlStruct[srcIdx].cmdName, csNameEndSequence ) ) { /* +++++++++++++++++++++++++++++++++++ */
    //for( bufIdx = 0; bufIdx < *stIdx; bufIdx++ ){

    if ( MATCH == strcmp( csCtrlStruct[srcIdx].cmdName, csNameSave ) ) {

      TCPRINTF(" -- find \"save\" function!! \n");
      /*
      if ( loopFlg > 0 ) {
        TCPRINTF(" ** Unexpected error occured! \n");
        rc = 2;
        goto END_CS_EXPAND_LOOP;
      }
      */
      loopCycle = csCtrlStruct[srcIdx].arg1;

      TCPRINTF(" -- loopcycle: %ld", loopCycle );
      if ( loopCycle > MaxStructCell ) {
        TCPRINTF(" ** Buffer over!! : too many loop!");
        rc = 2;
        goto END_CS_EXPAND_LOOP;
      }

      if ( 0 == loopCycle ) {
        rewriteIdx = bufIdx;
        TCPRINTF(" -- found 0 loop. skip function internal loop.");
      }

      //      insertIdx = bufIdx;
      srcIdx++; // forward to next including "save" structure.
      startLoopIdx = srcIdx;

      while ( MATCH != strcmp( csCtrlStruct[srcIdx].cmdName, csNameLoop ) ) {

        bufStruct[bufIdx] = csCtrlStruct[srcIdx];

        if ( srcIdx == *stIdx ) {
          TCPRINTF(" ** Syntax error!! : \"loop\" function was not called.");
          rc = 3;
          goto END_CS_EXPAND_LOOP;
        }
        srcIdx++;
        bufIdx++;
      }

      srcIdx++;
      endLoopIdx = srcIdx; /* this "endLoopIdx" is next including "loop" structure. */
      // srcIdx = startLoopIdx;
      cycleIdx = 1;

      while ( cycleIdx < loopCycle ) {

        for ( srcIdx = startLoopIdx; srcIdx < (endLoopIdx - 1); srcIdx++ ) {

          bufStruct[bufIdx] = csCtrlStruct[srcIdx];
          bufIdx++;
          if ( bufIdx > MaxStructCell ) {
            TCPRINTF(" ** Buffer over!! : too many loop cycle.");
            rc = 2;
            goto END_CS_EXPAND_LOOP;
          }
        }

        cycleIdx++;
      }

      srcIdx = endLoopIdx; /* this "srcIdx" position is next including "loop" structure.  */
      if ( loopCycle == 0 ) { // buffer index back to start loop function.
        bufIdx = rewriteIdx;
      }

    } else { /*** standard copy ***/

      if ( srcIdx == *stIdx ) {
        TCPRINTF(" ** Syntax error!! : not found function of endSequence");
        rc = 3;
        goto END_CS_EXPAND_LOOP;
      }

      bufStruct[bufIdx] = csCtrlStruct[srcIdx]; // buffering structure.
      bufIdx++;
      srcIdx++;

      if ( bufIdx > MaxStructCell ) {
        TCPRINTF(" ** Buffer over!! : too many loop cycle.");
        rc = 2;
        goto END_CS_EXPAND_LOOP;
      }
    }

  } /* +++++++++++++++++++++++++++++++++++++++++++++++ end while for "buffering and extend include loop contents." */

  bufStruct[bufIdx] = csCtrlStruct[srcIdx];
  bufIdx++; // endSequence word.

  *stIdx = bufIdx;

  /* ************************************************************************ */
#if 0 /* display execution for debug from bufferStructure. */
  TCPRINTF("* * *display from bufStruct* * *");

  for ( dispIdx = 0; dispIdx < *stIdx; dispIdx++ ) {

    bufStruct[dispIdx].function( bufStruct[dispIdx].arg1,
                                 bufStruct[dispIdx].arg2,
                                 bufStruct[dispIdx].arg3,
                                 bufStruct[dispIdx].string,
                                 bufStruct[dispIdx].length);

  }

#endif /* Disable -0- */
  /* ************************************************************************ */

  srcIdx = 0;
  //  Set back to source structure from buffer structure.
  for ( backIdx = 0; backIdx < *stIdx; backIdx++ ) {

    csCtrlStruct[srcIdx] = bufStruct[backIdx];
    srcIdx++;
  }
  for ( backIdx = 0; backIdx < srcIdx; backIdx++ ) {
    TCPRINTF("index: %d,cmdName: %s", backIdx, csCtrlStruct[backIdx].cmdName);
  }
END_CS_EXPAND_LOOP:
  TCPRINTF("Exit:%d", rc);
  return rc;
}


/*****************************************************************************/
int csConfigureResume( csCtrlStep *csCtrlStruct, const long stIdx, long *restartIdx, Dword *restartTime)
/*****************************************************************************/
{
  int rc = 0;
  int status = 0;
//  int resumeFlg = 0;
//  int resumeCell = 0;
  int searchIdx = 0;
  int cntIdx = 0;
//  int tmpIdx = 0;
  int prvIdx = 0;
  int setTempFlg = 0;

  long hour = 0, min = 0, sec = 0;
  long skipTime = 0;
  long countTime = 0;
  long adjustTime = 0;
  long totalTestTime = 0;
  Dword dwMinute = 0;
  Word  wIndex  = 0;
  Byte  tempResumeMode = 0;
  Byte  ret = 0;
  //  int errFlg = 0;

  TCPRINTF("Entry: %p, %ld, %ld", csCtrlStruct, stIdx, *restartIdx);

  *restartIdx = -99; // error value.

  // serch "resume" strings. not found, return 0;

  for ( searchIdx = 0; searchIdx < stIdx; searchIdx++ ) {
//
//    if ( !strcmp( csCtrlStruct[searchIdx].cmdName, csNameResume ) ) {
//      if (csCtrlStruct[searchIdx].arg1 | csCtrlStruct[searchIdx].arg2 | csCtrlStruct[searchIdx].arg3) {
//        resumeFlg++;
//        resumeCell = searchIdx;
//        for (tmpIdx = searchIdx;tmpIdx < MAX_CTRL_STEP;tmpIdx++) {
//          csCtrlStruct[tmpIdx] = csCtrlStruct[tmpIdx+1];
//        }
//      }
    /*  if( searchIdx != 0 ){ errFlg++; }  */
//    }

    if ( !strcmp( csCtrlStruct[searchIdx].cmdName, csNameHoldMin ) ) {
      totalTestTime += csCtrlStruct[searchIdx].arg1;
    }
  }

  // how many "resume" ? 1=OK, over 2 return 1;
  if ( resumeFlg != 1 ) { /* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

    if ( resumeFlg > 1 ) {
      rc = 1;
      TCPRINTF(" ** resume flag is more than 2!! %d ", resumeFlg);
      goto END_CS_CONFIGURE_RESUME;
    }
    if ( resumeFlg == 0 ) {
      TCPRINTF(" -- Sequence does not call \"resume\" function. It will start standard process.");
      rc = 0;
      *restartIdx = 0; // Disablement resume function.
      *restartTime = 0;
      dwChamberScriptJumpedTime = 0;
      goto END_CS_CONFIGURE_RESUME;
    }

  } else { // resumeCountFlg == 1
///////// insert y.katayama 2010.10.28 //////////////////////////

    if (tccb.optionSyncManagerDebug) {
      ret = csReadResumeIdx(&dwMinute, &wIndex, &tempResumeMode);
      TCPRINTF(" ** read resume index minute: %d index: %d mode: %d ", (int)dwMinute, (int)wIndex, (int)tempResumeMode);
      if (ret) {
        rc = 1;
        TCPRINTF(" ** read resume index error!!");
        goto END_CS_CONFIGURE_RESUME;
      }
      hour = (long)dwMinute / 60;
      min  = (long)dwMinute % 60;
      sec  = 0;
      dwChamberScriptJumpedTime = dwMinute;
    } else {
//     hour = csCtrlStruct[resumeCell].arg1;
//     min = csCtrlStruct[resumeCell].arg2;
//     sec = csCtrlStruct[resumeCell].arg3; // now version is not supported.
      if ( min >= 60 || sec >= 60 ) { // range parameter check.
        TCPRINTF(" ** Argument error!! : \"resume\" function argument is over range. min > %ld, sec > %ld", min, sec);
        rc = 2;
        goto END_CS_CONFIGURE_RESUME;
      }

      if ( hour == 0 && min == 0 && sec == 0 ) { // if all argument is 0, "resume" function is ignored.

        TCPRINTF(" ++ resume function was disabled.");
        *restartIdx = 0; // Disablement resume function.
        *restartTime = 0;
        rc = 0;
        goto END_CS_CONFIGURE_RESUME;
      }
    }
  } /* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Endif for resumeCountflg */

  /* <> <> <> check point for resume function <> <> <> */
  TCPRINTF(" ++ resume function has no error.");

  // calculate resume start time. Supported only "min" now.
  skipTime = ( hour * 60 ) + min;
  TCPRINTF(" -- Total test time : %ld[min] / Resume start time %ld[min] ( Sec no supported )", totalTestTime, skipTime);
  status = totalTestTime - skipTime;
  if ( status < 1 ) {
    TCPRINTF(" ** System error!! : resume time is longer than all test time.");
    rc = 5;
    goto END_CS_CONFIGURE_RESUME;
  }

  // search "holdMin" and find restart point.
  if (tccb.optionSyncManagerDebug) {
    *restartIdx = wIndex; // Disablement resume function.
    *restartTime = dwMinute;
//      cntIdx = wIndex-1;
    cntIdx = wIndex;
//      if ( strcmp(csCtrlStruct[wIndex].cmdName, csNameRestart)==MATCH ) { // in match case, entry process.
    if ( strcmp(csCtrlStruct[cntIdx].cmdName, csNameRestart) == MATCH ) { // in match case, entry process.
//     if(tempResumeMode!=csCtrlStruct[wIndex].arg2){
      if (tempResumeMode != csCtrlStruct[cntIdx].arg2) {
//           TCPRINTF(" ** sequence error!! : different temp resume mode is read in resume test %d,%d",(int)csCtrlStruct[wIndex].arg2,(int)tempResumeMode);
        TCPRINTF(" ** sequence error!! : different temp resume mode is read in resume test %d,%d", (int)csCtrlStruct[cntIdx].arg2, (int)tempResumeMode);
        rc = 6;
        goto END_CS_CONFIGURE_RESUME;
      }
//     if(csCtrlStruct[wIndex].arg2==0){      //// NO TEMP RESUME //////
      if (csCtrlStruct[cntIdx].arg2 == 0) {  //// NO TEMP RESUME //////
        TCPRINTF(" ++ Complete resume setup!! ");
        rc = 0;
        goto END_CS_CONFIGURE_RESUME;
      }
    } else {
//           TCPRINTF(" ** sequence error!! : start index is not restart command %s",csCtrlStruct[wIndex].cmdName);
      TCPRINTF(" ** sequence error!! : start index is not restart command %s", csCtrlStruct[cntIdx].cmdName);
      rc = 6;
      goto END_CS_CONFIGURE_RESUME;

    }
  } else {
    for ( cntIdx = 0; cntIdx < stIdx; cntIdx++ ) { /** <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> **/

      if ( !strcmp(csCtrlStruct[cntIdx].cmdName, csNameHoldMin) ) { // in match case, entry process.

        countTime  = countTime + csCtrlStruct[cntIdx].arg1;
        adjustTime = countTime - skipTime;
        if ( adjustTime >= 0 ) {
          if ( adjustTime < 1 ) {
            adjustTime = 0;
            *restartIdx = cntIdx + 1;
            break;
          } else {
            csCtrlStruct[cntIdx].arg1 = adjustTime;
            *restartIdx = cntIdx;
            break;
          }
        } else {
          continue;
        }
      }
    }
  } /** <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> <> end of FOR loop for search holdMin & restart point. **/

  TCPRINTF(" -- resume setup : restart cell > %ld, adjusted cell time > %ld", *restartIdx, adjustTime);

  for ( prvIdx = cntIdx; prvIdx >= 0; prvIdx-- ) { /* # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # */

    if ( !strcmp(csCtrlStruct[prvIdx].cmdName, csNameSetTemp) ) {

      TCPRINTF(" -- found \"setTemp\" function before restart point.");
      TCPRINTF(" -- setTemp execute & wait start.");
      status = csCtrlStruct[prvIdx].function( csCtrlStruct[prvIdx].arg1,
                                              csCtrlStruct[prvIdx].arg2,
                                              csCtrlStruct[prvIdx].arg3,
                                              csCtrlStruct[prvIdx].string,
                                              csCtrlStruct[prvIdx].length  );
      if ( status > 0 ) {
        TCPRINTF(" ** Control error!!: setTemp Function return error > %d ", status);
        rc = status;
        goto END_CS_CONFIGURE_RESUME;
      }

      status = csWaitUntilTemperatureOnTarget(); // dummy function now. [100612]
      if ( status > 0 ) {
        rc = status;
        goto END_CS_CONFIGURE_RESUME;
      }

      setTempFlg = 1;
      break;
    }

  } /* # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # end of FOR loop for search setTemp & exec/wait */

  TCPRINTF(" ++ Complete resume setup!! ");
  rc = 0;

END_CS_CONFIGURE_RESUME:
  TCPRINTF("Exit: %d", rc);
  return rc;
}
