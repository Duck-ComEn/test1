#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "cs.h"


int csUnitTest(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  TCPRINTF("Exit");

  return 0;
}

/*
 * unit test for release first version. [100624]
 */

int csUnitTest_chkHoldMin1(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    setTemp(57,1);\n\
    holdMin(0);\n\
    setVolt(0,0); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_chkHoldMin2(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    setTemp(57,1);\n\
    holdMin(1);\n\
    setVolt(0,0); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_chkHoldMin3(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    setTemp(57,1);\n\
    holdMin(-1);\n\
    setVolt(0,0); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_chkHoldMin4(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    setTemp(57,1);\n\
    holdMin(100000);\n\
    setVolt(0,0); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_chkHoldMin5(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    setTemp(57,1);\n\
    holdMin(100001);\n\
    setVolt(0,0); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_chkLoopTest1(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    setTemp(57,1);\n\
    save(1);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,0); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_chkLoopTest2(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    setTemp(57,1);\n\
    save(0);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,0); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_chkLoopTest3(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    setTemp(57,1);\n\
    save(-1);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,0); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_chkLoopTest4(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    setTemp(57,1);\n\
    save(1000000);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,0); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

/*
 *
 */

int csUnitTest_resumeOK1(void)
{
  int rc = 0;
  char testseq1[] = "\
  elapsed(000,32,11);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_resumeOK2(void)
{
  int rc = 0;
  char testseq1[] = "\
  elapsed(0002,32,11);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_resumeOK3(void)
{
  int rc = 0;
  char testseq1[] = "\
  elapsed(0018,32,29);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_resumeZeroOK(void)
{
  int rc = 0;
  char testseq1[] = "\
  elapsed(0000,00,00);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

/*
 *  check resume time.
 */

int csUnitTest_chkResume1(void)
{ // 1hour just.
  int rc = 0;
  char testseq1[] = "\
  elapsed(0001,00,00);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_chkResume2(void)
{
  int rc = 0;
  char testseq1[] = "\
  elapsed(0000,59,59);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_chkResume3(void)
{
  int rc = 0;
  char testseq1[] = "\
  elapsed(0003,40,00);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

int csUnitTest_chkResume4(void)
{ // total time over restart time.
  int rc = 0;
  char testseq1[] = "\
  elapsed(0020,35,30);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_chkResume5(void)
{
  int rc = 0;
  char testseq1[] = "\
  elapsed(0002,01,30);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 == rc);

  return 0;
}

/*
 *  resume error.
 */

int csUnitTest_erResume1(void)
{ // over time.
  int rc = 0;
  char testseq1[] = "\
  elapsed(0021,30,00);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erResume2(void)
{ // over range min.
  int rc = 0;
  char testseq1[] = "\
  elapsed(0001,60,00);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erResume3(void)
{ // over time.
  int rc = 0;
  char testseq1[] = "\
  elapsed(0011,30,60);\n\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}


/* 6patterns OK.
 * missing parameter.
 */

int csUnitTest_erParam1(void)
{ // minus loop
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(-16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erParam2(void)
{ // over voltage
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,50000); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erParam3(void)
{ // minus over voltage
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,50000); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,-500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erParam4(void)
{ // over temperature
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(570,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erParam5(void)
{ // minus over temperature
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(-16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(,0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(-125,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erParam6(void)
{ // over wait time
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,50000); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,-500); holdMin(200000);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erParam7(void)
{ // minus wait time
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,50000); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,-500); holdMin(-25);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

/*
 * missing punctuation character.
 */

int csUnitTest_erSeparator1(void)
{ // double left bracket.
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart((0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();";
  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erSeparator2(void)
{ // double semi-colon.
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);;\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erSeparator3(void)
{ // without right bracket.
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500; holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erSeparator4(void)
{ // double right bracket.
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500)); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erSeparator5(void)
{ // without left bracket.
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop);\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erSeparator6(void)
{ // without right bracket in case of command function.
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA;holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

/*  all 3types error.
 *  testing camma error at argument line.
 */

int csUnitTest_erCamma1(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(,0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

// 2 of 3 as camma
int csUnitTest_erCamma2(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

// 3 of 3 as camma
int csUnitTest_erCamma3(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0,);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");
  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

/* 2 test units. OK
 * Huge file size & 0 file size.
 */

int csUnitTest_erHugeInputFile(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0,);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], (96 * 1024) );
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erZeroLengthFile(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0,);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], 0 );
  assert(expected1 != rc);

  return 0;
}


/* OK
 * Address check & insert binary file.
 */

int csUnitTest_erMissAddress(void)
{
  int rc = 0;
  int expected1 = 0;
  char *p = (char *)1;


  //pp = &p;

  TCPRINTF("Entry");

  rc = csInterpreter(p, 1);
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erBinFile(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\b\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop(\a);\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA);holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

/*
 * check for EOF character
 */

int csUnitTest_EOF1(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA); holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  testseq1[strlen(testseq1) - 1] = 0x1A;
  rc = csInterpreter( testseq1, strlen(testseq1) );

  assert(expected1 == rc);

  return 0;
}

int csUnitTest_EOF2(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA); holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n   ";

  int expected1 = 0;

  TCPRINTF("Entry");

  testseq1[strlen(testseq1) - 1] = 0x1A;
  rc = csInterpreter( testseq1, strlen(testseq1) );

  assert(expected1 == rc);

  return 0;
}

int csUnitTest_EOF3(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA); holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n\n\n\
\
  ";

  int expected1 = 0;

  TCPRINTF("Entry");

  testseq1[strlen(testseq1) - 1] = 0x1A;
  rc = csInterpreter( testseq1, strlen(testseq1) );

  assert(expected1 == rc);

  return 0;
}

int csUnitTest_erEOF1(void)
{ // include EOF sequence file internal.
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA); holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  testseq1[ strlen(testseq1)/2 ] = 0x1A;
  rc = csInterpreter( testseq1, strlen(testseq1) );

  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erEOF2(void)
{
  int rc = 0;
  char testseq1[] = "\
  sequence(0x98);\n\
    saveInterval(0);\n\
    restart(0,0,0);\n\
    setVolt(0,500); holdMin(220);\n\
    setTemp(57,1);\n\
    save(16);\n\
      restart(4,1,0);\n\
      setVolt(0,500); holdMin(60);\n\
    loop();\n\
    setVolt(0,500); holdMin(10);\n\
    command(#PORTPREPDATA); holdMin(40);\n\
    setVolt(0,0); holdMin(5); setTemp(25,1);\n\
  endSequence();\n";

  int expected1 = 0;

  TCPRINTF("Entry");

  testseq1[strlen(testseq1) / 3] = 0x1A;
  rc = csInterpreter( testseq1, strlen(testseq1) );

  assert(expected1 != rc);

  return 0;
}


/*  OK
 *  error: change function name
 */

int csUnitTest_erFunction1(void)
{
  int rc = 0;
  char testseq1[] =
    "sequence(0x98);\
     saveInterval(0);\
     Restart(0,0,0);\
     setVolt(0,500); holdMin(220);\
     setTemp(57,1);\
     save(16);\
      Restart(4,1,0);\
      setVolt(0,500); holdMin(60);\
     loop();\
     setVolt(0,500); holdMin(10);\
     command(#PORTPREPDATA);holdMin(40);\
     setVolt(0,0); holdMin(5); setTemp(25,1);\
    endSequence();";
  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erFunction2(void)
{
  int rc = 0;
  char testseq1[] =
    "sequence(0x98);\
     saveInterval(0);\
     restart(0,0,0);\
     setVolt(0,500); holdMin(220);\
     setTemp(57,1);\
     save(16);\
      restart(4,1,0);\
      setVolt(0,500); holdmin(60);\
     loop();\
     setVolt(0,500); holdMin(10);\
     command(#PORTPREPDATA);holdMin(40);\
     setVolt(0,0); holdMin(5); setTemp(25,1);\
    endsequence();";
  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erFunction3(void)
{
  int rc = 0;
  char testseq1[] =
    "sequence(0x98);\
     saveInterval(0);\
     restart(0,0,0);\
     setVolt(0,500); holdMin(220);\
     setTemp(57,1);\
     save(16);\
      restart(4,1,0);\
      setVolt(0,500); holdMin(60);\
     loop();\
     setVolt(0,500); holdMin(10);\
     command(#PORTPREPDATA);holdMin(40);\
     setVolt(0,0); holdMin(5); setTemp(25,1);\
    endsequence();";
  int expected1 = 0;


  TCPRINTF("Entry");
  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erFunction4(void)
{
  int rc = 0;
  char testseq1[] =
    "sequence(0x98);\
     saveInterval(0);\
     restart(0,0,0);\
     setVolt(0,500); ho/ldMin(220);\
     se-tTemp(57,1);\
     save(16);\
      restart(4,1,0);\
      setVolt(0,500); holdMin(60);\
     loop();\
     set+Volt(0,500); holdMin(10);\
     command(#PORTPREPDATA);holdMin(40);\
     setVolt(0,0); holdMin(5); setTemp(25,1);\
    endsequence();";
  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}

int csUnitTest_erFunction5(void)
{
  int rc = 0;
  char testseq1[] =
    "sequence(0x98);\
     saveInterval(0);\
     restart(0,0,0);\
     setVolt(0,500); holdMin(220);\
     setTemp(57,1);\
     save(16);\
      restart(4,1,0);\
      setVolt(0,500); holdMin(60);\
     loop();\
     set+Volt(0,500); holdMin(10);\
     command(#PORTPREPDATA);holdMin(40);\
     setVolt(0,0); holdMin(5); setTemp(25,1);\
    endsequence();";
  int expected1 = 0;

  TCPRINTF("Entry");

  rc = csInterpreter(&testseq1[0], strlen(&testseq1[0]));
  assert(expected1 != rc);

  return 0;
}
