#ifndef _TC_H_
#define _TC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <stdarg.h>
#include <memory.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>  // for sleep
#include <execinfo.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "tcTypes.h"
#include "../slotio/libsi.h"
#include "../signalhandler/libsh.h"
#include "../chamberscript/libcs.h"
#include "../testscript/libts.h"
#include "../testscript/ts.h"
#include "../syncmanager/libsm.h"
#include "../versioncontrol/libvc.h"   // 2011.06.14 Satoshi Takahashi

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * <pre>
   * Description:
   *   Parse config file to find test location index
   * Arguments:
   *   config[] - INPUT - config file image
   *   gtcb - OUTPUT - generic thread control block
   * Return:
   *   zero if no error. otherwise, error
   * </pre>
   *****************************************************************************/
  Byte tcParseTestLocationIndex(const char config[], GTCB *gtcb);

  void tcQuickEnv(const char *param);


  void tcCleanUpCell(const char *param);


  void tcLogFilter(char *filename);
  void putBinaryDataDumpOfPrintf(Byte *dwPhysicalAddress, Dword dwLogicalAddress, Dword dwSize);

  void tcTestcaseInZip(void);
  void tcPrintfFileInZip(void);

  void tcInitCaller();
  const char *tcGetCaller(unsigned int level);
  void tcDumpCaller(void);

  void tcDumpIpAddress(void);
  void tcAddResumeInfomation(char *sp);
  Byte tcWaitCSReady(void);

#ifdef __cplusplus
}
#endif

#endif
