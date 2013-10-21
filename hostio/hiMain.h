/*
 * File:   hiMain.h
 * Author: Nishimura
 *
 * Created on 2010/05/17, 22:56
 */

#ifndef _HIMAIN_H
#define _HIMAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../testcase/tcTypes.h"
#include "../testscript/ts.h"

//** For mutex ***************************************************************
//extern int hi_is_initialize_mutex = 0;  //After initialize mutex, this value should be 1.
//static pthread_mutex_t  hi_qput_mutex = PTHREAD_MUTEX_INITIALIZER;
//******************************************************************************

  void hiCreateHeader(Byte *header, Byte *type, Byte *fileName, Byte cell_num, Byte* dataLenStr);
  void hiSend(Byte* header, Byte* data, Dword wLgth);
  int hiReceiveHeader(int cellNum, Byte *header);
  int hiReceiveData (int cellNum, Byte *data, Word dataLen);
  void hiSendCMDPkt(ZQUE *zque);
  void hiSendFILEPkt(ZDAT *zdata);
  void* analysisHeader(int cell_num, Byte *header);
  unsigned int getRandom(int min, unsigned int max);


#ifdef __cplusplus
}
#endif

#endif /* _HIMAIN_H */

