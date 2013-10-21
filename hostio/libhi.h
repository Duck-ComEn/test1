#ifndef _LIBHI_H_
#define _LIBHI_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../testcase/tcTypes.h"

  /**
   * <pre>
   * Description:
   *   Gut queue from host PC
   * Arguments:
   *   cell_num  // TODO Need cofirmation how to get cell num.
   * Return:
   *   pointer to queue in ZQUE or ZDAT format
   * Note:
   *   read FIFO (named pipe) to make queue and return to caller.
   *   this function alloc queue, and caller should free.
   * </pre>
   *****************************************************************************/
  void* hi_qget(Byte cell_num);

  /**
   * <pre>
   * Description:
   *   Put queue to host PC
   * Arguments:
   *   ptr - INPUT - pointer to queue in ZQUE or ZDAT format
   * Return:
   *   None
   * Note:
   *   parse queue and make packet to send to host PC via FIFO (named pipe)
   *   this function free memory of a given queue
   * </pre>
   *****************************************************************************/
  void hi_qput(void* ptr);

  /**
   * <pre>
   * Description:
   *   Response queue check
   * Arguments:
   *   None
   * Return:
   *   zero if queue is not available. otherwise, queue is available
   * Note:
   *   hi_qget to get queue pointer
   * </pre>
   *****************************************************************************/
  Byte hi_qis();

#ifdef __cplusplus
}
#endif

#endif
