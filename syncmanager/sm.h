#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>  // for sleep

#include "../testcase/tcTypes.h"
#include "../slotio/libsi.h"
#include "../testscript/libts.h"
#include "../chamberscript/libcs.h"
#include "libsm.h"
#define  MATCH 0

/**
 * <pre>
 * Description:
 *   To check time out
 * Arguments:
 *   initialTime - INPUT - begin time for time measurement
 *   time - INPUT - time out period by sec
 * Return:
 *   0:not time out
 *   1:time out
 * Note:
 * </pre>
 *****************************************************************************/
int smCheckTimeOut(time_t initialTime, int time);

/**
 * <pre>
 * Description:
 *   To initialize timer
 * Arguments:
 * Return: current time
 * Note:
 * </pre>
 *****************************************************************************/
time_t smInitializeTimer(void);

/**
 * <pre>
 * Description:
 *   To send message from syncmanager
 * Arguments:
 *   *smBuffer - INPUT - pointer to sync buffer
 *   *smMessage - INPUT - message to be sent
 * Return:
 *   zero if no error. otherwise, error
 * Note:
 *   this is non-blocking function that does not wait until the buffer is not full
 * </pre>
 *****************************************************************************/
Byte smSendSyncMessageForSM(SM_BUFFER *smBuffer, SM_MESSAGE *smMessage, Dword timeout);
/**
 * <pre>
 * Description:
 *   To receive to syncmanager
 * Arguments:
 *   *smBuffer - INPUT - pointer to sync buffer
 *   *smMessage - INPUT - message to be sent
 * Return:
 *   zero if no error. otherwise, error
 * Note:
 *   this is non-blocking function that does not wait until the buffer is not full
 * </pre>
 *****************************************************************************/
Byte smReceiveSyncMessageForSM(SM_BUFFER *smBuffer, SM_MESSAGE *smMessage);
